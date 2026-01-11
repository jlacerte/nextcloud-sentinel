/*
 * Nextcloud Sentinel Edition - Kill Switch Dashboard Widget Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "killswitchdashboard.h"
#include "ui_killswitchdashboard.h"

#include "libsync/killswitch/killswitchmanager.h"
#include "libsync/configfile.h"

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QLoggingCategory>
#include <QFileDialog>
#include <QTextStream>

namespace OCC {

Q_LOGGING_CATEGORY(lcKillSwitchDashboard, "nextcloud.gui.killswitch.dashboard", QtInfoMsg)

KillSwitchDashboard::KillSwitchDashboard(QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::KillSwitchDashboard)
{
    _ui->setupUi(this);

    setupConnections();
    loadStats();
    updateDisplay();

    qCInfo(lcKillSwitchDashboard) << "Kill Switch dashboard initialized";
}

KillSwitchDashboard::~KillSwitchDashboard()
{
    saveStats();
    delete _ui;
}

QSize KillSwitchDashboard::sizeHint() const
{
    return {400, 500};
}

void KillSwitchDashboard::setupConnections()
{
    // Time range selection
    connect(_ui->timeRangeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KillSwitchDashboard::slotTimeRangeChanged);

    // Export button
    connect(_ui->exportButton, &QPushButton::clicked,
            this, &KillSwitchDashboard::slotExportStats);

    // Refresh button
    connect(_ui->refreshButton, &QPushButton::clicked,
            this, &KillSwitchDashboard::refreshStats);

    // Connect to KillSwitchManager if available
    if (auto *manager = KillSwitchManager::instance()) {
        connect(manager, &KillSwitchManager::threatDetected,
                this, &KillSwitchDashboard::slotThreatDetected);
    }
}

void KillSwitchDashboard::loadStats()
{
    auto settings = ConfigFile::settingsWithGroup(QStringLiteral("KillSwitchStats"));

    _stats.filesAnalyzed = settings->value(QStringLiteral("filesAnalyzed"), 0).toLongLong();
    _stats.threatsBlocked = settings->value(QStringLiteral("threatsBlocked"), 0).toLongLong();

    // Load detector triggers
    QStringList detectors = settings->value(QStringLiteral("detectorNames")).toStringList();
    QList<QVariant> counts = settings->value(QStringLiteral("detectorCounts")).toList();
    for (int i = 0; i < detectors.size() && i < counts.size(); ++i) {
        _stats.detectorTriggers[detectors[i]] = counts[i].toInt();
    }

    // Load recent threats (last 100)
    QStringList threatTimes = settings->value(QStringLiteral("threatTimes")).toStringList();
    QStringList threatDetectors = settings->value(QStringLiteral("threatDetectors")).toStringList();
    for (int i = 0; i < threatTimes.size() && i < threatDetectors.size(); ++i) {
        _stats.recentThreats.append({
            QDateTime::fromString(threatTimes[i], Qt::ISODate),
            threatDetectors[i]
        });
    }
}

void KillSwitchDashboard::saveStats()
{
    auto settings = ConfigFile::settingsWithGroup(QStringLiteral("KillSwitchStats"));

    settings->setValue(QStringLiteral("filesAnalyzed"), _stats.filesAnalyzed);
    settings->setValue(QStringLiteral("threatsBlocked"), _stats.threatsBlocked);

    // Save detector triggers
    QStringList detectors;
    QList<QVariant> counts;
    for (auto it = _stats.detectorTriggers.begin(); it != _stats.detectorTriggers.end(); ++it) {
        detectors.append(it.key());
        counts.append(it.value());
    }
    settings->setValue(QStringLiteral("detectorNames"), detectors);
    settings->setValue(QStringLiteral("detectorCounts"), counts);

    // Save recent threats (limit to 100)
    QStringList threatTimes;
    QStringList threatDetectors;
    int startIdx = qMax(0, _stats.recentThreats.size() - 100);
    for (int i = startIdx; i < _stats.recentThreats.size(); ++i) {
        threatTimes.append(_stats.recentThreats[i].first.toString(Qt::ISODate));
        threatDetectors.append(_stats.recentThreats[i].second);
    }
    settings->setValue(QStringLiteral("threatTimes"), threatTimes);
    settings->setValue(QStringLiteral("threatDetectors"), threatDetectors);
}

void KillSwitchDashboard::refreshStats()
{
    // Reload from KillSwitchManager
    if (auto *manager = KillSwitchManager::instance()) {
        const auto threats = manager->threats();
        for (const auto &threat : threats) {
            // Check if this threat is already counted
            bool found = false;
            for (const auto &rt : _stats.recentThreats) {
                if (rt.first == threat.timestamp && rt.second == threat.detectorName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                _stats.recentThreats.append({threat.timestamp, threat.detectorName});
                _stats.detectorTriggers[threat.detectorName]++;
                _stats.threatsBlocked++;
            }
        }
    }

    updateDisplay();
    saveStats();

    qCInfo(lcKillSwitchDashboard) << "Stats refreshed";
}

void KillSwitchDashboard::slotThreatDetected(const ThreatInfo &threat)
{
    _stats.threatsBlocked++;
    _stats.detectorTriggers[threat.detectorName]++;
    _stats.recentThreats.append({threat.timestamp, threat.detectorName});

    // Trim old threats (keep last 1000)
    while (_stats.recentThreats.size() > 1000) {
        _stats.recentThreats.removeFirst();
    }

    updateDisplay();
    saveStats();
}

void KillSwitchDashboard::slotTimeRangeChanged(int index)
{
    switch (index) {
    case 0:
        _timeRangeDays = 1; // 24h
        break;
    case 1:
        _timeRangeDays = 7; // Week
        break;
    case 2:
        _timeRangeDays = 30; // Month
        break;
    default:
        _timeRangeDays = 1;
    }

    updateTimeline();
}

void KillSwitchDashboard::slotExportStats()
{
    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Export Kill Switch Statistics"),
        QDir::homePath() + QStringLiteral("/killswitch_stats.csv"),
        tr("CSV Files (*.csv);;All Files (*)")
    );

    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcKillSwitchDashboard) << "Failed to open file for export:" << filename;
        return;
    }

    QTextStream out(&file);
    out << "Nextcloud Sentinel - Kill Switch Statistics\n";
    out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "Files Analyzed," << _stats.filesAnalyzed << "\n";
    out << "Threats Blocked," << _stats.threatsBlocked << "\n\n";

    out << "Detector,Triggers\n";
    for (auto it = _stats.detectorTriggers.begin(); it != _stats.detectorTriggers.end(); ++it) {
        out << it.key() << "," << it.value() << "\n";
    }

    out << "\nRecent Threats\n";
    out << "Timestamp,Detector\n";
    for (const auto &threat : _stats.recentThreats) {
        out << threat.first.toString(Qt::ISODate) << "," << threat.second << "\n";
    }

    file.close();
    qCInfo(lcKillSwitchDashboard) << "Stats exported to:" << filename;
}

void KillSwitchDashboard::updateDisplay()
{
    // Update counters
    _ui->filesAnalyzedLabel->setText(QString::number(_stats.filesAnalyzed));
    _ui->threatsBlockedLabel->setText(QString::number(_stats.threatsBlocked));

    updateDetectorRanking();
    updateTimeline();
}

void KillSwitchDashboard::updateDetectorRanking()
{
    _ui->detectorsListWidget->clear();

    // Sort detectors by trigger count
    QVector<QPair<QString, int>> sorted;
    for (auto it = _stats.detectorTriggers.begin(); it != _stats.detectorTriggers.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    // Display top 5
    int displayed = 0;
    for (const auto &entry : sorted) {
        if (displayed >= 5) break;
        QString text = QStringLiteral("%1. %2 (%3 triggers)")
                           .arg(displayed + 1)
                           .arg(entry.first)
                           .arg(entry.second);
        _ui->detectorsListWidget->addItem(text);
        displayed++;
    }

    if (sorted.isEmpty()) {
        _ui->detectorsListWidget->addItem(tr("No threats detected yet"));
    }
}

void KillSwitchDashboard::updateTimeline()
{
    _ui->timelineListWidget->clear();

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-_timeRangeDays);

    // Filter threats by time range
    QVector<QPair<QDateTime, QString>> filtered;
    for (const auto &threat : _stats.recentThreats) {
        if (threat.first >= cutoff) {
            filtered.append(threat);
        }
    }

    // Update count label
    QString rangeText;
    switch (_timeRangeDays) {
    case 1:
        rangeText = tr("Last 24 hours");
        break;
    case 7:
        rangeText = tr("Last 7 days");
        break;
    case 30:
        rangeText = tr("Last 30 days");
        break;
    }
    _ui->timelineCountLabel->setText(
        tr("%1: %2 threats").arg(rangeText).arg(filtered.size()));

    // Display recent threats (newest first)
    for (int i = filtered.size() - 1; i >= 0 && i >= filtered.size() - 20; --i) {
        QString text = QStringLiteral("[%1] %2")
                           .arg(filtered[i].first.toString("yyyy-MM-dd hh:mm:ss"))
                           .arg(filtered[i].second);
        _ui->timelineListWidget->addItem(text);
    }

    if (filtered.isEmpty()) {
        _ui->timelineListWidget->addItem(tr("No threats in this period"));
    }
}

} // namespace OCC

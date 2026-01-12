/*
 * Nextcloud Sentinel Edition - Threat History Dialog
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "threathistorydialog.h"
#include "ui_threathistorydialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStyle>
#include <QLoggingCategory>

namespace OCC {

Q_LOGGING_CATEGORY(lcThreatHistoryDialog, "nextcloud.gui.killswitch.threathistory", QtInfoMsg)

ThreatHistoryDialog::ThreatHistoryDialog(QWidget *parent)
    : QDialog(parent)
    , _ui(new Ui::ThreatHistoryDialog)
{
    _ui->setupUi(this);

    // Setup period combo box
    _ui->periodComboBox->addItem(tr("Last 24 hours"), 1);
    _ui->periodComboBox->addItem(tr("Last 7 days"), 7);
    _ui->periodComboBox->addItem(tr("Last 30 days"), 30);
    _ui->periodComboBox->addItem(tr("All time"), 0);
    _ui->periodComboBox->setCurrentIndex(0);

    // Setup table headers
    _ui->threatTable->setColumnCount(4);
    _ui->threatTable->setHorizontalHeaderLabels({
        tr("Level"),
        tr("Time"),
        tr("Detector"),
        tr("Description")
    });
    _ui->threatTable->horizontalHeader()->setStretchLastSection(true);
    _ui->threatTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _ui->threatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _ui->threatTable->setAlternatingRowColors(true);

    // Connect signals
    connect(_ui->periodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThreatHistoryDialog::onPeriodChanged);
    connect(_ui->exportButton, &QPushButton::clicked,
            this, &ThreatHistoryDialog::onExportClicked);
    connect(_ui->clearButton, &QPushButton::clicked,
            this, &ThreatHistoryDialog::onClearClicked);
    connect(_ui->threatTable, &QTableWidget::cellDoubleClicked,
            this, &ThreatHistoryDialog::onThreatDoubleClicked);
    connect(_ui->closeButton, &QPushButton::clicked,
            this, &QDialog::accept);

    // Initial load
    loadThreats();

    qCInfo(lcThreatHistoryDialog) << "Threat history dialog opened";
}

ThreatHistoryDialog::~ThreatHistoryDialog()
{
    delete _ui;
}

void ThreatHistoryDialog::onPeriodChanged(int index)
{
    _currentDays = _ui->periodComboBox->itemData(index).toInt();
    loadThreats();
}

void ThreatHistoryDialog::loadThreats()
{
    ThreatLogger *logger = ThreatLogger::instance();

    if (_currentDays == 0) {
        _currentThreats = logger->loadThreats();
    } else {
        _currentThreats = logger->threatsFromLastDays(_currentDays);
    }

    populateThreatList(_currentThreats);
    updateStatistics();
}

void ThreatHistoryDialog::populateThreatList(const QVector<ThreatInfo> &threats)
{
    _ui->threatTable->setRowCount(0);
    _ui->threatTable->setRowCount(threats.size());

    for (int i = 0; i < threats.size(); ++i) {
        const ThreatInfo &threat = threats[i];

        // Level column with icon and color
        auto *levelItem = new QTableWidgetItem(formatThreatLevel(threat.level));
        levelItem->setIcon(threatLevelIcon(threat.level));
        levelItem->setData(Qt::UserRole, static_cast<int>(threat.level));
        _ui->threatTable->setItem(i, 0, levelItem);

        // Timestamp column
        auto *timeItem = new QTableWidgetItem(formatTimestamp(threat.timestamp));
        _ui->threatTable->setItem(i, 1, timeItem);

        // Detector column
        auto *detectorItem = new QTableWidgetItem(threat.detectorName);
        _ui->threatTable->setItem(i, 2, detectorItem);

        // Description column
        auto *descItem = new QTableWidgetItem(threat.description);
        descItem->setToolTip(threat.affectedFiles.join(QStringLiteral("\n")));
        _ui->threatTable->setItem(i, 3, descItem);

        // Apply row styling based on threat level
        QColor bgColor;
        switch (threat.level) {
        case ThreatLevel::Critical:
            bgColor = QColor(255, 200, 200); // Light red
            break;
        case ThreatLevel::High:
            bgColor = QColor(255, 220, 180); // Light orange
            break;
        case ThreatLevel::Medium:
            bgColor = QColor(255, 255, 180); // Light yellow
            break;
        case ThreatLevel::Low:
            bgColor = QColor(200, 220, 255); // Light blue
            break;
        default:
            bgColor = Qt::white;
        }

        for (int col = 0; col < 4; ++col) {
            if (auto *item = _ui->threatTable->item(i, col)) {
                item->setBackground(bgColor);
            }
        }
    }

    _ui->threatTable->resizeColumnsToContents();

    if (threats.isEmpty()) {
        _ui->threatTable->setRowCount(1);
        auto *emptyItem = new QTableWidgetItem(tr("No threats detected in this period"));
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setTextAlignment(Qt::AlignCenter);
        _ui->threatTable->setItem(0, 0, emptyItem);
        _ui->threatTable->setSpan(0, 0, 1, 4);
    }
}

void ThreatHistoryDialog::updateStatistics()
{
    ThreatLogger::Statistics stats = ThreatLogger::instance()->getStatistics();

    QString statsText = tr("%1 threats | %2 Critical | %3 High | %4 Medium | %5 Low")
        .arg(_currentThreats.size())
        .arg(stats.criticalCount)
        .arg(stats.highCount)
        .arg(stats.mediumCount)
        .arg(stats.lowCount);

    _ui->statsLabel->setText(statsText);
}

void ThreatHistoryDialog::onExportClicked()
{
    QString defaultName = QStringLiteral("sentinel-threats-%1.csv")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd")));

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export Threat History"),
        defaultName,
        tr("CSV Files (*.csv)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    if (ThreatLogger::instance()->exportToCsv(filePath)) {
        QMessageBox::information(
            this,
            tr("Export Successful"),
            tr("Threat history exported to:\n%1").arg(filePath)
        );
        qCInfo(lcThreatHistoryDialog) << "Exported threats to:" << filePath;
    } else {
        QMessageBox::warning(
            this,
            tr("Export Failed"),
            tr("Could not export threat history to:\n%1").arg(filePath)
        );
        qCWarning(lcThreatHistoryDialog) << "Failed to export threats to:" << filePath;
    }
}

void ThreatHistoryDialog::onClearClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Clear Threat History"),
        tr("Are you sure you want to clear all threat history?\n\n"
           "This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        ThreatLogger::instance()->clearLog();
        loadThreats();
        qCInfo(lcThreatHistoryDialog) << "Threat history cleared by user";
    }
}

void ThreatHistoryDialog::onThreatDoubleClicked(int row)
{
    if (row < 0 || row >= _currentThreats.size()) {
        return;
    }

    const ThreatInfo &threat = _currentThreats[row];

    QString details = tr("Threat Details\n\n"
                         "Level: %1\n"
                         "Time: %2\n"
                         "Detector: %3\n"
                         "Description: %4\n\n"
                         "Affected Files (%5):\n%6")
        .arg(formatThreatLevel(threat.level))
        .arg(formatTimestamp(threat.timestamp))
        .arg(threat.detectorName)
        .arg(threat.description)
        .arg(threat.affectedFiles.size())
        .arg(threat.affectedFiles.join(QStringLiteral("\n")));

    QMessageBox::information(this, tr("Threat Details"), details);
}

QString ThreatHistoryDialog::formatThreatLevel(ThreatLevel level) const
{
    switch (level) {
    case ThreatLevel::Critical:
        return tr("CRITICAL");
    case ThreatLevel::High:
        return tr("HIGH");
    case ThreatLevel::Medium:
        return tr("MEDIUM");
    case ThreatLevel::Low:
        return tr("LOW");
    default:
        return tr("NONE");
    }
}

QString ThreatHistoryDialog::threatLevelStyleSheet(ThreatLevel level) const
{
    switch (level) {
    case ThreatLevel::Critical:
        return QStringLiteral("color: white; background-color: #DC3545; font-weight: bold; padding: 2px 6px;");
    case ThreatLevel::High:
        return QStringLiteral("color: white; background-color: #FD7E14; font-weight: bold; padding: 2px 6px;");
    case ThreatLevel::Medium:
        return QStringLiteral("color: black; background-color: #FFC107; font-weight: bold; padding: 2px 6px;");
    case ThreatLevel::Low:
        return QStringLiteral("color: white; background-color: #0D6EFD; font-weight: bold; padding: 2px 6px;");
    default:
        return QString();
    }
}

QIcon ThreatHistoryDialog::threatLevelIcon(ThreatLevel level) const
{
    // Use standard Qt icons based on threat level
    QStyle *style = this->style();
    switch (level) {
    case ThreatLevel::Critical:
    case ThreatLevel::High:
        return style->standardIcon(QStyle::SP_MessageBoxCritical);
    case ThreatLevel::Medium:
        return style->standardIcon(QStyle::SP_MessageBoxWarning);
    case ThreatLevel::Low:
        return style->standardIcon(QStyle::SP_MessageBoxInformation);
    default:
        return style->standardIcon(QStyle::SP_MessageBoxQuestion);
    }
}

QString ThreatHistoryDialog::formatTimestamp(const QDateTime &timestamp) const
{
    if (!timestamp.isValid()) {
        return tr("Unknown");
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 secsAgo = timestamp.secsTo(now);

    // Format relative time for recent events
    if (secsAgo < 60) {
        return tr("Just now");
    } else if (secsAgo < 3600) {
        int mins = static_cast<int>(secsAgo / 60);
        return tr("%n minute(s) ago", "", mins);
    } else if (secsAgo < 86400) {
        int hours = static_cast<int>(secsAgo / 3600);
        return tr("%n hour(s) ago", "", hours);
    } else if (secsAgo < 604800) {
        int days = static_cast<int>(secsAgo / 86400);
        return tr("%n day(s) ago", "", days);
    }

    // For older events, show full date
    return timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

} // namespace OCC

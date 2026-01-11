/*
 * Nextcloud Sentinel Edition - Kill Switch Settings Widget Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "killswitchsettings.h"
#include "ui_killswitchsettings.h"

#include "libsync/killswitch/killswitchmanager.h"
#include "libsync/configfile.h"
#include "theme.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>

namespace OCC {

Q_LOGGING_CATEGORY(lcKillSwitchSettings, "nextcloud.gui.killswitch", QtInfoMsg)

KillSwitchSettings::KillSwitchSettings(QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::KillSwitchSettings)
{
    _ui->setupUi(this);

    setupConnections();
    loadSettings();
    updateStatusIndicator();

    qCInfo(lcKillSwitchSettings) << "Kill Switch settings widget initialized";
}

KillSwitchSettings::~KillSwitchSettings()
{
    delete _ui;
}

QSize KillSwitchSettings::sizeHint() const
{
    return {500, 600};
}

void KillSwitchSettings::setupConnections()
{
    // Enable/disable toggle
    connect(_ui->enableKillSwitch, &QCheckBox::toggled,
            this, &KillSwitchSettings::slotEnableToggled);

    // Threshold settings
    connect(_ui->deleteThresholdSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KillSwitchSettings::slotDeleteThresholdChanged);
    connect(_ui->timeWindowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KillSwitchSettings::slotTimeWindowChanged);
    connect(_ui->entropyThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &KillSwitchSettings::slotEntropyThresholdChanged);

    // Canary file management
    connect(_ui->addCanaryButton, &QPushButton::clicked,
            this, &KillSwitchSettings::slotAddCanaryFile);
    connect(_ui->removeCanaryButton, &QPushButton::clicked,
            this, &KillSwitchSettings::slotRemoveCanaryFile);

    // Actions
    connect(_ui->resetButton, &QPushButton::clicked,
            this, &KillSwitchSettings::slotResetKillSwitch);
    connect(_ui->clearHistoryButton, &QPushButton::clicked,
            this, &KillSwitchSettings::slotClearThreatHistory);

    // Connect to KillSwitchManager if available
    if (auto *manager = KillSwitchManager::instance()) {
        connect(manager, &KillSwitchManager::threatDetected,
                this, &KillSwitchSettings::slotThreatDetected);
        connect(manager, &KillSwitchManager::threatLevelChanged,
                this, [this](ThreatLevel level) {
                    slotThreatLevelChanged(static_cast<int>(level));
                });
        connect(manager, &KillSwitchManager::triggeredChanged,
                this, &KillSwitchSettings::slotTriggeredChanged);
    }
}

void KillSwitchSettings::loadSettings()
{
    _loading = true;

    ConfigFile cfg;

    // Load settings from config
    _ui->enableKillSwitch->setChecked(cfg.getValue("killSwitch/enabled", true).toBool());
    _ui->deleteThresholdSpinBox->setValue(cfg.getValue("killSwitch/deleteThreshold", 10).toInt());
    _ui->timeWindowSpinBox->setValue(cfg.getValue("killSwitch/timeWindow", 60).toInt());
    _ui->entropyThresholdSpinBox->setValue(cfg.getValue("killSwitch/entropyThreshold", 7.5).toDouble());

    // Load canary files
    _ui->canaryListWidget->clear();
    QStringList canaryFiles = cfg.getValue("killSwitch/canaryFiles",
                                           QStringList{"_canary.txt", ".canary", "zzz_canary.txt"}).toStringList();
    for (const QString &file : canaryFiles) {
        _ui->canaryListWidget->addItem(file);
    }

    _loading = false;

    updateThreatDisplay();
    updateStatusIndicator();
}

void KillSwitchSettings::saveSettings()
{
    if (_loading) return;

    ConfigFile cfg;

    cfg.setValue("killSwitch/enabled", _ui->enableKillSwitch->isChecked());
    cfg.setValue("killSwitch/deleteThreshold", _ui->deleteThresholdSpinBox->value());
    cfg.setValue("killSwitch/timeWindow", _ui->timeWindowSpinBox->value());
    cfg.setValue("killSwitch/entropyThreshold", _ui->entropyThresholdSpinBox->value());

    // Save canary files
    QStringList canaryFiles;
    for (int i = 0; i < _ui->canaryListWidget->count(); ++i) {
        canaryFiles << _ui->canaryListWidget->item(i)->text();
    }
    cfg.setValue("killSwitch/canaryFiles", canaryFiles);

    qCInfo(lcKillSwitchSettings) << "Kill Switch settings saved";
}

void KillSwitchSettings::slotEnableToggled(bool enabled)
{
    if (auto *manager = KillSwitchManager::instance()) {
        manager->setEnabled(enabled);
    }

    _ui->settingsGroup->setEnabled(enabled);
    _ui->canaryGroup->setEnabled(enabled);

    saveSettings();
    updateStatusIndicator();

    qCInfo(lcKillSwitchSettings) << "Kill Switch" << (enabled ? "enabled" : "disabled");
}

void KillSwitchSettings::slotDeleteThresholdChanged(int value)
{
    if (auto *manager = KillSwitchManager::instance()) {
        manager->setDeleteThreshold(value, _ui->timeWindowSpinBox->value());
    }
    saveSettings();
}

void KillSwitchSettings::slotTimeWindowChanged(int value)
{
    if (auto *manager = KillSwitchManager::instance()) {
        manager->setDeleteThreshold(_ui->deleteThresholdSpinBox->value(), value);
    }
    saveSettings();
}

void KillSwitchSettings::slotEntropyThresholdChanged(double value)
{
    if (auto *manager = KillSwitchManager::instance()) {
        manager->setEntropyThreshold(value);
    }
    saveSettings();
}

void KillSwitchSettings::slotAddCanaryFile()
{
    bool ok;
    QString filename = QInputDialog::getText(this,
                                             tr("Add Canary File"),
                                             tr("Enter canary filename pattern:"),
                                             QLineEdit::Normal,
                                             "_canary.txt",
                                             &ok);
    if (ok && !filename.isEmpty()) {
        _ui->canaryListWidget->addItem(filename);

        if (auto *manager = KillSwitchManager::instance()) {
            manager->addCanaryFile(filename);
        }

        saveSettings();
        qCInfo(lcKillSwitchSettings) << "Added canary file:" << filename;
    }
}

void KillSwitchSettings::slotRemoveCanaryFile()
{
    auto *item = _ui->canaryListWidget->currentItem();
    if (item) {
        QString filename = item->text();
        delete _ui->canaryListWidget->takeItem(_ui->canaryListWidget->row(item));
        saveSettings();
        qCInfo(lcKillSwitchSettings) << "Removed canary file:" << filename;
    }
}

void KillSwitchSettings::slotResetKillSwitch()
{
    auto *manager = KillSwitchManager::instance();
    if (!manager || !manager->isTriggered()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Reset Kill Switch"),
        tr("Are you sure you want to reset the Kill Switch?\n\n"
           "This will resume synchronization. Only do this if you have "
           "verified that the detected threat was a false positive."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        manager->reset();
        updateStatusIndicator();
        qCInfo(lcKillSwitchSettings) << "Kill Switch reset by user";
    }
}

void KillSwitchSettings::slotClearThreatHistory()
{
    _ui->threatListWidget->clear();
    qCInfo(lcKillSwitchSettings) << "Threat history cleared";
}

void KillSwitchSettings::slotThreatDetected(const ThreatInfo &threat)
{
    QString levelStr;
    QString color;

    switch (threat.level) {
    case ThreatLevel::Low:
        levelStr = tr("LOW");
        color = "#FFD700"; // Yellow
        break;
    case ThreatLevel::Medium:
        levelStr = tr("MEDIUM");
        color = "#FFA500"; // Orange
        break;
    case ThreatLevel::High:
        levelStr = tr("HIGH");
        color = "#FF4500"; // Red-Orange
        break;
    case ThreatLevel::Critical:
        levelStr = tr("CRITICAL");
        color = "#FF0000"; // Red
        break;
    default:
        return;
    }

    QString entry = QStringLiteral("[%1] %2 - %3: %4")
                        .arg(threat.timestamp.toString("hh:mm:ss"))
                        .arg(levelStr)
                        .arg(threat.detectorName)
                        .arg(threat.description);

    auto *item = new QListWidgetItem(entry);
    item->setForeground(QColor(color));
    _ui->threatListWidget->insertItem(0, item);

    // Limit history to 100 items
    while (_ui->threatListWidget->count() > 100) {
        delete _ui->threatListWidget->takeItem(_ui->threatListWidget->count() - 1);
    }

    updateStatusIndicator();
}

void KillSwitchSettings::slotThreatLevelChanged(int level)
{
    updateStatusIndicator();
}

void KillSwitchSettings::slotTriggeredChanged(bool triggered)
{
    updateStatusIndicator();

    if (triggered) {
        _ui->resetButton->setEnabled(true);
        _ui->statusLabel->setText(tr("TRIGGERED - Sync Paused"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FF0000; font-weight: bold; }");
    } else {
        _ui->resetButton->setEnabled(false);
    }
}

void KillSwitchSettings::updateThreatDisplay()
{
    auto *manager = KillSwitchManager::instance();
    if (!manager) return;

    // Display existing threats
    _ui->threatListWidget->clear();
    for (const auto &threat : manager->threats()) {
        slotThreatDetected(threat);
    }
}

void KillSwitchSettings::updateStatusIndicator()
{
    auto *manager = KillSwitchManager::instance();

    if (!manager || !manager->isEnabled()) {
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #808080; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("Protection Disabled"));
        _ui->statusLabel->setStyleSheet("");
        return;
    }

    if (manager->isTriggered()) {
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #FF0000; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("TRIGGERED - Sync Paused"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FF0000; font-weight: bold; }");
        _ui->resetButton->setEnabled(true);
        return;
    }

    _ui->resetButton->setEnabled(false);

    switch (manager->currentThreatLevel()) {
    case ThreatLevel::None:
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #00FF00; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("Protected - No Threats"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #00AA00; }");
        break;
    case ThreatLevel::Low:
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #FFD700; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("Low Activity Detected"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FFD700; }");
        break;
    case ThreatLevel::Medium:
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #FFA500; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("Moderate Activity Detected"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FFA500; }");
        break;
    case ThreatLevel::High:
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #FF4500; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("High Risk Activity!"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FF4500; font-weight: bold; }");
        break;
    case ThreatLevel::Critical:
        _ui->statusIndicator->setStyleSheet(
            "QLabel { background-color: #FF0000; border-radius: 10px; min-width: 20px; min-height: 20px; }");
        _ui->statusLabel->setText(tr("CRITICAL THREAT!"));
        _ui->statusLabel->setStyleSheet("QLabel { color: #FF0000; font-weight: bold; }");
        break;
    }
}

void KillSwitchSettings::slotStyleChanged()
{
    customizeStyle();
}

void KillSwitchSettings::customizeStyle()
{
    // Apply theme-specific styling if needed
}

} // namespace OCC

/*
 * Nextcloud Sentinel Edition - Kill Switch Settings Widget
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QWidget>
#include <QPointer>

namespace OCC {

class KillSwitchManager;
struct ThreatInfo;

namespace Ui {
    class KillSwitchSettings;
}

/**
 * @brief Kill Switch Settings Widget
 *
 * Provides GUI controls for:
 * - Enabling/disabling kill switch protection
 * - Configuring detection thresholds
 * - Viewing threat history
 * - Managing canary files
 * - Resetting triggered state
 */
class KillSwitchSettings : public QWidget
{
    Q_OBJECT

public:
    explicit KillSwitchSettings(QWidget *parent = nullptr);
    ~KillSwitchSettings() override;

    [[nodiscard]] QSize sizeHint() const override;

public slots:
    void slotStyleChanged();
    void loadSettings();
    void saveSettings();

private slots:
    void slotEnableToggled(bool enabled);
    void slotDeleteThresholdChanged(int value);
    void slotTimeWindowChanged(int value);
    void slotEntropyThresholdChanged(double value);
    void slotAddCanaryFile();
    void slotRemoveCanaryFile();
    void slotResetKillSwitch();
    void slotClearThreatHistory();

    // Updates from KillSwitchManager
    void slotThreatDetected(const ThreatInfo &threat);
    void slotThreatLevelChanged(int level);
    void slotTriggeredChanged(bool triggered);

private:
    void setupConnections();
    void updateThreatDisplay();
    void updateStatusIndicator();
    void customizeStyle();

    Ui::KillSwitchSettings *_ui;
    bool _loading = false;
};

} // namespace OCC

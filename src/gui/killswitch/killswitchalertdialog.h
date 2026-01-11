/*
 * Nextcloud Sentinel Edition - Kill Switch Alert Dialog
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QVector>

namespace OCC {

struct ThreatInfo;

namespace Ui {
    class KillSwitchAlertDialog;
}

/**
 * @brief Kill Switch Alert Dialog
 *
 * Modal dialog displayed when the kill switch is triggered.
 * Shows threat details and allows user to:
 * - Review affected files
 * - Reset the kill switch (resume sync)
 * - Keep protection active
 */
class KillSwitchAlertDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KillSwitchAlertDialog(const QString &reason,
                                    const QVector<ThreatInfo> &threats,
                                    QWidget *parent = nullptr);
    ~KillSwitchAlertDialog() override;

    /**
     * @brief Show alert dialog for kill switch trigger
     * @param reason The reason the kill switch was triggered
     * @param threats List of detected threats
     * @param parent Parent widget
     * @return true if user chose to reset, false to keep protection
     */
    static bool showAlert(const QString &reason,
                          const QVector<ThreatInfo> &threats,
                          QWidget *parent = nullptr);

private slots:
    void slotResetClicked();
    void slotKeepProtectionClicked();

private:
    void populateThreats(const QVector<ThreatInfo> &threats);

    Ui::KillSwitchAlertDialog *_ui;
    bool _resetRequested = false;
};

} // namespace OCC

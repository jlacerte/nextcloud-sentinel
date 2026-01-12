/*
 * Nextcloud Sentinel Edition - Threat History Dialog
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QVector>
#include "libsync/killswitch/threatlogger.h"
#include "libsync/killswitch/killswitchmanager.h"

namespace OCC {

namespace Ui {
    class ThreatHistoryDialog;
}

/**
 * @brief ThreatHistoryDialog - View and manage threat history
 *
 * This dialog provides a comprehensive view of all detected threats:
 * - Timeline of threats with color-coded severity
 * - Filtering by time period (24h, 7 days, 30 days, all)
 * - Export to CSV for analysis
 * - Clear history functionality
 * - Statistics summary
 */
class ThreatHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThreatHistoryDialog(QWidget *parent = nullptr);
    ~ThreatHistoryDialog() override;

private slots:
    void onPeriodChanged(int index);
    void onExportClicked();
    void onClearClicked();
    void onThreatDoubleClicked(int row);

private:
    void loadThreats();
    void updateStatistics();
    void populateThreatList(const QVector<ThreatInfo> &threats);
    QString formatThreatLevel(ThreatLevel level) const;
    QString threatLevelStyleSheet(ThreatLevel level) const;
    QIcon threatLevelIcon(ThreatLevel level) const;
    QString formatTimestamp(const QDateTime &timestamp) const;

    Ui::ThreatHistoryDialog *_ui;
    int _currentDays = 1; // 24h by default
    QVector<ThreatInfo> _currentThreats;
};

} // namespace OCC

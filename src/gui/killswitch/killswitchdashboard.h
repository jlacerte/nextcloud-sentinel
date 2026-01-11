/*
 * Nextcloud Sentinel Edition - Kill Switch Dashboard Widget
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QWidget>
#include <QHash>
#include <QDateTime>

namespace OCC {

struct ThreatInfo;

namespace Ui {
    class KillSwitchDashboard;
}

/**
 * @brief Kill Switch Dashboard Widget
 *
 * Provides a visual overview of Kill Switch statistics:
 * - Total files analyzed
 * - Threats blocked
 * - Top triggered detectors
 * - Activity timeline (24h/7d/30d)
 */
class KillSwitchDashboard : public QWidget
{
    Q_OBJECT

public:
    explicit KillSwitchDashboard(QWidget *parent = nullptr);
    ~KillSwitchDashboard() override;

    [[nodiscard]] QSize sizeHint() const override;

public slots:
    void refreshStats();

private slots:
    void slotThreatDetected(const ThreatInfo &threat);
    void slotTimeRangeChanged(int index);
    void slotExportStats();

private:
    void setupConnections();
    void loadStats();
    void saveStats();
    void updateDisplay();
    void updateDetectorRanking();
    void updateTimeline();

    // Stats structure
    struct Stats {
        qint64 filesAnalyzed = 0;
        qint64 threatsBlocked = 0;
        QHash<QString, int> detectorTriggers;
        QVector<QPair<QDateTime, QString>> recentThreats; // timestamp, detector
    };

    Ui::KillSwitchDashboard *_ui;
    Stats _stats;
    int _timeRangeDays = 1; // 1 = 24h, 7 = week, 30 = month
};

} // namespace OCC

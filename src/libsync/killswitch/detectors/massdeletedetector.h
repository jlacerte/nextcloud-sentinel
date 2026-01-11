/*
 * Nextcloud Sentinel Edition - Mass Delete Detector
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "../threatdetector.h"

namespace OCC {

/**
 * @brief Detects mass file deletion patterns
 *
 * This detector monitors for suspicious bulk deletion activity that could
 * indicate a "rm -rf" accident or ransomware clearing files before encryption.
 *
 * Detection criteria:
 * - Number of deletions exceeding threshold in time window
 * - Rapid deletion rate (files per second)
 * - Deletion of entire directory trees
 */
class MassDeleteDetector : public ThreatDetector
{
public:
    MassDeleteDetector();

    QString name() const override { return QStringLiteral("MassDeleteDetector"); }

    ThreatInfo analyze(const SyncFileItem &item,
                       const QVector<struct KillSwitchManager::Event> &recentEvents) override;

    // Configuration
    void setThreshold(int count) { m_threshold = count; }
    void setRateLimit(double filesPerSecond) { m_rateLimit = filesPerSecond; }

private:
    int m_threshold = 10;         // Number of deletions to trigger
    double m_rateLimit = 5.0;     // Max deletions per second before alert
};

} // namespace OCC

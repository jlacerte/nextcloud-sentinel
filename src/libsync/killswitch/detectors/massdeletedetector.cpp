/*
 * Nextcloud Sentinel Edition - Mass Delete Detector Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "massdeletedetector.h"
#include "syncfileitem.h"

namespace OCC {

MassDeleteDetector::MassDeleteDetector()
{
}

ThreatInfo MassDeleteDetector::analyze(const SyncFileItem &item,
                                        const QVector<struct KillSwitchManager::Event> &recentEvents)
{
    ThreatInfo result;
    result.level = ThreatLevel::None;
    result.detectorName = name();

    if (!m_enabled) {
        return result;
    }

    // Only interested in deletions
    if (item._instruction != CSYNC_INSTRUCTION_REMOVE) {
        return result;
    }

    // Count recent deletions
    int deleteCount = 0;
    QDateTime oldestDelete;
    QDateTime newestDelete;

    for (const auto &event : recentEvents) {
        if (event.type == QStringLiteral("DELETE")) {
            deleteCount++;
            if (!oldestDelete.isValid() || event.timestamp < oldestDelete) {
                oldestDelete = event.timestamp;
            }
            if (!newestDelete.isValid() || event.timestamp > newestDelete) {
                newestDelete = event.timestamp;
            }
            result.affectedFiles.append(event.path);
        }
    }

    // Calculate deletion rate
    double rate = 0.0;
    if (oldestDelete.isValid() && newestDelete.isValid()) {
        qint64 msElapsed = oldestDelete.msecsTo(newestDelete);
        if (msElapsed > 0) {
            rate = (deleteCount * 1000.0) / msElapsed; // files per second
        }
    }

    // Evaluate threat level
    if (deleteCount >= m_threshold * 2) {
        result.level = ThreatLevel::Critical;
        result.description = QStringLiteral("Critical: %1 files deleted (threshold: %2)")
                                 .arg(deleteCount)
                                 .arg(m_threshold);
    } else if (deleteCount >= m_threshold) {
        result.level = ThreatLevel::High;
        result.description = QStringLiteral("High: %1 files deleted, approaching critical threshold")
                                 .arg(deleteCount);
    } else if (rate > m_rateLimit) {
        result.level = ThreatLevel::High;
        result.description = QStringLiteral("High deletion rate: %.1f files/sec (limit: %.1f)")
                                 .arg(rate)
                                 .arg(m_rateLimit);
    } else if (deleteCount >= m_threshold * 0.5) {
        result.level = ThreatLevel::Medium;
        result.description = QStringLiteral("Medium: %1 files deleted in short window")
                                 .arg(deleteCount);
    }

    return result;
}

} // namespace OCC

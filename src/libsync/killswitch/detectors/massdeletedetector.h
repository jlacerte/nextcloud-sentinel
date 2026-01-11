/*
 * Nextcloud Sentinel Edition - Mass Delete Detector
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "../threatdetector.h"
#include <QSet>

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
 *
 * Whitelisted directories (not counted as suspicious):
 * - node_modules, build, dist, .git, __pycache__, .cache
 * - These are commonly bulk-deleted during development
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

    /**
     * @brief Add a directory pattern to the whitelist
     * @param pattern Directory name to ignore (e.g., "node_modules", ".git")
     *
     * Files inside whitelisted directories are not counted toward the
     * deletion threshold. This prevents false positives from common
     * development operations like `rm -rf node_modules`.
     */
    void addWhitelistedDirectory(const QString &pattern);

    /**
     * @brief Check if a path is inside a whitelisted directory
     */
    bool isWhitelisted(const QString &path) const;

    /**
     * @brief Detect if deletions form a complete directory tree
     * @param paths List of deleted file paths
     * @return Common parent directory if tree deletion detected, empty otherwise
     *
     * Tree deletion is detected when:
     * - All deletions share a common parent directory
     * - The parent itself is also being deleted
     */
    QString detectTreeDeletion(const QStringList &paths) const;

private:
    void initializeWhitelist();

    int m_threshold = 10;         // Number of deletions to trigger
    double m_rateLimit = 5.0;     // Max deletions per second before alert
    QSet<QString> m_whitelistedDirs; // Directories to ignore
};

} // namespace OCC

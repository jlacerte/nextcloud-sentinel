/*
 * Nextcloud Sentinel Edition - Backup Action
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "syncaction.h"
#include <QString>
#include <QDateTime>

namespace OCC {

/**
 * @brief BackupAction - Creates emergency backup copies of threatened files
 *
 * When the Kill Switch detects a threat, this action creates backup copies
 * of the affected files before they can be damaged or encrypted by ransomware.
 *
 * Backup structure:
 *   {backupDir}/
 *     └── {timestamp}/
 *         └── {relative-path}/
 *             └── {filename}
 *
 * Example:
 *   ~/.local/share/Nextcloud/sentinel-backups/
 *     └── 2026-01-11_153045/
 *         └── Documents/
 *             └── important.docx
 *
 * Features:
 * - Preserves directory structure
 * - Automatic cleanup of old backups
 * - Configurable size limits
 * - Logs all backup operations
 */
class BackupAction : public SyncAction
{
public:
    BackupAction();
    ~BackupAction() override = default;

    QString name() const override { return QStringLiteral("BackupAction"); }

    /**
     * @brief Execute backup for all affected files in the threat
     */
    void execute(const ThreatInfo &threat) override;

    // Configuration

    /**
     * @brief Set the root backup directory
     */
    void setBackupDirectory(const QString &path);
    QString backupDirectory() const { return m_backupDir; }

    /**
     * @brief Set maximum total backup size in MB
     * When exceeded, oldest backups are removed
     */
    void setMaxBackupSizeMB(int sizeMB) { m_maxSizeMB = sizeMB; }
    int maxBackupSizeMB() const { return m_maxSizeMB; }

    /**
     * @brief Set backup retention in days
     * Backups older than this are automatically removed
     */
    void setRetentionDays(int days) { m_retentionDays = days; }
    int retentionDays() const { return m_retentionDays; }

    // Statistics

    /**
     * @brief Get total number of files backed up in this session
     */
    int filesBackedUp() const { return m_filesBackedUp; }

    /**
     * @brief Get total bytes backed up in this session
     */
    qint64 bytesBackedUp() const { return m_bytesBackedUp; }

    /**
     * @brief Get the path of the last backup created
     */
    QString lastBackupPath() const { return m_lastBackupPath; }

    // Maintenance

    /**
     * @brief Clean up old backups based on retention policy
     * @return Number of backup directories removed
     */
    int cleanOldBackups();

    /**
     * @brief Get total size of all backups in bytes
     */
    qint64 totalBackupSize() const;

private:
    /**
     * @brief Backup a single file
     * @param sourcePath Full path to the source file
     * @param backupRoot Root directory for this backup session
     * @return true if backup succeeded
     */
    bool backupFile(const QString &sourcePath, const QString &backupRoot);

    /**
     * @brief Ensure the backup directory exists
     */
    bool ensureBackupDirExists();

    /**
     * @brief Generate a timestamped backup directory name
     */
    QString generateBackupSessionDir() const;

    /**
     * @brief Calculate directory size recursively
     */
    qint64 calculateDirSize(const QString &path) const;

    /**
     * @brief Remove oldest backup directories until under size limit
     */
    void enforceMaxSize();

    QString m_backupDir;
    int m_maxSizeMB = 500;      // Default 500MB max
    int m_retentionDays = 7;    // Default 7 days retention

    // Session statistics
    int m_filesBackedUp = 0;
    qint64 m_bytesBackedUp = 0;
    QString m_lastBackupPath;
};

} // namespace OCC

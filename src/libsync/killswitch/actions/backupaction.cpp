/*
 * Nextcloud Sentinel Edition - Backup Action Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "backupaction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QLoggingCategory>

namespace OCC {

Q_LOGGING_CATEGORY(lcBackupAction, "nextcloud.sync.killswitch.backup", QtInfoMsg)

BackupAction::BackupAction()
{
}

void BackupAction::setBackupDirectory(const QString &path)
{
    m_backupDir = path;
    ensureBackupDirExists();
}

bool BackupAction::ensureBackupDirExists()
{
    if (m_backupDir.isEmpty()) {
        qCWarning(lcBackupAction) << "Backup directory not configured";
        return false;
    }

    QDir dir(m_backupDir);
    if (!dir.exists()) {
        if (!dir.mkpath(m_backupDir)) {
            qCWarning(lcBackupAction) << "Failed to create backup directory:" << m_backupDir;
            return false;
        }
        qCInfo(lcBackupAction) << "Created backup directory:" << m_backupDir;
    }
    return true;
}

QString BackupAction::generateBackupSessionDir() const
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmmss"));
}

void BackupAction::execute(const ThreatInfo &threat)
{
    if (!m_enabled) {
        qCDebug(lcBackupAction) << "BackupAction is disabled, skipping";
        return;
    }

    if (threat.affectedFiles.isEmpty()) {
        qCDebug(lcBackupAction) << "No affected files to backup";
        return;
    }

    if (!ensureBackupDirExists()) {
        qCWarning(lcBackupAction) << "Cannot create backup - directory not available";
        return;
    }

    // Create a session directory for this backup
    QString sessionDir = generateBackupSessionDir();
    QString backupRoot = QDir(m_backupDir).filePath(sessionDir);

    QDir().mkpath(backupRoot);
    m_lastBackupPath = backupRoot;

    qCInfo(lcBackupAction) << "Starting backup for threat:" << threat.description;
    qCInfo(lcBackupAction) << "Backup location:" << backupRoot;
    qCInfo(lcBackupAction) << "Files to backup:" << threat.affectedFiles.size();

    int successCount = 0;
    int failCount = 0;

    for (const QString &filePath : threat.affectedFiles) {
        if (backupFile(filePath, backupRoot)) {
            successCount++;
        } else {
            failCount++;
        }
    }

    qCInfo(lcBackupAction) << "Backup complete:" << successCount << "succeeded," << failCount << "failed";

    // Cleanup old backups
    cleanOldBackups();
    enforceMaxSize();
}

bool BackupAction::backupFile(const QString &sourcePath, const QString &backupRoot)
{
    QFileInfo sourceInfo(sourcePath);

    if (!sourceInfo.exists()) {
        qCWarning(lcBackupAction) << "Source file does not exist:" << sourcePath;
        return false;
    }

    if (!sourceInfo.isFile()) {
        qCDebug(lcBackupAction) << "Skipping non-file:" << sourcePath;
        return false;
    }

    // Preserve relative directory structure
    // Extract the relative path from the sync root
    QString relativePath = sourceInfo.fileName();

    // If the path contains directory separators, preserve the structure
    // We take the last 3 directory components to avoid super long paths
    QStringList pathParts = sourcePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.size() > 1) {
        int start = qMax(0, pathParts.size() - 4); // Last 4 components (3 dirs + filename)
        QStringList relevantParts;
        for (int i = start; i < pathParts.size(); i++) {
            relevantParts.append(pathParts[i]);
        }
        relativePath = relevantParts.join(QLatin1Char('/'));
    }

    QString destPath = QDir(backupRoot).filePath(relativePath);
    QString destDir = QFileInfo(destPath).absolutePath();

    // Create destination directory
    if (!QDir().mkpath(destDir)) {
        qCWarning(lcBackupAction) << "Failed to create backup subdirectory:" << destDir;
        return false;
    }

    // Copy the file
    if (QFile::exists(destPath)) {
        // If file already exists (unlikely but possible), remove it first
        QFile::remove(destPath);
    }

    if (!QFile::copy(sourcePath, destPath)) {
        qCWarning(lcBackupAction) << "Failed to copy file:" << sourcePath << "to" << destPath;
        return false;
    }

    qint64 fileSize = sourceInfo.size();
    m_filesBackedUp++;
    m_bytesBackedUp += fileSize;

    qCInfo(lcBackupAction) << "Backed up:" << sourcePath << "(" << fileSize << "bytes)";
    return true;
}

int BackupAction::cleanOldBackups()
{
    if (m_backupDir.isEmpty()) {
        return 0;
    }

    QDir backupDir(m_backupDir);
    if (!backupDir.exists()) {
        return 0;
    }

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-m_retentionDays);
    int removedCount = 0;

    QStringList sessionDirs = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QString &sessionName : sessionDirs) {
        // Parse timestamp from directory name (format: yyyy-MM-dd_HHmmss)
        QDateTime sessionTime = QDateTime::fromString(sessionName, QStringLiteral("yyyy-MM-dd_HHmmss"));

        if (sessionTime.isValid() && sessionTime < cutoff) {
            QString sessionPath = backupDir.filePath(sessionName);
            QDir sessionDir(sessionPath);

            if (sessionDir.removeRecursively()) {
                qCInfo(lcBackupAction) << "Removed old backup:" << sessionName;
                removedCount++;
            } else {
                qCWarning(lcBackupAction) << "Failed to remove old backup:" << sessionName;
            }
        }
    }

    if (removedCount > 0) {
        qCInfo(lcBackupAction) << "Cleaned up" << removedCount << "old backup(s)";
    }

    return removedCount;
}

qint64 BackupAction::calculateDirSize(const QString &path) const
{
    qint64 totalSize = 0;
    QDirIterator it(path, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile()) {
            totalSize += it.fileInfo().size();
        }
    }

    return totalSize;
}

qint64 BackupAction::totalBackupSize() const
{
    if (m_backupDir.isEmpty() || !QDir(m_backupDir).exists()) {
        return 0;
    }
    return calculateDirSize(m_backupDir);
}

void BackupAction::enforceMaxSize()
{
    if (m_maxSizeMB <= 0) {
        return; // No limit
    }

    qint64 maxBytes = static_cast<qint64>(m_maxSizeMB) * 1024 * 1024;
    qint64 currentSize = totalBackupSize();

    if (currentSize <= maxBytes) {
        return;
    }

    qCInfo(lcBackupAction) << "Backup size" << (currentSize / 1024 / 1024) << "MB exceeds limit" << m_maxSizeMB << "MB";

    QDir backupDir(m_backupDir);
    QStringList sessionDirs = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // Remove oldest backups first (sorted by name = sorted by date)
    for (const QString &sessionName : sessionDirs) {
        if (currentSize <= maxBytes) {
            break;
        }

        QString sessionPath = backupDir.filePath(sessionName);
        qint64 sessionSize = calculateDirSize(sessionPath);

        QDir sessionDir(sessionPath);
        if (sessionDir.removeRecursively()) {
            currentSize -= sessionSize;
            qCInfo(lcBackupAction) << "Removed backup to enforce size limit:" << sessionName
                                   << "(" << (sessionSize / 1024 / 1024) << "MB)";
        }
    }
}

} // namespace OCC

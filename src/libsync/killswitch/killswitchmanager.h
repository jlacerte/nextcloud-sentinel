/*
 * Nextcloud Sentinel Edition - Kill Switch Manager
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include <QMutex>
#include <memory>

namespace OCC {

class SyncFileItem;
class ThreatDetector;
class SyncAction;

/**
 * @brief Threat level enumeration
 */
enum class ThreatLevel {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

/**
 * @brief Structure describing a detected threat
 */
struct ThreatInfo {
    ThreatLevel level;
    QString detectorName;
    QString description;
    QStringList affectedFiles;
    QDateTime timestamp;
};

/**
 * @brief Kill Switch Manager - Central threat detection and response coordinator
 *
 * This class coordinates multiple threat detectors and manages the response
 * to potential ransomware or mass deletion attacks. It can pause synchronization,
 * create backups, and alert the user.
 *
 * Architecture:
 *   KillSwitchManager
 *     ├── ThreatDetector[] (analyzers)
 *     │   ├── MassDeleteDetector
 *     │   ├── EntropyDetector
 *     │   ├── CanaryDetector
 *     │   └── PatternDetector
 *     └── SyncAction[] (responses)
 *         ├── PauseSyncAction
 *         ├── BackupAction
 *         └── AlertAction
 */
class KillSwitchManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool triggered READ isTriggered NOTIFY triggeredChanged)
    Q_PROPERTY(ThreatLevel currentThreatLevel READ currentThreatLevel NOTIFY threatLevelChanged)

public:
    explicit KillSwitchManager(QObject *parent = nullptr);
    ~KillSwitchManager() override;

    /**
     * @brief Get the singleton instance
     */
    static KillSwitchManager *instance();

    /**
     * @brief Check if kill switch protection is enabled
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable kill switch protection
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if the kill switch has been triggered
     */
    bool isTriggered() const;

    /**
     * @brief Get the current threat level
     */
    ThreatLevel currentThreatLevel() const;

    /**
     * @brief Register a threat detector
     */
    void registerDetector(std::shared_ptr<ThreatDetector> detector);

    /**
     * @brief Register a sync action to execute on threat detection
     */
    void registerAction(std::shared_ptr<SyncAction> action);

    /**
     * @brief Analyze a sync item for threats
     * @param item The sync file item to analyze
     * @return true if the item should be blocked
     */
    bool analyzeItem(const SyncFileItem &item);

    /**
     * @brief Analyze a batch of sync items
     * @param items Vector of sync items to analyze
     * @return true if sync should be paused
     */
    bool analyzeBatch(const QVector<SyncFileItem> &items);

    /**
     * @brief Manually trigger the kill switch
     * @param reason The reason for triggering
     */
    void trigger(const QString &reason);

    /**
     * @brief Reset the kill switch after user confirmation
     */
    void reset();

    /**
     * @brief Get the list of detected threats
     */
    QVector<ThreatInfo> threats() const;

    // Configuration
    void setDeleteThreshold(int count, int windowSeconds);
    void setEntropyThreshold(double threshold);
    void addCanaryFile(const QString &filename);
    void setAutoBackup(bool enabled);

signals:
    void enabledChanged(bool enabled);
    void triggeredChanged(bool triggered);
    void threatLevelChanged(ThreatLevel level);
    void threatDetected(const ThreatInfo &threat);
    void syncPaused(const QString &reason);
    void syncResumed();
    void backupCreated(const QString &path);

private slots:
    void onWindowTimeout();
    void evaluateThreatLevel();

private:
    void executeActions(const ThreatInfo &threat);
    void recordEvent(const QString &eventType, const QString &path);

    static KillSwitchManager *s_instance;

    bool m_enabled = true;
    bool m_triggered = false;
    ThreatLevel m_threatLevel = ThreatLevel::None;

    QVector<std::shared_ptr<ThreatDetector>> m_detectors;
    QVector<std::shared_ptr<SyncAction>> m_actions;
    QVector<ThreatInfo> m_threats;

    // Event tracking
    struct Event {
        QDateTime timestamp;
        QString type;
        QString path;
    };
    QVector<Event> m_recentEvents;
    QTimer m_windowTimer;
    QMutex m_mutex;

    // Configuration
    int m_deleteThreshold = 10;
    int m_windowSeconds = 60;
    double m_entropyThreshold = 7.5;
    QStringList m_canaryFiles;
    bool m_autoBackup = true;
};

} // namespace OCC

/*
 * Nextcloud Sentinel Edition - Kill Switch Manager Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "killswitchmanager.h"
#include "threatdetector.h"
#include "actions/syncaction.h"
#include "syncfileitem.h"
#include "logger.h"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <algorithm>

namespace OCC {

Q_LOGGING_CATEGORY(lcKillSwitch, "nextcloud.sync.killswitch", QtInfoMsg)

KillSwitchManager *KillSwitchManager::s_instance = nullptr;

KillSwitchManager::KillSwitchManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    // Setup window timer for event expiration
    m_windowTimer.setInterval(1000); // Check every second
    connect(&m_windowTimer, &QTimer::timeout, this, &KillSwitchManager::onWindowTimeout);
    m_windowTimer.start();

    qCInfo(lcKillSwitch) << "Kill Switch Manager initialized";
}

KillSwitchManager::~KillSwitchManager()
{
    // Stop the window timer to prevent callbacks on destroyed object
    m_windowTimer.stop();

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

KillSwitchManager *KillSwitchManager::instance()
{
    return s_instance;
}

bool KillSwitchManager::isEnabled() const
{
    return m_enabled;
}

void KillSwitchManager::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        qCInfo(lcKillSwitch) << "Kill Switch" << (enabled ? "enabled" : "disabled");
        emit enabledChanged(enabled);
    }
}

bool KillSwitchManager::isTriggered() const
{
    return m_triggered;
}

ThreatLevel KillSwitchManager::currentThreatLevel() const
{
    return m_threatLevel;
}

void KillSwitchManager::registerDetector(std::shared_ptr<ThreatDetector> detector)
{
    m_detectors.append(detector);
    qCInfo(lcKillSwitch) << "Registered detector:" << detector->name();
}

void KillSwitchManager::registerAction(std::shared_ptr<SyncAction> action)
{
    m_actions.append(action);
    qCInfo(lcKillSwitch) << "Registered action:" << action->name();
}

bool KillSwitchManager::analyzeItem(const SyncFileItem &item)
{
    if (!m_enabled || m_triggered) {
        return m_triggered; // Block if already triggered
    }

    QMutexLocker locker(&m_mutex);

    // Record the event
    QString eventType;
    switch (item._instruction) {
    case CSYNC_INSTRUCTION_REMOVE:
        eventType = QStringLiteral("DELETE");
        break;
    case CSYNC_INSTRUCTION_NEW:
        eventType = QStringLiteral("CREATE");
        break;
    case CSYNC_INSTRUCTION_SYNC:
    case CSYNC_INSTRUCTION_CONFLICT:
        eventType = QStringLiteral("MODIFY");
        break;
    default:
        return false; // No threat from other operations
    }

    recordEvent(eventType, item._file);

    // Run through all detectors
    for (const auto &detector : m_detectors) {
        ThreatInfo threat = detector->analyze(item, m_recentEvents);
        if (threat.level != ThreatLevel::None) {
            threat.timestamp = QDateTime::currentDateTime();
            m_threats.append(threat);

            qCWarning(lcKillSwitch) << "Threat detected by" << threat.detectorName
                                    << "- Level:" << static_cast<int>(threat.level)
                                    << "- " << threat.description;

            emit threatDetected(threat);

            if (threat.level >= ThreatLevel::High) {
                trigger(threat.description);
                return true; // Block this item
            }
        }
    }

    evaluateThreatLevel();
    return false;
}

bool KillSwitchManager::analyzeBatch(const QVector<SyncFileItem> &items)
{
    if (!m_enabled) {
        return false;
    }

    // Count deletions in this batch
    int deleteCount = 0;
    for (const auto &item : items) {
        if (item._instruction == CSYNC_INSTRUCTION_REMOVE) {
            deleteCount++;
        }
    }

    // Immediate trigger on massive batch deletion
    if (deleteCount > m_deleteThreshold * 2) {
        trigger(QStringLiteral("Massive batch deletion detected: %1 files").arg(deleteCount));
        return true;
    }

    // Analyze each item
    for (const auto &item : items) {
        if (analyzeItem(item)) {
            return true;
        }
    }

    return m_triggered;
}

void KillSwitchManager::trigger(const QString &reason)
{
    if (m_triggered) {
        return; // Already triggered
    }

    QMutexLocker locker(&m_mutex);

    m_triggered = true;
    m_threatLevel = ThreatLevel::Critical;

    qCCritical(lcKillSwitch) << "!!! KILL SWITCH TRIGGERED !!!";
    qCCritical(lcKillSwitch) << "Reason:" << reason;

    // Create threat info for this trigger
    ThreatInfo threat;
    threat.level = ThreatLevel::Critical;
    threat.detectorName = QStringLiteral("KillSwitchManager");
    threat.description = reason;
    threat.timestamp = QDateTime::currentDateTime();

    // Execute all registered actions
    executeActions(threat);

    emit triggeredChanged(true);
    emit threatLevelChanged(m_threatLevel);
    emit syncPaused(reason);
}

void KillSwitchManager::reset()
{
    QMutexLocker locker(&m_mutex);

    m_triggered = false;
    m_threatLevel = ThreatLevel::None;
    m_threats.clear();
    m_recentEvents.clear();

    qCInfo(lcKillSwitch) << "Kill Switch reset by user";

    emit triggeredChanged(false);
    emit threatLevelChanged(m_threatLevel);
    emit syncResumed();
}

QVector<ThreatInfo> KillSwitchManager::threats() const
{
    return m_threats;
}

void KillSwitchManager::setDeleteThreshold(int count, int windowSeconds)
{
    m_deleteThreshold = count;
    m_windowSeconds = windowSeconds;
    qCInfo(lcKillSwitch) << "Delete threshold set to" << count << "files in" << windowSeconds << "seconds";
}

void KillSwitchManager::setEntropyThreshold(double threshold)
{
    m_entropyThreshold = threshold;
    qCInfo(lcKillSwitch) << "Entropy threshold set to" << threshold;
}

void KillSwitchManager::addCanaryFile(const QString &filename)
{
    if (!m_canaryFiles.contains(filename)) {
        m_canaryFiles.append(filename);
        qCInfo(lcKillSwitch) << "Added canary file:" << filename;
    }
}

void KillSwitchManager::setAutoBackup(bool enabled)
{
    m_autoBackup = enabled;
    qCInfo(lcKillSwitch) << "Auto-backup" << (enabled ? "enabled" : "disabled");
}

void KillSwitchManager::onWindowTimeout()
{
    QMutexLocker locker(&m_mutex);

    // Remove events older than the window
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-m_windowSeconds);
    m_recentEvents.erase(
        std::remove_if(m_recentEvents.begin(), m_recentEvents.end(),
                       [&cutoff](const Event &e) { return e.timestamp < cutoff; }),
        m_recentEvents.end());
}

void KillSwitchManager::evaluateThreatLevel()
{
    // Count recent deletions
    int deleteCount = 0;
    for (const auto &event : m_recentEvents) {
        if (event.type == QStringLiteral("DELETE")) {
            deleteCount++;
        }
    }

    ThreatLevel newLevel = ThreatLevel::None;

    if (deleteCount >= m_deleteThreshold) {
        newLevel = ThreatLevel::Critical;
    } else if (deleteCount >= m_deleteThreshold * 0.7) {
        newLevel = ThreatLevel::High;
    } else if (deleteCount >= m_deleteThreshold * 0.5) {
        newLevel = ThreatLevel::Medium;
    } else if (deleteCount >= m_deleteThreshold * 0.3) {
        newLevel = ThreatLevel::Low;
    }

    if (newLevel != m_threatLevel) {
        m_threatLevel = newLevel;
        emit threatLevelChanged(newLevel);

        if (newLevel >= ThreatLevel::Critical && !m_triggered) {
            trigger(QStringLiteral("Deletion threshold exceeded: %1 files in %2 seconds")
                        .arg(deleteCount)
                        .arg(m_windowSeconds));
        }
    }
}

void KillSwitchManager::executeActions(const ThreatInfo &threat)
{
    for (const auto &action : m_actions) {
        qCInfo(lcKillSwitch) << "Executing action:" << action->name();
        action->execute(threat);
    }
}

void KillSwitchManager::recordEvent(const QString &eventType, const QString &path)
{
    Event event;
    event.timestamp = QDateTime::currentDateTime();
    event.type = eventType;
    event.path = path;
    m_recentEvents.append(event);
}

} // namespace OCC

/*
 * Nextcloud Sentinel Edition - Threat Detector Base Class
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "killswitchmanager.h"
#include <QString>
#include <QVector>

namespace OCC {

class SyncFileItem;

/**
 * @brief Abstract base class for threat detectors
 *
 * Subclasses implement specific detection strategies:
 * - MassDeleteDetector: Detects bulk deletions (rm -rf accidents, ransomware cleanup)
 * - EntropyDetector: Detects encrypted files via Shannon entropy analysis
 * - CanaryDetector: Detects modifications to honeypot/canary files
 * - PatternDetector: Detects ransomware extensions (.locked, .encrypted, ransom notes)
 */
class ThreatDetector
{
public:
    virtual ~ThreatDetector() = default;

    /**
     * @brief Get the detector's name
     */
    virtual QString name() const = 0;

    /**
     * @brief Analyze a sync item and recent events for threats
     * @param item The current sync item being processed
     * @param recentEvents List of recent events in the time window
     * @return ThreatInfo with level None if no threat detected
     */
    virtual ThreatInfo analyze(const SyncFileItem &item,
                               const QVector<struct KillSwitchManager::Event> &recentEvents) = 0;

    /**
     * @brief Check if the detector is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Enable or disable the detector
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = true;
};

} // namespace OCC

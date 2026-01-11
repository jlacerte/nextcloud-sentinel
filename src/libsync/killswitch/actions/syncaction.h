/*
 * Nextcloud Sentinel Edition - Sync Action Base Class
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "../killswitchmanager.h"
#include <QString>

namespace OCC {

/**
 * @brief Abstract base class for actions executed when threats are detected
 *
 * Subclasses implement specific response strategies:
 * - PauseSyncAction: Pauses all synchronization
 * - BackupAction: Creates emergency backup copies
 * - AlertAction: Notifies the user via system notification
 * - QuarantineAction: Moves suspicious files to quarantine
 */
class SyncAction
{
public:
    virtual ~SyncAction() = default;

    /**
     * @brief Get the action's name
     */
    virtual QString name() const = 0;

    /**
     * @brief Execute the action in response to a threat
     * @param threat The detected threat information
     */
    virtual void execute(const ThreatInfo &threat) = 0;

    /**
     * @brief Check if the action is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Enable or disable the action
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = true;
};

} // namespace OCC

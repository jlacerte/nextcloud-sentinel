# Nextcloud Sentinel Edition - Kill Switch Module
#
# SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(KILLSWITCH_SOURCES
    killswitchmanager.cpp
    killswitchmanager.h
    threatdetector.h

    # Detectors
    detectors/massdeletedetector.cpp
    detectors/massdeletedetector.h
    detectors/entropydetector.cpp
    detectors/entropydetector.h
    detectors/canarydetector.cpp
    detectors/canarydetector.h

    # Actions
    actions/syncaction.h
)

# Add to parent scope for libsync
set(KILLSWITCH_SOURCES ${KILLSWITCH_SOURCES} PARENT_SCOPE)

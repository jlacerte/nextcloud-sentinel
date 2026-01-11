/*
 * Nextcloud Sentinel Edition - Entropy Detector
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "../threatdetector.h"
#include <QHash>

namespace OCC {

/**
 * @brief Detects encrypted/ransomware files via entropy analysis
 *
 * Shannon entropy measures randomness in data (0-8 bits/byte).
 * Encrypted data has very high entropy (~7.9-8.0).
 * Normal files have lower entropy depending on type.
 *
 * Detection criteria:
 * - New files with suspiciously high entropy
 * - Existing files with sudden entropy increase (indicating encryption)
 * - Patterns of multiple high-entropy file modifications
 */
class EntropyDetector : public ThreatDetector
{
public:
    EntropyDetector();

    QString name() const override { return QStringLiteral("EntropyDetector"); }

    ThreatInfo analyze(const SyncFileItem &item,
                       const QVector<KillSwitchManager::Event> &recentEvents) override;

    // Configuration
    void setHighEntropyThreshold(double threshold) { m_highThreshold = threshold; }
    void setSuspiciousEntropyThreshold(double threshold) { m_suspiciousThreshold = threshold; }
    void setSampleSize(int bytes) { m_sampleSize = bytes; }

    /**
     * @brief Calculate Shannon entropy of data
     * @param data The data to analyze
     * @return Entropy in bits per byte (0.0 - 8.0)
     */
    static double calculateEntropy(const QByteArray &data);

    /**
     * @brief Calculate entropy of a file
     * @param filePath Path to the file
     * @param sampleSize Number of bytes to sample (0 = entire file)
     * @return Entropy in bits per byte, or -1.0 on error
     */
    static double calculateFileEntropy(const QString &filePath, int sampleSize = 65536);

private:
    /**
     * @brief Check if file type normally has high entropy
     */
    bool isNormallyHighEntropy(const QString &filePath) const;

    /**
     * @brief Get expected entropy range for file type
     */
    QPair<double, double> expectedEntropyRange(const QString &filePath) const;

    double m_highThreshold = 7.9;       // Definitely encrypted
    double m_suspiciousThreshold = 7.5; // Suspicious
    int m_sampleSize = 65536;           // 64KB sample

    // Cache of known file entropies for comparison
    QHash<QString, double> m_entropyCache;

    // File extensions that normally have high entropy
    static const QStringList s_highEntropyExtensions;
};

} // namespace OCC

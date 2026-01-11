/*
 * Nextcloud Sentinel Edition - Entropy Detector Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "entropydetector.h"
#include "syncfileitem.h"

#include <QFile>
#include <QFileInfo>
#include <cmath>
#include <array>

namespace OCC {

// File types that normally have high entropy (compressed/encrypted by design)
const QStringList EntropyDetector::s_highEntropyExtensions = {
    // Archives
    QStringLiteral(".zip"), QStringLiteral(".gz"), QStringLiteral(".bz2"),
    QStringLiteral(".xz"), QStringLiteral(".7z"), QStringLiteral(".rar"),
    QStringLiteral(".tar.gz"), QStringLiteral(".tgz"),
    // Images (compressed)
    QStringLiteral(".jpg"), QStringLiteral(".jpeg"), QStringLiteral(".png"),
    QStringLiteral(".gif"), QStringLiteral(".webp"),
    // Media
    QStringLiteral(".mp3"), QStringLiteral(".mp4"), QStringLiteral(".avi"),
    QStringLiteral(".mkv"), QStringLiteral(".flac"), QStringLiteral(".ogg"),
    // Documents (often compressed internally)
    QStringLiteral(".pdf"), QStringLiteral(".docx"), QStringLiteral(".xlsx"),
    QStringLiteral(".pptx"), QStringLiteral(".odt"), QStringLiteral(".ods"),
    // Encrypted
    QStringLiteral(".gpg"), QStringLiteral(".aes"), QStringLiteral(".enc"),
};

EntropyDetector::EntropyDetector()
{
}

double EntropyDetector::calculateEntropy(const QByteArray &data)
{
    if (data.isEmpty()) {
        return 0.0;
    }

    // Count byte frequencies
    std::array<int, 256> counts{};
    for (unsigned char byte : data) {
        counts[byte]++;
    }

    // Calculate Shannon entropy
    double entropy = 0.0;
    double total = static_cast<double>(data.size());

    for (int count : counts) {
        if (count > 0) {
            double probability = count / total;
            entropy -= probability * std::log2(probability);
        }
    }

    return entropy;
}

double EntropyDetector::calculateFileEntropy(const QString &filePath, int sampleSize)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1.0;
    }

    QByteArray data;
    if (sampleSize <= 0 || file.size() <= sampleSize) {
        data = file.readAll();
    } else {
        data = file.read(sampleSize);
    }

    file.close();

    if (data.isEmpty()) {
        return 0.0;
    }

    return calculateEntropy(data);
}

bool EntropyDetector::isNormallyHighEntropy(const QString &filePath) const
{
    QString lower = filePath.toLower();
    for (const QString &ext : s_highEntropyExtensions) {
        if (lower.endsWith(ext)) {
            return true;
        }
    }
    return false;
}

QPair<double, double> EntropyDetector::expectedEntropyRange(const QString &filePath) const
{
    QString lower = filePath.toLower();

    // Text files
    if (lower.endsWith(QStringLiteral(".txt")) ||
        lower.endsWith(QStringLiteral(".md")) ||
        lower.endsWith(QStringLiteral(".rst"))) {
        return {3.0, 5.5};
    }

    // Source code
    if (lower.endsWith(QStringLiteral(".cpp")) ||
        lower.endsWith(QStringLiteral(".h")) ||
        lower.endsWith(QStringLiteral(".py")) ||
        lower.endsWith(QStringLiteral(".js")) ||
        lower.endsWith(QStringLiteral(".ts")) ||
        lower.endsWith(QStringLiteral(".java")) ||
        lower.endsWith(QStringLiteral(".c"))) {
        return {4.0, 6.0};
    }

    // Config files
    if (lower.endsWith(QStringLiteral(".json")) ||
        lower.endsWith(QStringLiteral(".xml")) ||
        lower.endsWith(QStringLiteral(".yaml")) ||
        lower.endsWith(QStringLiteral(".yml")) ||
        lower.endsWith(QStringLiteral(".ini")) ||
        lower.endsWith(QStringLiteral(".conf"))) {
        return {3.5, 5.5};
    }

    // CSV/data files
    if (lower.endsWith(QStringLiteral(".csv")) ||
        lower.endsWith(QStringLiteral(".tsv"))) {
        return {3.0, 5.0};
    }

    // HTML
    if (lower.endsWith(QStringLiteral(".html")) ||
        lower.endsWith(QStringLiteral(".htm"))) {
        return {4.0, 6.0};
    }

    // Default for unknown types
    return {0.0, 8.0};
}

ThreatInfo EntropyDetector::analyze(const SyncFileItem &item,
                                     const QVector<struct KillSwitchManager::Event> &recentEvents)
{
    Q_UNUSED(recentEvents)

    ThreatInfo result;
    result.level = ThreatLevel::None;
    result.detectorName = name();

    if (!m_enabled) {
        return result;
    }

    // Only analyze file modifications and creations
    if (item._instruction != CSYNC_INSTRUCTION_SYNC &&
        item._instruction != CSYNC_INSTRUCTION_NEW) {
        return result;
    }

    // Skip directories
    if (item._type == ItemTypeDirectory) {
        return result;
    }

    // Skip files that normally have high entropy
    if (isNormallyHighEntropy(item._file)) {
        return result;
    }

    // Calculate current entropy
    double entropy = calculateFileEntropy(item._file, m_sampleSize);
    if (entropy < 0) {
        return result; // Could not read file
    }

    // Get expected range for this file type
    auto expectedRange = expectedEntropyRange(item._file);

    // Check if entropy is suspicious
    if (entropy >= m_highThreshold) {
        result.level = ThreatLevel::Critical;
        result.description = QStringLiteral("Critical entropy: %.3f bits/byte (file: %1)")
                                 .arg(item._file)
                                 .arg(entropy);
        result.affectedFiles.append(item._file);
    } else if (entropy >= m_suspiciousThreshold && entropy > expectedRange.second) {
        result.level = ThreatLevel::High;
        result.description = QStringLiteral("Suspicious entropy: %.3f (expected: %.1f-%.1f) for %1")
                                 .arg(item._file)
                                 .arg(entropy)
                                 .arg(expectedRange.first)
                                 .arg(expectedRange.second);
        result.affectedFiles.append(item._file);
    } else if (m_entropyCache.contains(item._file)) {
        // Check for sudden entropy increase (ransomware encryption)
        double oldEntropy = m_entropyCache[item._file];
        double increase = entropy - oldEntropy;

        if (increase > 2.0 && entropy > 7.0) {
            result.level = ThreatLevel::High;
            result.description = QStringLiteral("Entropy spike: %.1f -> %.1f (delta: +%.1f) for %1")
                                     .arg(item._file)
                                     .arg(oldEntropy)
                                     .arg(entropy)
                                     .arg(increase);
            result.affectedFiles.append(item._file);
        }
    }

    // Update cache
    m_entropyCache[item._file] = entropy;

    return result;
}

} // namespace OCC

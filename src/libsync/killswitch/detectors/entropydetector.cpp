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

double EntropyDetector::calculateMultiBlockEntropy(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1.0;
    }

    qint64 fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        return 0.0;
    }

    // Small files: analyze entirely
    if (fileSize <= SMALL_FILE_THRESHOLD) {
        QByteArray data = file.readAll();
        file.close();
        return calculateEntropy(data);
    }

    // Collect samples based on file size
    QVector<qint64> sampleOffsets;
    int blockSize = SAMPLE_BLOCK_SIZE;

    if (fileSize <= MEDIUM_FILE_THRESHOLD) {
        // Medium files: 3 samples (beginning, middle, end)
        sampleOffsets = {
            0,
            (fileSize / 2) - (blockSize / 2),
            fileSize - blockSize
        };
    } else {
        // Large files: 5 samples distributed evenly
        qint64 step = fileSize / 5;
        for (int i = 0; i < 5; i++) {
            sampleOffsets.append(i * step);
        }
    }

    // Read and analyze each sample
    double maxEntropy = 0.0;
    int sampleCount = 0;

    for (qint64 offset : sampleOffsets) {
        // Ensure we don't read past end of file
        if (offset + blockSize > fileSize) {
            offset = fileSize - blockSize;
        }
        if (offset < 0) {
            offset = 0;
        }

        if (!file.seek(offset)) {
            continue;
        }

        QByteArray sample = file.read(blockSize);
        if (sample.isEmpty()) {
            continue;
        }

        double entropy = calculateEntropy(sample);
        sampleCount++;

        if (entropy > maxEntropy) {
            maxEntropy = entropy;
        }

        // Early exit: if first sample shows very high entropy, likely encrypted
        if (sampleCount == 1 && entropy >= m_highThreshold) {
            file.close();
            return entropy; // Return early - definitely suspicious
        }
    }

    file.close();

    if (sampleCount == 0) {
        return -1.0;
    }

    // Return the maximum entropy found (most conservative approach for security)
    return maxEntropy;
}

void EntropyDetector::updateCache(const QString &filePath, double entropy) const
{
    // Skip caching if disabled
    if (!m_cacheEnabled) {
        return;
    }

    // Remove from current position if exists
    int existingIndex = m_cacheOrder.indexOf(filePath);
    if (existingIndex >= 0) {
        m_cacheOrder.removeAt(existingIndex);
    }

    // Add to end (most recently used)
    m_cacheOrder.append(filePath);
    m_entropyCache[filePath] = entropy;

    // Evict oldest if over limit
    while (m_cacheOrder.size() > m_cacheMaxSize && !m_cacheOrder.isEmpty()) {
        QString oldest = m_cacheOrder.takeFirst();
        m_entropyCache.remove(oldest);
    }
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

    // Calculate current entropy using multi-block sampling for accuracy
    // This is more reliable for detecting partial encryption
    double entropy = calculateMultiBlockEntropy(item._file);
    if (entropy < 0) {
        return result; // Could not read file
    }

    // Get expected range for this file type
    auto expectedRange = expectedEntropyRange(item._file);

    // Check if entropy is suspicious
    if (entropy >= m_highThreshold) {
        result.level = ThreatLevel::Critical;
        result.description = QStringLiteral("Critical entropy: %1 bits/byte (file: %2)")
                                 .arg(entropy, 0, 'f', 3)
                                 .arg(item._file);
        result.affectedFiles.append(item._file);
    } else if (entropy >= m_suspiciousThreshold && entropy > expectedRange.second) {
        result.level = ThreatLevel::High;
        result.description = QStringLiteral("Suspicious entropy: %1 (expected: %2-%3) for %4")
                                 .arg(entropy, 0, 'f', 3)
                                 .arg(expectedRange.first, 0, 'f', 1)
                                 .arg(expectedRange.second, 0, 'f', 1)
                                 .arg(item._file);
        result.affectedFiles.append(item._file);
    } else if (m_cacheEnabled && m_entropyCache.contains(item._file)) {
        // Check for sudden entropy increase (ransomware encryption)
        double oldEntropy = m_entropyCache[item._file];
        double increase = entropy - oldEntropy;

        if (increase > 2.0 && entropy > 7.0) {
            result.level = ThreatLevel::High;
            result.description = QStringLiteral("Entropy spike: %1 -> %2 (delta: +%3) for %4")
                                     .arg(oldEntropy, 0, 'f', 1)
                                     .arg(entropy, 0, 'f', 1)
                                     .arg(increase, 0, 'f', 1)
                                     .arg(item._file);
            result.affectedFiles.append(item._file);
        }
    }

    // Update cache with LRU eviction
    updateCache(item._file, entropy);

    return result;
}

} // namespace OCC

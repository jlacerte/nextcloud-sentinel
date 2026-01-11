/*
 * Nextcloud Sentinel Edition - Pattern Detector Implementation
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "patterndetector.h"
#include "syncfileitem.h"

#include <QFileInfo>
#include <QLoggingCategory>

namespace OCC {

Q_LOGGING_CATEGORY(lcPatternDetector, "nextcloud.sync.killswitch.pattern", QtInfoMsg)

PatternDetector::PatternDetector()
{
    initializePatterns();
}

void PatternDetector::initializePatterns()
{
    // Known ransomware extensions - comprehensive list
    // Sources: Recorded Future, ID Ransomware, Malwarebytes threat intelligence
    m_ransomwareExtensions = {
        // Generic encryption extensions
        QStringLiteral(".locked"),
        QStringLiteral(".encrypted"),
        QStringLiteral(".enc"),
        QStringLiteral(".crypt"),
        QStringLiteral(".crypto"),
        QStringLiteral(".crypted"),
        QStringLiteral(".cry"),
        QStringLiteral(".crinf"),
        QStringLiteral(".r5a"),
        QStringLiteral(".xrnt"),
        QStringLiteral(".xtbl"),
        QStringLiteral(".crypz"),
        QStringLiteral(".xxx"),
        QStringLiteral(".aaa"),
        QStringLiteral(".abc"),
        QStringLiteral(".xyz"),
        QStringLiteral(".zzz"),
        QStringLiteral(".micro"),
        QStringLiteral(".ecc"),
        QStringLiteral(".ezz"),
        QStringLiteral(".exx"),
        QStringLiteral(".bleep"),

        // Named ransomware families
        QStringLiteral(".wannacry"),
        QStringLiteral(".wncry"),
        QStringLiteral(".wcry"),
        QStringLiteral(".wncryt"),
        QStringLiteral(".locky"),
        QStringLiteral(".odin"),
        QStringLiteral(".zepto"),
        QStringLiteral(".osiris"),
        QStringLiteral(".aesir"),
        QStringLiteral(".thor"),
        QStringLiteral(".cerber"),
        QStringLiteral(".cerber2"),
        QStringLiteral(".cerber3"),
        QStringLiteral(".petya"),
        QStringLiteral(".notpetya"),
        QStringLiteral(".goldeneye"),
        QStringLiteral(".conti"),
        QStringLiteral(".ryuk"),
        QStringLiteral(".maze"),
        QStringLiteral(".lockbit"),
        QStringLiteral(".revil"),
        QStringLiteral(".sodinokibi"),
        QStringLiteral(".darkside"),
        QStringLiteral(".ragnar"),
        QStringLiteral(".avaddon"),
        QStringLiteral(".babuk"),
        QStringLiteral(".clop"),
        QStringLiteral(".egregor"),
        QStringLiteral(".netwalker"),
        QStringLiteral(".phobos"),
        QStringLiteral(".dharma"),
        QStringLiteral(".crysis"),
        QStringLiteral(".globe"),
        QStringLiteral(".teslacrypt"),

        // STOP/Djvu family (very common)
        QStringLiteral(".stop"),
        QStringLiteral(".djvu"),
        QStringLiteral(".djvuq"),
        QStringLiteral(".djvur"),
        QStringLiteral(".djvus"),
        QStringLiteral(".djvut"),
        QStringLiteral(".pdff"),
        QStringLiteral(".pdfn"),
        QStringLiteral(".desu"),
        QStringLiteral(".boot"),
        QStringLiteral(".nood"),
        QStringLiteral(".kook"),
        QStringLiteral(".gero"),
        QStringLiteral(".hese"),
        QStringLiteral(".seto"),
        QStringLiteral(".mado"),
        QStringLiteral(".jope"),
        QStringLiteral(".nppp"),
        QStringLiteral(".remk"),
        QStringLiteral(".lmas"),
        QStringLiteral(".boza"),
        QStringLiteral(".boty"),
        QStringLiteral(".kiop"),

        // Other families
        QStringLiteral(".vvv"),
        QStringLiteral(".ccc"),
        QStringLiteral(".rrr"),
        QStringLiteral(".ttt"),
        QStringLiteral(".wallet"),
        QStringLiteral(".arena"),
        QStringLiteral(".java"),  // Java ransomware, not Java files
        QStringLiteral(".onion"),
        QStringLiteral(".btc"),
        QStringLiteral(".nochance"),
        QStringLiteral(".paycrypt"),
        QStringLiteral(".serpent"),
        QStringLiteral(".venom"),
        QStringLiteral(".damage"),
        QStringLiteral(".fucked"),
        QStringLiteral(".rip"),
        QStringLiteral(".rdmk"),
        QStringLiteral(".helpme"),
    };

    // Ransom note patterns (case insensitive)
    m_ransomNotePatterns = {
        QRegularExpression(QStringLiteral("^readme.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^how[_\\-\\s]?to[_\\-\\s]?decrypt.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^how[_\\-\\s]?to[_\\-\\s]?restore.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^how[_\\-\\s]?to[_\\-\\s]?recover.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^decrypt[_\\-\\s]?instructions.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^restore[_\\-\\s]?files.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^your[_\\-\\s]?files.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^files[_\\-\\s]?encrypted.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^ransom.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^_readme[_\\-]?\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^!readme!?\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^read[_\\-\\s]?me[_\\-\\s]?now.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^warning.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^decrypt[_\\-]?all.*\\.(txt|html)$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("^unlock[_\\-]?instructions.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("@please_read_me@\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        // WannaCry specific
        QRegularExpression(QStringLiteral("@wannacry@\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("@wanadecryptor@\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        // Locky specific
        QRegularExpression(QStringLiteral("_locky_recover.*\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        // LockBit specific
        QRegularExpression(QStringLiteral("restore-my-files\\.txt$"), QRegularExpression::CaseInsensitiveOption),
        // Conti specific
        QRegularExpression(QStringLiteral("readme\\.conti\\.txt$"), QRegularExpression::CaseInsensitiveOption),
    };

    // Normal file extensions (for double extension detection)
    m_normalExtensions = {
        QStringLiteral(".pdf"),
        QStringLiteral(".doc"),
        QStringLiteral(".docx"),
        QStringLiteral(".xls"),
        QStringLiteral(".xlsx"),
        QStringLiteral(".ppt"),
        QStringLiteral(".pptx"),
        QStringLiteral(".jpg"),
        QStringLiteral(".jpeg"),
        QStringLiteral(".png"),
        QStringLiteral(".gif"),
        QStringLiteral(".bmp"),
        QStringLiteral(".mp3"),
        QStringLiteral(".mp4"),
        QStringLiteral(".avi"),
        QStringLiteral(".mov"),
        QStringLiteral(".txt"),
        QStringLiteral(".zip"),
        QStringLiteral(".rar"),
        QStringLiteral(".7z"),
        QStringLiteral(".csv"),
        QStringLiteral(".html"),
        QStringLiteral(".xml"),
        QStringLiteral(".json"),
        QStringLiteral(".sql"),
        QStringLiteral(".db"),
        QStringLiteral(".psd"),
        QStringLiteral(".ai"),
        QStringLiteral(".odt"),
        QStringLiteral(".ods"),
        QStringLiteral(".odp"),
    };

    qCInfo(lcPatternDetector) << "Initialized with"
                              << m_ransomwareExtensions.size() << "ransomware extensions,"
                              << m_ransomNotePatterns.size() << "ransom note patterns";
}

ThreatInfo PatternDetector::analyze(const SyncFileItem &item,
                                    const QVector<struct KillSwitchManager::Event> &recentEvents)
{
    ThreatInfo threat;
    threat.level = ThreatLevel::None;
    threat.detectorName = name();

    // Only check new or modified files
    if (item._instruction != CSYNC_INSTRUCTION_NEW &&
        item._instruction != CSYNC_INSTRUCTION_SYNC) {
        return threat;
    }

    const QString filePath = item._file;
    const QString fileName = QFileInfo(filePath).fileName();

    // Check for ransom note - CRITICAL threat (immediate trigger)
    if (isRansomNote(fileName)) {
        threat.level = ThreatLevel::Critical;
        threat.description = QStringLiteral("Ransom note detected: %1").arg(fileName);
        threat.affectedFiles.append(filePath);
        qCWarning(lcPatternDetector) << "CRITICAL: Ransom note detected:" << fileName;
        return threat;
    }

    // Check for ransomware extension
    bool hasRansomExt = hasRansomwareExtension(filePath);
    bool hasDoubleExt = hasDoubleExtension(fileName);

    if (!hasRansomExt && !hasDoubleExt) {
        return threat; // No suspicious pattern
    }

    // Count suspicious files in time window
    int suspiciousCount = countSuspiciousFiles(recentEvents);

    // Add current file to count if suspicious
    if (hasRansomExt || hasDoubleExt) {
        suspiciousCount++;
    }

    // Determine threat level based on count
    if (suspiciousCount >= m_threshold * 2) {
        threat.level = ThreatLevel::Critical;
        threat.description = QStringLiteral("Mass ransomware encryption detected: %1 suspicious files")
                                 .arg(suspiciousCount);
    } else if (suspiciousCount >= m_threshold) {
        threat.level = ThreatLevel::High;
        threat.description = QStringLiteral("Multiple ransomware patterns detected: %1 suspicious files")
                                 .arg(suspiciousCount);
    } else if (hasDoubleExt) {
        threat.level = ThreatLevel::Medium;
        threat.description = QStringLiteral("Suspicious double extension: %1").arg(fileName);
    } else {
        threat.level = ThreatLevel::Low;
        threat.description = QStringLiteral("Suspicious ransomware extension: %1").arg(fileName);
    }

    threat.affectedFiles.append(filePath);

    qCInfo(lcPatternDetector) << "Pattern detected:" << threat.description
                              << "- Level:" << static_cast<int>(threat.level);

    return threat;
}

bool PatternDetector::hasRansomwareExtension(const QString &filePath) const
{
    const QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension.isEmpty()) {
        return false;
    }
    return m_ransomwareExtensions.contains(QStringLiteral(".") + extension);
}

bool PatternDetector::isRansomNote(const QString &fileName) const
{
    for (const auto &pattern : m_ransomNotePatterns) {
        if (pattern.match(fileName).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool PatternDetector::hasDoubleExtension(const QString &fileName) const
{
    // Look for pattern like "document.pdf.locked"
    // Split on dots and check if second-to-last is a normal extension
    // and last is a ransomware extension

    const QStringList parts = fileName.split(QLatin1Char('.'));
    if (parts.size() < 3) {
        return false; // Need at least name.ext1.ext2
    }

    const QString lastExt = QStringLiteral(".") + parts.last().toLower();
    const QString secondLastExt = QStringLiteral(".") + parts[parts.size() - 2].toLower();

    // Check if last extension is ransomware and second-to-last is normal
    return m_ransomwareExtensions.contains(lastExt) &&
           m_normalExtensions.contains(secondLastExt);
}

int PatternDetector::countSuspiciousFiles(const QVector<struct KillSwitchManager::Event> &recentEvents) const
{
    int count = 0;
    for (const auto &event : recentEvents) {
        if (event.type == QStringLiteral("CREATE") ||
            event.type == QStringLiteral("MODIFY")) {
            if (hasRansomwareExtension(event.path) ||
                hasDoubleExtension(QFileInfo(event.path).fileName()) ||
                isRansomNote(QFileInfo(event.path).fileName())) {
                count++;
            }
        }
    }
    return count;
}

void PatternDetector::addCustomExtension(const QString &extension)
{
    QString ext = extension.toLower();
    if (!ext.startsWith(QLatin1Char('.'))) {
        ext.prepend(QLatin1Char('.'));
    }
    m_ransomwareExtensions.insert(ext);
    qCInfo(lcPatternDetector) << "Added custom extension:" << ext;
}

void PatternDetector::addCustomPattern(const QString &pattern)
{
    QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
    if (regex.isValid()) {
        m_ransomNotePatterns.append(regex);
        qCInfo(lcPatternDetector) << "Added custom pattern:" << pattern;
    } else {
        qCWarning(lcPatternDetector) << "Invalid regex pattern:" << pattern;
    }
}

} // namespace OCC

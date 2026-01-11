/*
 * Nextcloud Sentinel Edition - Kill Switch Unit Tests
 *
 * SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>
#include <QSignalSpy>

#include "libsync/killswitch/killswitchmanager.h"
#include "libsync/killswitch/detectors/massdeletedetector.h"
#include "libsync/killswitch/detectors/entropydetector.h"
#include "libsync/killswitch/detectors/canarydetector.h"
#include "libsync/killswitch/detectors/patterndetector.h"
#include "libsync/syncfileitem.h"

using namespace OCC;

class TestKillSwitch : public QObject
{
    Q_OBJECT

private:
    KillSwitchManager *_manager = nullptr;

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void init()
    {
        _manager = new KillSwitchManager(this);
    }

    void cleanup()
    {
        delete _manager;
        _manager = nullptr;
    }

    // ==================== KillSwitchManager Tests ====================

    void testManagerInitialization()
    {
        QVERIFY(_manager != nullptr);
        QVERIFY(_manager->isEnabled());
        QVERIFY(!_manager->isTriggered());
        QCOMPARE(_manager->currentThreatLevel(), ThreatLevel::None);
    }

    void testManagerEnableDisable()
    {
        QSignalSpy enabledSpy(_manager, &KillSwitchManager::enabledChanged);

        _manager->setEnabled(false);
        QVERIFY(!_manager->isEnabled());
        QCOMPARE(enabledSpy.count(), 1);

        _manager->setEnabled(true);
        QVERIFY(_manager->isEnabled());
        QCOMPARE(enabledSpy.count(), 2);
    }

    void testManagerTrigger()
    {
        QSignalSpy triggeredSpy(_manager, &KillSwitchManager::triggeredChanged);
        QSignalSpy pausedSpy(_manager, &KillSwitchManager::syncPaused);

        _manager->trigger("Test trigger reason");

        QVERIFY(_manager->isTriggered());
        QCOMPARE(_manager->currentThreatLevel(), ThreatLevel::Critical);
        QCOMPARE(triggeredSpy.count(), 1);
        QCOMPARE(pausedSpy.count(), 1);

        // Verify the reason is passed
        QList<QVariant> arguments = pausedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("Test trigger reason"));
    }

    void testManagerReset()
    {
        _manager->trigger("Test");
        QVERIFY(_manager->isTriggered());

        QSignalSpy resumedSpy(_manager, &KillSwitchManager::syncResumed);

        _manager->reset();

        QVERIFY(!_manager->isTriggered());
        QCOMPARE(_manager->currentThreatLevel(), ThreatLevel::None);
        QCOMPARE(resumedSpy.count(), 1);
    }

    void testManagerDoubleTrigger()
    {
        QSignalSpy triggeredSpy(_manager, &KillSwitchManager::triggeredChanged);

        _manager->trigger("First trigger");
        _manager->trigger("Second trigger");

        // Should only trigger once
        QCOMPARE(triggeredSpy.count(), 1);
    }

    void testRegisterDetector()
    {
        auto detector = std::make_shared<MassDeleteDetector>();
        _manager->registerDetector(detector);

        // Detector should be registered (we can't directly verify but at least no crash)
        QVERIFY(true);
    }

    void testThresholdConfiguration()
    {
        _manager->setDeleteThreshold(20, 120);
        _manager->setEntropyThreshold(7.8);
        _manager->addCanaryFile("test_canary.txt");

        // No crash means success
        QVERIFY(true);
    }

    // ==================== MassDeleteDetector Tests ====================

    void testMassDeleteDetectorCreation()
    {
        MassDeleteDetector detector;
        QCOMPARE(detector.name(), QString("MassDeleteDetector"));
        QVERIFY(detector.isEnabled());
    }

    void testMassDeleteDetectorDisabled()
    {
        MassDeleteDetector detector;
        detector.setEnabled(false);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testMassDeleteDetectorNoThreat()
    {
        MassDeleteDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        // Only 2 delete events - below threshold
        QVector<KillSwitchManager::Event> events;
        events.append({QDateTime::currentDateTime(), "DELETE", "file1.txt"});
        events.append({QDateTime::currentDateTime(), "DELETE", "file2.txt"});

        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testMassDeleteDetectorHighThreat()
    {
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        // 6 delete events - above threshold
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 6; i++) {
            events.append({QDateTime::currentDateTime(), "DELETE", QString("file%1.txt").arg(i)});
        }

        ThreatInfo result = detector.analyze(item, events);

        QVERIFY(result.level >= ThreatLevel::High);
        QCOMPARE(result.detectorName, QString("MassDeleteDetector"));
    }

    void testMassDeleteDetectorCriticalThreat()
    {
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        // 12 delete events - double threshold = critical
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 12; i++) {
            events.append({QDateTime::currentDateTime(), "DELETE", QString("file%1.txt").arg(i)});
        }

        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::Critical);
    }

    void testMassDeleteDetectorIgnoresNonDelete()
    {
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_SYNC; // Not a delete
        item._file = "test.txt";

        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 10; i++) {
            events.append({QDateTime::currentDateTime(), "DELETE", QString("file%1.txt").arg(i)});
        }

        ThreatInfo result = detector.analyze(item, events);

        // Should not trigger because current item is not a delete
        QCOMPARE(result.level, ThreatLevel::None);
    }

    // ==================== CanaryDetector Tests ====================

    void testCanaryDetectorCreation()
    {
        CanaryDetector detector;
        QCOMPARE(detector.name(), QString("CanaryDetector"));
        QVERIFY(detector.isEnabled());

        // Should have default canary patterns
        QVERIFY(detector.canaryPatterns().contains("_canary.txt"));
        QVERIFY(detector.canaryPatterns().contains(".canary"));
    }

    void testCanaryDetectorIsCanaryFile()
    {
        CanaryDetector detector;

        QVERIFY(detector.isCanaryFile("_canary.txt"));
        QVERIFY(detector.isCanaryFile("path/to/_canary.txt"));
        QVERIFY(detector.isCanaryFile(".canary"));
        QVERIFY(detector.isCanaryFile("folder/.canary"));

        QVERIFY(!detector.isCanaryFile("normal_file.txt"));
        QVERIFY(!detector.isCanaryFile("canary_backup.txt"));
    }

    void testCanaryDetectorAddRemovePattern()
    {
        CanaryDetector detector;

        detector.addCanaryPattern("my_honeypot.txt");
        QVERIFY(detector.isCanaryFile("my_honeypot.txt"));

        detector.removeCanaryPattern("my_honeypot.txt");
        QVERIFY(!detector.isCanaryFile("my_honeypot.txt"));
    }

    void testCanaryDetectorTriggerOnDelete()
    {
        CanaryDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "_canary.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::Critical);
        QVERIFY(result.description.contains("DELETED"));
    }

    void testCanaryDetectorTriggerOnModify()
    {
        CanaryDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_SYNC;
        item._file = "_canary.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::Critical);
        QVERIFY(result.description.contains("MODIFIED"));
    }

    void testCanaryDetectorIgnoresNewCanary()
    {
        CanaryDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "_canary.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        // New canary file is OK (initial setup)
        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testCanaryDetectorIgnoresNormalFiles()
    {
        CanaryDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "normal_document.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::None);
    }

    // ==================== EntropyDetector Tests ====================

    void testEntropyDetectorCreation()
    {
        EntropyDetector detector;
        QCOMPARE(detector.name(), QString("EntropyDetector"));
        QVERIFY(detector.isEnabled());
    }

    void testEntropyCalculationEmpty()
    {
        QByteArray empty;
        double entropy = EntropyDetector::calculateEntropy(empty);
        QCOMPARE(entropy, 0.0);
    }

    void testEntropyCalculationUniform()
    {
        // Single repeated byte - entropy should be 0
        QByteArray uniform(1000, 'A');
        double entropy = EntropyDetector::calculateEntropy(uniform);
        QCOMPARE(entropy, 0.0);
    }

    void testEntropyCalculationLowEntropy()
    {
        // Simple text - low entropy (3-5 bits)
        QByteArray text = "Hello World! This is a simple test message with repeated words. "
                          "Hello World! This is a simple test message with repeated words.";
        double entropy = EntropyDetector::calculateEntropy(text);

        QVERIFY(entropy > 2.0);
        QVERIFY(entropy < 5.5);
    }

    void testEntropyCalculationHighEntropy()
    {
        // Random-looking data - high entropy (close to 8)
        QByteArray random;
        random.reserve(1000);
        for (int i = 0; i < 1000; i++) {
            random.append(static_cast<char>(i % 256));
        }
        double entropy = EntropyDetector::calculateEntropy(random);

        QVERIFY(entropy > 7.0);
    }

    void testEntropyCalculationMaxEntropy()
    {
        // Perfect distribution of all 256 byte values
        QByteArray perfect;
        for (int i = 0; i < 256; i++) {
            perfect.append(static_cast<char>(i));
        }
        double entropy = EntropyDetector::calculateEntropy(perfect);

        // Should be exactly 8.0 bits
        QVERIFY(qAbs(entropy - 8.0) < 0.001);
    }

    void testEntropyDetectorWhitelist()
    {
        // Compressed/media files should be whitelisted
        EntropyDetector detector;

        // These should be recognized as normally high-entropy
        QVERIFY(detector.isNormallyHighEntropy("image.jpg"));
        QVERIFY(detector.isNormallyHighEntropy("video.mp4"));
        QVERIFY(detector.isNormallyHighEntropy("archive.zip"));
        QVERIFY(detector.isNormallyHighEntropy("document.pdf"));
        QVERIFY(detector.isNormallyHighEntropy("compressed.7z"));

        // These should NOT be whitelisted
        QVERIFY(!detector.isNormallyHighEntropy("script.py"));
        QVERIFY(!detector.isNormallyHighEntropy("code.cpp"));
        QVERIFY(!detector.isNormallyHighEntropy("readme.txt"));
        QVERIFY(!detector.isNormallyHighEntropy("data.csv"));
    }

    void testEntropyDetectorExpectedRange()
    {
        EntropyDetector detector;

        // Test expected ranges for different file types
        auto textRange = detector.expectedEntropyRange("readme.txt");
        QVERIFY(textRange.first >= 2.0 && textRange.first <= 4.0);
        QVERIFY(textRange.second >= 5.0 && textRange.second <= 6.0);

        auto codeRange = detector.expectedEntropyRange("main.cpp");
        QVERIFY(codeRange.first >= 3.0 && codeRange.first <= 5.0);
        QVERIFY(codeRange.second >= 5.5 && codeRange.second <= 7.0);

        auto unknownRange = detector.expectedEntropyRange("mystery.xyz");
        QCOMPARE(unknownRange.first, 0.0);
        QCOMPARE(unknownRange.second, 8.0);
    }

    // ==================== PatternDetector Tests ====================

    void testPatternDetectorCreation()
    {
        PatternDetector detector;
        QCOMPARE(detector.name(), QString("PatternDetector"));
        QVERIFY(detector.isEnabled());
    }

    void testPatternDetectorRansomwareExtensions()
    {
        PatternDetector detector;

        // Common ransomware extensions
        QVERIFY(detector.hasRansomwareExtension("document.locked"));
        QVERIFY(detector.hasRansomwareExtension("file.encrypted"));
        QVERIFY(detector.hasRansomwareExtension("photo.cry"));
        QVERIFY(detector.hasRansomwareExtension("data.wannacry"));
        QVERIFY(detector.hasRansomwareExtension("backup.locky"));
        QVERIFY(detector.hasRansomwareExtension("report.cerber"));
        QVERIFY(detector.hasRansomwareExtension("spreadsheet.conti"));
        QVERIFY(detector.hasRansomwareExtension("document.ryuk"));

        // STOP/Djvu family
        QVERIFY(detector.hasRansomwareExtension("file.stop"));
        QVERIFY(detector.hasRansomwareExtension("file.djvu"));

        // Normal extensions - should NOT match
        QVERIFY(!detector.hasRansomwareExtension("document.pdf"));
        QVERIFY(!detector.hasRansomwareExtension("image.jpg"));
        QVERIFY(!detector.hasRansomwareExtension("video.mp4"));
        QVERIFY(!detector.hasRansomwareExtension("code.cpp"));
        QVERIFY(!detector.hasRansomwareExtension("archive.zip"));
    }

    void testPatternDetectorRansomNotes()
    {
        PatternDetector detector;

        // Common ransom note names
        QVERIFY(detector.isRansomNote("README.txt"));
        QVERIFY(detector.isRansomNote("readme.txt"));
        QVERIFY(detector.isRansomNote("HOW_TO_DECRYPT.txt"));
        QVERIFY(detector.isRansomNote("How-to-restore.txt"));
        QVERIFY(detector.isRansomNote("DECRYPT_INSTRUCTIONS.txt"));
        QVERIFY(detector.isRansomNote("_readme_.txt"));
        QVERIFY(detector.isRansomNote("!README!.txt"));
        QVERIFY(detector.isRansomNote("RESTORE-MY-FILES.txt"));

        // Normal files - should NOT match
        QVERIFY(!detector.isRansomNote("document.txt"));
        QVERIFY(!detector.isRansomNote("notes.txt"));
        QVERIFY(!detector.isRansomNote("readme_project.txt"));
        QVERIFY(!detector.isRansomNote("config.txt"));
    }

    void testPatternDetectorDoubleExtension()
    {
        PatternDetector detector;

        // Suspicious double extensions
        QVERIFY(detector.hasDoubleExtension("document.pdf.locked"));
        QVERIFY(detector.hasDoubleExtension("report.docx.encrypted"));
        QVERIFY(detector.hasDoubleExtension("image.jpg.cry"));
        QVERIFY(detector.hasDoubleExtension("data.xlsx.wannacry"));
        QVERIFY(detector.hasDoubleExtension("backup.zip.cerber"));

        // Normal files - should NOT match
        QVERIFY(!detector.hasDoubleExtension("document.pdf"));
        QVERIFY(!detector.hasDoubleExtension("archive.tar.gz")); // tar.gz is normal
        QVERIFY(!detector.hasDoubleExtension("file.backup.txt")); // Not ransomware ext
        QVERIFY(!detector.hasDoubleExtension("simple.locked")); // No normal ext before
    }

    void testPatternDetectorCriticalOnRansomNote()
    {
        PatternDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "HOW_TO_DECRYPT.txt";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::Critical);
        QVERIFY(result.description.contains("Ransom note"));
    }

    void testPatternDetectorLowThreatSingleFile()
    {
        PatternDetector detector;
        detector.setThreshold(3);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "document.locked";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        // Single suspicious file = Low threat
        QCOMPARE(result.level, ThreatLevel::Low);
    }

    void testPatternDetectorHighThreatMultipleFiles()
    {
        PatternDetector detector;
        detector.setThreshold(3);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "document4.locked";

        // Simulate 3 previous suspicious files
        QVector<KillSwitchManager::Event> events;
        events.append({QDateTime::currentDateTime(), "CREATE", "document1.locked"});
        events.append({QDateTime::currentDateTime(), "CREATE", "document2.locked"});
        events.append({QDateTime::currentDateTime(), "CREATE", "document3.locked"});

        ThreatInfo result = detector.analyze(item, events);

        // 4 suspicious files >= threshold = High threat
        QVERIFY(result.level >= ThreatLevel::High);
    }

    void testPatternDetectorIgnoresNormalFiles()
    {
        PatternDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "document.pdf";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testPatternDetectorIgnoresDeleteOperations()
    {
        PatternDetector detector;

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "document.locked";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        // Delete of suspicious file is not a threat (cleanup is OK)
        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testPatternDetectorMediumThreatDoubleExtension()
    {
        PatternDetector detector;
        detector.setThreshold(5); // High threshold so single file doesn't reach High

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "important_document.pdf.locked";

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        // Double extension = Medium threat
        QCOMPARE(result.level, ThreatLevel::Medium);
    }

    void testPatternDetectorAddCustomExtension()
    {
        PatternDetector detector;

        // Custom extension should not match initially
        QVERIFY(!detector.hasRansomwareExtension("file.myransomware"));

        // Add custom extension
        detector.addCustomExtension(".myransomware");

        // Now it should match
        QVERIFY(detector.hasRansomwareExtension("file.myransomware"));
    }

    void testPatternDetectorCaseSensitivity()
    {
        PatternDetector detector;

        // Extensions should be case-insensitive
        QVERIFY(detector.hasRansomwareExtension("file.LOCKED"));
        QVERIFY(detector.hasRansomwareExtension("file.Encrypted"));
        QVERIFY(detector.hasRansomwareExtension("file.WANNACRY"));

        // Ransom notes should be case-insensitive
        QVERIFY(detector.isRansomNote("README.TXT"));
        QVERIFY(detector.isRansomNote("How_To_Decrypt.TXT"));
    }

    void testPatternDetectorFullIntegration()
    {
        auto patternDetector = std::make_shared<PatternDetector>();
        _manager->registerDetector(patternDetector);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "HOW_TO_DECRYPT.txt";

        bool blocked = _manager->analyzeItem(item);

        QVERIFY(blocked);
        QVERIFY(_manager->isTriggered());
    }

    // ==================== Integration Tests ====================

    void testFullIntegrationMassDelete()
    {
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(3);
        _manager->registerDetector(massDeleteDetector);

        // Simulate 5 delete items
        for (int i = 0; i < 5; i++) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_REMOVE;
            item._file = QString("file%1.txt").arg(i);

            _manager->analyzeItem(item);

            if (_manager->isTriggered()) {
                break;
            }
        }

        QVERIFY(_manager->isTriggered());
    }

    void testFullIntegrationCanary()
    {
        auto canaryDetector = std::make_shared<CanaryDetector>();
        _manager->registerDetector(canaryDetector);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "_canary.txt";

        bool blocked = _manager->analyzeItem(item);

        QVERIFY(blocked);
        QVERIFY(_manager->isTriggered());
    }

    void testDisabledManagerDoesNotBlock()
    {
        auto canaryDetector = std::make_shared<CanaryDetector>();
        _manager->registerDetector(canaryDetector);
        _manager->setEnabled(false);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "_canary.txt";

        bool blocked = _manager->analyzeItem(item);

        QVERIFY(!blocked);
        QVERIFY(!_manager->isTriggered());
    }

    // ==================== False Positive Prevention Tests ====================

    void testFalsePositive_BatchUpload()
    {
        // Scenario: User uploads 20 new files rapidly (batch upload from phone)
        // Expected: No trigger - batch uploads are normal behavior
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(10);
        _manager->registerDetector(massDeleteDetector);

        // Batch of 20 CREATE operations - NOT deletions
        for (int i = 0; i < 20; i++) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_NEW;
            item._file = QString("photos/vacation/IMG_%1.jpg").arg(i, 4, 10, QChar('0'));

            _manager->analyzeItem(item);
        }

        // Should NOT trigger - these are creates, not deletes
        QVERIFY(!_manager->isTriggered());
    }

    void testFalsePositive_ArchiveExtraction()
    {
        // Scenario: User extracts a large ZIP file, creating many files at once
        // Expected: No trigger - archive extraction is normal
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(10);
        _manager->registerDetector(massDeleteDetector);

        // Simulate extraction: many new files
        QStringList extractedFiles = {
            "archive/README.md",
            "archive/src/main.cpp",
            "archive/src/utils.cpp",
            "archive/src/config.h",
            "archive/include/types.h",
            "archive/docs/manual.pdf",
            "archive/tests/test1.cpp",
            "archive/tests/test2.cpp",
            "archive/build/Makefile"
        };

        for (const auto &file : extractedFiles) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_NEW;
            item._file = file;
            _manager->analyzeItem(item);
        }

        QVERIFY(!_manager->isTriggered());
    }

    void testFalsePositive_BuildCleanup()
    {
        // Scenario: Developer runs "rm -rf node_modules" or "make clean"
        // Expected: Eventually triggers (protection against accidental rm -rf)
        // Note: This is intentionally a VALID trigger - we want to catch rm -rf
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(10);
        _manager->registerDetector(massDeleteDetector);

        // Simulate deleting node_modules (massive deletion)
        for (int i = 0; i < 15; i++) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_REMOVE;
            item._file = QString("project/node_modules/package%1/index.js").arg(i);
            _manager->analyzeItem(item);
        }

        // SHOULD trigger - mass deletion is dangerous even in node_modules
        // Users should use .gitignore or sync exclusions for build folders
        QVERIFY(_manager->isTriggered());
    }

    void testFalsePositive_TempFilesSystemCleanup()
    {
        // Scenario: System cleans temp files one by one (not mass deletion)
        // Expected: No trigger if below threshold
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(10);
        _manager->registerDetector(massDeleteDetector);

        // Only 5 temp file deletions - below threshold
        for (int i = 0; i < 5; i++) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_REMOVE;
            item._file = QString("temp/session_%1.tmp").arg(i);
            _manager->analyzeItem(item);
        }

        // Should NOT trigger - below threshold
        QVERIFY(!_manager->isTriggered());
    }

    void testFalsePositive_HighEntropyMediaFiles()
    {
        // Scenario: Syncing compressed media (JPEG, MP4) which has high entropy
        // Expected: No trigger - compressed media is normal
        EntropyDetector detector;

        // JPEG and MP4 are naturally high entropy - should be whitelisted
        QVERIFY(detector.isNormallyHighEntropy("photo.jpg"));
        QVERIFY(detector.isNormallyHighEntropy("video.mp4"));
        QVERIFY(detector.isNormallyHighEntropy("archive.zip"));
        QVERIFY(detector.isNormallyHighEntropy("compressed.7z"));
        QVERIFY(detector.isNormallyHighEntropy("document.pdf")); // PDFs often have compressed content
    }

    void testFalsePositive_CompressedArchivesNotRansomware()
    {
        // Scenario: User syncs legitimate compressed files
        // Expected: Pattern detector should NOT flag .zip, .7z as ransomware
        PatternDetector detector;

        // Normal archive extensions should NOT be flagged
        QVERIFY(!detector.hasRansomwareExtension("backup.zip"));
        QVERIFY(!detector.hasRansomwareExtension("data.7z"));
        QVERIFY(!detector.hasRansomwareExtension("archive.tar.gz"));
        QVERIFY(!detector.hasRansomwareExtension("files.rar"));
    }

    void testFalsePositive_GitOperations()
    {
        // Scenario: git checkout or branch switch causing many file changes
        // Expected: Modifications shouldn't trigger mass delete detector
        auto massDeleteDetector = std::make_shared<MassDeleteDetector>();
        massDeleteDetector->setThreshold(10);
        _manager->registerDetector(massDeleteDetector);

        // Simulate git checkout with many modified files
        for (int i = 0; i < 30; i++) {
            SyncFileItem item;
            item._instruction = CSYNC_INSTRUCTION_SYNC; // Modify, not delete
            item._file = QString("src/file%1.cpp").arg(i);
            _manager->analyzeItem(item);
        }

        // Should NOT trigger - modifications are not deletions
        QVERIFY(!_manager->isTriggered());
    }

    void testFalsePositive_RenameNotRansomware()
    {
        // Scenario: File renamed to have suspicious-looking extension legitimately
        // Expected: Single file rename should only be Low threat
        PatternDetector detector;
        detector.setThreshold(3);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_NEW;
        item._file = "my_locked_door_photo.jpg.locked"; // Legitimate filename? Suspicious!

        QVector<KillSwitchManager::Event> events;
        ThreatInfo result = detector.analyze(item, events);

        // Should be Medium (double extension) but not Critical
        // User can dismiss single false positive
        QVERIFY(result.level <= ThreatLevel::Medium);
        QVERIFY(result.level != ThreatLevel::Critical);
    }

    // ==================== Edge Case Tests ====================

    void testEdgeCase_EmptyFile()
    {
        // Empty files should have 0 entropy
        QByteArray empty;
        double entropy = EntropyDetector::calculateEntropy(empty);
        QCOMPARE(entropy, 0.0);
    }

    void testEdgeCase_SingleByteFile()
    {
        // Single byte file - entropy should be 0 (no randomness possible)
        QByteArray single;
        single.append('X');
        double entropy = EntropyDetector::calculateEntropy(single);
        QCOMPARE(entropy, 0.0);
    }

    void testEdgeCase_TwoBytesFile()
    {
        // Two different bytes - entropy should be 1.0 (log2(2))
        QByteArray two;
        two.append('A');
        two.append('B');
        double entropy = EntropyDetector::calculateEntropy(two);
        QVERIFY(qAbs(entropy - 1.0) < 0.001);
    }

    void testEdgeCase_LongFilePath()
    {
        // Windows MAX_PATH is 260 characters, but we should handle longer
        PatternDetector detector;

        QString longPath = "very/deep/nested/folder/structure/";
        for (int i = 0; i < 10; i++) {
            longPath += QString("level%1/").arg(i);
        }
        longPath += "document.locked";

        QVERIFY(longPath.length() > 100);
        QVERIFY(detector.hasRansomwareExtension(longPath));
    }

    void testEdgeCase_UnicodeFilename()
    {
        // Unicode characters in filename
        PatternDetector detector;

        // Japanese filename with ransomware extension
        QString unicodeFile = QString::fromUtf8("ドキュメント.locked");
        QVERIFY(detector.hasRansomwareExtension(unicodeFile));

        // Emoji in filename (should still detect extension)
        QVERIFY(detector.hasRansomwareExtension("my_docs_🔒.encrypted"));
    }

    void testEdgeCase_ExactThreshold()
    {
        // Test behavior exactly at threshold boundary
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        // Exactly 5 events (at threshold)
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 5; i++) {
            events.append({QDateTime::currentDateTime(), "DELETE", QString("file%1.txt").arg(i)});
        }

        ThreatInfo result = detector.analyze(item, events);

        // At exact threshold should trigger
        QVERIFY(result.level >= ThreatLevel::High);
    }

    void testEdgeCase_BelowThreshold()
    {
        // Test behavior just below threshold
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "test.txt";

        // Only 4 events (below threshold of 5)
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 4; i++) {
            events.append({QDateTime::currentDateTime(), "DELETE", QString("file%1.txt").arg(i)});
        }

        ThreatInfo result = detector.analyze(item, events);

        // Below threshold should not trigger high threat
        QVERIFY(result.level < ThreatLevel::High);
    }

    void testEdgeCase_RapidResetTrigger()
    {
        // Test trigger -> reset -> trigger sequence
        auto canaryDetector = std::make_shared<CanaryDetector>();
        _manager->registerDetector(canaryDetector);

        // First trigger
        SyncFileItem item1;
        item1._instruction = CSYNC_INSTRUCTION_REMOVE;
        item1._file = "_canary.txt";
        _manager->analyzeItem(item1);
        QVERIFY(_manager->isTriggered());

        // Reset
        _manager->reset();
        QVERIFY(!_manager->isTriggered());

        // Should be able to trigger again
        SyncFileItem item2;
        item2._instruction = CSYNC_INSTRUCTION_REMOVE;
        item2._file = ".canary";
        _manager->analyzeItem(item2);
        QVERIFY(_manager->isTriggered());
    }

    void testEdgeCase_MultipleDetectorsSameFile()
    {
        // File triggers multiple detectors
        auto patternDetector = std::make_shared<PatternDetector>();
        auto canaryDetector = std::make_shared<CanaryDetector>();

        _manager->registerDetector(patternDetector);
        _manager->registerDetector(canaryDetector);

        // This file is both a canary pattern AND has ransomware extension
        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_SYNC; // Modify
        item._file = "_canary.txt.encrypted";

        bool blocked = _manager->analyzeItem(item);

        // Should definitely be blocked
        QVERIFY(blocked);
        QVERIFY(_manager->isTriggered());
    }

    // ==================== MassDeleteDetector Whitelist Tests ====================

    void testMassDelete_WhitelistNodeModules()
    {
        MassDeleteDetector detector;

        // Paths inside node_modules should be whitelisted
        QVERIFY(detector.isWhitelisted("project/node_modules/lodash/index.js"));
        QVERIFY(detector.isWhitelisted("node_modules/react/package.json"));
        QVERIFY(detector.isWhitelisted("src/node_modules/lib.js"));
    }

    void testMassDelete_WhitelistBuildDirs()
    {
        MassDeleteDetector detector;

        // Build directories should be whitelisted
        QVERIFY(detector.isWhitelisted("project/build/output.js"));
        QVERIFY(detector.isWhitelisted("dist/bundle.min.js"));
        QVERIFY(detector.isWhitelisted("target/classes/Main.class"));
    }

    void testMassDelete_WhitelistGitDir()
    {
        MassDeleteDetector detector;

        // .git directories should be whitelisted
        QVERIFY(detector.isWhitelisted(".git/objects/pack/abc123"));
        QVERIFY(detector.isWhitelisted("project/.git/HEAD"));
    }

    void testMassDelete_WhitelistPythonCache()
    {
        MassDeleteDetector detector;

        // Python caches should be whitelisted
        QVERIFY(detector.isWhitelisted("src/__pycache__/module.cpython-39.pyc"));
        QVERIFY(detector.isWhitelisted(".pytest_cache/v/cache/nodeids"));
        QVERIFY(detector.isWhitelisted("venv/lib/python3.9/site-packages/pkg.py"));
    }

    void testMassDelete_NotWhitelisted()
    {
        MassDeleteDetector detector;

        // Regular files should NOT be whitelisted
        QVERIFY(!detector.isWhitelisted("src/main.cpp"));
        QVERIFY(!detector.isWhitelisted("documents/report.pdf"));
        QVERIFY(!detector.isWhitelisted("photos/vacation.jpg"));
        QVERIFY(!detector.isWhitelisted("config.json"));
    }

    void testMassDelete_CustomWhitelist()
    {
        MassDeleteDetector detector;

        // Add custom whitelist
        detector.addWhitelistedDirectory("my_temp_folder");

        QVERIFY(detector.isWhitelisted("project/my_temp_folder/data.txt"));
        QVERIFY(!detector.isWhitelisted("project/important_folder/data.txt"));
    }

    void testMassDelete_WhitelistCaseInsensitive()
    {
        MassDeleteDetector detector;

        // Whitelist should be case-insensitive
        QVERIFY(detector.isWhitelisted("project/NODE_MODULES/pkg/index.js"));
        QVERIFY(detector.isWhitelisted("project/Build/output.exe"));
        QVERIFY(detector.isWhitelisted("project/.GIT/config"));
    }

    void testMassDelete_WhitelistedNotCounted()
    {
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "project/node_modules/pkg/file.js";

        // 10 deletions but all in node_modules
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 10; i++) {
            events.append({
                QDateTime::currentDateTime(),
                "DELETE",
                QString("project/node_modules/pkg%1/index.js").arg(i)
            });
        }

        ThreatInfo result = detector.analyze(item, events);

        // Should not trigger because all paths are whitelisted
        QCOMPARE(result.level, ThreatLevel::None);
    }

    void testMassDelete_MixedWhitelistedAndNot()
    {
        MassDeleteDetector detector;
        detector.setThreshold(5);

        SyncFileItem item;
        item._instruction = CSYNC_INSTRUCTION_REMOVE;
        item._file = "src/important.cpp";

        // 3 whitelisted + 6 non-whitelisted = should trigger (6 >= 5)
        QVector<KillSwitchManager::Event> events;
        for (int i = 0; i < 3; i++) {
            events.append({
                QDateTime::currentDateTime(),
                "DELETE",
                QString("project/node_modules/pkg%1/index.js").arg(i)
            });
        }
        for (int i = 0; i < 6; i++) {
            events.append({
                QDateTime::currentDateTime(),
                "DELETE",
                QString("src/file%1.cpp").arg(i)
            });
        }

        ThreatInfo result = detector.analyze(item, events);

        // Should trigger because 6 non-whitelisted files >= threshold of 5
        QVERIFY(result.level >= ThreatLevel::High);
    }

    void testMassDelete_TreeDeletionDetection()
    {
        MassDeleteDetector detector;

        // All files under same directory = tree deletion
        QStringList paths = {
            "project/src/module/file1.cpp",
            "project/src/module/file2.cpp",
            "project/src/module/subdir/file3.cpp",
            "project/src/module/subdir/file4.cpp",
            "project/src/module/other/file5.cpp"
        };

        QString treeRoot = detector.detectTreeDeletion(paths);
        QVERIFY(!treeRoot.isEmpty());
        QVERIFY(treeRoot.contains("module") || treeRoot.contains("src"));
    }

    void testMassDelete_NoTreeDeletion()
    {
        MassDeleteDetector detector;

        // Files from different directories = no tree deletion
        QStringList paths = {
            "project1/file1.cpp",
            "project2/file2.cpp",
            "other/file3.cpp"
        };

        QString treeRoot = detector.detectTreeDeletion(paths);
        QVERIFY(treeRoot.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestKillSwitch)
#include "testkillswitch.moc"

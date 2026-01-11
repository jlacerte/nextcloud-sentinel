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
};

QTEST_GUILESS_MAIN(TestKillSwitch)
#include "testkillswitch.moc"

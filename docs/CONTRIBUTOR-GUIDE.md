# Nextcloud Sentinel - Contributor Guide

## Architecture Overview

The Kill Switch system follows a modular architecture with pluggable detectors.

```
src/libsync/killswitch/
├── killswitchmanager.h/cpp      # Orchestrator (singleton)
├── threatdetector.h              # Abstract interface
├── detectors/
│   ├── patterndetector.h/cpp    # Ransomware patterns
│   ├── massdeletedetector.h/cpp # Bulk deletion detection
│   ├── entropydetector.h/cpp    # Shannon entropy analysis
│   └── canarydetector.h/cpp     # Trap file monitoring
└── actions/
    └── syncaction.h              # Action interface

src/gui/killswitch/
├── killswitchsettings.h/cpp/ui  # Settings widget
├── killswitchdashboard.h/cpp/ui # Statistics dashboard
└── killswitchalertdialog.h/cpp  # Alert dialog

test/
└── testkillswitch.cpp           # 81 unit tests
```

---

## Core Concepts

### ThreatLevel Enum

```cpp
enum class ThreatLevel {
    None = 0,      // No threat
    Low = 1,       // Slightly suspicious
    Medium = 2,    // Moderately suspicious
    High = 3,      // Likely threat - triggers kill switch
    Critical = 4   // Definite threat - immediate trigger
};
```

### ThreatInfo Struct

```cpp
struct ThreatInfo {
    ThreatLevel level;
    QString description;
    QString detectorName;
    QStringList affectedFiles;
};
```

### ThreatDetector Interface

```cpp
class ThreatDetector {
public:
    virtual QString name() const = 0;
    virtual ThreatInfo analyze(const SyncFileItem &item,
                               const QVector<KillSwitchManager::Event> &recentEvents) = 0;

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

protected:
    bool m_enabled = true;
};
```

---

## Creating a New Detector

### Step 1: Create Header File

`src/libsync/killswitch/detectors/mydetector.h`:

```cpp
#pragma once

#include "../threatdetector.h"

namespace OCC {

class MyDetector : public ThreatDetector
{
public:
    MyDetector();

    QString name() const override { return QStringLiteral("MyDetector"); }

    ThreatInfo analyze(const SyncFileItem &item,
                       const QVector<KillSwitchManager::Event> &recentEvents) override;

private:
    // Your private members
    int m_threshold = 10;
};

} // namespace OCC
```

### Step 2: Create Implementation

`src/libsync/killswitch/detectors/mydetector.cpp`:

```cpp
#include "mydetector.h"
#include "syncfileitem.h"
#include <QLoggingCategory>

namespace OCC {

Q_LOGGING_CATEGORY(lcMyDetector, "nextcloud.sync.killswitch.mydetector", QtInfoMsg)

MyDetector::MyDetector()
{
    qCInfo(lcMyDetector) << "MyDetector initialized";
}

ThreatInfo MyDetector::analyze(const SyncFileItem &item,
                                const QVector<KillSwitchManager::Event> &recentEvents)
{
    ThreatInfo result;
    result.level = ThreatLevel::None;
    result.detectorName = name();

    if (!m_enabled) {
        return result;
    }

    // Your detection logic here
    // Example: Check file extension
    if (item._file.endsWith(".suspicious")) {
        result.level = ThreatLevel::High;
        result.description = QStringLiteral("Suspicious file detected: %1").arg(item._file);
        result.affectedFiles.append(item._file);
    }

    return result;
}

} // namespace OCC
```

### Step 3: Update CMakeLists.txt

In `src/libsync/CMakeLists.txt`, add your files:

```cmake
set(libsync_SRCS
    # ... existing files ...
    killswitch/detectors/mydetector.cpp
)
```

### Step 4: Register the Detector

In `src/gui/application.cpp` or `src/libsync/syncengine.cpp`:

```cpp
#include "killswitch/detectors/mydetector.h"

// In initialization code:
killSwitch->registerDetector(std::make_shared<MyDetector>());
```

**Important**: Register detectors in order of speed (fastest first):
1. PatternDetector (regex only)
2. CanaryDetector (path comparison)
3. MassDeleteDetector (event counting)
4. EntropyDetector (file I/O - slowest)

---

## Detector Best Practices

### Performance

1. **Avoid file I/O when possible** - Reading files is slow
2. **Use early returns** - Exit quickly if nothing suspicious
3. **Cache results** - EntropyDetector uses LRU cache
4. **Check `m_enabled` first** - Skip analysis if disabled

### Logging

Use Qt logging categories:

```cpp
Q_LOGGING_CATEGORY(lcMyDetector, "nextcloud.sync.killswitch.mydetector", QtInfoMsg)

// Debug (not shown by default)
qCDebug(lcMyDetector) << "Analyzing:" << item._file;

// Info (shown by default)
qCInfo(lcMyDetector) << "Initialized with threshold:" << m_threshold;

// Warning (for detected threats)
qCWarning(lcMyDetector) << "Threat detected:" << description;
```

### Thread Safety

KillSwitchManager handles thread safety. Your detector's `analyze()` method may be called from multiple threads. Use:

- `const` methods where possible
- `mutable` for caches with proper locking
- Avoid global/static state

---

## Writing Tests

### Test File Location

`test/testkillswitch.cpp`

### Test Structure

```cpp
void testMyDetector_BasicDetection()
{
    MyDetector detector;
    detector.setEnabled(true);

    SyncFileItem item;
    item._file = "test.suspicious";
    item._instruction = CSYNC_INSTRUCTION_NEW;

    QVector<KillSwitchManager::Event> events;

    ThreatInfo result = detector.analyze(item, events);

    QVERIFY(result.level == ThreatLevel::High);
    QVERIFY(result.description.contains("suspicious"));
}

void testMyDetector_IgnoresNormalFiles()
{
    MyDetector detector;

    SyncFileItem item;
    item._file = "document.txt";
    item._instruction = CSYNC_INSTRUCTION_NEW;

    QVector<KillSwitchManager::Event> events;

    ThreatInfo result = detector.analyze(item, events);

    QVERIFY(result.level == ThreatLevel::None);
}

void testMyDetector_DisabledDoesNothing()
{
    MyDetector detector;
    detector.setEnabled(false);

    SyncFileItem item;
    item._file = "test.suspicious";
    item._instruction = CSYNC_INSTRUCTION_NEW;

    QVector<KillSwitchManager::Event> events;

    ThreatInfo result = detector.analyze(item, events);

    QVERIFY(result.level == ThreatLevel::None);
}
```

### Running Tests

```bash
# Build with Docker
./build-wsl.sh --docker

# Run Kill Switch tests only
cd build-docker && ctest -R KillSwitch --output-on-failure

# Run all tests
cd build-docker && ctest --output-on-failure
```

---

## Event System

### Event Structure

```cpp
struct KillSwitchManager::Event {
    QString type;      // "DELETE", "CREATE", "MODIFY"
    QString path;      // File path
    QDateTime timestamp;
};
```

### Using Events in Detectors

```cpp
ThreatInfo MyDetector::analyze(const SyncFileItem &item,
                                const QVector<KillSwitchManager::Event> &recentEvents)
{
    // Count recent events of interest
    int relevantCount = 0;
    for (const auto &event : recentEvents) {
        if (event.type == "DELETE" && event.path.endsWith(".important")) {
            relevantCount++;
        }
    }

    if (relevantCount >= m_threshold) {
        // Return threat...
    }
}
```

**Note**: Events are pre-filtered by time window (default 60 seconds) by KillSwitchManager before being passed to detectors.

---

## UI Integration

### Adding Settings

1. Edit `src/gui/killswitch/killswitchsettings.ui` in Qt Designer
2. Add widget to form
3. Connect in `killswitchsettings.cpp`:

```cpp
connect(_ui->mySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &KillSwitchSettings::slotSettingsChanged);
```

### Dashboard Statistics

To add stats for your detector, update `KillSwitchDashboard`:

```cpp
void KillSwitchDashboard::updateStats()
{
    // Add your detector's stats
    auto myStats = _manager->getDetectorStats("MyDetector");
    _ui->myDetectorLabel->setText(QString::number(myStats.triggerCount));
}
```

---

## Code Style

- Use Qt naming conventions (camelCase for methods, m_ prefix for members)
- Use `QStringLiteral()` for string literals
- Use `auto` for complex types
- Document public methods with Doxygen comments
- Keep lines under 100 characters

---

## Checklist for New Detectors

- [ ] Header file with class declaration
- [ ] Implementation with `analyze()` method
- [ ] Logging category defined
- [ ] Added to CMakeLists.txt
- [ ] Registered in application initialization
- [ ] Unit tests (minimum 3: basic, negative, disabled)
- [ ] Documentation updated

---

## Questions?

- Open an issue on GitHub
- Check existing detector implementations for examples
- Review `testkillswitch.cpp` for test patterns

---

*Happy coding! - The Sentinel Team*

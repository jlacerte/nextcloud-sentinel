# Nextcloud Sentinel - User Guide

## What is Kill Switch?

Kill Switch is an anti-ransomware protection system built into Nextcloud Sentinel. It monitors file synchronization activity and automatically pauses sync when suspicious patterns are detected.

### Protection Against:

- **Ransomware attacks** - Detects encrypted files before they sync
- **Accidental mass deletion** - Catches `rm -rf` mistakes
- **Malicious encryption** - Identifies files with suspicious entropy
- **Ransom notes** - Blocks sync when ransom note files appear

---

## How It Works

Kill Switch uses four detection methods:

| Detector | What it detects | Speed |
|----------|-----------------|-------|
| **PatternDetector** | Ransomware file extensions (.locked, .encrypted, etc.) and ransom notes | Fastest |
| **CanaryDetector** | Modifications to trap files you place in folders | Fast |
| **MassDeleteDetector** | Bulk file deletions exceeding thresholds | Fast |
| **EntropyDetector** | Files with unusually high randomness (encrypted data) | Slower |

When a threat is detected at **High** or **Critical** level, sync is immediately paused and you receive an alert.

---

## Configuration

### Accessing Settings

1. Open Nextcloud Desktop Client
2. Go to **Settings** (gear icon)
3. Click **Kill Switch Protection**

### Presets

Choose a preset based on your needs:

| Preset | Delete Threshold | Time Window | Entropy | Best For |
|--------|------------------|-------------|---------|----------|
| **Light** | 20 files | 90 seconds | 7.9 | Developers with frequent bulk operations |
| **Standard** | 10 files | 60 seconds | 7.5 | Most users (recommended) |
| **Paranoid** | 5 files | 30 seconds | 7.2 | Maximum security |

### Custom Settings

- **Delete Threshold**: Number of deletions before triggering (default: 10)
- **Time Window**: Seconds to count deletions (default: 60)
- **Entropy Threshold**: Randomness level to flag (default: 7.5, max 8.0)

---

## Understanding Alerts

When Kill Switch triggers, you'll see an alert dialog:

### Threat Levels

| Level | Color | Meaning | Action |
|-------|-------|---------|--------|
| **Low** | Blue | Slightly suspicious | Logged only, sync continues |
| **Medium** | Yellow | Moderately suspicious | Warning shown, sync continues |
| **High** | Orange | Likely threat | **Sync paused**, review required |
| **Critical** | Red | Definite threat | **Sync paused**, immediate review |

### What To Do When Alerted

1. **Don't panic** - Sync is paused, your cloud data is safe
2. **Review the files** - Check the list of affected files in the alert
3. **Investigate** - Are these files you recognize? Did you delete them intentionally?
4. **Decide**:
   - If legitimate: Click "Resume Sync"
   - If suspicious: Click "Keep Paused" and investigate further

---

## False Positives

Some legitimate operations may trigger alerts. Kill Switch includes whitelists to reduce false positives:

### Whitelisted Directories (not counted as suspicious)

These directories are commonly bulk-deleted during development:

- `node_modules`, `.npm`, `.yarn`
- `build`, `dist`, `out`, `target`
- `.git`, `.svn`, `.hg`
- `__pycache__`, `.pytest_cache`, `venv`
- `.idea`, `.vscode`, `.vs`
- `.cache`, `.gradle`, `.m2`
- `tmp`, `temp`

### High-Entropy Files (not flagged)

These file types naturally have high entropy:

- Compressed: `.zip`, `.7z`, `.rar`, `.gz`
- Media: `.jpg`, `.png`, `.mp4`, `.mp3`
- Documents: `.pdf`, `.docx`, `.xlsx` (internally compressed)
- Already encrypted: `.gpg`, `.aes`

---

## Using Canary Files

Canary files are "trap" files that should never be modified. If ransomware encrypts them, Kill Switch triggers immediately.

### Setting Up Canary Files

1. Create a small text file named `_canary.txt` or `.canary`
2. Place it in important folders you want to protect
3. Never modify these files yourself

Example canary file content:
```
DO NOT MODIFY THIS FILE
This is a canary file for ransomware detection.
If this file changes, sync will be paused.
```

### How It Works

- CanaryDetector monitors files matching `*canary*` or `_canary*`
- Any modification or deletion triggers **Critical** alert
- Sync pauses immediately

---

## Dashboard

Access the Kill Switch Dashboard from Settings to view:

- **Files Analyzed**: Total files checked
- **Threats Blocked**: Number of threats stopped
- **False Positives**: Reports you've marked as false alarms
- **Top Detectors**: Which detectors trigger most often
- **Timeline**: Activity over 24h, 7 days, or 30 days

### Exporting Statistics

Click "Export CSV" to download statistics for analysis or reporting.

---

## Troubleshooting

### Kill Switch triggers too often

1. Switch to **Light** preset
2. Add frequently-deleted directories to whitelist
3. Review your workflow for bulk operations

### Kill Switch doesn't detect threats

1. Ensure Kill Switch is **Enabled** in settings
2. Switch to **Paranoid** preset for maximum detection
3. Check that detectors are registered (see Dashboard)

### Sync won't resume after alert

1. Click the Kill Switch icon in system tray
2. Select "Reset Kill Switch"
3. Review and dismiss any pending alerts
4. Sync will resume automatically

---

## FAQ

**Q: Does Kill Switch slow down sync?**
A: Minimal impact. PatternDetector (fastest) runs first and catches most threats. EntropyDetector (slowest) only runs if needed.

**Q: Can ransomware bypass Kill Switch?**
A: No protection is 100%. Kill Switch detects known patterns. Keep backups and stay vigilant.

**Q: What happens to files when sync pauses?**
A: Local files remain unchanged. Cloud files are not affected. Once you resume, sync continues normally.

**Q: Can I disable Kill Switch temporarily?**
A: Yes, toggle "Enable Kill Switch" in settings. Not recommended for extended periods.

**Q: How do I report a false positive?**
A: In the alert dialog, click "Report False Positive". This helps improve detection.

---

## Getting Help

- **Documentation**: https://github.com/nextcloud/desktop/wiki
- **Issues**: https://github.com/nextcloud/desktop/issues
- **Forum**: https://help.nextcloud.com

---

*Nextcloud Sentinel Edition - Protection You Can Trust*

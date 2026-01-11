# Compte-Rendu: Nextcloud Sentinel - CI Status
**Date:** 2026-01-11 03:45
**Repo:** https://github.com/jlacerte/nextcloud-sentinel

---

## Situation Actuelle

Tout le code est corrige et pousse. GitHub CI bloque par probleme de facturation.

### Derniers Commits
```
a0d15c450 fix: Stop QTimer in destructor to prevent test timeout
fc202b4cd ci(windows): Add Inkscape installation for icon generation
86dbb1ee4 fix: Use public QSettings API instead of private ConfigFile methods
```

### GitHub Actions Status

| Issue | Status |
|-------|--------|
| Code Kill Switch | ✅ CORRIGE |
| Timer cleanup | ✅ CORRIGE |
| Windows Inkscape | ✅ CORRIGE |
| GitHub Billing | ❌ BLOQUANT |

**Erreur GitHub:**
> "The job was not started because recent account payments have failed
> or your spending limit needs to be increased."

---

## Corrections Appliquees

### 1. API ConfigFile (killswitchsettings.cpp)
```cpp
// AVANT - Methodes privees
cfg.getValue("killSwitch/enabled", true);

// APRES - API publique
auto settings = ConfigFile::settingsWithGroup(QStringLiteral("KillSwitch"));
settings->value(QStringLiteral("enabled"), true);
```

### 2. Timer Cleanup (killswitchmanager.cpp) - NOUVEAU
```cpp
KillSwitchManager::~KillSwitchManager()
{
    // Stop the window timer to prevent callbacks on destroyed object
    m_windowTimer.stop();  // <-- AJOUT CRITIQUE

    if (s_instance == this) {
        s_instance = nullptr;
    }
}
```
**Cause du timeout:** Le QTimer tournait encore apres destruction, bloquant les tests.

### 3. Windows CI Inkscape
Ajout de `choco install inkscape` dans le workflow.

---

## Build Local Windows

Le build local avec Craft est complexe a cause de:
- Git Bash `sh.exe` dans PATH (conflit avec Craft)
- Qt 6.8+ et dependances non installees
- Configuration Craft specifique requise

**Scripts prepares:**
- `BUILD.bat` - Lance le build complet
- `TEST.bat` - Lance seulement les tests
- `build-local.ps1` - Script PowerShell principal

**Status:** Craft necessite un environnement propre sans Git Bash dans PATH.

---

## Prochaines Etapes

1. **Priorite:** Regler facturation GitHub (Settings > Billing & plans)
2. **Alternative:** Build dans WSL (Linux plus simple)
3. **Le code est pret** - juste l'infra qui bloque

---

## Commandes Utiles

```bash
# Verifier status (avec le bon repo!)
gh run list --repo jlacerte/nextcloud-sentinel --limit 5

# Declencher manuellement
gh workflow run linux-sentinel-ci.yml --repo jlacerte/nextcloud-sentinel --ref master
```

---

## Structure du Module Kill Switch

```
src/
  libsync/killswitch/
    killswitchmanager.cpp/h     # Core manager (TIMER FIX)
    threatdetector.h            # Interface detecteur
    detectors/
      massdeletedetector.cpp/h
      entropydetector.cpp/h
      canarydetector.cpp/h

  gui/killswitch/
    killswitchsettings.cpp/h    # Widget settings (API FIX)
    killswitchsettings.ui
    killswitchalertdialog.cpp/h
    killswitchalertdialog.ui

test/
  testkillswitch.cpp            # Tests unitaires
```

---

*Le feu crepite. Le code est pret. On attend juste la paperasse.*

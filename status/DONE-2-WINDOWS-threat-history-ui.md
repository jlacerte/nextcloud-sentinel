# DONE: UI Historique des Menaces

**Plateforme:** Windows (implémenté en Qt6/C++)
**Date complétée:** 2026-01-12
**Session:** Claude Code

---

## Résumé

L'interface ThreatHistoryDialog a été implémentée avec succès, permettant aux utilisateurs de visualiser l'historique complet des menaces détectées par le Kill Switch.

---

## Fichiers créés

| Fichier | Description |
|---------|-------------|
| `src/gui/killswitch/threathistorydialog.h` | Header avec namespace OCC correct |
| `src/gui/killswitch/threathistorydialog.cpp` | Implémentation complète |
| `src/gui/killswitch/threathistorydialog.ui` | Design Qt avec QTableWidget |

---

## Fichiers modifiés

| Fichier | Modifications |
|---------|---------------|
| `src/gui/killswitch/killswitchsettings.h` | Ajout slot `slotViewThreatHistory()` |
| `src/gui/killswitch/killswitchsettings.cpp` | Include + connection + implémentation |
| `src/gui/killswitch/killswitchsettings.ui` | Ajout bouton "View Full History..." |
| `src/gui/CMakeLists.txt` | Ajout des fichiers threathistorydialog |
| `src/libsync/CMakeLists.txt` | Ajout threatlogger.cpp/h et backupaction.cpp/h |

---

## Fonctionnalités implémentées

### 1. Liste des menaces
- QTableWidget avec colonnes: Date/Time, Level, Detector, Description, Files
- Tri par colonnes (sortingEnabled)
- Double-clic pour voir les détails

### 2. Filtrage par période
- 24 heures (défaut)
- 7 jours
- 30 jours
- Tout l'historique

### 3. Export CSV
- QFileDialog pour choisir l'emplacement
- Format CSV standard avec en-têtes
- Message de succès

### 4. Effacer l'historique
- QMessageBox de confirmation
- Suppression via ThreatLogger::clearLog()
- Rafraîchissement automatique

### 5. Indicateurs visuels
- Code couleur par niveau de menace:
  - Critical: Rouge (#DC3545)
  - High: Orange (#FD7E14)
  - Medium: Jaune (#FFC107)
  - Low: Bleu (#17A2B8)
- Timestamps formatés relativement ("Just now", "5 minutes ago", etc.)

### 6. Statistiques
- Compteur total de menaces affiché en bas
- Mis à jour automatiquement lors du filtrage

---

## Build & Tests

### Docker build (Linux):
```
[615/615] Linking CXX executable bin/UpdateChannelTest
[*] Running Kill Switch tests...
Test #20: KillSwitchTest ...................   Passed    0.59 sec
100% tests passed, 0 tests failed out of 1
[OK] Docker build completed!
```

### Fix technique important:
Les fichiers `threatlogger.cpp` et `backupaction.cpp` n'étaient pas inclus dans `src/libsync/CMakeLists.txt`, causant des erreurs de linkage. Corrigé en ajoutant:
```cmake
killswitch/actions/backupaction.h
killswitch/actions/backupaction.cpp
killswitch/threatlogger.h
killswitch/threatlogger.cpp
```

---

## Critères de succès

- [x] Dialog s'ouvre depuis les settings Kill Switch
- [x] Liste les menaces avec icônes couleur
- [x] Filtre par période fonctionne
- [x] Export CSV fonctionne
- [x] Effacer historique fonctionne (avec confirmation)
- [x] Statistiques affichées en bas
- [x] Build Docker passe
- [ ] Build Windows Craft (non testé - pas de CraftRoot)
- [ ] CI GitHub (à déclencher via push)

---

## Notes

Le pattern namespace Qt a été corrigé pour suivre le style du projet:
```cpp
namespace OCC {

namespace Ui {
    class ThreatHistoryDialog;
}

class ThreatHistoryDialog : public QDialog { ... };

} // namespace OCC
```

La classe UI dans le fichier .ui utilise `OCC::ThreatHistoryDialog` comme nom de classe pour matcher.

---

*Le feu crépite. L'historique est visible.*

# TÂCHE: UI Historique des Menaces

**Plateforme:** Windows (Craft/Qt Designer)
**Difficulté:** Moyenne-Haute
**Estimation:** 2-3 heures
**Priorité:** Haute (Phase 3 - UI)

---

## Objectif

Créer une interface utilisateur pour visualiser l'historique des menaces détectées par le Kill Switch. L'utilisateur doit pouvoir voir ce qui s'est passé, quand, et prendre des décisions éclairées.

---

## Contexte

Le `ThreatLogger` existe déjà et stocke toutes les menaces dans un fichier JSON:
- `%APPDATA%/Nextcloud/sentinel-threats.json`

Il manque une UI pour visualiser ces données.

---

## Maquette UI

```
┌─────────────────────────────────────────────────────────────┐
│ Historique des Menaces                              [X]     │
├─────────────────────────────────────────────────────────────┤
│ Période: [Dernières 24h ▼]  [Exporter CSV]  [Effacer]      │
├─────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 🔴 CRITICAL - 15:30:21                                  │ │
│ │ PatternDetector: Ransom note detected                   │ │
│ │ Fichiers: HOW_TO_DECRYPT.txt                           │ │
│ │ Action: sync_paused                                     │ │
│ ├─────────────────────────────────────────────────────────┤ │
│ │ 🟠 HIGH - 14:22:05                                      │ │
│ │ EntropyDetector: Entropy spike detected                 │ │
│ │ Fichiers: document.pdf, spreadsheet.xlsx               │ │
│ │ Action: detected                                        │ │
│ ├─────────────────────────────────────────────────────────┤ │
│ │ 🟡 MEDIUM - 12:15:33                                    │ │
│ │ MassDeleteDetector: 15 files deleted in 30s            │ │
│ │ Fichiers: (15 fichiers)                                │ │
│ │ Action: detected                                        │ │
│ └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ Statistiques: 3 menaces | 1 Critical | 1 High | 1 Medium   │
└─────────────────────────────────────────────────────────────┘
```

---

## Architecture existante

```
src/gui/killswitch/
├── killswitchsettings.cpp/h/ui    # Settings existants
├── killswitchalertdialog.cpp/h/ui # Dialog d'alerte existant
├── killswitchdashboard.cpp/h/ui   # Dashboard stats existant
```

Le `ThreatLogger` a déjà les méthodes:
```cpp
QVector<ThreatInfo> loadThreats() const;
QVector<ThreatInfo> threatsFromLastDays(int days) const;
bool exportToCsv(const QString &filePath) const;
void clearLog();
Statistics getStatistics() const;
```

---

## À implémenter

### 1. Créer `threathistorydialog.ui` (Qt Designer)

Composants:
- `QComboBox` pour la période (24h, 7 jours, 30 jours, Tout)
- `QPushButton` "Exporter CSV"
- `QPushButton` "Effacer historique"
- `QListWidget` ou `QTableView` pour la liste des menaces
- `QLabel` pour les statistiques en bas

### 2. Créer `threathistorydialog.h`

```cpp
#pragma once

#include <QDialog>
#include "libsync/killswitch/threatlogger.h"

namespace Ui {
class ThreatHistoryDialog;
}

namespace OCC {

class ThreatHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThreatHistoryDialog(QWidget *parent = nullptr);
    ~ThreatHistoryDialog() override;

private slots:
    void onPeriodChanged(int index);
    void onExportClicked();
    void onClearClicked();

private:
    void loadThreats();
    void updateStatistics();
    QString formatThreatLevel(ThreatLevel level) const;
    QIcon threatLevelIcon(ThreatLevel level) const;

    Ui::ThreatHistoryDialog *ui;
    int m_currentDays = 1; // 24h par défaut
};

} // namespace OCC
```

### 3. Créer `threathistorydialog.cpp`

Points clés:
- Charger les menaces depuis `ThreatLogger::instance()`
- Afficher avec icônes couleur selon le niveau
- Formater les timestamps en local
- Gérer l'export CSV avec `QFileDialog`
- Confirmer avant d'effacer l'historique

### 4. Ajouter au CMakeLists.txt

```cmake
# Dans src/gui/CMakeLists.txt, section killswitch
set(KILLSWITCH_UI_SOURCES
    ...
    killswitch/threathistorydialog.cpp
    killswitch/threathistorydialog.h
    killswitch/threathistorydialog.ui
)
```

### 5. Intégrer dans le menu/settings

Ajouter un bouton "Voir l'historique" dans:
- `killswitchsettings.ui` (bouton en bas)
- Ou dans le dashboard

```cpp
// Dans killswitchsettings.cpp
void KillSwitchSettings::onHistoryButtonClicked()
{
    ThreatHistoryDialog dialog(this);
    dialog.exec();
}
```

---

## Icônes par niveau

| Niveau | Couleur | Icône suggérée |
|--------|---------|----------------|
| Critical | 🔴 Rouge | `dialog-error` ou `security-high` |
| High | 🟠 Orange | `dialog-warning` |
| Medium | 🟡 Jaune | `dialog-information` |
| Low | 🔵 Bleu | `dialog-question` |

Utilise les icônes système Qt ou crée des QPixmap colorés.

---

## Build et test

```powershell
# Dans PowerShell Windows
cd D:\CraftRoot
.\craft\craftenv.ps1
craft --compile nextcloud-client

# Lancer le client pour tester l'UI
.\bin\nextcloud.exe
```

---

## Fichiers à créer/modifier

| Fichier | Action |
|---------|--------|
| `src/gui/killswitch/threathistorydialog.h` | Créer |
| `src/gui/killswitch/threathistorydialog.cpp` | Créer |
| `src/gui/killswitch/threathistorydialog.ui` | Créer |
| `src/gui/killswitch/killswitchsettings.cpp` | Modifier |
| `src/gui/killswitch/killswitchsettings.ui` | Modifier |
| `src/gui/CMakeLists.txt` | Modifier |

---

## Critères de succès

- [ ] Dialog s'ouvre depuis les settings Kill Switch
- [ ] Liste les menaces avec icônes couleur
- [ ] Filtre par période fonctionne
- [ ] Export CSV fonctionne
- [ ] Effacer historique fonctionne (avec confirmation)
- [ ] Statistiques affichées en bas
- [ ] Build Windows passe
- [ ] CI GitHub passe

---

## Ressources

- Regarde `killswitchdashboard.ui` pour le style existant
- `ThreatLogger` dans `src/libsync/killswitch/threatlogger.h`
- Qt Designer: `D:\CraftRoot\bin\designer.exe`

---

## Communication

Quand tu as fini ou si tu as des questions, crée un fichier:
`status/DONE-2-WINDOWS-threat-history-ui.md` ou `status/QUESTION-2-WINDOWS-threat-history-ui.md`

*Bonne chance! Le feu crépite.*

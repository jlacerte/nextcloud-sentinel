# TÂCHE: Implémenter BackupAction

**Plateforme:** WSL (Docker build)
**Difficulté:** Moyenne-Haute
**Estimation:** 2-3 heures
**Priorité:** Haute (Phase 3)

---

## Objectif

Créer une action qui sauvegarde automatiquement les fichiers menacés AVANT qu'ils ne soient supprimés/écrasés par la sync. C'est la dernière ligne de défense contre la perte de données.

---

## Contexte

Quand le Kill Switch détecte une menace (ransomware, suppression massive), il peut:
1. Bloquer l'opération ✅ (déjà fait)
2. Pauser la sync ✅ (déjà fait)
3. **Sauvegarder les fichiers menacés** ❌ (à faire)

---

## Architecture existante

```
src/libsync/killswitch/
├── killswitchmanager.cpp    # Coordonne les détecteurs et actions
├── actions/
│   └── syncaction.h         # Interface de base (existe déjà)
```

L'interface `SyncAction` existe déjà:

```cpp
// src/libsync/killswitch/actions/syncaction.h
class SyncAction {
public:
    virtual ~SyncAction() = default;
    virtual QString name() const = 0;
    virtual bool execute(const ThreatInfo &threat) = 0;
};
```

---

## À implémenter

### 1. Créer `backupaction.h`

```cpp
// src/libsync/killswitch/actions/backupaction.h
#pragma once

#include "syncaction.h"
#include <QString>

namespace OCC {

class BackupAction : public SyncAction
{
public:
    BackupAction();

    QString name() const override { return QStringLiteral("BackupAction"); }
    bool execute(const ThreatInfo &threat) override;

    // Configuration
    void setBackupDirectory(const QString &path);
    QString backupDirectory() const;

    void setMaxBackupSizeMB(int sizeMB);
    void setRetentionDays(int days);

private:
    bool backupFile(const QString &sourcePath);
    bool ensureBackupDirExists();
    void cleanOldBackups();
    QString generateBackupPath(const QString &originalPath);

    QString m_backupDir;
    int m_maxSizeMB = 500;      // Max 500MB de backups
    int m_retentionDays = 7;    // Garder 7 jours
};

} // namespace OCC
```

### 2. Créer `backupaction.cpp`

Points clés:
- Copier chaque fichier dans `threat.affectedFiles` vers le dossier backup
- Préserver la structure des dossiers (ex: `backup/2026-01-11/Documents/important.docx`)
- Gérer les erreurs (disque plein, permissions)
- Logger chaque backup via `Q_LOGGING_CATEGORY`
- Nettoyer les vieux backups (> retention days)

### 3. Ajouter au CMakeLists.txt

```cmake
# Dans src/libsync/killswitch/CMakeLists.txt
set(KILLSWITCH_SOURCES
    ...
    actions/backupaction.cpp
    actions/backupaction.h
)
```

### 4. Enregistrer dans application.cpp

```cpp
// Après l'initialisation des détecteurs
auto backupAction = std::make_shared<BackupAction>();
backupAction->setBackupDirectory(
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    + QStringLiteral("/sentinel-backups"));
killSwitch->registerAction(backupAction);
```

---

## Tests à écrire

Dans `test/testkillswitch.cpp`:

```cpp
void TestKillSwitch::testBackupAction_SingleFile()
void TestKillSwitch::testBackupAction_MultipleFiles()
void TestKillSwitch::testBackupAction_PreservesStructure()
void TestKillSwitch::testBackupAction_HandlesFullDisk()
void TestKillSwitch::testBackupAction_CleansOldBackups()
```

---

## Build et test

```bash
# Dans WSL
cd /mnt/d/nextcloud/nextcloud-sentinel
./build-wsl.sh --docker

# Les tests Kill Switch
docker exec -it nextcloud-build bash
cd /build && ctest -R KillSwitch --output-on-failure
```

---

## Fichiers à créer/modifier

| Fichier | Action |
|---------|--------|
| `src/libsync/killswitch/actions/backupaction.h` | Créer |
| `src/libsync/killswitch/actions/backupaction.cpp` | Créer |
| `src/libsync/killswitch/CMakeLists.txt` | Modifier |
| `src/gui/application.cpp` | Modifier |
| `test/testkillswitch.cpp` | Modifier |

---

## Critères de succès

- [ ] Les fichiers menacés sont copiés avant suppression
- [ ] La structure des dossiers est préservée
- [ ] Les vieux backups sont nettoyés automatiquement
- [ ] 5 tests unitaires passent
- [ ] Build Docker passe
- [ ] CI GitHub passe

---

## Ressources

- Regarde `ThreatLogger` pour un exemple de classe similaire
- `QFile::copy()` pour la copie
- `QDir::mkpath()` pour créer l'arborescence

---

## Communication

Quand tu as fini ou si tu as des questions, crée un fichier:
`status/DONE-1-WSL-backup-action.md` ou `status/QUESTION-1-WSL-backup-action.md`

*Bonne chance! Le feu crépite.*

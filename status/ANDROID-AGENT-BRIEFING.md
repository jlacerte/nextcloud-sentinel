# Nextcloud Sentinel - Briefing Agent Android

**Date:** 2026-01-11
**De:** Agent Desktop (Claude)
**Pour:** Agent Android
**Projet:** Portage du systeme Kill Switch vers Android

---

## Contexte: Qu'est-ce que Sentinel?

Nextcloud Sentinel est un **fork du client desktop Nextcloud** avec un module anti-ransomware appele "Kill Switch". L'objectif est de detecter les attaques ransomware en temps reel et de **couper la synchronisation avant que les fichiers chiffres ne se propagent au serveur**.

### Le probleme qu'on resout:
1. Un ransomware chiffre les fichiers locaux
2. Le client Nextcloud sync ces fichiers chiffres vers le serveur
3. Les fichiers sains sur le serveur sont ecrases par les versions chiffrees
4. Catastrophe: backup corrompu, propagation a tous les appareils

### Notre solution:
Detecter les patterns de ransomware **avant** la sync et bloquer.

---

## Architecture du Kill Switch (Desktop)

### Fichiers cles a etudier:

```
src/gui/killswitch/
├── KillSwitchManager.h/cpp      # Cerveau du systeme
├── KillSwitchConfig.h/cpp       # Configuration utilisateur
├── ThreatDetector.h/cpp         # Detection des menaces
├── FileAnalyzer.h/cpp           # Analyse des fichiers
└── AlertDialog.h/cpp            # UI alertes
```

### Pattern de detection:

1. **Monitoring des changements fichiers** - On observe les fichiers modifies
2. **Analyse d'entropie** - Fichiers chiffres = haute entropie (proche de 8.0)
3. **Detection de patterns** - Extensions suspectes (.encrypted, .locked, .cry)
4. **Seuils configurables** - X fichiers suspects en Y secondes = alerte
5. **Action** - Pause sync + notification utilisateur

### Metriques de detection:

```cpp
// Seuils par defaut (ajustables)
static constexpr double HIGH_ENTROPY_THRESHOLD = 7.5;  // Sur 8.0 max
static constexpr int SUSPICIOUS_FILE_COUNT = 5;        // Fichiers suspects
static constexpr int TIME_WINDOW_SECONDS = 60;         // Fenetre de temps
```

---

## Lecons apprises (Desktop) - IMPORTANT!

### 1. Threading et Mutex

**Probleme rencontre:** Deadlock dans KillSwitchManager quand plusieurs threads accedaient aux donnees.

**Solution:** Utiliser `QRecursiveMutex` au lieu de `QMutex`:
```cpp
// MAUVAIS - cause deadlock si meme thread re-entre
QMutex m_mutex;

// BON - permet re-entrance du meme thread
QRecursiveMutex m_mutex;
```

**Pour Android:** Attention aux coroutines Kotlin et aux acces concurrents. Utiliser `synchronized` ou `Mutex` de kotlinx.coroutines.

### 2. Cleanup des Timers

**Probleme:** Tests qui timeout parce que QTimer continue apres destruction.

**Solution:** Toujours arreter les timers dans le destructeur:
```cpp
KillSwitchManager::~KillSwitchManager() {
    if (m_analysisTimer) {
        m_analysisTimer->stop();
    }
}
```

**Pour Android:** Annuler les `Handler.postDelayed()` ou coroutines dans `onDestroy()`.

### 3. API Publiques vs Privees

**Probleme:** On utilisait `ConfigFile` (API privee Nextcloud) qui a change.

**Solution:** Utiliser `QSettings` directement (API Qt publique stable).

**Pour Android:** Utiliser `SharedPreferences` ou `DataStore` - APIs stables Android.

### 4. Calcul d'entropie

L'entropie de Shannon est notre detecteur principal:

```cpp
double FileAnalyzer::calculateEntropy(const QByteArray &data) {
    std::array<int, 256> frequency{};
    for (unsigned char byte : data) {
        frequency[byte]++;
    }

    double entropy = 0.0;
    const double dataSize = data.size();

    for (int count : frequency) {
        if (count > 0) {
            double probability = count / dataSize;
            entropy -= probability * std::log2(probability);
        }
    }
    return entropy;  // 0.0 (uniforme) a 8.0 (random/chiffre)
}
```

**Pour Android (Kotlin):**
```kotlin
fun calculateEntropy(data: ByteArray): Double {
    val frequency = IntArray(256)
    data.forEach { frequency[it.toInt() and 0xFF]++ }

    return frequency
        .filter { it > 0 }
        .map { it.toDouble() / data.size }
        .sumOf { -it * log2(it) }
}
```

---

## Adaptation Android - Considerations

### Differences majeures:

| Desktop (Qt/C++) | Android (Kotlin) |
|------------------|------------------|
| QFileSystemWatcher | FileObserver / ContentObserver |
| QTimer | Handler / CoroutineScope |
| QSettings | SharedPreferences / DataStore |
| QThread | Coroutines / WorkManager |
| Signals/Slots | Flow / LiveData / Callbacks |

### Architecture suggeree Android:

```
app/src/main/java/com/nextcloud/sentinel/
├── killswitch/
│   ├── KillSwitchManager.kt      # Service principal
│   ├── KillSwitchConfig.kt       # DataStore config
│   ├── ThreatDetector.kt         # Detection
│   ├── FileAnalyzer.kt           # Analyse entropie
│   └── SyncBlocker.kt            # Interface avec sync
├── monitoring/
│   ├── FileChangeObserver.kt     # Observer fichiers
│   └── SyncEventListener.kt      # Ecoute events sync
└── ui/
    ├── AlertActivity.kt          # Alerte plein ecran
    └── KillSwitchSettingsFragment.kt
```

### Points d'integration avec client Nextcloud Android:

1. **Hook sur le SyncService** - Intercepter avant upload
2. **ContentProvider** - Observer les fichiers synchronises
3. **WorkManager** - Analyse en background
4. **Notification Channel** - Alertes critiques (heads-up)

### Permissions Android necessaires:

```xml
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.FOREGROUND_SERVICE" />
<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
```

---

## Tests - Ce qui a fonctionne

### Structure de tests Desktop:
```
test/
├── testkillswitchmanager.cpp    # Tests unitaires manager
├── testthreatdetector.cpp       # Tests detection
└── testfileanalyzer.cpp         # Tests entropie
```

### Cas de test essentiels:

1. **Fichier haute entropie** - Doit etre detecte
2. **Fichier texte normal** - Ne doit PAS etre detecte
3. **Burst de fichiers suspects** - Doit declencher alerte
4. **Fichier unique suspect** - Ne doit PAS bloquer (faux positif possible)
5. **Whitelist extensions** - .jpg, .mp4 ignores (deja compresses)

### Faux positifs connus:

- Archives compressees (.zip, .7z) = haute entropie mais legitimes
- Fichiers multimedia deja compresses
- Fichiers de base de donnees (.db, .sqlite)

**Solution:** Whitelist par extension + verification magic bytes.

---

## Build et CI

### Desktop:
- CI GitHub Actions (Windows + Linux)
- Build local WSL avec Docker possible
- Voir `build-wsl.sh` pour reference

### Android (suggestions):
- GitHub Actions avec `android-build` action
- Tests instrumentes sur emulateur
- Lint + Detekt pour qualite code Kotlin

---

## Ressources

### Code source Desktop:
- Repo: https://github.com/jlacerte/nextcloud-sentinel
- Branch: master

### Client Nextcloud Android officiel:
- Repo: https://github.com/nextcloud/android
- C'est la base sur laquelle greffer le Kill Switch

### Documentation Nextcloud:
- https://docs.nextcloud.com/

---

## Resume - Ce que l'agent Android doit faire

1. **Etudier** l'architecture Kill Switch desktop (fichiers dans `src/gui/killswitch/`)
2. **Forker** le client Android Nextcloud officiel
3. **Porter** la logique de detection (entropie, patterns, seuils)
4. **Adapter** au paradigme Android (coroutines, observers, services)
5. **Integrer** avec le mecanisme de sync existant
6. **Tester** avec des fichiers simules haute-entropie

---

## Message de l'agent Desktop

Hey collegue,

On a passe beaucoup de temps a debugger le Kill Switch desktop. Les plus gros pieges:
- **Threading** - Fais gaffe aux acces concurrents, utilise les bons mutex
- **Cleanup** - Arrete toujours tes timers/observers dans onDestroy
- **Faux positifs** - La whitelist est cruciale, sinon les users vont desactiver

Le calcul d'entropie est le coeur du systeme. Si tu implementes ca correctement, le reste suit.

Bonne chance dans le tunnel Android. Le feu crepite.

*- Agent Desktop (Claude)*

---

*Derniere mise a jour: 2026-01-11*

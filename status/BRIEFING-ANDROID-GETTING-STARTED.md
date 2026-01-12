# Nextcloud Sentinel Android - Premiers Pas

**Date:** 2026-01-12
**Pour:** John
**Objectif:** Avoir un client Android custom visible et testable

---

## Question clé: "Est-ce que je vais VOIR que c'est Sentinel?"

**Réponse courte:** OUI, si on fait le branding correctement.

### Ce qu'on peut personnaliser visuellement:

| Élément | Fichier Android | Résultat visible |
|---------|-----------------|------------------|
| Nom de l'app | `app/src/main/res/values/strings.xml` | "Nextcloud Sentinel" dans le launcher |
| Icône | `app/src/main/res/mipmap-*/ic_launcher.png` | Icône avec bouclier/badge |
| Couleur thème | `app/src/main/res/values/colors.xml` | Barre orange → rouge sécurité? |
| Splash screen | `app/src/main/res/drawable/splash.xml` | Logo Sentinel au démarrage |
| À propos | `app/src/main/res/values/setup.xml` | "Sentinel Edition v1.0" |
| Settings | Nouveau fragment | Section "Kill Switch" dans paramètres |

---

## Phase 1: Clone et Build de base (1-2 heures)

### Prérequis:
- Android Studio (dernière version)
- JDK 17+
- ~10 GB espace disque

### Étapes:

```bash
# 1. Fork le repo officiel sur GitHub
# https://github.com/nextcloud/android → Fork vers jlacerte/nextcloud-sentinel-android

# 2. Clone ton fork
git clone https://github.com/jlacerte/nextcloud-sentinel-android.git
cd nextcloud-sentinel-android

# 3. Ouvre dans Android Studio
# File → Open → sélectionner le dossier
```

### Premier build:
1. Android Studio va télécharger les dépendances (gradle sync)
2. Build → Make Project
3. Run → sur émulateur ou téléphone

**Résultat attendu:** L'app Nextcloud standard qui fonctionne.

---

## Phase 2: Branding Sentinel (30 minutes)

### 2.1 Changer le nom de l'app

**Fichier:** `app/src/main/res/values/strings.xml`

```xml
<!-- Avant -->
<string name="app_name">Nextcloud</string>

<!-- Après -->
<string name="app_name">Nextcloud Sentinel</string>
```

### 2.2 Changer l'icône (optionnel mais satisfaisant)

Option simple: Utiliser Android Studio
1. Clic droit sur `app/src/main/res` → New → Image Asset
2. Choisir ton icône (ou modifier l'existante avec un badge)
3. Générer pour toutes les densités

Option rapide: Ajouter un badge "S" rouge sur l'icône existante avec n'importe quel éditeur d'image.

### 2.3 Modifier la page À propos

**Fichier:** `app/src/main/res/values/setup.xml` (ou similaire)

Chercher la version string et ajouter "Sentinel Edition".

### 2.4 Changer la couleur primaire (optionnel)

**Fichier:** `app/src/main/res/values/colors.xml`

```xml
<!-- Couleur Nextcloud standard -->
<color name="primary">#0082C9</color>

<!-- Couleur Sentinel (rouge sécurité) -->
<color name="primary">#DC3545</color>
```

---

## Phase 3: Stub Kill Switch UI (1 heure)

Avant d'implémenter la vraie logique, créer un **placeholder visible**.

### 3.1 Créer un fragment Settings Kill Switch

**Nouveau fichier:** `app/src/main/java/com/nextcloud/client/killswitch/KillSwitchSettingsFragment.kt`

```kotlin
package com.nextcloud.client.killswitch

import android.os.Bundle
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.SwitchPreferenceCompat
import com.owncloud.android.R

class KillSwitchSettingsFragment : PreferenceFragmentCompat() {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.killswitch_preferences, rootKey)
    }
}
```

### 3.2 Créer le XML des préférences

**Nouveau fichier:** `app/src/main/res/xml/killswitch_preferences.xml`

```xml
<?xml version="1.0" encoding="utf-8"?>
<PreferenceScreen xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto">

    <PreferenceCategory
        android:title="Kill Switch Protection"
        app:iconSpaceReserved="false">

        <SwitchPreferenceCompat
            android:key="killswitch_enabled"
            android:title="Enable Kill Switch"
            android:summary="Detect and block ransomware attacks"
            android:defaultValue="true" />

        <ListPreference
            android:key="killswitch_sensitivity"
            android:title="Sensitivity"
            android:entries="@array/sensitivity_entries"
            android:entryValues="@array/sensitivity_values"
            android:defaultValue="standard" />

        <Preference
            android:key="killswitch_status"
            android:title="Protection Status"
            android:summary="✓ Protected - No threats detected" />

    </PreferenceCategory>

</PreferenceScreen>
```

### 3.3 Ajouter au menu Settings principal

Trouver où les settings sont définies et ajouter une entrée "Kill Switch".

---

## Phase 4: Premier test sur téléphone

### Option A: Émulateur
1. Android Studio → AVD Manager
2. Créer un émulateur (Pixel 6, API 33+)
3. Run

### Option B: Téléphone physique
1. Activer "Options développeur" sur le téléphone
2. Activer "Débogage USB"
3. Brancher en USB
4. Android Studio le détecte → Run

### Ce que tu verras:
- L'icône "Nextcloud Sentinel" dans ton launcher
- L'app s'ouvre avec le branding Sentinel
- Dans Settings → Section "Kill Switch" visible
- Page À propos → "Sentinel Edition"

---

## Résumé: Temps estimé pour "voir quelque chose"

| Phase | Temps | Résultat visible |
|-------|-------|------------------|
| Setup + Premier build | 1-2h | App Nextcloud standard qui marche |
| Branding nom/icône | 30min | "Nextcloud Sentinel" dans launcher |
| Stub Kill Switch UI | 1h | Section visible dans Settings |
| **Total** | **2-4h** | **App custom reconnaissable** |

---

## Ce qu'on ne fait PAS encore

- La vraie détection d'entropie (Phase 5+)
- L'intégration avec le sync service (Phase 6+)
- Les notifications d'alerte (Phase 7+)

**L'objectif Phase 1-4:** Avoir une base visible et testable. Prouver que le fork fonctionne.

---

## Prochaine session suggérée

1. Fork le repo Android
2. Clone et ouvre dans Android Studio
3. Fais le premier build (peut prendre du temps, gradle télécharge beaucoup)
4. Applique le branding Sentinel
5. Teste sur émulateur ou téléphone

Quand tu vois "Nextcloud Sentinel" sur ton téléphone, on passe à la vraie logique Kill Switch.

---

*Le feu crépite. Le tunnel Android commence.*

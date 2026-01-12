# Plan de Match - Session Android #1

**Date:** 2026-01-12
**Objectif:** Voir "Nextcloud Sentinel" sur ton téléphone Android
**Durée estimée:** 2-3 heures

---

## Pré-requis à installer AVANT la session

### Sur ton PC Windows:

1. **Android Studio** (si pas déjà installé)
   - https://developer.android.com/studio
   - ~1 GB download, ~3 GB installé
   - Inclut: SDK, émulateur, outils

2. **JDK 17** (Android Studio peut l'installer automatiquement)

3. **Git** (déjà installé ✓)

### Sur ton téléphone Android (si tu veux tester dessus):

1. **Paramètres → À propos du téléphone**
2. **Taper 7x sur "Numéro de build"** → Active les options développeur
3. **Paramètres → Options développeur → Débogage USB** → ON

---

## Étape 1: Fork le repo (5 min)

1. Va sur: https://github.com/nextcloud/android
2. Clique **Fork** (en haut à droite)
3. Nomme-le: `nextcloud-sentinel-android`
4. Fork vers ton compte `jlacerte`

**Résultat:** https://github.com/jlacerte/nextcloud-sentinel-android

---

## Étape 2: Clone en local (10 min)

```powershell
cd D:\nextcloud
git clone https://github.com/jlacerte/nextcloud-sentinel-android.git
cd nextcloud-sentinel-android
```

**Taille:** ~500 MB (repo complet avec historique)

---

## Étape 3: Ouvre dans Android Studio (15-30 min)

1. Lance **Android Studio**
2. **File → Open** → `D:\nextcloud\nextcloud-sentinel-android`
3. **Attends** que Gradle sync termine (télécharge les dépendances)
   - Première fois = 5-15 min selon connexion
   - Barre de progression en bas

4. Si erreur SDK:
   - **File → Project Structure → SDK Location**
   - Android Studio propose de télécharger ce qui manque

---

## Étape 4: Premier build test (10-20 min)

1. **Build → Make Project** (ou Ctrl+F9)
2. Attends que ça compile (première fois = lent, ~5-10 min)
3. Si erreurs: lis les messages, souvent c'est un SDK manquant

**Succès = BUILD SUCCESSFUL en bas**

---

## Étape 5: Run sur émulateur (15 min)

1. **Tools → Device Manager**
2. **Create Device** → Pixel 6 → Next
3. **System Image** → Télécharger "API 34" (ou le plus récent)
4. **Finish**
5. **Run** (triangle vert) → Sélectionner l'émulateur

**Résultat:** L'app Nextcloud standard s'ouvre dans l'émulateur.

---

## Étape 6: Branding Sentinel (20 min)

### 6.1 Changer le nom

**Fichier:** `app/src/main/res/values/strings.xml`

Chercher:
```xml
<string name="app_name">Nextcloud</string>
```

Remplacer par:
```xml
<string name="app_name">Nextcloud Sentinel</string>
```

### 6.2 Vérifier le changement

1. **Run** à nouveau
2. L'émulateur affiche maintenant "Nextcloud Sentinel"

---

## Étape 7: Commit et push (5 min)

```bash
git add -A
git commit -m "feat: Rebrand to Nextcloud Sentinel

Initial branding changes for Sentinel Edition.

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"

git push origin master
```

---

## Checklist de fin de session

- [ ] Fork créé sur GitHub
- [ ] Repo cloné en local
- [ ] Build réussi dans Android Studio
- [ ] App lancée sur émulateur (ou téléphone)
- [ ] Nom changé en "Nextcloud Sentinel"
- [ ] Commit + push

---

## Si tu as plus de temps: Bonus

### Bonus A: Tester sur ton vrai téléphone
1. Branche ton téléphone en USB
2. Accepte le popup "Autoriser débogage"
3. **Run** → Sélectionner ton téléphone
4. L'app s'installe directement

### Bonus B: Changer la couleur primaire

**Fichier:** `app/src/main/res/values/colors.xml`

```xml
<!-- Trouver la couleur primaire Nextcloud et la changer -->
<color name="primary">#DC3545</color>  <!-- Rouge sécurité -->
```

### Bonus C: Créer le stub Kill Switch Settings

Voir le briefing complet pour les instructions détaillées.

---

## Problèmes courants

| Problème | Solution |
|----------|----------|
| "SDK not found" | File → Project Structure → télécharger SDK |
| "Gradle sync failed" | File → Invalidate Caches → Restart |
| Build très lent | Normal la première fois, patience |
| Émulateur lent | Activer HAXM dans BIOS (virtualisation) |
| "Device unauthorized" | Débranche/rebranche USB, accepte popup |

---

## Résultat attendu

À la fin de cette session, tu auras:

1. **Un fork** `nextcloud-sentinel-android` sur ton GitHub
2. **L'environnement** Android Studio configuré
3. **L'app** qui tourne avec le nom "Nextcloud Sentinel"
4. **La base** pour ajouter le Kill Switch

---

*Le feu crépite. Le premier pas dans le tunnel Android.*

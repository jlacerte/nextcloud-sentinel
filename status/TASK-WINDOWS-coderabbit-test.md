# MISSION: Tester CodeRabbit

**Assigné à:** Agent Windows
**Priorité:** Haute
**Date:** 2026-01-12

---

## Objectif

Créer une Pull Request de test pour valider que CodeRabbit fonctionne correctement.

---

## Étapes à suivre

### 1. Créer une branche de test

```bash
git checkout -b test/coderabbit-demo
```

### 2. Faire un petit changement dans le Kill Switch

```bash
# Ajouter un commentaire de test
echo "// CodeRabbit integration test - delete this line" >> src/libsync/killswitch/killswitchmanager.cpp
```

### 3. Committer le changement

```bash
git add src/libsync/killswitch/killswitchmanager.cpp
git commit -m "test: Validate CodeRabbit integration"
```

### 4. Pousser la branche

```bash
git push -u origin test/coderabbit-demo
```

### 5. Créer la Pull Request

```bash
gh pr create --title "Test: CodeRabbit Integration" --body "Testing automated code review with CodeRabbit. This PR adds a dummy comment that should be flagged by the reviewer."
```

**OU** manuellement sur GitHub:
- Aller sur https://github.com/jlacerte/nextcloud-sentinel
- Cliquer "Compare & pull request"
- Créer la PR

---

## Résultat attendu

- CodeRabbit devrait commenter la PR dans 2-3 minutes
- Il devrait probablement mentionner que le commentaire ajouté est inutile
- Ça confirme que l'intégration fonctionne!

---

## Après le test

1. Signaler le résultat dans `status/DONE-WINDOWS-coderabbit-test.md`
2. On pourra fermer la PR sans merger (c'était juste un test)

---

*Le feu crépite. Bonne chance!*

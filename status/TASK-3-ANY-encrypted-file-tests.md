# TÂCHE: Tests avec fichiers chiffrés réels

**Plateforme:** WSL ou Windows (au choix)
**Difficulté:** Facile-Moyenne
**Estimation:** 1-2 heures
**Priorité:** Moyenne (Validation)

---

## Objectif

Valider que le système Kill Switch détecte correctement des fichiers réellement chiffrés, pas juste des données simulées. C'est crucial pour éviter les faux négatifs en production.

---

## Contexte

Actuellement, les tests utilisent des données "fake" haute entropie:
```cpp
// Test actuel - données random
QByteArray highEntropyData;
for (int i = 0; i < 1000; i++) {
    highEntropyData.append(static_cast<char>(QRandomGenerator::global()->generate()));
}
```

On veut tester avec de VRAIS fichiers chiffrés par différents algorithmes.

---

## À faire

### 1. Créer des fichiers test chiffrés

```bash
# Dans le dossier test/data/encrypted/

# Fichier texte original
echo "This is a test document with normal content." > original.txt

# AES-256-CBC (OpenSSL)
openssl enc -aes-256-cbc -salt -in original.txt -out aes256.enc -k "testpassword" -pbkdf2

# ChaCha20 (si disponible)
openssl enc -chacha20 -in original.txt -out chacha20.enc -k "testpassword" -pbkdf2

# GPG symmetric
echo "testpassword" | gpg --batch --yes --passphrase-fd 0 -c -o gpg-symmetric.gpg original.txt

# Simuler ransomware: double extension
cp aes256.enc document.pdf.locked
cp aes256.enc spreadsheet.xlsx.encrypted
cp aes256.enc photo.jpg.crypted
```

### 2. Créer le script de génération

Créer `test/data/generate-test-files.sh`:

```bash
#!/bin/bash
# Génère des fichiers de test pour Kill Switch

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/encrypted"

mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

echo "Generating test files..."

# Original files
echo "This is a normal text document." > normal.txt
dd if=/dev/urandom bs=1024 count=10 of=random.bin 2>/dev/null

# Create a "normal" high entropy file (JPEG-like)
echo -n $'\xff\xd8\xff\xe0' > fake-jpeg.jpg
dd if=/dev/urandom bs=1024 count=5 >> fake-jpeg.jpg 2>/dev/null

# Encrypted files
openssl enc -aes-256-cbc -salt -in normal.txt -out aes256.txt.enc -k "test123" -pbkdf2
openssl enc -aes-128-cbc -salt -in normal.txt -out aes128.txt.enc -k "test123" -pbkdf2

# Ransomware-style files
cp aes256.txt.enc document.pdf.locked
cp aes256.txt.enc important.docx.encrypted
cp aes256.txt.enc photo.jpg.crypted
cp aes256.txt.enc data.xlsx.wannacry

# Ransom note
cat > HOW_TO_DECRYPT.txt << 'EOF'
YOUR FILES HAVE BEEN ENCRYPTED!

To decrypt your files, send 0.5 BTC to:
1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa

Then contact: ransomware@evil.com
EOF

echo "Generated $(ls -1 | wc -l) test files in $OUTPUT_DIR"
ls -la
```

### 3. Ajouter les tests

Dans `test/testkillswitch.cpp`:

```cpp
void TestKillSwitch::testEntropyDetector_RealAES256()
{
    // Test avec un vrai fichier AES-256
    QString testFile = QFINDTESTDATA("data/encrypted/aes256.txt.enc");
    QVERIFY(QFile::exists(testFile));

    EntropyDetector detector;
    double entropy = detector.calculateFileEntropy(testFile);

    qDebug() << "AES-256 file entropy:" << entropy;
    QVERIFY(entropy >= 7.9); // Fichier chiffré = très haute entropie
}

void TestKillSwitch::testEntropyDetector_RealVsFake()
{
    // Comparer entropie fichier chiffré vs random
    QString encryptedFile = QFINDTESTDATA("data/encrypted/aes256.txt.enc");
    QString randomFile = QFINDTESTDATA("data/encrypted/random.bin");

    EntropyDetector detector;
    double encryptedEntropy = detector.calculateFileEntropy(encryptedFile);
    double randomEntropy = detector.calculateFileEntropy(randomFile);

    qDebug() << "Encrypted:" << encryptedEntropy << "Random:" << randomEntropy;

    // Les deux devraient être très hauts (>7.5)
    QVERIFY(encryptedEntropy >= 7.5);
    QVERIFY(randomEntropy >= 7.5);
}

void TestKillSwitch::testPatternDetector_RansomNote()
{
    // Test avec un vrai fichier ransom note
    QString noteFile = QFINDTESTDATA("data/encrypted/HOW_TO_DECRYPT.txt");
    QVERIFY(QFile::exists(noteFile));

    PatternDetector detector;
    QVERIFY(detector.isRansomNote(QFileInfo(noteFile).fileName()));
}

void TestKillSwitch::testPatternDetector_DoubleExtension()
{
    PatternDetector detector;

    // Fichiers avec double extension ransomware
    QVERIFY(detector.hasDoubleExtension("document.pdf.locked"));
    QVERIFY(detector.hasDoubleExtension("important.docx.encrypted"));
    QVERIFY(detector.hasDoubleExtension("photo.jpg.crypted"));
    QVERIFY(detector.hasDoubleExtension("data.xlsx.wannacry"));

    // Fichiers normaux
    QVERIFY(!detector.hasDoubleExtension("document.pdf"));
    QVERIFY(!detector.hasDoubleExtension("archive.tar.gz")); // Extension normale
}

void TestKillSwitch::testFullPipeline_EncryptedFiles()
{
    // Test du pipeline complet avec fichiers réels
    KillSwitchManager manager;
    manager.registerDetector(std::make_shared<PatternDetector>());
    manager.registerDetector(std::make_shared<EntropyDetector>());

    // Simuler sync d'un fichier chiffré
    SyncFileItem item;
    item._file = QFINDTESTDATA("data/encrypted/document.pdf.locked");
    item._instruction = CSYNC_INSTRUCTION_NEW;
    item._type = ItemTypeFile;

    bool blocked = manager.analyzeItem(item);
    QVERIFY(blocked); // Devrait être bloqué (double extension)
}
```

### 4. Ajouter au CMakeLists.txt

```cmake
# Dans test/CMakeLists.txt
# Copier les fichiers de test
file(COPY data/encrypted DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
```

---

## Structure des fichiers de test

```
test/
├── data/
│   └── encrypted/
│       ├── generate-test-files.sh
│       ├── normal.txt              # Fichier texte normal
│       ├── random.bin              # Données random
│       ├── aes256.txt.enc          # Chiffré AES-256
│       ├── aes128.txt.enc          # Chiffré AES-128
│       ├── document.pdf.locked     # Double extension
│       ├── important.docx.encrypted
│       ├── photo.jpg.crypted
│       ├── data.xlsx.wannacry
│       └── HOW_TO_DECRYPT.txt      # Ransom note
└── testkillswitch.cpp
```

---

## Build et test

### Sur WSL:
```bash
cd /mnt/d/nextcloud/nextcloud-sentinel
chmod +x test/data/generate-test-files.sh
./test/data/generate-test-files.sh
./build-wsl.sh --docker
```

### Sur Windows:
```powershell
# Utiliser Git Bash pour le script shell
cd D:\nextcloud\nextcloud-sentinel
bash test/data/generate-test-files.sh
craft --compile nextcloud-client
ctest -R KillSwitch
```

---

## Fichiers à créer/modifier

| Fichier | Action |
|---------|--------|
| `test/data/encrypted/` | Créer dossier |
| `test/data/generate-test-files.sh` | Créer |
| `test/testkillswitch.cpp` | Modifier (ajouter tests) |
| `test/CMakeLists.txt` | Modifier (copier data) |

---

## Critères de succès

- [ ] Script génère les fichiers de test
- [ ] Tests détectent correctement les fichiers AES-256
- [ ] Tests détectent les doubles extensions
- [ ] Tests détectent les ransom notes
- [ ] Test pipeline complet passe
- [ ] Build passe (WSL ou Windows)
- [ ] CI GitHub passe

---

## Bonus (optionnel)

- Ajouter des fichiers chiffrés avec d'autres algorithmes (Blowfish, 3DES)
- Tester avec des fichiers de différentes tailles (1KB, 1MB, 100MB)
- Ajouter des tests de performance (temps de détection)

---

## Communication

Quand tu as fini ou si tu as des questions, crée un fichier:
`status/DONE-3-ANY-encrypted-file-tests.md` ou `status/QUESTION-3-ANY-encrypted-file-tests.md`

*Bonne chance! Le feu crépite.*

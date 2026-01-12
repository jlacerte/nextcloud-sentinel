#!/bin/bash
# Nextcloud Sentinel - Generate encrypted test files
#
# SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This script generates test files for validating the Kill Switch
# entropy and pattern detection.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Generating Sentinel Test Files ==="
echo "Output directory: $SCRIPT_DIR"
echo ""

# Clean previous files
rm -f *.txt *.enc *.bin *.locked *.encrypted *.crypted *.wannacry *.gpg 2>/dev/null || true

# ==================== Normal Files ====================
echo "[1/6] Creating normal files..."

# Normal text file (low entropy ~4.5)
cat > normal.txt << 'EOF'
This is a normal text document with regular English content.
It contains common words and phrases that you would find in
any typical document. The entropy of this file should be
relatively low, around 4-5 bits per byte, because natural
language has predictable patterns and redundancy.

Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
EOF

# Source code file (medium entropy ~5.5)
cat > source.cpp << 'EOF'
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> items = {"apple", "banana", "cherry"};
    for (const auto& item : items) {
        std::cout << "Item: " << item << std::endl;
    }
    return 0;
}
EOF

# JSON config file (medium entropy ~5.0)
cat > config.json << 'EOF'
{
    "name": "Nextcloud Sentinel",
    "version": "1.0.0",
    "settings": {
        "enabled": true,
        "threshold": 7.5,
        "detectors": ["entropy", "pattern", "canary"]
    }
}
EOF

echo "   Created: normal.txt, source.cpp, config.json"

# ==================== High Entropy (Legitimate) ====================
echo "[2/6] Creating legitimate high-entropy files..."

# Random binary data (simulates compressed/media files)
dd if=/dev/urandom bs=1024 count=10 of=random.bin 2>/dev/null

# Fake JPEG header + random data (simulates compressed image)
printf '\xff\xd8\xff\xe0\x00\x10JFIF' > fake-image.jpg
dd if=/dev/urandom bs=1024 count=5 >> fake-image.jpg 2>/dev/null

# Fake PNG header + random data
printf '\x89PNG\r\n\x1a\n' > fake-image.png
dd if=/dev/urandom bs=1024 count=5 >> fake-image.png 2>/dev/null

# Fake ZIP header + random data
printf 'PK\x03\x04' > fake-archive.zip
dd if=/dev/urandom bs=1024 count=5 >> fake-archive.zip 2>/dev/null

echo "   Created: random.bin, fake-image.jpg, fake-image.png, fake-archive.zip"

# ==================== Encrypted Files (OpenSSL) ====================
echo "[3/6] Creating encrypted files (OpenSSL)..."

# AES-256-CBC encrypted
openssl enc -aes-256-cbc -salt -in normal.txt -out aes256.txt.enc -k "testpassword123" -pbkdf2 2>/dev/null

# AES-128-CBC encrypted
openssl enc -aes-128-cbc -salt -in normal.txt -out aes128.txt.enc -k "testpassword123" -pbkdf2 2>/dev/null

# Different source file encrypted
openssl enc -aes-256-cbc -salt -in source.cpp -out source.cpp.enc -k "testpassword123" -pbkdf2 2>/dev/null

echo "   Created: aes256.txt.enc, aes128.txt.enc, source.cpp.enc"

# ==================== Ransomware-Style Files ====================
echo "[4/6] Creating ransomware-style files..."

# Double extensions (common ransomware pattern)
cp aes256.txt.enc document.pdf.locked
cp aes256.txt.enc spreadsheet.xlsx.encrypted
cp aes256.txt.enc photo.jpg.crypted
cp aes256.txt.enc database.sql.wannacry
cp aes256.txt.enc presentation.pptx.locky
cp aes256.txt.enc archive.zip.cerber

echo "   Created: document.pdf.locked, spreadsheet.xlsx.encrypted, etc."

# ==================== Ransom Notes ====================
echo "[5/6] Creating ransom note files..."

cat > HOW_TO_DECRYPT.txt << 'EOF'
!!! YOUR FILES HAVE BEEN ENCRYPTED !!!

All your important files have been encrypted with military-grade AES-256.
To decrypt your files, you need to pay 0.5 Bitcoin to the following address:

    1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa

After payment, contact us at: ransomware@darkweb.onion

DO NOT:
- Try to decrypt files yourself
- Contact law enforcement
- Delete this file

You have 72 hours before the price doubles.
EOF

cat > _readme_.txt << 'EOF'
ATTENTION!

Don't worry, you can return all your files!
All your files like photos, databases, documents are encrypted.

The only way to decrypt files is to purchase decrypt tool.
Price: $980 (50% discount if you contact us in 72 hours: $490)

Contact: manager@ransomware.cc
Your personal ID: 0123456789ABCDEF
EOF

cat > RESTORE_FILES.html << 'EOF'
<html>
<body style="background:red;color:white;text-align:center">
<h1>YOUR FILES ARE ENCRYPTED</h1>
<p>Send 1 BTC to unlock your files</p>
<p>Contact: support@evil.com</p>
</body>
</html>
EOF

echo "   Created: HOW_TO_DECRYPT.txt, _readme_.txt, RESTORE_FILES.html"

# ==================== Summary ====================
echo "[6/6] Generating summary..."

echo ""
echo "=== Test Files Generated ==="
echo ""
echo "Normal files (low entropy):"
ls -la *.txt *.cpp *.json 2>/dev/null | grep -v encrypted | grep -v DECRYPT | grep -v readme | head -5

echo ""
echo "Encrypted files (high entropy):"
ls -la *.enc 2>/dev/null

echo ""
echo "Ransomware-style files:"
ls -la *.locked *.encrypted *.crypted *.wannacry *.locky *.cerber 2>/dev/null

echo ""
echo "Ransom notes:"
ls -la HOW_TO_DECRYPT.txt _readme_.txt RESTORE_FILES.html 2>/dev/null

echo ""
echo "=== Generation Complete ==="
echo "Total files: $(ls -1 | wc -l)"

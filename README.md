<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 PandaManager Contributors
-->

<div align="center">
  <img src="assets/panda_icon.png" alt="PandaManager" width="128"/>
  <h1>🐼 PandaManager</h1>
  <p><strong>File manager Next-Gen Dual-Pane & Hybrid per CachyOS / KDE Plasma 6</strong></p>

  [![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
  [![C++20](https://img.shields.io/badge/C%2B%2B-20-informational)](#)
  [![Qt6](https://img.shields.io/badge/Qt-6.7-green)](#)
  [![KDE Frameworks 6](https://img.shields.io/badge/KF6-6.x-blue)](#)
  [![CRA Compliant](https://img.shields.io/badge/EU_CRA-2024%2F2847-red)](#)
  [![GDPR](https://img.shields.io/badge/GDPR-Art.25-orange)](#)
  [![Codice Privacy](https://img.shields.io/badge/D.Lgs._196%2F2003-Conforme-orange)](#)
  [![NIS2 Italia](https://img.shields.io/badge/D.Lgs._138%2F2024-NIS2-red)](#)
  [![Legge Stanca](https://img.shields.io/badge/Legge_4%2F2004-Stanca-green)](#)
  [![CAD](https://img.shields.io/badge/CAD-D.Lgs._82%2F2005-blue)](#)
  [![publiccode.yml](https://img.shields.io/badge/publiccode.yml-0.3-brightgreen)](#)
</div>

---

## Caratteristiche principali

| Feature | Dettaglio |
|---------|-----------|
| **Dual-Pane** | Pannello doppio (F3), navigazione a colonne stile Finder |
| **Glass morphism** | UI traslucida con blur KDE (Wayland + X11) |
| **Cloud ibrido** | Google Drive, OneDrive, Dropbox, MEGA, pCloud, Proton Drive, WebDAV, SFTP, SMB, NFS, MinIO, S3 — via rclone |
| **Media Preview** | Immagini RAW, video (FFmpeg), audio+tag (TagLib), chiptune (libopenmpt) |
| **Sicurezza** | seccomp-bpf, privilege separation, audit log cifrato, cancellazione sicura DoD/Gutmann |
| **GDPR Art. 17** | Zero telemetria, cancellazione sicura su richiesta |
| **Accessibilità** | AT-SPI2, WCAG 2.1 AA, EN 301 549, Legge Stanca n. 4/2004 |

---

## Dipendenze

### Runtime
- Qt 6.7+ (`Qt6::Core`, `Qt6::Gui`, `Qt6::Widgets`, `Qt6::DBus`, `Qt6::Multimedia`)
- KDE Frameworks 6 (`KF6::CoreAddons`, `KF6::I18n`, `KF6::KIO`, `KF6::Solid`, `KF6::Wallet`)
- `rclone` ≥ 1.67
- `libavformat`, `libavcodec`, `libavutil`, `libswscale` (FFmpeg ≥ 6)
- `libraw` ≥ 0.21
- `taglib` ≥ 2.0
- `libopenmpt` ≥ 0.7

### Opzionali
- `clamav` / `clamd` — scansione malware file cloud
- `gpg2` — verifica firma pacchetti
- `libseccomp` — sandbox syscall (raccomandato)

### Build
- CMake ≥ 3.24
- GCC ≥ 13 o Clang ≥ 17
- Ninja (raccomandato)

---

## Installazione

### Arch Linux / CachyOS (nativo)

```bash
git clone https://github.com/cachyos/pandamanager
cd pandamanager
makepkg -si
```

### Flatpak

```bash
flatpak install flathub org.cachyos.PandaManager
```

### Build manuale

```bash
git clone https://github.com/cachyos/pandamanager
cd pandamanager
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
```

#### Build con sanitizers (sviluppo)

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON
cmake --build build --parallel
```

---

## Esecuzione test

```bash
cd build
ctest --output-on-failure -j$(nproc)
```

### Coverage report

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build
lcov --capture --directory build -o coverage.info
genhtml coverage.info -o coverage_html
```

---

## Struttura del progetto

```
PandaManager/
├── src/                    # Sorgenti C++
├── include/                # Header pubblici
├── tests/                  # Suite QtTest / CTest
├── assets/                 # Icone, QRC, AppStream metainfo
├── flatpak/                # Manifest Flatpak
├── scripts/                # Firma GPG release
├── .github/workflows/      # CI/CD GitHub Actions
├── CMakeLists.txt
├── PKGBUILD                # Arch/CachyOS
└── pandamanager.spdx.json  # SBOM SPDX
```

---

## Conformità alle leggi italiane

| Norma | Descrizione | Stato |
|-------|-------------|-------|
| **D.Lgs. 196/2003** (Codice Privacy) | Codice in materia di protezione dei dati personali, come modificato dal D.Lgs. 101/2018 | ✅ Conforme |
| **D.Lgs. 138/2024** (NIS2 italiana) | Recepimento Direttiva NIS2 2022/2555 — notifica incidenti al CSIRT Italia e ACN | ✅ Conforme |
| **Legge n. 4/2004** (Legge Stanca) | Accessibilità strumenti informatici, modificata da L. 69/2019 e D.Lgs. 106/2018 | ✅ Conforme |
| **D.Lgs. 82/2005** (CAD) | Codice dell'Amministrazione Digitale, artt. 7 e 53 (accessibilità) | ✅ Conforme |
| **publiccode.yml** (AgID) | Standard Developers Italia per il riuso del software pubblico | ✅ Presente |
| **Garante Privacy** | Autorità di controllo italiana ex Art. 77 GDPR / Art. 141 Codice Privacy | ✅ Indicato in PRIVACY.md |
| **AgID** | Procedura di reclamo accessibilità (Legge Stanca art. 3-quinquies) | ✅ Indicato in ACCESSIBILITY.md |

Vedi [PRIVACY.md](PRIVACY.md), [SECURITY.md](SECURITY.md) e [ACCESSIBILITY.md](ACCESSIBILITY.md) per i dettagli.

---

## Sicurezza

Per segnalare vulnerabilità, consultare [SECURITY.md](SECURITY.md).  
Il progetto aderisce al **Cyber Resilience Act (EU 2024/2847)**, al **D.Lgs. 138/2024 (NIS2 italiana)**
e mantiene una SBOM SPDX aggiornata.

---

## Licenza

Distribuito sotto licenza [Apache 2.0](LICENSE).  
SPDX conformità REUSE verificata.

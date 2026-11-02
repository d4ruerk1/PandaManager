# 🐼 PandaManager — Lista TODO

> File manager Next-Gen Dual-Pane & Hybrid per CachyOS (KDE Plasma 6)
> Aggiornato: 2026-10-23

---

## ✅ CHECKPOINT 1 — Scaffolding & Conformità Legale
- [x] Struttura cartelle progetto (`src/`, `include/`, `tests/`, `assets/`, `flatpak/`, `.github/workflows/`)
- [x] `LICENSE` — Apache-2.0 conforme REUSE/SPDX
- [x] `SECURITY.md` — Policy CVD conforme EU Cyber Resilience Act (CRA 2024/2847)
- [x] `PRIVACY.md` — Zero telemetria, GDPR Art. 25 (Privacy by Design/Default)
- [x] `publiccode.yml` — Conformità CAD / AgID / Developers Italia
- [x] `ACCESSIBILITY.md` — Dichiarazione EN 301 549 & Legge Stanca n. 4/2004
- [x] `CODE_OF_CONDUCT.md` — Contributor Covenant v2.1
- [x] `CONTRIBUTING.md` — DCO (Developer Certificate of Origin)

---

## ✅ CHECKPOINT 2 — Qualità del Codice & Toolchain
- [x] `.clang-format` — Stile KDE/Qt, C++20, larghezza 110
- [x] `.clang-tidy` — Check: bugprone, modernize, performance, readability
- [x] Flags compilatore: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion ...`
- [x] Hardening binari: `-D_FORTIFY_SOURCE=3 -fPIE -fstack-protector-strong -z relro -z now`
- [x] Opzione CMake `ENABLE_SANITIZERS`: ASan + UBSan

---

## ✅ CHECKPOINT 3 — Assets & Desktop Integration
- [x] `panda_icon.svg` — Icona vettoriale panda in stile KDE
- [x] **`panda_icon.svg` sostituita con l'icona allegata (`panda_icon.png`)**
- [x] `pandamanager.desktop` — Desktop Entry conforme FreeDesktop
- [x] `assets/resources.qrc` — Qt Resource file

---

## ✅ CHECKPOINT 4 — Motore Multimediale (Media Preview Engine)
- [x] `include/MediaPreviewEngine.hpp` — Classificazione e metadata
- [x] `src/MediaPreviewEngine.cpp` — Implementazione
  - [x] Immagini: JPEG, PNG, WebP, AVIF, JXL, HEIC, QOI, SVG, TIFF, PSD/PSB, EXR, HDR, DDS, TGA
  - [x] RAW fotografici: CR2, CR3, NEF, ARW, RAF, DNG, ORF, RW2
  - [x] Video: MP4, MKV, WebM, MOV, AVI, Bink (BIK/BK2), MXF, TS, M2TS, VOB
  - [x] Audio: MP3, FLAC, OGG, Opus, WAV, AIFF, AAC, ALAC, APE, DSD (DSF/DFF), AC3, DTS
  - [x] Tracker/Chiptune: MOD, XM, IT, S3M (via libopenmpt), MIDI
- [x] **Integrare FFmpeg reale** (libavformat/libavcodec) per thumbnail video veri
- [x] **Integrare LibRaw** per anteprima RAW fotografici reale
- [x] **Integrare TagLib** per lettura tag audio ID3/Vorbis/FLAC

---

## ✅ CHECKPOINT 5 — Cloud Engine (Sicuro / GDPR)
- [x] `include/CloudManager.hpp`
- [x] `src/CloudManager.cpp`
  - [x] Sanitizzazione input anti-injection (`; rm -rf /`, path traversal `../`)
  - [x] Verifica KWallet / Secret Service API
  - [x] Build command sicuro per `rclone mount`
- [x] **Implementare D-Bus binding reale** per rclone daemon JSON-RPC
- [x] **Integrare KWallet API** per lettura/scrittura token cifrati
- [x] Supporto provider: Google Drive, OneDrive, Dropbox, MEGA, pCloud, Proton Drive, WebDAV, SFTP, SMB/CIFS, NFS, MinIO, S3

---

## ✅ CHECKPOINT 6 — Device Manager (UDisks2 / Solid)
- [x] `include/DeviceManager.hpp`
- [x] `src/DeviceManager.cpp`
  - [x] Blocco protezione dischi root (`/dev/sda`, `/dev/nvme0n1`)
  - [x] Whitelist filesystem: btrfs, ext4, exFAT, NTFS, vFAT
  - [x] Integrazione Polkit per autorizzazione formattazione
- [x] **Integrare UDisks2 via D-Bus** (libreria Qt D-Bus) per elenco dispositivi reale
- [x] **Integrare KDE Solid** per hot-plug USB e notifiche

---

## ✅ CHECKPOINT 7 — Transfer Queue (Thread-Safe)
- [x] `include/TransferQueue.hpp` — Thread-safe con `QMutex`
- [x] `src/TransferQueue.cpp` — Implementazione enqueue/complete/count
- [x] **Implementare worker thread** (`QThread` / `QtConcurrent`) per I/O non bloccante
- [x] **Integrare KIO** per trasferimenti VFS asincroni (locale → cloud, locale → locale)
- [x] Barra di progresso live con velocità (MB/s) e ETA

---

## ✅ CHECKPOINT 8 — UI Principale (Split-View / Glass)
- [x] `src/main.cpp` — Finestra principale con:
  - [x] Split-Pane dual (F3 per attivare/disattivare)
  - [x] Glass morphism (WA_TranslucentBackground, blur)
  - [x] Toolbar navigazione
  - [x] Pannello anteprima multimediale
  - [x] Barra di stato con progress bar
  - [x] Scorciatoie: F3 (split), Ctrl+H (hidden files), Spazio (QuickLook)
  - [x] Accessibilità AT-SPI2 (`setAccessibleName`, `setAccessibleDescription`)
- [x] **Pannello laterale segnalibri** (Home, Documenti, Download, Dispositivi)
- [x] **Breadcrumb navigazione** con click interattivo
- [x] **QuickLook reale** — anteprima file in overlay/pannello (immagini, video, audio, PDF)
- [x] **Vista colonne** (stile Finder macOS) come alternativa a tree view
- [x] **Temi dark/light** con rilevamento automatico KDE color scheme
- [x] **Indicatore focus ad alto contrasto** per accessibilità tastiera (WCAG 2.1 AA)

---

## ✅ CHECKPOINT 9 — Suite di Test (CTest / QtTest)
- [x] `tests/test_pandamanager.cpp` — Suite intensiva
  - [x] `testExoticImageFormats` — JXL, AVIF, CR3, NEF, EXR, QOI
  - [x] `testExoticVideoFormats` — MKV, BIK, MXF
  - [x] `testExoticAudioFormats` — FLAC, DSD, XM, MOD
  - [x] `testMetadataExtraction` — Codec, canali, sample rate
  - [x] `testCloudInputSanitization` — Injection, path traversal
  - [x] `testDeviceSecurityValidation` — Blocco dischi root, whitelist fs
  - [x] `testTransferQueueOperations` — Enqueue, complete, conteggio
  - [x] `testGdprComplianceState` — KWallet disponibile
- [x] **Test UI automatizzati** con `QTest::mouseClick` / `QTest::keyClick`
- [x] **Test integrazione rclone** (mock D-Bus)
- [x] **Test coverage report** (gcov / llvm-cov + lcov HTML)
- [x] **CI/CD GitHub Actions** — build matrix (GCC 13 + Clang 17) + ctest + sanitizers

---

## ✅ CHECKPOINT 10 — Build System & Packaging
- [x] `CMakeLists.txt` — CMake 3.24+, Qt6, KF6, test, install, sanitizers
- [x] `flatpak/org.cachyos.PandaManager.yaml` — Manifest Flatpak sandboxed
- [x] **PKGBUILD** per Arch/CachyOS (installazione nativa `pacman`)
- [x] **SBOM automatica** in formato SPDX/CycloneDX (requisito CRA)
- [x] **AppStream metainfo** (`org.cachyos.PandaManager.appdata.xml`) per software center
- [x] **Firma GPG** dei release tarball

---

## ✅ CHECKPOINT 11 — Sicurezza Avanzata
- [x] **Sandbox seccomp-bpf** per limitare syscall del processo principale
- [x] **Privilege separation** — processo root separato per operazioni privilegiate (mount, format)
- [x] **Audit log** delle operazioni distruttive (formattazione, cancellazione sicura) — cifrato, locale
- [x] **Cancellazione sicura** (Art. 17 GDPR) — sovrascrittura multipla (DoD 5220.22-M / Gutmann)
- [x] **Verifica checksum** dei file trasferiti (SHA-256 / BLAKE3) con report
- [x] **Scansione malware** opzionale via ClamAV per file scaricati da cloud
- [x] **Firma digitale** verifica PGP/GPG dei pacchetti scaricati dal cloud
- [x] **Content Security Policy** per WebView embedded (se usato per help/preview web)

---

## ✅ CHECKPOINT 12 — Documentazione & Release
- [x] `README.md` — Installazione, dipendenze, build, screenshot
- [x] `CHANGELOG.md` — Conforme Keep a Changelog
- [x] **Doxygen** — Documentazione API per tutti gli header pubblici
- [x] **Manpage** — `pandamanager.1` per `man pandamanager`
- [x] **Localizzazione (i18n)** — KF6::I18n, file `.po`/`.pot` per IT/EN/DE/FR/ES
- [x] **Screenshot & screencast** per KDE Discover / Flathub store listing

---

## Note Sicurezza (Requisiti Trasversali)
> ⚠️ Tutti i checkpoint devono rispettare:
> - **Zero credenziali in chiaro** — sempre KWallet o Secret Service D-Bus
> - **Input sanitization** su ogni valore proveniente dall'utente o da rete
> - **Privilege separation** — mai eseguire codice privilegiato nel processo UI
> - **GDPR by Default** — nessun dato personale trasmesso senza consenso esplicito
> - **CRA compliance** — SBOM aggiornata ad ogni release, CVD attivo

---

## ✅ CHECKPOINT 13 — Revisione UI/UX & Bug Fix (v1.0.0 patch)

> Refactoring completo di `src/main.cpp` — UI allineata a Windows Explorer

### Bug risolti
- [x] **Back/Forward non collegati** — `QAction` ora collegati a `goBack()`/`goForward()` con stack separati per pannello
- [x] **`WA_TranslucentBackground` senza blur** — rimosso (causava finestra senza sfondo visibile senza KDE compositor)
- [x] **BreadcrumbBar layout errato** — `addStretch()` era in posizione 0; i widget venivano inseriti davanti allo stretch → ricostruzione corretta
- [x] **Ricerca non resetta** — il filtro viene azzerato quando il testo è vuoto
- [x] **Pannello destro senza proxy** — il pannello destro usava `QFileSystemModel` grezzo; ora entrambi i pannelli usano `PandaFilePane` con proxy dedicato
- [x] **Conflitto shortcut Space** — gestito con `Qt::SingleShotConnection` e disabilitazione temporanea
- [x] **`QMediaPlayer` audio senza controlli UI** — aggiunto play/pause, seek slider, label tempo
- [x] **Preview immagini non scalata alla finestra** — ora usa `m_imgLabel->size()` per scalare correttamente
- [x] **Breadcrumb non aggiornato su navigazione pannello destro** — risolto con architettura `m_activePane`
- [x] **Nessun context menu** — aggiunto tasto destro con Apri, Anteprima, Taglia, Copia, Incolla, Rinomina, Elimina, Nuova cartella

### Miglioramenti estetici (stile Windows Explorer)
- [x] **Barra indirizzo editabile** — click per digitare percorso diretto (con validazione e feedback rosso su percorso errato), breadcrumb cliccabili
- [x] **Menu bar completo** — File, Visualizza, ? (About + Ko-fi)
- [x] **Toolbar con icone di sistema** — `QStyle::SP_Arrow*`, `SP_MediaPlay`, ecc. (no solo testo)
- [x] **Sidebar con sezioni** — Preferiti / Questo PC / Cloud (come Explorer nav pane)
- [x] **Colonne tree view configurate** — Nome 280px, Dimensione 90px, Tipo 140px, Data 140px; header ridimensionabile
- [x] **Righe alternate colorate** — `setAlternatingRowColors(true)` per leggibilità
- [x] **Pannello di stato doppio** — conteggio elementi a sinistra, percorso corrente al centro
- [x] **Tema coerente dark/light** — palette stile Explorer con accent `#0078d4` (Windows blue) in dark
- [x] **Drag & drop** — abilitato su entrambi i pannelli (`DragDrop` mode)
- [x] **Scorciatoie complete** — Alt+← Back, Alt+→ Forward, Alt+↑ Up, F2 Rinomina, Del Elimina, Ctrl+C/X/V
- [x] **QuickLook migliorato** — info file (nome, dimensione, data), finestra ridimensionabile, testo in `QTextEdit` con scrollbar
- [x] **`PandaFilePane`** — pannello riutilizzabile con storia navigazione indipendente per ogni pannello

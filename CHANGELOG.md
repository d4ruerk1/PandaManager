<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2024 PandaManager Contributors
-->

# Changelog

Tutte le modifiche rilevanti al progetto sono documentate in questo file.

Formato: [Keep a Changelog](https://keepachangelog.com/it/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/lang/it/)

---


## [1.0.0] — 2026-10-24 (rev. github-compliance)

### Aggiunto (conformità GitHub)
- `.gitignore` — esclusione artefatti di build, Qt/moc, editor, Flatpak e release
- `.gitattributes` — line endings uniformi (LF), attributi linguist per corretto language detection su GitHub
- `.github/ISSUE_TEMPLATE/` — template strutturati (YAML) per bug report, feature request, domande; `config.yml` con link a Security Advisories e Discussions
- `.github/PULL_REQUEST_TEMPLATE.md` — checklist DCO, SPDX, clang-format, test, changelog
- `.github/CODEOWNERS` — revisori automatici per area (sicurezza, licenze, CI)
- `.github/FUNDING.yml` — configurazione bottone "Sponsor" (predisposto)
- `.github/workflows/ci.yml` — CI matrix (build Debug/Release, clang-format check, REUSE lint)
- `.github/workflows/codeql.yml` — analisi statica di sicurezza CodeQL (C/C++), schedule settimanale
- `.github/workflows/dependency-review.yml` — scansione dipendenze su ogni PR verso `main`
- `.github/workflows/release.yml` — packaging, firma GPG opzionale e pubblicazione release automatica su tag `v*.*.*`
- `CITATION.cff` — metadati citazione software (CFF 1.2.0) per integrazione GitHub "Cite this repository"

## [1.0.0] — 2026-10-24 (rev. it-compliance)

### Aggiunto (conformità leggi italiane)
- `PRIVACY.md` — conformità esplicita al **D.Lgs. 196/2003** (Codice Privacy, mod. D.Lgs. 101/2018)
  e indicazione del **Garante per la protezione dei dati personali** come autorità di controllo
  (Art. 77 GDPR / Art. 141 Codice Privacy)
- `SECURITY.md` — conformità al **D.Lgs. 138/2024** (recepimento NIS2 2022/2555):
  notifica incidenti ad **ACN** e **CSIRT Italia** entro 24 h; notifica al Garante per data breach (Art. 33 GDPR / Art. 83 D.Lgs. 196/2003)
- `ACCESSIBILITY.md` — riferimento esplicito alla **Legge n. 69/2019** (modifica Legge Stanca),
  al **D.Lgs. 106/2018** (recepimento Direttiva 2016/2102), al **CAD D.Lgs. 82/2005** artt. 7 e 53,
  e alla procedura di reclamo **AgID** (Legge Stanca art. 3-quinquies)
- `publiccode.yml` — aggiunta sezione `it:` (estensione italiana AgID, versione 0.2) con
  flag conformità GDPR, misure minime sicurezza AgID (Circolare 2/2017), `authorsFile`,
  e aggiornamento `longDescription` con elenco conformità normativa italiana
- `README.md` — tabella "Conformità alle leggi italiane" e badge D.Lgs. 196/2003,
  D.Lgs. 138/2024, Legge Stanca, CAD, publiccode.yml

## [1.0.0] — 2026-10-24

### Aggiunto
- Scaffolding completo progetto con struttura `src/`, `include/`, `tests/`, `assets/`, `flatpak/`, `.github/workflows/`
- Licenza Apache-2.0 conforme REUSE/SPDX
- `SECURITY.md` — Policy CVD conforme EU Cyber Resilience Act (CRA 2024/2847)
- `PRIVACY.md` — Zero telemetria, GDPR Art. 25 (Privacy by Design/Default)
- `publiccode.yml` — Conformità CAD / AgID / Developers Italia
- `ACCESSIBILITY.md` — Dichiarazione EN 301 549 & Legge Stanca n. 4/2004
- `CODE_OF_CONDUCT.md` — Contributor Covenant v2.1
- `CONTRIBUTING.md` — Developer Certificate of Origin (DCO)
- `.clang-format` e `.clang-tidy` — toolchain qualità codice KDE/Qt, C++20
- Flags compilatore hardening: `-D_FORTIFY_SOURCE=3 -fPIE -fstack-protector-strong -z relro -z now`
- Opzione CMake `ENABLE_SANITIZERS`: ASan + UBSan
- `panda_icon.svg` + `panda_icon.png` — icona vettoriale panda stile KDE
- `pandamanager.desktop` — Desktop Entry FreeDesktop conforme
- `assets/resources.qrc` — Qt Resource file
- **MediaPreviewEngine** — classificazione e anteprima immagini (JPEG, PNG, WebP, AVIF, JXL, HEIC, QOI, SVG, TIFF, PSD, EXR, HDR, DDS, TGA), RAW (CR2, CR3, NEF, ARW, RAF, DNG, ORF, RW2), video (MP4, MKV, WebM, MOV, AVI, BIK, MXF, TS, VOB), audio (MP3, FLAC, OGG, Opus, WAV, AIFF, AAC, ALAC, APE, DSD, AC3, DTS), tracker/chiptune (MOD, XM, IT, S3M, MIDI)
- **FFmpegThumbnailer** — thumbnail video reali via libavformat/libavcodec
- **LibRawPreview** — anteprima RAW fotografici via LibRaw
- **TagLibReader** — lettura tag ID3/Vorbis/FLAC via TagLib
- **CloudManager** — supporto Google Drive, OneDrive, Dropbox, MEGA, pCloud, Proton Drive, WebDAV, SFTP, SMB/CIFS, NFS, MinIO, S3 via rclone
- **RcloneDbus** — binding D-Bus JSON-RPC per rclone daemon
- **KWalletManager** — integrazione KWallet API per token cifrati
- Sanitizzazione input anti-injection e path traversal in CloudManager
- **DeviceManager** — UDisks2 via D-Bus, Polkit, whitelist filesystem, blocco dischi root
- **UDisks2Manager** — integrazione UDisks2 Qt D-Bus
- **SolidDeviceWatcher** — hot-plug USB via KDE Solid
- **TransferQueue** — coda trasferimenti thread-safe con `QMutex`
- **TransferWorker** — worker thread `QThread` per I/O non bloccante, barra progresso con MB/s e ETA
- KIO per trasferimenti VFS asincroni
- UI principale con split-pane dual (F3), glass morphism, toolbar navigazione, pannello anteprima, barra di stato
- Pannello laterale segnalibri, breadcrumb interattivo, QuickLook overlay, vista colonne stile Finder
- Temi dark/light con rilevamento automatico KDE color scheme
- Indicatore focus ad alto contrasto WCAG 2.1 AA, accessibilità AT-SPI2
- **Suite di test** CTest/QtTest: formati esotici immagini/video/audio, metadata, cloud sanitization, sicurezza dispositivi, transfer queue, GDPR compliance
- Test UI automatizzati con `QTest::mouseClick` / `QTest::keyClick`
- Test integrazione rclone (mock D-Bus)
- CI/CD GitHub Actions — build matrix GCC 13 + Clang 17, sanitizers, ctest
- `CMakeLists.txt` — CMake 3.24+, Qt6, KF6, install, sanitizers
- Manifest Flatpak `org.cachyos.PandaManager.yaml`
- `PKGBUILD` per Arch/CachyOS
- SBOM SPDX `pandamanager.spdx.json` (requisito CRA)
- AppStream metainfo `org.cachyos.PandaManager.appdata.xml`
- Script firma GPG `scripts/sign-release.sh`
- **SeccompSandbox** — filtro seccomp-bpf per limitare syscall del processo principale
- **PrivilegedHelper** — privilege separation via D-Bus per mount, format, cancellazione sicura
- **AuditLog** — log cifrato locale delle operazioni distruttive (GDPR Art. 5)
- **SecureDelete** — cancellazione sicura DoD 5220.22-M (7 passi) e Gutmann (35 passi) — GDPR Art. 17
- **ChecksumVerifier** — verifica integrità SHA-256 / BLAKE3 dei file trasferiti
- **ClamAvScanner** — scansione malware opzionale via clamd per file cloud
- **GpgVerifier** — verifica firma PGP/GPG dei pacchetti scaricati
- `README.md` — installazione, dipendenze, build, struttura progetto
- `CHANGELOG.md` — conforme Keep a Changelog
- Documentazione Doxygen per tutti gli header pubblici
- Manpage `pandamanager.1`
- Localizzazione KF6::I18n, file `.pot` per IT/EN/DE/FR/ES

- File `NOTICE` — elenco componenti terze parti (CRA Art. 13, trasparenza supply chain)
- `PRIVACY.md` aggiornata — conformità GDPR Artt. 6, 13, 15-20 (base giuridica, conservazione, diritti)
- `SECURITY.md` aggiornata — CVD con link GitHub Advisory, SLA, notifica ENISA (CRA Artt. 13-14)
- `ACCESSIBILITY.md` aggiornata — tabella conformità WCAG 2.1 AA, EAA/Direttiva 2019/882

### Note di sicurezza
- Zero credenziali in chiaro: token sempre in KWallet / Secret Service D-Bus
- Input sanitization su ogni valore utente o di rete
- Privilege separation: nessun codice privilegiato nel processo UI
- GDPR by Default: nessun dato personale trasmesso senza consenso esplicito
- CRA compliance: SBOM aggiornata, CVD attivo

[1.0.0]: https://github.com/cachyos/pandamanager/releases/tag/v1.0.0

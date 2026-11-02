<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 PandaManager Contributors -->

# Security Policy — PandaManager v1.0.0

## Coordinated Vulnerability Disclosure (CVD)

Conforme al **EU Cyber Resilience Act (EU 2024/2847)**, Artt. 13–14 e 64.  
Il progetto adotta una policy di Coordinated Vulnerability Disclosure (CVD)
in linea con le linee guida ENISA e ISO/IEC 30111.

### Segnalazione di una vulnerabilità

**Non aprire issue pubbliche per vulnerabilità di sicurezza.**

Canali di segnalazione (in ordine di preferenza):

1. **GitHub Security Advisory** (preferito):  
   `Settings → Security → Advisories → New draft security advisory`  
   <https://github.com/d4ruerk1/PandaManager/security/advisories>

2. **Email GPG cifrata:**  
   Indirizzo operativo configurato nel repository.  
   Chiave pubblica disponibile su: <https://keys.openpgp.org/>  
   *(Aggiornare con indirizzo reale prima del rilascio pubblico)*

### SLA di risposta

| Gravità  | Conferma ricezione | Obiettivo fix |
|----------|--------------------|---------------|
| Critical | 24 h               | 7 giorni      |
| High     | 48 h               | 30 giorni     |
| Medium   | 72 h               | 90 giorni     |
| Low      | 7 giorni           | Next release  |

In caso di patch, verrà pubblicato un **GitHub Security Advisory** con CVE (se applicabile)
e aggiornata la **SBOM SPDX** (`pandamanager.spdx.json`).

### Scope

- Escalation di privilegi tramite il D-Bus helper
- Path traversal / directory escape nel browser file
- Command injection nei parametri rclone o nei comandi shell
- Esposizione di credenziali KWallet / Secret Service
- Bypass del filtro seccomp-bpf
- Vulnerabilità nelle dipendenze incluse (supply chain)

### Hardening binario (CRA Art. 13)

Flags di compilazione obbligatori in release:

```cmake
-D_FORTIFY_SOURCE=3
-fPIE
-fstack-protector-strong
-Wl,-z,relro
-Wl,-z,now
```

Opzionale ma raccomandato: abilitare `ENABLE_SANITIZERS=ON` in CI.

### SBOM e trasparenza supply chain (CRA Art. 13)

Una Software Bill of Materials (SBOM) in formato **SPDX 2.3** è distribuita
con ogni release: `pandamanager.spdx.json`.  
Endpoint SBOM: <https://github.com/d4ruerk1/PandaManager/releases/latest>

### Comunicazione alle autorità (CRA Art. 14 — D.Lgs. 138/2024 NIS2)

In caso di vulnerabilità attivamente sfruttata, il manutentore notificherà,
ove applicabile, entro 24 ore dalla scoperta:

- **ENISA** — European Union Agency for Cybersecurity, conformemente all'Art. 14 CRA (EU 2024/2847);
- **ACN** — Agenzia per la Cybersicurezza Nazionale (autorità NIS2 italiana),
  conformemente al **D.Lgs. 4 settembre 2024, n. 138** (recepimento Direttiva NIS2 2022/2555),
  artt. 23–24 (notifica incidenti significativi al CSIRT Italia);
- **Garante per la protezione dei dati personali** (<https://www.garanteprivacy.it/>),
  qualora l'incidente comporti una violazione di dati personali ai sensi dell'Art. 33 GDPR
  e dell'Art. 83 D.Lgs. 196/2003 (notifica entro 72 ore).

**CSIRT Italia:** <https://www.csirt.gov.it/> — segnalazioni@csirt.gov.it

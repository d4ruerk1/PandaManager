<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 PandaManager Contributors -->

# Informativa sulla Privacy — PandaManager

**Versione:** 1.0.0 — **Data:** 2026-10-24  
Conforme al **Regolamento (UE) 2016/679 (GDPR)**, in particolare agli artt. 13, 25 e 17,  
e al **D.Lgs. 30 giugno 2003, n. 196** (Codice in materia di protezione dei dati personali),
come modificato dal **D.Lgs. 10 agosto 2018, n. 101** di adeguamento al GDPR.

---

## 1. Titolare del trattamento

Il software PandaManager è un'applicazione open-source desktop. Non è gestito da un
soggetto commerciale centralizzato. Il titolare del trattamento è **l'utente finale**
che installa ed esegue il software sul proprio dispositivo.

Manutentore del progetto (contatto tecnico):  
**d4ruerk1** — <https://github.com/d4ruerk1/PandaManager>

---

## 2. Zero Telemetria — Privacy by Design (Art. 25 GDPR)

PandaManager **non raccoglie, trasmette né elabora dati personali** verso server esterni.
Non sono presenti:
- Tracker, analytics o ping di telemetria
- Cookie o identificatori persistenti trasmessi in rete
- Profilazione comportamentale

Conformità: **GDPR Art. 25** — Privacy by Design and by Default.

---

## 3. Dati trattati localmente

Tutti i dati rimangono **esclusivamente sul dispositivo dell'utente**:

| Dato | Posizione | Scopo | Base giuridica (Art. 6 GDPR) |
|------|-----------|-------|-------------------------------|
| Configurazione app | `~/.config/pandamanager/` | Funzionamento applicazione | Art. 6(1)(b) — esecuzione del contratto / uso del software |
| Credenziali cloud | KWallet / Secret Service D-Bus (cifrate) | Autenticazione servizi cloud opzionali | Art. 6(1)(a) — consenso esplicito dell'utente |
| Log operativi di audit | `~/.local/share/pandamanager/audit.log.enc` | Tracciabilità operazioni distruttive (sicurezza) | Art. 6(1)(f) — interesse legittimo (integrità dati) |

### Periodo di conservazione dei log
I log di audit sono conservati **localmente** fino a quando l'utente non li elimina.
Non esiste un periodo di conservazione automatico imposto dal software nella v1.0.0;
una funzione di rotazione automatica configurabile è prevista nelle versioni future.
L'utente può eliminare manualmente il file `audit.log.enc` in qualsiasi momento.

---

## 4. Diritti dell'interessato (Artt. 15–22 GDPR)

Poiché tutti i dati sono locali e sotto il controllo esclusivo dell'utente:

- **Art. 15 — Accesso:** i file di configurazione e log sono leggibili direttamente.
- **Art. 16 — Rettifica:** modificabile tramite le impostazioni dell'applicazione.
- **Art. 17 — Diritto all'oblio:** la cancellazione sicura con sovrascrittura multipla
  (standard DoD 5220.22-M, 7 passi; Gutmann, 35 passi) è disponibile direttamente
  nell'applicazione per qualsiasi file.
- **Art. 18 — Limitazione:** non applicabile (nessun trattamento esterno).
- **Art. 20 — Portabilità:** la configurazione è esportabile in formato JSON standard.
- **Art. 21 — Opposizione:** non applicabile (nessun trattamento per marketing o profilazione).

---

## 5. Trasferimenti internazionali (Art. 44 GDPR)

PandaManager **non trasferisce dati personali verso Paesi terzi**.  
Le connessioni ai provider cloud (Google Drive, OneDrive, ecc.) sono gestite
dall'utente tramite rclone; i termini di privacy di ciascun provider si applicano
indipendentemente da PandaManager.

---

## 6. Sicurezza dei dati (Art. 32 GDPR)

Misure tecniche implementate:
- Credenziali sempre cifrate (KWallet / libsecret, mai in chiaro)
- Log di audit cifrati (placeholder AES-256-GCM con chiave KWallet)
- Sandbox seccomp-bpf per limitare le syscall del processo principale
- Privilege separation tramite D-Bus helper per operazioni privilegiate
- Hardening binario: `-D_FORTIFY_SOURCE=3 -fPIE -fstack-protector-strong -z relro -z now`

---

## 7. Autorità di controllo (Art. 77 GDPR — Art. 141 Codice Privacy)

L'utente ha il diritto di proporre reclamo al **Garante per la protezione dei dati personali**
(autorità di controllo italiana), qualora ritenga che il trattamento dei propri dati violi
il GDPR o il D.Lgs. 196/2003:

- **Sito web:** <https://www.garanteprivacy.it/>
- **PEC:** garante@pec.gpdp.it
- **Telefono:** +39 06 69677.1
- **Indirizzo:** Piazza Venezia 11, 00187 Roma (RM)

Resta salva la possibilità di adire le vie giurisdizionali ordinarie (Art. 79 GDPR).

---

## 8. Contatti per la privacy

Per domande sulla gestione dei dati nel codice sorgente:  
**GitHub Issues / Discussions:** <https://github.com/d4ruerk1/PandaManager/issues>

Per segnalazioni di vulnerabilità relative alla privacy, seguire la procedura
di Coordinated Vulnerability Disclosure in [SECURITY.md](SECURITY.md).

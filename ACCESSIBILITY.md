<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 PandaManager Contributors -->

# Dichiarazione di Accessibilità — PandaManager

**Versione:** 1.0.0 — **Data:** 2026-10-24

Conforme a:
- **EN 301 549** v3.2.1 (2021-03) — requisiti di accessibilità per prodotti e servizi ICT
- **Direttiva UE 2019/882** (European Accessibility Act — EAA) — per prodotti ICT consumer
- **Legge n. 4/2004** (Legge Stanca, Italia) — accessibilità degli strumenti informatici,
  come modificata dalla **Legge 9 luglio 2019, n. 69** (recepimento EAA)
  e dal **D.Lgs. 10 agosto 2018, n. 106** (recepimento Direttiva 2016/2102 accessibilità PA)
- **D.Lgs. 7 marzo 2005, n. 82** (CAD — Codice dell'Amministrazione Digitale),
  artt. 7 e 53 — accessibilità dei servizi informatici
- **WCAG 2.1 Level AA** — Web Content Accessibility Guidelines (adattate all'UI desktop)

---

## Funzionalità di accessibilità implementate (v1.0.0)

### Tastiera e navigazione
- Navigazione completa da tastiera: Tab, Shift+Tab, frecce direzionali, Enter, Escape
- Shortcut documentate: F3 (split-pane), Alt+← (indietro), Alt+→ (avanti)
- Focus trap corretto nei dialoghi modali

### Screen reader (AT-SPI2)
- `setAccessibleName()` e `setAccessibleDescription()` applicati a tutti i widget
- Compatibilità verificata con **Orca** (GNOME) e **KDE Accessibility**
- Ruoli ARIA-equivalenti corretti per toolbar, breadcrumb, pannelli

### Contrasto e visibilità
- Indicatori di focus ad alto contrasto ≥ 3:1 (WCAG 2.1 SC 1.4.11 — Non-text Contrast)
- Rilevamento automatico tema KDE dark/light con adattamento cromatico
- Supporto temi ad alto contrasto KDE

### Ridimensionamento
- UI responsive: supporta zoom sistema KDE fino a 200%
- Font size rispetta le impostazioni di sistema

---

## Stato della conformità

| Criterio WCAG 2.1 | Livello | Stato in v1.0.0 |
|-------------------|---------|-----------------|
| 1.3.1 Info e relazioni | A | Conforme |
| 1.4.3 Contrasto (minimo) | AA | Conforme |
| 1.4.11 Contrasto non-testo | AA | Conforme |
| 2.1.1 Tastiera | A | Conforme |
| 2.4.3 Ordine focus | A | Conforme |
| 2.4.7 Focus visibile | AA | Conforme |
| 4.1.2 Nome, ruolo, valore | A | Conforme |
| Vista colonne (stile Finder) | — | Disponibile in v1.0.0 — navigazione tastiera completa prevista in v1.1 |
| QuickLook overlay tastiera | — | Parzialmente implementata — miglioramenti previsti in v1.1 |

---

## Problemi noti e roadmap

- **Vista colonne:** la navigazione da tastiera orizzontale tra colonne è parziale;
  sarà completata in v1.1.
- **QuickLook overlay:** la chiusura con Escape è funzionante; la navigazione interna
  con frecce sarà aggiunta in v1.1.

---

## Contatti per l'accessibilità

Per segnalare barriere di accessibilità:  
**GitHub Issues** (label `accessibility`):  
<https://github.com/d4ruerk1/PandaManager/issues>

Tempo di risposta obiettivo: **14 giorni lavorativi**.

---

## Procedura di reclamo (Legge 4/2004, art. 3-quinquies — AgID)

Qualora l'utente ritenga che una segnalazione di accessibilità non sia stata
adeguatamente gestita, può presentare reclamo o richiesta di arbitrato all'**AgID**
(Agenzia per l'Italia Digitale):

- **Modulo reclamo:** <https://form.agid.gov.it/view/form-accesibilita/>
- **Sito:** <https://www.agid.gov.it/it/design-servizi/accessibilita>

L'AgID è l'autorità di monitoraggio designata ai sensi dell'art. 7 della
Direttiva 2016/2102/UE e della Legge Stanca n. 4/2004.

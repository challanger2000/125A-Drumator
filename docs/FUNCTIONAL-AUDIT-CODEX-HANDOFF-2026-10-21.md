# 125A Drumator — Funktionsaudit und Codex-Pruefauftrag (21.10.2026)

## Status / unveraenderliche Basis

- Repository: `challanger2000/125A-Drumator`
- Technisch gepruefter **Produkt- und Teststand**: `d7e682544150f30ad51d50c45baa6afa9aa84ba2` auf `develop/v1.0.0`
- Windows QA: [#39650413485](https://github.com/challanger2000/125A-Drumator/actions/runs/39650413485) **SUCCESS**. Separat bestaetigt: Build, deterministische Regressionen, standardmaessiges Windows-VST3-Bundle und Validierung.
- **KEINE** Studio-One-Ausfuehrung durch den Assistenten, **KEINE** V1-Freigabe und **KEIN** finaler Bitmap-Skin.
- Dokumentationscommits nach diesem Basisstand erfordern ihre eigene Provenienz; ein gruener Run eines frueheren SHA gilt nicht automatisch fuer den spaeteren HEAD.
- Quellen: root `AGENTS.md`, `docs/V1-FUNCTIONAL-AUDIT-2026-10-19.md`, `docs/SELF-AUDIT-FOR-CODEX-2026-10-19.md`, `docs/INTEGRATION-QA-T01-T04-O01-Z01-Z03-2026-10-20.md`, aktuelle `src/` und `tests/`.

## Abgleich Funktionsbereiche

| Bereich | Nachgewiesene Implementierung / Tests | Offene Nachweise |
| --- | --- | --- |
| Sample Import / Preview | WAV / FLAC, Sample-Drop/Pick, Preview, Clearance, Kit-Recall; reale paketierte Host-Smokes | Sample-Browser-D&D und Preview in Studio One |
| Sequencer / Velocity | A-H / 1T-2T / 16-32 Steps; Undo/Fill/Velocity Lane/Random one-shot; Windows-Integration `testRandomVelocityOneShot`, `testFillActivePatternLength`, Velocity/Pattern Tests | Echte Maus-/Modifier-Bedienung und Velocity-Bar-Darstellung im Host |
| MIDI / Song | F05, F05b (nichtchronologische Event-Auflistung), T18, T24, T27; SongChain- und Pattern-Timeline-Tests | Studio-One MIDI-Patternwechsel, Song-A-B-A, Projekt-Reopen |
| Transport / Timing | Start Takt 2, Locate, Backloop, T16 outputless, T25 Restart gleiche PPQ; deterministisches Step-Scheduling | Ausgedehnte reale Song-Playback-/Swing-Driftmessung, Offline-Bounce-Vergleich, unterschiedliche Host-Blockgroessen |
| Multi-Out | 16 feste Stereo-Busse, hostgesteuerte Aktivierung, MAIN/AUX-Tests M27/M30/F08/T02 fuer bis zu 15 Stems; T20/21/23 Eingabegueltigkeit | Tatsachliche Aktivierung im Studio-One-Instrumentenrack und eindeutiges Routing ohne MAIN-Doppelung |
| Mixer / Voices | Pan/Level/Pitch/Choke/Solo/Mute, 64-Stimmen-Pruefungen, Master; D11 Extremwerte; Samples-Retrigger | Grenzbedienung Pan +100, laengere Echtzeitlast mit p99/max-Deadline-Erfassung |
| Speicher / Restore | Kits mit 16 Samples, korruptes/abgeschnittenes State-Rollback M34, wiederholter bytegenauer R26, T04 laufender Restore | Reales Speichern/Schliessen/Neuoeffnen eines Studio-One-Projekts |
| Controller / View | Windows-Integration mit nativer View, Zoom, Header-Hit-Test, Text-Edit, Song Editor und Precision Controls | Host-Resize, Zoom 130/150%, Sample-D&D und alle Hit-Zonen direkt in Studio One |

**Wichtig:** `docs/V1-SCOPE.md` ist ein Entwurf, keine bereits abgenommene V1-Spezifikation. Die optionale SP-1200-Klangfaerbung ist nicht implementiert und ohne explizite Entscheidung kein spaeter 'entdeckter' P1-Fehler. Die finale GUI wird **nicht** vor dem unabhaengigen Gegencheck begonnen.

## Eigenaudit: konkrete Rest-Risiken

1. **Studio-One-Host-Gates (MANUAL-UNVERIFIED):** Multi-Out-Busaktivierung, Browser-D&D, reale Editor-Resize-/Zoom-Reaktion, Mouse-Modifikatoren, Schritt- und Songbedienung, Projekt-Recall und Start ab Takt 2. Automatisierte Host-Fakes beweisen die echte Anwendung nicht.
2. **Realtimelast (NOT MEASURED):** Nachweise zu p95/p99/max, Audio-Overruns, 16 simultanen Stems, 64 Stimmen, Sample-Austausch und 44.1/48 kHz unter realistischen Blockgroessen fehlen als eigener Performancebericht. Keinen CPU-PASS erfinden.
3. **Race/Recall (REVIEW-REQUIRED):** `restoreGate_`, `selected_.restoreIfUnchanged`, `songMailbox_.snapshot`, `patterns_.snapshotCoherentForState` und `exchange_` unter konkurrierender MIDI-Automation / SetState / Datenpublikation gezielt auf Worst-Case Interleavings pruefen; aktuelle Windows-T03/T04/O01/Z01 belegen nur ihre definierten Orakel.
4. **Busvertrag (SOURCE-CONFIRMED; HOST-UNVERIFIED):** `setBusArrangements` nimmt exakt 16 Stereo-Busse an, zusaetzliche Ausgaenge sind `kMain` und bei Start inaktiv. Reale Studio-One-Aktivierung ist weiter ein zwingendes Owner-Gate.
5. **Dokumentationsdrift (CONFIRMED):** Historische Angaben in `AGENTS.md`, frueheren Audits und `tests/integration/README.md` referenzieren alte SHAs. Fuer den Codex-Gegencheck nur den aktuellen exakten Branch-HEAD als Codebaseline verwenden.

## Auftrag an Codex — unabhaengig, read-only zuerst

1. **Vor allem anderen** `125A-Engineering/START-HERE.md` samt relevanten Guardrails, Drumator `AGENTS.md` und aktuellen `develop/v1.0.0` HEAD selbst lesen. Weder alten Commit noch `main` als aktiven Stand annehmen.
2. Source-to-test-Matrix fuer **jede** V1-Pflichtfunktion erstellen: UI -> Controller -> Processor -> Audio/MIDI -> Kit/State -> Host/Automation -> wirklicher Test. Test-Orakel kritisch auf Fehlpositive, Fake-Stubs, ausgeschaltete Release-Asserts, falsch vorbereitete Fixture, Trigger-Order sowie fehlende Negativfaelle untersuchen.
3. Unabhaengig den gesamten Diff seit dem letzten Codex-Review und die aktuellen kritischen Pfade in `src/core/`, `src/vst3/`, `resource/`, `tests/`, CI und CMake pruefen. Reproduzierbare Bugs nach Schweregrad P0/P1/P2/P3 mit Datei/Zeile, Ursache und konkretem Testfall dokumentieren.
4. Live-Realtime (Heap/Locks/I-O), race condition, Song/MIDI Tie-Breaks, Host PPQ/Tempo/Loop/Offline-Render, 16 Stem-Routing/MAIN, Save/Restore, GUI/Host-Events und VST3-SDK-Compliance priorisieren. Bei einer Behauptung 'PASS' exakten SHA und Run-ID angeben.
5. **Keine** spekulativen Produktveraenderungen, **keine** versteckte GUI-Neugestaltung, **keine** unnötigen Actions-Runs. Nach Review identifizierte reproduzierte Defekte erst mit begruendeter Regression korrigieren und separat verifizieren.
6. Ergebnis als `docs/audits/CODEX-FINAL-FUNCTIONAL-RECHECK-2026-10-21.md` mit klarer Zuordnung `PASS / FAIL / NOT TESTED / OWNER HOST TEST` ablegen. Keine V1-Freigabe vor Owner-Studio-One-Tests.

## Reihenfolge fuer den Abschluss

**Aktuelles Funktionsaudit / nachweisbare Fehler beheben -> Codex-Gegenpruefung -> eventuelle Codex-Befunde reparieren und erneut pruefen -> Owner-Studio-One-Funktionsabnahme -> finale grafische VSTGUI.** Das Dokument ist ein nachpruefbarer *Zwischenstand/Handoff*, keine Behauptung, dass die komplette Abnahme abgeschlossen sei.

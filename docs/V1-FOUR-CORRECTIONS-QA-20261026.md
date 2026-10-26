# 125A Drumator – gezielte letzte Funktionskorrekturen vor V1

**Datum:** 26.10.2026 · **Status:** Vier Korrekturen implementiert, aktuelle Windows-QA bestanden. **KEINE V1-FINAL-FREIGABE.** Pluginversion weiterhin **0.1.0**.

## Verifizierte Quell-/Build-Baseline

- Repository: `challanger2000/125A-Drumator`; Entwicklungsbranch `gui/ppg-blue-background-20261024`.
- Ausgangsbasis: `c02c1679ec268b29ef586aca1cc4bdf4b0a2f6a6` (S13-Abschluss vor den vier Änderungswünschen).
- Fachlicher Fix-Commit: `617307bb8aae8312f92919e4935acfb5acd4b9e0`.
- Nachfolgende **ausschließlich Test-Fixture**-Anpassungen für das neue MAIN-Default-Routing: `6a7a674710e10bc1087b3613b9417827dee8af92`, `72f9bc7ca3897b2f32d9d51e1cec208c93517f63`, `dca83e6d138a967dc076af2df1a8e3bd4d376515`, `f50b1ee431584813068a7e7a7da155c265745fc3`.
- **Exakter gebauter VST3-Codecommit:** `f50b1ee431584813068a7e7a7da155c265745fc3`, [Windows Core/Integration Actions #40383029993](https://github.com/challanger2000/125A-Drumator/actions/runs/40383029993), **SUCCESS**.
- Produktpaket: [Windows-VST3-Artefakt 12679806888](https://github.com/challanger2000/125A-Drumator/actions/runs/40383029993/artifacts/12679806888), Archiv SHA256 `db0998daf18af917bc36d97a05bfafac469e7d642f9b923cb86e4227bca97871`, DLL SHA256 `8c24b310b291663d7a414c1a501ae221a848d43f7f2bcd3d7869fdda81fc3d0d`. ZIP-Integrität unabhängig geprüft (6 Einträge, keine defekten Mitglieder).
- Separater exakter Testworkflow-Pin: `4ac0322ce6c88c51519e7527b4150545c08955fc` (nur `.github/workflows/125a-plugin-tester-qa.yml`, kein neuer Core-Build), [125A Tester Windows #40384040046](https://github.com/challanger2000/125A-Drumator/actions/runs/40384040046), **SUCCESS**, [Tester-Report Artefakt 12680395053](https://github.com/challanger2000/125A-Drumator/actions/runs/40384040046/artifacts/12680395053). Tester EXE **0.3.2**, separat gepinnt an Tester-Code `91c975536a45190085304dcc28a43d93ff56ca88`, Windows Testerlauf `39823684764`; weder das alte Tester-V0.2.10-Release noch ein ungeprüfter neuester Download wurde verwendet.

## Änderungsumfang und erwartete Verträge

| Änderungswunsch | Implementierung | Wichtigste Regression |
|---|---|---|
| Ab Werk nur A mit Kick | `AtomicPatternBank` (Processor) und `Controller::initialize` enthalten nur bei Pattern A acht 32-Step-Kick-Positionen; B–H sind in **allen 32 Steps aller 16 Zeilen** leer. Factory-Ziel B sofort für DUPLICATE verfügbar. | `AtomicPatternTests`; echte `V1FIX` Controller/Processor-Initialisierung, vollständige 4096-Zellen- und DUPLICATE/UNDO-Prüfung |
| COPY ROW / PASTE ROW | Eigener sitzungsbezogener 32-Byte-Zeilen-Zwischenspeicher im Controller, getrennt vom ganzen Pattern. Die zwei Menüpunkte wurden an das **bestehende FILL-Zeilenmenü** angehängt. PASTE führt eine atomare `PatternReplace`-Transaktion aus und erfasst genau ein Undo im Zielpattern. | `V1FIX` mit tatsächlichen VSTGUI-Menüklicks, Quelle A/Zeile 2 → Ziel B/Zeile 3, inkl. 1-Takt-verstecktem Step 32, unterschiedlicher Velocity, unveränderter anderer Zeilen und Sound-/Output-Metadaten, UNDO |
| CLIP | Idle-Beschriftung **leer**. Nur gesetzte echte `OutputMeter::clipMask`-Übersteuerung (> 0 dBFS) erzeugt rote Meldung mit exaktem Namen `CLIP MAIN` oder `CLIP 2` … `CLIP 16`. LEDs, Clip-Hold und Reset-Aktion nicht verschoben. | `clipWarningText`-Orakel für idle, MAIN, OUT5, OUT16 und Priorität; bestehende native Meter-/Reset-/Mehrfachbus-Tests |
| Routing | Neue Processor-/Controller-Instanzen und leere `KitState`-Initialisierung: alle 16 Zeilen **MAIN**. OUT 2–16 weiter durch explizite Benutzerzuweisung anwählbar, unveränderte 16 Stereo-Busse/Audio-Engine. | `KitStateTests`, `V1FIX` Factory-/Recall-Prüfung; paketierte M27/M30-, F08-, T02-15-Stereo-Stems-, T29- und T04-Hosttests mit jetzt ausdrücklich gewählten AUX-Routen |

**State-/Bestandskompatibilität:** Für neuere gespeicherte Kits werden die bestehenden `output`- und `grid`-Daten geladen, **nicht** durch Defaults überschrieben. Der frühere implizite row-N→OUT-N-Verlauf wird bei älteren Formaten ohne gespeichertes Output-Feld (vor DM11 und DMR1–3) beim Dekodieren/Legacy-Recall ausdrücklich rekonstruiert; die ursprüngliche alte DMR1-Bassdrum in A–H bleibt beim Laden alter Projekte bestehen. Factory-Defaults gelten nur für tatsächlich neue Instanzen/Kits. Keine Parameter-IDs, Kit-Codec-Version, Audio-Routing-Engine oder Instrumentenmodule geändert.

**UNDO-Grenze:** DUPLICATE schreibt ein neues freies Zielpattern samt dessen Undo-Historie; für Rücknahme das Zielpattern auswählen. COPY ROW erfasst selbst keine Edit-Historie, erst das erfolgreiche PASTE ROW (nur geänderte Inhalte). Das bestehende einstellige Pattern-Undo-Modell wird nicht zu einer globalen Mehrschritt-History erweitert. Ein vollständig belegter Pattern-Satz wird nach wie vor nicht überschrieben.

## Tatsächlich ausgeführte Windows-QA

| Prüfpunkt | Exakte Evidenz | Resultat |
|---|---|---|
| Windows MSVC VST3 und native Tests | #40383029993 | **PASS** Build, **23/23 CTests**, 0 FAIL (Integrationstest #23, 109,51 s) |
| Neu: `V1FIX` | Integrationsexekution im obigen Build | **PASS**: Factory A/B–H, MAIN, DUPLICATE und UNDO, row copy/paste/undo, versteckte Steps, nicht-desktruktive andere Zeilen, gespeichertes Routing, volle Pattern-Bank |
| Steinberg Validator | #40383029993, offizielle SDK-Prüfung | **47 PASS / 0 FAIL** |
| Paketierte VST3, Editor und Bus-PCM | #40383029993 | **PASS**: Editorattach/Geometrie; WAV/MIDI/State/Realtime-Hostsimulation; 16 Busse; T02 mit 15 verschiedenen AUX-Stereo-Signaturen und L/R-Gegenproben; T04 mit 24 State-Restores |
| Unabhängiger 125A Plugin Tester **v0.3.2** am **neuen** Paket | #40384040046, exakte f50b1ee-DLL verifiziert | **58 PASS / 0 WARNING / 0 FAIL**, Exit 0, JSON status PASS; 50/50 Setup-Matrix |
| Referenz-CPU-Diagnostik | #40383029993, Windows shared CI runner | F14 64 Voices/16x p95 1,0948 ms, p99 1,1594 ms, max 1,1602 ms bei 48k/256, 0/180 Offline-Deadline-Misses; **keine ASIO-Zertifizierung** |

Die frühen Kandidatenläufe #40379445036, #40380789183, #40381301682 und #40382223230 waren **FAIL** (unter anderem wegen nicht mehr zutreffender Fixture-Annahmen zum alten automatischen AUX-Routing). **Nur** #40383029993 und #40384040046 belegen den erfolgreichen aktuellen Stand. Die Anpassung dieser Test-Fixtures ändert nicht das neue gewünschte Standard-Routing zurück.

## Verbleibende Grenzen und nächster Besitzer-Test

- **PASS** für die oben einzeln ausgewiesenen automatisierten Windows-/Hostsimulationsprüfungen.
- **Vom Nutzer bereits berichtet:** Studio One 8.1.2 Pro lief mit dem vorherigen VST3-Kandidaten bei Speichern/Laden, ohne bisherige Audio-Aussetzer oder andere Auffälligkeiten. Dieser positive Praxisbefund gilt **nicht als Test der jetzt veränderten DLL**.
- **NOT RUN:** interaktive Studio-One-8.1.2-Pro-Nachprüfung **mit der DLL SHA256 8c24b310…**, insbesondere FILL-Kontextmenü COPY ROW/PASTE ROW, A→B DUPLICATE/UNDO, CLIP-Warnung und alle Zeilen standardmäßig MAIN. Echte Mehrprozess-Host-Meterkommunikation bleibt separat nicht vollständig nachgewiesen.
- Keine Änderung an Main, keine V1-FINAL-Markierung, keine fiktive Benutzerabnahme. Erst nach erneuter Praxisprüfung kann der Besitzer die finale Freigabe erteilen.

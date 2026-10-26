# 125A Drumator – Schritt 13: Systematischer Quellcode-, Test- und Reparatur-Audit

**Datum:** 26.10.2026 · **Status:** WINDOWS-QA GEPRÜFTER ENTWICKLUNGSKANDIDAT, **KEINE V1-FINAL-FREIGABE**. Externe Host-Abnahme **NOT RUN**. Dieser Bericht ersetzt den vorläufigen Stand vom gleichen Tag.

## 1. Baseline, Prüfumfang, QA-Grenze
- Source-Baseline vor Schritt 13: `43a28c594b625642ca592162e00f8d6180b10b7e`; bestätigter Windows-Lauf [#40329346263](https://github.com/challanger2000/125A-Drumator/actions/runs/40329346263) **SUCCESS**, 23/23 CTests, 47/47 Steinberg Validator.
- Ziel: `challanger2000/125A-Drumator`, Branch `gui/ppg-blue-background-20261024`; `0.1.0`, **unveröffentlichter** Kandidat; weder eine veröffentlichte FULL-Baseline noch ein Release-Manifest wurde erfunden.
- Kontrolliert: GitHub-HEAD, `AGENTS.md`, Engineering START-HERE, Engineering Guardrails, Standards DSP/GUI/Professional-Behavior/Repository-Workflow, Release-/Realtime-/Audio-Fixture-QA, Source-Registry, Roadmap. Der Git-Tree enthält 168 Einträge; 89 wesentliche Build-, Source-, Test- und Integrationsdateien wurden über GitHub geladen und auf verdächtige Synchronisations-/Allokations-/Grenzpfade untersucht. Kritische reale Audio-/Controller-/Editor-/Kit-/Sample-/Scheduler-/VST3-Pfade wurden zusätzlich im Zusammenhang nachverfolgt. Dies ist kein Beleg für semantische Vollständigkeit jedes denkbaren Host-Verhaltens.
- Nach Reparaturen: [Aktueller Branch/Windows Actions](https://github.com/challanger2000/125A-Drumator/actions/workflows/core-ci.yml). Die exakte erfolgreiche Run-ID des letzten Code-/Dokumentationscommits ist bei dessen Abschluss mit der GitHub-API zu kontrollieren; der zuletzt bereits verifizierte Reparaturlauf [#40362353314](https://github.com/challanger2000/125A-Drumator/actions/runs/40362353314) meldet 23/23 und 47/47 PASS sowie S13-Mehrfachansicht-/IPC-Test PASS.

## 2. Befundregister

| ID / Gewichtung | Datei, Funktion, reproduzierbare Ursache | Reparatur und Nachweis |
|---|---|---|
| S13-01 HIGH | `src/core/OutputMeter.h::meterRegistry`, `Processor::connect`, `Controller::notify`. Eine prozesslokale Registry ist über Host-Prozessgrenzen nicht erreichbar; Controller-Pointer konnte NULL bleiben und die GUI still anzeigen, während bisherige In-Process-Tests PASS meldeten. | VST3-`Drumator.MeterRequest/MeterSnapshot` mit kontrollthreadseitigem Antworttransport und rein atomaren Audio-Messwerten. Der Controller verwendet die erhaltenen Daten unabhängig von `findMeter`. Negativtest injiziert einen nicht auflösbaren Registry-Eintrag und prüft echten Processor-/Controller-/View-Pfad. Ein real getrenntes DAW-Prozessmodell bleibt **NOT RUN**. |
| S13-02 HIGH | `DrumatorView::pollMeter()`: zwei GUI-Fenster `exchange(0)` auf denselben LEFT/RIGHT-Atomics; erste Anzeige löschte die zweite. | Views lesen nun dieselbe Controller-Momentaufnahme pro Audio-Generation. Zwei echte verbundene Editoren prüfen identische Kanal-Peaks, Clip-/Reset und Leerlauf-Verfall. |
| S13-03 MEDIUM | Anfangsreparatur zeigte per `lastPair` nur den jeweils **letzten** Block. Ein kurzer Peak vor einem leisen Block hätte die GUI nicht erreicht. | Peak-Maximum wird über mehrere Audio-Callbacks gesammelt, bis der Controller es abruft. Bounded CAS in `OutputMeter::accumulate`; Test mit lauter Transiente gefolgt von stillem Block und notwendigem Peak-Erhalt. |
| S13-04 MEDIUM | Der bisherige 11I-Meter-Test verwendete direkt `Controller::meterLink()` und konnte einen Prozess-Grenzfehler nicht erkennen. | Neue End-to-End-Negativkontrolle mit nicht auflösbarer Registry, zwei Editorfenstern und tatsächlichen Processor-PCM-Ausgaben. |
| S13-05 MEDIUM | Einzel-OUT-Übersteuerung allein beweist keine korrekte Überwachung bei mehreren gleichzeitig aktiven Bussen bzw. exaktem Grenzpegel. | Direktes Produkt-`OutputMeter::observe`-Orakel: OUT2 exakt 0 dBFS, OUT3 über 0 dBFS, danach zwei gleichzeitig übersteuernde Stems und deaktivierter OUT3. |
| S13-06 RELEASE-RISK | Realer Studio-One-/ASIO-Host, getrennte Plugin-Prozesse, 125A Plugin Tester und fremde Hosts können nicht aus einer Windows-CI-Simulation abgeleitet werden. | **NOT RUN**. Externe Abnahme bleibt notwendig. **KEIN V1-FINAL**. |

Im normalen Audioblock gibt es weiterhin **keine** IConnectionPoint-/GUI-Nachrichten. `OutputMeter::observe()` liest nach dem Master-Gain die aktiven berechneten PCM-Ausgänge und veröffentlicht Peaks/CLIP über atomare Werte; GUI-Lichtabfall und Hold bleiben UI-seitig. Die Nachrichtenerstellung, Verarbeitung und Controller-Caches laufen nur außerhalb `process()`. Es wurden keine neuen Parameter-IDs, Audiorouten, GUI-Geometrien oder serialisierten State-Felder eingeführt.

## 3. Anforderungs- und Testabdeckungsmatrix (bestehende Tests kritisch bewertet)

| Funktionsvertrag | Reale Tests/Evidenz | Ergebnis / verbliebene Einschränkung |
|---|---|---|
| 64 Voices, Voice-Stealing, Choke, Mute/Solo, Sinc, Mono/Stereo, Gain/Pan/Pitch | SampleVoiceTests; F14/F15; RE09; RE10; A26; SDK-Hostsimulation | Umfangreiche direkte PCM-/Voice-/Timing-/16-Bus-Orakel; reale Studio-One-ASIO-Ausführung **NOT RUN**. |
| 44,1/48 kHz, 64/128/256/512 Frames, 4-/16-fache SRC-Verhältnisse | `testReauditDeadlineMatrix` / RE10, 16 **aktivierte** Stereo-Busse mit 16 eigenständigen WAVs, 64 Stimmen, 32 Blocks pro Kombination, Allokationsprobe | **VORHANDEN** und im älteren Zwischenbericht zu Unrecht übersehen. Kein Nachweis für jedes beliebige Audiogerät/CPU-Scheduler-Verhalten. |
| Sampletausch, Stop/Play, Loop, Seeks und CPU-Deadline | RE09, A10–A26, A26 500 paced 48k/256 Blöcke/16 Busse/64 Voices/12 Imports, TORTURE 96 Sequenzen, RM-Mutationskontrollen | Positive Produktkette und Negativ-Orakel vorhanden. Keine native ASIO-Dropout-Zertifizierung. |
| MIDI Pattern 24–31, Pattern A–H, Hostautomation, Prioritäten, Chain, 1/2 BAR, Song 64 | FunctionalAuditCases, TimelineTests, SongChainTests, RE01/RE03/RE11, FA02/FA03, A05/A13–A23, paged Song GUI | Echte Processor-/Controller-Timeline- und Zustandsprüfungen, inklusive Cold-SONG in eigenem Kindprozess; Studio-One-Projekt-Recall **NOT RUN**. |
| WAV/FLAC, beschädigte Inputs, Rücknahme bei Fehler, Sample Editor, Stereo-Fade, Preview | WavDecoderTests, SampleEditTests, WE01, W11F, F13, FA04/FA23, FR05, RV01–RV03 | Tatsächliche Datei-/PCM-/Editor-Mauskontrollen; beliebige exotische FLAC-Kodierungen und Benutzerdateien nicht vollständig beweisbar. |
| 16 feste Stereo-Busse, MAIN/Samples, Aktivierung und Clip | RE10/A26; 11I MAIN/OUT2 und S13 Mehrfach-Clip-Test | Mehrfach-Clip-Bits und Grenzen werden auf actualer `OutputMeter`-Funktion getestet; Busaktivierung im Windows-Host; echte Studio-One-Mehrfachausgänge **NOT RUN**. |
| Mehrere GUI-Instanzen, Windows Hitboxen, 100/130/150% Zoom, Doppelklick-Reset, SONG, Wave | V18, Z02, native screenshots, FR04/FR05/FR10, 11I, Schritt12A/12C, SA01 | Vollständige editorinterne Steuerung und Hitflächen weitgehend abgedeckt; echte manuelle Host-Interaktion weiterhin **NOT RUN**. |
| Zustandsverwaltung und atomarer Recall (DMR/DMxx) | KitStateTests, Z01, RE02/05/06, Z03, FA23/FR08, A24/A25, viele OOM-Fehlerinjektionen | Definierte Controller/Processor-Rollback-/Speichervorgänge; echter DAW save/close/reopen nicht erfolgt. |
| Packaged final VST3 und SDK Validator | PackagedAudioHostSmoke, EditorGeometrySmoke, CTest 23, 47/47 Validator | Echte Windows-Modulverpackung und Host-Simulation, aber nicht 125A Plugin Tester/Studio One. |

### Fehlertoleranz und Testqualität
- Bestehende Integrationssuite benutzt reale `Processor`, `Controller`, `DrumatorView`, VST3-`ProcessData`, Host-Message-Connections und echte PCM-Fixtures, nicht nur DSP-Mocks.
- **Positive und negative Orakel:** RM01–RM29 sowie andere Mutationskontrollen können gezielt das Weglassen/Verzögern von MIDI-, Transport-, Import- und Peak-Verhalten entdecken. Nicht jeder Produktpfad besitzt jedoch eine Negativmutation.
- Typische falsche PASS-Risiken: GUI-Pixelpräsenz ist keine reale Hostfunktion; Steady-State-PCM ist kein transportgetreuer Loop-/Stop-Nachweis; gleiche Prozessadresse ist keine getrennte Host-Prozess-Kompatibilität; grüne Validator-Ergebnisse ersetzen keine Praxisprüfung.
- Aufbauende Softwaretests validieren die definierten Varianten; unbekannte Drittanbieter-Kombinationen und Endlos-Dauerläufe sind nicht mathematisch ausgeschlossen.

## 4. Realtime- und CPU-Messwerte

Gemessen im Windows-CI-Runner, nicht auf einem lokalen Studio-One/ASIO-System:

| Lauf, Bedingung | p95 | p99 | Maximum | Deadline | Misses |
|---|---:|---:|---:|---:|---:|
| Ausgangsrun #40329346263, F14 64 Voices/16x 48kHz/256 | 1,088 ms | 1,186 ms | 1,318 ms | 5,333 ms | 0/180 |
| Erster Meter-IPC-PASS #40361078125, F14 gleiche Nominalbedingungen | 1,086 ms | 1,171 ms | 1,237 ms | 5,333 ms | 0/180 |
| Peak-Akkumulation #40362353314, F14 gleiche Nominalbedingungen | 1,157 ms | 1,234 ms | 1,379 ms | 5,333 ms | 0/180 |

Runner-Last und Scheduling unterscheiden sich; diese Werte dürfen nicht als gepaarte kausale CPU-Regression oder als DAW-Aussetzerfreiheit interpretiert werden. RE10 enthält zusätzlich die dokumentierte 44,1/48kHz- und 64/128/256/512-Matrix. Der finale genaue Code-/Dokumentationscommit muss in seiner eigenen CI überprüft werden.

## 5. Änderungen und Compliance
Änderungen auf dem Entwicklungsbranch beschränkt auf `src/core/OutputMeter.h`, `src/vst3/DrumatorProcessor.cpp`, `src/vst3/DrumatorController.cpp/.h`, `src/vst3/DrumatorView.cpp/.h` und `tests/integration/WindowsIntegration.cpp` sowie diesen Bericht. Die vorher genehmigten GUI-Materialien, Positionen, Knöpfe, Hitboxes, SONG-Editor, Samples, State-Serialisierung, Audio-DSP-Algorithmen und Pluginversion 0.1.0 sind absichtlich unverändert. Zwei neue IPC-Message-IDs sind **kein** neuer musikalischer Parameter.

## 6. Abnahmegrenzen und verbleibende Risiken
- **PASS (Windows)** nur für tatsächlich ausgeführte Einzel- und Integrationsprüfungen sowie Validator am exakten verifizierten Commit.
- **NOT RUN:** Studio One 8.1.2 / Windows ASIO in realem Projekt, getrennte Audio-/GUI-Prozesse eines realen Hosts, 125A Plugin Tester, andere DAWs und langfristiger multi-plugin Lastbetrieb.
- **OPEN:** unabhängige technische Abnahme und verbindliche Nutzerfreigabe der V1. Eine erfolgreiche CI-Run-Bestätigung ist dafür nicht ausreichend.
- Keine Änderung auf `main`, keine V1-FINAL-Benennung oder Release-Manifeste.

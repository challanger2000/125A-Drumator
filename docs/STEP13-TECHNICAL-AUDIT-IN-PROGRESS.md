# Schritt 13 – Quellcode- und Test-Audit (laufender Befundstand, NICHT ABGESCHLOSSEN)

## Provenienz und Freigabe
- Ausgangscommit: `43a28c594b625642ca592162e00f8d6180b10b7e`.
- Repository: `challanger2000/125A-Drumator`; Entwicklungsbranch `gui/ppg-blue-background-20261024`.
- Produktstatus: Version 0.1.0, Entwicklungskandidat, weder V1-FINAL noch owner-approved FULL.
- Referenz-QA: [Windows Actions #40329346263](https://github.com/challanger2000/125A-Drumator/actions/runs/40329346263), **SUCCESS** laut API für den genannten Ausgangsstand; Nutzer nennt 23/23 CTests und 47/47 Validator.
- Engineering-Referenz: `125A-Engineering/START-HERE.md`, `ENGINEERING-GUARDRAILS.md`, `STANDARDS/REPOSITORY-WORKFLOW.md`, `STANDARDS/DSP-GUIDELINES.md`, `STANDARDS/PROFESSIONAL-PLUGIN-BEHAVIOR.md`, `QA/RELEASE-QA.md`, `QA/REALTIME-QA.md`, `QA/AUDIO-FIXTURES.md`.
- Diese Datei ist eine **offene, evidenzbasierte Befundsammlung**, ausdrücklich keine Abnahme. Keine DSP-/GUI-/State-Fixes durch diesen Dokumentationscommit.

## Quell- und Testinventar (Stand Ausgangscommit)
Die rekursive Git-Tree-Abfrage ergibt 168 Einträge und ist nicht abgeschnitten. Enthalten sind Core-Header, VST3 Processor/Controller/View, FLAC-Implementierung, 23 CTest-Ziele einschließlich realer Windows-Integrationsfälle, CMake, CI und Instrumentierungsdateien. Die folgenden Befunde beruhen auf konkreter Quellinspektion, **nicht** auf einer ausgeführten Fehlerreproduktion.

## Kritische Befunde und Gegenproben

### S13-01 – HIGH: Meter-Live-Verbindung funktioniert nicht prozessübergreifend
- **Quellen:** `src/vst3/DrumatorProcessor.cpp` Konstruktor/`connect()`; `src/vst3/DrumatorController.cpp::notify()`; `src/core/OutputMeter.h::findMeter()`.
- **Ursache:** Die Processor-Seite speichert eine `shared_ptr<OutputMeter>` in einer *prozesslokalen* statischen `unordered_map` und sendet nur die numerische `MeterId`. Die Controller-Seite versucht denselben Eintrag über `findMeter()` aus ihrem lokalen Prozessraum zu lesen. In Host-Architekturen mit getrennten Processor-/Controller-Prozessen existiert dort keine gemeinsame Map/kein Shared-Memory-Transport; `findMeter()` liefert keinen Verweis.
- **Folge:** Die im selben Prozess bestandenen Meter-Tests können trotz fehlender Live-Pegel und CLIP-Anzeige im getrennten Host erfolgreich sein.
- **Benötigter Negativtest:** Echte Processor-/Controller-Instanzen in zwei Windows-Prozessen, VST3-Verbindung über Host-IPC, definierte PCM-Messung und überprüfte Anzeigen; Meldungs- und Deaktivierungsfälle. **NOT RUN**. Architekturentscheidung und Fix offen.
- **Status:** **OFFEN / release-blocking**, nicht als Windows-Host-PASS ausgegeben.

### S13-02 – HIGH: Mehrere gleichzeitig geöffnete Editoren stehlen sich MAIN-Peaks
- **Quellen:** `src/vst3/DrumatorView.cpp::pollMeter()`; `src/core/OutputMeter.h::mainL/mainR`.
- **Ursache:** Jeder GUI-Timer liest `mainL.exchange(0)` und `mainR.exchange(0)`. Damit löscht Editor A die Messung vor dem nächsten Poll von Editor B. Die beiden Meter können je nach Timerphase stark voneinander abweichen, obwohl sie dieselbe Processor-Instanz beobachten.
- **Benötigter Positiv-/Negativtest:** Zwei echte verbundene `DrumatorView`-Instanzen, identischer definierter Main-Peak, verschiedene Poll-Reihenfolgen, unabhängiger Peak-Hold, GUI-Lifecycle; bei der bisherigen Implementierung ist die Zweit-View-Messung nach dem ersten `exchange(0)` zwangsläufig null.
- **Status:** **OFFEN / release-blocking**. Lösung benötigt nichtdestruktiven versionierten Peak-Datentransport und Liveness-/Stillebehandlung, anschließend CPU-/Pacing-Nachweis.

### S13-03 – MEDIUM: Echtzeitnachweis derzeit nur Teilmatrix
- **Quellen:** `tests/integration/WindowsIntegration.cpp` paced Callback-Stresstest; `src/vst3/DrumatorProcessor.cpp::process()`; `src/core/OutputMeter.h::observe()`.
- Der vorhandene PACING-Lauf misst 500 Aufrufe bei 48 kHz, 256 Frames, parallel 12 Controller-Sample-Imports, verfolgt Maximalzeit und Deadline-Misses und wertet p95/p99 außerhalb des Audiothreads aus. Das ist wertvoll, aber **kein** Beweis für 44,1 kHz, 64/128/512/1024 Frames, langen Loop-/Stop-/Play-Betrieb und Studio-One-ASIO mit Belastung anderer Plugins. Der 16-Bus-Meter-Scan erhöht die Arbeit um die Anzahl aktiver Kanäle und darf nur mit gepaarten Messungen vor/nach Änderung als kostenneutral bezeichnet werden.
- **Status:** Relevante Teile **BESTEHEN ALS TEST**, geforderte vollständige Matrix **NOT RUN**.

### S13-04 – MEDIUM: Meter-Fixiertheit und Automation der Anzeige nicht unabhängig getestet
- **Quelle:** `tests/integration/WindowsIntegration.cpp::test11IMeterStereoAndStems`.
- Vorhandene Assertions prüfen MAIN-L/R, Grenze 0 dBFS versus oberhalb, einen OUT-2-Übersteuerungsfall, die UI-Rücksetzung und den normal integrierten Processorausgang. Noch nicht automatisch belegt: zwei gleichzeitig übersteuerte Stems, mehrere unabhängige GUI-Timer, getrennte Host-Prozesse, Timer-Abfall während des Stopps und Multi-Out-Reaktivierung nach Host-Recall.
- **Status:** **TEILABDECKUNG**.

## Bisher nachgewiesene und offene Testbereiche
| Bereich | Reale Produktkette im vorhandenen Test? | Hauptlücke | Stand |
|---|---|---|---|
| Processor PCM, Timing, Choke, Voice/Bus-Stress | Ja, in nativen Integrations-/RE-Fällen | Vollständige DAW-/ASIO-Pacing-Matrix | TEILABDECKUNG |
| SONG, Paging, Pattern, Zoom, Sample-Import | Ja, mehrere echte Controller/View-/PCM-Fälle | Eigentlicher Host-Save/Close/Reopen in Studio One | TEILABDECKUNG |
| MAIN- und OUT-2-Meter | Ja, direkte reale Process-/View-Fälle | Separate Prozesse und mehrere Views | **KRITISCH UNGENÜGEND** |
| Mono-FLAC-Import | Ja, echter 48k/16bit Mono-Import | Mehr Format-/Fehlerklassen und langer Import | TEILABDECKUNG |
| VST3 Validator | Laut Ausgangsrun 47/47 | Nicht gleichbedeutend mit End-to-End-Host/QA | BESTANDEN (Ausgangsrun) |
| Studio One/ASIO | Keine autorisierte DAW-Ausführung | Host-spezifische Realtime, Bus und Editor | **NOT RUN** |
| 125A Plugin Tester | Nicht in dieser Audit-Ausführung | Abnahme | **NOT RUN** |

## Maßgaben für weitere Arbeit
1. Reproduktion und Testadäquanz für S13-01 und S13-02 vor einer Reparatur absichern.
2. Transportarchitektur der Meterdaten prozesssicher und realtime-fähig gestalten; nicht mit einer weiteren prozesslokalen Map kaschieren.
3. Mehrfach-Editor-/CLIP-Stress einschließlich Lebensdauer, Poll-Reihenfolge, Scroll-/Host-Stop und aktiver Busmasken prüfen.
4. Weitere Vollketten (MIDI 24–31, Song 64 Einträge, Audio-Exchange und Kit-Recall, WAV/FLAC negatives, Sample-Editor, Build- und QA-Skripte) vollständig systematisch prüfen; vorliegende Teilinspektion ersetzt das nicht.
5. CPU p95/p99/max und Deadline-Misses bei 44,1/48 kHz und praxisrelevanten Blockgrößen mit gepaarten Builds messen.
6. Danach erst Windows-Gesamt-QA und unabhängige Freigabe.

**Abschlussstatus: IN PROGRESS / NOT VERIFIED. Keine vollständige Reparatur, keine V1-FINAL-Freigabe und kein vollständiger Gesamt-Audit behauptet.**

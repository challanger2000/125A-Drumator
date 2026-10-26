# 125A Drumator – gezielte Reparatur des unabhängigen V1-Audits

**Auftrag:** A01, A02, A03, A06 reparieren; A04 und A05 verbindlich abgrenzen; A07/A08 getrennt abnehmen.  
**Ausgangsrevision:** `471116f4bed26487ec84aaa486b2d93df628807f`, Branch `gui/ppg-blue-background-20261024`.  
**Version:** weiterhin `0.1.0`, unveröffentlichter V1-Kandidat.  
**Änderungsumfang:** kein neues Instrument-/DSP-Feature, kein GUI-Redesign, kein Merge nach `main`, keine V1-FINAL-Freigabe.

## Befunde und Maßnahmen

| Befund | Vorherige Ursache | Reparatur / Scope | Regression |
| --- | --- | --- | --- |
| A01 | Mutiertes `IMessage` wurde fälschlich als unmittelbare Antwort gelesen; funktioniert nicht zuverlässig über Host-Proxys. | Separate `Drumator.SampleEditorReply`/`Drumator.PatternReply`; monotone RequestId, veraltete Antworten ignorieren; Referenzlebensdauer per `addRef/release`, nicht-blockierende Sample-Editor-Anforderung und Timeout; metadata-only Antworten bei unveränderter WAV-Revision, neue PCM-Daten nur bei geänderter Quelle. | `A01SerializedConnection` kopiert sämtliche Request-/Reply-Attribute und WAV-Binärdaten, hält Rückantworten zurück; Sample-Editor-WAV, Stereo, verzögertes Pattern-Recall, spätere Hostautomation und veraltete Antworten bei Projekt-Recall. Vorhandene Cache-/OOM-/GUI-/Recall-Tests bleiben obligatorisch. |
| A02 | Tester-Gate verwendete fest `989f454` und einen alten DLL-Hash. | Reusable Workflow `125a-plugin-tester-qa.yml` läuft erst nach erfolgreichem Windows-Build desselben GitHub-Runs. Prüft Run/Source, eindeutiges Artefakt, DLL-SHA256, Tester-Binär-SHA256 und Tester 0.3.2; archiviert TXT, JSON und `candidate-provenance.json`. | GitHub-Job-/Artifacts-/Report-Evidenz auf exakt dem neuen HEAD; 58/0/0 des alten Pakets ist **kein** neuer Tester-PASS. |
| A03 | Nach Füllen des 2048er Trigger-Arrays wurden zusätzliche gültige Events still verworfen. | Einfügereihenfolge und 2048-Grenze bleiben echtzeitfest; `TriggerBatch` zählt ausschließlich gültige Overflow-Rejections. Processor akkumuliert den Zähler lock-frei; lesbar als `TriggerDrops` in der bestehenden Meter-IPC, ohne UI-Redesign. | `TriggerBatchTests` plus echte Processor-Integration: 2048/2200 gültige Noten = 0/152 Drops, ungültige MIDI-Noten verändern den Zähler nicht. |
| A04 | Nur VST3-Pattern-Parameter registriert. | Bewusste V1-Scope-Entscheidung: **nur A–H Pattern** als DAW-automatisierbarer Parameter; andere Regler speicherbar, keine neue DAW-Automation. | `docs/V1-SCOPE.md`; vorhandene Pattern-Automation, Projekt-Recall regressionsprüfen. |
| A05 | SONG BAR berechnet 16 Sechzehntel pro Takt. | V1-SONG ausschließlich **4/4**; keine neue Time-Signature-Engine. | `docs/V1-SCOPE.md`; bestehende BAR2/BAR4-/REST-/Cycle-Tests. |
| A06 | Submodulstatus und externe `dr_flac.h`-Bytes nicht vollständig geprüft. | SDK `v3.8.1_build_84` mit exakter SHA, alle rekursiven Submodule müssen aus dem gepinnten Gitlink sauber ausgecheckt sein. Pinned `dr_flac.h` zusätzlich mit SHA256 `111144e778f55738db6851cb226015c419e00d04b916a09506d4856d9cff945c`. | CMake `EXPECTED_HASH` und Windows-Workflow-Fail-Closed. |
| A07 | Studio One 8.1.2 / ASIO, Stop-Play, Cycle, Multibus, realer Sample-Drop und Projekt-Neustart nicht vor Ort geprüft. | **NOT RUN / FREIGABEBLOCKER**: spätere Nutzerabnahme in Studio One. | Außerhalb CI; keine Freigabe allein aus Windows-Smokes. |
| A08 | Offscreen VSTGUI entspricht nicht garantiert realer Studio-One-HWND-Darstellung. | **NOT RUN / FREIGABEBLOCKER**: spätere Nutzerprüfung 100/130 % und Ressourcen im installierten VST3. | Außerhalb CI. |

## Betroffene Dateien

`src/vst3/DrumatorProcessor.cpp/.h`; `src/vst3/DrumatorController.cpp/.h`; `src/vst3/DrumatorView.cpp/.h`; `src/core/TriggerBatch.h`; `tests/integration/WindowsIntegration.cpp`; `tests/integration/FinalAudit20261023.inl`; `tests/integration/ReauditInstrumentation.cmake`; `tests/TriggerBatchTests.cpp`; `.github/workflows/core-ci.yml`; `.github/workflows/125a-plugin-tester-qa.yml`; `CMakeLists.txt`; `docs/V1-SCOPE.md` und dieses Dokument.

## Qualitätsnachweis und Freigabestatus

Die Reparaturen werden auf einem **einzigen kohärenten Produktcommit** gebündelt. Der Core-Workflow muss mit dessen exakter SHA folgende Ausgaben belegen: alle CTests, 47/47 Steinberg Validator, paketierter DLL-Audio-/Editorhost, verifizierte Ressourcen. Der nachgelagerte Tester-Job muss die aktuelle DLL-SHA256 und den aktuellen Source-Commit zusammen mit seinem eigenen Ergebnis archivieren. Tester-Status und reale Action-ID werden **erst nach tatsächlich abgeschlossener Ausführung** eingetragen; ein geplanter Test ist kein PASS.

**QA-Abschlussstand (26.10.2026):** Technische Windows-Prüfung für **exakt `7da6d5a4fd42a76fd8a561b9637ee8ebdd810c13`** bestanden; Studio One/ASIO und native Abnahme im tatsächlichen Anwenderhost **NOT RUN**. V1-FINAL ausdrücklich **NICHT** erteilt.

## 26.10.2026 — tatsächliche Windows- und unabhängige QA-Evidenz

### Diagnose des ersten Reparaturlaufs (FAIL, nicht unterdrückt)

- Reparaturstand `41f58dbfbf46ab742bcd7bffe49543799fa790a0`, [Actions #40409623269](https://github.com/challanger2000/125A-Drumator/actions/runs/40409623269): Kompilation und SDK-Prüfung PASS; **22/23 CTests**, ein Integrationstest FAIL: `A01 independent copied and delayed Processor/Controller IPC` mit `A01 separate delayed Wave reply did not open real stereo editor`. Der A03-Processor-Überlauftest bei 2200 MIDI-Hits PASS. Validator, Paket-Hosttest und unabhängiger Tester SKIPPED, daher keine irreführenden PASS-Behauptungen.
- Root-Cause-Fix in `7da6d5a`: Die bereits eingetroffene SampleEditorReply wird exakt einmal als gültige ausstehende Antwort verarbeitet (`waveReplyFresh_`), bevor die nächste Metadaten-/Revision-Abfrage erlaubt wird. Ohne diese Trennung verdrängte eine synchron zugestellte Reply beim verschachtelten Öffnen des Editors die noch nicht konsumierte Wellenform. Der Proxy-Regressionstest erzwingt **genau eine** kopierte Abfrage und überprüft erst danach die getrennt zugestellte Antwort. Keine Audio-/DSP-Änderung im Nachkorrekturcommit.
  
### Verifizierter Abschlusslauf

**[GitHub Actions #40410436596](https://github.com/challanger2000/125A-Drumator/actions/runs/40410436596), vollständiger Jobstatus SUCCESS, Commit `7da6d5a4fd42a76fd8a561b9637ee8ebdd810c13`.**

| Prüfschritt | Tatsächliches Ergebnis | Evidenz |
| --- | --- | --- |
| Windows x64 MSVC Release + gepinnter SDK-Checkout | PASS | Job `windows`, SDK-Hauptrevision `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, rekursive Submodule, CMake dr_flac SHA256-Gate |
| Deterministische CTests | **23/23 PASS**, 0 FAIL | `windows` Joblog, `100% tests passed, 0 tests failed out of 23` |
| A01 kopierender IPC-Proxy | **PASS** | `A01 independent copied and delayed Processor/Controller IPC PASS`; abgedeckt: verzögerte Editor-/Pattern-Replies, andere Anfrageobjekte, Projekt-Recall/Stale-Replies, Hostautomations-Priorität |
| A03 echter Processor-MIDI-Overflow | **PASS** | `A03 actual processor 2200-event bounded overflow PASS`; 2048 akzeptiert/152 nachvollziehbare Drops, ungültige Events ausgenommen; Meter-IPC-Gegenprüfung |
| Steinberg Validator | **47/47 PASS** | `Result: 47 tests passed, 0 tests failed` |
| Paketierter Editorhost/GUI-Ressourcen | **PASS** | reale DLL via `LoadLibraryW`, nativ angeschlossenes HWND, 100/130-%-Renderings, metallisches Logo und Faceplate |
| Paketierter Audio-/MIDI-/State-Host | **PASS** | reale VST3-DLL, 16 Stereo-Busse, Main-/Stem-Routing, MIDI, Zeitposition/SONG, FLAC/WAV, DM16/DM17, unterbrochene/unzulässige Zustände, Sample-Audio |
| Unabhängiger 125A Plugin Tester **v0.3.2** | **58 PASS / 0 WARNING / 0 FAIL**, Exitcode 0, JSON `status=PASS` | `independent-plugin-tester / tester` Job im selben Run, nach `windows`; Detailberichte TXT/JSON und Provenienz im separaten Artifact |
| Studio One 8.1.2 Pro / Windows 11 ASIO / reale Bedienoberfläche | **NOT RUN** | getrennte Anwenderabnahme; kein simuliertes PASS |

**Nachvollziehbare Zuordnung der geprüften Binärdatei:**
- Source-Commit: `7da6d5a4fd42a76fd8a561b9637ee8ebdd810c13`
- Build/Test-Run: `40410436596`
- Windows-VST3-Paket: Artifact ID **`12692093334`**, `125A-Drumator-M11-Full-Kit-Stress-QA`; Archiv-Digest `sha256:41e5886c6997a8e969c639c3f76903863448e59c08345b92926f6bcc33025760`.
- **Exakter SHA256 der geprüften DLL** `Contents/x86_64-win/125A Drumator.vst3`: `8e1b8d1815d2529a35ccc6417d15416637c488933469bfba91a4f82c9f17e89a`.
- Unabhängige 125A-Tester-Evidenz: Artifact ID **`12692158314`**, `125A-Drumator-Independent-Plugin-Tester-v0.3.2`; Archiv-Digest `sha256:4ffdb8c06da1cdcab076192de75cee2c5354a31f785b5d4dd52a6f7ce6bd770e`.
- Native VSTGUI-Pixel-Evidenz: Artifact ID **`12693875894`**, `125A-Drumator-VSTGUI-Native-Renderings`.

**Release-Restbedingungen:** A07/A08 sind nicht erledigt: reales Studio One/ASIO-Langzeitverhalten, Transport-Cycle/Stop/Start, gleichzeitige Multi-Out-Ausgabe, Bedienung des Sample-Editors, tatsächliche Host-Fensterdarstellung bei 100/130 % sowie vollständiges DAW-Projekt-Speichern/Neuladen werden separat auf dem Anwenderrechner geprüft. Ein echter separater Hostprozess ist trotz kopierendem Proxy-Test als **nicht praktisch bestätigt** ausgewiesen. Keine eigenmächtige V1-FINAL-Freigabe, keine Versionsanhebung, kein Merge nach `main`.


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

**QA-Status zum Erstellen dieses Dokuments:** neuer Windows-Build **NOT RUN**; neuer 125A Plugin Tester **NOT RUN**; Studio One/ASIO **NOT RUN**. Vorherige Runs sind historische Ausgangsnachweise und gelten nicht als Abschluss der neuen Änderungen. V1-FINAL ausdrücklich nicht erteilt.

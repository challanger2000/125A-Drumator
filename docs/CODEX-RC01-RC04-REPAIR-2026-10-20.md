# Drumator – Codex-Recheck RC01–RC04: Reparaturen und Windows-QA

**Datum:** 20.10.2026 (Europe/Berlin)  
**Produktquellstand:** `d984188457882a4463b9263593d88f7bf1fffb71`  
**Branch:** `develop/v1.0.0`  
**Windows-Run:** [#39341195970](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970) — **SUCCESS** exakt für `d984188457882a4463b9263593d88f7bf1fffb71`  
**Unabhängiger Ausgangsaudit:** [Codex A01–A09 Recheck](https://github.com/challanger2000/125A-Drumator/blob/audit/codex-recheck-a01-a09-2026-10-19/docs/audits/CODEX-RECHECK-A01-A09-2026-10-19.md) auf Basis `dbd9106`.

## Durchgeführte Codeänderungen

| Befund | Konkrete Korrektur | Einordnung |
|---|---|---|
| **RC01 HOCH** — alter Velocity-Stroke kann neu geladenes Kit überschreiben | `KitEditGeneration.h` und `Controller::invalidateTransientEditsForRecall()`: erfolgreich decodierte moderne und Legacy-Recalls verwerfen alten Controller-Stroke und erhöhen die Editgeneration **vor** Mutationen. Registrierte Views löschen aktive Stroke-/Namens-Flags ohne alte Pattern-Daten zurückzuschreiben. `endVelocityStroke()` und View-Mausereignisse beachten Generationstoken. Abgelehnte Decodes lassen gültige alte Gesten unberührt. | Quellreparatur + Generationseinheitstest. Native Mouse/Host-Integration weiterhin nicht ausgeführt. |
| **RC02 MITTEL** — geerbter, nicht gesicherter SDK-Rechtsklickzoom | `Controller::createView()` registriert bewusst **keine** `allowedZoomFactors` mehr, womit der gepinnte VSTGUI-`onMouseDown` **kein** ungesichertes „UI Zoom“-Kontextmenü erstellt. Die zwei sichtbaren Zoomtasten bleiben 100/150 %. `setState` gleicht *tatsächliche* offene View-Zoomfaktoren auch bei unveränderter gespeicherter Präferenz ab und rollt pro View auf ihren vorherigen Faktor zurück, wenn eine Zoom-Änderung fehlschlägt. | SDK-Quellvertrag verifiziert und Windows kompiliert. Hostresize-Ablehnung und Mehrfach-View-Rollback native offen. Der geerbte Menü-Zoom ist absichtlich nicht mehr verfügbar. |
| **RC03 MITTEL** — ausstehendes Rename wirkt nach Kitwechsel auf neues Kit | `nameEditGeneration_` wird beim Start der Texteingabe gespeichert, beim erfolgreichen Recall invalidiert und bei `valueChanged` geprüft. Bestehendes `CTextEdit` wird ausblendbar gehalten; späte alte Fokus-/Commit-Ereignisse dürfen kein Rename auf die neue Kitgeneration ausführen. | Quellreparatur + Generationseinheitstest. Echte native Fokusreihenfolge bleibt in Studio One zu testen. |
| **RC04 NIEDRIG** — Dropziel verdeckt, aber irreführend „kein WAV/FLAC-Dateipfad“ | `DropFeedback.h` trennt `TargetBlocked`, `InvalidFilePath` und `ValidTarget`. `onDragEnter` verwendet diese Diagnose und gibt bei Overlayablehnung unverändert `DragOperation::None` zurück. | Aktiv getesteter Pure-Function-Contract; echter Hostbrowser-/Explorer-Drop weiter manuell. |

**Gezielt geändert:** `CMakeLists.txt`, `DrumatorController.cpp/.h`, `DrumatorView.cpp/.h`, `src/core/KitEditGeneration.h`, `src/core/DropFeedback.h`, `tests/RecallUIContractTests.cpp`.

**Unverändert:** DSP, Sample-Decoder, MIDI-/Pattern-Timeline, VST3-Parameter-IDs, Kit-Codec, Audio-Routing, optisches Design und alle 125A-Branding-Assets.

## Verifizierte Windows-QA

- GitHub Actions [#39341195970](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970) `completed/success`, `head_sha=d984188457882a4463b9263593d88f7bf1fffb71`.
- `ctest -C Release`: **16/16 PASS**, insbesondere `drumator_recall_ui_tests` PASS.
- Steinberg `validator.exe`: **47 passed, 0 failed**.
- Paketierter VST3-Editor-Geometrie-/DPI-Smoke: PASS.
- Paketierter VST3-Audio-Host-Smoke (einschließlich Multi-Out, MIDI/Pattern, State, WAV/FLAC, Kit-Restore): PASS innerhalb der bereits bestehenden Produktfixture.
- Artefakt: `125A-Drumator-M11-Full-Kit-Stress-QA`, ID `12228852134`; von GitHub gemeldeter Archivinhaltsdigest `sha256:b93bff4157bffc61d001cc5f3ebe5cca639bf19ee9cac736c2e4f5d23c1e114c`.
- Es wurde **kein zusätzlicher nativer Windows-Anwendungstest mit angeschlossenem VSTGUI-Fenster** ausgeführt. Neuer Coretest kontrolliert die Generations- und Drop-Klassifikationshelfer, aber noch **nicht** mit einem vollständigen geöffneten Controller-/View-/Host-Interleaving.

## Offen / keine V1-Freigabe

1. **RC01/RC03 – native Kitwechsel während gehaltenem Velocity-Drag / ausstehendem Namen:** mehrere Kits mit unterschiedlichen Werten, moderne/Legacy-Recall, fehlgeschlagener Recall, MouseUp/Cancel/Editorclose/Undo, Enter/Escape und Host Dirty exakt prüfen. Keine alte Kitgeneration darf das neue Kit verändern.
2. **RC02 – native Resize/Zoom:** 100/150-Tasten, gespeicherter Zustand bei identischem und abweichendem Wert, Hostresize-Ablehnung, DPI, erneutes Öffnen, ggf. mehrere Views. Prüfen, dass das **entfernte** geerbte Rechtsklick-Zoommenü nicht mehr angeboten wird; andere Kontextmenüs bleiben unabhängig.
3. **RC04 – Drop/Diagnose:** gültiges WAV/FLAC bei offenem Song-/Fill-Overlay, ungültiger Pfad im freien Bereich; Daten erhalten, Status korrekt, keine schleichende Änderung im Hintergrund.
4. **Codex O01/R01 und weitere bereits dokumentierte Abnahmelücken:** mögliche Hostautomation-ABA/Restore-Priorität, gepacete Echtzeit-/Deadline-Messungen, Tests T01–T04 mit stärkeren Orakeln und komplette Studio-One-Owner-Abnahme sind durch diese punktuellen Reparaturen **nicht** erledigt.
5. **Finale Bildgrafik / Skinintegration** bleibt Aufgabe des anderen Chats. Keine FULL V1, kein Release oder Gumroad.

**Bewertung:** vier gezielte Codekorrekturen erfolgreich unter Windows gebaut und gegen vorhandene CTests/Validator/Hostfixtures regressiert. **Noch keine unabhängige Codex-Nachprüfung dieses Commits und keine native Studio-One-Funktionsfreigabe.**

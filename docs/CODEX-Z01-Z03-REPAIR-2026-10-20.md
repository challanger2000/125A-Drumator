# 125A Drumator – Codex Z01–Z03: Reparaturen und Windows-Verifikation

**Datum:** 20.10.2026 · **Branch:** `develop/v1.0.0`  
**Produkt- und Testcommit:** [`62f10a4edade31217c90a32eba3d4e39eefc54ee`](https://github.com/challanger2000/125A-Drumator/commit/62f10a4edade31217c90a32eba3d4e39eefc54ee)  
**Windows Actions:** [#39347442164](https://github.com/challanger2000/125A-Drumator/actions/runs/39347442164) — `completed/success` auf exakt `62f10a4edade31217c90a32eba3d4e39eefc54ee`.  
**Auslöser:** [Codex RC01–RC04 Abschlussaudit](https://github.com/challanger2000/125A-Drumator/blob/audit/codex-recheck-rc01-rc04-2026-10-20/docs/audits/CODEX-RECHECK-RC01-RC04-2026-10-20.md).

## Änderungen und Grenzen

| Befund | Neue Implementierung | Was noch nicht bestätigt ist |
|---|---|---|
| **Z01 HOCH – teilweiser Kit-Commit bei `bad_alloc`** | `PreparedKitNames<kRows>` kopiert Customnamen und Samplelabels vor der ersten Controllerdatenänderung. Erst nach erfolgreicher Dekodierung und Stringvorbereitung werden die alte Bearbeitungsgeneration invalidiert, Pattern/Parameterstate und Namen über `noexcept`-Swap aktualisiert. Auch die allokationsanfälligen Legacy-Defaultnamen werden vorab vorbereitet. Native Editor-Bereinigung erfolgt erst **nach** der Übernahme und darf keinen bereits übernommenen Kitstate als abgelehnt melden. | Der neue Test injiziert Fehler in **32 tatsächlichen Name-Copy-Allokationen** innerhalb der verwendeten Stagingklasse; eine separate vollständige Controller-DLL-/Host-/Allocator-Fault-Injection liegt noch nicht vor. Die Testfixture prüft keine Prozessorsynchronisation. |
| **Z02 MITTEL – partieller Zoom nach fehlgeschlagenem Resize-Rollback** | `applyZoomFromState` nutzt **nur einen** geprüften Editor-/Frame-Zoom; die danach bisher mögliche zweite `adjustEditorHeight`-Anfrage und deren fragiler Rückrollback entfallen. `Controller::setState` prüft die tatsächlichen registrierten Views. Würden mehrere Views unabhängige native Zoomwechsel benötigen, wird **vor jeder Änderung `kResultFalse` zurückgegeben**, statt einen partiellen Gesamtwechsel zu versuchen. | **Bewusst eingeschränkte Mehrfach-Editor-Funktion:** simultaner Zoom-Recall über zwei zu ändernde Views wird derzeit abgelehnt. Echte kontrollierte `IPlugFrame`-Resizeablehnungen sowie Studio-One-100/150%-Praxis sind noch nicht durchgeführt. |
| **Z03 MITTEL – ausgeblendetes natives Win32-Name-HWND lebt weiter** | Stale Edit-/Stroke-Tokens werden **vor** Datenübernahme und Plattformereignissen ungültig. Nach erfolgreichem Commit schließt `finishTransientEditsAfterRecall` über `CFrame::setFocusView(nullptr)` bzw. `CTextEdit::looseFocus()` die native Windows-Textkontrolle. Auch `removed()` beendet einen noch bestehenden nativen Editor, ohne einen alten Rename ins neue Kit zu übernehmen. | Die API wurde an gepinntem VSTGUI geprüft und im Windows-VST3 kompiliert; ein tatsächlich angehängter HWND-/Fokustest mit Enter/Escape, mehreren Views und Host-Recall bleibt ausstehend. |

`RC04` (Dropdiagnose) bleibt unverändert; Codex hatte seine eingegrenzte Quelllogik bestätigt. DSP, Multi-Out, PCM/FLAC, Patternquantisierung, MIDI-/Hostereignisse, VST3-Parameter-IDs und grafische Skinressourcen wurden nicht verändert.

## Windows-Nachweise

- **17/17 CTests PASS** (Release), einschließlich neuem `drumator_recall_transaction_tests` und vorhandenem `drumator_recall_ui_tests`.
- Der Transaktionstest prüft die 32 aufeinanderfolgenden `bad_alloc`-Stellen während der Namensvorbereitung, alte Live-Strings und Generationstoken sowie nicht-allokierendes Commit unter aktivem Allocator-Fail.
- **Steinberg Validator: 47 passed, 0 failed.**
- Paketierter VST3-Editor-Geometrie-Smoke **PASS**.
- Paketierter VST3-Audio-Host-Smoke **PASS**, einschließlich bestehender WAV/FLAC-/Multi-Out-/State-/MIDI-Szenarien.
- Actions-Artifakt `125A-Drumator-M11-Full-Kit-Stress-QA`, ID `12231034358`, von GitHub gemeldeter Digest `sha256:f79be0f06b64fb4ad340a1ef139818c898325f710f8c2df75a5f74eab2d81424`.
- Keine Studio-One-DAW wurde durch ChatGPT gestartet; der Smoke ist kein natives Interaktions-/Lifecycle-PASS.

## Noch offene Freigabegates

1. Unabhängige Codex-Nachprüfung **des genauen neuen Codes `62f10a4edade31217c90a32eba3d4e39eefc54ee`**, inklusive Z01–Z03, Fehler- und Positivpfade, und aller unbeabsichtigten Nebenwirkungen.
2. **Native Studio-One-/Win32-Prüfung** bei geöffnetem Texteditor: KitX→KitY, Fokus/EDIT-HWND weg, alter Rename darf nicht übertragen werden; gültiger Namensedit muss wieder funktionieren.
3. **Hostresize-/Mehrfach-Editor-Prüfung**: reale 100/150%-Zoombuttons, gespeicherte Präferenz, Ablehnung im echten `IPlugFrame`, erneut öffnen, DPI; beabsichtigte Mehrfach-View-Ablehnung verifizieren.
4. **Z01 vollständiger DLL-/Host-Fault-Injection-Test** mit modernem/Legacy-Kit und separatem Prozessorzustand. Helper-Tests sind kein Ersatz.
5. Die Codex-Testlücken **T01–T04** und das nicht deterministisch reproduzierte Hostautomation-/Restore-Risiko **O01** sind von diesen Reparaturen unverändert.
6. Der andere Chat gestaltet die finale VSTGUI-Skin; keine Änderung an seinem Artwork/Asset-Handoff. Keine V1-/Gumroad-/Releasefreigabe.

**Urteil:** Code für Z01–Z03 repariert und unter Windows erfolgreich regressiert; eine unabhängige technische Endprüfung und native Studio-One-Abnahme stehen ausdrücklich noch aus. **NOT FINAL.**

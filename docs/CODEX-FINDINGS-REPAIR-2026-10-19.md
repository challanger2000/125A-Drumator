# Codex-Fehler A01–A09 — Korrektur- und QA-Nachweis

**Datum:** 19.10.2026 · **Repository:** `challanger2000/125A-Drumator` · **Branch:** `develop/v1.0.0`  
**Ursprünglicher unabhängiger Codex-Audit:** [unveränderter Bericht](https://github.com/challanger2000/125A-Drumator/blob/audit/codex-drumator-2026-10-19/docs/audits/CODEX-INDEPENDENT-AUDIT-2026-10-19.md), geprüft auf `ee712dd094e28fdc9bd6da9917f249aa7ef7b185`.  
**Quellcodekorrektur 1:** [`e3218ff`](https://github.com/challanger2000/125A-Drumator/commit/e3218ffaa27669106233cf2c8ceb98d10da4b75f) — Windows [#39334639581](https://github.com/challanger2000/125A-Drumator/actions/runs/39334639581) SUCCESS.  
**Quellcodekorrektur 2 / derzeit geprüfter Funktionsstand:** [`dbd9106f`](https://github.com/challanger2000/125A-Drumator/commit/dbd9106f7af48b23ad4874a1a4c74395baa14173) — Windows [#39335925418](https://github.com/challanger2000/125A-Drumator/actions/runs/39335925418) **SUCCESS; 15/15 CTests, 47/47 Steinberg Validator, paketierter VST3-Editor- und Audio-Host-Smoke PASS**.

## Einzelkorrekturen

| Codex | Quellkorrektur | Tests / verbleibende Grenze |
|---|---|---|
| **A01 HOCH** MIDI-Patternwechsel ohne GUI-/Hostrückmeldung | `DrumatorProcessor.cpp`: MIDI-Auswahl erzeugt zeitlich korrekte `ProcessData::outputParameterChanges` für `kPatternParam`. Keine Audio-Thread-UI-Aufrufe, gleichzeitige Hostpunkte bleiben autoritativ. | Paketierter VST3-Audiohost prüft Offset 64, gemeldeten Wert B, simulierte Controller-Host-Rückgabe. Tatsächliche Hostweiterleitung zur offenen Studio-One-GUI weiter **MANUAL**. |
| **A02 HOCH** 257 Punkte löschen alle Änderungen | `core/PatternEventTimeline.h` + Processor: beim `setupProcessing` vorab reservierte Timeline **pro Sample-Offset**, kein 256-Ereignis-Limit, keine Audio-Thread-Allokation; bei gleichem Offset gewinnt Hostautomation. | Neue `PatternEventTimelineTests.cpp`: 257 Punkte, 2048 Werte an einem Offset, Host/MIDI-Kollision, Ausgrenzung ungültiger Werte; paketierter VST3-Host **250 Hostpunkte + 8 MIDI-Ereignisse** bei unterschiedlichen Sample-Offsets mit hörbarem B. |
| **A03 MITTEL** Strg+Master setzt 0 | `DrumatorView.cpp`: Delta = **100 − aktueller Masterwert**. | Quellcode Default verifiziert; wirklicher Strg-Mausklick und Host-Dirty in Studio One weiter **MANUAL**. |
| **A04 MITTEL** Namenseditor bleibt nach Escape/Enter stehen | `DrumatorView.h/.cpp`: mit dem gepinnten VSTGUI abgeglichener `ITextEditListener`, Fokusverlust blendet den Textedit-Overlay aus und beendet den Bearbeitungszustand; Listener bei View-Remove entfernt. | Windows-VST3 kompiliert; nativer Enter/Escape/Unicode/Fokusfall in Studio One weiter **MANUAL**. |
| **A05 MITTEL** Drop durch Song/Fill-Overlay | `core/UiOverlayHitZones.h` und `DrumatorView.cpp`: gleiches sichtbarkeitsabhängiges Ziel für DragEnter/Move/Drop und vorhandenen Drop-Adapter; verdeckte Felder werden abgelehnt. | Neuer `UiOverlayHitZonesTests.cpp` mit gültigen und verdeckten Zielen; echter Host-Browser-Drop bei Overlay/Zoom weiter **MANUAL**. |
| **A06 MITTEL** abgeschnittener Compact-Fallback | `CompactRowViewport::fittingRows()` und View: Page-Größe aus tatsächlicher bestehender Clienthöhe statt fix 4; bei unzureichender Höhe wird offene Velocity-Lane geschlossen, damit Footer erreichbar bleibt. | `VisibleRowsTests.cpp`: Fälle 300/357/792 Pixel und unzureichende Höhe. Echte Host-`resizeView`-Ablehnung / 100/150 % **MANUAL**. |
| **A07 MITTEL** Velocity-Drag Undo nur letzte Bewegung | `Controller`: `beginVelocityStroke` hält vollständigen Vorzustand, Zwischenbewegungen überschreiben nicht mehr den Undo-Snapshot; `endVelocityStroke` commit/cancel; View nutzt festen Patternindex während der Geste. | VST3 kompiliert; echte Mausgesten, Undo für mehrere Steps und Cancel/Hostreject **MANUAL**. |
| **A08 MITTEL** Zoom-Recall bei offener View | `Controller::setState` wendet die geladene Zoompräferenz über `DrumatorView::applyZoomFromState` auf bestehende registrierte Views an; Rückfall bei Hostablehnung. | VST3 kompiliert; angeschlossenes Studio-One-Editor-Recall, Ablehnung und Mehrfachviews **MANUAL**. |
| **A09 NIEDRIG** unsichtbare Buttonlücken aktiv | `DrumatorView.cpp`: Song-Pattern- und Längenraster erlauben nur tatsächlich gezeichnete Tasten; leere Footer-Lücke meldet keinen erfolgreichen Velocity-Reset mehr. | Quellcodeprüfung und Windows-Regression; native UI-Hitbox-Grenzen **MANUAL**. |

## Unveränderte Architektur

- Keine neuen Audio-DSP-Algorithmen, Samples, Routing-Regeln, VST3-Parameter-IDs oder Kit-Formatänderungen.
- Die rein **technische** VSTGUI ist weiterhin die Grundlage für die eigenständige finale grafische Gestaltung des **anderen Chats**; keine unbeauftragten neuen Skin-Assets.
- Studio One, realer Editor-Attach, Host-Automationsrückkanal, Preview/Drag, Undo-/Namens-/Zoom-Praxis, Multi-Out und paced-Realtime **nicht selbst nativ ausgeführt**; Windows-CI/Validator allein sind keine Besitzerfreigabe.
- Getrennter möglicher ABA-Pattern-Restore-Konkurrenzfall **R01** aus dem Codex-Audit bleibt als noch zu reproduzierendes Risiko, nicht als bestätigter A01–A09-Fall.
- **Keine FINAL V1, kein Release/Tag/Gumroad.**

## Nächste unabhängige Codex-Prüfung

Den exakten Quellstand `dbd9106f7af48b23ad4874a1a4c74395baa14173` gegen A01–A09 erneut prüfen und fehlende interaktive/nichtautomatisierte Gates gesondert protokollieren. Kein anderes GitHub-Actions-Budget ohne begründete Notwendigkeit verbrauchen.

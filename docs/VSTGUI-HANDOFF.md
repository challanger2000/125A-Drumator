# 125A Drumator – VSTGUI-Handoff (neue technische Vorlage)

**Aktualisiert: 20.10.2026. Status: VSTGUI-Entwicklungsstand, kein Skin- oder Release-PASS.**

Die gültige GUI-Grundlage wird aktiv im Branch `design/vstgui-foundation-2026-10-20` entwickelt. Für visuelle Arbeit sind **erst nach Windows- und Studio-One-Gate** die genauen Layoutdaten zu verwenden.

## Autoritative Dokumente
- [Layout/UX und Prüfplan](VSTGUI-FOUNDATION-2026-10-20.md).
- [Grafik-/Widget-Koordinaten](../resource/VSTGUI_ASSET_SLOTS.json).
- `src/core/UiRowGeometry.h`: Position von Instrumentzeile, Step- und Velocity-Zeile, Y-Hitbereichen.
- `src/vst3/DrumatorView.cpp/.h`: dynamischer VSTGUI-Renderer, OnMouseDown/Move/Up, Sample-Files, Song/Fill, DrawOrder, DPI.
- `src/vst3/DrumatorController.cpp/.h`: tatsächliche Aktionen, Projekt-Recall und Zoombindung.

## Erwarteter Arbeitsablauf
1. Instrument/Datei links laden oder per Drag-and-drop in die Zeile ziehen.
2. A–H-Pattern und 1-/2-Takt wählen; 32 Steps rechts anklicken.
3. Bei Bedarf VELOCITY global einschalten. Je **angezeigter Zeile** erscheinen 32 individuell bedienbare Velocity-Balken exakt unter den 32 Step-Tasten. Die vorherige isolierte Velocity-Leiste im Footer wird nicht weiterverwendet.
4. Bis 16 Instrumente, mit eingeblendeter Velocity höchstens acht gleichzeitig; Rest über Seitenwechsel.
5. Song-Board per Song-Ketten-Taste öffnen und Pattern/Längen direkt wählen.
6. 100/120/130/150 Prozent Zoom; die gespeicherten 100/150-States (DZM1) bleiben lesbar, Zwischenwerte verwenden DZM2.

## Für spätere Bildintegration
- Grafik nie von einem alten Screenshot pixelweise abpausen; tatsächliche C++-/Manifestkoordinaten und die vom Eigentümer abgenommene Mockup-Stimmung sind die Referenz.
- Alle Sample-Namen, Controllerwerte, Chain-Zustände, Steps und Velocity-Balken bleiben **dynamisch**, niemals als Bildtext.
- Designerfamilien: Kopfleiste, Instrumentfläche, Sample-Navigation, Vol/Pan/Pitch-Regler, Step-Taste, Velocity-Balken, Song-Slots, Fill-Menü, Status-/Footerflächen. Zunächst technische VSTGUI, erst dann freigegebene Knob-Designer-Filmstrips und 125A Branding Master.
- GitHub-Logoquelle `challanger2000/125A-Branding/125A_Logo_Master_FINAL.svg` unverändert proportioniert.

## Sperren
Die Placeholder-Oberfläche enthält noch sehr kompakte Vol/Pan/Pitch-Felder und wird **nicht als finaler Look** freigegeben. Ohne echte native Visual-QA ist auch ein 18/18-CTest-PASS keine Bestätigung von Textbreite, Windows-DPI, Font-Collision oder Studio-One-Usability. Vor Artwork-Lock zuerst 4/8/16 Zeilen, Zoom, Song, Fill, Sampleinput, Velocity, Projekt-Recall und tatsächliche Klickflächen unter Windows messen.

Stand dieses Dokuments ist die neue Arbeitsgrundlage; der historische Handoff-Commit `6a8384b` ist nicht mehr als gültige GUI-Pixelvorlage zu verwenden.

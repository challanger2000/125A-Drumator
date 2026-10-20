# 125A Drumator – Technischer VSTGUI-Handoff

**Stand: 20.10.2026 · Windows-validierte technische GUI-Vorlage · kein FINAL-V1-/Studio-One-PASS und kein fertiger Bitmap-Skin.**

## Nachgewiesene Basis
- Programm-/GUI-Quellstand: [`eaa035bb43cc9fa20682f711b8631a75da0ece4d`](https://github.com/challanger2000/125A-Drumator/commit/eaa035bb43cc9fa20682f711b8631a75da0ece4d).
- [Windows-Workflow #39418839905](https://github.com/challanger2000/125A-Drumator/actions/runs/39418839905): Build SUCCESS, 21/21 CTests, Steinberg-Validator 47/47, gepackte VST3 mit Editor-/Audio-Hostprüfung.
- [Reale VSTGUI-Renderings](https://github.com/challanger2000/125A-Drumator/actions/runs/39418839905/artifacts/12262412156): 9 Windows-Offscreen-Aufnahmen. Enthalten: vier Zoomstufen, 8er-Velocity, 16-Zeilen-Ansicht, zwei Seiten mit 16 Instrumenten und Velocity sowie Song-Tafel. 16er-Deaktivierungsbeschriftung ist lesbar; die ausgewählte Klangzeile folgt dem Seitenwechsel.
- [Aktuelle installierbare Windows-VST3](https://github.com/challanger2000/125A-Drumator/actions/runs/39418839905/artifacts/12262387312): nur Entwickler-/Testpaket, noch keine V1-Freigabe.

## Verbindliche Geometrie
- **Technisch maßgeblich sind die gemeinsamen C++-Koordinaten** und der Asset-Slot-Vertrag, nicht der blaue Mockup und kein altes Standbild.
- `src/core/UiRowGeometry.h`: Step-/Velocity-Geometrie, Rendering und Hitbereiche gemeinsam: 1280 logische Pixel Breite, y=108 Instrumentenstart, 29 px Hauptzeile, 27 px inline Velocity, Steps ab x=622 in 32 Zellen zu je 19 px.
- `src/core/UiRowControls.h`: FILL, P, Sample-Text (164 px), Sample vor/zurück (je 24 px), Ausgang, VOL, PAN, PITCH, MUTE/SOLO mit exakt den gezeichneten Klickflächen. Sample-Text wird per VSTGUI-Glyphbreite passend gekürzt, Dateiendung bleibt nach Möglichkeit sichtbar.
- `src/core/UiDetailGeometry.h`: 5 ausgewählte Klangparameter – Volume, Pan, Pitch, Random Velocity, Choke. Ein einheitlicher 64-Pixel-Detailbereich; sichtbare Werte, lineare Trackpositionen, +/- und Strg-Klick auf den tatsächlichen Standardwert.
- `src/vst3/DrumatorView.cpp/.h`: Zeichnen und Click/Drag, Fill-/Song-Overlays, Scroll-/Zoom-/Sample-Bedienung und textgebundene Anzeigen. Jeder Position- oder Schriftgrößenwechsel muss mit den zugehörigen Klick-/Dropbereichen und Tests verbunden sein.
- `src/vst3/DrumatorController.cpp/.h`: tatsächlicher Host-/Prozessor-Aufruf und Session-/Projektzustand. Audioengine, Audioformat, MIDI- und Parameter-IDs sind nicht für den Skin zu ändern.
- `resource/VSTGUI_ASSET_SLOTS.json`: dokumentierter Austauschvertrag mit Größen, Zuständen und Grafikgruppen; vor Export mit **aktuellem HEAD** vergleichen.

## Anwenderablauf
Sample auf eine Instrumentenzeile ziehen oder Dateifeld anklicken. Ein Step-Klick aktiviert/deaktiviert, Shift-Klick verändert Velocity. A–H oben wählen, 1/2 Takte per Shift-Klick. Beim globalen VELOCITY-Schalter erhält **jede sichtbare Instrumentenzeile** ihre 32 vertikalen Balken direkt unter den jeweiligen Steps. Ab neun Instrumenten mit Velocity erscheinen 8 Zeilen/Seite; die Seitenauswahl wechselt die Detailanzeige korrekt mit. Klick/Ziehen in den Balken verändert Werte 1–127, Strg setzt 0, jeweils ein Undo-Stroke. Die Klangleiste steuert das ausgewählte Instrument; Song besitzt eine eigene 64-Slot-Ansicht.

Zoom: 100 / 120 / 130 / 150 %, unter Berücksichtigung von Host- und Windows-DPI. Alte gespeicherte DZM1-Zustände bleiben kompatibel, 120/130 nutzen DZM2. Windows-Render-Auflösungen für 4 Instrumente: 1280×398 / 1536×477 / 1664×517 / 1920×597.

## Grafik-Integration
Der Nutzer-Mockup dient der **gestalterischen Orientierung** (blau/blaugraue Studiomaschine, FL-artiger Step-Workflow, E-mu-/DMX-/808-/909-Anmutung), **nicht** als Pixelquelle. Die offizielle 125A-Master-Geometrie kommt aus `125A-Branding/125A_Logo_Master_FINAL.svg`, gerenderte Drehregler-/Schalterfamilien nach aktueller Freigabe aus `125A-Knob-Designer`. Dynamische Beschriftungen, Step-Lauflicht, Parameterwerte und Velocity müssen echte Bedienelemente bleiben und dürfen nicht in statischen Hintergrundbildern verschwinden.

## Offene Abnahme
Der technische Editor hat automatische Windows-Gates bestanden und die echten Renderings wurden auf Position und sichtbare Überlagerungen geprüft. **Noch nicht nachgewiesen**: Studio-One-Praxistest mit realen WAV/FLAC-Dateien und Browser-D&D, Mausbedienung sämtlicher Funktionen im Host, Zoom/Resize/Recall im tatsächlichen Produktionsfenster und die endgültige Bitmap-/Knob-Designer-Grafikintegration. Erst danach die finale GUI/Pluginversion freigeben.

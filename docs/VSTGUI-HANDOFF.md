# 125A Drumator — VSTGUI UX V2 Handoff

**Stand: 20.10.2026 · V2 technische Arbeitsoberfläche · Windows-Freigabe und Studio-One-Praxistest getrennt.**

**Verbindlicher Änderungsnachtrag RV01/SO01, 20.10.:** Der alte Runtime-Regler `RANDOM VEL.` ist für die neue GUI **verworfen**. Das GUI-Ziel ist ein **zeilenbezogener Taster** `RANDOM VEL`, der `Controller::randomizeRowVelocity(row)` einmalig aufruft und dabei gespeicherte Velocity-Balken mit Undo ändert. Die alte Playback-RandomVelocity-Logik existiert im aktuellen Runtime-Code noch und ist ausdrücklich ein **offenes Migrations-/Freigabegate**. Die Busse `DRUM 02–16` sind nach SO01 als `kMain` gemeldet (Windows-QA geprüft), die tatsächliche Studio-One-Aktivierung bleibt Nutzer-Gate. **Bei Widerspruch zum historischen Text unten gilt dieser Nachtrag und `resource/VSTGUI_ASSET_SLOTS.json` mit seinen deklarierten Alt-/Ziel-Slots.**


Die bisherige V1-GUI mit 29px Instrumentenzeilen, verteilten Plus-/Minus-Knöpfen, vier einzelnen Zoom-Tasten und zusätzlichem riesigem unteren Präzisions-/Velocitybereich ist **verworfen**. Ihre Koordinaten sind weder Designreferenz noch gültige Asset-Slots.

## Ziel und Bedienhierarchie
**Sample laden oder hineinziehen → Steps setzen → abspielen.** Regler und ihre Werte gehören direkt zur jeweiligen Instrumentenzeile. Alles was selten gebraucht wird, öffnet man am Ort der betroffenen Funktion. Sichtbare direkte Zeilenauswahl ohne Nebeneffekt. Keine doppelte Bedienfläche.

1. **Header:** Pattern A–H als Tasten; aktive Länge als zwei direkt anwählbare Taster `1 TAKT` / `2 TAKTE`; ein `PATTERN …` Aktionsmenü für Copy/Paste/Clear/Undo/Velocity-Patternaktionen ohne Längen-Duplikat. Rotary für Swing (0–50%), Humanize (0–100%) und Master (0–200%). Song ON/OFF, Song EDIT und **ein** Zoommenü (100/120/130/150%).
2. **Instrument:** Genau vier gut lesbare Zeilen gleichzeitig. Bis 16 Instrumente bleiben im Kit erhalten, Seitenwechsel über Pfeile am rechten Fuß. Jede Zeile hat: Auswahlnummer; Samplefeld zum Laden, sichtbares `…`-Menü Laden/Umbenennen/Entfernen, Ordnerpfeile, PLAY, Fill, Mute, Solo, Auswahlmenü MAIN/OUT sowie Choke 0–4. Die derzeitige VSTGUI hat noch vier Rotaries einschließlich **veraltetem** Random-Velocity-Regler. Die **endgültige** GUI erhält nur **drei Rotaries** (VOL, PAN, PITCH) und anstelle des vierten Rotaries den einmaligen zeilenbezogenen **RANDOM-VEL-Button**. Die Button-Hitbox muss anhand des tatsächlichen neuen Layouts definiert und getestet werden.
3. **Step-Raster:** Immer 32 Kacheln ab x622 mit 19px pro Step. Eindeutige 4/16er-Gliederung, 1T/2T ohne Datenverlust. Rechts pro Zeile der ausdrücklich als VELO beschriftete Pfeil.
4. **Velocity:** Jede Instrumentenzeile einzeln per Pfeil um 30px aufklappbar; beliebig mehrere zur gleichen Zeit. Genau 32 Balken unter den 32 Steps dieser Zeile, editierbar durch Klick/Ziehen. Kein separater globaler Footer-Toggle, kein zweiter Editor unten.
5. **Footer:** nur +/− Instrumentzahl (letztes nur leer entfernen), sichtbarer Seitenbereich und Ereignis-/Fehlerstatus.
6. **Song:** eigener kompakter Editor mit maximal 64 Sequenzplätzen, **16 pro Seite** mit Vor/Zurück, direktem Pattern-/Längenwechsel und 370 logischen Pixeln Höhe (sofern die DAW Resize annimmt). Kontextmenüs dürfen nicht hinter der Songtafel verschwinden.

## Maßgeblicher Code und UI-Werte
- `src/core/UiCompactLayout.h`: **verbindliche Geometrie/Hitmap** für Seiten, 94px-Reihen, individuell 30px zusätzliche Velocity, Rotary, Step-Zellen. Alt-Dateien `UiRowGeometry.h`, `UiRowControls.h`, `UiDetailGeometry.h` werden nicht mehr als aktuelle Layoutreferenz verwendet (alte Tests bleiben als Regressionen erhalten).
- `src/vst3/DrumatorView.cpp/.h`: dynamisches Rendering, Overlay-Priorität, Maus- und Drop-Zonen, Vorgabe „Row header = nur auswählen“.
- `src/vst3/DrumatorController.cpp/.h`: echte hostbestätigte Parameteränderungen; direkte Output-/Choke-Sets statt 16-maligem Durchschalten.
- `resource/VSTGUI_ASSET_SLOTS.json`: aktuelle **Ist-Geometrie** des Prototyps und explizite **Zieländerungen**; die markierte `legacyRandomVelocityKnob`-Zone ist **kein freigegebener Ziel-Button**. Bei neuer GUI mit neuer Geometrie JSON und nativen Klicktests gemeinsam aktualisieren. Niemals aus einem alten Screenshot ablesen.
- `docs/DRUMATOR-UX-V2-CONCEPT-2026-10-20.md` und `docs/UX-AUDIT-2026-10-20.md`: vollständige Funktionsentscheidung + mehrfache kritische Prüfung.
- VSTGUI-Zoom standardisierte Optionen 100/120/130/150; DZM1/DZM2 Projektkompatibilität. 140% ist nicht zur Anzeige freigegeben, da Code/Projektzustand die Stufe nicht unterstützt. Host-DPI kann Skalierung zusätzlich beeinflussen.
- Maus-Regler: vertikal ziehen ohne Mausrad. Ctrl+Click oder Doppelklick auf den **tatsächlichen** Standardwert. Kit-Recall invalidiert offene Gesten. Für die spätere Bitmap-Grafik darf die Bedienung nicht umprogrammiert werden.

## Echte Statusbelege / Grenzen
- Vorgänger UX V2 Basis bei Commit `8fce9025`: Windows-Lauf [#39425815341](https://github.com/challanger2000/125A-Drumator/actions/runs/39425815341) **22/22 CTests, 47/47 Steinberg Validator, Audio-/Editor-Hostsimulator PASS**; VSTGUI-Native-BMPs vorhanden.
- Die danach vorgenommenen Präzisierungen (sichtbare Sample-Aktionen, längere Outputbeschriftung, VELO-Label, Kopfzeilenabstand, vereinheitlichte direkte Pattern-Längenauswahl, aktualisierte Asset-Slots) sind im [aktuellen gemeinsamen Windows-Lauf #39428171691](https://github.com/challanger2000/125A-Drumator/actions/runs/39428171691) zu prüfen. Bis Abschluss **nicht PASS für den Folge-Commit behaupten**.
- Unabhängige native Studio-One-Prüfung, echte Mausbedienung/D&D aus dem Browser, 130% FHD und Fensterhöhe bei 150% sowie finaler Vintage-Skin (125A-Branding Master und freigegebene 125A-Knob-Designer-Filmstrips) bleiben externe Gates.

## Skin-Integration
Bitmap-Austausch nur anhand der tatsächlichen V2-Zonen bei gleichem Quellstand. Kein funktionsloser GUI-Knopf. Werte, Dateinamen, Steps, LEDs und Velocity bleiben dynamisch. Wechsel an Koordinaten erfordert automatisch angepasste Click- und Drag-Tests; die Grafik darf die Oberfläche niemals erneut komplizierter machen.

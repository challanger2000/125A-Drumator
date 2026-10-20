# 125A Drumator – Funktionale VSTGUI-Vorlage
Datum: 20.10.2026. Branch: `design/vstgui-foundation-2026-10-20`.
Status: **Entwicklung; Windows-Build, visuelle QA und DAW-Abnahme noch NICHT nachgewiesen.**

## Produktprinzip / Anwendersicht
Zuerst Sample laden, danach Step aktivieren und sofort hören. Pattern A–H, Song-Kette und Master sind oben. Sample und Instrumentinformationen sind links, die 32 Steps rechts. Die Bedienung darf keine unbeschrifteten verdeckten Funktionen voraussetzen. Der blaue Mockup aus dem Chat ist ein atmosphärisches **Referenzbild, kein geometrischer Bauplan**.

## Technische Geometrie
Einzig verbindliche Quelle für Zeilen: `src/core/UiRowGeometry.h`. Alle Koordinaten in logischen VSTGUI-Pixeln.
- Fensterbreite 1280, Kopfbereich y0–107, erster Step y108.
- Normale Instrumentenzeile 29 px hoch. Jede Zeile enthält Sampleauswahl, Laden/Vorhören, Fill, Vol/Pan/Pitch, Mute/Solo/Choke, Output, Random sowie 32 Steps à 19 px ab x622.
- VELOCITY ist ein **globaler Ein-/Aus-Schalter**. Bei Einblendung wird **jede sichtbare Instrumentenzeile** um 27 px nach unten erweitert. Ein Velocity-Balken ist genau seinem Step zugeordnet. Das ist nicht mehr die getrennte Balkenansicht am Fensterfuß.
- Bis 8 Instrumente gleichzeitig bei eingeblendeter Velocity; danach Paging mit den vorhandenen Pfeilen. Im normalen Modus weiterhin 4–16 Zeilen, sofern der Host deren Fensterhöhe erlaubt.
- Die sichtbare Fensterhöhe hängt von der angezeigten Zeilenzahl ab, nicht blind von allen 16 gespeicherten Instrumenten. Ablehnung von Host-Resizes führt zum bisherigen Kompakt-Fallback; nicht vorsätzlich über den Hostbereich malen.
- Footer: `108 + displayedRows * (velocityEnabled ? 56 : 29) + 5`. Nach 29px Aktionsleiste folgen 30px Editorsteuerelemente; Status unterhalb.
- Zweiter Takt bleibt bei 1-Takt-Patterns deaktiviert dargestellt, aber Steps 17–32 werden NICHT gelöscht.
- Sichtebene und Klick-/Dropflächen verwenden dieselbe Y-Geometrie. Samples dürfen nicht versehentlich in den Velocity-Balkenbereich gedroppt werden.

## Werte und Bedienung
- `VOL 100%`, `MASTER 100%`: 100 ist nomineller Basiswert, Controllergrenzen bleiben unverändert (bis 200). `PAN C` bzw. `L N` / `R N`: Mitte, links, rechts, numerisch. `P +0st`: Halbtöne. Swing/Humanize/Random ausdrücklich `%`. Step-Velocity 0–127, Null löscht den Step.
- Steps: Klick schaltet an/aus, Shift-Klick variiert Velocity. Unter den Steps: Klick/Ziehen verändert vertikale Balken (ein Stroke = ein Undo); Strg erlaubt 0. Die vorhandene Footer-Numerik ±1/±10 bleibt für Präzision.
- Leerer Sampleplatz: Dateidialog mit Klick, Vorhören P, Pfeile blättern den Ordner, Shift auf Name = umbenennen, Alt+Klick = Sample entfernen, Steps bleiben erhalten.
- Pattern A–H: Klick Auswahl, Shift-Klick 1/2 Takte. Song und Pattern sind getrennte Modi; Song-Hitflächen bleiben geschützt. Dynamischer Status darf keine Tasten überdecken.
- Zoom: 100/120/130/150 %, getrennt vom Host-DPI-Skalierungsfaktor. 100/150-Zustände als DZM1, 120/130 als DZM2. Bestehende Projekte und Host-Ablehnungen werden unverändert respektiert.

## Designphase, nicht vorwegnehmen
Das Enddesign orientiert sich lose an klassischer blauer E-mu-/DMX-Studiomaschinenoptik, mit eindeutig modernen 32-Step-/Velocitygruppen. **Keine originalgetreue Hardwarekopie.** Der offizielle 125A-Master aus `125A-Branding` und freigegebene Knob-Designer-Sets sind vor finaler Art-Integration separat zu validieren. Die verbleibenden kleinen numerischen Volume/Pan/Pitch-Felder sind **technische Platzhalter** und brauchen vor dem visuellen Design noch eine ergonomische Entscheidung für konkrete, bedienbare Reglerflächen. Diese Fassung ist noch keine vom Eigentümer abgenommene Grafikvorlage.

## Test-/Freigabegates
1. Windows MSVC Build mit echten VST3/VSTGUI, alle CTests inklusive `drumator_row_geometry_tests`, Validator und Hostsmokes.
2. Windows-native Screenshots / Textbreitenprüfung bei 100/120/130/150 % und DPI 100/150/200 % für 4,8,16 Zeilen; Fill, Song, Sampling, Rename, Velocity und Paging. Keine Kollision, Trunkierung oder unsichtbaren Controls.
3. Host-Rückweisung der Fensterhöhe, rückrollsicheres View-Zoom, Mehrfach-View, Projektwiederherstellung DZM1/DZM2 und Drag/Undo.
4. Owner-Test in Studio One: Sample-D&D, korrekte Werte, alle Buttons, Start ab Takt 2, Projekt speichern und wieder öffnen.
5. Erst nach allen Gates die Asset Slots einfrieren und Grafiken einsetzen.

**Keine V1-/Skin-Freigabe durch Code-Review oder grünen Validator allein.**

## Umsetzung: Präzisionsregler und Sample-Zeilen
- Die instrumentbezogenen Bedienelemente haben exakt definierte und gemeinsame Zeichen-/Hitboxbereiche in `src/core/UiRowControls.h`, darunter 164 Pixel Sample-Textfeld, 24-Pixel Sample-Pfeile und klar abgegrenzte MAIN/AUX-Auswahl. Die Instrumentzeile zeigt weiter schnelle Mix-Werte mit +/-; Choke und Random Velocity liegen vollständig im größeren Klangbereich.
- Nach dem Raster-Footer folgt ein 64-Pixel-Klangbereich ab `gridBottom+70` mit Lautstärke (0..200 %, Standard 100), Panorama (-100..+100, Mitte 0), Pitch (-24..+24 Halbtöne, Standard 0), Random Velocity (0..100 %, Standard 0) und Choke (0..4, Standard 0). Die horizontale Präzisionsbedienung erlaubt +/- Tasten, absolute Reglerposition und Ziehen. Ctrl+Klick stellt den korrekten Parameterstandard her.
- `src/core/UiDetailGeometry.h` bestimmt Hitboxen und lineare Zahlenzuordnung. `DrumatorView::changeDetailValue` benutzt ausschließlich die echten Controller-Aufrufe; keine DSP-/Projektformatänderungen und keine UI-Attrappen.
- Die zuletzt angeklickte Instrumentzeile ist blau umrahmt; deren Werte erscheinen im Klangbereich. Andere Instrumente bleiben unabhängig. Ein Kit-Recall beendet unfertige Slider-Gesten.
- Gesamt-Footer jetzt 174 logische Pixel; Statuszeile bei `gridBottom+144`. Fenster bleibt bei bis zu 16 normalen Zeilen unter 850 logischen Pixeln; bei geöffnetem Velocity bis 8 angezeigte Zeilen pro Seite. Größere Userzoom-/Windows-DPI-Kombinationen bleiben hostabhängig.
- Automatische QA: `drumator_row_control_tests`, `drumator_detail_geometry_tests`, `drumator_row_geometry_tests` plus native Windows-Integration für direkte Parameter-Endwerte, Ctrl-Reset und Instrumentwechsel.
- Nach Windows-QA weiterhin offen: unabhängige visuelle Text-/Fontmetrikprüfung bei allen Zoomstufen, finale Knob-Designer-Grafiken sowie Praxistest in Studio One.

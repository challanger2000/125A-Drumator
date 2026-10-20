# Drumator UX V2 – verbindliches Bedienkonzept (Entwurf zur Implementierung)
Datum: 20.10.2026 · Quellbasis: develop/v1.0.0 @ 1d1203b · Status: UX-Redesign, Windows-/DAW-Gates zunächst OFFEN.

## Nicht verhandelbares Produktziel
Ein Beat entsteht durch drei Schritte: Sample in Zeile laden/ziehen → Steps setzen → Wiedergabe. Keine Suche nach Elementen; jede Aktion an ihrem Gegenstand, keine doppelten Controls. Bestehende Audioengine, DSP, MIDI, Pattern-Daten, SongChain, Controller-Aktionen, Undo, Projektformat, Zoom-Migration und QA-Gates unverändert.

## Entscheidungsmatrix (vollständige Funktionsinventur)
| Funktion | V2-UI und Ort | Entscheidung / Verworfenes |
|---|---|---|
| Pattern A–H | acht direkte Tasten im Kopf | keine Dropdown-Verzögerung; aktives Pattern optisch eindeutig |
| Länge 16/32 Steps | zwei direkte Taster „1 TAKT“ / „2 TAKTE“ im Kopf neben Pattern | Shift-Klick als unauffälliger Ersatz darf bleiben, wird nicht vorausgesetzt |
| Pattern Copy/Paste/Clear/Undo | **ein** PATTERN-AKTIONEN Menü oben neben A–H (ohne zusätzliche Längenwahl) | vier räumlich getrennte Buttons unten fallen weg |
| Velocity Variation / Reset | Menüeinträge in PATTERN-AKTIONEN | zwei Großbuttons unten fallen weg; keine Änderung von Random Velocity |
| Swing | echter bipolar? Nein: **unipolarer Drehregler** 0..50 % (bestehende DSP-/Controllergrenze) mit Wert | +/- fallen weg; 0 % = kein Swing |
| Humanize | Drehregler 0..100 % im Kopf | keine zusätzliche Fußzeile |
| Master Volume | eigenständiger größerer Drehregler im Kopf, 0..200 % mit neutralem 100 | +/- fallen weg |
| Zoom | ein Auswahlmenü 100/120/130/150 % rechts oben | keine vier Zoom-Kästchen. 140 % erst mit Controller-State-Test ergänzen; niemals rein optische Fake-Auswahl |
| Song On/Off | ein Taster in der Kopfzeile | Status aus Controller; eine einzige echte Ein/Aus-Funktion |
| Song-Kette bearbeiten | neben Song ein Edit-Button, separate View statt Instrumentraster | keine Duplizierung im Footer |
| Instrumentauswahl | sichtbarer Zeilenkopf mit Nummer + klares Selektionslicht | **nur** dieser Klick wählt – kein Sample-Öffnen/Pattern-Ändern |
| Sample laden | klarer Dateifeld-Klick oder Drag & Drop auf Instrument | keine unbeschrifteten Modifiertasten als einzige Methode |
| Vorhören | ▶-Button direkt beim Sample | keine globale Umleitung |
| Sample im Ordner vor/zurück | ◀ ▶ neben Sample | keine Modifikatoren erforderlich |
| Instrument umbenennen | eigenes sichtbares „…“-Menü neben dem Samplefeld (Laden / Umbenennen / Entfernen); Shift-Klick optional | Rename darf kein versehentlicher File-Open sein |
| Instrument + / - | zwei klare Tasten am Fuß bei Zeilenverwaltung | nur leere letzte Zeile entfernen, vorhandene Sicherheit bleibt |
| VOL | **kleiner Rotary** in Instrumentzeile, 0..200 % | kein +/-; Doppel-/Ctrl-Klick Standard 100 % |
| PAN | zentrierter bipolarer Rotary -100..+100, Mitte C | kein +/-; Double/Ctrl reset 0 |
| PITCH | diskreter Rotary -24..+24 st | Kein seitenweites Zusatzpanel |
| Random Velocity | eigener kleiner Rotary in **derselben Zeile**, 0..100 % | nicht mit festem Velocity-Pattern-Vary verwechseln |
| Choke Gruppe | kleines Dropdown 0..4 in Instrumentzeile | nur fünf diskrete Werte, kein Rotary |
| Ausgang | Dropdown MAIN / OUT 2..OUT 16 | nicht 16x zyklisch klicken müssen; Optionen bei Bedarf |
| Mute / Solo | direkt sichtbare beschriftete Zwei-Zustands-Tasten | kein Doppelklick erforderlich |
| Fill pro Instrument | beschrifteter Menüknopf direkt bei dieser Zeile | nicht als globales Fill verstecken |
| Steps | 32 beleuchtete Taster mit Taktgruppierung (16/32 mit klar inaktivem Takt 2) | kein 64-Step-Versprechen (DSP nur 16/32) |
| Velocity | **pro Zeile** kleiner Aufklapppfeil rechts neben Steps, darunter 32 vertikale Balken für diese Zeile | globaler Footer-Toggle und ±1/±10-Fußbereich entfallen; Mehrfachaufklappen möglich |
| Velocity-Wert | Wert des fokussierten Balkens direkt links an seiner Zeile | keine globale versteckte Step-Auswahl |
| Status/Hilfe | ein schmaler Statusstreifen ganz unten, nur relevante Fehler/Erfolge | keine Statuszeile zwischen die Bedienelemente |

## Geometrieentscheidung
1280 logische VSTGUI-Pixel, obere Kopfzeile 108 px, 4 Instrumente je Seite, 16 insgesamt; bei weniger als vier zeigt die Seite nur vorhandene Zeilen. Deutlich 4 statt 16 gleichzeitig sichtbare Reihen: vier vollwertige 94px-Reihen sind schneller verständlich als sechzehn 29px-Zeilen. **Jede einzelne Zeile** kann sich um 30px zur Velocity-Bearbeitung erweitern. Das Maximum 108 + 4*124 + 50 = 654 logische px, bei Zoom 130 % = ~850 phys. px; Windows 1080p. Ohne Velocity 108 + 4*94 + 50 = 534 logical. Bei 150 % max 981px Höhe und 1920px Breite. Fensterhöhe wird passend verhandelt, bei Host-Resize-Refusal nicht über Fenster malen.

Je Zeile: linker Bereich x16..616, Steps x622..1230. Gezielte Auswahl x16..66. Oben Samplebereich x78..398 inklusive sichtbarem „…“/PLAY/Ordnernavigation; Fill/M/S/Output x403..615, Choke darunter; unten zugeordnet die Rotary-Regler Vol/Pan/Pitch/Random und Choke. 32 Step-Kacheln beginnen x622, Breite 19; am Ende ein kleiner Schalter x1230..1266 (32-Step-Grid endet 1230) schaltet **nur diese** Velocity-Zeile um. Optionaler Balkenbereich unter Step-Reihe, nicht am Fensterfuß. Selected row farbig, eigene Hitfläche.

Das Layout der Bedienelemente wird vor endgültigem Code anhand der echten Zeichnungs-Hitboxen zentralisiert. Nicht von einem Bild abmalen. Schrifthöhen bei 100 %, 130 %, 150 %, deutsche Beschriftungen, 4 Zeilen und 2 Seiten prüfen.

## Interaktionsvertrag
- Rotary: Klicken selektiert/fokussiert nur das Control, vertikal ziehen verändert um definierten Schritt; links/rechts Unterteilung ist verboten. Ctrl-Klick oder Doppelklick setzt den echten Parameterstandard. Wert daneben live. Keine Mausradpflicht. Große sichtbare Knopffläche und passende Maus-Hitbox.
- Menüs: ein Klick öffnet den sichtbaren Auswahlbereich, nochmal klicken oder außerhalb schließt. Nur vorhandene Handler/Aktionen. Menüzustände sind transient, Recall schließt sie.
- Per-Row-Velocity: Pfeil blendet nur die eigene Zeile; alle anderen bleiben in ihrer vorherigen Ansicht. Mehrere gleichzeitig möglich. Die Balken bleiben genau unter ihren Steps. Velocity-Stroke + Undo und Recall-Generation gesichert; offene Velo-Anzeigen sind **UI-only** und verändern weder Audio noch Kit-State.
- Ein Klick auf Nummer/Zeilenkopf wählt **ausschließlich** Instrument aus. Ein Step-Klick toggelt nur den Step, Auswahlen sind nicht mit versteckten Parameteränderungen vermischt.
- Patternaktionen wirken stets auf aktives Pattern. 1T/2T ist **keine 64-Step-Funktion**. SONG-/FILL-Overlays blockieren Drag & Drop auf verdeckte Zeilen.
- Fehler, Host-Resize-Ablehnung, No Sample, No Output, unzulässige Aktion führen zu eindeutiger Rückmeldung; keine scheinbar erfolgreiche Aktion.

## Sechs Gegenprüfungen vor Abnahme
1. Musiker: ohne Handbuch Kick/HiHat laden, Pattern programmieren, Swing verändern, je Zeile Velocity editieren, Song öffnen.
2. Redundanz: jedes Bedienelement hat genau einen logischen Ort; keine duplizierten Regler, keine zusätzlichen +/--Bedienbatterien.
3. Ergonomie: Hitfläche mindestens ca. 24x24 logische Pixel; <100%-Zoom nicht unlesbar; Werte bei jeder Änderung sichtbar.
4. Edge Cases: 1/4/5/8/16 Zeilen, mehrere Velocity-Reihen gleichzeitig, Seitenwechsel, Host verweigert Resize, laufender Host, 1T/2T, Song/FILL-Overlay.
5. Technisch: View→Controller→Processor-Message→Projekt-Recall; Transient-Edit-Generation, Zoom, MIDI-/Hostpatternwechsel, Undo.
6. Kritischer Gegenentwurf: unnötige 2. Bedienbereiche, große permanente Fußzeilen, unsichtbare Spezialgesten und Menüs mit falscher Wertebereichsoption werden entfernt.

## QA / Definition „fertig“
MSVC Windows build, aktive CTests, Validator 47/47, gepackte VST3 Editor-/Audio-Hostsmoke, tatsächliche Windows-VSTGUI-Rasterbilder aller Views bei 100/120/130/150, UI-Interaktionstests für Knob/Dropdown/Row-Select/Velocity/Pages, Projekt-/Host-Resize-Regression. **Studio One bleibt Eigentümer-Abnahme**. Der spätere echte Skin verwendet ausschließlich Branding Master und freigegebene Knob-Designer-Assets, ohne GUI-Verhalten zu ändern.

## Änderungsabgleich nach unabhängiger UX-Gegenprüfung
- Der separate 1/2-Takt-Wert im Pattern-Aktionsmenü wäre eine weitere unnötige versteckte Funktion gewesen: V2 zeigt **direkt** „1 TAKT“ und „2 TAKTE“. Pattern-Aktionsmenü enthält nur Copy/Paste/Clear/Undo/Velocity-Operationen; keine doppelte Längensteuerung. Aktuelle C++-Implementation und Win32-Klick-Test sind maßgeblich.
- Sample-Bereich besitzt eine sichtbare „…“-Aktion mit Laden, Umbenennen, Entfernen. Unentdeckbare Shift/Alt-Spezialklicks sind lediglich optionale Kurzwege.
- PLAY, MAIN/OUT und CHOKE werden ausdrücklich bezeichnet. Ein eigener VELO-Text benennt den zeilenbezogenen Pfeil.
- Keine dekorativen Scheinregler: alle Knobs senden die vorhandenen echten Controller-/Processor-Parameter; kein 140%-Zoom ohne persistierten Host-Pfad.
- Verbindlicher Grafik-/Klickflächen-Handoff ist `resource/VSTGUI_ASSET_SLOTS.json` auf demselben HEAD. Alte V1-Footer-Metadaten sind ungültig.

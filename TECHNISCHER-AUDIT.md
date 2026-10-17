# 125A Drumator – unabhängiger technischer Zweitaudit

> **Bereitstellung auf GitHub:** Dieser Dokumentationscommit enthält ausschließlich den Auditbericht. Die technischen Feststellungen beziehen sich auf den unten genannten Prüfstand `a64c94a`. Die Angaben zu „keinen Commits/Pushes“ beschreiben die ursprüngliche Prüfphase; die anschließende Bereitstellung dieses Berichts wurde vom Auftraggeber ausdrücklich angefordert. Plugin-Code, Tests und Workflows werden dabei nicht geändert.

**Prüfstand:** `challanger2000/125A-Drumator`, Branch `develop/v1.0.0`, Commit **`a64c94a117ae7b485450b9367b7703039b949fa4`**, Version laut CMake und Factory **0.1.0**. Datum: 17.10.2026.

**Ergebnis: Die technische und provisorische GUI-Abnahme ist offen. Die endgültige Vintage-GUI sollte noch nicht implementiert werden.** Der Audit bestätigt die wesentlichen geometrischen Beanstandungen des Erstberichts und findet zusätzliche Fehler in Bedienzuordnung, Windows-Drag-and-drop, Host-Kommunikation, Legacy-Recall, Multi-Out und Importgrenzen. Außerdem ergibt eine unabhängige Filterberechnung eine erhebliche Schwäche bei hohen erlaubten Wiedergaberaten.

Dies ist ein **vollständiger quellenbasierter Review des vorhandenen Implementierungsstands mit analytischen Gegenprüfungen**, keine vollständige Laufzeitabnahme. Alle 20 Produktionsquelldateien, die GUI-Ressource, alle zehn Testdateien, Build/CI-Konfiguration und einschlägige Projektunterlagen wurden untersucht. **Kein Windows-VST3, kein nativer Editor und kein Studio One wurden in diesem Audit ausgeführt.** Aus der Prüfung folgt deshalb weder „alle übrigen Funktionen fehlerfrei“ noch eine Freigabe.

## 1. Vorgehen, Grenzen und Beweisklassen

Zuerst wurden `125A-Engineering/START-HERE.md`, die einschlägigen Engineering-Guardrails, Plugin-/DSP-/GUI-/Workflow-/AI-Engineering-Standards und QA-/Research-/Roadmap-Vorgaben gelesen. Danach folgten der vorhandene Auditbericht und der Auftrag aus [Issue #1](https://github.com/challanger2000/125A-Drumator/issues/1). Engineering-Referenzstand: `5f13a850c065167526519f0249fb7a38b5841a6a`.

Der Quellstand wurde über Git gelesen und außerhalb des bestehenden Checkouts als unverändertes Archiv untersucht. Der ursprüngliche Checkout blieb auf `7f5b4a7baba2371d9cc42c12406191ecbf3fe32f` und sauber. Es gab **keine Quellcodeänderungen, Commits, Pushes, Pull Requests, Issue-Kommentare oder ausgelösten Actions-Builds**. Bericht, CSV-Dateien und Prüfskript liegen außerhalb der Repository-Checkouts. Die allgemeine Engineering-Anweisung zur Pflege der Guardrails wurde wegen des ausdrücklich angeordneten reinen Prüfmodus nicht als Schreibauftrag ausgeführt.

Für SDK-Aussagen wurden tatsächlich die gepinnten Quellen gelesen:

| Abhängigkeit | Gelesener Commit |
|---|---|
| VST3 SDK 3.8.1 | `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| `pluginterfaces` | `4f547e8e102b47de4a8b8aaf343c73b700786372` |
| `public.sdk` | `586dc5e6c8012c3e4b01c79389375cbe96bdb1da` |
| `base` | `fcf9da0bd27a16f7f03773a3a39822f28f5c8477` |
| VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| `dr_flac.h` | Upstream-Datei aus `dfe8377631000664666519fdb83da193fd8037f4` |

**CONFIRMED:** Widerspruch oder fehlender Pfad ist unmittelbar im Quellcode/SDK nachweisbar; bei F14 zusätzlich numerisch abgeleitete Filterantwort. Das ist kein nachträglich behaupteter DAW-Reproduktionslauf.

**RISK:** Eine konkrete Implementierungseigenschaft kann einen Fehler verursachen, aber Auftreten, Akzeptanzkriterium oder Laufzeitfolge bleibt ungeprüft.

**HOST TEST REQUIRED:** Native Windows-Prüfung oder ausdrücklich Studio-One-spezifischer Nachweis fehlt. Viele generische VST3-/Mausprüfungen wären auch mit einem Windows-Testhost möglich; nur proprietäre Browser-, Song-, Mixer- und Integrationsabläufe erfordern Studio One selbst.

Prioritäten: **P1** vor technischer Abnahme beheben; **P2** vor GUI-/Bedienabnahme klären oder beheben; **P3** Dokumentation/Komfort. Es wird kein unbelegter P0-Crash behauptet.

## 2. Priorisierte Befundübersicht

| ID | Priorität | Status | Befund |
|---|---|---|---|
| F01 | P1 | CONFIRMED | Custom-View 1100 statt 1280: Steps 27–32 und 150%-Button vollständig abgeschnitten |
| F17 | P1 | CONFIRMED | Windows-Dateidrops werden wegen falscher Interpretation der SDK-Pfadlänge abgewiesen |
| F03 | P1 | CONFIRMED | Host-Parameter-/State-Änderungen lösen kein gezieltes GUI-Redraw/Relayout aus |
| F04 | P1 | CONFIRMED | Parameter-Flush ohne Audiosamples/-ausgänge wird vollständig ignoriert |
| F05 | P1 | CONFIRMED | Nichtparameter-Änderungen melden dem Host keinen Dirty-Zustand |
| F06 | P1 | CONFIRMED | DMR1–DMR3-Recall setzt Controller-Zustände unvollständig zurück |
| F07 | P1 | CONFIRMED | Ausgeblendete Solo-Zeile kann alle sichtbaren Instrumente blockieren |
| F08 | P1 | CONFIRMED | Legaler inaktiver MAIN mit Null-Samplepointern verhindert auch aktive Stems |
| F13 | P1 | CONFIRMED | FLAC-Größen-/Kanalgrenzen werden erst nach vollständiger Dekodierung geprüft |
| F14 | P1 | CONFIRMED, analytisch | 32-Tap-Filter dämpft bestimmte Aliasquellen bei Ratio 16 nur ca. 5,24 bzw. 12,27 dB |
| F02 | P2 | CONFIRMED | Abstände zwischen VOL/PAN/PITCH verändern VOL; weitere Draw-/Hitbox-Widersprüche |
| F09 | P2 | CONFIRMED, bedingt auf HiDPI | Manueller Resize berücksichtigt UI-Zoom, aber nicht Host-Content-Scale |
| F10 | P2 | CONFIRMED | Abgelehnter Host-Resize wird ignoriert; Inhalt und Modell werden trotzdem vergrößert |
| F11 | P2 | CONFIRMED | Verpflichtendes Ctrl+Links-Reset fehlt; Klick verändert stattdessen den Wert |
| F12 | P2 | CONFIRMED | Dateinamenverkürzung kann gültiges UTF-8 in ungültige Bytefolgen zerlegen |
| F15 | P2 | CONFIRMED | Kein ausgewählter Zoomzustand; Zoom wird beim Erstellen eines Editors nicht wiederhergestellt |
| F16 | P2 | CONFIRMED | Host-Parameter liefert normalisierte Zahlen statt der Pattern-Bezeichnungen A–H |

Die folgende Detailprüfung nennt für jeden Befund Ursache, Reproduktionsweg, Empfehlung und erforderlichen Gegenbeweis nach einer später genehmigten Korrektur.

## 3. Nachgewiesene Fehler und Abweichungen

### F01 – Breitenfehler schneidet funktionale Teile vollständig ab

**Belege:** [Drumator View.cpp:85–94](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L85), Zeichnung Zeilen 102–130 und 181–196; [Controller.cpp:100–104](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L100). SDK-Nachweis: VSTGUI `CViewContainer::drawRect`, `cviewcontainer.cpp:876–885`, begrenzt den Clip auf die Child-View; `CView::getMouseableArea`, `cview.cpp:312–321`, verwendet ohne Sonderattribut die View-Größe.

`adjustEditorHeight()` setzt Host-Anforderung und Parent auf 1280 Pixel, die eigene View aber auf 1100. Das ist mehr als ein potenzieller äußerer Fensterfehler: Im regulären VSTGUI-Container werden Zeichnung und Standard-Hit-Test an der Child-Grenze abgeschnitten.

**Exakte Folge:** Step 26 liegt mit seiner Füllung bei x=1087…1103 und ist teilweise abgeschnitten. Steps **27–32** beginnen mit ihrer Füllung ab x=1108 und liegen vollständig außerhalb. Der 100%-Button ist nur teilweise sichtbar/erreichbar; der 150%-Button x=1136…1217 vollständig außerhalb. Das gilt für jede Instrumentenzahl. Bei 150% wird die falsche logische Grenze lediglich auf 1650 physische Pixel skaliert.

**Reproduktion:** Editor attachen; rechten Teil von Step 26 sowie Steps 27–32 und 150%-Button prüfen. Bereits unabhängig vom DAW-Rendering ist der Rechteckwiderspruch beweisbar. Ein `getSize()` des ungeöffneten Editors prüft ihn nicht.

**Empfehlung:** Eine zentrale logische Breite für UIDESC, Parent, Custom-View, Zeichnung, Resize und Hit-Test verwenden. Anschließend tatsächliches Attach, Screenshots und Randklicks bei 1/4/16 Zeilen und 100/150% prüfen. Es wurde hier nichts korrigiert.

### F02 – Falsche Funktionszuordnung und abweichende Hitboxen

**Belege:** [Drumator View.cpp:285–292](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L285), [313–327](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L313), Zeichnung 117–130 und 164–180.

1. Der gesamte Bereich x=280…475 wird angenommen. PAN ist aber nur x=350…415 und PITCH erst ab420. Deshalb verändern Klicks **x=345…350 und x=415…420** jeweils **VOL +10**, obwohl sie zwischen sichtbaren Feldern liegen. Der zweite Abstand liegt unmittelbar vor PITCH. Modell-Gegenbeispiele: `(346,120)` und `(416,120)` → VOL+.
2. A–H: Gezeichnet werden 54 Pixel, angenommen werden 62. `(398,50)` liegt im sichtbaren Abstand nach A und wählt trotzdem A. Es gibt acht solcher 8-Pixel-Abstände einschließlich des Bereichs hinter H.
3. Zoom: gezeichnet y=39…69, angenommen im Header y=38…68. Oberhalb ist ein zusätzlicher Streifen aktiv; der unterste gezeichnete Streifen y=68…69 ist inaktiv. `(1080,68.5)` trifft keinen Zoompfad.
4. Pattern-Buchstaben werden in einem 58-Pixel-Textrechteck über einer 54-Pixel-Füllung zentriert: rechnerisch **2 Pixel nach rechts** verschoben.

**Empfehlung:** Zeichnungs- und Hit-Geometrie aus denselben Zielrechtecken ableiten. Die beiden numerischen Abstände dürfen keine benachbarte Funktion auslösen. Größere Step- oder M/S-Trefferflächen können sinnvoll sein, müssen aber bewusst definiert werden; sie sind nicht automatisch ein Fehler. Nach Korrektur positive Innenpunkte und negative Zwischenraum-/Randpunkte parametrisiert für alle Zeilen prüfen.

### F03 – GUI verfolgt Host-Automation und offenen State-Recall nicht zuverlässig

**Belege:** [Controller.cpp:38–90](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L38), [Controller.h:17–68](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.h#L17), [View.cpp:102–215](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L102). SDK: `EditController::setParamNormalized` ändert nur das Parameterobjekt; VSTGUI `ParameterChangeListener` bindet `CControl`-Instanzen. Drumator zeichnet in einer eigenen `CView` ohne Parameter-Dependent, Timer oder entsprechende Controller-Overrides.

Bei einem Host-Aufruf `setParamNormalized(1000, 1/7)` ändert sich der gelesene Patternwert, aber kein Drumator-Pfad invalidiert das Raster. `draw()` setzt es anschließend wieder explizit sauber. Eine zufällige spätere Fenster-Neuzeichnung kann den Fehler verdecken.

`setComponentState()` ersetzt zusätzlich Raster, Labels, Zeilenanzahl und Mixwerte, ohne die offene Custom-View zu benachrichtigen. Bei geändertem Zeilenstand fehlt auch der erneute `adjustEditorHeight()`-Aufruf. Dadurch können Display, Hit-Position der Aktionsleiste und vorhandene Fensterhöhe bis zu einem anderen Bedien-/Expose-Ereignis auseinanderfallen.

**Reproduktion:** Offenen Editor ruhen lassen, Host-Patternautomation A↔B ausführen; separat ein anderes Kit mit 4→16 Zeilen bei offenem Editor laden. Keine zusätzlichen Mausbewegungen oder manuellen Repaints als verdeckte „Lösung“ verwenden.

**Empfehlung:** Lebensdauersichere, UI-threadgebundene Model-/Parameterbenachrichtigung mit Invalidierung und erforderlichem Relayout; Registrierung beim Öffnen, Abmeldung beim Schließen. Alle offenen Ansichten berücksichtigen. Mit einem Windows-Testhost automatisierbar; tatsächliches Studio-One-Verhalten zusätzlich offen.

### F04 – Gültiger Parameter-Flush geht verloren

**Beleg:** [Processor.cpp:331–339](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorProcessor.cpp#L331). SDK: `public.sdk/source/vst/testsuite/processing/automation.cpp:234–242` erzeugt ausdrücklich einen Flush mit `numSamples=0`, `numInputs=0`, `numOutputs=0` und Nullpointern.

Die frühe Rückgabe kommt vor der gesamten Parameterbehandlung. Wählt der Host B während unterbrochener Audioverarbeitung und liefert den Wert einmalig als Flush, bleibt `selected_` auf A. Ein anschließendes `getState()` kann A speichern; bei Resume ohne erneute Übermittlung wird A gespielt. Die vorhandene Schleife akzeptiert außerdem nur `offset < data.numSamples`, wäre also auch nach bloßem Verschieben der Rückgabe für Null-Sample-Flush noch falsch.

**Empfehlung:** Parameterübernahme von der Audiopufferprüfung trennen und den dokumentierten Flushfall ausdrücklich behandeln. Regression: B ausschließlich in einen Null-Sample-/Null-Bus-Aufruf liefern, danach State und Resume ohne weitere Parameterpunkte auf B prüfen. Kein Rendern oder Asset-Dekodieren im Flush erforderlich.

### F05 – Steps, Samples und Mixänderungen bleiben dem Host als Projektänderung verborgen

**Belege:** Controller-Nachrichtenpfade [141–289](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L141) und [291–352](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L291). SDK `IComponentHandler2::setDirty`, `ivsteditcontroller.h:311–314`: Änderungen außerhalb von Parametern sollen dem Host als Dirty gemeldet werden.

Nur Pattern benutzt `beginEdit/performEdit/endEdit`. Laden/Löschen, Steps, Velocity, Gain, Pan, Pitch, M/S, Choke, Swing, Humanize und Zeilenzahl senden ausschließlich eigene Nachrichten. Kein einziger Pfad ruft `setDirty(true)` auf. Der Host erfährt somit über den vorgesehenen VST3-Kanal nicht, dass sein Songzustand geändert wurde.

**Folge abgrenzen:** `getState()` enthält die Werte grundsätzlich. Es ist **nicht** bewiesen, dass ein ausdrückliches „Speichern“ sie verliert. Gefährdet sind insbesondere Modified-Anzeige, Schließen-/Speichern-Rückfrage, Dirty-basierte Autosaves und Host-Undo. Welche Folgen Studio One konkret zeigt, ist ein eigener Praxistest.

**Empfehlung:** Erfolgreiche Nichtparameter-Edits im Controller als Dirty melden; für tatsächlich automatisierbare Einstellungen eine bewusst definierte stabile Parameterarchitektur wählen. Nicht bei abgelehnten Änderungen oder bloßem Editoröffnen Dirty setzen. Ein Mock-`IComponentHandler2` muss für jeden erfolgreichen Edit die Benachrichtigung messen.

### F06 – Legacy-Recall erzeugt unterschiedliche Controller- und Prozessorzustände

**Belege:** [Controller.cpp:74–90](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L74), [Processor.cpp:511–567](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorProcessor.cpp#L511), [Kit State.h:28–39](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/core/KitState.h#L28).

Bei **DMR1** stellt der Prozessor die Factory-Gridwerte wieder her. Der Controller lässt sein bestehendes `grid_` unangetastet. Nach früheren Edits zeigt er somit nicht den Zustand des Prozessors. Bei **DMR1–DMR3** setzt er weder `pitch_` noch `choke_` auf die Legacy-Defaults zurück. Der Prozessor erstellt dafür einen neuen `KitState` mit Pitch 0 Halbtönen und Choke 0.

**Reproduktion:** Dieselbe Controllerinstanz zunächst mit verändertem Raster, Pitch +12 und Choke 3 betreiben; dann DMR1 beziehungsweise DMR2/3 auf Processor und Controller laden. Anschließend Raster-/Pitch-/Choke-Anzeigen vergleichen. Auf den nächsten Plus-/Chokeklick achten: Der Controller rechnet vom falschen Altwert weiter.

**Empfehlung:** Alle Stateversionen zunächst in dasselbe vollständig initialisierte Kandidatenmodell migrieren und erst danach übernehmen. DMR1-Factory-Raster identisch rekonstruieren. Native Controller-/Prozessor-Regressionen müssen Legacy-Recall **nach einem bereits veränderten neuen Projekt** testen; nur Codec-Migration in einer frischen Instanz reicht nicht.

### F07 – Unsichtbares Solo blockiert sichtbare Zeilen

**Belege:** [Controller.cpp:129–165](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L129), [Visible Rows.h:14–21](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/core/VisibleRows.h#L14), [Processor.cpp:211–220](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorProcessor.cpp#L211), [Trigger Batch.h:38–43](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/core/TriggerBatch.h#L38).

**Reproduktion:** Vier Zeilen anzeigen. Leere Zeile 4 ohne Steps solo schalten. Minus betätigen. Die Prüfung sieht nur Sample/Steps; sie erlaubt das Ausblenden. `soloMask_` bleibt auf Bit 3. Keine der drei sichtbaren Zeilen erfüllt diesen Solo-Maskenwert; ihre neuen MIDI-/Sequencertrigger werden verworfen. Das blockierende Solo ist unsichtbar und wird mitgespeichert. Das unabhängige Policy-Gegenbeispiel bestätigt diese Konsequenz rechnerisch.

**Empfehlung:** Sichtbarkeit und Solopolitik gemeinsam definieren: Solo-Zeile nicht ausblenden, oder ihren Solozustand bei diesem ausdrücklich definierten Vorgang konsistent aufheben, oder sichtbare und versteckte Solozustände bedienbar anzeigen. Kein stilles Löschen anderer Einstellungen als beiläufige Reparatur. Regression für alle letzten Zeilen 2…16 sowie Save/Recall.

### F08 – Inaktiver MAIN verhindert nutzbare Aux-Ausgänge

**Belege:** [Processor.cpp:334–342](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorProcessor.cpp#L334), Stem-Verarbeitung 430–444. SDK `ivstaudioprocessor.h:185–198` erlaubt Null-Sampleadressen in einem inaktiven Audiobus; `ProcessData` behält die Busindizes bei.

`activateBus` erlaubt über die Basisklasse das Deaktivieren von MAIN und das Aktivieren einer Aux. Trotzdem verlangt `process()` immer zwei gültige MAIN-Samplepointer und bricht vorher ab. Damit wird auch ein gültiger aktiver DRUM-02-Ausgang nicht gerendert.

**Reproduktion:** MAIN deaktivieren, Aux 1 aktivieren; 16 Busbeschreibungen mit passenden Kanalpointerarrays liefern, aber MAIN-Samplepointer null lassen; MIDI 37 an eine geladene zweite Zeile. Erwartet wird Aux-Audio. Der Quellpfad liefert stattdessen `kResultFalse`.

**Empfehlung:** Busaktivierung und zulässige Pufferformen berücksichtigen. Ungenutzte MAIN-Summierung ohne Laufzeitallokation behandeln; aktive Ausgänge weiterhin rendern. Regression auch für inaktive Lücken zwischen aktiven Bussen und Aux 15. Ob Studio One diese konkrete Konfiguration anbietet, ist von der VST3-Vertragsabweichung zu trennen.

### F09 – Host-Content-Scale fehlt in manuell berechneter Fenstergröße

**Belege:** [View.cpp:85–89](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L85). Gepinntes VSTGUI `vst3editor.cpp:575–577`: `getAbsScaleFactor() = getZoomFactor() * getContentScaleFactor()`; `checkSizeConstraint()` verwendet diese Gesamtskalierung.

Drumator fordert `1280 * UI-Zoom` und `Höhe * UI-Zoom` an. Wenn der Host Content-Scale 2 liefert, fehlt Faktor 2: bei 100% fordert der eigene Pfad 1280 statt 2560 Pixel an. Diese Formelabweichung ist belegt; Hostkorrektur und tatsächliche Windows-DPI-Auswirkung wurden nicht ausgeführt.

**Empfehlung:** Offiziellen Gesamtskalierungs-/Frame-Resizevertrag verwenden. Betriebssystem-DPI und Benutzer-Zoom als getrennte Größen testen: jeweils 100/150% UI-Zoom bei Hostscale 1/1,5/2, einschließlich Wechsel zwischen Monitoren. DPI-unabhängige logische Rechtecke beibehalten.

### F10 – Resize-Ablehnung wird entgegen dem Kommentar nicht behandelt

**Belege:** [View.cpp:85–94](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L85), [294–306](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L294). SDK `VST3Editor::requestResize():628–636` liefert bei fehlendem Frame oder Hostablehnung `false`.

Drumator ignoriert das Ergebnis, verändert Parent-/Childgröße dennoch und hat die Zeilenzahl bereits im Controller und Prozessor erhöht. Der Kommentar „reject … explicitly“ entspricht dem Code nicht. Bei Ablehnung können neu angelegte Zeilen und die Aktionsleiste außerhalb des tatsächlichen Hostfensters liegen. Eine Rückkehr zu weniger Zeilen kann dadurch selbst unerreichbar werden.

**Empfehlung:** Hostantwort, reale Framegröße und Modellzustand abgleichen; bei Ablehnung einen definierten bedienbaren Fallback oder kontrollierten Rollback nutzen. Testhost mit absichtlich abgelehntem `resizeView()` und verzögerter Resizebestätigung verwenden. Nicht durch größere Hardcoded-Fenstermaße kaschieren.

### F11 – Standard-Reset ist nicht implementiert

**Belege:** [View.cpp:272–359](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L272), insbesondere 313–327; Engineering `STANDARDS/GUI-GUIDELINES.md` verlangt Ctrl+Links auf den tatsächlichen Default.

Ctrl wird nicht ausgewertet. Ctrl+Links auf PITCH rechts addiert einen Halbton; auf VOL rechts +10; auf Swing/Humanize ebenfalls einen Schritt. Das ist ein nachweisbarer Unterschied zum verbindlichen Resetvertrag. Es handelt sich außerdem um gezeichnete Textzonen, nicht um native Sliders mit geerbter Resetlogik. Mausrad, Drag und Zahleneingabe sind ebenfalls nicht implementiert; diese zusätzlichen Gesten sind als Funktionslücke zu bewerten, soweit sie für die endgültige Bedienung gefordert werden.

**Empfehlung:** Pro Ziel einen ausdrücklichen Gestenvertrag: Gain 100, Pan 0, Pitch 0, Swing 0, Humanize 0; Resetfähigkeit anderer Toggles gesondert definieren. Ctrl darf keine unbeabsichtigte normale Wertänderung bewirken. Native Klicktests an beiden Hälften aller entsprechenden Felder.

### F12 – Unicode-Dateinamen werden byteweise zerstückelt

**Beleg:** [View.cpp:154–160](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L154).

Ab mehr als 27 Bytes wird `substr(0,17) + "..." + substr(size-7)` erzeugt. UTF-8-Zeichen können in der Mitte getrennt werden. Konkretes Gegenbeispiel: 16×`a`, dann `ä`, dann 20×`b`, dann `.wav`. Das erste Teilstück endet nach dem ersten Byte von `ä`; die Ausgabe ist ungültiges UTF-8. Das wurde im unabhängigen Prüfskript nachgewiesen.

Unabhängig davon garantiert eine Bytezahl keine passende Schriftbreite: 27 schmale und 27 breite Zeichen benötigen unterschiedliche Pixelbreiten.

**Empfehlung:** Am gültigen Unicode-/Graphemrand kürzen und anhand tatsächlicher Fontmetriken ellipsieren. Testnamen: Umlaute, kombinierte Zeichen, CJK, Emoji, breite ASCII-Zeichen, sehr lange Basenames und `.flac`. Die konkrete Ersatzglyphen-/Renderreaktion in Windows bleibt ungeprüft.

### F13 – FLAC-Ressourcenlimit greift zu spät

**Belege:** [Flac Import.cpp:21–38](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/FlacImport.cpp#L21), [Controller.cpp:301–320](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L301). Upstream `dr_flac.h:11900–11965`: verwendete Komfortfunktion liest bis zum Ende und vergrößert den Floatpuffer wiederholt.

16 MiB komprimierte Eingabe begrenzen die dekomprimierte Länge nicht ausreichend. Erst **nach** vollständiger Floatdekodierung werden Kanalzahl, Frameanzahl und die 16-MiB-WAV-Zielgröße geprüft. Auch ein ohnehin nicht unterstütztes Mehrkanalfile wird vorher vollständig gelesen. Der Pfad läuft synchron aus dem GUI-Thread; er kann erheblichen RAM- und Zeitbedarf verursachen, bevor er ein File ablehnt. Ein realer OOM/DAW-Absturz wurde nicht provoziert und wird nicht behauptet.

**Empfehlung:** Decoder zunächst öffnen und Metadaten prüfen; Ausgabe mit begrenzter Allokation und begrenztem Framebudget inkrementell dekodieren. Auch unbekannte Gesamtlänge begrenzen, früh abbrechen und das gültige alte Sample erhalten. Tests mit stark komprimierter Stille, langen Streams, Mehrkanal, unbekannter Länge und knapp über der dekomprimierten Grenze; Ablehnung muss vor unzulässigem Ressourcenverbrauch eintreten.

### F14 – Antialiasing ist bei hohen erlaubten Ratios deutlich zu schwach

**Belege:** [Sample Voice Engine.h:15–54](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/core/SampleVoiceEngine.h#L15), [Sample Exchange.h:46–50](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/core/SampleExchange.h#L46). Vorhandener Alias-Test: `SampleVoiceTests.cpp::testBandlimiting`, ausschließlich Ratio 2.

Das Filter bleibt immer 32 Taps lang; nur sein Cutoff sinkt mit `1/ratio`. Die Übergangsbreite des endlichen Blackman-Filters wird dadurch bei großen Ratios relativ viel zu breit. Das unabhängige Prüfskript berechnet die komplexe Frequenzantwort aus den dokumentierten Koeffizientengleichungen, einschließlich Float32-Koeffizienten. Für die folgenden ganzzahligen Ratios durchläuft der Renderer Phase 0; die Rechnung ist deshalb unmittelbar relevant.

| Verhältnis | Frequenz in Zyklen je Quellframe | Ableitung der Aliasdämpfung | Aliasfrequenz bei 48 kHz Ausgabe |
|---:|---:|---:|---:|
| 2 | 0,40 | −87,49 dB | 9.600 Hz |
| 4 | 0,20 | −51,56 dB | 9.600 Hz |
| 8 | 0,10 | **−19,34 dB** | 9.600 Hz |
| 16 | 0,04 | **−5,24 dB** | 17.280 Hz |
| 16 | 0,06 | **−12,27 dB** | 1.920 Hz |

Konkreter unterstützter Fall: 192-kHz-Quelle, Host 48 kHz, Pitch +24 → Ratio 16. Ein 7.680-Hz-Quellton würde nach Pitch bei 30.720 Hz liegen, oberhalb der Ausgangs-Nyquistgrenze. Das Filter lässt seine Aliasquelle rechnerisch mit etwa 0,547 Amplitude durch; der gefaltete Anteil liegt bei 17.280 Hz. Ein 11.520-Hz-Quellton erzeugt den zweiten genannten Fall bei 1.920 Hz. Gemeint ist die stationäre Filterantwort fern der Samplegrenzen.

**Abgrenzung:** Das ist ein numerischer Filterbefund, **kein gemessener Plugin-Bounce und kein Hörurteil**. Es wurde auch kein nachträglicher willkürlicher QA-Grenzwert erfunden. Die Zahlen zeigen, dass der vorhandene Ratio-2-Test den erlaubten Gesamtbereich nicht charakterisiert.

**Empfehlung:** Qualitätsziel für Passband, Übergang und Stopband vorab festlegen; geeignete längenabhängige/multistufige oder anderweitig ausreichend geprüfte SRC-Lösung ableiten. Danach mit unabhängigen Sinus-Sweeps und musikalischen Samples für 0/±12/±24 Halbtöne sowie 44,1/48/96/192-kHz-Kombinationen messen. Rechenzeit und Latenz separat prüfen. Keine ungeprüfte Filtervergrößerung als fertige Reparatur behandeln.

### F15 – Zoomzustand fehlt als Anzeige und persistenter Editorzustand

**Belege:** [View.cpp:125–130](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L125), [Controller.cpp:92–96](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L92), Controller.h.

Beide Zoomflächen werden immer gleich gezeichnet. Es gibt keinen markierten aktuellen Modus. Ein neuer `VST3Editor` erhält nur erlaubte Faktoren; der vorherige Wert wird weder zwischengespeichert noch über Controllerstate wiederhergestellt. Damit kann ein neu erzeugter Editor auf SDK-Default 100% zurückfallen. Das ist zusätzlich zu F01 zu betrachten: Auch nach einer Breitenkorrektur bleiben Anzeige und Persistenz ungelöst.

**Empfehlung:** Exklusive, aus dem tatsächlichen Editorzoom abgeleitete Markierung; Editorpräferenz beim Schließen/Öffnen konsistent halten, falls projektbezogen gewünscht versioniert speichern. Wiederholter Klick darf den gewählten Modus nicht optisch deaktivieren. Ob Studio One eine View wiederverwendet oder neu erstellt, gesondert prüfen.

### F16 – Host-Patternwerte sind keine A–H-Bezeichnungen

**Belege:** [Controller.cpp:28–29](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorController.cpp#L28). SDK `vstparameters.cpp:83–119`: der generische `Parameter` gibt bei `stepCount=7` normalisierte Fließkommazahlen aus und besitzt keine A–H-Texte.

Intern gilt korrekt `round(normalized*7)`. Die Host-String-API kennt aber beispielsweise B nur als normalisierten Zahlenwert ungefähr 0,1429. Das erschwert verständliche Automation und die Gegenkontrolle zwischen Frontpanel und Host. Es ist kein Beweis für falsch spielende Pattern; der Fehler liegt in der Darstellung/Parametersemantik.

**Empfehlung:** Diskrete Stringliste A–H unter Beibehaltung von ParamID **1000**, Normalisierung und gespeichertem Auswahlwert. API-Regression für alle acht Strings und ihre Rückkonvertierung; tatsächliche Studio-One-Automationsdarstellung zusätzlich prüfen.

### F17 – Normales Windows-Datei-Drag-and-drop scheitert am Längenvertrag

**Pluginbelege:** [DrumatorView.cpp:63–67](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L63) und [247–252](https://github.com/challanger2000/125A-Drumator/blob/a64c94a117ae7b485450b9367b7703039b949fa4/src/vst3/DrumatorView.cpp#L247). **Primärbeleg der exakt gepinnten Abhängigkeit:** [VSTGUI Win32DataPackage::getData, Zeilen 144–156](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32datapackage.cpp#L144).

Das SDK gibt bei einem Dateipfad `buffer = strings[index].c_str()` zurück, als Größe jedoch **`strings[index].length()`**. Der Nullterminator liegt damit bei `buffer[size]`, nicht bei `buffer[size-1]`.

Drumator prüft an beiden Stellen ausdrücklich `bytes[size-1] == '\0'`. Bei einem gewöhnlichen Pfad `C:\Drums\Kick.wav` ist das Byte an dieser Stelle `v`. Bereits `allowed()` liefert daher `false`: kein Copy-Cursor, kein `loadPath()`, kein eigentlicher Import. FLAC-Pfade scheitern entsprechend am letzten `c`. Dies betrifft den normalen Windows-`kFilePath`-Pfad unabhängig von proprietären Studio-One-Browserobjekten.

**Reproduktion:** Ein einzelnes WAV aus dem Windows-Explorer auf eine erreichbare linke Zeilenfläche ziehen. Im Testhost kann dasselbe mit einem `IDataPackage` reproduziert werden, dessen Rückgabewerte exakt dem SDK entsprechen. Das beigefügte Python-Gegenbeispiel prüft genau diese Byte-/Längenbedingung; ein nativer Drop wurde hier nicht ausgeführt.

**Empfehlung:** Den wirklichen `IDataPackage`-Längenvertrag berücksichtigen und die Bytefolge begrenzt in einen gültigen Pfad überführen. Optional mitgelieferte Terminatoren nur innerhalb der tatsächlich gemeldeten Länge behandeln; kein unkontrolliertes Lesen darüber hinaus. `allowed()` und `acceptFileDrop()` dürfen keine widersprüchlichen Längenregeln besitzen. Regressionen für das echte Win32-Datenpaket, Unicodepfade, WAV/FLAC, falsche Datentypen und Mehrdateidrops. Danach Studio-One-Browserpayloads separat untersuchen.

**Korrektur gegenüber Erstaudit G07:** Dort wurde nur die Host-/Payloadkompatibilität offen gelassen. Für den vorhandenen normalen Win32-Dateipfad liegt bereits eine konkrete SDK-Vertragsabweichung vor.

## 4. Konkrete Risiken und offene Verhaltensentscheidungen

### R01 – Kit-Restore ist nicht als vollständige Audio-Transaktion veröffentlicht (P1)

`Processor.cpp:456–476` bereitet zunächst alle Assets vor, veröffentlicht sie dann aber in **16 einzelnen Pending-Slots**. `setState():550–567` ersetzt danach Raster und Mixwerte ebenfalls einzeln. Der Host darf `setState()` laut `IComponent` auch während Processing auf dem UI-Thread aufrufen. Der Audiothread kann während der Veröffentlichung dazwischenlaufen: Er kann Zeile 1 bereits neu, Zeile 2 noch alt adoptieren und alte/neue Grid-/Mixteile beobachten. Der Mutex schützt den Controlcache, nicht diesen Audio-Snapshot.

Fehlerhafte Kandidaten werden gut vorab validiert; das ist jedoch **nicht dasselbe wie atomare hörbare Übernahme eines gültigen neuen Kits**. Außerdem bleiben alte bereits gestartete Stimmen absichtlich am alten Asset. Ob das bei einem kompletten Presetwechsel erwünscht ist, ist nicht festgelegt. Kein konkreter Mischblock wurde hier gerendert.

**Empfehlung/QA:** Ein vollständig vorbereitetes Kit samt Pattern/Mix als Generation am Blockrand übernehmen; alte Stimmen-/Tailpolitik ausdrücklich festlegen. Während realer Audiocallbacks wiederholt zwei sehr unterschiedliche Kits laden, in jedem Block Generation und Audio gegen einen unabhängigen Sollzustand prüfen. Individuelle Atomics nicht mit einer kompletten Transaktion gleichsetzen.

### R02 – Echtzeitfreigabe fehlt trotz sinnvoller Ownership-Architektur (P1)

`process()` enthält nach statischem Review keine direkte Dateiarbeit, Dekodierung, Mutex-Sperre, Formatierung oder offensichtliche Heapallokation. `SampleExchange` nimmt Pointer vor der Dereferenzierung atomar in Besitz; alte Stimmen werden vor Reclaim abgefragt; tatsächliche Löschung erfolgt außerhalb Audio. Das sind positive Befunde.

Nicht nachgewiesen sind Callback-Allokationsfreiheit der finalen Windows-Binärdatei, Lockfreiheit sämtlicher verwendeter Atomics im Zielbuild sowie p95/p99/max und Deadlines. Bei 64 Stimmen sind allein 64×32×2 = **4096 Tap-/Kanalbeiträge pro Ausgabesample** zu berechnen. Dazu kommen Sortieren, bis zu 96 Retire-Slots und Referenzprüfungen. Host-Event-/Parameterpunktzahlen begrenzen die Eingabeschleifen, aber es gibt kein eigenes gemessenes Lastbudget. Ein 8192-Block-Throughput-Test ist kein gepacter Realtime-Test.

**QA:** Windows-End-to-End bei 32/64/128/256/1024 Frames und mehreren Raten, 64 langen Stimmen, UI-/Import-/Automationstress, mehreren Instanzen; echte Callbackkadenz, Allokationszähler, Baseline, p95/p99/max, Deadlineüberschreitungen und Dropouts. Keine Aussage „realtime-safe vollständig verifiziert“ allein aus dem Quellreview.

### R03 – Überläufe und Choke bei voller Polyphonie sind nicht nutzerseitig diagnostiziert (P1/P2)

`TriggerBatch` fasst 2048 Events. `Processor.cpp:372,417` ignoriert fehlgeschlagene Appends; `rendered.rejectedCapacity` wird nicht ausgewertet. Ein verworfener Trigger bleibt daher im Host/GUI ungemeldet. Bei voller 64-Stimmen-Auslastung führt `SampleVoiceEngine.h:248–280` die Choke-Fades **vor** der Suche nach einem freien Voiceplatz aus. Eine neue geschlossene Hi-Hat kann somit alte Stimmen abblenden, obwohl ihr eigener Trigger anschließend verworfen wird.

Die Grundentscheidung „kein hartes Voice Stealing, zusätzliche Hits ablehnen“ ist ausdrücklich dokumentiert und deshalb nicht selbst ein Fehler. Die Choke-/Kapazitätskombination und gleichzeitige MIDI-/Sequencerhits brauchen eine verbindliche Musik-/Prioritätspolitik. Bei gleichem Offset sortiert `TriggerBatch` nach Zeile; dadurch bestimmen Zeilenreihenfolge und Kapazität das Ergebnis.

**QA:** 64 lange Stimmen plus neuer Choke-Hit, 16 gleichzeitige Gruppenhits, Midi+Step-Duplikate, >2048 Eingangsereignisse und dichte Patternautomation; tatsächliche Drops messen und auf dem Controlthread anzeigen.

### R04 – Import und Pitch-Neuberechnung können UI und Speicher stark belasten (P2)

`Controller::loadWav` liest synchron und dekodiert zum Preflight; der Prozessor kopiert die Quelldaten und dekodiert erneut. `adjustRowPitch` dekodiert das vorhandene WAV und erstellt alle 32.768 Kernelkoeffizienten für **jeden** Halbtontastendruck neu. State-Decode validiert WAVs ebenfalls, `setupProcessing` präpariert alle Zeilen erneut. Das bleibt außerhalb Audio, kann aber den DAW-UI-Thread merklich blockieren.

64 MiB eingebettete Quelldaten sind kein Gesamt-RAM-Limit: PCM8 kann als Float deutlich wachsen; alte klingende Sampleversionen bleiben zusätzlich erhalten. Das feste erlaubte Verhältnis `ratio<=16` kann an ungewöhnlichen Hostraten oder hohen Quellraten zusätzlich Samples ablehnen, obwohl die Dateiformate an sich gültig sind.

**Empfehlung:** UI-freundliche begrenzte Workerarbeit mit Generation/Abbruchpolitik und intaktem Single-Producer-Vertrag; gesamte RAM-/Zeitbudgets messen, Fehlermeldungen nach tatsächlicher Ursache differenzieren. Keine direkte Einführung mehrerer Publisher in das bestehende SPSC-Modell.

### R05 – FLAC-Kurzdekodierung ist nicht auf Vollständigkeit geprüft (P2)

Die verwendete `dr_flac`-Komfortfunktion liefert im gelesenen Upstreamcode die **tatsächlich gelesene** Framezahl. Der Wrapper prüft nicht unabhängig, ob sie zur deklarierten Gesamtzahl passt oder ein Abbruch durch EOF/Defekt vorlag. Ein beschädigter oder gekürzter Mehrframe-Stream mit bereits gültigem Anfang könnte deshalb als kürzeres Sample erscheinen. Die vorhandene Negativprüfung zerstört nur den ersten Magic-Bytewert und deckt diesen Fall nicht ab.

**QA:** Echte Mehrframe-FLACs an Frame-/Blockgrenzen und innerhalb des letzten Blocks kürzen, CRC-/Metadatenfehler setzen; nie unbemerkt ein gekürztes Sample als vollständig geladen melden. Der konkrete Fehlerfall bleibt bis zum ausgeführten Decoderfixture **RISK**, kein behaupteter beobachteter Datenverlust.

### R06 – Transportdiskontinuitäten und Tail-/Mixsemantik sind nicht vollständig festgelegt (P2)

`process():344–347` erkennt PLAY→STOP, aber keinen Seek/Loop-Sprung bei weiter gesetztem Playing-Flag. Neue Steps folgen dem aktuellen PPQ; alte Stimmen können weiterlaufen. Das kann für Drumtails musikalisch richtig sein, braucht aber eine explizite Entscheidung und Loop-/Seektests.

Ebenso sind **Mute/Solo, Gain, Pan, Pitch, Sample Clear/Replace** teilweise Trigger-/Launch-Einstellungen: Bereits klingende Stimmen behalten Sample, Kernel und Launchgain/-pan. Der vorhandene Test fordert sogar ausdrücklich, dass Mute keine Tails tötet. Deshalb wird dieses Verhalten nicht nachträglich als neuer Bug ausgegeben. Bei langen Samples muss der Anwendervertrag aber klar sein; insbesondere „VOL 0“ bedeutet derzeit nicht, dass eine bereits klingende Stimme sofort still wird. Ein normaler Mixfader würde häufig anders erwartet.

### R07 – Multi-Out-Verhandlung ist noch keine vollständige Hostmatrix (P2)

Es werden 16 Stereoausgänge registriert. Aux 1…15 erhalten fest Zeilen 2…16; bei deaktivierter/fehlender Stemdestination fallen diese Zeilen auf MAIN zurück. Die positiven Tests prüfen drei gleichzeitig gerenderte Busse, nicht alle 16, und verwenden teilweise kürzere `ProcessData`-Buslisten.

Der Erstbericht nennt die Annahme variabler `numOuts` als Risiko. **Kein isolierter Fehler allein daraus nachgewiesen:** Auch die gelesene SDK-Basisklasse toleriert kürzere Arrangementarrays. Die Spezifikation erwartet vom Host jedoch grundsätzlich eine zur registrierten Busliste passende Struktur. Diese Unterscheidung wurde im Erstaudit nicht ausgeführt.

**QA:** Vollständige 16-Bus-Arrays, inaktive Lücken, letzter Aux, MAIN inaktiv (F08), ein-/ausschalten während klingender Stimmen und nach Projektneustart. Keine doppelten Summen, keine Altbuffer als aktive Ausgaben. MAIN-only und alle Stems getrennt messen. Freies Output-Assignment ist im aktuellen Modell nicht vorhanden.

### R08 – Weitere Grenz- und Robustheitsprüfungen fehlen (P2/P3)

- WAV-Float akzeptiert endliche Werte bis Betrag 16; das erhält Headroom absichtlich. NaN wird abgewiesen. Vollständige ±Inf-, Subnormal-, Floatgrenz- und Stereo-/Extensible-Matrizen sind nicht vorhanden; FTZ/DAZ bzw. Denormalmessung ist nicht belegt.
- WAVE_FORMAT_EXTENSIBLE wird nur bei `validBits == containerBits` akzeptiert; 24 gültige Bits in einem 32-Bit-Container sind ausdrücklich nicht unterstützt. RF64, RIFX und AIFF werden ebenfalls nicht versprochen. Diese Einschränkungen sind keine bewiesenen Decoderfehler, müssen aber transparent dokumentiert werden.
- MIDI ist NoteOn-36…51-One-shot, NoteOff wird absichtlich ignoriert. Kanal-/Busprüfung, All-Sounds-Off/All-Notes-Off und ein mögliches MIDI-Learn sind nicht implementiert bzw. nicht spezifiziert. Keine allgemeine GM-Map behaupten: es ist eine zusammenhängende 16-Noten-Zuordnung.
- `fileName.assign` und einzelne String-/Pathoperationen liegen außerhalb umfassender Exceptiongrenzen. Eine garantierte OOM-sichere VST3-/GUI-Grenze ist nicht nachgewiesen; daraus folgt kein beobachteter Crash.
- Der FLAC-Download ist auf einen Commit gepinnt und TLS wird geprüft, aber CMake besitzt keinen erwarteten Dateihash. Lokales `VST3_SDK_ROOT` wird nicht auf den gepinnten Stand geprüft; CI prüft ihn. Reproduzierbarkeit außerhalb CI und Lizenz-/Notice-Beigaben sollten vor einem Kundenpaket überprüft werden.

## 5. Gesamte provisorische GUI: Geometrie und Bedienprüfung

Es gibt **keine einzeln angelegten nativen Knobs, Slider, Eingabefelder oder Stepbuttons**. Die gesamte Oberfläche ist ein Custom-`CView`, das Rechtecke und Strings zeichnet und Mauskoordinaten selbst auswertet. Leere `control-tags` sind in dieser Architektur nicht automatisch eine fehlende Tagverdrahtung; entscheidend ist die explizite Aktionskette. Die Anlagen erfinden deshalb keine angeblich realen UI-Tags.

### 5.1 Vollständige Feld-/Elementliste

Koordinaten sind logische Pixel. Für eine Zeile mit Nullindex `r` gilt `y=108+29*r`, für Step mit Nullindex `s` gilt `x=560+21*s`. Die CSV-Anlage expandiert diese Formeln auf **jede Zeile und jeden Step**.

| Element | Zeichnung/Anordnung | Aktion / Zustand / Bewertung |
|---|---|---|
| Titel | x16…1000, y8…32, Font18, links | Rein textuell; Schriftbild/Pixelscreenshot offen |
| Untertitel | x16…320, y47…67, Font12 | Pattern-/Shift-Hinweis; nicht interaktiv |
| Pattern A–H | x340+62p…+54, y38…68 | Param1000; gewähltes Pattern farbig, aber F02/F03 |
| 100% | x1070…1127, y39…69 | UI-Zoom1; teilweise abgeschnitten, vertikale Randabweichung |
| 150% | x1136…1217, y39…69 | UI-Zoom1,5; vollständig außerhalb Custom-View |
| SWING-Wert | x16…182, y74…98 | 0…50%; −/+5; ein gemeinsamer Text `- / +`, keine getrennten sichtbaren Buttons |
| Swing −/+ | x185…214 /214…244 | Nachrichtenweg; kein Reset/Hostparameter |
| HUMAN-Wert | x275…447, y74…98 | 0…100%; −/+10; ebenfalls geteilter Text |
| Human −/+ | x448…477 /477…505 | Nachrichtenweg; kein Reset/Hostparameter |
| Bedienhinweis | x545…1240, y74…98 | Erklärt VOL/PAN/P und Alt-Sample; rechter Teil außerhalb 1100 |
| Zeilenband | x10…1246, y…y+28 | Alternierende Hintergründe; rechter Teil abgeschnitten |
| Zeilennummer | x16…47, y…y+26 | Ebenfalls Teil der Samplelade-Hitbox, kein separater Nummernbutton |
| Sample-/Leername | x52…275, y…y+26 | Klick lädt; Alt löscht; Hitbox x16…275 über volle 29px; F12 |
| VOL | x280…345, y…y+26 | 0…200 linear, 100=Unity; linke/rechte Hälfte −/+10; keine Prozent-/dB-Einheit |
| PAN | x350…415, y…y+26 | −100…+100 Stereo-Balance, ±10; zentrale Null; F02-Abstände |
| PITCH `P` | x420…475, y…y+26 | −24…+24 Halbtöne, ±1; kein eindeutiger Einheitenhinweis |
| M | x478…500, y+3…y+24 | Toggle; orange wenn an; Hitbox reicht über volle Zeile |
| S | x507…529, y+3…y+24 | Toggle; Accent wenn an; volle Zeilenhöhe als Hitbox; F07 |
| Choke | x535…558, y+3…y+24 | Nur Ziffer 0…4; zyklischer Klick; keine sichtbare Spaltenbezeichnung/Erklärung |
| Step1…32 | Füllung x+2…x+18, y+4…y+22 | 16×18px sichtbar; Klickzelle 21×29px; normal 0↔127, Shift 32/64/96/127/0 |
| Velocitybalken | x+14…x+16, Höhe 3…16 | Kann die fünf Stufen zusätzlich zu den drei Aktivfarbgruppen unterscheiden; kein Zahleneditor |
| Vierergruppenrahmen | x+1…x+19, y+3…y+23 | Nur bei s%4=0; keine Stepnummern und kein Lauflicht |
| Hinzufügen | x16…213, Aktions-y…+29 | +1 bis 16; bei 16 anderer Text; weiterhin Klick mit Statusmeldung möglich |
| Letzte Zeile ausblenden | x226…266, Aktions-y…+29 | Nur bei sample-/stepfreier letzter Zeile; ein Minimum von 1; Solo nicht berücksichtigt |
| Zeilenanzahl | x277…455, Aktions-y…+29 | Reiner Text `N / 16 ZEILEN` |
| Status | x16…1240, Footer-y…+21 | Lokal im Editor; zugehörige Fehlertexte vorhanden, teilweise generisch; keine persistente Fehlerhistorie |

**Keine unbelegte Beschriftungsfreigabe:** Dateiname, VOL/PAN/P, Chokezahl, Hilfe und Status wurden hinsichtlich Quellzuordnung geprüft. Genaue Glyphenbreiten, Baselines, Beschnitt an Fontgrenzen und Lesbarkeit auf Windows brauchen echte Font-/Pixelprüfungen. Die gemeinsame linke numerische Ausrichtung und die M/S-Rechtecke sind rechnerisch konsistent; die Pattern-Zentrierung ist es nicht. Für die finale Gestaltung sind eindeutig erkennbare ±-Ziele, Maßeinheiten und Choke-/Schrittorientierung zu verbessern.

### 5.2 Größen bei verschiedenen Zeilenzahlen

`H(N)=max(300,172+29*N)`. Die Resource erlaubt maximal 650 logische Pixel Höhe; das ist für den größten Inhalt 636 ausreichend. **650 gegen 636 ist für sich kein Fehler.** Die Pixelgrößen setzen Host-Content-Scale 1 voraus.

| Zeilen | Soll 100% | Soll 150% | Eigenes View-Clip rechts 100/150% | Aktionen-y / Status-y logisch |
|---:|---:|---:|---:|---:|
| 1 | 1280×300 | 1920×450 | 1100 /1650 | 142 /176 |
| 4 | 1280×300 | 1920×450 | 1100 /1650 | 229 /263 |
| 5 | 1280×317 | 1920×475,5 | 1100 /1650 | 258 /292 |
| 8 | 1280×404 | 1920×606 | 1100 /1650 | 345 /379 |
| 16 | 1280×636 | 1920×954 | 1100 /1650 | 577 /611 |

SDK-Fenstergrößen werden auf ganze Pixel abgerundet. Insbesondere bei ungeraden Höhen und 150% sind Rundung, Randklicks und Schriftpixel zu prüfen. Für alle 1…16 Zeilenzahlen und beide Zooms enthält `GUI-GEOMETRIE.csv` vollständige nominale Draw-/Hitrechtecke und Clippingmarkierung.

Bei 150% beträgt schon der Inhaltsbereich 1920 Pixel Breite; auf einem 1920-Pixel-Monitor bleibt damit kein Platz für normalen Hostfensterrand. Es gibt keinen eigenen Scroll-/Fit-Fallback. Das ist ein **Platz-/Praxistestrisiko**, kein aus der Quellgröße allein bewiesenes Studio-One-Fensterproblem. Auch die Studio-One-Chrome und verfügbare Arbeitsfläche sind zu messen.

### 5.3 Zustands- und Interaktionsmatrix

Die Anlage **`BEDIENELEMENTE.csv`** enthält 4.288 Kontrollzeilen: alle **4.096 Pattern-/Zeilen-/Stepadressen** plus Pattern-, Zoom-, globale und zeilenbezogene Aktionen. Für jeden Eintrag stehen Aktion, Controllercallback, ParamID/Nachricht, DSP-Folge, GUI-Rückmeldung, Recall und Automation. Das sind **4.288 nachvollzogene Quellzuordnungen, keine 4.288 ausgeführten Klicktests**.

`GUI-GEOMETRIE.csv` enthält 13.520 expandierte Rechtecke einschließlich Texten und Hintergründen über 32 Layoutvarianten. Jede Zeile trägt ausdrücklich `native_visual_and_click_evidence=NOT RUN`. Eine positive mathematische Erreichbarkeit darf nicht als native Mausabnahme gelesen werden.

## 6. Funktionsprüfung über die GUI hinaus

| Bereich | Unabhängig im Quellstand festgestellt | Verbleibender Nachweis |
|---|---|---|
| Factory/Identität | Separate stabile Processor-/Controller-FUIDs; InstrumentDrum; `kDistributable`; Produktname 125A Drumator; Version 0.1.0 | Tatsächliches installiertes Paket/FUIDs/Rescan in Studio One |
| Audioformat | Float32 vorgesehen; Float64 wird bewusst abgelehnt | Kein Float64-Bug allein aus Nichtunterstützung; native Format-/Lifecyclematrix |
| Lifecycle | Eigene `setProcessing`-Implementierung; Stop/Deactivate setzt Stimmen zurück | Wiederholtes Start/Stop, Offline/Realtime, Mehrinstanzen, Native Editor-Reopen |
| Sequencer | 8×16×32, 1/16, aus Host-PPQ abgeleitet; negative PPQ und 32-Step-Wrap; keine eigene freie Clock | Tempo-/Loop-/Seekmatrix, Automation+Swing+Humanize über echte VST3-Blöcke |
| Patternautomation | Einziger exponierter Hostparameter 1000; zeitliche Teilintervalle; gleiche Offsets ergeben letzten Wert vor nächstem Intervall | F03/F04/F16; viele Punkte, A→B→A in einem Block, Grenzen und Flush |
| Swing | Verzögert ungerade Steps um 0…0,5 Step; kein vorgezogener Hit | Native Audiozeitmessung aller Eckwerte, Änderungen während laufender verzögerter Hits |
| Humanize | Deterministische Verzögerung 0…10% Step und zeilenbezogene Velocityvariation bis ±12; bei 0 neutral | Reale Transienten/Offline-Identität, Verhalten bei Liveänderungen festlegen |
| MIDI | NoteOn 36…51, Offset und Velocityvalidierung; Velocity 0 ignoriert; NoteOff absichtlich ignoriert; gestoppter Transport möglich | Alle Kanäle/Busaktivierung, Controller-/Panikvertrag, reale Tastatur/Piano Roll |
| Voices | Max 64, absichtlich kein harter Steal; Retrigger überlappt; neue Voices behalten Launchparameter | R03, echte Lastverteilung, Prioritäten, Rückmeldung bei Drops |
| Choke | Gruppe 0 aus, 1…4 aktiv; 2-ms-Fade sampleratenabhängig | Alle Gruppen, simultane Hits und volle Voices; Hörtest |
| Mix | Gain 0…2; Stereo-Balance unity in Mitte, keine versteckte Normalisierung | Bereits laufende Stimmen behalten Werte; Produktentscheidung; Stereo-/Endpunktmatrix |
| Multi-Out |16 Stereoausgänge; feste Row→Bus-Zuordnung; Aux-Deaktivierung fällt auf MAIN zurück | F08/R07; alle 16 in Studio One, Mixer-/Songrecall |
| WAV | RIFF PCM8/16/24/32, Float32, Mono/Stereo, ausgewählte Extensible; Nonfinite abgewiesen | Breiter Fixturekorpus, headroom/subnormal, R08-Einschränkungen |
| FLAC | Dateidialog dekodiert in Float32-WAV; Windows-Drop ist durch F17 blockiert; gespeicherter Inhalt ist konvertiertes WAV bei ursprünglichem FLAC-Namen | F13/F17/R05; längere Stereo-/Mehrframe-/Defektfixtures |
| Replace/Clear | Steps bleiben erhalten; neue Trigger adoptieren neue/silent Assets; alte Voices behalten Eigentümer; Dateidrop erreicht wegen F17 den Ladepfad nicht | Clear-/Replace-Tailvertrag, Importfehlerstatus, Nebenläufigkeit |
| Projektstate | DM10 speichert Auswahl, Zeilen, M/S, Gain/Pan/Pitch/Choke, Swing/Humanize, 4096 Gridbytes, Namen/WAVs; Längen-/Hashprüfung | F05/F06/R01; tatsächlich geöffnete View und echter Studio-One-Song |
| Stategrenzen |16 MiB je eingebettetem WAV, 64 MiB Quellen pro Kit; FNV-1a als Korruptionsprüfung | Keine kryptografische Authentizität versprochen; RAM-/Kontrollthreadbudget separat |
| Sicherheit/Seiteneffekte | Laufzeitpfad zeigt Dateilesen nur für Nutzersample; keine ersichtliche Telemetrie, Prozessstarts oder Runtime-Netzwerkpfade | Kein Binär-/Supply-Chain-Sicherheitsaudit behauptet; Importrobustheit offen |

### Noch nicht implementierte Produktziele

`docs/V1-SCOPE.md` ist ausdrücklich ein **Entwurf**, keine gesperrte fertige Spezifikation. Daher werden die folgenden Lücken getrennt von nachgewiesenen Implementierungsfehlern geführt:

- **Lauflicht/aktueller Step:** Kein Playhead-/Transportstatus wird an die GUI übertragen oder gezeichnet, obwohl README und Konzept einen Running-Light-Sequencer nennen. Auch Stepnummern fehlen.
- **Freier Instrumentenname:** Nur der Samplebasename wird angezeigt und gespeichert; keine vom Dateinamen unabhängige Umbenennung.
- **Freie Ausgangszuordnung:** Keine Zeilen-Outputauswahl; Routing ist fest durch Rowindex und Hostbusaktivierung bestimmt.
- **Samplepreview im Editor:** MIDI-Audition existiert, aber kein Previewbutton/-klick und kein Vorschauworkflow aus der Samplewahl.
- **Copy/Paste und Undo:** Keine eigene Implementierung; Nichtparameter-Edits haben auch keine normalen Hostparameter-Gesten.
- **Numerische Eingabe/Mausrad/Reglerdrag:** Nicht vorhanden; die Provisoriumsbedienung besteht aus Halbfeldklicks.
- **Vintage-Charakter:** Kein entsprechender DSP oder Bypass; als optional/aufgeschoben dokumentiert. Das ist keine Aufforderung, ihn vor der technischen Bereinigung einzubauen.

Vor Festlegung des endgültigen Frontpanels muss entschieden werden, welche dieser Ziele zu V1 gehören und wie sie bedient werden. Andernfalls würde das finale Layout auf einer unvollständigen Funktionsliste basieren.

## 7. Vorhandene Tests und unabhängige Beleglage

### 7.1 Tatsächlich vorhandene Testinhalte

| Datei / Target | Tatsächlich geprüfter Inhalt im gelesenen Test | Wesentliche nicht abgedeckte Punkte |
|---|---|---|
| `CoreTests.cpp` | Schedulerpositionsorakel, negative PPQ, Wrap, Blockpartition, Swing/Humanize, verschiedene Raten/Tempi, Workcap, Patternstate | SDK-Adapter mit vielen in-block Patternpunkten; live Clock-/Statewechsel |
| `SampleVoiceTests.cpp` | DC/Unity, Stereo/Pan, Retrigger, absichtliches Tailverhalten bei Mute, Choke, 64 Voices, Replacement, Ratio2-Alias-Test, drei simultane Stems | Hohe SRC-Ratios; Choke+volle Polyphonie; p99; alle 16Stems |
| `WavDecoderTests.cpp` | Chunkreihenfolge/Padding, PCM-Endpunkte, 8/16/24/32/Float, Extensible-Beispiele, NaN, ungültige Container | Vollständige Subformat-/Stereo-/Extremwert-/Subnormalmatrix |
| `SampleExchangeTests.cpp` | Ownership, alte Voices, Reclaim, falsche Rate, Withdraw, abgelehnte Samples | Kein tatsächlicher Zwei-Thread-Publish/Withdraw/Adopt-Stresstest; überwiegend sequentielle Aufrufe |
| `TriggerBatchTests.cpp` | Noten/Velocity/Offsets, Sortierung, Masken, 2048 Kapazität | Adapter ignoriert Dropzähler; Bus-/Kanal-/Paniksemantik |
| `AtomicPatternTests.cpp` | Alle Randindices, Restore, ein Writer/Reader mit 40000 Schreibvorgängen | Keine atomare komplette Kit-/Mehrparameter-Transaktion |
| `VisibleRowsTests.cpp` | 1…16, Sample-/Stepsperre in anderem Pattern | Solo auf auszublendender Zeile fehlt |
| `KitStateTests.cpp` | DM10-Roundtrip, DMR4…9-Migration, Korruption/Truncation, 16 Rows, 16-/64-MiB-Grenzen | Controller-Migration DMR1…3, laufender Audio-Snapshot, echte große dekodierte Kits |
| `EditorGeometrySmoke.cpp` | Lädt finales Windowsmodul, erzeugt Controller/View, prüft äußeres `getSize()==1280×300` | **Kein Attach, kein Draw, kein Click, kein Zoom, kein Row-Resize, keine Screenshots** |
| `PackagedAudioHostSmoke.cpp` | Factory/Connection/Lifecycle, PCM16/24/Float, MIDI-Offset, Embedded Recall frische Instanzen, 16 Samples nacheinander, drei Busse, Steuernachrichten/State, Stop, ein Midblock Patternwechsel, 8192 Durchsatzblöcke, FLAC-Minifixture, 48→44,1kHz | Keine nativen Controller-GUI-Aktionen; überwiegend direktes `processorPoint->notify`; kein Dirty-/Flush-/vollständiger Automationstest; kein Realtimepacing |

Die Tests nutzen überwiegend bedingungsunabhängige Checkfunktionen mit Throw/Exit und werden nicht allein durch Release-`NDEBUG` entkräftet. Die Datei-/Testzählung wurde gegen CMake abgeglichen: **acht CTests**, dazu **zwei separate Windows-Smoke-Executables**, die im Workflow manuell aufgerufen werden. Der automatisch abgeleitete `drumator_test_suite` vermeidet die frühere veraltete Targetliste.

### 7.2 Bestehender Windows-Run

Die vorhandene [Actions-Ausführung #39058731130](https://github.com/challanger2000/125A-Drumator/actions/runs/39058731130) wurde **nur gelesen**. Die zugängliche GitHub-Zusammenfassung bestätigt **Success**, Commit `22567d67af7834d8da97fff9e2dd78c86e456487`, Branch `develop/v1.0.0`, Dauer 2 min 42 s und ein Artefakt. Der Gitvergleich von diesem Commit zu `a64c94a` zeigt ausschließlich den hinzugefügten Erstbericht; Quellcode, Tests, Ressource und Workflow sind gleich.

Die behaupteten Einzelwerte wie „47/47 Validator“ sind in den Projektunterlagen dokumentiert. Die Detail-Logs erfordern im verfügbaren Webzugang eine Anmeldung; der direkte GitHub-API-Zugriff antwortete 403. Sie wurden deshalb **nicht erneut unabhängig vollständig ausgelesen**, das Artefakt wurde nicht heruntergeladen und die Binärdatei wurde hier nicht getestet. Der bestätigte Runstatus darf als vorhandener CI-Beleg berücksichtigt werden, nicht als neue vollständige Verifikation aller Logzahlen oder GUI-Interaktionen.

### 7.3 In diesem Audit ausgeführt

- Read-only Git-/Quell-/SDK-Prüfungen und Vergleich der festgehaltenen Baseline.
- Python-Gegenbeispiele für falsche Hitzuordnung, Patternabstände, Zoomrand, Stepbeschnitt, verstecktes Solo, ungültige UTF-8-Kürzung und Win32-Dateidrop-Längenvertrag.
- Unabhängige komplexe DTFT-Berechnung der SRC-Filterantwort für fünf Gegenbeispiele.
- Expansion der gesamten Kontrollmatrix und sämtlicher 1…16-Zeilen-/100-/150%-Layouts.
- SHA-256-Prüfung aller 50 unveränderten Snapshotdateien.

**Nicht ausgeführt:** CTests, Windowsbuild, Steinberg Validator, 125A Plugin Tester, reale Audio-/VST3-Harnessläufe, native GUI, Screenshots, Studio One, gepacter Realtimebenchmark. Es stehen keine Windowsausführung und keine Studio-One-Installation bereit; es wurden entsprechend der Windows-only-Regel auch keine Linux-Pluginbuilds als Ersatz erzeugt.

## 8. Bewertung des bisherigen Auditberichts

| Erstaudit | Unabhängiges Ergebnis |
|---|---|
| G01 Breitenwiderspruch | **Bestätigt und verschärft:** exakte abgeschnittene Stepindices und Zoomfolgen, durch gepinnten VSTGUI-Clip-/Hit-Testvertrag abgesichert |
| G02 Patternabstände | **Bestätigt:** zusätzlich falsche VOL-Zuordnung in numerischen Abständen und Zoom-y-Randfehler |
| G03 Geteilte Hitbereiche/Choke | **Bestätigte Quellgestaltung:** schlecht erkennbare Halbfeldaktionen; Ctrl-Reset fehlt; Benutzerakzeptanz noch offen |
| G04 Text/Fonts | **Teilweise als echter Fehler konkretisiert:** ungültiges UTF-8 nachweisbar; tatsächliche Glyphen-/Baselineprüfung weiterhin offen |
| G05 Responsive Lifecycle | **Offen plus konkrete Ursachen:** unbehandeltes Resize-Ergebnis false, fehlende Host-Content-Scale und fehlendes State-Relayout |
| G06 Synchronisation | **Konkreter als bloßes Hostrisiko:** fehlende Invalidierung, Legacy-Controllerabweichung, Flushverlust und fehlende Dirty-Nachricht |
| G07 Drag/Drop | **Neuer bestätigter Fehler F17:** Win32 liefert die Pfadlänge ohne Terminator, Drumator verlangt ihn innerhalb dieser Länge. Normale Dateidrops werden abgewiesen. `true` als „Drop behandelt“ bleibt unabhängig davon legitim |
| Teilintervalle Patternautomation | Grundalgorithmus plausibel; vorhandener Test deckt nur einen Midblockwechsel ab; viele Punkte und Flush weiterprüfen |
| Variable Busanzahl | Nicht pauschal als Fehler bestätigt; SDK toleriert teilweise Arrays ebenfalls. Neuer konkreter Fehler F08 ist davon unabhängig |
| Infinite Tail | Im Code absichtliche Generatorentscheidung; nicht allein als Fehler bewertet. Tatsächliche Hostlast/Stopped-Idle separat messen |
| „ENGINE CI: PASS“ | Vorhandener Runstatus bestätigt; Umfang bleibt CI. Kein eigener Windows-/GUI-/Realtime-PASS aus diesem Audit |

Der Erstbericht ist als Ausgangspunkt nützlich, aber seine offene Risikoliste hätte weder den Flush-, Dirty-, Legacy-, Solo- noch den inaktiven-MAIN-Fall zuverlässig abgefangen. Diese zusätzlichen Prüfpfade gehören in die nächste QA-Runde.

## 9. Noch ausstehende native und Studio-One-Prüfungen

### 9.1 Native Windows-Prüfungen – auch außerhalb Studio One automatisierbar

1. Editor wirklich attachen, zeichnen und Eingaben zustellen; Custom-View-, Parent-, Frame- und HWND-Clientmaße getrennt messen. F01 zuerst beheben, sonst sind Randklicktests zwangsläufig unvollständig.
2. Alle 4.096 Step-Adressen über A–H, alle Zeilenfunktionen, sämtliche ±Hälften, Zwischenräume und Ränder mit echten Mausereignissen prüfen. Normal/Shift/Alt/Ctrl, wiederholte Klicks, Default/Minimum/Maximum und abgelehnte Edits berücksichtigen.
3. 1/4/16 Zeilen als Bild-/Klickabnahme, zusätzlich sämtliche Zwischenhöhen als Resizegrenztest; beide UI-Zooms und Hostscale 1/1,5/2. Vollständige sichtbare Pixel erfassen; leere/unvollständige PrintWindow-Bilder nicht als GUI-Beleg akzeptieren.
4. Patternautomation mit 0-Sample-Flush, Punkten 0/N−1, mehreren Wechseln in einem Block und gleichem Offset; mit Swing/Humanize und angrenzenden Blockgrenzen kombinieren. Audio-Onsets und gespeichertes Pattern vergleichen.
5. Offener Editor: Automation, vollständiger Kitwechsel, neue Zeilenzahl, Close/Reopen mindestens 20 Zyklen, zwei Views/zwei Instanzen und Hostparameteranzeige.
6. Vollständige 16-Bus-Pufferstruktur einschließlich MAIN inaktiv, letzter Aux und inaktiver Lücken; korrekte L/R-Werte, Isolation, keine Doppelung.
7. Legacy DMR1…9 und DM10 in bereits verändertem Controller/Processor laden; abgelehnte Zustände unverändert lassen; gültige Kitwechsel unter Audio auf einheitliche Generation prüfen.
8. Importkorpus mit kurzen/langen mono/stereo WAV/FLAC, Unicodepfaden, allen dokumentierten Bitformaten, defekten Chunks, gekappten FLACs und Grenzgrößen; alte Assets bei Ablehnung erhalten.
9. SRC-Frequenz-/Alias-/Passbandmatrix, Impulse und reale Drums; Leistungs- und Präzisionsmetriken vorab festlegen. Offline- und Realtime-Audio unter identischer Eventfolge vergleichen.
10. Gepacter Realtime-/Allokations-/Deadline-Stress sowie Steinberg Validator und 125A Plugin Tester auf dem später autorisierten finalen Windows-Paket. Keine neue Actions-Ausführung wurde hier gestartet.

### 9.2 Ausschließlich durch Studio-One-Praxis belastbar abzunehmen

| Szenario | Objektiver Sollnachweis |
|---|---|
| Browser/OS-Drag-and-drop | Betriebssystem-Dateipfad und Studio-One-Browser getrennt; Sample landet exakt in Zielzeile, Status stimmt, Pitch/Mix/Steps bleiben erhalten |
| Song Modified/Schließen/Autosave | Nur Sample oder Step ändern; Host erkennt Änderung; Rückfrage/Autosave/Undo entsprechen festgelegtem Vertrag |
| Song Save/Reopen | Echtes Songprojekt mit 16 unterschiedlichen Samples, Patterns, Velocity, M/S, Mix, Pitch, Choke, Swing, Humanize speichern; Quelldateien verschieben; nach Neustart identisches Audio |
| Hostautomation | Patternbezeichnungen, Aufnahme/Wiedergabe, gestoppte Audioengine, Editor offen/geschlossen, Recall und sichtbare Markierung stimmen überein |
| Mixer/Multi-Out | Alle 16 Stereoausgänge, MAIN-only, Aux-Aktivierung, Channelnamen und Songrestore; gemessene Isolation ohne Summendoppelung |
| GUI/HiDPI | Vollständige Screenshots und Randklicks bei 100/150%,1/4/16 Zeilen, mehreren DPI-/Monitor-/Fensterkonfigurationen; kein abgeschnittener Button |
| Transport/Render | Preroll/negative PPQ, Loop, Seek, Tempoänderung, Stop/Resume; Echtzeit-/Offlinebounce und MIDI-Audition erwartungskonform |
| Reale Session | Mehrere Instanzen, Trackwechsel, Editorhide/reopen, gleichzeitiges Importieren und Abspielen, lange Samples; keine ungeklärten Dropouts/Deadlines |

„Studio Pro 8.1.2“ in alten Protokollen und das tatsächlich eingesetzte Studio One sind mit **genauer Produkt-/Versions-/Windows-/ASIO-Angabe** zu protokollieren. Keine stillschweigende Gleichsetzung unterschiedlicher Hostversionen.

## 10. Reparaturreihenfolge und Freigabekriterien für die Vintage-GUI

**Empfohlene Reihenfolge nach gemeinsamer Auswertung und ausdrücklicher späterer Korrekturfreigabe:**

1. GUI-Grundgeometrie und Lebenszyklus: F01/F02/F03/F09/F10/F17; echte 100/150%-Klick-/Screenshotmatrix als messbare Basis.
2. Host-/Projektintegrität: F04/F05/F06/F07/F08; Kontrollmatrix durch native Controller-/Prozessorchecks schließen; R01 Kit-Transaktion und Tailvertrag klären.
3. Import-/Audiogrundlagen: F13/F14 und R03/R04/R05; SRC-Qualitätsziel, Last-/RAM-Budgets und vollständige Importrobustheit belegen.
4. Bedienverträge: F11/F12/F15/F16, Maßeinheiten, Preview/Benennung/Outputauswahl/Undo und Lauflicht als V1-Ziele entscheiden.
5. Erst nach erfolgreicher Technik-/Provisoriumsabnahme statisches Vintage-Produktdesign mit aktuellen Branding-/Knob-Designer-Mastern festlegen; anschließend GUI implementieren und die gleiche Funktionsmatrix erneut validieren.

**Objektive Gates vor Beginn der endgültigen GUI-Implementierung:**

- Keine offenen P1-Funktions-/State-/Hostfehler; P2-Abweichungen behoben oder als bewusste Provisoriumseinschränkung nachvollziehbar akzeptiert.
- Jede UI-Aktion hat vollständigen belegten Pfad bis Audio/State/Rückmeldung; jede Hostparameteränderung ist sichtbar; keine geheimen/falschzugeordneten Klickzonen.
- Vollständig bedienbares Layout bei 100/150% und 1/4/16 Zeilen; Full-View-Screenshots und native Eingaben, nicht nur `getSize()`.
- Erfolgreiche Legacy-/DM10-Migration und echter Studio-One-Songrecall einschließlich Samples, Outputbusse und Projektänderungserkennung.
- Alle 16 Outputs und MIDI-/Automationseckfälle verifiziert; Offline-/Realtimeverhalten dokumentiert.
- SRC-/Import-/Choke-/Voicegrenzen quantifiziert; gepacter Realtimebericht mit p95/p99/max, Deadline- und Dropzählern für festgelegte Zielhardware.
- Unabhängiger 125A Plugin Tester-/Validatornachweis auf dem eindeutig zugeordneten Windows-Paket, plus separate Studio-One-Abnahme. Build-/CI-Erfolg allein genügt nicht.
- Finale Funktionsliste und Bedienverträge stehen fest, damit das Vintage-Panel keine fehlenden Grundfunktionen verdeckt oder nachträglich umgebaut werden muss.

Diese Gates sind Empfehlungen und Prüfkriterien. **Es wurde keine Reparatur vorgenommen und keine Freigabe unterstellt.**

## 11. Anlagen und Reproduzierbarkeit

Die folgenden Prüfdateien wurden im ursprünglichen Cloud-Workspace erstellt und geprüft. **Sie sind nicht Teil dieses auf ausdrücklichen Wunsch auf den Bericht beschränkten Commits und über die folgenden Dateinamen nicht auf GitHub abrufbar.** Die Befunde, Quellenverweise, Reproduktionswege und zusammenfassenden Geometrietabellen stehen vollständig oben im Bericht.

- `BEDIENELEMENTE.csv`: 4.288 expandierte Aktions-/Datenflusszuordnungen.
- `GUI-GEOMETRIE.csv`: 13.520 Rechtecke über alle 32 Zeilen-/Zoomvarianten; logische und nominalphysische Geometrie.
- `PRUEFERGEBNISSE.json`: konkrete Gegenbeispiele, 32 Layoutmaße und numerische Filterantworten.
- `audit_checks.py`: reproduzierbare Quellguards, Geometrie-/Policygegenbeispiele und unabhängige DTFT-Rechnung; führt keinen C++-/VST3-Code aus.
- `source-manifest.json`: SHA-256 der 50 Dateien des unveränderten Prüfsnapshots.
- `PROVENIENZ.json`: Baseline, Git-Objektvergleich, ursprünglicher Checkoutstatus und genaue Grenzen des vorhandenen CI-Belegs.

Wiederholung der analytischen Checks im vorbereiteten Workspace:

```bash
python /workspace/scratch/drumator-audit-results/audit_checks.py
```

Die Prüfartefakte sind außerhalb der Quellcheckouts abgelegt. Das Prüfskript kontrolliert zuerst die Snapshot-Hashes; es ersetzt die Windows-/Studio-One-Tests nicht.

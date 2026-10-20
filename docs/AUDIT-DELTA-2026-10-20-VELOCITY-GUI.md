# 125A Drumator – Funktions-/UX-Audit: Fortsetzung 20.10.2026

**Basis:** `develop/v1.0.0` @ `5f7944ba4ca50745cfc6520cca70c6d36d623189`. Vor Merge erneut aktuellen Branch/Dateiversion prüfen. Dieses Delta ergänzt frühere Audits, es erklärt nicht ältere bereits behobene Fehler erneut zu FAIL. **Kein Studio-One- oder Release-PASS aus dem Windows-Simulator ableiten.**

## A-01: Leere Velocity-Balken erzeugen neue Noten

**Einstufung: bestätigter UX-Bug, Patch + Regressionsprüfung.** Im bisherigen `DrumatorView::onMouseDown` wird beim Klick auf einen leeren Velocity-Balken ohne Vorabprüfung `setStepVelocity(...,1..127)` aufgerufen. `onMouseMoved` schreibt beim Ziehen über leere Spalten weitere positive Werte. Das widerspricht dem expliziten Standardablauf: Note im Step-Raster aktivieren, Velocity darunter bearbeiten. Der Nutzer kann sonst versehentlich Steps erzeugen, die nicht bewusst im Raster gesetzt wurden.

**Änderung im Audit-Branch:** Nur bestehende Noten (ursprünglicher Wert > 0) lassen sich im offenen Velocity-Bereich durch Klick/Ziehen verändern. Der Drag über 0-Spalten erstellt keine Notes und keine zusätzlichen Undo-Einträge. Ein Klick auf 0 gibt die konkrete Handlungsanweisung `LEERER STEP: ZUERST IM RASTER AKTIVIEREN`. Ein Klick auf eine reguläre Step-Zelle aktiviert die Note weiterhin. Ctrl-Erase vorhandener Notes bleibt unterstützt; Undo bündelt den Drag. Neuer nativer Windows-Regressionstest `UX Velocity empty-step editing` prüft Blank-Klick, Drag über Blank-Zelle, Änderungen bestehender Notes, Undo und anschließenden regulären Step-Klick.

## A-02: Veraltete *verbindliche* GUI-Slotdatei

**Einstufung: bestätigt, bisherige Dokumentation widerspricht ausführbarem Programm und RV01-Spezifikation.** `resource/VSTGUI_ASSET_SLOTS.json` zeigte noch 4 Rotaries, darunter `RANDOM VEL 0–100%`, obwohl für das neue GUI ein Button mit `randomizeRowVelocity(row)` beschlossen ist; enthielt den längst entfernten Pattern-Menüpunkt `LAENGE 1T / 2T`; verwendete 49px breite `+`/`−`-Footerzonen statt `+ ZEILE`/`− ZEILE` und unterschlug 16er-Paginierung sowie 370px Höhe des Song-Editors. Gerade für einen separaten GUI-Chat ist das eine reale Implementierungsfalle.

**Änderung:** Slot-Manifest auf **Ist-Stand** korrigiert und vom **verbindlichen Ziel-Override** abgegrenzt: Drei künftige Rotaries plus one-shot Random-Velocity-Button. Nicht fälschlich eine neue Button-Hitbox aus alter Rotary-Mitte erfinden. Song-Paginierung, Footer, direkte Takt-Schalter, Drag-Sensitivität und QA-Status synchronisiert. Das beschreibende `docs/VSTGUI-HANDOFF.md` erhält einen unmissverständlichen Änderungsvorrang.

## A-03: Legacy-Realtime-Randomizer hinter neuem Taster

**Einstufung: RELEASE-BLOCKER, noch **nicht** repariert.** Der gespeicherte `randomVelocity_[row]`-Prozentwert und der Audio-Callback `triggers_.varyVelocity(...)` bleiben wirksam. Wird nur die optische Rotary durch einen One-Shot-Taster ersetzt, können ältere Projekte weiterhin unsichtbar laufende Random-Velocity-Modulation nutzen, sodass gespeicherte Balken und Klang erneut auseinanderlaufen. Die neue Controller-Methode RV01 funktioniert bereits und ist separat getestet, sie deaktiviert die Altfunktion nicht.

**Entscheidung vor endgültigem GUI-Merge erforderlich:** Entweder die Realtime-Funktion explizit als eigenständige sichtbare Option weiterführen **oder** eine dokumentierte, versionierte Abkündigung mit Projekt-Recall-/Audio-Regression. Das alte gespeicherte Kitformat und seine Wiedergabe dürfen nicht versehentlich durch eine optische GUI-Änderung stillschweigend anders interpretiert werden. Bis zu dieser Entscheidung: keinen RELEASE-PASS und nicht so tun, als sei der neue Taster bereits in der sichtbaren GUI.

## A-04: Multi-Out und Host

**Status: technischer Patch SO01 Windows PASS, Studio One offen.** Sekundäre Ausgänge sind nun `kMain` (vorher `kAux`) mit optionaler Inaktivität beim Start. Gepackter VST3-SO01-Test prüft Bus-Metadaten, M27/M30/F08/T02 prüfen Audioausgabe. `MAIN` bis `DRUM 16` können zuvor bereits im Studio-One-Dialog angezeigt, aber nicht aktiviert werden. Einen tatsächlich erfolgreichen Klick auf `DRUM 02` und neuen Studiomixer-Kanal hat der Nutzer auf dieser Buildversion noch **nicht** bestätigt.

## A-05: Übrige Besitzerbefunde / Feature-Design

- Genauigkeit der Velocity-Balken: bestehender Expanded-Lane-Code zeigt für die gewählte Zeile bereits `STEP n: value /127`; die GUI-Endabnahme muss beweisen, dass dieser Wert für jeden Klick/Drag lesbar und eindeutig bleibt.
- Song-Länge und Pattern-Länge: aktuell zwei **bewusst unabhängige** Einstellungen; kein funktionaler Fehler allein durch unterschiedliche Anzeigen. Bessere Kennzeichnung in endgültiger GUI sicherstellen.
- 4-/8-Takt-Wunsch: `kSteps=32`, maximal 32 Step-Spalten; 128 Steps pro Pattern wurden vom Nutzer ausdrücklich abgelehnt. Eine größere Patternlänge braucht getrennt genehmigtes kompaktes Paging oder Pattern-Chain-Konzept einschließlich Song-Laufzeit, serialisiertem Zustand und Regression. Nicht ad hoc DSP-/Kit-Format auf 64/128 ändern.
- Browser Drag-and-drop, 130–150% Skalierung und echte Studio-One-Projektwiederherstellung sind zusätzliche native Owner-Gates.
- Frühere Geometrie-Header und historische Tests nicht vorschnell löschen: erst Referenzen, Produktruntime und Testabdeckung ermitteln, um keine nützlichen Negativregressionen zu verlieren.

## CI und Übergabe

Dieser Commit liegt auf isoliertem Audit-Branch. **Windows-Teststatus zum Zeitpunkt der Niederschrift: AUSSTEHEND.** Test-/Validator-PASS ausschließlich für den konkreten späteren GitHub-Actions-Commit und dessen Logs festhalten. Die fertige visuelle GUI muss die angepasste Slotdatei sowie `Controller::randomizeRowVelocity(row)` verbindlich übernehmen, eigenständig Klickgrenzen testen und darf funktionierende Processor- und State-Implementierungen nicht durch Skinning überschreiben.

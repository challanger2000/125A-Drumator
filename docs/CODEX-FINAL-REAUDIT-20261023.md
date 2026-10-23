# Unabhängiger FINAL-Re-Audit – 125A Drumator

**Prüfdatum:** 23.10.2026, Europe/Berlin.

**Repository:** `challanger2000/125A-Drumator`.

**Geprüfter Produktcommit:** `6879d829afabbe5d21be2fea17d58b018f77d5cc`.

**Geprüfter Branch:** `fix/studio-one-acceptance-20261022`.

**Berichtsbranch:** `audit/codex-final-reaudit-20261023`.

**Gesamturteil: Noch keine technische V1-Freigabe.**

Der geprüfte Branch enthält die zuletzt überarbeitete VSTGUI und lag zum Prüfzeitpunkt 112 Commits vor `develop/v1.0.0`. Sein HEAD wurde vor und nach der Untersuchung abgeglichen. Die tatsächliche Pluginversion ist weiterhin **0.1.0**; „V1“ bezeichnet den Abnahmeauftrag, keine durch diesen Audit veröffentlichte Version.

Die früheren Reparaturen haben wesentliche Fehler tatsächlich beseitigt. Dennoch bleiben **zwei HIGH-, sieben MEDIUM- und zwei LOW-Befunde**. Einer der MEDIUM-Befunde betrifft einen unwirksam gewordenen Regressionstest. Einen neuen BLOCKER habe ich nicht nachgewiesen. Das ist keine Garantie, dass außerhalb der verfügbaren Nachweise keine weiteren Fehler existieren.

**Arbeitsweise:** ausschließlich lesende Quellcode-, SDK- und Testanalyse. Keine Produktreparaturen, keine Änderungen an Tests oder Workflows, keine Builds und keine Studio-One-/ASIO-Ausführung. Dieser vollständige Bericht wurde nach ausdrücklicher Ergänzung des Auftrags als einzige neue Datei auf einem separaten Auditbranch gespeichert und committet. Die im ursprünglichen Chatbericht genannte rein lesende Untersuchung und die anschließend autorisierte Berichtsablage sind getrennte Arbeitsschritte.

## 1. Nachweise und Grenzen

Gelesen wurden `125A-Engineering/START-HERE.md`, die einschlägigen Engineering-, GUI-, DSP-, QA- und Repository-Regeln, Projekt-`AGENTS.md` sowie die aktuellen Branding-Vorgaben. Engineering-Regelstand: `9854275cdad1d35476397d874ba95efc4f2b184a`.

Untersucht wurden insbesondere:

- Audio-, Sample-Exchange-, SRC-, Voice-, Choke- und Routingpfade.
- Pattern-, Song-, Transport-, MIDI-, Automations- und Restorelogik.
- WAV-/FLAC-Import, WAV-Bearbeitung und Vorhörengine.
- Aktive VSTGUI-Zeichen-, Maus-, Drop-, Resize-, Fokus- und Lifecyclepfade.
- Bestehende Regressionen, ihre Instrumentierung und ihre tatsächlichen Prüfaussagen.

Der vorhandene [Windows-Lauf #40001184842](https://github.com/challanger2000/125A-Drumator/actions/runs/40001184842), Job/Check `120577371474`, gehört **exakt zum geprüften Produktcommit**:

| Prüfung | Vorhandener Nachweis |
|---|---|
| Windows-x64-Kompilation und Linken | PASS |
| CTest | **23/23 PASS** |
| Native Integrationsfälle | **88 einzeln protokollierte PASS**, einschließlich FA23-01–FA23-10 |
| Steinberg Validator | **47/47 PASS** |
| Paketierter Audiohost | PASS |
| Paketierter Editorhost | PASS |
| Studio One/Studio Pro, ASIO, interaktive Abnahme | **NICHT DURCHGEFÜHRT** |

Die individuellen Fallabschlüsse sind über die GitHub-Checks-Annotations verfügbar. Die anschließende ausführliche Diagnoseannotation ist abgeschnitten; daraus rekonstruiere ich keine fehlenden Messwerte. Vollständige Artefakt-/Logauswertung und eine selbst durchgeführte Windowsausführung werden nicht behauptet. Es wurde kein neuer Actions-Lauf gestartet.

Alle folgenden Zeilenangaben beziehen sich auf **6879d829afabbe5d21be2fea17d58b018f77d5cc**. Sie sind nach späteren Reparaturen gegebenenfalls verschoben. **„Quellnachweis“ bedeutet eine konkrete, anhand der Implementierung nachvollziehbare Fehlerfolge – keine neu ausgeführte Windows- oder DAW-Reproduktion.**

Statusmaßstab:

| Status | Bedeutung |
|---|---|
| PASS, benannter Fall | Geeigneter vorhandener erfolgreicher Test am geprüften SHA und zusätzlich nachvollzogene Implementierung. Keine allgemeine Funktionsgarantie. |
| FAIL durch Quellnachweis | Konkrete Eingabe oder zulässige Aufrufreihenfolge führt durch einen nachweisbar fehlerhaften Produktpfad. Die Laufzeitnachweisgrenze wird genannt. |
| TEILWEISE | Teilfunktion ist implementiert oder geprüft; die umfassendere Zusage ist damit nicht belegt. |
| OFFEN / NICHT NACHGEWIESEN | Geeigneter Ausführungsnachweis fehlt, insbesondere echte Host-/ASIO-Bedienung. |

## 2. Befunde nach Priorität

### FR01 – HIGH – STOP kann eine bereits vorgemerkte Vorhörwiedergabe nicht zuverlässig abbrechen

**Status: FAIL durch Quellnachweis. Native Gegenprobe fehlt.**

**Datei:** [src/vst3/DrumatorProcessor.cpp](https://github.com/challanger2000/125A-Drumator/blob/6879d829afabbe5d21be2fea17d58b018f77d5cc/src/vst3/DrumatorProcessor.cpp#L153-L156), Zeilen **153–156, 860–861 und 1095–1099**.

**Ursache:** Start und Stop sind getrennte atomare Flags. Der Audiothread übernimmt das Startflag früh in eine lokale Variable:

```cpp
const bool fireAudition = auditionStart_.exchange(false, ...);
```

Wesentlich später konsumiert er das Stopflag, setzt gegebenenfalls die Voices zurück und startet anschließend trotzdem die lokal vorgemerkte Wiedergabe.

**Konkrete zulässige Reihenfolge:**

1. PREVIEW steht an.
2. Audiothread liest `fireAudition=true`.
3. UI sendet STOP: Start=false, Stop=true.
4. Audiothread konsumiert Stop=true und setzt die Vorhörengine zurück.
5. Weil seine lokale Variable weiterhin true ist, startet er die Vorhörstimme.
6. Beide atomaren Flags sind danach false. Im nächsten Block liegt kein Stop mehr vor.

Das ist **nicht bloß die normale Verzögerung eines UI-Kommandos bis zum nächsten Audioblock**: Das Stopkommando wurde bereits verarbeitet und anschließend durch eine ältere lokale Startanforderung überholt.

**Auswirkung:** Nach STOP kann Preview weiterlaufen. Derselbe Mechanismus kann einen beim Schließen beziehungsweise Abbrechen gesendeten Preview-Stop betreffen.

**Empfehlung:** Start, Stop und Sourcegeneration über einen gemeinsamen geordneten Kommandostand verbinden. Eine spätere Stopgeneration muss einen früher gelesenen Start ungültig machen. Keine Audiothreadlocks einführen.

**Fehlender Regressionstest:** Tatsächlichen Processor nach Übernahme des Startflags pausieren, über den echten Controller STOP senden, anschließend rendern. Aktueller und folgender Block dürfen die verworfene Preview nicht weiterführen. Zusätzlich CANCEL und Editor-Schließen prüfen.

**Bestehende Abdeckung:** FA23-03 prüft Veröffentlichung nach Audition-Adopt. Dieser andere Zeitpunkt deckt den beschriebenen Stopkonflikt nicht ab.

### FR02 – HIGH – OOM-Schutz ist außerhalb des reparierten Waveformquery weiterhin unvollständig

**Status: FAIL durch Quell- und SDK-Nachweis. Kein tatsächlicher Hostabsturz behauptet.**

**Dateien:**

- [src/vst3/DrumatorController.cpp](https://github.com/challanger2000/125A-Drumator/blob/6879d829afabbe5d21be2fea17d58b018f77d5cc/src/vst3/DrumatorController.cpp#L785-L800), insbesondere **793–800, 853–892**.
- Gepinntes SDK `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`: `public.sdk/source/vst/hosting/hostclasses.cpp`, **221–226 und 294–299**.

**Ursache:** Der Processor fängt Speicherfehler beim binären `SampleEditorQuery` inzwischen korrekt ab. Andere Teile derselben Bedienkette erzeugen jedoch Nachrichtenattribute ohne entsprechende Ausnahmegrenze.

Beispielsweise erzeugen `commitSampleEdit` und `previewSampleEdit` mehrere Attribute mit `setInt`. Die SDK-Implementierung verwendet dafür:

```cpp
list[aid] = Attribute(value);
```

Ein neuer Map-Eintrag kann allokieren und `std::bad_alloc` werfen. Diese Aufrufe liegen im Controller außerhalb eines passenden Catch-/RAII-Schutzes; die View-Ereignispfade fangen diese Ausnahme ebenfalls nicht.

**Reproduzierbarer Fehlerpfad:** Eine gültige EDIT-/PREVIEW-/APPLY-Aktion ausführen und die Allokation beim ersten neuen Nachrichtenattribut gezielt fehlschlagen lassen. Der Aufruf kann mit einer Exception aus dem Bediencallback entweichen, statt kontrolliert abzulehnen. Manuell gehaltene Nachrichten können auf diesem Weg zusätzlich ihr `release()` verpassen.

**Auswirkung:** Unsichere Fehlerbehandlung bei Speicherdruck. Ob ein bestimmter Host die Exception abfängt oder beendet wird, ist nicht nachgewiesen.

**Empfehlung:** Die gesamte allokierende Nachrichtenkette absichern: Nachrichtenerzeugung, Attribute, Übertragung und Cleanup. Fehler müssen einen definierten Rückgabewert beziehungsweise sichtbaren UI-Fehler liefern und den alten Zustand erhalten.

**Fehlender Regressionstest:** Allokationsinjektion entlang der tatsächlichen Controller-/View-Aktionen, nicht ausschließlich in einem vorbereiteten Processorquery. Mehrere Failpoints bis zum erfolgreichen Abschluss durchlaufen und Zustand sowie Ressourcenbesitz prüfen.

**Abgrenzung:** Der ursprüngliche FA05-Queryfehler ist repariert und FA23-10 besteht. Das beweist nicht die Ausnahmesicherheit aller anderen Nachrichtenerzeuger.

### FR03 – MEDIUM – Nach Legacy-Recall kann EDIT für eine tatsächlich leere Zeile aktiviert bleiben

**Status: FAIL durch Quellnachweis.**

**Dateien:**

- `src/vst3/DrumatorController.cpp`, **283–336**, insbesondere **311–322**.
- `src/vst3/DrumatorController.h`, **38 und 151**.
- `src/vst3/DrumatorView.cpp`, **638 und 1515–1517**.

**Ursache:** Das neu eingeführte `samplePresent_` wird beim aktuellen Kit-Recall, Import und Clear aktualisiert. Der erfolgreiche Legacy-Recall für DMR1/DMR2/DMR3 setzt dagegen Namen, Pfade und weitere Zeilenwerte zurück, **aber nicht dieses Array**.

**Gegenbeispiel:**

1. WAV in Row 0 laden: `samplePresent_[0]` wird true.
2. Ein altes Gridprojekt ohne eingebettete Samples restaurieren.
3. Processor besitzt danach für diese Zeile kein WAV.
4. Controller meldet weiterhin `hasSample(0)==true`.
5. EDIT wird aktiviert dargestellt; ein Klick scheitert erst beim Snapshot mit „NO SAMPLE TO EDIT“.

**Auswirkung:** Controller, GUI und tatsächlicher Samplebestand stimmen nach einer unterstützten Migration nicht überein. Der Snapshot verhindert hier eine Bearbeitung leerer Daten; ein Speicherzugriffsfehler ist damit nicht belegt.

**Empfehlung:** Samplepräsenz in jedem erfolgreichen Legacy-Commit vollständig auf den migrierten Zustand setzen.

**Fehlender Regressionstest:** Bereits geladene Instanz → jedes unterstützte frühe Format restaurieren → tatsächliches Processor-WAV, `hasSample`, EDIT-Darstellung und Klickverhalten gemeinsam prüfen. Der vorhandene Test einer frisch leeren Instanz findet diesen Folgefehler nicht.

### FR04 – MEDIUM – Die gezeichnete Wellenform und ihr Klickbereich stimmen nicht überein

**Status: FAIL durch konkrete Geometrieprüfung.**

**Dateien:**

- `src/vst3/DrumatorView.cpp`, **859–899 und 1173–1200**.
- [src/core/UiWaveEditorLayout.h](https://github.com/challanger2000/125A-Drumator/blob/6879d829afabbe5d21be2fea17d58b018f77d5cc/src/core/UiWaveEditorLayout.h#L13), **13**.

**Ursache:** Der neue Wellenformbereich wird von **y=155 bis 382** gezeichnet. Die zentrale Hitdefinition beginnt weiterhin bei **y=175**.

**Gegenbeispiel:** Im SAMPLE EDITOR bei logischem Punkt `(500,165)` auf den sichtbaren Signalbereich klicken beziehungsweise ziehen. Dieser Punkt gehört zur sichtbaren Wellenform, erreicht aber den Marker-/Fade-Ereignispfad nicht.

**Auswirkung:** Ein 20 Pixel hoher sichtbarer Streifen reagiert anders als der übrige Signalbereich. Bei Stereo liegt dieser Streifen innerhalb des oberen Kanalpanels.

**Empfehlung:** Signalpanel, Marker-, Fade- und Mausgeometrie aus derselben Definition ableiten. Falls ein Bereich absichtlich nicht interaktiv sein soll, muss er sichtbar als eigener Bereich gestaltet werden.

**Fehlender Regressionstest:** Echte View-Ereignisse entlang aller vier Wellenformränder und an inneren Punkten bei 100/120/130/150 %. Der bestehende Testpunkt `(400,250)` liegt weit innerhalb beider Rechtecke und erkennt diese Abweichung nicht.

### FR05 – MEDIUM – Live-Fade-Darstellung entspricht am Fade-Out-Ende nicht der DSP-Hüllkurve

**Status: FAIL durch Formelvergleich.**

**Dateien:**

- `src/vst3/DrumatorView.cpp`, **876–899**.
- `src/core/SampleEdit.h`, **26–33**.

**Ursache:** Die Darstellung verwendet für Fade Out:

```cpp
(end - sourceFrame) / fadeOut
```

Der DSP verwendet für ganzzahlige Sourceframes:

```cpp
(end - 1 - sourceFrame) / fadeOut
```

Die Grafik bildet damit nicht denselben Endpunktvertrag ab.

**Konkretes Gegenbeispiel:** Eine Einframe-Auswahl mit Fade Out=1 ist gültig. Der DSP ergibt für ihren einzigen Frame Amplitude 0. Die aktuelle grafische Berechnung zeigt über die Auswahl dagegen eine fallende, teilweise deutlich von null verschiedene Signalfläche.

Auch bei längeren Samples ist der letzte Frame im DSP null, während die gezeichnete Formel unmittelbar davor noch einen positiven Faktor liefert.

**Auswirkung:** Die als live dargestellte Bearbeitung kann hörbare beziehungsweise stumme Ergebnisse falsch vermitteln. Das ist besonders bei sehr kurzen Samples und starker Vergrößerung relevant.

**Empfehlung:** Eine gemeinsame definierte Hüllkurvenauswertung für DSP und Anzeige verwenden. Für Envelopebins festlegen, wie die Hüllkurve über deren tatsächliche Sourceframes abgebildet wird.

**Fehlender Regressionstest:** Anzeige und PCM für Einframe-, Zweiframe- und kurze Auswahlen, Fade 0/1/volle Länge sowie verschiedene Zoomstufen vergleichen. Ein allgemeiner Test „Bitmap ist nicht leer“ genügt nicht.

### FR06 – MEDIUM – Bestimmte WAV-Editorfehler sind erneut unsichtbar

**Status: FAIL durch Ereignis- und Zeichenpfad.**

**Datei:** `src/vst3/DrumatorView.cpp`, **975–980, 1091–1094 und 1157–1158**.

**Ursache:** Der Modalstatus wird nur gezeichnet, wenn der Text bestimmte Teilstrings enthält, beispielsweise `FAILED`, `REJECTED`, `ERROR`, `NOT ` oder `PREVIEW`.

Die tatsächlichen Fehlermeldungen

- `ZOOM: WAVEFORM UNAVAILABLE`
- `SCROLL: WAVEFORM UNAVAILABLE`

enthalten keinen dieser Filterbegriffe.

**Gegenbeispiel:**

1. WAV-Editor öffnen.
2. In einer zweiten View das Sample entfernen.
3. Im alten Editor ZOOM oder Scroll ausführen.
4. Snapshot scheitert und setzt einen passenden Fehlertext.
5. Das Modal zeichnet diesen Fehler nicht. Die normale Footerzeile liegt hinter dem Modalfenster.

**Auswirkung:** Die Bedienung reagiert scheinbar nicht, obwohl eine konkrete Ablehnung vorliegt. Die vorherige Reparatur des verdeckten Modalstatus ist durch die neue Textfilterung nur noch teilweise wirksam.

**Empfehlung:** Typisierten Status verwenden, beispielsweise Fehler/Information/Fortschritt, statt Sichtbarkeit aus englischen Wortfragmenten abzuleiten. Fehler immer innerhalb des aktiven Modals anzeigen.

**Fehlender Regressionstest:** Tatsächliches Queryversagen über Controller/Processor erzeugen und sichtbare Fehlerpixel beziehungsweise ein echtes Modalstatus-Control prüfen. Ein Vergleich von `status_` allein findet die Verdeckung nicht.

### FR07 – MEDIUM – Die Oberfläche ist weiterhin nicht durchgehend englisch

**Status: FAIL durch direkte Quellstellen.**

**Datei:** `src/vst3/DrumatorView.cpp`, **673–676 und 750–761**.

Im aktiven Song-Modus werden weiterhin unter anderem gezeichnet:

- `SONG-ABLAUF`
- `POSITION WAEHLEN, DANACH PATTERN UND LAENGE`
- `< ZURUECK`
- `WEITER >`

Bei alten Projekten mit aktivierter Echtzeit-Velocitymodulation erscheint außerdem `ALT RT`.

**Reproduktion:** SONG EDIT öffnen; für die Legacyanzeige ein Projekt mit `randomVelocity[row] > 0` laden.

**Auswirkung:** Die ausdrücklich verlangte englische Bedienoberfläche ist nicht vollständig umgesetzt. Die genannten Songtexte sind aktive Zeichenpfade, keine Kommentare oder historischen Dokumente.

**Empfehlung:** Sichtbare Strings in einem gemeinsamen englischen Katalog verwalten und auch Song-, Legacy-, Fehler- und dynamisch zusammengesetzte Texte einbeziehen.

**Fehlender Regressionstest:** Vollständiger Stringkatalogtest plus Render-/Bedienfälle für Song und Legacyzustände. Einzelne geänderte Defaultlabels reichen nicht.

### FR08 – MEDIUM – Akzeptierte lange Dateipfade können die Controller-Speicherung verhindern

**Status: Quellseitiger Grenzwertkonflikt; Windows-Langpfad-Reproduktion nicht ausgeführt.**

**Datei:** `src/vst3/DrumatorController.cpp`, **108–122 und 1044–1117**.

**Ursache:** Der Import übernimmt einen erfolgreich gelesenen vollständigen Dateipfad ohne dieselbe Längenbegrenzung, die später für den Controllerstate gilt. `getState` lehnt jedoch jeden Pfad mit mehr als **4.096 UTF-8-Bytes** ab.

**Gegenbeispiel:** Ein gültiges WAV über einen von Windows unterstützten langen, beispielsweise mit `\\?\` adressierten Pfad importieren. Der Pfad kann aus mehreren gültigen Verzeichnisnamen bestehen und über 4.096 UTF-8-Bytes lang sein, während Dateiname und Datei selbst innerhalb aller Samplegrenzen liegen.

Wenn der Dateizugriff gelingt, akzeptiert der Import diesen Zustand. Anschließend liefert `Controller::getState` wegen der Pfadlänge `kResultFalse`.

**Auswirkung:** Der Processor kann das eingebettete Audio weiterhin speichern, der Controllerstream mit Zoom/Herkunft schlägt jedoch fehl. Das ist kein erneut nachgewiesener Totalausfall der Processorserialisierung wie der frühere FA01.

**Empfehlung:** Import und Stateformat auf dieselbe unterstützte Pfadgrenze abstimmen. Alternativ Herkunft optional und verlustfrei behandelbar speichern, ohne wegen einer Dateireferenz den gesamten Controllerstate abzulehnen. Keine stille Pfadabschneidung.

**Fehlender Regressionstest:** Windows-Dateien an und über der Grenze, einschließlich mehrbytefähiger UTF-8-Pfade: Import → beide echten Streams speichern → frischer Recall → Navigation prüfen.

### FR09 – MEDIUM – Der Modal-D&D-Regressionstest prüft nach dem Layoutwechsel kein gültiges Dropziel mehr

**Status: Nachgewiesene Testlücke, kein Beweis eines aktuell defekten Modalguards.**

**Dateien:**

- [tests/integration/FinalAudit20261023.inl](https://github.com/challanger2000/125A-Drumator/blob/6879d829afabbe5d21be2fea17d58b018f77d5cc/tests/integration/FinalAudit20261023.inl#L47-L62), **47–62**.
- `src/core/UiCompactLayout.h`, **14–18**.
- `src/vst3/DrumatorView.cpp`, **357–363**.

**Ursache:** FA23-04 droppt weiterhin bei `(250,128)`. Nach Einführung des direkten EDIT-Buttons endet das eigentliche Sample-Dropziel aber bereits bei **x=205**. Bei x=250 liegt EDIT.

Der Testdrop wird deshalb auch bei geschlossenem Modal abgelehnt.

**Konkreter Nachweis:** Selbst ohne den Ausdruck `sampleEditorOpen_` in der Modalsperre bleibt für x=250 die Bedingung

```cpp
x >= sample.left && x < sample.right
```

falsch. Der Test kann somit eine entfernte WAV-Modalsperre nicht mehr zuverlässig erkennen.

**Auswirkung:** Das aktuelle PASS von FA23-04 belegt den ursprünglich behaupteten Schutz nicht mehr. Der Produktguard ist im Quellcode vorhanden; seine Regression wird durch diesen Testpunkt nicht abgesichert.

**Empfehlung:** Zuerst einen echten positiven Drop auf ein gültiges Sampleziel ohne Modal nachweisen. Danach denselben Punkt mit geöffnetem Modal prüfen, beispielsweise `(150,128)`. Ein gezielt entfernter Modalguard muss den Test rot machen.

**Fehlende Negativkontrolle:** Genau dieser Mutationsnachweis. Ich habe weder den Guard verändert noch dafür einen Build gestartet.

### FR10 – LOW – Globale Regler verwenden unterschiedliche Zeichen- und Hitmittelpunkte

**Status: FAIL durch Koordinatenvergleich.**

**Dateien:**

- `src/vst3/DrumatorView.cpp`, **597–601 und 1350–1352**.
- `src/core/UiCompactLayout.h`, **20**.

**Ursache:** Gezeichnet werden Swing/Humanize/Master bei x=**80/178/1178**. Die Mausbehandlung verwendet aus dem Layout x=**78/176/1180**.

**Gegenbeispiel:** Ein Punkt am rechten sichtbaren Swing-Reglerrand, beispielsweise x=96,5/y=76, liegt noch innerhalb des gezeichneten Kreises, aber außerhalb des Hitbereichs mit `x<96`.

**Auswirkung:** Kleine tote beziehungsweise zusätzliche unsichtbare Randbereiche. Kein großer Funktionsausfall, aber ein belegtes Beispiel für weiterhin doppelte Layoutdefinitionen.

**Empfehlung:** Zeichen- und Ereignispfad aus derselben Reglergeometrie erzeugen. Sinnvoll vergrößerte Trefferflächen dürfen davon abweichen, müssen aber bewusst symmetrisch definiert sein.

**Fehlender Regressionstest:** Randpunkte aller globalen Regler in tatsächlichen View-Callbacks prüfen; nicht nur die zentrale Position.

### FR11 – LOW – Die WAV-Anleitung beschreibt weiterhin entfernte oder geänderte Bedienelemente

**Status: FAIL; ursprünglicher Dokumentationsbefund nicht vollständig erledigt.**

**Datei:** `docs/WAV-EDITOR-IMPLEMENTATION.md`, insbesondere **5, 15–24**.

Die aktuelle Datei beschreibt weiterhin:

- WAV EDITOR im Samplemenü statt direktem EDIT.
- Fadebuttons statt Fadegriffen.
- AUTO START, obwohl die aktuelle Toolbar diese Aktion nicht mehr enthält.
- OK und deutsche Bedienbezeichnungen.
- Fehlendes Scrollen, obwohl ein Scrollbalken vorhanden ist.

**Nachweis:** Die Datei ist gegenüber dem zuvor auditierten Stand byteidentisch. Der Reparaturbericht nennt FA14 dagegen als bearbeitet.

**Auswirkung:** Falsche Bedienungs- und Abnahmeanweisungen; Tester können Aktionen suchen, die nicht mehr existieren.

**Empfehlung:** Anleitung und Abnahmeschritte anhand des tatsächlichen aktuellen Bedienpfades aktualisieren. Historische Berichte klar als solche kennzeichnen.

**Erforderliche Prüfung:** Dokumentreview gegen die aktuelle Kontrollinventarliste.

## 3. Bewertung der früheren FA01–FA14-Reparaturen

Die alten Befunde werden nicht pauschal erneut als offen geführt:

| Früherer Befund | Unabhängiges aktuelles Ergebnis |
|---|---|
| FA01: Bearbeitetes Sample leeren macht Processorstate unspeicherbar | **Behoben.** Clear setzt jetzt auch `SampleEdit{}`; FA23-01 PASS. |
| FA02: Alter WAV-Entwurf wird auf neue Source angewendet | **Ursprünglicher Gegenfall behoben.** View übergibt Revision, Processor prüft sie; FA23-05 PASS. |
| FA03: Previewanforderung geht nach Adopt verloren | **Benannter Gegenfall behoben**, FA23-03 PASS. Der neue Stopkonflikt FR01 bleibt. |
| FA04: Wiederholtes Preview stapelt Stimmen | **Behoben.** Vor Neustart wird Audition zurückgesetzt; FA23-02 PASS. |
| FA05: OOM beim binären Processorquery entweicht | **Dieser Pfad behoben**, FA23-10 PASS. Weitere Controllerpfade: FR02. |
| FA06: WAV-Modal lässt Sampledrops durch | **Guard implementiert.** Regressionstest nach Geometrieänderung unzureichend: FR09. |
| FA07: Herkunft geht nach unpaarigem Restorezyklus verloren | **Benannter Gegenfall behoben**, FA23-06 PASS. Keine universelle Zusage für alle Hosts/Streamfolgen. |
| FA08: Undo mischt Gestendaten und alte Länge | **Behoben.** Geste zuerst beenden, unveränderliches Steps-/Längentupel erfassen; FA23-07 PASS. |
| FA09: SRC liest außerhalb der Trimauswahl | **Behoben.** Beide Faltungspfade beachten Editgrenzen; FA23-08 PASS. |
| FA10: Audition überlebt Deaktivierung | **Benannter Gegenfall behoben.** Pending-Start wird verworfen und Stop vorgemerkt; FA23-09 PASS. |
| FA11: Reset-Fadegriffe unsichtbar | **Quellseitig behoben.** Randgriffe werden innerhalb des Panels gezeichnet; umfassender Pixel-/Mausnachweis fehlt. |
| FA12: Float-WAV zeichnet außerhalb des Panels | **Quellseitig behoben.** Plotkoordinaten werden begrenzt. |
| FA13: Modalfehler verdeckt | **Nur teilweise.** Eigenes Modalstatusfeld vorhanden, aber FR06 filtert reale Fehler wieder heraus. |
| FA14: Veraltete WAV-Anleitung | **Weiterhin offen**, siehe FR11. |

Auch die ursprünglichen RE01–RE11- und RM01–RM13-Fälle bestehen im aktuellen Lauf. Dazu gehören der deterministische VFA02, Peer-FILL/Cancel, reentrantes Speichern und der kalte SONG-Fallback. Diese früheren konkreten Fehler sind damit nicht erneut als aktuelle FAILs zu zählen.

## 4. Technische Bewertung der übrigen Funktionsbereiche

### 4.1 Audioengine und Echtzeitsicherheit

Die normale Engine besitzt feste 64-Voice-Kapazität. Sampledaten und SRC-Kerne werden auf der Kontrollseite vorbereitet. Der Audiopfad übernimmt unveränderliche Assets und hält deren Lebensdauer über Ownerreferenzen aufrecht. Maßgeblich sind `src/core/SampleVoiceEngine.h`, `src/core/SampleExchange.h` und ihre Aufrufer in `src/vst3/DrumatorProcessor.cpp`.

Positiv nachvollziehbar:

- Keine WAV-/FLAC-Decodierung im normalen Audiocallback.
- Keine dynamische Voiceallokation im Renderloop.
- Alte Samples bleiben für noch aktive Voices gültig.
- Freigabe zurückgegebener Assets erfolgt außerhalb des Audiothreads.
- Choke wird erst angewendet, nachdem ein freier Voiceplatz gefunden wurde.
- Ein kapazitätsbedingt abgelehnter Hit würgt bestehende Voices nicht ab.
- Trimgrenzen werden inzwischen auch bei Sinc-Interpolation eingehalten.
- Unity-Rate-Wiedergabe verwendet den direkten Samplepfad.
- Mastergain besitzt eine sampleratenabhängige Rampe.

Wichtige bestehende Semantik:

- Note-off beendet die One-Shots nicht.
- Gain, Pan, Choke und Ausgang werden beim Voice-Launch übernommen.
- Bereits klingende Tails behalten ihre kopierten Werte; Mute/Solo filtert neue Trigger.
- Bei ausgeschöpfter Kapazität werden weitere Hits abgelehnt.
- WAV-Audition ist eine separate Engine und wird MAIN vor Master zugemischt.

Diese Eigenschaften müssen bewusst als Produktverhalten abgenommen werden. Ein langer importierter Loop erhält dadurch keine automatische Tempoanpassung oder Time-Stretch-Funktion.

**Offen:** Gesamte Echtzeitqualifikation auf Zielhardware, Denormalstress, vollständiges Heap-/Lock-/Pagefault-Tracing und hörbare Übergangsprüfung. Lockfreie atomare Operationen allein garantieren keine maximale Callbackdauer; insbesondere sind CAS-Wiederholungen kein allgemeiner Wait-free-Nachweis.

### 4.2 Multi-Out

Die Implementierung bietet MAIN plus 15 zusätzliche Stereoausgänge. Rows dürfen denselben Bus verwenden; die Engine summiert dort.

Für die benannten Fälle sind belegt:

- Unterschiedliche Stereocodes auf allen zusätzlichen Ausgängen.
- Erkennung von Nachbarbus- und L/R-Vertauschungen.
- AUX-Ausgabe bei MAIN mit null Kanälen.
- Keine doppelte MAIN-Ausgabe im geprüften Routingfall.
- Speicherung und Wiederherstellung der Buszuordnung.

T02 und T29 prüfen tatsächliche paketierte Pluginpfade. Die Registrierung zusätzlicher Busse als `kMain` statt `kAux` ist im Code bewusst für die bisherige Studio-One-Kompatibilität vorgenommen.

**Nicht nachgewiesen:** Tatsächliche Aktivierung aller Busse und Mixerwiederherstellung in Studio One/Studio Pro.

### 4.3 Sequencer, MIDI und Song

Die aktuellen Pfade enthalten:

- Acht Patterns mit 16/32 Steps.
- Erhalt der Steps 17–32 bei vorübergehendem Einbarbetrieb.
- Gemeinsame Generation für Patterninhalt und Länge.
- Gültigen vollständigen Cache als Fallback bei konkurrierender Veröffentlichung.
- Begrenzte Songchain mit 64 Einträgen.
- PPQ-basierte Planung, Swing und deterministisches Humanize.
- MIDI-Patternwahl und Hostautomation mit definierter Priorität bei gleichem Offset.
- Zero-Frame-Parameterverarbeitung und Generationsschutz gegen Restore/ABA.

T01, T28, T30, RE01/RE03/RE11 und die O01-Fälle liefern dafür konkrete Nachweise. T28 prüft insbesondere Step 1 bei PPQ 4, also Start ab Takt 2, sowie unterschiedliche Blockpartitionen.

Transportstarts und erkannte Positionssprünge setzen die Patternphase bewusst neu. Normale fortlaufende Blöcke behalten dieselbe Phase.

**Offen:** Vollständige reale DAW-Kombination aus Tempoautomation, Loopgrenzen innerhalb von Blöcken, Locate, Sampleimport und hoher Stimmenlast. Die vorhandenen Tests sind wesentlich stärker geworden, bilden aber nicht jede Hostzeitlinie ab.

### 4.4 Sampleverwaltung und Projektzustand

Direkt importierte WAV-Bytes bleiben eingebettet; Bearbeitungen werden als Metadaten gespeichert. FLAC wird dagegen in Float-WAV konvertiert. „Originaldatenerhalt“ bedeutet dort nicht Erhaltung des ursprünglichen FLAC-Containers.

Vorhanden sind:

- Begrenzte Datei-, Frame- und Kitgrößen.
- Validierung unterstützter RIFF-/PCM-/Floatformate.
- Ablehnung ungültiger Werte und nichtendlicher Floatdaten.
- Vorbereitungsphase vor erfolgreichem Statecommit.
- DM16-Speicherung und Migration älterer Formate.
- Schutz alter Daten bei den geprüften abgelehnten Import-/Recallfällen.

Der vollständige Kit-Recall benutzt ein Gate: Der Audiothread wartet nicht auf den Kontrollmutex, sondern liefert während gesperrter Verarbeitung Stille. Das schützt Konsistenz, ist aber **kein unterbrechungsfreier Recallvertrag**.

Der aktuelle T04-Nachweis protokolliert 24 X/Y-Restores mit 41 kohärenten X-, 41 Y- und **27.294 gatebedingt stillen Blöcken**. Diese stillen Blöcke dürfen nicht als Aussetzerfreiheit umgedeutet werden.

### 4.5 VST3 und Synchronisation

Factory-/Controllerzuordnung, Drum-Instrumentklassifizierung, MIDIinput, Float32-Verarbeitung und feste Stereoausgänge sind vorhanden. Float64 wird bewusst abgelehnt.

**Hostautomatisierbarer Parameter ist Pattern, ID 1000.** Andere Row-, Master-, Swing-, Humanize- und Editwerte werden über eigene Nachrichten und Kitstate verwaltet. Eine Zusage „sämtliche Regler sind hostautomatisierbar“ wäre deshalb nicht zutreffend.

Die aktuellen Tests bestätigen die ursprünglichen Fälle für:

- MIDI-/Hostarbitration.
- Mehr als 256 Patternbefehle.
- Controllerfeedback während Restore.
- Equal-/Different-/ABA-Situationen.
- Reentrantes Speichern beim Host-Dirty-Callback.
- Native Editor- und Namenseditorlebenszyklen.

Ein verbleibendes **Kompatibilitätsrisiko** ist das Queryverfahren: Der Processor schreibt Antworten synchron in Attribute der eingehenden Nachricht. Das funktioniert im direkten Testhost. Für einen entkoppelnden beziehungsweise verteilten Hostproxy ist damit noch nicht nachgewiesen, dass Änderungen am Anfrageobjekt zum Sender zurückgelangen. Das ist hier kein behaupteter Studio-One-Fehler.

## 5. GUI-Kontrollmatrix

Die Prüfung umfasst alle Wiederholungen der genannten Kontrollfamilien: **Patterns A–H, Rows 0–15, Steps 0–31 und Songslots 0–63**. Sie ist eine Quell-/Evidenzmatrix, kein behaupteter physischer Klicktest sämtlicher Kombinationen.

Die aktive Oberfläche ist ein Custom-`DrumatorView`, keine automatisch verdrahtete Sammlung von UIDESC-Controls. Leere Ressource-`control-tags` sind deshalb nicht allein ein Fehler. Der native Namenseditor benutzt seine eigene Callback-/Generationsführung; Patternautomation ist davon unabhängig.

| Kontrolle | Tatsächliche Funktionszuordnung | Bewertung |
|---|---|---|
| Pluginname | Zentriertes Textrechteck `(465,5)–(815,35)`, Mittelpunkt x=640 | Quellseitig korrekt zentriert |
| Originales 125A-Logo | 30 native Vektorpfade aus dem Brandingmaster | Herkunft und Geometrie bestätigt |
| 1 BAR / 2 BARS | Gemeinsame X-Geometrie, `togglePatternLength`, Processorpatternlänge | Benannte Tests PASS |
| Pattern A–H | Hostparameter 1000, Controllerwahl, Processortimeline | Benannte Tests PASS |
| Patternmenü | Copy, Paste, Clear, Undo, Random Velocity, Velocity 100, Duplicate | Zuordnung vorhanden; alte Undo-Reparatur bestätigt |
| SONG ON/OFF | Controller-Songconfig → Mailbox → Scheduler | Teilfälle PASS |
| SONG EDIT / PATTERN | Öffnen/Schließen des Songboards und Resize | Pfad vorhanden; reale Hostbedienung offen |
| Swing/Humanize/Master | Controllerkommandos → Scheduler beziehungsweise Masterrampe | Verdrahtet; FR10 |
| Zoommenü 100/120/130/150 % | Geprüfter Editorzoom und gespeicherte Controllerpräferenz | Native Teilfälle PASS; vollständige Bedienmatrix offen |
| Rowauswahl | Lokale Instrument-/Velocityauswahl | Verdrahtet |
| Instrumentname | Nativer Texteditor, Generation, Rename-Nachricht | Z03-Fälle PASS |
| Samplefeld | Filepicker; zusätzliche Rename-/Clearmodifikatoren | Verdrahtet |
| Direkter EDIT je Row | `hasSample` → Embedded-Snapshot → Modal | Vorhanden; Legacyfehler FR03 |
| Samplemenü | Load Sample, Rename Row, Remove Sample | Aktuelle Zuordnung vorhanden |
| Sample `<` / `>` | Sortierte Ordnernavigation mit Umlauf | N12-Fälle PASS |
| Row-PLAY | PreviewRow → normale Triggerengine | Eigener Scope, nicht WAV-Audition |
| FILL, neun Aktionen | `fillRow` → Patternreplace | Aktuelle Menüfälle PASS |
| Mute/Solo | RowGate → Triggerfilter | Verdrahtet; Tails behalten Launchzustand |
| Output, 16 Ziele je Row | RowOutput → Voicebus | T02/T29 und Zustandsfälle PASS |
| Choke, Gruppen 0–4 | RowChoke → kapazitätsgeprüfter Choke | Benannte Fälle PASS |
| VOL/PAN/PITCH | RowMix beziehungsweise vorbereitetes Pitchasset | Verdrahtet; bestehende Präzisionsfälle PASS |
| Random Velocity / Legacy RT | Einmalige Patternänderung beziehungsweise gespeicherte alte RT-Funktion | Beide Pfade vorhanden; Legacylabel nicht englisch |
| 32 Stepbuttons je Row/Pattern | Toggle-/Velocityaktion → echte Patternbank | Verdrahtet; versteckte Tailsteps geschützt |
| Velocityexpand und Balken | Zeilenlayout, Owner-/Generationsgeste, StepEdit, Undo | Benannte Fälle PASS |
| Add/Remove Row | Sichtbarkeitsänderung mit Resize-/Rollbackpfad | Vorhanden; kein Löschen belegter letzter Row |
| Rowseiten und Bereichstext | Lokale Seitenwahl mit echten Rowindizes | Vorhanden |
| Songslots, Add, Seiten | Gemeinsame Songgeometrie und Controllerchain | Verdrahtet; Sprachbefund FR07 |
| Songpattern und 8/16/32-Längen | `setSongEntry` → gespeicherte Chain | Verdrahtet |
| WAV START/END | Lokaler Entwurf → revisionsgeprüftes Preview/Apply → DSP | FR04 betrifft Trefferbereich |
| WAV Fadegriffe | Entwurf mit begrenzter Fadesumme → SampleEdit-Hüllkurve | DSP vorhanden; FR05 betrifft Darstellung |
| WAV Gain 0–400 % | Entwurf, validierte Floatübertragung, Preview/Apply | Modell/DSP vorhanden; vollständige UI-Extremmatrix offen |
| WAV Zoom/Fit/Scroll | Neuer sichtbarer Bereich, erneuter Embedded-Query | Vorhanden; FR06 und Aufwandrisiko |
| WAV Preview/Stop | Separate Auditionengine | Neustart repariert; Stoprace FR01 |
| WAV Reset | Entwurf auf Vollsample/Fades 0/Gain 1 | Vorhanden |
| WAV Cancel | Previewstop, Entwurf schließen | Vorhanden; FR01 betrifft Stopordnung |
| WAV Apply | Revisionsprüfung, vorbereitetes Asset, gespeicherter Edit | Ursprünglicher Stale-Commit geschützt |
| Modalstatus | Eigenes Fehlerfeld | Unvollständig, FR06 |

### 5.1 Brandingprüfung

Der eingebundene SVG-Master `resources/branding/125A_Logo_Master_FINAL.svg` besitzt denselben Git-Blobhash wie das aktuelle Original aus `125A-Branding`:

`bd398ffa3b728b1a847bb519fad8168222bccb48`

Zusätzlich wurden alle **30 Pfadgeometrien und ihre Farben** mit `src/vst3/BrandLogoGeometry.h` verglichen: Sie stimmen überein. Die Darstellung in `src/vst3/DrumatorView.cpp:54–90` skaliert proportional mit `32/887`.

Damit ist die Behauptung eines improvisierten oder neu gezeichneten Logos hier **nicht gerechtfertigt**. Lesbarkeit des sehr kleinen Logos auf einem konkreten Monitor bleibt eine visuelle Abnahmefrage.

### 5.2 Buttonfamilien und Wiederverwendbarkeit

Teilweise umgesetzt:

- Ein gemeinsamer Buttonpainter.
- Gemeinsame Geometrie für BAR und direkten EDIT.
- Gemeinsame Zeichen-/Hitdefinition für die acht WAV-Toolbarbuttons.
- Einheitliche Größen innerhalb mehrerer WAV-Buttonpaare.

Noch nicht durchgehend umgesetzt:

- Ein gemeinsames Action-/Controlmodell für Zeichnung, Enabledzustand und Ereignisdispatch.
- Einheitliche gedrückte beziehungsweise Hoverzustände aller Buttonfamilien.
- Zentral definierte Popup- und Reglergeometrie.
- Vollständige Ablösung historischer Layoutmodelle.

`drawControl()` vereinheitlicht hauptsächlich das Aussehen. Es ist noch kein wiederverwendbares Bedienelement mit gemeinsamem Eingabe- und Zustandsvertrag. Die WAV-Toolbar hat einen zusätzlichen Pressedzustand; viele andere Momentanbuttons nicht.

Das ist überwiegend ein **Architektur- und Bedienverbesserungspunkt**, kein Beweis, dass alle unterschiedlich breiten Buttons fehlerhaft wären. Unterschiedliche Rollen dürfen unterschiedliche Maße haben.

## 6. Zoom, Fenster und Aussagekraft der GUI-Tests

Die logische Breite beträgt 1.280 Pixel. Bei 100 % Host-DPI ergeben sich für 100/120/130/150 % Userzoom entsprechend 1.280/1.536/1.664/1.920 physische Pixel.

Die vorhandenen Zoompfade berücksichtigen:

- Userzoom und Host-DPI.
- Tatsächlichen Frame-Transform.
- Hostablehnung.
- Gespeicherte Zoompräferenz.
- Reopen.
- Ablehnung nicht atomar möglicher Mehrview-Recalls.
- Compact-Paging bei verweigerter Höhenänderung.

**Eine wichtige Testgrenze:** `testVisualRender`, `tests/integration/WindowsIntegration.cpp:611–619`, übergibt die vier Skalierungswerte an `renderBitmapOffscreen`. Es setzt dabei nicht für jeden Bildfall den tatsächlichen Userzoom des Editors.

Diese Bilder prüfen skalierte native Zeichenoperationen. Sie sind **kein vollständiger Nachweis** für:

- tatsächliche VST3-Resizeverhandlung bei jeder Zoomstufe;
- transformierte Mauskoordinaten;
- Capture-/Dragverhalten;
- alle WAV-Modalkontrollen bei allen Zooms.

Separate native Zoomtests existieren und bestehen. Sie schließen diese zusätzliche Bedien-/Pixelmatrix aber nicht vollständig.

Auch `tests/UiCompactLayoutTests.cpp` prüft für die WAV-Toolbar überwiegend `buttonAt()` und Rechtecke. Damit ist nicht automatisch nachgewiesen, dass jeder Button im tatsächlichen View die richtige Controller-/Audiowirkung auslöst.

### 6.1 Instrumentenzahlen und modale Fenster

`src/core/UiCompactLayout.h` verwendet eine gemeinsame Zeilengeometrie mit maximal vier Rows pro Seite, optionalen Velocityhöhen und Paging bis 16 Instrumente. `firstRow_` geht in die tatsächliche Sample-/Step-/Namenszuordnung ein. Einzelne aktuelle Compact-, Paging-, Menu-Gutter- und Mindesthöhenfälle sind nativ geprüft; daraus wird keine vollständige physische Klickkombinatorik abgeleitet.

Bei zu niedriger View fordert der WAV-Editor eine erfolgreiche Erweiterung auf logisch 620 Pixel an. Wenn der Host ablehnt, öffnet er nicht. Ein leeres Sample wird vor der Resizeanfrage abgelehnt. Diese Quellpfade sind vorhanden; tatsächliche WAV-Bedienung bei allen Kombinationen aus 1/4/16 Rows, Velocityhöhen, Zoom und Host-DPI bleibt offen.

## 7. Performance und Audioaussetzer

Aktuelle vorhandene F14-Messung, 64 Voices, 16×, 48 kHz/256 Frames:

| Kennzahl | Wert |
|---|---:|
| p95 | 3,5484 ms |
| p99 | 3,5721 ms |
| Maximum | 3,6410 ms |
| Blockfrist | 5,3333 ms |
| Überschreitungen | 0/180 |

F15, jeweils 32 Blöcke:

- 1×: p99/Maximum 0,0746 ms.
- 4×: 1,8836 ms.
- 16×: 3,5877 ms.

Diese Ergebnisse sind positiv, aber begrenzte Offline-CI-Diagnostik. Bei 32 Messungen ist nearest-rank p99 lediglich das beobachtete Maximum.

**Nicht daraus ableitbar:**

- Garantierte Einhaltung jeder künftigen Audiofrist.
- ASIO-Aussetzerfreiheit.
- Gleiches Verhalten bei 64-Frame-Buffern.
- Einhaltung unter gleichzeitigem Import, GUI-Scrolling, Hostlast und Preview.
- Vollständige Denormal- und Pagefaultfreiheit.

Der Import bleibt synchron und enthält mehrfaches Decodieren/Kopieren. WAV-Zoom und Scroll fordern weiterhin die vollständigen WAV-Daten an und decodieren sie erneut. Bei Dragbewegungen kann dieser Aufwand wiederholt anfallen.

Das ist ein belegter aufwendiger Kontrollpfad. **Ein dadurch tatsächlich verursachter Studio-One-Dropout wurde hier nicht gemessen.**

Außerdem begrenzen 16 MiB Einzeldatei beziehungsweise 64 MiB Kit-Payload nicht den gesamten RAMbedarf. Dekodierte Samples, SRC-Kerne, alte aktive Assets, Nachrichtenkopien und serialisierte Zustände können gleichzeitig existieren.

## 8. Erforderliche Regressionen und Abnahmen

Vor einer neuen Freigabebewertung sind insbesondere nötig:

1. **STOP-Interleaving:** Start bereits lokal übernommen, danach STOP/CANCEL/Close; kein anschließender Neustart.
2. **OOM entlang der gesamten Bedienkette:** tatsächlicher Controller und View, mehrere Allokationspositionen, definierte Ablehnung und unveränderte Daten.
3. **Legacy-Recall in bereits benutzter Instanz:** Samplepräsenz, EDIT, Namen, Processorstate und Rückmeldung zusammen prüfen.
4. **Echte Modalgeometrie:** sichtbare Wellenformränder, Marker, Fadegriffe, Gain und Toolbar bei allen vier Zooms.
5. **DSP-/Darstellungsabgleich:** sehr kurze Mono-/Stereo-Samples, Fade 0/1/volle Länge, Gain 0/100/400 %, unterschiedliche Sampleraten.
6. **Modalfehleranzeige:** kontrollierte Query-/Preview-/Applyfehler müssen tatsächlich sichtbar sein.
7. **D&D-Negativkontrolle:** gültiges Ziel ohne Modal muss angenommen werden; derselbe Drop mit Modal muss scheitern. Entfernen des Guards muss den Test rot machen.
8. **Pfadgrenzen:** Import und beide gespeicherten Streams mit Windows-Langpfaden und UTF-8-Grenzwerten.
9. **Englische UI vollständig:** Song-, Legacy-, Status- und Fehlerzustände einbeziehen.
10. **Reale Studio-One-/ASIO-Abnahme:** Multi-Out, Browser-D&D, DPI/Monitorwechsel, Projekt-Reopen, Loop/Locate/Stop-Play und Import unter Last.

Bestehende RE-/RM-/FA23-Orakel sollten erhalten bleiben. Neue Gegenfälle dürfen nicht durch geänderte Sollwerte oder abgeschwächte Assertions „grün“ gemacht werden. In dieser Berichtsablage wurden keine der hier empfohlenen Tests implementiert oder ausgeführt.

### 8.1 Konkrete Host-Abnahmeszenarien

Alle folgenden echten Praxistests sind **NICHT DURCHGEFÜHRT** und erst an einem eindeutig bezeichneten Reparaturstand abzuarbeiten:

| Praxisfall | Erwarteter Nachweis |
|---|---|
| Bearbeitetes WAV entfernen, speichern, DAW schließen und frisch öffnen | Kein Speicherfehler; übrige Rows, Patterns und Edits erhalten; entfernte Source leer |
| Zwei Views, Sample-/Editwechsel bei offenem altem Entwurf | Keine stille Fremdänderung, sichtbare Ablehnung, Daten erhalten |
| Preview/Stop/Cancel/Close bei Stop und Playing | Richtige einzelne Source, keine verwaiste Wiedergabe und keine Veränderung normaler Pattern-/MIDI-Stimmen |
| WAV-Editor bei 100/120/130/150 % plus Monitor-/DPIwechsel | Sichtbare Griffe, korrekte Trefferflächen, sichtbare Fehler, erreichbares Apply/Cancel |
| Explorer- und DAWbrowser-Drops, WAV/FLAC und Unicodepfade | Richtige Instrumentzeile; gesperrte Ziele und ungültige Pfade verändern keine Source |
| MAIN und alle aktivierten Stems, mehrere Rows pro Bus, save/reopen | Eindeutige Stereoausgabe, keine L/R-Vertauschung oder doppelte MAIN-Ausgabe |
| Start bei PPQ0/4/beliebig, Loop, Locate, Stop/Play und Tempoänderung | Keine verlorenen/doppelten Trigger oder alten Tails; Phase entspricht festgelegtem Vertrag |
| Beide Stateordnungen, Component-only zwischen vollständigen Recalls, Automation Equal/ABA | Sichtbares und hörbares Pattern, Sourceherkunft und gespeicherter Zustand konsistent |
| Reale ASIOhardware, 64/128/256/512 Frames, 64 Voices, SRC/Pitch und paralleler GUI-/Importlast | Fristverteilung, Maxima, Overruns, RAM/UIlatenz und Audioaufnahme dokumentiert; keine bloße Durchschnitts-CPU-Zusage |

## 9. Entscheidung vor der endgültigen GUI-Neugestaltung

**Zwingend technisch zu korrigieren:**

- FR01: geordneter Preview-/Stopvertrag.
- FR02: sichere Allokations- und Ausnahmebehandlung.
- FR03: vollständige Controller-Synchronisation bei Legacy-Recall.
- FR06: verlässliche sichtbare Fehlerzustände.
- FR08: konsistente Import-/Speichergrenzen.

**Vor beziehungsweise zusammen mit der endgültigen GUI zu bereinigen:**

- FR04/FR05: gemeinsame Signal-/Hitgeometrie und korrekte Live-Hüllkurve.
- FR07/FR10: vollständige englische Oberfläche und zentrale Reglerkoordinaten.
- Gemeinsames Bedienmodell für wiederverwendbare Controls.
- FR09: wirksamer D&D-Regressionstest.
- FR11: zutreffende Bedienungs- und Abnahmedokumentation.

**Abschließendes Urteil: V1 technisch noch nicht freigabefähig.** Die alten schweren Gegenfälle sind weitgehend tatsächlich repariert. Der aktuelle erfolgreiche Windows-Lauf belegt jedoch weder den neuen Stopkonflikt noch vollständige Ausnahmesicherheit, GUI-Vertragskonformität oder Aussetzerfreiheit in Studio One.

## 10. Ablage und Schutz des Entwicklungsstands

Diese Ablage ergänzt ausschließlich `docs/CODEX-FINAL-REAUDIT-20261023.md` auf `audit/codex-final-reaudit-20261023`, ausgehend vom tatsächlich geprüften Produktcommit. Kein Produktcode, Test, Asset, Workflow oder bestehender Bericht wird durch diesen Commit geändert; `main`, `develop/v1.0.0` und der Abnahmebranch werden nicht beschrieben oder gemergt.

Die geprüfte `.github/workflows/core-ci.yml:3–13` startet auf Push nur für `develop/v1.0.0` beziehungsweise `fix/studio-one-acceptance-20261022` mit den dort festgelegten Quell-/Test-/Buildpfaden oder durch expliziten manuellen Dispatch. Diese reine Berichtsablage auf dem separaten Auditbranch erfüllt keine dieser Pushbedingungen. Es wird kein Workflowdispatch und kein Pull Request ausgelöst.

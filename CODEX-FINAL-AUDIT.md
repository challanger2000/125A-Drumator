# 125A Drumator – unabhängiger technischer Nach-Audit

**Prüfdatum:** 18.10.2026, UTC.

**Repository / Branch:** `challanger2000/125A-Drumator` / `develop/v1.0.0`.

**Geprüfter Entwicklungscommit:** **`32f68ca7b2c3dc395c5a68efeb991760c373d4ac`**.

**Pluginversion:** `0.1.0`, unverändert. Der spätere Dokumentationscommit dieses Berichts ist kein neuer geprüfter Implementierungsstand.

**Vergleichsgrundlagen:** [TECHNISCHER-AUDIT.md][original], F01–F17, ursprünglicher Quellstand `a64c94a117ae7b485450b9367b7703039b949fa4`; [vorheriger CODEX-FINAL-AUDIT.md][previous], N01–N05, geprüfter Quellstand `6b8fa5446512f89262c109866d1c5443faac1945`, veröffentlicht mit Berichtcommit `04e8b285afe0b5b151ef9d4d275201ea0fc144ff`. Auch [Issue #1][issue] wurde berücksichtigt; die jüngste ausdrückliche Benutzeranweisung erlaubt ausschließlich die Veröffentlichung dieses Berichts.

## 1. Entscheidung

**Ist die technische Grundlage bereit für die endgültige Vintage-GUI? NEIN.**

Die Reparaturen sind substanziell und wurden am tatsächlichen Code nachgeprüft. Insbesondere sind der belegte FLAC-Längenwiderspruch N03 und die aktuelle Host-/DSP-Abbildungsabweichung N05 gezielt angegangen. Das Routing ohne MAIN wird inzwischen mit gemessenen positiven Pegeln auf allen 15 AUX-Bussen geprüft. Der frühere Rollback benutzt keinen normalen, möglicherweise gesperrten Hide-Befehl mehr. Eine Zoompräferenz wird jetzt im Controllerstate gespeichert.

Die funktionale Entwicklung ist trotzdem noch nicht belastbar abgeschlossen:

1. **N02 bleibt FAIL:** Der neue Zoomcheck verwendet einen SDK-Cache, der nach Öffnen mit gespeicherten 150 % von der tatsächlichen Transformation abweichen kann. Ein abgelehnter Wechsel zu 100 % kann als erfolgreich gelten.
2. **N06, P1:** Die Pattern-Reparatur verändert ohne Migration die hörbare Bedeutung bestimmter bereits gespeicherter Automationswerte.
3. **N07, P1:** Während der neuen Kit-Restoresperre werden Parameter-Flushes mit Erfolg quittiert und verworfen. Der vorher reparierte F04-Pfad ist unter dieser Bedingung wieder umgangen.
4. **N08, P2:** Die neue Kitrevision setzt im ersten laufenden Block `wasPlaying_` zurück. Ein unmittelbar folgender Stop kann deshalb neue Sequencertails weiterlaufen lassen.
5. **N09, P2:** Bei voller Polyphonie verändert ein anschließend verworfener Choke-Trigger bereits laufende Stimmen. Der versprochene Erhalt aktiver Sounds bei Kapazitätsablehnung gilt dann nicht.

Die vollständige Atomizität des neuen Kitprotokolls ist außerdem **nicht nachgewiesen**. Der Konkurrenztest akzeptiert durchgehend stumme Blöcke und misst keine Generationenkonsistenz. Ein konkreter Memory-Ordering-Einwand wird als Risiko R01 getrennt von beobachteten Laufzeitfehlern dokumentiert. Native GUI-Abnahme und belastbare Echtzeitmessung bleiben offen.

Diese Entscheidung ist eine technische Arbeitsfreigabeentscheidung vor dem endgültigen GUI-Umbau. Sie behauptet weder einen hier ausgeführten Studio-One-Fehlschlag noch einen pauschal funktionsunfähigen Sampler.

## 2. Prüfumfang, Regeln und Evidenz

### 2.1 Verbindliche Grundlagen

`125A-Engineering/START-HERE.md`, die einschlägigen Engineering-Guardrails, Standards und QA-Regeln sowie das aktuelle Projekt-`AGENTS.md` wurden abgeglichen. Engineering-HEAD: `5f13a850c065167526519f0249fb7a38b5841a6a`, gegenüber dem vorigen Audit unverändert. Maßgeblich sind insbesondere Zustands-/Automationskompatibilität, nachweisbare GUI-Bedienketten und die Trennung von Durchsatz und zeitlich getakteter Echtzeitprüfung. Es wurden keine Regeln oder Dateien im Engineering-Repository geändert.

Die SDK-Quellen wurden gegen die Projektbaseline gelesen: VST3 SDK3.8.1 `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, public.sdk `586dc5e6c8012c3e4b01c79389375cbe96bdb1da`, pluginterfaces `4f547e8e102b47de4a8b8aaf343c73b700786372`, VSTGUI `5db272256172557818b6158cf0bb2c4410bddb25`. dr_flac ist in CMake auf Upstreamcommit `dfe8377631000664666519fdb83da193fd8037f4` festgelegt. Die lokale SDK-Kopie ist nur eine Quellenreferenz; nicht benötigte Build-/Dokumentations-Submodule wurden nicht als vollständig eingerichtete Windows-Toolchain ausgegeben.

Die Cloud-Einrichtung wurde anhand des Skills `cloud-environment-onboarding:setup` geprüft. Die vorhandene Umgebung eignet sich für Quell-, Git- und analytische Prüfungen. Sie enthält hier keinen nutzbaren Windows-VST3-/Studio-One-Laufzeitplatz. Gemäß Projektregel wurden keine Linux-Builds als Ersatz durchgeführt. Es wurde keine neue Umgebungskonfiguration benötigt.

Untersucht wurden alle **24 Produktionsquelldateien**, alle **10 Testdateien**, UIDESC, CMake, Workflow, relevante Projektdokumentation und die vollständige Reparaturhistorie. Gegenüber dem vorigen geprüften Implementierungsstand sind neun Produktionsdateien und drei Testdateien geändert oder ergänzt; die neue Produktionsdatei ist `PatternQuantization.h`. Unveränderte Scheduler-, Decoder-, Exchange-, Kit- und Voicepfade wurden erneut in den Gesamtablauf einbezogen. Die Gleichheit beziehungsweise Änderung der Dateien wurde anhand eines SHA-256-Inventars überprüft.

### 2.2 Status und Nachweisarten

- **PASS:** Der genau eingegrenzte Fehlerfall wird durch geeignete vorhandene Assertions und den zugeordneten erfolgreichen Windows-Workflow gestützt; Quellpfad und unabhängige Gegenprüfung passen dazu. Kein allgemeines GUI-/Release-PASS.
- **TEILWEISE:** Implementierung vorhanden, aber mindestens ein relevanter Pfad, Gegenfall oder praktischer Wirkungsnachweis fehlt.
- **FAIL:** Konkreter Widerspruch mit nachvollziehbarem Gegenfall am tatsächlichen Produkt-/SDK-Pfad. Quellenbeweis beziehungsweise analytischer Ablauf ist ausdrücklich kein hier ausgeführter Binär- oder DAW-Test.
- **OFFEN:** Zusätzliche Messung, Windows-/Hostprüfung oder Spezifikationsentscheidung erforderlich.

**Hier ausgeführt:** read-only Git-/Diff-/Quell-/SDK-Abgleich, unabhängige Python-Berechnungen für Patterngrenzen, GUI-Koordinaten, Zoomzustände, Transportzustände und die unveränderte SRC-Koeffizientenformel. Die Ergebnisse werden unten mit ausreichend Angaben zur Wiederholung wiedergegeben. Hilfsmodelle werden nicht als Produktregressionstests gewertet.

**Nicht hier ausgeführt:** C++-Tests, Windows-Build, VST3-Ausführung, native Editor-Attaches/Klicks, Studio One, 125A Plugin Tester, eigener Validatorlauf, Allocation-Profiler oder zeitlich getakteter Audiotest. Keine bestehende Testdatei wurde verändert. Kein Actions-Build wurde gestartet.

Die im vorigen Audit extern mit `ffprobe` untersuchte FLAC-Gegenfixture deklarierte 256 Frames bei 128 tatsächlichen Frames. Im aktuellen Quellstand ist genau dieser Fall nun als Repositorytest vorhanden. Die frühere externe Untersuchung wird nicht als erneute Ausführung des reparierten Wrappers ausgegeben.

### 2.3 Aktuelle Windows-Evidenz

Der vorhandene [Actions-Run #39085666516][run], Workflowlauf #186, zeigt **Success**, Gesamtdauer **2 min 41 s**, und verweist auf exakt den geprüften Commit `32f68ca7b2c3dc395c5a68efeb991760c373d4ac`. Der [Windows-Job][job] ist erfolgreich, Dauer 2 min 37 s. Die öffentliche Zusammenfassung zeigt ein Artefakt `125A-Drumator-M11-Full-Kit-Stress-QA`, ca. 1,4 MB, Digest `sha256:9b6c142e862a77cdaa564a6ec5f93639442003c8ce45f9ac71ac036420edbef4`.

Der Workflow [W:31] baut acht CTest-Executables und zwei separate Paket-Smokes; deren Aufrufe und Validatoraufrufe prüfen Fehlercodes. Somit sind die tatsächlich vorhandenen, verpflichtenden Assertions eng begrenzte vorhandene Windows-Evidenz. Die vollständigen Logs sind im verfügbaren Zugriff an eine Anmeldung gebunden; ein direkter Step-Log-Abruf lieferte 404. **Individuelle Testausgaben, numerische Performancewerte und genaue Validator-Testzahlen wurden nicht erneut ausgelesen.** Das Paket wurde hier nicht heruntergeladen oder ausgeführt. Eine Run-Zusammenfassung ersetzt weder Binärinspektion noch Studio-One-Abnahme.

### 2.4 Sämtliche Folgecommits seit dem letzten Bericht

| Commit | Tatsächlich geprüfter Inhalt |
|---|---|
| `c28f256e133bc8f359516dcc549e97bed756a970` | Direkter Zeilenzahl-Rollback, FLAC-Ist/Soll-Längenvergleich, gemeinsame Patternquantisierung und neue Assertions. |
| `889c813b16090aaac6d3bbe2b989b2adb60b0ddb` | Korrektur einer Windows-`min`-Makrokollision im Test. |
| `da0c25e137bcadc0a408c02aaf9ebef472aa5421` | Zoomcheck, Controller-Zoomstate, getrennte Header-Hitbereiche. |
| `a61ed7242120cb2b258926a3f469d85019820d2a` | Kit-Busyflag, Zähler aktiver Audioaufrufe, Kitrevision, Nullausgabe während Restore, Konkurrenztest. |
| `7f04fc0e670912b00057d3055b58070c9f1d820d` | Tatsächlicher Prozessor-Flush bei Zwischenwerten; Maskierung versteckter Solos und Recalltest. |
| `caa976370d78f3ac862eda388323223fa7af0811` | Pakettest für alle 15 AUX-Ausgänge mit Null-MAIN und Pegelassertionen. |
| `2c99e688121010e5cec666e764020eae7607aad1` | Korrektur einer MSVC-Testwarnung wegen Variablenschatten. |
| `32f68ca7b2c3dc395c5a68efeb991760c373d4ac` | Messung einzelner Core-Renderblöcke und Ausgabe von p95/p99/Maximum; kein neues Echtzeit-Gate. |

Committexte wurden nur als Navigation benutzt. Die Bewertung beruht auf dem tatsächlichen Diff und den vollständigen betroffenen Aufrufketten.

## 3. Ergebnis F01–F17

Die Zeilenlinks beziehen sich immer auf den geprüften Entwicklungscommit. Kürzel: C Controller, CH Controllerheader, P Prozessor, PH Prozessorheader, V View, E Editor, K Voice/SRC, S Exchange, B KitCodec, F FLAC, G Patternquantisierung, H Paket-Hosttest, T Voice-Tests, R VisibleRows-Tests, Q Editor-Smoke, W Workflow, L Zeilenpolicy, A AtomicPatternBank, Z Sequencer.

| Befund | Status | Nachprüfung, Nachweis und verbleibende Grenze |
|---|---|---|
| **F01 – Breite/Clipping** | **TEILWEISE** | [V:96], [V:108], [C:164] und UIDESC verwenden konsistent 1280. Alle 32 Stepflächen passen rechnerisch hinein; Step32 endet bei x=1229. Der konkrete 1100-Widerspruch ist beseitigt. [Q:45] prüft weiterhin nur das ungeöffnete äußere Editorobjekt; Attach, tatsächliche Childgröße und Randklicks fehlen. |
| **F02 – Hitboxen/Lücken** | **TEILWEISE** | Numerische Lücken 345…350 und 415…420 werden in [V:376] abgewiesen. A–H-Lücken werden in [V:323] abgewiesen; Headerbereiche sind nun passend getrennt, N04. Kein nativer negativer Klicktest. Zeilenfelder haben teilweise bewusst größere Ziele als die Glyphen/Stepfüllungen; diese Unterschiede sind in Abschnitt8 ausgewiesen. |
| **F03 – Host→GUI/Relayout** | **TEILWEISE** | Registrierung, Abmeldung und Invalidierung sind vorhanden [C:23], [C:32], [V:84]. State-Recall ruft Relayout auf [C:120]. [H:817] prüft Werte und drei ungeöffnete Editorobjekte, ohne tatsächliche Custom-View. Sichtbarer Rasterwechsel und abgelehntes Recall-Resize bleiben ungetestet. |
| **F04 – Parameter-Flush** | **TEILWEISE** | Außerhalb Restore verarbeitet [P:372] Nullframes/Nulloutputs korrekt; [H:851] prüft Auswahl5, [H:882] zusätzlich Zwischenwerte. Dieser Normalfall ist abgesichert. Der neue Restore-Frühreturn liegt davor und verwirft Flushes: **N07 / FAIL**. |
| **F05 – Host Dirty** | **TEILWEISE** | Nachrichtenänderungen laufen über [C:38] mit `IComponentHandler2::setDirty(true)` bei Erfolg. [R:46] prüft nur einen Bool-/Callbackhelper, keinen Controller und keinen Hosthandler. Song-Modified, Autosave, Ablehnungen und Meldereihenfolge fehlen. Zoomänderungen durch `rememberZoom` senden keine Dirtymeldung; deren Persistenz im ansonsten unveränderten Song ist gesondert zu prüfen. |
| **F06 – Legacy-Defaults** | **TEILWEISE** | [C:135] stellt bei DMR1 das Factoryraster her; [C:144] setzt alle modernen Felder zurück. Das passt zum Prozessor [P:588]. Codec-Migrationstests betreffen DMR4–9, nicht den tatsächlichen Controller nach modernen Edits beim Laden von DMR1–3. Dieser gezielte Zustandstest fehlt weiterhin. |
| **F07 – Unsichtbare Solos** | **TEILWEISE** | Direkter Hide bleibt bei Solo gesperrt [C:192]. Neu maskiert [L:14] versteckte Solobits beim Prozessor- und Controller-Recall [P:610], [C:110]. [H:1056] prüft am Paket, dass ein verstecktes Bit nach Restore nicht mehr gespeichert wird; dieser Recall-Teil ist sinnvoll belegt. Er prüft weder hörbares sichtbares Instrument noch tatsächlichen GUI-Hide. Versteckte Samples bleiben absichtlich erhalten. |
| **F08 – AUX ohne MAIN** | **PASS, enger Fehlerfall** | [P:394] und [K:160] erlauben Null-MAIN. [T:310] prüft bekannte Stemamplitude. Neu [H:928] liefert über 16 Busdeskriptoren bei inaktivem Null-MAIN auf allen 15 AUX-Bussen positive erwartete L/R-Pegel. Reine Nullen bestehen diesen neuen Test nicht. Nicht freigegeben sind damit alle Host-Rekonfigurationen und Isolationstoleranzen, siehe Abschnitt7. |
| **F09 – HiDPI im Resize** | **TEILWEISE** | [E:11] gibt den kombinierten SDK-Faktor frei; [V:100] multipliziert Breite/Höhe richtig mit Zoom×ContentScale. Kein nativer ContentScale-/Monitorwechseltest. Der geerbte SDK-ContentScale-Pfad kann bei Resize-Ablehnung ebenfalls inkonsistent werden; siehe N02/Risiken. |
| **F10 – Resize-Ablehnung** | **TEILWEISE** | [V:348] und [V:362] stellen bei Ablehnung nun den gespeicherten Count direkt wieder her; N01 wurde im Code sinnvoll repariert. Die Regression verwendet weiterhin nur einen Integerhelper. Attach/Recall übernehmen einen neuen Count vor einem eventuell abgelehnten Resize, ohne Scroll-/Fit-Fallback [C:105], [V:87]. Bedienbarkeit dieses Falls bleibt offen. |
| **F11 – Ctrl+Links-Reset** | **TEILWEISE** | [V:307], [V:314], [V:384] und [V:388] ergeben Human/Swing/Pan/Pitch=0 und Gain=100. Keine neue native Ctrl-Klick-/Audio-/Stateprüfung, weder pro Zeile noch bei 150 %. |
| **F12 – UTF-8-Namen** | **TEILWEISE** | [V:177] kürzt an Codepointgrenzen. Der frühere kaputte UTF-8-Schnitt ist im gelesenen Pfad beseitigt. Weiterhin Bytezahl statt Fontbreite, kein Graphemschutz, keine Messung langer breiter Namen. Keine Produktregression für CJK, Emoji, kombinierende Zeichen oder tatsächliche Darstellung. |
| **F13 – FLAC-Grenzen** | **TEILWEISE** | Metadatenprüfung vor Vollausgabe, 4096-Frame-Blöcke und Framebudget [F:27], [F:36], [F:49]. Riesige deklarierte Länge plus Erhalt des Outputs werden geprüft [H:765]; kleiner Längenwiderspruch nun ebenfalls, N03. Unbekannte Länge, tatsächlich bis/über die Ausgabegrenze wachsende Streams, Stereo-/Mehrkanalmatrix, Peak-RAM und Decoderzeit sind unzureichend verifiziert. |
| **F14 – Resampling/Last** | **TEILWEISE** | Adaptive 32/64/128/256 Taps bleiben unverändert [K:39]. Der ursprüngliche Aliasgegenfall ist deutlich verbessert; [T:33] prüft einen Frequenz-/Phasenpunkt. Neu werden 180 Core-Blockzeiten gemessen [T:268], aber die Messwerte sind nicht dauerhaft zugänglich, kein Deadline-/Overrun-Gate, kein Paced-Realtime. Vollständiger SRC-Qualitätsvertrag fehlt. |
| **F15 – Zoomanzeige/Persistenz** | **FAIL im Ablehnungsfall; Persistenzteil implementiert** | DZM1 mit fünf Bytes speichert 100/150 [C:51]; neue Editorobjekte übernehmen die Präferenz [C:155]. [H:127] prüft den Controllerstate im selben Controller. Keine native Reopenprüfung. Im belegten N02-Fall wird nach Ablehnung eine falsche Präferenz/Anzeige gespeichert. `setState` aktualisiert außerdem keine bereits geöffnete Editorinstanz. |
| **F16 – Patterntexte A–H** | **PASS, aktueller Abbildungsvertrag** | Stringliste [C:71] und gemeinsame `patternFromNormalized` [G:6] stimmen jetzt mit dem SDK überein. [H:107] prüft A–H; [H:115] prüft Zwischenwerttexte; [H:882] tatsächliche Prozessorzustände. Keine aktuelle `round(7v)`-/`floor(8v)`-Differenz mehr zwischen diesen Codepfaden. Altprojektkompatibilität und Restore-Flush sind separate Fehler N06/N07. |
| **F17 – Win32-Drop-Länge** | **TEILWEISE** | [V:65] und [V:277] akzeptieren die gemeldete Länge ohne erforderlichen Terminator und entfernen einen optionalen letzten NUL. Das entspricht dem gepinnten Win32-Datenpaket. Kein tatsächlicher `IDataPackage`-/Explorer-/Studio-One-Droptest; keine sichtbare Zielzeilen-/Zoomregression. |

Ein enger PASS schließt den genau bezeichneten ursprünglichen Fehlerfall. Er ist keine Abnahme des gesamten betroffenen Subsystems. TEILWEISE bedeutet nicht automatisch fehlerhafte Reparatur, sondern fehlende hinreichende Absicherung des vollständigen Befunds.

## 4. Ergebnis N01–N05

| Befund | Status | Tatsächlicher Reparaturstand |
|---|---|---|
| **N01 – Rollback mit belegter versteckter Zeile** | **TEILWEISE** | Der Produktionsfehler wurde zielgerichtet beseitigt: [V:347] speichert den alten Count, [C:214] stellt ihn unabhängig von Sample/Steps/Solo wieder her. Das ist der richtige Unterschied zu einer normalen Benutzer-Hide-Aktion. Der Test [R:31] ruft diese Methode weiterhin nicht auf; er kennt weder Nachrichtenversand noch versteckte Belegung oder ablehnendes `IPlugFrame`. Ein PASS der tatsächlichen Interaktionskette ist deshalb nicht begründet. |
| **N02 – Zoomablehnung** | **FAIL** | Der neue Check [E:18] erkennt einen häufigen Fall, wenn der Framecache vorher korrekt gesetzt war. Er prüft aber `CFrame::getZoom()` statt tatsächlicher Transformation. Nach Wiederöffnung mit gespeichertem Zoom kann dieser Cache anders initialisiert sein. Konkreter Gegenfall unten. |
| **N03 – FLAC deklarierte/decodierte Länge** | **PASS, genau dieser Fall** | [F:46] merkt die deklarierte Zahl; [F:57] verweigert eine abweichende bekannte Länge vor `out.swap`. [H:777] verwendet ausdrücklich „deklariert256, tatsächlich128“ und verlangt unveränderten vorherigen Output. Geeigneter gezielter Wrappertest im erfolgreichen Windowslauf. Unbekannte Länge und beliebige Korruptionsfälle sind damit nicht pauschal bestanden. |
| **N04 – Header-Randstreifen** | **TEILWEISE** | Pattern wird in [38,68), Zoom in [39,69) gezeichnet und nun entsprechend getrennt getroffen [V:137], [V:320], [V:329]. Die beiden alten y=68,5-/38,5-Gegenpunkte sind rechnerisch korrekt. [R:64] prüft allerdings zwei testlokale Lambdas mit denselben Zahlen und keinen Produkt-Hit-Test. Eine erneute Produktregression würde diesen Test nicht zwingend brechen. |
| **N05 – Host-/DSP-Zwischenwerte** | **PASS, ursprünglicher Abbildungswiderspruch** | Controller und alle drei Prozessorpfade verwenden nun [G:6], also `min(7,floor(8*clamp(v)))`, gleich dem gepinnten SDK. Die neuen Hosttests prüfen Zwischenwerttexte und echte Prozessor-Flush-/Stateauswahl. Unabhängiger 1001-Punkte-Vergleich bestätigt die aktuelle mathematische Übereinstimmung. Die gesamte Automation bleibt wegen N06/N07 nicht freigegeben; Audio bei mehreren Punkten im laufenden Block ist ergänzend nötig. |

### N02: Noch fehlerhafter Weg über gespeicherte 150 %

**Priorität P2; nachgewiesener Quellen-/SDK-Gegenfall, kein hier ausgeführter nativer Test.**

Produktdateien: [DrumatorEditor.h:14][E:14], [DrumatorController.cpp:155][C:155], [DrumatorView.cpp:334][V:334]. Maßgebliche SDK-Dateien: [vst3editor.cpp:2053][sdk-open], [cframe.cpp:89][sdk-frame-cache], [cframe.cpp:248][sdk-frame-zoom].

1. Ein gültiger Controllerstate enthält 150 %. `createView` ruft `setZoomChecked(1.5)` vor dem Öffnen auf. Ohne Frame setzt dies den Editorfaktor auf 1,5.
2. Der SDK-Normalpfad `enableEditing(false)` erzeugt den Frame und setzt dessen Transformation direkt mit `setTransform(scale(...))`, Zeile2081. Er ruft in diesem Pfad kein `CFrame::setZoom` auf. `userScaleFactor`, den `getZoom()` zurückgibt, ist zunächst **1,0**. Tatsächliche Transformation und Cache können folglich **1,5 / 1,0** sein. Ein Host bei normalem DPI muss das optionale ContentScale-Interface nicht aufrufen; der Gegenfall ist somit ein zulässiger Lifecycle.
3. Benutzer klickt 100 %. Der SDK setzt den Editorwert zunächst auf1,0. Der Host lehnt die Größenänderung ab. `CFrame::setZoom` stellt die vorherige Transformation1,5 wieder her; sein Cache bleibt1,0.
4. Der Wrapper vergleicht Cache1,0 mit erwartetem Faktor1,0 und meldet **Erfolg**. `rememberZoom(1.0)` speichert100 %, die Anzeige markiert100 %, die Darstellung bleibt150 %. Künftige Row-Resizes verwenden ebenfalls den falschen Faktor.

Der unabhängige Zustandsdurchlauf ergibt: `editor=1.0`, `preferred=1.0`, `frame_transform=1.5`, `frame_zoom_cache=1.0`, `wrapper_reports_success=true`.

**Empfehlung:** Die Zoomtransaktion an der tatsächlich wirksamen Transformation und dem tatsächlichen Host-/Frameergebnis absichern; den Initialisierungsweg einschließen. Dabei die SDK-API und Resize-Reentranz respektieren. Kein weiterer bloßer Vergleich eines ungesichert synchronen Caches. Den geerbten `setContentScaleFactor`-Pfad einbeziehen: auch er setzt den Faktor vor `CFrame::setZoom` und ignoriert dessen Ablehnung.

**Fehlende Regression:** Echter attachter Editor bei gespeichertem150 %, Host ohne ContentScale-Aufruf; Ablehnung150→100, Ablehnung100→150, erneuter gleicher Zoom, Close/Reopen, ContentScale1/1,5/2. Prüfen: reale Transformation, Frame-/Hostrect, Editorfaktor, Präferenz, Auswahlfarbe und nachfolgende Rowgröße. Der vorhandene State-Bytetest kann all diese Fehler übersehen.

## 5. Neue beziehungsweise jetzt konkret nachgewiesene Fehler

### N06 – P1 / FAIL: Reparatur verändert alte Patternautomation ohne Migration

**Ursache:** [PatternQuantization.h:6][G:6], Verwendung [C:170], [P:382], [P:451], [P:468]. Der Vorgängerstand verwendete `lround(clamp(v)*7)`. Der neue Stand verwendet `min(7,floor(clamp(v)*8))`. Beide sind für kanonische `p/7` gleich, für andere gültige Normalwerte nicht.

| Gespeicherter Automationswert | Alter Prozessor | Aktueller Prozessor/Hosttext |
|---|---|---|
| 0,10 | B | A |
| 0,124 | B | A |
| 0,125 | B | B |
| 0,875 | G | H |
| 0,90 | G | H |
| 0,929 | H | H |

Der unabhängige Vergleich `v=i/1000, i=0…1000` findet **213 geänderte Auswahlen**. Das ist die jetzt beseitigte interne Abweichung N05, als **Kompatibilitätsregression gegenüber gespeicherten älteren Automationskurven** betrachtet. Eine alte Kurve mit Wert0,10 kann nach Update einen anderen Beat spielen. Alte Projektstates speichern nur eine diskrete Auswahl; sie migrieren keine hostseitig gespeicherten Kurven. Parameter-ID1000, Version0.1.0 und StateformatDM10 bleiben unverändert; ein Übergangspfad ist nicht vorhanden.

**Abgrenzung:** Aktuelle interne Übereinstimmung ist verbessert. Ein Projekt mit ausschließlich den acht `p/7`-Werten ist von diesem Gegenfall nicht betroffen. Die tatsächliche Quantisierung beziehungsweise Speicherung von Zwischenwerten in Studio One ist separat zu testen. Die Änderung der öffentlich akzeptierten Parameterabbildung ist trotzdem unmittelbar nachweisbar und verletzt ohne vereinbarte Migration die Engineering-Kompatibilitätsregel.

**Empfehlung:** Einen ausdrücklich kompatiblen gemeinsamen Parametervertrag einschließlich Hosttext/Plainwert festlegen. Alte Wiedergabesemantik bewahren oder einen nachvollziehbaren, ausdrücklich vereinbarten Migrations-/Versionsweg entwickeln. Nicht einfach wieder zwei unterschiedliche aktuelle Abbildungen einführen.

**Fehlende Regression:** Dasselbe alte Projekt beziehungsweise dieselbe feste Normalwertkurve mit alter und neuer Paketversion rendern; A–H enthalten unverwechselbare Rhythmen. Hosttext, `normalizedParamToPlain`, Controller, Prozessorstate und Audio an allen alten/neuen Schwellen und unmittelbar beidseitig davon vergleichen. Die aktuellen Tests schreiben ausschließlich den neuen Vertrag fest und können diesen Kompatibilitätsbruch nicht erkennen.

### N07 – P1 / FAIL: Kit-Restoresperre verwirft erfolgreiche Parameter-Flushes

**Ursache:** [DrumatorProcessor.cpp:355][P:355] prüft die Restoresperre und kehrt mit `kResultOk` zurück, bevor [P:372] die Parameterwarteschlangen verarbeitet. Bei Nullframes wird nicht einmal ein Puffer beschrieben; die Änderungen verschwinden vollständig. Es gibt keine Pending-Parameterübernahme oder spätere Wiederholung im Plugin.

**Konkreter Ablauf:** Ein Restore installiert PatternA und befindet sich noch innerhalb der Sperre. Ein legaler Zero-Frame-/Zero-Output-`process()` liefert danach die AuswahlH. Der Aufruf gibt Erfolg zurück, liest die Queue aber nicht. Nach Freigabe der Sperre und ohne erneutes Hostevent bleibt der Prozessor beiA, während Controller/HostH anzeigen können. Besonders eindeutig ist das Interleaving nach `selected_.store` in Zeile623 und vor dem Entsperren beim Verlassen des Blocks. Es bedarf keines defekten WAVs und keiner fehlerhaften Hostdaten.

Im normalen Audioblock während Restore werden auch MIDI/Automation vollständig ignoriert. Ob Trigger beim Kitladen verworfen werden dürfen, benötigt eine Produktentscheidung; ein ohne Effekt erfolgreich bestätigter Parameter-Flush ist bereits ein konkreter Funktionsverlust. Das ist eine Regression des reparierten F04-Normalpfads.

**Empfehlung:** Eine eindeutige Reihenfolge zwischen Statecommit und eintreffender Automation definieren und die erforderlichen Werte mit begrenztem, allokationsfreiem Verfahren erhalten. Nur den Flushcode vor die Sperre zu verschieben reicht nicht automatisch: spätere Restorestores könnten ihn erneut überschreiben. Auch getState, Resume und Controlleranzeige müssen denselben endgültigen Wert sehen.

**Fehlende Regression:** Deterministisch überlappender tatsächlicher Paket-Restore plus Zero-Frame-Flush; ein Flush nach Stateauswahl-Commit und vor Entsperren; anschließend getState und hörbare Wiedergabe ohne erneutes Event. Zusätzlich laufende Blockautomation und gewünschte MIDI-Policy. Der vorhandene Flushtest läuft außerhalb der Sperre, der Konkurrenztest sendet keine Parameterqueues.

### N08 – P2 / FAIL: Transport-Stop direkt nach erstem Restoreblock wird übersehen

**Ursache:** [DrumatorProcessor.cpp:400][P:400] prüft Stop und merkt `wasPlaying_=playing`. Bei neuer Kitrevision setzt [P:408] diese Merkung im selben laufenden Block wieder auf `false`. Danach kann der Scheduler im neuen Kit bereits neue Stimmen starten.

**Konkreter Ablauf:** Gültiges Kit mit langem Sample laden. Erster vollständiger Block danach: `playing=true`, PPQ auf einem aktiven Step, Revision neu. Der Block setzt `wasPlaying_` zunächsttrue, im Revisionszweigfalse und startet anschließend einen neuen One-Shot. Der direkt folgende Block meldet `playing=false`. Die Bedingung `wasPlaying_ && !playing` ist false; der vorgesehene Stop-Reset bleibt aus und der lange Sound läuft weiter. Ein zusätzlicher laufender Block dazwischen würde die Merkung wiedertrue setzen und den Fehler verdecken.

**Empfehlung:** Die Revisionsbereinigung darf die tatsächlich im aktuellen Block verarbeitete Transportlage nicht verlieren. Gewünschte Stop-/Tailsemantik konsistent erhalten.

**Fehlende Regression:** Tatsächlicher Prozessor: Restore → genau ein laufender Block mit neuem Sequencerhit → Stop; der Folgebuffer muss gemäß vorhandener Stop-Policy still sein. MIDI-Audition bei dauerhaft gestopptem Transport separat erhalten. Im neuen Konkurrenztest ist `rtContext.state=0`; er kann diesen Ablauf grundsätzlich nicht prüfen.

### N09 – P2 / FAIL des vorhandenen Ablehnungsvertrags: Choke verändert Stimmen vor Kapazitätsentscheidung

**Ursache:** [SampleVoiceEngine.h:261][K:261] aktiviert Chokefades vor der Suche nach einer freien Stimme [K:269]. Sind alle64 Stimmen belegt, folgt [K:293] `rejectedCapacity++`, obwohl vorherige Stimmen bereits verändert wurden. Der Kommentar in Zeile267 verspricht bei Polyphonieerschöpfung einen verworfenen Trigger bei intaktem aktivem Sound.

**Konkreter Gegenfall:** 64 lange Stimmen starten, darunter mindestens eine mit Gruppe1; die übrigen können Gruppe0 sein. Ein weiteres gültiges Rowinstrument mit Gruppe1 wird getriggert. Es gibt keinen freien Slot: **kein neuer Sound**, aber die bestehende Gruppe1 bekommt einen Fade. Bei48kHz und2ms endet sie nach96 Frames. Ein kapazitätsbedingt abgelehnter Hit ist somit hörbar wirksam.

Dieser Fehler wurde nicht durch die acht jüngsten Commits eingeführt; das bisher als fehlender Kombinationsnachweis genannte Risiko ist am konkreten Ablauf und dem bestehenden Vertragskommentar jetzt präzisiert. Separate Tests für Choke und Kapazität reichen nicht.

**Empfehlung:** Slotannahme und Chokewirkung entsprechend einer ausdrücklich festgelegten Policy transaktional behandeln. Bei der vorhandenen „Drop ohne Änderung“-Policy dürfen abgelehnte Trigger keine Voicezustände ändern. Falls eine andere musikalische Policy gewollt ist, muss sie dokumentiert und auf hörbaren Ausfall geprüft werden.

**Fehlende Regression:** 64 belegte Slots plus Chokehit, Ablehnungszähler und Ausgang vor/nach dem Hit gegen einen Referenzlauf ohne diesen Hit vergleichen. Außerdem mehrere Chokegruppen, wiederholte Ablehnungen und Rückkehr nach Freigabe eines Slots.

## 6. Kitwechsel, Synchronisation und Echtzeit

### 6.1 Was verbessert wurde

[P:614] setzt eine explizite Restoresperre. Ein bereits aktiver Callback soll fertig werden; spätere Callbacks sollen nur Nullausgabe erzeugen. Nach vollständigem Statecommit erhöht sich die Revision [P:635]. Der nächste normale Audioblock setzt alte Stimmen zurück [P:405]. Dekodieren, Kernelvorbereitung, Speicherfreigabe und `kitMutex_` bleiben auf dem Controlthread. In `process()` befindet sich weder dieser Mutex noch ein Spinloop.

Die SPSC-Ownership bleibt grundsätzlich nachvollziehbar: [S:131] veröffentlicht immutable Assets, [S:143] nimmt nur noch nicht konsumierte Pointer zurück, [S:169] übernimmt einen Pointer atomar vor Dereferenzierung, [S:195] gibt nicht mehr referenzierte Assets über die Rückgabequeue an den Controlthread. Alte Einzel-Sample-One-Shots halten weiterhin ihr eigenes Asset. Diese Pfade sind besser begrenzt als eine beliebige synchrone Freigabe im Audioaufruf.

Das begründet **TEILWEISE**, keine pauschale Atomizitäts-/Echtzeitfreigabe.

### 6.2 R01 – P1 / OFFEN: Zwei-Atomic-Barriere besitzt keinen ausreichenden Ordnungsnachweis

Die Barriere besteht aus unterschiedlichen Atomics: Reader erhöht `active` mit acquire/release und liest `busy` mit acquire; Writer setzt `busy` mit release und liest `active` mit acquire [P:28]. Acquire/Release garantiert Sichtbarkeit bei einer tatsächlich gelesenen korrespondierenden Veröffentlichung. Es erzwingt hier keine gemeinsame sequentielle Ordnung zwischen beiden Atomics.

Der relevante Store-Buffering-Einwand lautet:

| Audio-/Readerthread | Control-/Writerthread |
|---|---|
| RMW `active:0→1` | `busy=true` release |
| liest `busy=false` aus vorherigem Zustand | liest `active=0` aus vorherigem Zustand |
| betritt normalen Produktpfad | betritt Installation |

Die beiden alten Beobachtungen werden nicht durch eine erfolgreiche gegenseitige Acquire/Release-Synchronisation ausgeschlossen. Das Reader-RMW allein macht die Writerfolge `store(busy); load(active)` nicht zu einem gemeinsamen atomaren Eintrittsprotokoll. Ein Quellenargument „beides ist atomic“ genügt daher nicht zum Beweis, dass kein Audioblock eine Teilinstallation sehen kann. Einzelne Patternzellen, Mixfelder und pro-Row-Pendingpointer sind weiterhin separat atomar [A:24], [P:618], [P:537].

**Einstufung:** Konkreter, zu klärender Memory-Ordering-Einwand; **kein hier beobachteter Windows-Race, kein behaupteter Use-after-free und keine behauptete Crashreproduktion**. Die starke Behauptung vollständiger Kitatomizität erhält bis zum Ordnungsnachweis kein PASS. Die Pointer-CAS-Ownership kann auch dann weiterhin korrekt sein, wenn ein gemischter Kitstand hörbar wird.

**Nacharbeit:** Das tatsächliche Protokoll gegen das C++-Speichermodell und den Windows-x64-Zielbuild prüfen. Eine klar belegbare Eintritts-/Veröffentlichungsordnung oder eine begrenzte Generationstransaktion verwenden; keine Audio-Mutexe/Spinlocks ergänzen. Litmus-/Interleavingtests und generationseindeutige Audiosignaturen kombinieren. Nur Stresswiederholungen ohne passenden Fehleroracle beweisen den Ausschluss nicht.

### 6.3 R02 – P1 / TEILWEISE: Sperre umfasst komplette Kernelvorbereitung und erzeugt Stummzeit

Die Sperre wird **vor** `publishRestoredKit` genommen [P:614]. Diese Funktion bereitet alle16 Assets einschließlich WAV-Dekodierung und bis zu16 großen SRC-Kerneln erst danach vor [P:521]. Währenddessen schreibt jeder neue Audioblock Nullen und verwirft seine Events [P:356]. Die maximale Stummzeit ist weder spezifiziert noch gemessen. Auch eine am Ende abgelehnte Vorbereitung hat bis dahin die Wiedergabe unterbrochen.

Eine begrenzte, lockfreie Nullausgabe kann das Callbackdeadline schützen. Sie ist aber kein Nachweis kontinuierlicher Audiowiedergabe, eines kurzen atomaren Umschaltpunkts oder verlustfreier MIDI-/Automationsverarbeitung. Zusätzlich kann der Controlthread in `yield()` warten, solange aktive Aufrufe nicht verschwinden. Das ist kein Audio-Spinlock, wohl aber ohne gemessenes Maximum eine mögliche UI-/Ladeverzögerung.

**Nacharbeit:** Teure Vorbereitungen möglichst vor dem kurzen Commitfenster vollständig validieren; State-/Assetgeneration und Fehler-Rollback klar definieren. Lade-/Dropoutpolicy festlegen und messen, ohne die Audioownership zu verletzen. Testfall: großes16-Row-Kit, Maximalpitch/-ratio, volle Voices, alternierende gegensätzliche Kits, fehlgeschlagene Vorbereitung und sofortiger Stop. Nullblöcke, maximale Nullsequenz, verlorene Events und Wiederanlaufzeit zählen.

### 6.4 Warum der neue Konkurrenztest nicht genügt

[H:1010] startet einen Audioactor und lädt denselben gespeicherten Kitstand sechsmal. Im Audioactor wird nur `kResultOk` und `isfinite` geprüft. **Eine Implementierung, die während und nach dem Test ausschließlich Nullen liefert, kann bestehen.** Alte und neue Kits haben keine unterschiedlichen Audiosignaturen; kein Sample prüft, ob Assets, Grid, Pattern, Mix und Solomasken dieselbe Generation besitzen.

Es gibt keine deterministische Start-/Überlappungsbarriere, keinen erforderlichen Audioblock pro Restore und keinen erfolgreichen Wiederanlauf nach jedem Restore. Der Actor kann bei4000 Blöcken bereits enden; verlangt wird nur `audioBlocks>0`. Keine Callbackzeiten, keine Stummblock-/Eventverlustzählung, kein Speicher-/Lifetimeinstrument. Das ist ein nützlicher Nichtabsturz-/Finite-Smoke, aber kein vollständiger Transaktionsnachweis und kein Echtzeittest.

## 7. Audioengine, Performance, Import und Hostintegration

### 7.1 Resamplingqualität

Adaptive Taps bleiben gegenüber dem vorigen Audit unverändert. Die unabhängige Berechnung der tatsächlichen Koeffizientenformel mit Float32-Zwischenspeicherung, Phase0, ergibt für Ratio16:

| Quellfrequenz, Zyklen/Frame | Aktuelle Amplitude relativ DC | Bedeutung |
|---|---|---|
| 0,04 | −47,678 dB | Der alte deutliche Aliasgegenfall ist stark verbessert. |
| 0,06 | −92,328 dB | Zweiter alter Gegenfall ebenfalls stark verbessert. |
| 0,028125 | −1,918 dB | Bei Ratio16 entspricht dies21,6kHz am48k-Ausgang: messbare Passbandrandabsenkung. |
| 0,034375 | −14,064 dB |26,4kHz am48k-Ausgang faltet auf21,6kHz zurück; Übergangsband noch nicht stark gedämpft. |

Berechnung: `cutoff=min(1,1/ratio)`, Blackmanfenster, `x=i-(taps/2-1)-phase/1024`, Sinc nach [K:35], DC-Normalisierung und `20log10(abs(sum(h[i]*exp(-j*2π*f*i))))`. Dies ist eine analytische Filtergegenprüfung, **keine gerenderte Plugin-FFT und kein Hörurteil**. Ohne festgelegte Bandgrenzen und zulässige Dämpfung wird aus dem Übergangsbandwert kein frei erfundenes Qualitäts-FAIL abgeleitet.

Der neue beziehungsweise bestehende [T:33]-Test prüft nur Ratio16, Phase0, Frequenz0,04 gegen40dB. Fractional Ratios, alle relevanten Phasen, tatsächliche Audioaliasprodukte, Grenzimpulse, Passband-/DC-Verhalten, Stereo und realistische Drums sind nicht ausreichend vermessen. Pitch wirkt über SRC auf neue Stimmen; bestehende Voices behalten ihren Startkernel. Keine Samplelängen- oder Pitchkontrolle darf als Echtzeitgarantie ausgegeben werden.

### 7.2 Was die vorhandenen p95-/p99-/Max-Messungen tatsächlich aussagen

**Die Messung ist implementiert; ihre numerischen Ergebnisse sind im zugänglichen Repository/Runmaterial nicht vorhanden. Es werden keine Werte erfunden.**

[T:252] erzeugt64 Voices mit Ratio16 und256 Taps,48kHz,256 Frames. `measuredRender` misst ausschließlich `SampleVoiceEngine::render` mit `steady_clock`. Pro fünf Zyklen werden36 Blöcke erfasst: **180 Messpunkte**, davon160 Blöcke mit laufenden Voices und20 vollständig leere Drainblöcke. Gesamte verarbeitete Audiolänge **0,96s**. Der nächste-Rang-p95 ist der171., p99 der179. sortierte Messwert; der Maxwert ist der180. Das ist für diesen kleinen Offline-Datensatz rechnerisch schlüssig.

Die Blockdeadline beträgt **256/48000 = 5,333333ms**. Sie wird nur als Text daneben ausgegeben. Es wird weder eine Deadlineüberschreitung gezählt noch ein p95/p99-/Max-Limit verlangt. Der einzige Zeit-Abbruch bleibt die großzügige Gesamtlaufzeit `<20s` [T:308], inklusive anderer Arbeiten im Test. Ein deutlich zu langsamer Audiokern kann deshalb trotzdem grün sein. Der Testkommentar bezeichnet diesen Umstand korrekt als CI-Sicherheitsgrenze und nicht als Realtimequalifikation.

**Fehlende Veröffentlichung:** Der Workflow verwendet `ctest --output-on-failure` [W:36]. stdout erfolgreicher CTests, also auch die gerade hinzugefügten p95-Zeilen, wird damit nicht im normalen erfolgreichen Workflowlog ausgegeben. CTest kann sie lokal im Runner unter `Testing/Temporary/LastTest.log` hinterlassen; hochgeladen wird jedoch nur `artifact/` mit dem VST3-Bundle [W:110]. Eine Messdatei oder dieser Testlog wird nicht archiviert. Somit ist nicht einmal eine nachprüfbare quantitative Auswertung des aktuellen Laufs verfügbar.

**Grenzen des Versuchs:** ungepacte Offlinefolge; nur eine Blockgröße/eine Rate; Monoquelle und identischer Kernel für alleRows; kein vollständiger VST3-Prozesspfad; keine realen interleavten Stereoquellen/Multi-Out-Last; keine Kit-/UI-/Automationparallelität; keine CPU-/Takt-/Energiespar-/Lastmetadaten; kein Timer-Overheadvergleich; keine ausreichend lange Tail-Latency-Stichprobe. Auch ein zugänglicher Maxwert unter5,333ms wäre eine Stichprobenbeobachtung auf diesem Runner, keine garantierte Echtzeitfähigkeit.

**Erforderlicher Nachweis:** Messwerte dauerhaft als CSV/JSON plus Metadaten speichern; vollständiges `IAudioProcessor::process` messen; end-to-end ungepacten Durchsatz und zeitlich getaktete Echtzeit getrennt erfassen. Für32/64/128/256/1024 Frames und44,1/48/96/192kHz auf festgelegter Zielhardware: mean/p95/p99/max, Deadline, Overrunzahl, Dropouts, Voice-/Eventdrops, Callback-Allokationen sowie Dauer-/Mehrinstanz-/UI-/Kitstress. Keine neue Messkampagne wurde für diesen Audit gestartet.

### 7.3 Speicherverwaltung und Import

WAV bleibt auf RIFF, Mono/Stereo, PCM8/16/24/32 und Float32 begrenzt; passende Extensible-GUIDs werden geprüft. Float32 NaN/Inf und Werte über Absolut16 werden abgewiesen, leere/zu lange Frames ebenfalls. RF64/RIFX, Float64 und abweichende ValidBits werden nicht behauptet. Fehlgeschlagene Dekodierung übergibt den Kandidaten nicht an den vorherigen Output. Dateigröße pro importierter Datei beziehungsweise behaltenem WAV maximal16MiB, gesamte eingebettete WAV-Daten maximal64MiB [B:24].

FLAC wird off-audio in Blöcken gelesen. Ein bekannter Soll/Ist-Längenwiderspruch wird jetzt vor Austausch abgewiesen, N03. Bei `totalPCMFrameCount==0` fehlt naturgemäß dieser Vergleich: wie ein vorzeitig endender oder teilweise beschädigter Stream von einem zulässigen unbekannt langen Stream zu unterscheiden ist, benötigt zusätzliche Testfixtures und eine definierte Fehlerpolitik. „Jede beschädigte FLAC sicher abgelehnt“ ist nicht nachgewiesen.

Die16/64MiB-Grenzen sind **keine Prozess-RAM-Grenzen**. PCM8 kann beim Floatdecode etwa auf das Vierfache wachsen; UI-Preflight, Nachrichtenkopien, behaltene WAVs, Kandidaten, alter/neuer Kit und alte Voiceassets können gleichzeitig existieren. Jeder nichtleere PreparedSample reserviert aktuell1024×256 Floatkoeffizienten, also1MiB, auch wenn32 Taps benutzt werden.16 aktuelle Kernel beanspruchen16MiB zusätzlich. Retire96/Return128 begrenzen Pointerzahlen, nicht ein einheitliches Bytebudget.

Import, Pitchneuvorbereitung und komplette Kitvorbereitung sind synchron im UI-/Controlpfad. Das vermeidet Decoderarbeit im Audio, garantiert aber keine reaktionsfähige GUI bei großen Dateien. Peak-RAM, Ablehnung unter Speicherdruck, Rückgewinnung nach vielen Wechseln und maximaler Kitwechsel müssen gemessen werden. Für den gewöhnlichen Sampletausch ist die immutable Ownership nachvollziehbar; ein pauschales Lifetime-PASS der neuen Gesamtkitsperre folgt daraus nicht.

### 7.4 Stimmen, Choke, Mute/Solo, Panorama und MIDI

Der Voicepool hat64 feste Slots. Sampleende beendet Stimmen; Retrigger überlagert One-Shots. Gain/Pan/Pitch/Asset werden beim Start übernommen. Spätere Gain-/Panänderungen und Mute/Solo verändern neue Trigger, laufende Tails bleiben grundsätzlich erhalten; [T:96] verlangt dieses Verhalten ausdrücklich. Das ist keine neu erfundene Reparaturanforderung, sondern die aktuell getestete Policy. Stop setzt Tails dagegen zurück; N08 verletzt diesen vorhandenen Unterschied.

Pan ist ein Stereo-Balancegesetz mit unverändertem Pegel pro Kanal in der Mitte und Absenkung der Gegenseite zum Rand. Gain linear0…2. Keine versteckte Normalisierung oder Limiter. Mehrere summierte Stimmen dürfen deshalb über0dBFS gehen; Headroom ist keine Fehlerbehebung durch nachträglichen Limiter.

Choke verwendet2ms Fades. N09 betrifft die Kombination mit voller Kapazität. Die Anzahl verworfener Stimmen ist im Core-Ergebnis verfügbar, wird vom Prozessor aber nicht als Host-/GUI-Diagnose ausgewertet. Ebenso werden TriggerBatch-Überläufe nicht sichtbar gemacht. Belastungsgrenzen sollen dokumentiert und gemessen werden.

MIDI NoteOn36…51 triggert16 Rows, Velocity wird auf1…127 abgebildet, NoteOff ist bei One-Shots ohne Gatefunktion. Alle Kanäle werden derzeit gleich behandelt; kein MIDI-Learn-/Notenzuordnungseditor. Es gibt nur den automationsfähigen Patternparameter1000; die übrigen UI-Felder arbeiten über Editnachrichten. Nicht vorhandene Automatisierung dieser Felder darf nicht als implementiert beworben werden.

Scheduler: acht Patterns mit32 Sechzehnteln, absolutes Host-PPQ, begrenzte Boundaryarbeit, deterministisches Humanize, Swing auf ungeraden Steps. Das ist für reproduzierbares Offlineverhalten sinnvoll. Erforderlich bleiben Audio-/Timingabgleiche über Tempoänderungen, negative PPQ, Loop/Seek, Blockteilungen und Patternwechsel unmittelbar an geswingten/humanisierten Grenzen. Seek/Loop haben keinen eigenen expliziten Voiceflush; die gewünschte Tailpolicy ist festzulegen. Mehrere Parameterpunkte pro Block benötigen hörbare, nicht nur gespeicherte Assertions.

### 7.5 Multi-Out und übriger VST3-Lifecycle

Fester Vertrag: sichtbare Zeile1 auf MAIN, Zeilen2…16 auf ihre zugeordneten AUX-Busse, sofern aktiviert und gepuffert, sonst MAIN. Kein doppeltes MAIN-Signal für aktiv ausgelagerte Stimmen. Freie Ausgangswahl ist nicht implementiert.

Der neue15-AUX-Test ist wesentlich aussagekräftiger als der alte Finite-Smoke. Seine erwarteten Pegel liegen aber nur **0,015** auseinander, die Toleranz ist **0,03** [H:951], [H:1001]. Ein Vertauschen benachbarter Busse kann damit bestehen. Nicht alle15 Signaturen werden unter dieser Toleranz eindeutig unterschieden. Die tatsächliche Implementierung [K:224] routet anhand der gespeicherten Row; aus der Testschwäche allein wird kein Routingdefekt behauptet. Für strenge Isolation: einzelne Rows schalten oder orthogonale Signaturen und aus der Numerik begründete Toleranzen verwenden; inaktive Lücken, AUX15, Rückfall auf MAIN und Reaktivierung getrennt prüfen.

Es werden ausschließlich32-Bit-Samples unterstützt;64-Bit wird korrekt abgelehnt und darf nicht als unterstützt gelten. `setActive(false)` und `setProcessing(false)` setzen Voices zurück. `setupProcessing` bereitet vorhandene Samples für die neue Rate auf dem nicht verarbeitenden Thread neu vor; der vorhandene48→44,1kHz-Pakettest prüft danach wieder positiven Pegel [H:794]. Mehrfachzyklen, andere Raten, variable Hostblockgrößen, Offlineexport und tatsächliche Windows-Editorlebensdauer bleiben Praxisgates.

`getTailSamples()` meldet bewusst unendlichen Tail für den selbstlaufenden Sequencer. Projektstate enthält eingebettete Samples, alle Patterns, Mix/Gates/Pitch/Choke/Swing/Humanize und sichtbare Zeilen. DMR4–9-Migrationen haben Codec-Assertions; DMR1–3-Controllerpfade und alle Legacyformate im tatsächlichen neuen Host-/GUI-Lifecycle nicht. Der neue DZM1-Controllerstate ist unabhängig vom Komponentensoundstate. Sein Restore in bereits geöffnete Editoren sowie ein alter leerer/nicht vorhandener Controllerstate benötigen Prüfung.

## 8. Gesamte provisorische GUI: Kontroll- und Geometriematrix

Dies ist die vollständige **Quellenzuordnung**, keine behauptete native GUI-Abnahme. Es gibt keine individuellen VSTGUI-Controltags: sämtliche folgenden Aktionen werden manuell in einer `DrumatorView` gezeichnet und getroffen. Für jede Zeile `r=0…visibleRows−1` und jeden Step `s=0…31` gelten die Formeln. Damit sind alle bis zu512 Step-Tasten erfasst; kein nicht geprüfter Einzelstep wird als praktisch bestanden ausgewiesen.

Logische Größen: `y=108+29*r`, `xStep=560+21*s`, Breite1280, `H=max(300,172+29*visibleRows)`. Physische Anforderung: logisch × Benutzerzoom × Host-ContentScale. Rechtecke nachfolgend links/oben inklusive, rechts/unten für Hit-Tests überwiegend exklusiv.

| Element / Anzahl | Position, Maße und tatsächlicher Klickbereich | Daten-/Funktionszuordnung, Rückmeldung, State und Auditstatus |
|---|---|---|
| Titel1 | x16…1000, y8…32; Schrift18, links | Anzeige, kein Klick. Keine native Font-/Clippingmessung. |
| Patternhilfe1 | x16…320, y47…67, links | Anzeige `PATTERN A-H / SHIFT+STEP`; kein Parameter. Keine Stepnummern oder Lauflichtanzeige implementiert. |
| Pattern A–H,8 | x340+62p…+54, y38…68;54×30;8px Zwischenräume ohne Aktion | `changePattern` → begin/perform/endEdit Parameter1000 → Hostqueue → Prozessor; Auswahlfarbe aus `currentPattern`; State selected. Aktuelle Abbildung N05 korrigiert; N06/N07 offen beziehungsweise FAIL. Native Audio-/Anzeigeinteraktion OFFEN. |
| Zoom100,1 | x1070…1127,y39…69;57×30 | `setZoomChecked(1)` → rememberZoom → DZM1; Farbe vom Editorwert. N02 FAIL. Kein nativer Ablehnungstest. |
| Zoom150,1 | x1136…1217,y39…69;81×30 | Entsprechend Faktor1,5. Wiederöffnung/HiDPI/Hostablehnung OFFEN beziehungsweise N02 FAIL. |
| Swinganzeige/−/+,1 | Label16…182,y74…98; Bedienzone185…244, Teilung214 | Klick±5, Ctrl→0, Grenzen0…50; Nachricht `Drumator.Swing`, Prozessoratomik, DM10; Anzeige sofort bei Erfolg. Nur ±-Zone ist bedienbar. Native Reset-/Timingprüfung OFFEN. |
| Humananzeige/−/+,1 | Label275…447,y74…98; Bedienzone448…505, Teilung477 | Klick±10, Ctrl→0, Grenzen0…100; `Drumator.Humanize`, deterministisches Timing/Velocity, DM10. Native Wirkung OFFEN. |
| Zeilennummern,1…16 | x16…47,y…y+26; links | Anzeige; liegt im Sample-Klickbereich16…275 und öffnet daher bei Klick ebenfalls den Dateidialog. Kein unabhängiger Zeilenwahlparameter. |
| Samplelabel/Load/Clear,1…16 | Text52…275,y…y+26; Klick16…275 über volle29px | Klick→WAV/FLAC-Picker→Controllerpreflight→WAV-Nachricht→PreparedSample→nächster Block; Alt→Clear, Steps bleiben. Filename statt unabhängigem Instrumentennamen; Erfolg/Fehler im Footer. Sample/Name im Kitstate. Native Auswahl/Abbruch/Drop/Unicode OFFEN. |
| VOL,1…16 | Text/Klick x280…345; Hit volle29px, Text26px; Halbierung312,5 | ±10 linear, Ctrl100; RowMix→Gain0…2; neuer Voicegain, State; numerische Anzeige. Breiten-/Fontmessung OFFEN. |
| PAN,1…16 | x350…415, Halbierung382,5; übrige y wie VOL | ±10, Ctrl0; RowMix→Balance−1…1, State; Vorzeichenanzeige. Laufende Voices behalten Startbalance. Native Wirkung OFFEN. |
| P/Pitch,1…16 | x420…475, Halbierung447,5 | ±1 Halbton, Ctrl0, Grenzen±24; RowPitch→Assetneuvorbereitung→neue Voices, State; Anzeige mit Vorzeichen. Native Reaktionszeit OFFEN. |
| Mute,1…16 | Gezeichnet478…500,y+3…y+24; Hit478…500 über volle29px | RowGate→Mute-Maske→Filter neuer Trigger, orange Rückmeldung; State. Größerer Hitbereich berührt keinen anderen Rowcontrol. Keine native Abnahme. |
| Solo,1…16 | Gezeichnet507…529,y+3…y+24; Hit über volle29px | RowGate→Solo-Maske, cyan Rückmeldung; State und neue Recallmaskierung. F07 TEILWEISE. |
| Choke,1…16 | Text535…558,y+3…y+24; Hit über volle29px | Klickzyklus0…4→RowChoke→neue Voicegruppe; numerische Anzeige/State. Kein eigener beschrifteter CHOKE-Header und kein eigener Ctrl-Resetpfad; N09 bei voller Kapazität. Native Zuordnung OFFEN. |
| Step-Tasten,32 je sichtbarer Row | Zelle21×29; Füllung(x+2…x+18,y+4…y+22)=16×18; Viertelrahmen bei s%4=0 | Ganze zugehörige Zelle klickbar; Klick0↔127, Shift0/32/64/96/127; StepEdit→AtomicPatternBank→Scheduler→Voice; Farbe und Velocitybar aus aktuellem Pattern; im State alle8×16×32 Zellen. F01/F02 teilweise; native Tests jedes Steps bei beiden Zooms fehlen. |
| Add,1 | x16…213, yA…yA+29, `yA=113+29*n` | Count+1→Hostresize→direkter alter Count bei Ablehnung; Status bei Max16/Fehler. Keine Datenlöschung. N01 Quellreparatur, native Verifikation fehlt. Rechter Randx=213 ist inklusiv getroffen. |
| Hide,1 | x226…266, gleiche yA | Nur letzte leere/nichtsolo Row; prüft Steps in allen8Patterns. Bei Ablehnung Countrestore. Farbe aus `canHideLast`; Fehlermeldung erwähnt nur Steps/Sample, nicht Solo. Native Prüfung OFFEN; rechter Rand inklusiv. |
| Zeilenzahl1 | x277…455, gleiche yA, links | Anzeige n/16, keine eigene Aktion; State visible. |
| Status/Footer1 | x16…1240,y=147+29*n…+21, links | Import-/Edit-/Resizefehler und Erfolg. Keine eigene Interaktion; keine Messung der tatsächlichen langen Textbreite. |
| Bedienhilfe1 | x545…1240,y74…98, links | Anzeige der Halbflächen-/Alt-Bedienung. Ctrl-Reset/Choke/Outputmapping werden nicht vollständig erklärt; kein automatischer Funktionsnachweis. |

Die größeren Row- und Step-Hitflächen sind eindeutig einer Zeile/einem Step zugeordnet; sie werden nicht allein wegen Abweichung von Glyphen als Fehler deklariert. Headerlücken und falsche Zielzuordnung werden dagegen gesondert geprüft. Kein Scroll-, Drag-, Wheel-, Texteingabe- oder Keyboard-Editingpfad für diese numerischen Anzeigen ist implementiert. Die Engineering-Bedienanforderungen für präzise numerische Bedienung sind vor der Final-GUI verbindlich zu klären und nativ abzunehmen.

### Größen- und Instrumentenzahlmatrix

| Sichtbare Rows | Logische Größe100 % | Größe150 %, ohne zusätzliches DPI | Letzte Stepfüllung, y-Bereich | Add-/Hide-Oberkante | Footerunterkante |
|---|---|---|---|---|---|
|1|1280×300|1920×450|112…130|142|197|
|4|1280×300|1920×450|199…217|229|284|
|5|1280×317|1920×475,5|228…246|258|313|
|8|1280×404|1920×606|315…333|345|400|
|16|1280×636|1920×954|547…565|577|632|

SDK-Resizeanforderungen runden physische Koordinaten mit `floor`; bei5Rows/150 % ist die halbe Pixelhöhe daher nativ nachzuprüfen. Letzter Step rechts: Füllungx1213…1229, Zelle1211…1232, innerhalb1280. Bei16Rows/150 % benötigt das Fenster1920×954 plus Hostrahmen; auf einem1920 breiten Monitor kann dies tatsächlich Ablehnungen oder Platzprobleme auslösen. Mit ContentScale1,5 kämen2880×1431 hinzu. Das sind geometrische Anforderungen, keine Behauptung über das tatsächliche Verhalten eines konkreten Studio-One-Fensters.

**Weiteres offenes Layoutproblem:** Nach gültigem16-Row-Recall kann [C:105] bereits16Rows anzeigen wollen, während [V:88] eine Vergrößerung anfordert. Bei Hostablehnung bleibt die kleine View; es gibt keinen Scroll-/Fit-Fallback. N01 behandelt nur Add/Hide-Rollback und löst diesen Lifecyclefall nicht. Ebenso kann `Controller::setState` die Zoompräferenz ändern, ohne einen bereits geöffneten Editor zu aktualisieren. Beide Wege benötigen echte Hosttests und gegebenenfalls eine definierte Bedienstrategie.

**Noch offene Produktziele:** Eigenständige Instrumentennamen, frei wählbare Outputs, Lauflicht/Stepnummern, Samplepreview, Copy/Paste, Undo und optionaler Vintage-Samplercharakter sind im Draft `docs/V1-SCOPE.md` genannt, aber nicht vollständig implementiert. Der Draft ist ausdrücklich keine gesperrte Spezifikation. Daraus werden keine erfundenen neuen Pflichtbugs abgeleitet; vor „funktionale Entwicklung abgeschlossen“ muss der Owner den tatsächlichen V1-Umfang festlegen.

## 9. Regressionstests: geeignete Assertions und wesentliche Lücken

| Testpfad | Was er tatsächlich belegt | Was er übersehen kann / notwendige Ergänzung |
|---|---|---|
|[H:115], [H:882], [R:52] Pattern | Hosttexte an Zwischenwerten; echte Prozessor-Flushwerte im serialisierten State;1001 mathematische Punkte im Helper | Alte Automationssemantik N06, hörbare Auswahl, mehrere Punkte im laufenden Block, Grenznachbarn und Flush unter Restore N07. |
|[H:777] FLAC | Bekannte deklarierte256/actual128-Differenz wird abgelehnt; vorheriger Output unverändert | Native Controllerlade-/Dropkette, unbekannte Länge, realer Decode-Oversize, alle Korruptionsarten, Peak-RAM/Decodezeit. |
|[H:127] Zoomstate | DZM1-Fünfbytestate wird im selben Controller gelesen/geschrieben | Neuer Controller, tatsächliches Attach/Reopen, vorhandener Editor bei setState, Ablehnung N02, ContentScale und Dirty. |
|[R:31] Resize | Generische Edit/Resize/Revert-Reihenfolge mit unbedingtem Integer++/-- | Echte `restoreVisibleRows`, Nachrichtenausfall, versteckt belegte Row, Host-Ablehnung und Custom-View-Lifecycle. Der ursprüngliche fehlerhafte Controllerrevert könnte wieder eingeführt werden, ohne dass dieser Test zwingend scheitert. |
|[R:64] Header | Zwei lokal definierte y-Prädikate | Die gezeichneten und tatsächlich geklickten Produktrechtecke werden nicht aufgerufen. Eine Änderung nur in `DrumatorView` wird dadurch nicht abgesichert. |
|[R:46] Dirty | Booltrue führt einen Lambda aus | `IComponentHandler2`, Controlleredits, Ablehnung, tatsächliches Hostmodified. |
|[H:1056], [L:14] HiddenSolo | Paketstate verliert ein verstecktes Solo-Bit, Zahl sichtbarer Rows bleibt4 | Hörbares sichtbares Instrument, tatsächlicher Hide, Maskenmischung aus sichtbaren und versteckten Solos,16Rows-Editor. |
|[H:928]15AUX | Jeder AUX liefert einen positiven Pegel bei Null-MAIN | Benachbarte Busvertauschung kann wegen0,03-Toleranz bei0,015-Abstand bestehen; separate Isolation/Single-Row-Signaturen fehlen. |
|[H:1010] Konkurrenz | Wiederholtes `setState` und Audioactor enden ohne nichtfinite Werte oder Rückgabefehler | Kein beweiskräftiger Überlappungs-/Generations-/Wiederanlauf-/Pegel-/Eventoracle. All-zero-Implementierung kann bestehen. |
|[T:252] p95/p99/max | Zeitmessung von180 ungepacten Core-Renderaufrufen und Hanggrenze | Keine archivierten Zahlen, kein Overrun-Gate, keine vollständige Callback-/Realtime-/Systemlastmessung. |
|[T:134], Kapazitätstests | Choke bei verfügbaren Slots;64-Voice-Grenze in anderer Konfiguration | Kombinationsfehler N09; abgelehnter Trigger verändert alten Sound. |
|[H:817], [Q:45] Editor | Ressourcenrect und ungeöffnete Editorobjekte existieren | Kein HWND/`attached`, kein wirklicher `DrumatorView`-Listener, kein Render, keine Pixel-/Klick-/Dropprüfung. |
|KitState-/WAV-/Exchange-/Coretests | Relevante lokale Codec-, Grenz-, Ownership- und Timingfälle | Nicht automatisch VST3-Controller-Migration, nativer Lifecycle, Gesamtkitsynchronisation oder Studio-One-Projektrecall. |

Die8 CTests sind echte Tests mit Assertions; sie sind nicht wertlos. Umgekehrt schließen neue Logzeilen mit dem Wort „PASS“ keine größeren Pfade ein als die dahinterstehenden Assertions. Erfolgreiches CI schützt nur die tatsächlich geprüften Oracles.

## 10. Priorisierte notwendige Nacharbeiten und Freigabekriterien

### Vor einer technischen Freigabe für die endgültige GUI

1. **P1 N06:** Kompatiblen Patternvertrag/Migrationsweg festlegen und alte Automationsfixtures vor/nach Update hörbar vergleichen.
2. **P1 N07:** Reihenfolge Restore/Automation und verlustfreien letzten Flushwert absichern; deterministische Paketregression.
3. **P1 R01/R02:** Kit-Atomizität formal nachvollziehbar absichern; generationseindeutigen Audiooracle, garantierte Überlappung und Wiederanlaufassertion ergänzen. Stumm-/Dropout-/Fehlerpolitik und maximalen Umschalt-/Vorbereitungsaufwand messen.
4. **P2 N02:** Zoomzustände über echten Frame-/Hosterfolg einschließlich initialisiertem Reopen und ContentScale behandeln; keine Cachegleichheit als ausreichendes Erfolgskriterium.
5. **P2 N08/N09:** Die konkreten Transport-/Kapazitätskombinationen entsprechend dem vorhandenen Vertrag reparieren und durch echte Regressionsfälle absichern.
6. **P2 Testqualität:** Echte attachte GUI und tatsächliche Controller-/Dirty-/Resizepfade testen;15-AUX-Isolation mit unterscheidbaren Signaturen; DMR1–3-Controllerdefaults und State-/Zoomrecall bei offenem Editor.
7. **P2 Performance/SRC/Import:** Numerische Messartefakte zugänglich machen, Echtzeitmessung auf Zielhardware durchführen, SRC-Qualitätsgrenzen und Import-/Peak-RAM-Politik begründen.
8. **Produktumfang:** Offene V1-Bedienfunktionen aus dem Draft ausdrücklich entscheiden. Die endgültige GUI darf nicht als Ersatz für noch fehlende Funktionsverträge dienen.

**Objektives GUI-Startgate:** Keine offenen P1-Funktions-/Synchronisationsfehler; behobene P2-Interaktions-/Lifecyclefehler mit geeigneten Regressionen; vollständige native100/150%-Kontrollmatrix; nachgewiesene Project-/Automation-Kompatibilität; belegbarer16-Row-/Multi-Out-/Kit-Wiederanlauf; zugängliche Messwerte und ein ausreichend definierter Realtime-/SRC-Vertrag. Owner-Abnahme und späterer Releasegate bleiben davon getrennt.

## 11. Konkrete letzte Studio-One-Praxistests

Diese Fälle sind **OFFEN** und wurden hier nicht durchgeführt. Automatisch reproduzierbare Gegenfälle sollen zuvor im Windows-Paketharness abgesichert werden; ein Studio-One-Test ersetzt diese Regressionen nicht.

1. **Saubere Installation/Baseline:** Paketcommit und DLL-Hash dokumentieren; genau eine geladene Pluginversion. Windows11 x64, konkrete Studio-One-/Studio-Pro-Version, Audiointerface/ASIO, CPU, Energieprofil und DPI festhalten.
2. **Vollständige Bedienmatrix:** Bei1/4/5/8/16Rows und100/150 % jeden A–H-/Zoom-/Add-/Hide-Button, alle32Steps jeder sichtbaren Row, Shiftvelocity, VOL/PAN/Pitch beide Hälften und Ctrlreset, M/S/Choke, Load/Altclear bedienen. Vorher/nachherwerte, hörbaren Effekt, Anzeige und Save/Reopen protokollieren. Lücken und Ränder ausdrücklich negativ testen.
3. **Zoom/Hostablehnung:** Song mit150 % speichern, Editor schließen/öffnen und100 % wählen; reale Fenster-/Transformationsgröße gegen Markierung vergleichen. Kontrolliert ablehnenden nativen Testhost für beide Richtungen verwenden. Windows100/150/200 %-DPI, Monitorwechsel, kleine Arbeitsfläche,16Rows und bereits geöffneten Editor beim Projekt-/Controllerstate-Recall testen.
4. **Automation/Kompatibilität:** Alte und neue Paketversion mit fester Kurve0,10/0,124/0,125/0,875/0,90/0,929 und allen Schwellen vergleichen. Acht unverwechselbare Pattern verwenden; Hosttext, GUI, Audio und gespeicherte Auswahl abgleichen. Nullframeflush, Stopp/Resume, mehrere Punkte pro Block, Loop, Offlineexport und Save/Reopen. Restoreüberlappung aus N07 gezielt im Harness synchronisieren.
5. **Livekit und Stop:** Zwei Kits mit gegensätzlichen Audiosignaturen, anderen Steps, Mixen und Gates unter laufendem Transport mehrfach wechseln. Kleine/große Kits, Fehlerkandidat und volle Voices; Stummstrecken/Dropouts und Events aufzeichnen. Restore→genau ein laufender Block→Stop für N08. Alle16Samples nach neuem Instanz-/Songreload messen; Samplepfade dürfen nicht erneut benötigt werden.
6. **Import/Erhalt:** WAV PCM8/16/24/32, Float32, Extensible und FLAC mono/stereo; fremde/hohe Samplerates, unbekannte FLAC-Länge, deklarierter Längenwiderspruch, beschädigter letzter Frame, Nearlimit/Oversize und Gesamtkits knapp unter/über64MiB. Fehlversuch darf bisherigen Sampleklang, Namen und Steps nicht verlieren. Pickerabbruch, Explorer- sowie interner Browserdrop separat testen, inklusive Unicode/langer Namen und Zielrow bei150 %.
7. **Routing/Gates:** Alle15AUX aktiv, MAIN aus, AUX15 allein, inaktive Lücken, Busaktivierung/Reaktivierung und Songrecall. Einzelne Rows nacheinander mit eindeutigen Signaturen: korrekter Bus, kein Übersprechen, keine MAIN-Doppelung. Mute/Solo-Kombinationen, versteckter Solo-Recall und64-Voice-Choke testen.
8. **Echtzeit/Dauer:** Kleine und wechselnde Buffer,44,1/48/96/192kHz,64 lange Stereo-Voices, hohe SRC-Ratio und mehrere Instanzen bei UI-/Import-/Automationstress. Daueraufzeichnung plus mean/p95/p99/max, Deadline, Overruns, Dropouts, verlorene Trigger und RAM-Verlauf. Ungepacten Bounce gesondert vergleichen. Eine CTest-Gesamtlaufzeit oder allein niedrige ASIO-Mittellast genügt nicht.
9. **Dirty/Lifecycle/Legacy:** Unveränderten Song mit jedem nichtautomatisierten Edit verändern; Modified-/Autosave-/Undoverhalten prüfen. Zoom-only-Änderung und Projektzoomrecall. Wiederholtes tatsächliches Attach/Close/Reopen, Deactivate/Activate,48→44,1kHz und zurück. DMR1–DM10 mit vorher verstellten modernen Controllerfeldern laden; Anzeige, nächster Edit und Audio müssen zusammenpassen.

## 12. Arbeitsprotokoll und Grenzen

Die Auditgrundlage wurde aus dem tatsächlichen Branchcommit isoliert gelesen; Produktions-/Testdateien wurden nicht verändert. Beide vorigen Berichte und alle acht späteren Commits wurden verglichen. Der Branch-HEAD wurde vor Veröffentlichung erneut geprüft. Veröffentlicht wird ausschließlich diese Markdown-Datei mit einem Dokumentationscommit und CI-Skip; es wurden weder Reparaturen, Teständerungen, PRs noch Builds ausgelöst.

Die Definition eines PASS ist absichtlich enger als „Source sieht plausibel aus“. Ein ausführlicher Quellen-Audit ist möglich, eine vollständige praktische Funktionsfreigabe in dieser Umgebung nicht. Alle nicht ausgeführten Windows-/Studio-One-/Realtimepfade sind deshalb ausgewiesen. Dieser Bericht ersetzt den vorherigen Status zum Entwicklungscommit6b8fa54; dessen historischer Inhalt bleibt über die Commit-URL zugänglich.

## Quellen

[original]: https://github.com/challanger2000/125A-Drumator/blob/491f45ebcb3028e607897cf838f5b9e5eba702da/TECHNISCHER-AUDIT.md
[previous]: https://github.com/challanger2000/125A-Drumator/blob/04e8b285afe0b5b151ef9d4d275201ea0fc144ff/CODEX-FINAL-AUDIT.md
[issue]: https://github.com/challanger2000/125A-Drumator/issues/1
[run]: https://github.com/challanger2000/125A-Drumator/actions/runs/39085666516
[job]: https://github.com/challanger2000/125A-Drumator/actions/runs/39085666516/job/117551579655
[sdk-open]: https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L2053
[sdk-frame-cache]: https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L89
[sdk-frame-zoom]: https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L248
[sdk-pattern]: https://github.com/steinbergmedia/vst3_pluginterfaces/blob/4f547e8e102b47de4a8b8aaf343c73b700786372/base/futils.h#L93
[sdk-state]: https://github.com/steinbergmedia/vst3_pluginterfaces/blob/4f547e8e102b47de4a8b8aaf343c73b700786372/vst/ivstcomponent.h#L197
[sdk-scale]: https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L588

Zusätzliche Primärverträge: [SDK-Normalwert→diskreter Plainwert][sdk-pattern], [setState auf UI-Thread auch im Processing-Zustand][sdk-state], [SDK-ContentScale-Implementierung][sdk-scale].

[A:24]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/AtomicPatternBank.h#L24
[B:24]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/KitState.h#L24
[C:23]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L23
[C:32]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L32
[C:38]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L38
[C:51]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L51
[C:71]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L71
[C:105]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L105
[C:110]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L110
[C:120]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L120
[C:135]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L135
[C:144]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L144
[C:155]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L155
[C:164]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L164
[C:170]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L170
[C:192]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L192
[C:214]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorController.cpp#L214
[E:11]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorEditor.h#L11
[E:14]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorEditor.h#L14
[E:18]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorEditor.h#L18
[F:27]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/FlacImport.cpp#L27
[F:36]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/FlacImport.cpp#L36
[F:46]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/FlacImport.cpp#L46
[F:49]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/FlacImport.cpp#L49
[F:57]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/FlacImport.cpp#L57
[G:6]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/PatternQuantization.h#L6
[H:107]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L107
[H:115]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L115
[H:127]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L127
[H:765]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L765
[H:777]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L777
[H:794]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L794
[H:817]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L817
[H:851]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L851
[H:882]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L882
[H:928]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L928
[H:951]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L951
[H:1001]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L1001
[H:1010]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L1010
[H:1056]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/PackagedAudioHostSmoke.cpp#L1056
[K:35]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L35
[K:39]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L39
[K:160]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L160
[K:224]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L224
[K:261]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L261
[K:269]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L269
[K:293]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleVoiceEngine.h#L293
[L:14]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/VisibleRows.h#L14
[P:28]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L28
[P:355]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L355
[P:356]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L356
[P:372]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L372
[P:382]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L382
[P:394]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L394
[P:400]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L400
[P:405]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L405
[P:408]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L408
[P:451]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L451
[P:468]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L468
[P:521]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L521
[P:537]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L537
[P:588]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L588
[P:610]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L610
[P:614]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L614
[P:618]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L618
[P:635]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorProcessor.cpp#L635
[Q:45]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/EditorGeometrySmoke.cpp#L45
[R:31]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/VisibleRowsTests.cpp#L31
[R:46]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/VisibleRowsTests.cpp#L46
[R:52]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/VisibleRowsTests.cpp#L52
[R:64]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/VisibleRowsTests.cpp#L64
[S:131]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleExchange.h#L131
[S:143]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleExchange.h#L143
[S:169]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleExchange.h#L169
[S:195]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/core/SampleExchange.h#L195
[T:33]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L33
[T:96]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L96
[T:134]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L134
[T:252]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L252
[T:268]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L268
[T:308]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L308
[T:310]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/tests/SampleVoiceTests.cpp#L310
[V:65]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L65
[V:84]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L84
[V:87]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L87
[V:88]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L88
[V:96]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L96
[V:100]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L100
[V:108]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L108
[V:137]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L137
[V:177]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L177
[V:277]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L277
[V:307]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L307
[V:314]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L314
[V:320]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L320
[V:323]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L323
[V:329]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L329
[V:334]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L334
[V:347]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L347
[V:348]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L348
[V:362]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L362
[V:376]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L376
[V:384]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L384
[V:388]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/src/vst3/DrumatorView.cpp#L388
[W:31]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/.github/workflows/core-ci.yml#L31
[W:36]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/.github/workflows/core-ci.yml#L36
[W:110]: https://github.com/challanger2000/125A-Drumator/blob/32f68ca7b2c3dc395c5a68efeb991760c373d4ac/.github/workflows/core-ci.yml#L110

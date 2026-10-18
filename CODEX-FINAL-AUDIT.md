# 125A Drumator – unabhängiger Abschluss-Audit vor der Vintage-GUI

**Prüfdatum:** 18.10.2026 (Europe/Berlin).

**Repository/Branch:** `challanger2000/125A-Drumator`, `develop/v1.0.0`.

**Geprüfter Branch-HEAD:** **`6b8fa5446512f89262c109866d1c5443faac1945`**.

**Pluginversion:** weiterhin `0.1.0`; keine finale V1-Freigabe.

**Vergleich:** `TECHNISCHER-AUDIT.md`, F01–F17, Prüfstand `a64c94a117ae7b485450b9367b7703039b949fa4`.

## 1. Entscheidung

**Die funktionale Entwicklung ist noch nicht belastbar abgeschlossen. Eine Freigabe zur Implementierung der endgültigen Vintage-GUI wird nicht erteilt.**

Die Reparaturen verbessern den Stand wesentlich: Die falsche 1100-Pixel-Childbreite, die numerischen Klicklücken, der Win32-Drop-Längenfehler, der ursprüngliche Flushverlust und die fehlenden Legacy-Defaults wurden im tatsächlichen Code angegangen. Der aktuelle Windows-Workflow ist erfolgreich. Das ist ein positiver Entwicklungsnachweis, aber keine vollständige Funktions-, Echtzeit- oder Studio-One-Abnahme.

Es bestehen weiterhin konkrete Fehler. Besonders relevant sind:

1. **P1 – Neue Pattern-Quantisierungsabweichung:** Der Host-Stringparameter und Prozessor/GUI interpretieren Zwischenwerte unterschiedlich. Bei `0,10` nennt der Host Pattern **A**, während Prozessor und GUI **B** auswählen. Der neue A–H-Test prüft nur die acht kanonischen Werte und übersieht das.
2. **P1 – Unvollständiger Resize-Rollback:** Wird beim Hinzufügen eine belegte, zuvor versteckte Zeile sichtbar und der Host lehnt die Vergrößerung ab, scheitert die Rücknahme an der normalen „nur leere Zeile ausblenden“-Sperre. Der neue Hilfsfunktionstest bildet diese reale Controllerbedingung nicht ab.
3. **P2 – Abgelehnter Zoom kann Modell und Darstellung trennen:** Der Editor kann „150%“ melden, obwohl der Frame nach Resize-Ablehnung bei 100% bleibt.
4. **P2 – FLAC-Vollständigkeit ungeprüft:** Eine kleinere tatsächliche Framezahl als die deklarierte wird weiterhin nicht ausdrücklich abgewiesen.
5. **P2/P3 – Zoompräferenz und Hitboxränder:** Zoompersistenz fehlt weiterhin; kleine Draw-/Hit-Rechteckabweichungen bleiben bestehen.

Zusätzlich fehlen belastbare Nachweise für die angehängte native GUI, komplette Kitwechsel unter laufendem Audio, die vollständige 16-Ausgangs-Matrix und gepacte Echtzeit-Deadlines. Der Bericht trennt diese offenen Nachweise von den belegten Fehlern.

## 2. Prüfgrundlage und Beweisgrenzen

### 2.1 Verbindliche Regeln und Umfang

`125A-Engineering/START-HERE.md` und `AGENTS.md` wurden zuerst gelesen. Der Engineering-Stand ist unverändert bei `5f13a850c065167526519f0249fb7a38b5841a6a`. Die bereits im Erstaudit gelesenen einschlägigen Guardrails und Standards bleiben anwendbar; insbesondere wurden die Vorgaben zu geeigneten unabhängigen Tests, GUI-Traceability, Host-Verträgen und Echtzeitmessung berücksichtigt. Die AI-Engineering-, Release-QA- und Realtime-QA-Vorgaben wurden erneut abgeglichen.

Untersucht wurden alle **23 Produktionsquelldateien** im Vergleich zum Erstaudit: 13 sind bytegleich geblieben, 10 wurden geändert oder neu hinzugefügt. Dazu wurden die zehn Testdateien, CMake, UIDESC, Workflow und der bisherige Auditbericht abgeglichen. Der neue Stand verändert gegenüber dem veröffentlichten Erstaudit 13 Implementierungs-/Testdateien. Die unveränderten Scheduler-, Codec-, Exchange- und Routinggrundlagen wurden nicht allein deshalb als fehlerfrei behandelt, weil sie unverändert sind.

Die SDK-Verifikation erfolgt gegen die tatsächlich gelesenen, gepinnten Quellen:

| Abhängigkeit | Commit |
|---|---|
| VST3 SDK 3.8.1 | `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| public.sdk | `586dc5e6c8012c3e4b01c79389375cbe96bdb1da` |
| pluginterfaces | `4f547e8e102b47de4a8b8aaf343c73b700786372` |
| VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| dr_flac | Datei aus Upstream-Commit `dfe8377631000664666519fdb83da193fd8037f4` |

### 2.2 Bedeutung der Statusangaben

- **PASS:** Der ausdrücklich bezeichnete Fehlerfall ist durch einen geeigneten vorhandenen Test und den zugeordneten erfolgreichen Windows-Durchlauf belegt. Das ist keine pauschale Pluginfreigabe.
- **TEILWEISE:** Eine plausible beziehungsweise nachvollziehbare Reparatur ist vorhanden, aber der vollständige Befund oder seine tatsächliche Interaktionskette ist nicht ausreichend verifiziert.
- **FAIL:** Ein konkreter verbleibender oder neuer Fehler ist am tatsächlichen Quell-/SDK-Pfad und einem nachvollziehbaren Gegenbeispiel belegt. Ein solcher Quellenbeweis wird nicht als hier ausgeführter DAW-Test bezeichnet.
- **OFFEN:** Der nötige Praxis-, Binär- oder Messnachweis ist mit dem vorliegenden Material nicht abschließend möglich.

Die folgenden Kürzel verweisen auf konkrete Dateien und Zeilen des oben festgehaltenen Prüfcommits: **C** Controller, **V** View, **P** Prozessor, **K** Voice/SRC, **F** FLAC, **H** packaged Host-Smoke, **T** Voice-Tests, **R** VisibleRows-Tests, **B** KitCodec, **E** Editor, **X** Resize-Policy, **Q** EditorGeometry-Smoke, **S** SampleExchange. Jeder verlinkte Zeilenverweis führt zu einer unveränderlichen Commit-URL.

### 2.3 Bestehende Windows-Evidenz

Der vorhandene [Windows-Run #39081934139](https://github.com/challanger2000/125A-Drumator/actions/runs/39081934139), Workflowlauf #178, ist eindeutig dem geprüften Commit **`6b8fa5446512f89262c109866d1c5443faac1945`** zugeordnet. Die zugängliche Run-Zusammenfassung und der [Windows-Job](https://github.com/challanger2000/125A-Drumator/actions/runs/39081934139/job/117540495952) zeigen **Success/succeeded**; Gesamtdauer 2 min 53 s. Es wird ein Artefakt angezeigt.

Der unveränderte Workflow baut acht CTest-Executables sowie zwei separate Windows-Smokes, führt CTest und anschließend Validator, Editor-Smoke und Audio-Host-Smoke mit Fehlerprüfung aus. Deshalb werden vorhandene konkrete Testassertionen als eng begrenzte CI-Evidenz berücksichtigt. **Die vollständigen individuellen Logs sind im verfügbaren Webzugang an eine Anmeldung gebunden und wurden nicht erneut ausgelesen.** Es werden keine neuen Einzelmesswerte aus diesen Logs erfunden. Das Artefakt wurde hier nicht heruntergeladen oder ausgeführt; seine angezeigte Existenz ersetzt keine Binärprüfung.

### 2.4 In diesem Audit tatsächlich ausgeführt

- Read-only Git-, Quell-, SDK- und Testabgleich mit dem aktuellen Branch-HEAD.
- Unabhängige Python-Gegenprüfungen von Rechtecken, Resize-/Zoomzuständen, UTF-8-Kürzung und den unterschiedlichen Pattern-Abbildungen.
- Unabhängige numerische DTFT-Berechnung der aktuellen SRC-Koeffizientenformel.
- Erzeugung einer externen FLAC-Gegenfixture aus dem vorhandenen Testfixture und Untersuchung mit dem installierten `ffprobe`: deklarierte und tatsächliche Framezahl getrennt erfasst.

**Nicht hier ausgeführt:** C++-Regressionstests, Windows-VST3, native Editor-Attaches/Klicks, Studio One, 125A Plugin Tester, eigene Validatorläufe oder gepacte Echtzeitmessungen. Die Windows-only-Projektregel wurde respektiert; es wurde kein Linux-Pluginbuild als Ersatz erzeugt. Die analytischen Modelle sind keine Ausführung des Plugin-Codes. Die neuen Prüfungen verändern keine bestehenden Tests.

## 3. Status sämtlicher ursprünglicher Befunde F01–F17

| Befund | Status | Implementierungsprüfung und verbleibender Nachweis |
|---|---|---|
| **F01 – Childbreite/Clipping** | **TEILWEISE** | [V:95] und [V:108] verwenden jetzt 1280; Konstruktor, Parent und UIDESC passen dazu. Alle 32 Stepfüllungen liegen rechnerisch innerhalb dieser Breite. Der vorhandene Smoke [Q:43] misst weiterhin nur die ungeöffnete äußere View. Kein Attach-/Draw-/Randklicktest der tatsächlichen Child-View bei 1/4/16 Zeilen und beiden Zooms. Der konkrete 1100-Widerspruch ist im Quellstand beseitigt; praktische GUI-Abnahme fehlt. |
| **F02 – Klicklücken/Hitboxen** | **FAIL** | Numerische Lücken werden jetzt abgewiesen [V:364], A–H-Abstände ebenfalls [V:323]; Patterntexte verwenden die richtige Breite [V:140]. Der Headerbereich wurde jedoch pauschal auf y=38…69 erweitert [V:320]: Patternflächen enden gezeichnet bei 68, Zoom beginnt gezeichnet erst 39. Zwei zusätzliche aktive Randstreifen bleiben, siehe N04. Keine echte negative Randklickregression. |
| **F03 – Host→GUI/State-Relayout** | **TEILWEISE** | Registrieren/Abmelden von Views, `notifyViews`, Override von `setParamNormalized` und State-Relayout sind vorhanden [C:22], [C:31], [C:98], [V:82]. Der neue Test [H:780] prüft Controllerwerte und erzeugt drei **ungeöffnete** Editorobjekte. Er erzeugt keine angehängte `DrumatorView` und prüft weder Invalidierung noch sichtbaren Rasterwechsel. Rejected-Resize bei State-Recall bleibt unaufgelöst. |
| **F04 – Null-Sample-Flush** | **PASS** | [P:334] verarbeitet Parameter vor allen Audiopufferbedingungen. [H:814] sendet einen echten Zero-Frame-/Zero-Output-`ProcessData`-Aufruf und prüft nach `getState` Auswahl5 statt nur den Rückgabecode. Dieser konkrete ehemalige Datenverlust ist im erfolgreichen Windows-Durchlauf sinnvoll abgesichert. Mehrpunkt-/Resume-/Offline-Flushmatrix zusätzlich sinnvoll, aber kein verbleibender ursprünglicher Frühreturnfehler erkennbar. |
| **F05 – Host Dirty** | **TEILWEISE** | Alle früheren Edit-Nachrichten gehen nun über `sendEditMessage`, das bei Erfolg `componentHandler2->setDirty(true)` aufruft [C:37]. Die neue Regression [R:45] prüft ausschließlich die generische Bool-/Callback-Hilfsfunktion. Sie registriert keinen echten oder Mock-`IComponentHandler2`, ruft keinen Controlleredit auf und misst keine tatsächliche Hostbenachrichtigung. Reale Song-Modified-/Autosave-Abnahme fehlt. |
| **F06 – Legacy-Controllerstate** | **TEILWEISE** | DMR1 setzt das Factoryraster neu auf [C:115]; DMR1–3 setzen nun Pitch und Choke zurück [C:125]. Das passt zum Prozessor [P:543]. Die vorhandenen Codec-Migrationstests für DMR4–9 prüfen diese Controllerpfade nicht. Keine neue Regression „veränderter moderner Controller → DMR1/2/3 → Anzeige/nächster Edit/Prozessorvergleich“. |
| **F07 – Unsichtbares Solo nach Hide** | **TEILWEISE** | `canHideLast()` verweigert jetzt eine Solo-Zeile [C:171]. Der ursprüngliche unmittelbare Bedienweg ist dadurch gesperrt. Die VisibleRows-Tests verwenden aber die ältere Corepolicy ohne Controller-Solozustand; ein Controller-Hide-/Recalltest fehlt. Schon gespeicherte Zustände mit Solo oberhalb der sichtbaren Zeilen werden vom Codec weiterhin akzeptiert; deren Bedien-/Migrationspolitik bleibt offen. |
| **F08 – Aux ohne MAIN-Puffer** | **TEILWEISE** | Prozessor und Renderer erlauben zwei Null-MAIN-Pointer [P:355], [K:160]. Der Coretest [T:288] prüft tatsächlich L/R-Amplitude am Stem. Der neue Pakettest [H:844] prüft hingegen nur Erfolg und endliche Werte, lässt also auch reine Nullen passieren. Kein vollständiger 16-Bus-Pakettest mit gemessenen Auxwerten und inaktiven Lücken. Der frühere Abbruch ist repariert; die vollständige Routingbehauptung ist noch nicht belegt. |
| **F09 – Host-DPI im Resize** | **TEILWEISE** | `DrumatorEditor::effectiveScale()` [E:9] macht den tatsächlichen SDK-Gesamtfaktor zugänglich; [V:100] verwendet ihn. Das ist mathematisch der richtige Faktor `UI-Zoom × ContentScale`. Es gibt keinen nativen ContentScale-/Monitorwechsel-/Hostrect-Test. |
| **F10 – Resize-Ablehnung** | **FAIL** | Ein boolesches Resizeergebnis und eine Rollbackpolicy existieren [V:101], [X:6]. Der reale Revert des Hinzufügens ist aber `hideLastEmptyRow()` [V:340] und kann bei belegter versteckter Zeile scheitern. Die neue Regression [R:30] ersetzt diesen Pfad durch bedingungsloses `--rows` und deckt den Gegenfall nicht ab. Siehe N01. Für Attach/State-Relayout existiert außerdem kein bedienbarer Fallback bei Ablehnung. |
| **F11 – Ctrl+Links-Reset** | **TEILWEISE** | Formeln für Swing/Humanize/Gain/Pan/Pitch sind jetzt vorhanden [V:307], [V:371] und führen rechnerisch zu 0/0/100/0/0. Kein Test sendet native Ctrl-Klicks an beide Hälften aller entsprechenden Zeilenfelder und prüft Audio/State. Die Implementierung ist erkennbar, die Interaktionsabnahme fehlt. |
| **F12 – UTF-8-Kürzung** | **TEILWEISE** | Präfix/Suffix werden jetzt an UTF-8-Codepointgrenzen getrennt [V:177]. Das frühere `ä`-Gegenbeispiel und weitere CJK-/Emoji-/Kombinationsfälle ergeben im unabhängigen Modell gültiges UTF-8. Graphemgrenzen, tatsächliche Fontbreite und Clipping werden weiterhin nicht gemessen. Keine entsprechende neue Repositoryregression. |
| **F13 – FLAC-Dekodiergrenzen** | **TEILWEISE** | Metadaten werden vor Vollausgabe geprüft und PCM wird in 4096er-Blöcken mit Framebudget gelesen [F:27], [F:36], [F:47]. Der große unbeschränkte Full-Read-Pfad ist entfernt. [H:736] prüft eine riesige deklarierte Länge plus Erhalt des alten Outputs. Unbekannte Länge, echte Oversize-Datei, Stereo-/Mehrkanalgrenzen, Allokations-/Zeitmessung und Vollständigkeit werden nicht ausreichend geprüft; N03 bleibt relevant. |
| **F14 – SRC/Aliasing** | **TEILWEISE** | Ratioabhängige 32/64/128/256 Taps verbessern die beiden alten Gegenfälle erheblich [K:39]. Der neue Test [T:33] misst aber nur eine Phase/Frequenz bei Ratio 16. Der Stress [T:252] ist ein Durchsatz-/Hangtest mit 20-s-Grenze und ausdrücklich kein Realtime-Gate. Übergangsband, fractional ratios/phases, Audiofixtures und p99/Deadlines sind nicht freigegeben; siehe Abschnitt6. |
| **F15 – Zoomanzeige/Persistenz** | **FAIL** | Die Auswahl wird jetzt farbig gezeichnet [V:142]; neue Editorobjekte werden jedoch weiterhin ohne gespeicherten Zoom angelegt [C:131]. Kein Controller-Zoomstate, keine Wiederherstellung. Ein abgelehnter Zoom kann außerdem eine falsche Auswahl anzeigen, N02. Der zugehörige Commit bezeichnete die Änderung selbst als partiell. |
| **F16 – A–H-Hosttexte** | **FAIL** | `StringListParameter` ist eingeführt [C:52], und [H:103] prüft alle acht `p/7`-Texte. Dessen SDK-Quantisierung unterscheidet sich aber von `lround(value*7)` in [C:152] und [P:406]. Hosttext/Plainwert und tatsächlich gewähltes Pattern widersprechen sich bei Zwischenwerten. Neue Regression N05. |
| **F17 – Win32-Dateidrop** | **TEILWEISE** | [V:65] und [V:278] übernehmen jetzt die gemeldete Länge, entfernen optional einen letzten Terminator und weisen innere NULs ab. Das passt zum gepinnten Win32-Datenpaket. Es gibt aber keinen neuen `IDataPackage`-/Drop-Regressionstest und keinen Explorer-/Studio-One-Nachweis über den tatsächlichen Zeilenladepfad. |

**Wichtig:** TEILWEISE bedeutet nicht, dass die Reparatur falsch sein muss. Es bedeutet, dass der vollständige Fehlerfall einschließlich seiner behaupteten Wirkung nicht hinreichend nachgewiesen ist. Umgekehrt wird ein konkreter neuer Widerspruch nicht durch einen grünen Workflow zum PASS.

## 4. Neue beziehungsweise weiter konkretisierte Fehler

### N01 – P1 / FAIL: Rollback benutzt eine gesperrte Benutzeraktion

**Quellen:** [V:337], [C:168], [C:193], [X:6]; Testgegenstück [R:30].

Der neue Add-Row-Vorgang ist:

1. `controller_.addRow()` erhöht und publiziert die sichtbare Zeilenzahl.
2. `adjustEditorHeight()` verlangt ein größeres Fenster.
3. Bei Ablehnung soll `controller_.hideLastEmptyRow()` den alten Zustand zurücksetzen.

Der dritte Schritt ist keine uneingeschränkte Transaktionsrücknahme. Er ist die normale Benutzerfunktion mit Sample-, Step- und Solosperre.

**Konkreter zulässiger Gegenfall:** Ein DM10-Kit hat `visible=4`, aber ein Sample oder Steps auf Zeile 5. Der Codec erlaubt das ausdrücklich: Alle 16 Engineplätze werden gespeichert, Sichtbarkeit ist davon getrennt. Der bestehende Pakettest lädt sogar alle 16 Samples, ohne zuvor die sichtbaren vier Zeilen zu erweitern [H:274]. Nach Recall dieses Kits klickt der Nutzer Plus. Zeile 5 wird sichtbar. Der Host lehnt die Vergrößerung von 300 auf 317 logische Pixel ab. Der Revert sieht nun die belegte letzte Zeile und verweigert das Ausblenden. Ergebnis: Sichtbarkeit 5 bleibt bestehen, Fenster 300; die neue Aktionsleiste liegt bei y258; nach dem ersten Fehlversuch ist zunächst vor allem der Status unterhalb y300 abgeschnitten. Bei wiederholtem Plus auf weitere belegte versteckte Zeilen wächst die Inkonsistenz: 4→5→6→7 bei unverändert 300 Pixeln Höhe. Die Plusfläche bei sechs Zeilen ist mit y287…300 noch teilweise erreichbar; danach liegt die Aktionsleiste bei y316…345 vollständig außerhalb und die siebte Zeile ist teilweise abgeschnitten. Der normale Bedienweg kann dadurch tatsächlich unzugänglich werden.

Der Code zeigt zwar „ZEILEN-ROLLBACK FEHLGESCHLAGEN“, behebt den Zustand aber nicht. Das unabhängige Policymodell ergibt: Start 4 → Add 5 → Resizefalse → Hidefalse → End 5.

**Warum der neue Test das übersieht:** Seine Revertlambda dekrementiert bedingungslos einen `int`. Er kennt weder Labels noch Raster noch Solo und ruft keinen Controller auf. Der Testname darf nicht als Prüfung des tatsächlichen Controller-/Hostpfads gelesen werden.

**Empfehlung:** Eine echte Rücknahme der ursprünglichen Sichtbarkeit beziehungsweise eine erst nach erfolgreicher Fensterverhandlung wirksame Änderung vorsehen. Normale Lösch-/Hide-Sicherungen nicht pauschal entfernen. Regression über tatsächlichen Controller, gültiges Kit mit belegter versteckter Zeile und ablehnenden `IPlugFrame`; Modelle, State und reale Fenstergröße müssen konsistent bleiben. Keine Korrektur wurde hier durchgeführt.

### N02 – P2 / FAIL: Abgelehnter Zoom lässt den Editorwert zurück

**Pluginquellen:** [V:328], [V:142], [V:100].

**SDK-Primärquellen:** `vst3editor.cpp:600–613`, `cframe.cpp:248–275` und `cframe.cpp:921–944` im gepinnten VSTGUI.

`VST3Editor::setZoomFactor()` setzt zuerst `zoomFactor=factor`. Es ruft danach `CFrame::setZoom()` auf, wertet dessen Fehler aber nicht aus. `CFrame::setZoom()` stellt bei einer abgelehnten Größenänderung seinen alten Transform wieder her. Drumators direkter Aufruf erhält keinen Erfolgscode und führt weder einen eigenen Abgleich noch eine Rücknahme durch.

**Gegenfall:** Bei 100% lehnt der Host den Wechsel auf 150% ab. Der Frame bleibt tatsächlich bei 1,0, während `getZoomFactor()` 1,5 liefert. Der neue Marker [V:142] markiert 150%; ein späterer Rowresize rechnet mit 1,5. Das ist ein konkreter SDK-/Pluginpfad, kein hier ausgeführter Studio-One-Test.

**Empfehlung:** Nach der Zoomverhandlung tatsächliche Framegröße/Transformation, Editorwert und ausgewählte Anzeige konsistent halten. Einen ablehnenden Resizehost und anschließend weitere Row-/Zoomaktionen testen. SDK-Verhalten nicht allein aus dem gespeicherten `getZoomFactor()` als erfolgreich interpretieren.

### N03 – P2 / FAIL des Vollständigkeitsvertrags: Dekodierte FLAC-Länge nicht mit Metadaten verglichen

**Quellen:** [F:38], [F:47], [F:55], [H:736]. Decoderprimärquelle: `drflac_read_pcm_frames_f32`, gepinnte `dr_flac.h:11701ff`.

Die neue Begrenzung verhindert die frühere unbeschränkte PCM-Ausgabe. Sie prüft jedoch nur `totalPCMFrameCount > maxFrames`. Anschließend wird gelesen, bis der Decoder 0 liefert. Nach `close()` wird lediglich `framesRead != 0` gefordert; ein Vergleich mit einer bekannten deklarierten Gesamtzahl fehlt. Der gelesene Upstreamdecoder beendet einen nicht weiter dekodierbaren Stream und gibt bereits gelesene Frames zurück.

**Konkrete Gegenfixture:** Das vorhandene 98-Byte-Test-FLAC enthält 128 Frames. Nur die 36-Bit-Gesamtframezahl in STREAMINFO wurde von 128 auf 256 geändert, alle Audioframes blieben gleich. Eine unabhängige `ffprobe`-Prüfung zeigt `duration_ts=256`, aber genau einen dekodierten Frame mit `nb_samples=128`. Beide Werte liegen weit unter den neuen Größenlimits. Der Wrapperpfad speichert 128 gelesene Frames und besitzt keinen Zweig, der den Widerspruch 256≠128 abweist.

**Beweisgrenze:** Die Fixture wurde unabhängig untersucht und der gepinnte Decoder-/Wrapperkontrollfluss geprüft. Der Windows-Drumator-Wrapper wurde hier nicht ausgeführt. Es wird kein beobachteter Studio-One-Datenverlust behauptet. Belegt ist die fehlende Vollständigkeitsbedingung samt konkreter Eingabe; deren Ablehnung sollte vor dem Überschreiben des bisherigen Samples gefordert werden.

**Empfehlung:** Bekannte deklarierte Gesamtlänge vor dem Schließen aufbewahren und vollständige Dekodierung überprüfen; unbekannte Länge getrennt behandeln. Tests mit zu großer, zu kleiner und unbekannter deklarierter Länge sowie echten Mehrframe-Dateien mit abgeschnittenem Ende. Der bestehende „enorm große Metadaten“-Test trifft diesen kleinen, durch die Budgetprüfung zulässigen Gegenfall nicht.

### N04 – P3 / FAIL: Header-Hitbox wurde für zwei verschiedene y-Bereiche vereinheitlicht

**Quellen:** Zeichnung [V:137], [V:144]; Dispatch [V:320].

Die Patternflächen haben y=38…68, die Zoomflächen y=39…69. Der neue gemeinsame Headerzweig akzeptiert y=38…69. Damit wählt `(350,68.5)` A unterhalb seiner gezeichneten Fläche; `(1080,38.5)` aktiviert 100% oberhalb seiner Fläche. Diese kleinen Restabweichungen sind kein schwerer Audiofehler, widersprechen aber der geforderten identischen Draw-/Hit-Geometrie.

**Empfehlung:** Die tatsächlichen Zielrechtecke auch zum Hit-Test verwenden. Negative Randfälle pro Buttontyp prüfen, statt einen gemeinsamen Bereich zu verlängern. Keine neuen UI-Geometrie-/Mausregressionen wurden hierfür gefunden.

### N05 – P1 / FAIL: Neue SDK-Stringliste und bestehender Scheduler verwenden verschiedene diskrete Abbildungen

**Pluginquellen:** [C:52], [C:149], [P:342], [P:405]; Test [H:103].

**SDK-Primärquellen:** `public.sdk/source/vst/vstparameters.cpp:291–330` und `pluginterfaces/base/futils.h:93–97`, jeweils im oben angegebenen Commit.

Der neue `StringListParameter::toString()` verwendet `toPlain()`, und dieses verwendet das SDK-`FromNormalized(value,7)`:

```text
Host-Plainwert / Textindex = min(7, int(value * 8))
Drumator-Prozessor / GUI   = lround(value * 7)
```

Für nichtnegative normalisierte Werte ist `int` hier Abschneiden nach unten. Die Grenzstellen unterscheiden sich. Konkrete Beispiele:

| Normalisierter Wert | Host-String/Plainindex | Prozessor- und GUI-Auswahl |
|---:|---|---|
| 0,10 | A /0 | B /1 |
| 0,124 | A /0 | B /1 |
| 0,875 | H /7 | G /6 |
| 0,90 | H /7 | G /6 |
| 0,929 | H /7 | H /7 |

Ein unabhängiger Vergleich an 1001 Werten `i/1000` fand 213 Abweichungen. Das ist keine Ausführung des Plugins, sondern die direkte Gegenprüfung der beiden tatsächlichen Implementierungsformeln. Alle acht kanonischen Werte `p/7` stimmen überein; genau deshalb bleibt der neue Regressionstest grün.

**Reproduktion im nativen Testhost:** `getParamStringByValue(1000,0.10)` und `normalizedParamToPlain(1000,0.10)` abfragen; denselben Wert als Patternparameter liefern und anschließend `getState().selected` beziehungsweise Audio mit deutlich unterschiedlichen A/B-Patterns prüfen. Dasselbe für 0,90. Die Inkonsistenz betrifft auch alte nichtkanonische Automationswerte; sie darf nicht pauschal als ungültiger Hostinput abgetan werden, denn die öffentliche Normalized-/Plain-API definiert gerade deren Abbildung.

**Empfehlung:** Eine gemeinsame verbindliche Quantisierung für Text, Plainwert, GUI, Flush, laufende Automation und Stateübernahme festlegen. Wegen der vorhandenen Projekte nicht unbesehen die bisherigen DSP-Auswahlgrenzen ändern. Neben `p/7` alle Grenzstellen und ihre Nachbarn testen; zwischen Hostanzeige und hörbarem Pattern muss dieselbe Auswahl gelten.

## 5. Was die Regressionstests tatsächlich belegen – und was nicht

| Neuer/berufener Test | Geeigneter Teilnachweis | Fehlender oder schwacher Teil |
|---|---|---|
| F03, [H:780] | Public-Controllerwert und ComponentState können gesetzt/gelesen werden; Ressourcenrechteck vorhanden | Kein `IPlugView::attached`, kein HWND, keine Custom-View, kein Render, keine Invalidierungsbeobachtung. `views_` ist in diesem Pfad leer. |
| F03, drei `createView`/`release`, [H:801] | Lebensdauer dreier ungeöffneter Editorobjekte | Kein echter Close/Reopen-Zyklus eines nativen Editors; keine Registrierung/Abmeldung der tatsächlichen Child-View getestet. |
| F04, [H:814] | Zero-Frame/Zero-Output wird vom echten Paketprozessor verarbeitet; gespeicherte Auswahl wird geprüft | Nur ein Wert und keine anschließende Audio-Resume-Messung; die wichtigste ursprüngliche Assertion ist dennoch funktional geeignet. |
| F05, [R:45] | Ein Callback wird bei Booltrue, nicht bei Boolfalse ausgeführt | Kein Hosthandler, kein Nachrichtenversand, kein Controlleredit, kein tatsächlicher Dirty-Call gemessen. |
| F08, [T:288] | Voicecore liefert bekannte L/R-Amplitude ohne MAIN | Nicht die gesamte SDK-/Bus-/Adapterkette. |
| F08, [H:844] | Paketprozessor bricht ohne MAIN nicht mehr ab und liefert keine NaNs | Ausgabearrays beginnen mit 0; nur `isfinite` wird geprüft. Ein vollständig stummer Aux würde bestehen. Kommentar bezeichnet Bus1 außerdem missverständlich als row1 statt sichtbarer Zeile 2. |
| F10, [R:30] | Reihenfolge und Rückgabeklassen der generischen Transaktionspolicy | Revert kennt keine echten Hide-Sperren. Keine Ausführung von `Controller::canHideLast`, keine `IPlugFrame`-Resizeverhandlung, kein Gegenfall „versteckt und belegt“. |
| F13, [H:736] | Riesige deklarierte Framezahl wird abgelehnt; vorheriger Output bleibt erhalten | Keine Allokations-/Zeitinstrumentierung; ein langsamer Decoder mit nachträglicher Ablehnung könnte dieselbe Assertion erfüllen. Unbekannte Länge und kleine Längenwidersprüche fehlen. |
| F14, [T:33] | Bei Ratio 1 sind 32 Taps eingestellt; bei Ratio 16 misst Phase 0 am Punkt 0,04 unter −40 dB | Keine tatsächliche Audiorender-/FFT-Referenz dieses Gegenfalls, kein Sweep und kein fractional-ratio-/Phasen-/Passbandvertrag. |
| F14, [T:252] | 64 Voices bei Ratio 16 können starten, finite Samples erzeugen und nach genügend Frames auslaufen; grober Hangschutz | Eine einzige gesamte Laufzeit inklusive Kernelvorbereitung, kein per-block Timing, keine Callbackkadenz, keine Deadlines, kein p95/p99. |
| Bestehende Kit-/Codec-Tests | Roundtrip und Korruptionsablehnung, DMR4…9-Migration, Größenlimits | Keine Legacy-DMR1…3-Controllerabweichung aus verändertem Zustand und kein atomarer Kitwechsel während laufender Audiocallbacks. |
| Bestehender Editor-Smoke | Factory/Controller/View und äußeres 1280×300-Rechteck | Verifiziert weiterhin weder Childbreite nach Attach noch Zeichnung, Maus, Zoom, HiDPI oder dynamische Zeilenhöhe. |
| F16, [H:103] | Acht exakte Buchstaben bei `p/7` | Keine Zwischenwerte, Plain-API, Auswahlgrenzen oder Audio-/Statekonsistenz; N05 bleibt unsichtbar. |

Die Tests verwenden aktive Throw-/Exit-Prüfungen und beruhen nicht allein auf durch `NDEBUG` abschaltbaren Assertions. Der CMake-Sammelbuild für alle acht CTests besteht weiter. Die Diagnose „fehlende Tests“ meint deshalb vor allem **fehlende relevante Gegenfälle und Integrationspfade**, nicht pauschal eine nicht ausführbare Testsuite.

Die Ausgabe „sample-voice regression:10 groups PASS“ ist inzwischen veraltet: Es werden 13 benannte Testfunktionen nach dem zusätzlichen Empty-Samplerblock aufgerufen. Das verfälscht keine Checks, sollte aber nicht als exakte Testgruppenzählung zitiert werden.

## 6. Audioengine, SRC, Stimmen und Echtzeit

### 6.1 SRC: nachgewiesene Verbesserung, noch kein vollständiger Qualitätsvertrag

Der neue Kernel verwendet 32/64/128/256 Taps ab den Verhältnissen 1 beziehungsweise 2/4/8 [K:39]. Die Tabelle liegt nun als Vector auf dem Controlthread-Heap [K:41]; ihre Konstruktion findet nicht im Audiocallback statt. Das vermeidet die entsprechend große lokale Stacktabelle und vermeidet eine pauschale 256-Tap-Rechenschleife bei Ratio 1.

Die unabhängige komplexe Frequenzantwort aus der tatsächlichen Koeffizientenformel einschließlich Float32-Koeffizienten ergibt für Phase 0:

| Ratio | Taps | Quellfrequenz in Zyklen/Frame | Antwort |
|---:|---:|---:|---:|
|16|256|0,040000|−47,678 dB|
|16|256|0,060000|−92,328 dB|
|16|256|0,028125|−1,918 dB|
|16|256|0,034375|−14,064 dB|
|8|256|0,100000|−113,854 dB|
|4|128|0,200000|−129,309 dB|
|1|32|0,100000|0,000 dB|

Die beiden ursprünglichen Ratio-16-Gegenfälle lagen bei ungefähr −5,24 und −12,27 dB. Die Verbesserung ist also real aus der Implementierung ableitbar. Sie wird ausdrücklich anerkannt.

Die zusätzlichen Punkte zeigen jedoch, weshalb ein einzelner Stopbandpunkt nicht genügt: Bei Ratio 16 und 48 kHz Ausgabe liegt 0,028125 bei 21,6 kHz Ausgangsfrequenz im Passbandrand und wird um etwa 1,92 dB abgesenkt.0,034375 entspräche 26,4 kHz und faltet auf 21,6 kHz zurück; dieser Übergangsbereich wird nur um etwa 14,06 dB gedämpft. Ohne vorab festgelegtes nutzbares Passband, Übergangsband und Stopbandziel ist daraus weder eine pauschale Freigabe noch ein nachträglich erfundener Grenzwert abzuleiten.

**Noch erforderlich:** Audio-Sweep/FFT mit unabhängiger Referenz, Phasen und nichtganzzahlige Ratios, 44,1/48/96/192 kHz-Quellen/Hosts, Pitch 0/±12/±24, Sampleanfang/-ende und reale Drums. Den 40-dB-Einzeltest nicht mit einem vollständigen SRC-Qualitätsziel gleichsetzen.

### 6.2 CPU-/RAM-Folgen der Reparatur

Bei 64 Stimmen und 256 Taps entstehen bis zu `64×256×2=32768` Tap-/Kanalbeiträge pro Ausgabesample, also achtmal so viele wie beim bisherigen 32-Tap-Kern. Das ist eine rechnerische Lastbeschreibung, kein gemessener CPU-Prozentwert.

Die Koeffiziententabelle wird für jedes vorbereitete nichtleere Sample weiterhin auf `1024×256` Floatwerte dimensioniert, selbst wenn nur 32 Taps genutzt werden. Das sind 1 MiB pro Kernel statt zuvor 128 KiB. 16 aktuelle geladene Zeilen können dafür allein 16 MiB benötigen; alte klingende Sampleversionen und ein gerade vorbereitetes neues Kit kommen hinzu. Die Auslagerung vom Stack war korrekt, ersetzt aber kein gemessenes Gesamt-RAM-Budget.

Der neue Stress umfasst pro Zyklus 8192 aktive Ausgabeframes plus Drain. Über fünf Zyklen sind 40960 aktive Frames bei 48 kHz ungefähr 0,853 s aktive Audiolänge; mit allen 36 Blöcken je Zyklus werden 46080 Frames verarbeitet. Eine Gesamtlaufzeit unter 20 s ist ein sinnvoller großzügiger Hangschutz, aber kann einen großen Realtimefehlbetrag unentdeckt lassen. Der Testkommentar kennzeichnet diese Einschränkung korrekt. Es liegt kein neuer p95/p99-/Max-/Deadline-Nachweis vor.

**Gesamtstatus Echtzeit: OFFEN.** Keine hörbare Glitchfreiheit oder Realtimekapazität unter 64 langen hochtransponierten Stereo-Voices behaupten. Paket-, Host- und Mehrinstanzlast mit echter Callbackkadenz messen.

### 6.3 Stimmen, Mix, Choke und Transport

Die Grundlagen bleiben nachvollziehbar: maximal 64 Voices, keine harte Voice-Stealing-Abschneidung, One-shot-Überlappung, Stereo-Balance mit Unity in der Mitte und ein sampleratenabhängiger Chokefade. Leere Rows bleiben still. Samples und Kernel bleiben für alte Voices am ursprünglichen Eigentümer.

Folgende alte Risiken bestehen weiterhin:

- `fire()` beginnt Chokefades **vor** der Suche nach einem freien Voiceplatz [K:261]. Bei voller Polyphonie kann ein neuer Hit alte Stimmen abblenden, obwohl er selbst anschließend verworfen wird. Kein neuer Kombinationsregressionstest.
- Event-/Voice-Überläufe bleiben ohne nutzerseitige Diagnose; der Prozessor wertet Appendergebnisse und `rejectedCapacity` nicht aus [P:388], [P:435], [P:460].
- Mute/Solo filtern neue Trigger. Bereits klingende Voices behalten Gain, Pan, Pitch und Asset. Das ist bestehendes, teilweise ausdrücklich getestetes Verhalten und wird nicht als neue Regression ausgegeben; für lange Samples muss der Bedienvertrag klar sein.
- PLAY→STOP setzt Stimmen zurück. Seek/Loop bei weiter gesetztem Playing hat keinen eigenen Discontinuity-Reset; gewünschtes Tailverhalten ist noch festzulegen.
- MIDI bleibt eine 16-Noten-One-shot-Zuordnung 36…51, keine vollständige GM-Map und kein unbegrenzt spezifizierter MIDI-Controller-/Panikvertrag.

## 7. Import, Projektzustände und Hostintegration

### 7.1 Import und Speicher

WAV-Decoder und Kitlimits sind unverändert: PCM8/16/24/32 und Float32 mono/stereo, ausgewählte Extensible-Fälle,16 MiB je WAV und 64 MiB eingebettete Quellen pro Kit. RF64/RIFX, Float64 und 24 gültige Bits im 32-Bit-Container sind nicht als voll unterstützt anzunehmen. NaN/Inf-Schutz und Extrema müssen mit dem tatsächlichen Fixturekorpus nachvollzogen werden.

Die FLAC-Grenzreparatur ist substanziell besser, siehe F13. Dateilesen, Preflight, Decode und Pitch-Neuberechnung laufen aber weiterhin synchron über den UI-/Controlthread. Der Prozessor dekodiert beim Pitchschritt neu und baut den Kernel neu auf. Die längeren Kernel erhöhen diese Kosten. Ein Importworker wurde nicht eingeführt; dabei müsste ein späterer Worker die bestehende Single-Producer-Ownership respektieren. Kein Worker oder Streamingverhalten darf aus dem Commitstand heraus behauptet werden.

### 7.2 Vollständige Kittransaktion bleibt ein offenes P1-Risiko

`publishRestoredKit()` [P:474] präpariert alle Assets vorab, veröffentlicht sie aber weiterhin über 16 einzelne Pending-Slots. Danach ersetzt `setState()` Raster, Auswahl und Mixfelder einzeln [P:568]. Der Audiothread greift nicht auf den Controlmutex zu, was für seine Echtzeitfähigkeit richtig ist, aber diese getrennten Publikationen nicht zu einer vollständigen Generationstransaktion macht.

Bei einem laut SDK zulässigen `setState()` während laufender Verarbeitung ist eine Ausführung zwischen zwei Publikationen möglich: Zeile 1 neu, Zeile 2 alt; anschließend neue/alte Raster- oder Mixteile. Bereits gestartete Voices behalten ohnehin ihre alten Assets. Die bisherigen erfolgreichen Recalltests stoppen/reaktivieren oder verwenden frische Instanzen beziehungsweise liefern einen beschädigten Kandidaten, bevor überhaupt publiziert wird. Sie belegen nicht, dass ein **gültiger** kompletter Kitwechsel unter laufendem Audio kohärent erfolgt.

**Status: TEILWEISE/technisches Risiko, kein hier gemessener Mischblock.** Für eine Abnahme sind eine ausdrückliche Kitwechsel-/Tailpolitik und ein Test unter tatsächlicher Nebenläufigkeit erforderlich. Ungültige-Kandidaten-Rollback und atomare Audioübernahme sind verschiedene Anforderungen.

### 7.3 Legacy- und unsichtbare Zustände

DMR1-Griddefaults sowie Pitch-/Choke-Defaults sind korrigiert. Ein Test muss trotzdem einen absichtlich veränderten Controller **vor** DMR1/2/3-Recall verwenden; eine frische Instanz kann das frühere Problem verdecken. Prüfen: Raster, Labels, M/S, Gain/Pan/Pitch/Choke, Auswahl, Zeilenzahl und nächster Edit, jeweils gegen den Prozessor.

DM10 erlaubt weiterhin versteckte geladene Rows und Solobits außerhalb des sichtbaren Bereichs [B:70]. Die direkte Hide-Solosperre heilt keine bereits gespeicherten versteckten Solos aus früheren Versionen. Dafür ist eine Bedien-/Migrationsentscheidung nötig. Das ist von N01 zu unterscheiden: Versteckte geladene Rows sind im aktuellen Stateformat zulässig und müssen daher beim Resize-Rollback berücksichtigt werden.

### 7.4 Multi-Out, Dirty und Lifecycle

Die Routingreparatur für Null-MAIN ist nachvollziehbar. Freie Ausgangszuordnung bleibt nicht implementiert: Zeile 1 geht auf MAIN, Zeilen 2…16 auf ihre festen Aux-Busse, sofern aktiv/vorhanden, ansonsten auf MAIN. Die vollständige Matrix aus 16 registrierten Busbeschreibungen, inaktiven Lücken, MAIN-inaktiv, Aux15 und Songrecall ist weiterhin nicht end-to-end belegt. Der neue F08-Pakettest darf wegen seiner reinen Finite-Assertion nicht als gemessener Audio-Isolationstest zitiert werden.

Dirty-Aufrufe sind im Controller jetzt eingebaut. Die generische Callbackpolicy kann die VST3-Anbindung jedoch nicht unabhängig beweisen. Ein Mock-/Testhost muss `IComponentHandler2` tatsächlich anbieten und bei Sample, Clear, Step, Velocity, Mix, Pitch, M/S, Choke, Swing, Humanize und Zeilenzahl echte Aufrufe zählen; abgelehnte Edits dürfen keinen Erfolg vortäuschen. Abgebrochene Resize-Transaktionen verursachen bereits erfolgreich versandte Hin-/Rücknachrichten und Dirtymeldungen; die gewünschte No-op-/Undo-/Modifiedpolitik gehört geprüft.

`setProcessing`, Float32-Unterstützung, InfiniteTail und bestehende Aktivierungs-/Sampleratenwechselpfade bestehen weiter. Float64 wird bewusst abgelehnt und ist nicht allein deshalb ein Fehler. Native Editorlifecycle, mehrere Ansichten/Instanzen, Host-DPI-Wechsel und tatsächlich geöffnete/geschlossene Fenster fehlen als belastbare Abnahme.

## 8. Gesamte GUI: Bedienung und Zeilen-/Zoommatrix

Die Oberfläche bleibt eine gezeichnete Custom-View, keine Sammlung nativer Slider und Texteingaben. Deshalb sind leere UIDESC-Controltags hier nicht automatisch ein Fehler; die manuelle Koordinatenroute muss getestet werden.

| Gruppe | Aktueller Pfad und wesentliche Prüfung | Status der vollständigen Bedienabnahme |
|---|---|---|
| A–H | Zeichnung/Hit-Test und `changePattern` vorhanden; Abstände horizontal korrigiert; N04/N05 beachten | FAIL für die genannten Rest-/Abbildungsfehler; native Klickkette offen |
| Zoom100/150 | Beide Buttons liegen jetzt in 1280; Auswahlfarbe vorhanden; SDKzoom wird direkt gesetzt | FAIL wegen Persistenz und N02; HiDPI-/Fensterpraxis offen |
| Swing/Humanize ± | Nachrichteneffekt und Recall vorhanden; Ctrlformel hinzugefügt | TEILWEISE; keine nativen Modifier-/Mausprüfungen |
| Samplefeld/Zeilennummer | Dialog/Alt-Clear und begrenzter einzelner WAV-/FLAC-Dateidrop | TEILWEISE; echter `IDataPackage` und Browserpfad nicht getestet |
| VOL/PAN/PITCH-Hälften | Eigene Zielbereiche statt durchgehender Lücke; korrekte Resetformeln | TEILWEISE; alle 16 Rows, beide Hälften, Grenzen und Audio/State testen |
| M/S/Choke | Zustandsfarben/Zahl und Nachrichten vorhanden; Solo verhindert direktes Hide | TEILWEISE; keine komplette Controller-zu-Audio-Klickmatrix |
| 32 Steps × 16 Rows × 8 Patterns | On/Off und Shift-Velocity vorhanden; rechnerisch alle Stepflächen innerhalb 1280 | TEILWEISE; vollständige native Raster-/Modifier-/Stateprüfung fehlt |
| Plus/Minus | Transaktionshelper eingebaut | FAIL N01; Recall-/Attach-Ablehnung zusätzlich offen |
| Namen, Hilfen, Status | UTF-8-Codepointkürzung verbessert; feste Font-/Textrechtecke | TEILWEISE; Fontmetriken, Grapheme und echte Windowsbilder fehlen |

Die logische Höhe bleibt `max(300,172+29*rows)`:1/4 Zeilen 300, 5 Zeilen 317, 16 Zeilen 636. Bei 150% werden daraus 450, 475,5 und 954 nominelle Pixel Höhe; bei zusätzlichem Hostscale 2 nochmals doppelt so groß. Der allgemeine Größenvertrag ist nun mathematisch richtiger, aber Rundung, tatsächlicher HWND-Clientbereich und ein abgelehnter Resize müssen beobachtet werden. Eine 1920-Pixel-Breite bei 150% lässt auf einem 1920-Pixel-Display keinen normalen Fensterrand übrig; einen Fit-/Scrollfallback gibt es nicht.

Keine nativen Bilder dieses Audits wurden erzeugt. Gute Farben und korrekte Zahlenfelder werden deshalb nicht allein aus Quellkoordinaten als visuell abgenommen bezeichnet.

### Noch offene funktionale Produktziele

Der Scopeentwurf nennt weiterhin unabhängige Instrumentennamen, frei bedienbares Output-Assignment, Preview, Copy/Paste, praktisches Undo und einen laufenden Stepindikator. Diese sind im aktuellen Provisorium nicht vollständig vorhanden. Es gibt keine neue Playhead-/Transportanzeige; „Running-Light“ in README ist damit nicht als implementierte GUI-Funktion belegt. Numerische Texteingabe, Mausrad und Reglerdrag fehlen ebenfalls.

Das Scope-Dokument ist ausdrücklich ein Entwurf. Diese Punkte werden daher nicht automatisch zu 17 weiteren Bugs erklärt. Sie müssen aber **vor dem endgültigen Layout** als V1-Pflicht, bewusste Verschiebung oder Streichung entschieden sein. Vintage-Charakter-DSP ist weiterhin optional und nicht vorhanden; dieser Audit entwickelt keine neue GUI und keinen solchen DSP.

## 9. Priorisierte notwendige Nacharbeiten

### Vor einer technischen Freigabe

1. **P1 N05:** Einheitliche Patternabbildung einschließlich kompatibler Automationsgrenzen herstellen und mit Zwischen-/Grenzwerten gegen Hosttext, Plainwert, GUI, State und Audio testen.
2. **P1 N01/F10:** Tatsächlichen Resize-Rollback mit belegten versteckten Rows absichern; Attach/State-Recall mit abgelehntem Fensterresize bedienbar halten.
3. **P1 Risiko Kitwechsel:** Kohärenten kompletten Kitwechsel bei laufendem Audio nachweisen oder den zulässigen Host-/Produktvertrag belastbar begrenzen. Keine bloße Annahme „State kommt immer bei gestoppter Verarbeitung“.
4. **P2 N02/F15:** Zoomwert, tatsächliche Darstellung und Wiederherstellung über Ablehnung/Close/Reopen konsistent halten.
5. **P2 N03:** FLAC-Vollständigkeits- und Fehlerpolitik absichern, bestehendes Sample bei Ablehnung erhalten.
6. **P2/N04:** Verbleibende Randhitboxen korrigieren und die eigentlichen nativen GUI-Regressionspfade etablieren.

Diese Punkte sind Empfehlungen für einen nachgelagerten Korrekturauftrag. Im Rahmen dieses Audits wurde keine Reparatur vorgenommen.

### Vor Beginn der endgültigen Vintage-GUI-Implementierung

- Alle bestätigten P1-Fehler geschlossen; weitere P1-Risiken durch geeignete Tests oder einen ausdrücklich akzeptierten Produktvertrag geklärt.
- Für jedes Bedienelement eine nachvollziehbare Aktion→Controller→Prozessor→Audio/State→GUI-Kette; keine Freigabe nur aufgrund des Vorhandenseins einer Methode.
- Tatsächlich angehängte GUI mit 100/150%, 1/4/16 Rows, Host-DPI und ablehnender Resizeverhandlung geprüft; keine abgeschnittenen oder falsch markierten/zugeordneten Ziele.
- Verbindliche Funktionsliste und Bedienverträge für die bisher fehlenden V1-Ziele.
- SRC-Qualitätsziel und echte Realtime-/RAM-Zielbudgets mit Messungen; funktionierender Song-/Sample-/Routingrecall und tatsächliche Dirty-Anbindung.
- Separate Studio-One-Abnahme der folgenden Liste. Ein bestehender erfolgreicher Build ist dafür kein Ersatz.

**Entscheidung bleibt: Noch keine Freigabe.** Eine rein statische Designstudie wäre eine separate Produktentscheidung; die hier angefragte belastbare Bestätigung „funktional abgeschlossen, endgültige GUI kann gebaut werden“ ist durch den vorliegenden Stand nicht gedeckt.

## 10. Konkrete letzte Studio-One-Praxistests

Genaue Studio-One-Version, Windowsversion, ASIO-Treiber, CPU, Monitor-DPI, Samplerate, Blockgröße und installierte VST3-Binäridentität protokollieren. Alte Angaben „Studio Pro 8.1.2“ nicht ungeprüft als Nachweis einer anderen aktuellen Hostversion übernehmen.

1. **GUI und Randbedienung:** Bei 1/4/16 Rows und 100/150% jeden globalen und zeilenbezogenen Button, alle 32 Steps, beide numerischen Hälften und die Abstände klicken. Shift/Alt/Ctrl prüfen. Vollständige Screenshots, reale Clientmaße und Fensterränder erfassen. Mehrere DPI-/Monitorstellungen einschließlich rechts/unten am Displayrand.
2. **Resize/Zoom-Ablehnung:** Kleiner Monitor beziehungsweise kontrolliert ablehnender Testhost zuerst automatisiert; in Studio One groß/klein, Monitorwechsel, wiederholter Zoom und Rowwechsel. Reale Transformation, Zoommarkierung, Controller-/Prozessor-State und sichtbare Zeilenzahl vergleichen. N01 mit einem gültigen Kit mit versteckter belegter Zeile nachstellen.
3. **Patternautomation:** A–H jeweils mit eindeutig unterschiedlichen hörbaren Pattern belegen. Hostanzeige, GUI, State und Audio bei normalen Punkten sowie 0,10/0,90 vergleichen. Gestoppte Audioengine, Zero-Frame-Flush, Wiederaufnahme, mehrere Punkte pro Block und Save/Reopen. N05 muss zunächst im nativen Harness geschlossen werden.
4. **Sampleimport:** WAV/FLAC aus dem Explorer und aus dem Studio-One-Browser getrennt testen. Lange/Unicodepfade, Stereo, unterschiedliche Bit-/Sampleraten, defekte und abgeschnittene FLACs sowie Größenränder. Zielzeile und tatsächliche Audioquelle prüfen; Fehlversuch muss das alte Sample erhalten. Clear/Replace während langer Tails prüfen.
5. **Song Modified/Recall:** Nur einen Step, nur ein Sample und nur einen Mixwert ändern: Modified-Zustand, Schließenrückfrage, Autosave und Undo beobachten. 16 unterschiedliche Samples und sämtliche Pattern-/Roweinstellungen speichern; Originaldateien verschieben; Studio One vollständig neu starten und identisches Audio/Routing verlangen. Alte DMR1/2/3-Projekte aus bereits verändertem Zustand laden.
6. **Multi-Out:** Alle 16 Stereoausgänge einzeln und gemeinsam, letzte Aux, inaktive Lücken, MAIN-inaktiv und Rückfall auf MAIN. L/R-Amplituden und Übersprechen numerisch messen; keine Doppelung und keine verschwundenen Rows nach Songneustart.
7. **Sequencer/MIDI:** Verschiedene Tempi, negative PPQ/Preroll, Loop/Seek, Stop/Resume, MIDI-Audition bei gestopptem Transport; Velocitystufen, Chokegruppen und volle Polyphonie. Samplegenaue Onsets und erwartete Tails gegen einen unabhängigen Sollplan prüfen.
8. **SRC/Realtime:** Realistische und synthetische Samples bei 44,1/48/96/192 kHz, Pitch bis ±24, kleine und wechselnde Blockgrößen, 64 lange Stereo-Voices und mehrere Instanzen. Audio kontinuierlich mit UI-/Import-/Automationstress; p95/p99/max, Deadlineüberschreitungen und Dropzähler erfassen. Ein 20-s-Throughputlimit ist kein Realtime-PASS.
9. **Editorlifecycle:** Mindestens 20 echte Öffnen→Bedienen→Schließen→Neuöffnen-Zyklen, Track-/Instanzwechsel, zwei Instanzen, State-/Zeilenwechsel bei offenem Editor und Hostneustart. Zoompräferenz und sichtbares Raster müssen stimmen. Bloßes `createView/getSize/release` zählt nicht als nativer Zyklus.
10. **Getrennte Abnahmen:** Steinberg Validator, 125A Plugin Tester und native automatisierte Hosttests auf dem endgültig zugeordneten Windows-Paket dokumentieren; danach Studio-One-Bedienung und musikalische Hörprüfung separat akzeptieren. Keine Zusammenfassung aller Ebenen zu einem einzigen „PASS“.

## 11. Arbeitsprotokoll und Grenzen dieses Berichts

Dieser Bericht basiert auf dem oben genannten aktuellen Quellstand und vergleicht die 17 Befunde einzeln. Es wurden keine Pluginquellen, bestehenden Tests, Ressourcen oder Workflows verändert, keine Reparaturen ausgeführt und keine neuen Actions-Builds gestartet. Ausschließlich `CODEX-FINAL-AUDIT.md` wird für die angeforderte Bereitstellung auf `develop/v1.0.0` committed und gepusht; der Dokumentationscommit liegt naturgemäß nach dem geprüften Implementierungscommit.

Die im Bericht aufgeführten mathematischen/Quellen-Gegenbeispiele sind ausreichend konkret, um sie in einem später genehmigten Korrekturauftrag als native Regressionen umzusetzen. Sie ersetzen keinen hier nicht verfügbaren Windows-/Studio-One-Lauf. Verbleibende Unsicherheit wird deshalb ausdrücklich als TEILWEISE oder OFFEN bezeichnet.

## 12. Direkte Primärquellen der entscheidenden SDK-Verträge

- [StringListParameter::toString/toPlain](https://github.com/steinbergmedia/vst3_public_sdk/blob/586dc5e6c8012c3e4b01c79389375cbe96bdb1da/source/vst/vstparameters.cpp#L291).
- [FromNormalized: int(norm × (numSteps+1))](https://github.com/steinbergmedia/vst3_pluginterfaces/blob/4f547e8e102b47de4a8b8aaf343c73b700786372/base/futils.h#L93).
- [VST3Editor::setZoomFactor und RequestResize](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L600).
- [CFrame::setZoom und Rücknahme bei Ablehnung](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L248).
- [CFrame::setSize und beforeSizeChange](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L921).
- [VST3Editor: ungeöffnetes init](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L452) und [tatsächliches open](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L1305).
- [Win32DataPackage::getData: Pfadlänge ohne Terminator](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32datapackage.cpp#L144).
- [IComponent::setState ist auch im Processing-Zustand zulässig](https://github.com/steinbergmedia/vst3_pluginterfaces/blob/4f547e8e102b47de4a8b8aaf343c73b700786372/vst/ivstcomponent.h#L197).
- [dr_flac: Frames bis zum Dekodier-/Streamende lesen](https://github.com/mackron/dr_libs/blob/dfe8377631000664666519fdb83da193fd8037f4/dr_flac.h#L11701).

[B:70]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/KitState.h#L70
[C:22]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L22
[C:31]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L31
[C:37]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L37
[C:52]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L52
[C:98]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L98
[C:115]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L115
[C:125]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L125
[C:131]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L131
[C:149]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L149
[C:152]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L152
[C:168]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L168
[C:171]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L171
[C:193]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorController.cpp#L193
[E:9]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorEditor.h#L9
[F:27]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/FlacImport.cpp#L27
[F:36]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/FlacImport.cpp#L36
[F:38]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/FlacImport.cpp#L38
[F:47]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/FlacImport.cpp#L47
[F:55]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/FlacImport.cpp#L55
[H:103]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L103
[H:274]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L274
[H:736]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L736
[H:780]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L780
[H:801]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L801
[H:814]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L814
[H:844]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/PackagedAudioHostSmoke.cpp#L844
[K:39]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/SampleVoiceEngine.h#L39
[K:41]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/SampleVoiceEngine.h#L41
[K:160]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/SampleVoiceEngine.h#L160
[K:261]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/SampleVoiceEngine.h#L261
[P:334]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L334
[P:342]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L342
[P:355]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L355
[P:388]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L388
[P:405]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L405
[P:406]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L406
[P:435]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L435
[P:460]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L460
[P:474]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L474
[P:543]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L543
[P:568]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorProcessor.cpp#L568
[Q:43]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/EditorGeometrySmoke.cpp#L43
[R:30]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/VisibleRowsTests.cpp#L30
[R:45]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/VisibleRowsTests.cpp#L45
[T:33]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/SampleVoiceTests.cpp#L33
[T:252]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/SampleVoiceTests.cpp#L252
[T:288]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/tests/SampleVoiceTests.cpp#L288
[V:65]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L65
[V:82]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L82
[V:95]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L95
[V:100]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L100
[V:101]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L101
[V:108]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L108
[V:137]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L137
[V:140]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L140
[V:142]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L142
[V:144]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L144
[V:177]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L177
[V:278]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L278
[V:307]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L307
[V:320]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L320
[V:323]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L323
[V:328]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L328
[V:337]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L337
[V:340]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L340
[V:364]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L364
[V:371]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/vst3/DrumatorView.cpp#L371
[X:6]: https://github.com/challanger2000/125A-Drumator/blob/6b8fa5446512f89262c109866d1c5443faac1945/src/core/RowResizeTransaction.h#L6

# 125A Drumator – unabhängiger V1-Funktionsaudit, 21.10.2026

**Abschlussstatus: Windows-QA SUCCESS. Sechs bestätigte Fehler korrigiert. Bereit für kontrollierte Host-Abnahme; keine V1- oder endgültige GUI-Freigabe.**

## 1. Prüfgrundlage und Beweisgrenzen

- Repository: `challanger2000/125A-Drumator`, ausschließlich `develop/v1.0.0`.
- Unabhängig per Remote-Abfrage bestätigter Ausgangs-HEAD: **`3d8b0647741e30ecf1ef097d6567f05f20e1ab07`**. Auch unmittelbar vor dem Korrektur-Push war dies der Remote-HEAD.
- Korrektur-/Testcommit: **`826c6d8b0a3c0d038678061d7ad97092e37927e3`**. Die nachstehenden Quellzeilen beziehen sich auf diesen Commit, sofern nicht ausdrücklich als Ausgangsstand bezeichnet.
- Engineering gelesen: `125A-Engineering/START-HERE.md`, vorgeschriebene Lesereihenfolge, anwendbare globale und Drumator-Regeln, QA-, DSP-, Repository-, GUI- und Baseline-Vorgaben; Engineering-Stand [**`9854275cdad1d35476397d874ba95efc4f2b184a`**](https://github.com/challanger2000/125A-Engineering/blob/9854275cdad1d35476397d874ba95efc4f2b184a/START-HERE.md). Danach Drumator `AGENTS.md`, Scope/Handoff und aktuelle Tests.
- Ziel: Windows x64, VST3, SDK **3.8.1**, Tag `v3.8.1_build_84`, SHA **`3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`**, zugehöriges VSTGUI **`5db272256172557818b6158cf0bb2c4410bddb25`**. Kein SDK-/Versionswechsel, kein Linux-Build, kein Release und keine finale GUI-Gestaltung.
- `main` blieb **`7f5b4a7baba2371d9cc42c12406191ecbf3fe32f`**. Das Projekt hat noch keine veröffentlichte, freigegebene V1-Baseline. Historische „aktuelle“ Abschnitte in AGENTS/Handoff ersetzen die aktuelle Remote-/Quellprüfung nicht.
- Frühere Berichte dienen als Gegenprüfliste, ihre PASS-Aussagen wurden nicht als Nachweis übernommen. Der vom Eigentümer genannte Ausgangslauf [#39657454315](https://github.com/challanger2000/125A-Drumator/actions/runs/39657454315) ist laut API für exakt `3d8b064` SUCCESS. Dies beweist allein keine vollständige Funktionsabdeckung.

**Statusdefinitionen:** `PASS` bedeutet, dass die jeweils ausdrücklich genannte automatisierte Prüfung ausgeführt wurde und ihre geeignete Prüflogik bestanden hat. Es ist kein Beweis für alle möglichen Eingangsdaten oder alle Hosts. `FAIL` bezeichnet einen konkret nachgewiesenen Fehler. `NOT TESTED` bezeichnet einen fehlenden Ausführungsnachweis oder eine nicht ausreichend abgedeckte Variante. `OWNER HOST TEST` bezeichnet die ausstehende Prüfung in Studio One durch den Eigentümer. Quellprüfung, Build, automatisierter Test und echter DAW-Praxistest bleiben getrennt.

Die Arbeit umfasst relevante Produktionsquellen, Grenzen und Fehlerpfade sowie Testorakel. Die grundlegende Produktionsinventur umfasst `src/core/*`, `src/vst3/*`, `resource/Drumator.uidesc`, Factory/IDs sowie die CMake-/Workflow-Anbindung; Tests werden unten nach tatsächlichem Orakel und Produktnähe eingeordnet. Sie ist keine mathematische Verifikation. Insbesondere werden aus einem unauffälligen Datenrennen-Stresstest weder vollständige Race-Freiheit noch garantierte Echtzeitfristen abgeleitet.

## 2. Gesamturteil

**Entscheidung: Die korrigierte technische Basis ist für eine kontrollierte abschließende Host-Abnahme bereit. Eine V1-Freigabe und der Beginn der endgültigen GUI werden hiermit ausdrücklich nicht freigegeben.**

Die sechs konkret bestätigten Fehler sind im geprüften Windows-Stand behoben; kein weiterer konkret reproduzierter Produktfehler bleibt aus diesem Audit ungelöst. Die unten aufgeführten Risiken und fehlenden Prüfungen sind weiterhin offen. Besonders die gemessene Überschreitung der Offline-Blockfrist bei p99/Max verhindert jede Behauptung garantierter Echtzeitfähigkeit. Vor einer V1-/GUI-Entscheidung müssen Owner-Hosttests und die priorisierten Nacharbeiten bewertet werden.

| Befund | Ausgangsstand | Geprüfter Korrekturstand | Tatsächlicher Nachweis |
|---|---|---|---|
| FA01 | FAIL | PASS für den beschriebenen Gegenfall | Echte paketierte AUX-/Nullkanal-MAIN-Regression; normaler erfolgreicher Hosttestexit, Einzelmarker nicht separat zugänglich; Beweisgrenze Abschnitt 8 |
| FA02 | FAIL | PASS | Native 16 MIDI-/Host-Interleavings plus zwei Zero-Frame-Fälle, echte Klassen; Fallresultate in Checks-Annotations |
| FA03 | FAIL | PASS | Echte SongConfig-/Audio-Überlappung mit unabhängiger Stereo-Signatur; Annotation |
| FA04 | FAIL | PASS | Gezieltes reales Pfad-OOM, echtes View-D&D mit Datei, ungültigem UTF8 und Recovery; Annotation |
| FA05 | FAIL | PASS | Alle FILL-/Zoom-Gutters 100/150 und positiver CLEAR/Undo; Annotation |
| FA06 | FAIL (C++-Vorbedingung) | PASS | Geordnete Bounds plus echter 300px-Hostablehnungsfall mit 9 FILL-Goldens und 16 Outputs; Annotation |

**Keine Nachweisverwechslung:** Die neuen Fehlerfälle wurden in diesem Auftrag nicht noch einmal mit einem eigenen absichtlich fehlschlagenden Build des unveränderten Ausgangsstands ausgeführt. Der Vorher-Nachweis besteht aus dem konkreten ursprünglichen Quellpfad und dem dokumentierten Gegenbeispiel; der Nachher-Nachweis aus der ausgeführten Regression. Negativkontrollen beweisen die Empfindlichkeit der jeweiligen Prüffunktion, nicht automatisch eine vollständige Mutationsprüfung des gesamten Plugins.

Im Ausgangsstand wurden sechs konkrete Fehlerpfade FA01–FA06 identifiziert und gezielt korrigiert. Drei zusätzliche Schwächen der Testaussagen wurden behoben: T28 ohne unabhängigen hörbaren Clock-Nachweis, T29 mit möglichen Reststimmen und schwachem Energie-Orakel sowie eine unzutreffende Random-Velocity-Blockaufteilung. Die funktionale Oberfläche wurde nur in ihren fehlerhaften Bedienpfaden korrigiert; es wurden weder Skin noch Gestaltung oder neue Funktionen entwickelt.

**Prioritäten:** P0 = katastrophaler Datenverlust/grundsätzliche Unbrauchbarkeit; P1 = Speicherzugriff oder inkonsistenter/lost control state in kritischem Pfad; P2 = begrenzter Funktions-/Fehlerpfad; P3 = geringfügige Abweichung. Zählung der Produktbefunde: **P0: 0, P1: 3, P2: 3, P3: 0**. Die Testlücken Q01–Q03 werden separat gezählt und nicht als zusätzliche Produktfehler verkauft.

## 3. Befunde, Ursache, Gegenbeispiel und Korrektur

### FA01 – P1 – Nullkanal-MAIN wird trotzdem als Pointer-Tabelle gelesen

- **Ausgangsfehler:** `src/vst3/DrumatorProcessor.cpp:617–621` im Ausgangscommit. MAIN mit `numChannels == 0` war zugelassen. Ein nichtnuller `channelBuffers32` führte dennoch zum Zugriff auf Elemente 0 und 1.
- **Gegenbeispiel:** MAIN hat null Kanäle, aber einen übrig gebliebenen Tabellenzeiger `{guardBuffer, nullptr}`; AUX1 besitzt zwei gültige Kanäle. Der alte Code erzeugt `left != nullptr`, `right == nullptr` und lehnt den gesamten Block ab. Bei null Kanälen existiert kein gültiger Tabelleninhalt, auf den sich das Plugin stützen darf. Andere Pointerinhalte können unzulässige Zugriffe ermöglichen. Ein tatsächlicher DAW-Absturz wurde nicht behauptet oder provoziert.
- **Korrektur:** [src/vst3/DrumatorProcessor.cpp:658](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorProcessor.cpp#L658) liest MAIN-Pointer ausschließlich bei exakt zwei Kanälen. AUX-Rendering bleibt verfügbar; nullkanaliges MAIN wird weder gelesen noch beschrieben.
- **Regression:** [tests/PackagedFunctionalAudit.inl:70](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/PackagedFunctionalAudit.inl#L70) lädt im echten paketierten VST3 für jede Variante ein frisches Kit, MIDI-Note 37 bei Sample 64, AUX1 mit unabhängiger Signatur `L=+0.2, R=-0.4`; MAIN sowohl mit nuller als auch nichtnuller Tabelle. Vor Sample 64 muss Stille herrschen, danach muss die erwartete Stereo-Signatur erscheinen; MAIN-Guard bleibt 17.5. Dieselbe Prüffunktion verwirft Stille, vertauschte Kanäle und eine vorzeitig laufende Stimme.
- **Nachweisgrenze:** Der Gegenfall ist aus dem ursprünglichen Kontrollfluss eindeutig ableitbar; die neue Windows-Regression wurde gegen den korrigierten Stand ausgeführt und bestanden. Es wurde kein separater absichtlich abstürzender Vorher-Build gestartet.

### FA02 – P1 – MIDI-Patternwechsel gehen während Kit-Recall verloren

- **Ausgangsfehler:** `src/vst3/DrumatorProcessor.cpp:506–533` verarbeitete bei geschlossenem Restore-Gate nur Hostparameter und gab danach Stille zurück. Note-On 24–31 wurde in diesem Pfad nicht gelesen. Die vorhandene M37-Prüfung testete nur `AtomicPatternSelection::midiSubmit`, nicht dessen tatsächlichen Aufruf aus `process()`.
- **Gegenbeispiel:** SetState hat seine Auswahlgeneration bereits erfasst und hält das Gate geschlossen. Das nächste Audioblock-Ereignis wählt MIDI-Pattern F. Im alten Pfad steigt die Ereignisgeneration nicht, es gibt keine Ausgabeparameter-Rückmeldung, und die ältere Recall-Auswahl gewinnt. Dasselbe gilt für echte gleichwertige und A→B→A-Kommandos.
- **Zusatzfehler im selben Auswahlpfad:** Der Gate-Pfad akzeptierte Hostoffsets außerhalb eines positiven Blocks; Zero-Frame-Flush akzeptierte positive Offsets. Beispielsweise konnte Offset 256 einen 256-Sample-Block beeinflussen, obwohl er außerhalb `[0,256)` liegt.
- **Korrektur:** [src/vst3/DrumatorProcessor.cpp:506](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorProcessor.cpp#L506) nutzt im Gate-Pfad dieselbe vorab konfigurierte `PatternEventTimeline`, chronologische Auswahl, Hostpriorität bei gleichen Offsets und MIDI-Ausgabeparameter. Zero-Frame verarbeitet ausschließlich Offset 0. Keine Voices, Kit-Daten oder nichtatomaren Song-Caches werden im gesperrten Pfad berührt. [src/vst3/DrumatorProcessor.h:64](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorProcessor.h#L64) liest hierfür ausschließlich den atomaren Song-Modus.
- **Regression:** [tests/integration/FunctionalAuditCases.inl:30](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L30) hält echte `Processor::setState`-Aufrufe an zwei verschiedenen Stellen an: nach dem Auswahl-Snapshot und innerhalb des Commits. Je acht Szenarien prüfen MIDI-Wechsel, gleiche Auswahl, ABA, unsortierte MIDI-Ereignisse, konkurrierende Hostpunkte, Host/MIDI-Gleichstand, unzulässigen Endoffset und Song-Unterdrückung. Gespeicherter Prozessorzustand, tatsächliche Ausgabeparameter, Controller-Recall und Host-String A–H werden kontrolliert. [tests/integration/FunctionalAuditCases.inl:134](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L134) ergänzt die beiden Zero-Frame-Gegenfälle. Das sind instrumentierte echte Produktionsklassen, keine nachgebaute Hilfszustandsmaschine.
- **Grenze:** Die MIDI-Regression kontrolliert Auswahl, Rückmeldung und Recall; der unabhängig hörbare Host-/Restore-Auswahltest O01 und normale paketierte MIDI-Auswahltests bleiben zusätzlich erforderlich. Eine echte Studio-One-Rückführung der Ausgabeparameter wurde hier nicht ausgeführt.

### FA03 – P1 – Song-Fallback kann nach Recall noch zum alten Kit gehören

- **Ausgangsfehler:** `src/vst3/DrumatorProcessor.cpp:1000` publizierte den neuen Song-Mailboxzustand, setzte aber `audioSongChain_` und `audioSongEnabled_` nicht auf die neue Kitgeneration. Die Audioseite behält diese Caches bei einem abgelehnten kohärenten Mailbox-Snapshot bei.
- **Gegenbeispiel:** Kit X läuft im Patternmodus, Kit Y besitzt Song ON mit Eintrag B. Noch vor Ys erstem Audioblock publiziert die UI eine SongConfig; die Mailbox-Sequenz ist gerade ungerade. Der erste Y-Block lehnt den Snapshot ab und benutzt im alten Code Xs Patternmodus zusammen mit Ys Samples/Patterns. Ys ausgewähltes Pattern A erklingt statt Song-Eintrag B.
- **Korrektur:** [src/vst3/DrumatorProcessor.cpp:1046](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorProcessor.cpp#L1046) setzt beide Fallback-Caches während des exklusiven, quieszenten Restore-Commits auf Y. Der Gate-verweigerte Prozesspfad liest diese nichtatomaren Felder ausdrücklich nicht. Nach Freigabe des Gates stammen auch vorübergehend weiterbenutzte Song-Caches aus dem aktuellen Kit.
- **Regression:** [tests/integration/FunctionalAuditCases.inl:96](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L96) primt Xs tatsächlichen Audiocache, lädt Y ohne anschließenden Render und pausiert eine echte UI-SongConfig-Publikation nach dem Mailbox-Count-Store. Ein Audiothread rendert genau in diesem Fenster. Unabhängig erwartete Plateauwerte `+0.4/-0.2` für Ys B müssen erscheinen; A besitzt bewusst Velocity 16 statt 127. Der Testhook wird nur in erzeugte Testkopien injiziert: [tests/integration/Configure.cmake:26](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/Configure.cmake#L26), [tests/integration/Probe.h:1](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/Probe.h#L1).
- **Grenze:** Dies prüft das konkrete veraltete Fallback-Fenster. Es ersetzt keine formale Verifikation jeder Speicherreihenfolge oder aller künftigen Songänderungen.

### FA04 – P2 – Pfadaufbereitung kann aus VSTGUI/Controller herauswerfen

- **Ausgangsfehler:** `Controller::loadWav` führte UTF-8-Pfad-/Endungskonvertierung und Stringallokationen vor seinem bisherigen `try` aus (`DrumatorController.cpp:827`). Auch die D&D-Extraktion (`DrumatorView.cpp:105`) hatte keine äußere Ausnahmegrenze. `filesystem(..., error_code)` verhindert keine vorherigen Konvertierungs- oder Allokationsausnahmen.
- **Gegenbeispiel:** Ein langer gültiger Dateipfad erzwingt eine Stringallokation in `pathFromUtf8`. Ein dort gezielt ausgelöstes `bad_alloc` entkommt im Ausgangscode. Ungültige UTF-8-Dateipfade können auf Windows ebenfalls bei der Konvertierung scheitern. Dies ist ein Fehlerpfad, kein Nachweis eines normalen gültigen WAV-Drops, der immer abstürzt.
- **Korrektur:** [src/vst3/DrumatorController.cpp:825](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorController.cpp#L825) umfasst auch die Pfadvorbereitung mit der Ausnahmegrenze; Dateisystemfehler werden sicher abgelehnt, sonstige Standardausnahmen liefern ReadError. [src/vst3/DrumatorView.cpp:106](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorView.cpp#L106) lehnt fehlgeschlagene Extraktionen ab. Bereits akzeptierte Samples und Metadaten bleiben erhalten. Der bewährte Drop-Target-/Dateipfad-Lösungsweg wurde nicht ersetzt.
- **Regression:** [tests/integration/FunctionalAuditCases.inl:166](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L166) prüft zuerst als Negativkontrolle, dass genau die anfängliche Pfadkonvertierung beim präparierten Allokationsfehler wirklich wirft. Dann echte `Controller::loadWav`- und View-Drop-Callbacks: OOM, ungültige UTF-8-Bytes, vollständiger alter Prozessor-/Controllerzustand inklusive Undo/Pfad, anschließend erfolgreicher Import derselben tatsächlichen WAV-Datei.
- **Grenze:** Dies deckt gezielte Eingangskonvertierung ab, nicht jedes denkbare OOM während externer Host-/SDK-Fehleranzeige und nicht den proprietären Browser-Datenweg von Studio One.

### FA05 – P2 – Unsichtbare Menüzwischenräume führen Aktionen aus

- **Ausgangsfehler:** FILL zeichnete 24 Pixel hohe Einträge mit 25 Pixel Abstand, prüfte im Maushandler aber nur den Gesamtbereich; Zoom entsprechend 29 bei 30 Pixeln. Ein Klick in die sichtbare Ein-Pixel-Lücke nach CLEAR löschte eine Instrumentenzeile.
- **Korrektur:** [src/vst3/DrumatorView.cpp:861](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorView.cpp#L861) und [src/vst3/DrumatorView.cpp:923](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/vst3/DrumatorView.cpp#L923) prüfen zusätzlich die halboffene Einzelhöhe jedes Eintrags. Zeichnung und restliches Verhalten bleiben erhalten.
- **Regression:** [tests/integration/FunctionalAuditCases.inl:219](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L219) prüft alle neun FILL-/CLEAR- und alle vier Zoomzwischenräume bei 100 und 150 Prozent gegen den vollständigen Controllersnapshot bzw. tatsächliche Frame-Transformation und Präferenz. Ein positiver CLEAR-Innenklick mit Undo muss weiterhin funktionieren.
- **Grenze:** Attached-Windows-View mit direkten logischen VSTGUI-Mauscallbacks; dies ist keine Prüfung physischer Mausklicks durch Studio Ones DPI-Transformation.

### FA06 – P2 – Ungültige std::clamp-Grenzen im unterstützten 300-Pixel-Fenster

- **Ausgangsfehler:** UIDESC erlaubt 300 Pixel Mindesthöhe. Lehnt der Host initiales Wachstum ab, bleibt ein kompakter Editor mit einer sichtbaren Zeile. FILL verwendete `clamp(rowY+36,108,height-230)` mit `108 > 70`, Output entsprechend `108 > 90`. Das verletzt die C++-Vorbedingung `lower <= upper` und ist undefiniertes Verhalten.
- **Korrektur:** [src/core/UiCompactLayout.h:24](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/src/core/UiCompactLayout.h#L24) berechnet geordnete Grenzen und lässt hohe modale Menüs bei Platzmangel über den Header reichen, sodass alle Einträge zugänglich bleiben. Dieselbe Funktion steuert Zeichnung und Hitbereiche von FILL, Output, Sample und Choke. Ein bloßes Hochsetzen des oberen Limits auf 108 hätte die unteren Einträge abgeschnitten und wurde deshalb vermieden.
- **Regression:** [tests/UiCompactLayoutTests.cpp:86](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/UiCompactLayoutTests.cpp#L86) prüft Geometrieinvarianten für verschiedene Höhen/Zeilen/Menüs. [tests/integration/FunctionalAuditCases.inl:249](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/integration/FunctionalAuditCases.inl#L249) verweigert die initiale echte Hostvergrößerung, verlangt den 300-Pixel-/Einzeilen-Fallback und wählt sämtliche neun FILL-Aktionen anhand unabhängiger 16-Step-Goldtabellen sowie alle 16 Ausgangszuweisungen. Der gesamte Menüinhalt muss im Client liegen.
- **Nachweisgrenze:** Die verletzte Vorbedingung ist direkt nachgewiesen. Ein konkreter Release-Absturz wurde nicht beobachtet. Auf einer Bibliothek, die zufällig dieselben sinnvollen Koordinaten zurückgibt, muss der alte Code keinen roten Lauf erzeugen; ein grüner Lauf beseitigt die ursprüngliche C++-Vertragsverletzung nicht.

## 4. Tests kritisch nachgeprüft und verbessert

| ID | Bisherige Schwäche | Neue Prüfung / verbleibende Grenze |
|---|---|---|
| Q01 / T28 | Native Prüfung setzte private Clockfelder und kontrollierte deren Reset. Der paketierte „T28“-Kommentar lag in einem MIDI-/Sample-Rate-Test ohne unabhängige Sequencer-Clock-Erwartung. | [tests/PackagedFunctionalAudit.inl:8](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/PackagedFunctionalAudit.inl#L8): öffentliche echte VST3-Lebenszyklen 48k→44.1k, ausschließlich Step 1, kein MIDI, erster neuer Block bei PPQ4, unabhängige `+0.25/-0.5`-Signatur; identisches Audio für 256 und 128+128 Samples, Sample-/Grid-Erhalt und verworfene Stille/LR-Swap-Kontrollen. Weil ein zulässiger Lifecycle auch `setActive`/`setProcessing` zurücksetzt, ist dies kein isolierter Beweis für eine bestimmte private Reset-Zeile. |
| Q02 / T29 | Wiederholte Note nach vorausgegangenem F08-Rendering ohne frische Stimmen; `energy > 1` kann von alten Tails stammen. Nur nuller MAIN-Zeiger getestet. | Frisches Kit je Variante, sauberer Vorlauf bis Offset64, eindeutiges Stereo, nuller/nichtnuller MAIN-Zeiger, Guards und negative Orakel; FA01 oben. |
| Q03 / Random Velocity | Referenz Offset0/Start8 wurde mit Offset100 und um100 Samples erhöhtem Start verglichen. Das ist nicht derselbe Zeitpunkt; beide lagen zufällig noch im selben musikalischen Schritt. | [tests/TriggerBatchTests.cpp:39](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/TriggerBatchTests.cpp#L39): Offset6100/Start8 gegen Offset100/Start8+6000 Samples über eine Stepgrenze; bewusst falscher absoluter Zeitpunkt muss ein anderes Ergebnis erzeugen. Algorithmus nicht verändert. |
| T01 | Zu prüfen war tatsächlicher Pluginstate, nicht lediglich eine Kopie des Testkits. | [tests/PackagedAudioHostSmoke.cpp:1084](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/PackagedAudioHostSmoke.cpp#L1084) liest tatsächlichen `component->getState` nach 2T/1T/2T, kontrolliert zweite Balken aller A–H/Rows und ein absichtlich gelöschtes Negativbeispiel. Geeignet für diesen Fehler; keine visuelle DAW-Aussage. |
| T02 | Energie allein würde vertauschte Ausgänge/Kanäle nicht erkennen. | [tests/PackagedAudioHostSmoke.cpp:1893](https://github.com/challanger2000/125A-Drumator/blob/826c6d8b0a3c0d038678061d7ad97092e37927e3/tests/PackagedAudioHostSmoke.cpp#L1893) verwendet 15 verschiedene Stereo-Signaturen, unabhängig erwartete Amplituden und Neighbor-/LR-Swap-Negativkontrollen. |
| T03 | Ein gestarteter Writer-Thread allein garantiert keine überlappenden Operationen. | Native Probe hält den echten Patternbank-Writer in der Mitte fest und lässt den echten Reader auflaufen; erfolgreicher späterer Snapshot plus Ablehnung hybrider Daten. Der zusätzliche Thread-Stress allein wird nicht als erzwungener Überlappungsbeweis gewertet. |
| T04 | Gleiche Kits oder fehlender Render während Restore würden Generationsfehler verdecken. | Paketierter Host mit verschiedenen X/Y-Samples, Patterns, Gains, Master und vertauschtem Routing; 24 bestätigte Wechsel, unabhängige Vierkanalcodes und Hybrid-Negativkontrolle. Zusätzlich echte Gate-Pause: vollständig stille Übergangsblöcke und anschließend nur neue Generation. FA03 ergänzt den davor nicht abgedeckten Song-Fallback. |
| O01 | Gleichwertige Hostevents/ABA können bei reinem Wertvergleich verschwinden; Queue kann gleichzeitige Punkte zusammenfassen. | Zwölf native tatsächliche Restore-Interleavings: Zero-/Audio, Snapshot-/Commitfenster, anders/gleich/ABA; ABA über getrennte `process`-Aufrufe. Tatsächlicher gespeicherter State, Controller/Hostlabel und hörbare Pattern-Signatur. FA02 ergänzt MIDI, das davor nicht denselben Prozesspfad durchlief. |
| Z01 | Hilfsfunktion `PreparedKitNames` allein beweist keinen Controller-Recall. | Tatsächlicher `setComponentState`, Allokationsfehler an den erreichten Stellen, zwei echte Views, Undo/Clipboard/Geste und nativer Textedit. Abgelehnte Recalls müssen Daten und Editorzustand erhalten; akzeptierte Recalls dürfen durch alte Callbacks nicht zurückgeschrieben werden. Zusätzlich DMR1/2/3. |
| Z02 | Bloßer gespeicherter Zoomwert beweist keine Frame-Transformation. | Attached HWND und reale VSTGUI-Transformation, 100/150, DPI 100/150/200, Hostablehnung ohne Teiländerung, DZM1/2, Wiederöffnung. Mehrere notwendige View-Resizes werden bewusst vor Änderung abgelehnt; das ist die bestehende Produktpolitik, kein atomarer Mehrfenster-Zoom-Commit. |
| Z03 | Ein CTextEdit-Objekt ohne natives HWND beweist keine Windows-Lebensdauer. | Echter Win32 EDIT, Enter/Escape/Fokusverlust, abgelehnter/akzeptierter 16→4-Recall, verspätete Listenercallbacks und zwei Views/Schließen. Das zerstörte HWND wird geprüft. |

**Keine Tests wurden abgeschaltet oder für Produktfehler auf erwarteten Fehlschlag umgestellt.** Der bereits vorhandene Song-Chain-Negativtest mit `WILL_FAIL` bleibt als Test der aktiven Release-Prüflogik bestehen. Die neuen Testhooks stehen ausschließlich in generierten Testkopien, nicht im ausgelieferten VST3. Globale strenge Warnungen für eigenen Code bleiben aktiv.

## 5. Funktions- und Testabdeckungsmatrix

Die PASS-Einträge beziehen sich ausschließlich auf die konkret genannten, ausgeführten automatisierten Tests des korrigierten Commits. Wo der tatsächliche Studio-One-/OS-Pfad fehlt, bleibt dies ausdrücklich OWNER HOST TEST oder NOT TESTED. Die automatische Aussage bleibt auch dann beschränkt, wenn mehrere geprüfte Teilpfade in einer Zeile zusammengefasst sind.

| Funktion / Grenzen | Produktionspfad | Ausführbare Nachweise | Status / Grenze |
|---|---|---|---|
| Leeres Instrument bleibt still; Sample-Vorschau ohne Transport | Processor::notify/process; SampleVoiceEngine | Voice empty; packaged empty/preview/T22 | PASS (Automatik); Host-Vorschau OWNER HOST TEST |
| WAV mono/stereo, PCM8/16/24/32, Float32, Extensible | WavPcm16Decoder; PreparedSample | WavDecoderTests, packaged PCM/Float-Amplituden | PASS (Automatik); Mehrkanal/RF64 absichtlich nicht unterstützt |
| Korrupte WAV, RIFF-Grenzen, NaN, alter Klang bleibt | Decoder Candidate-Swap; Controller::loadWav | Decoder-Mutationen, Exchange und packaged ungültiger Import; FA04 | PASS (Automatik); keine exhaustive Dateiformat-Fuzzkampagne |
| FLAC inklusive deklarierter Größenlimits und beschädigter Metadaten | FlacImport.cpp; Processor::notify | Echter eingebetteter FLAC, hörbare Wiedergabe, Oversize/Frame-Mismatch | PASS (Automatik); keine vollständige beliebige Bitstream-Fuzzkampagne |
| 16MiB Einzel-WAV / 64MiB rohe Kit-WAVs | KitCodec und LoadSample | KitStateTests exakte Grenze/Überlauf, packaged Limits | PASS (Automatik); keine Gesamt-RAM-Garantie |
| Samplewechsel/alte Tails/Reclaim | PreparedSample / SampleExchange | Exchange Lebensdauer, Retired/Withdraw/Ratefail, Voice old+new | PASS für definierte Sequenzen; langes paced RT NOT TESTED |
| Sinc-Resampling, +24/-24 Pitch, Ratewechsel | SincKernel / PreparedSample / setupProcessing | DC, Stereo, Oktav-Aliasdämpfung, 16x-Kernel, packaged Pitch/M35/T28 | PASS für geprüfte Punkte; vollständiger Frequenz-/Transientensweep NOT TESTED |
| 64 Stimmen, Überlast, Erholung | SampleVoiceEngine::render | 65/128 Hits, Capacity-Counter, Drain/Recovery | PASS (Automatik); kein garantierter Deadline-PASS |
| Choke 0–4, Fade, volle Polyphonie | RowSound / Voice Launch | ChokeAndStereo und N09: abgewiesener Hit darf alte Stimme nicht choken | PASS (Automatik); musikalischer Höreindruck OWNER HOST TEST |
| Mute/Solo, verborgenes Solo | Triggerfilter, RowMuteSolo | Voice-Policy, packaged Grenzen/F07 | PASS (Automatik); laufende Tails bleiben absichtlich erhalten |
| MAIN +15 Stereo-AUX, mehrere Rows auf einem Bus | activateBus / process / Voice routing | Host-Busnegotiation, SharedBus, F08/T02/T29/FA01 | PASS (Automatik); echtes Studio-One-Busmanagement OWNER HOST TEST |
| AUX deaktiviert → MAIN-Fallback, kein paralleles Doppelsignal | Voice stem pointers | AssignableOutput, simultaneous stems, packaged active/inactive | PASS für geprüfte Matrix |
| Gain 0–200%, Pan L100–R100, Pitch ±24st | AtomicRowMix, RowMix/RowPitch | Helper-Grenzen, Voice amplitudes/pan, packaged persisted state | PASS (Automatik); Änderungen gelten bei Launch für neue Stimmen |
| Master 0–200%, Rampe ohne Blockabhängigkeit | MasterGainRamp / process | Core Ramp Partition/Endpoints und packaged G02 | PASS (Automatik); kein separater Studio-One-Abhörtest |
| 32 Steps, A–H, 1T/2T ohne Verlust17–32 | StepScheduler, patternLengths | Core boundaries/partition, packaged DM15/T01, native lengths/U09 | PASS (Automatik) |
| Song bis 64 Einträge, A–H und8/16/32 Steps | SongChain/Mailbox/Scheduler | SongChain Tests, Core ABA/negative PPQ, packaged ABA, native Song, FA03 | PASS (Automatik); Start-/Locate-Produktpolitik siehe unten |
| PPQ0, Takt2, beliebiger Locate, Stop/Restart, Loop | process clock/origin | Core signed/midstep, packaged Transport/T16/T25/N08 | PASS (Automatik); reale DAW-Loops/Tempoänderungen OWNER HOST TEST |
| Swing 0–50%, Humanize0–100%, blockunabhängige Trigger | StepScheduler | Unabhängige Sampleoffsets, Whole-vs-Split bei mehreren Raten/Tempi | PASS für getestete Bereiche |
| MIDI 36–51, Velocity/Offset, NoteOff-OneShot-Policy | TriggerBatch / Processor input | Range/NaN/offset/capacity, packaged sample128 onset | PASS (Automatik); reale Ports/MIDI-Routing OWNER HOST TEST |
| MIDI24–31 Pattern, Host Param1000, Zwischenwerte | PatternQuantization / Timeline / Selection | N05, sample64 Hosttie, A01 feedback, >256 Timeline-Helfer, O01, FA02 | PASS für diese Fälle; >256 Punkte im echten Hostadapter NOT TESTED; weitere Regler nicht als Hostparameter registriert |
| Zero-Frame und outputless Automation | process early branches | F04/F05/T18/T22/T27, FA02 Offsetprüfung | PASS (Automatik) |
| Copy/Paste/Duplicate/Clear, kein Überschreiben belegter Ziele | Controller PatternOperations/Undo | Atomic helper nextEmpty, native duration/duplicate/persist | PASS für konkrete Szenarien |
| Step toggle/cycle, Velocity Drag/Cancel/Undo | Controller StepEdit/Stroke + View | Core velocity, PatternUndo/U09, native existing/empty/inactivebar/Recall | PASS (Automatik); alle 4096 physischen Zellen einzeln NOT TESTED |
| Neun Row-Fills,1T-Schutz, Clear Row | RowFill / PatternReplace | Core all-row isolation, native V17, FA05/FA06 goldens | PASS (Automatik) |
| RANDOM VEL einmalig / VELO100 / Legacy realtime RND | Controller one-shot edit / TriggerBatch | Native occupied-only/Undo, packaged neutrality, Q03 partition | PASS (Automatik); Legacy-RND ist getrennte gespeicherte Modulation |
| WAV/FLAC-Picker, D&D, Ordner `<`/`>` | View DropTarget/load, Controller stepSample | Native echte Ordner/WAV, callback invalid/OOM/recovery FA04 | PASS für Adapter; OS/DAW Payload und Dialog OWNER HOST TEST |
| Instrumente hinzufügen/geschützt entfernen, Paging | RowVisibility / resize transaction / View | VisibleRows, native compact/song refusal, Grenzen und Seiten | PASS (Automatik); mehrere Views/normaler Live-Repaint NOT TESTED |
| Namen, Unicode-Grenzen, Enter/Escape/Blur/Recall | validRowLabel / Controller / CTextEdit | KitState UTF8 validation, native Z03/N10/N11, FA04 path | PASS (Automatik); tatsächliche native Texteingabe in Studio One OWNER HOST TEST |
| State DM15; historische DMR1–14 | KitCodec + Processor/Controller setState | KitState migration fixtures; native legacy; fresh-instance packaged | PASS für vorhandene Fixture-Formate; nicht jede historische DAW-Datei |
| Abgeschnittene/manipulierte Zustände, CRC, Import-Reject | Candidate parse/preparation + gated commit | KitState corrected-CRC invalids/truncation, packaged M34, native OOM | PASS für konkrete Fehlerstellen; umfassendes Fuzzing NOT TESTED |
| Gleichzeitiger Render und Recall | KitRestoreGate / bank / mailboxes | T03/T04/O01 + FA02/FA03 erzwungene Interleavings | PASS für definierte Fenster; allgemeine Race-/Wait-Free-Garantie NOT TESTED |
| Host-Dirty und Edit-/Flush-Protokoll | Controller::sendEditMessage / Pattern param | Quellprüfung Dirty nur nach akzeptiertem Edit; preview ohne Dirty; host smoke | Echter Studio-One-Projektstern/Undo/Save OWNER HOST TEST |
| VST3 Lebenszyklus, feste IDs, Busse, Float32 | Factory/Processor/Editor | Offizieller Validator + tatsächliche paketierte Audio-/Editorhosts | PASS (Automatik); Float64 absichtlich abgelehnt |
| Realtime: keine eigenen Locks/I/O/Asset-Allokationen im normalen Callback | process, Gate, preallocated Timeline, Exchange | Quellprüfung; bounded buffers; concurrent tests; Offline-Messung | NOT TESTED als vollständige Deadline-/Allocation-Hook-Qualifikation |
| Benutzerzoom100/120/130/150, DPI, gespeicherter Zoom | DrumatorEditor/setState/View | Native Z02 + packaged geometry; FA05 gap | PASS für synthetischen Host; Studio One OWNER HOST TEST |
| Reales GUI-Rendering/Geometrie/Hitbereiche | View draw/onMouse*; UiCompactLayout/UiSongLayout | Windows VSTGUI-Offscreen-BMP + attached logical events | PASS für Fälle; echte DPI/physische Klicks OWNER HOST TEST |

## 6. Vollständige Bedienfamilien-Traceability der vorhandenen Oberfläche

Diese Liste erfasst die dynamisch erzeugten Bedienfamilien und ihre gesamten Indexbereiche. Sie behauptet nicht, dass jede einzelne physische Instanz schon angeklickt wurde. Maßgeblich sind `DrumatorView.cpp:501` (Zeichnen), `:848` (Maus), `:1211` (Drag), `:1266` (Cancel), `UiCompactLayout.h` und `UiSongLayout.h`; nicht die älteren, nur noch isoliert getesteten Geometrie-Helfer.

Alle Koordinaten sind **logische** VSTGUI-Pixel. Native Z02 prüft den Framefaktor Benutzerzoom×DPI. Ein direkter `onMouseDown`-Aufruf in diesen Koordinaten beweist noch nicht die Host-/OS-Transformation physischer Mauskoordinaten.

| Bedienfamilie / alle Instanzen | Geometrie und sichtbarer Zustand | Handler → Controller → Processor / Speicherung | Teststatus und Restbedarf |
|---|---|---|---|
| Titel, Versions-/Status- und Pagingtexte | Customdraw, kein versteckter UIDESC-Control-Tag | Anzeige aus aktuellem Modell/Status | Source/native Pixel; vollständige Lesbarkeit aller Texte OWNER HOST TEST |
| A–H (`p=0..7`) | Je49px aktive Breite,30px Höhe, sichtbare Auswahl und1T/2T; Lücken gesperrt | changePattern; Shift togglePatternLength → Pattern Param1000 / PatternLength → selected/DM15 | Native pattern/menu/length und packaged Auswahl; jede physische Instanz OWNER HOST TEST |
| 1 TAKT / 2 TAKTE | Header x460..526 und533..608,y8..34; Auswahl aus aktueller Länge | togglePatternLength nur bei benötigtem Wechsel → PatternLength | Native direct length, T01 |
| PATTERN… alle 7 Aktionen | Copy/Paste/Clear/Undo/Velocity-Aktionen/Duplicate; deaktivierte Optionen aus can* | copy/paste/clear/undo/vary/reset/duplicate → PatternReplace/Selection → DM15 | Native duplicate/duration/random/velocity + Helper Undo; Hostklick OWNER HOST TEST |
| SONG ON/OFF und SONG EDIT/PATTERN | Header910..991 bzw998..1098,y52..82 | configureSong/SongConfig bzw lokale Editoransicht | Native Song + FA03; Sequenceraudio packaged ABA |
| Song 64 Slots in 4 Seiten à 16 | 8×2 Zellen150×45, Pitch154×52, y145; gewählte/belegte/nächste freie Slots | lokale Auswahl bzw adjustSongCount → SongConfig | Native pages/count/extremes/compact; OWNER HOST TEST für echten Ablauf |
| Song Pattern A–H, Länge8/16/32, Eintrag+/−, Vor/Zurück | Gemeinsame UiSongLayout-Grenzen, ausgesparte Gutters | setSongEntry/adjustSongCount/editSongEntry → SongConfig → Kit.chain | Native paged/compact/signed extreme; Source |
| Zoommenü100/120/130/150 | Header1120..1250,y7..35; Optionen29px bei30Pitch | setZoomChecked; nur Erfolg rememberZoom → DZM2; DZM1 lesbar | Z02/FA05; OS-DPI/Host OWNER HOST TEST; unsicherer SDK-Kontextzoom deaktiviert |
| SWING/HUMANIZE/MASTER | Drei Header-Drehregler, Prozentwerte; Ctrl/Doppelklick Reset, Shift fein | changeKnobValue → adjustSwing/Humanize/Master → atomare Parameter/DM15 | Native precision/extreme/reset, Core Ramp, packaged Grenzen; reale Draggesten OWNER HOST TEST |
| Instrument auswählen `r=0..15` | Zahl01..16,x16..74; farbige Zeilenauswahl | lokale velocityRow; kein unerwarteter Sampleload | Native selection/page controls; alle Zeilen über Paging erreichbar |
| Sampletext, …, `<`, `>`, PLAY je r | x78..272 /275..301 /303..329 /332..358 /362..398,y+5..35; getrennte Bereiche | Picker/rename; sampleMenu; stepSample; previewSample → LoadSample/Rename/Preview | Native folder, FA04, N11; echte Picker/Browser OWNER HOST TEST |
| Samplemenü3 Optionen je r | Laden/Umbenennen/Entfernen,29px/30Pitch; nur gültige sichtbare Zeile | load/editName/clearSample; akzeptierter Commit vor Metadatenswap | Native names/metadata + FA04; modale Hostdialoge OWNER HOST TEST |
| FILL je r / alle 9 Einträge | x403..452; Popup24px/25Pitch; FA05 inaktive1px Lücke, FA06 vollständige Lage | fillRow → PatternReplace → eine Zeile/aktive Länge, Undo | FA05/FA06 golden alle 9, V17 Schutz |
| M/S je r | x457..491/496..530,y+5..35; Zustand Controller | toggleRowGate → RowGate → mute/solo/DM15 | Native/source, Core Gate und packaged Grenzen; Tailpolitik Owner abhören |
| Output je r / alle 16 Busse | x535..615,y+5..35; MAIN/OUT2..16; Popup2×8 | setRowOutput → RowOutput → output[16]/DM15 | FA06 alle 16 Menüeinträge, T02/T29 realer Audioweg |
| Choke je r /0..4 | x570..615,y+54..86; Gruppentext | setRowChoke → RowChoke → choke[16]/DM15 | Source/native control, Voice fade/N09; alle physischen Optionen OWNER HOST TEST |
| VOL/PAN/PITCH je r | Mittelpunkte 133/254/375,y+68, Radius 17; Werte%,C/L/R,st | adjustRowMix/adjustRowPitch → RowMix/RowPitch → DM15 | Native precision/bounds, Voice gain/pan, packaged Pitch; weitere Views siehe Risiko |
| RANDOM VEL je r bzw Legacy RANDOM RT | Button448..544,y+52..84 falls Legacy 0; sonst gespeicherter RT-Regler | randomizeRowVelocity → PatternReplace, eine Undoaktion; Legacy RowRandomVelocity | Native occupied-only/inactivebar/Undo; Q03 und packaged neutrality |
| Alle Step-Tasten `p=0..7,r=0..15,s=0..31` | Rasterx622+19s; sichtbare Taste15×32 plus Rahmen; y+44..78; je 1T zweite Hälfte deaktiviert | toggleStep/cycleStepVelocity/setStepVelocity → StepEdit →4096 DM15-Zellen | Indizes/Bounds/Quelle geprüft; native repräsentative Fälle; exhaustive4096 physische Klicks NOT TESTED |
| VELO je r, alle 32 Balken je sichtbarer Lane | Toggle1233..1266,y+45..79; Lanehöhe30; belegte/gesperrte Steps sichtbar | begin/endVelocityStroke; StepEdit; KitEditGeneration; Cancel/Undo | Native empty/existing/inactive/Recall/OOM/close; echte100/150-Mausbewegung OWNER HOST TEST |
| Instrument+/−, Seiten Vor/Zurück | Footer unter gridBottom; max16, Remove nur leere letzte Row; Rollback bei Hostablehnung | addRow/hideLastEmptyRow/restoreVisibleRows → VisibleRows; lokale Seiten | VisibleRows/compact native; multi-view normale Layoutänderung NOT TESTED |
| Drag&Drop alle sichtbaren Sampleziele | Samplebereich gültiger Row; offene modale Menüs sperren Ziel; TargetBlocked≠InvalidPath | View/DropTarget → Pfadextraktion → loadWav → Samplepublish | FA04 reale Datei und Callback; OS Explorer/Studio-One-Browser100/150 OWNER HOST TEST |
| Namenseditor je r | Echter Win32 Textedit; Kitgeneration entscheidet Gültigkeit | CTextEdit valueChanged/focusfinish → renameRow → RenameRow | Native Z03 Enter/Escape/Blur, akzept./abgelehnter Recall, verspätete Callbacks; echte DAW OWNER HOST TEST |

**Geometrieabdeckung:** Grundbreite 1280, Header 108, Basiszeile 94, Velocity zusätzlich 30, Footer 54, regulär 4 Rows/Seite; bei abgelehntem Resize dynamisch weniger. UIDESC 300..850 logische Pixel Höhe. Native Renderings und gemeinsame Geometrie prüfen repräsentative kompakte, Velocity- und Songvarianten. Die älteren Tests `UiDetailGeometry`, `UiRowControls`, `UiRowGeometry`, `HeaderHitZones` und `VelocityLane` werden nicht pauschal als Nachweis der heutigen Zeichen-/Klickgeometrie gewertet: Sie testen teilweise nicht mehr maßgebliche Hilfsmodelle. Aktuelle Pfade benötigen die Attached-View-/UiCompactLayout-/UiSongLayout-Prüfungen.

## 7. Echtzeit-, Speicher- und Nebenläufigkeitsbewertung

1. **Positiver Quellnachweis:** Audioadoption/Rendering benutzt vorab angelegte Trigger-/Timeline-/Voice-/Mailbox-Speicher. WAV/FLAC-Dateilesen, Dekodieren und Kernelaufbau laufen auf der Kontrollseite. `SampleExchange` überträgt Besitz per Atomics; aktive/retired Samples werden erst nach Ende ihrer Stimmen auf Kontrollseite freigegeben. Tails behalten beim normalen Samplewechsel das alte Asset. Kit-Recall dagegen stoppt alte Stimmen als zusammenhängender Modellwechsel.
2. **Gate-Vertrag:** Audio macht atomare Entry/Leave-Operationen; nur die Kontrollseite wartet auf bestehende Leser. Während Restore erzeugt Audio Stille und verarbeitet Auswahlkommandos im begrenzten vorallokierten Timeline-Speicher. Die Anzahl eingehender Hostevents liegt dennoch beim Host; vollständige konstante Laufzeit unabhängig von der Eventmenge wird nicht behauptet.
3. **Nicht gleichbedeutend mit Wait-Free:** `AtomicPatternSelection` verwendet CAS-Wiederholungen. Lock-free ist keine formale obere Callbackzeitgrenze. Die Kontrollseite kann bei starker Gegenaktivität warten. Es wurde keine allgemeine Wait-Free- oder harte Deadline-Garantie nachgewiesen.
4. **Allokationsfreiheit:** Quellprüfung findet keine eigene gewöhnliche File-I/O/Mutex-/Assetallokation im Renderpfad. Die Hostmethoden zur Ausgabeparameter-Rückmeldung gehören zur Hostimplementierung; ihre Laufzeit wird nicht durch den Pluginquelltext bewiesen. Ein pro Callback instrumentierter Allokator-/Lockdetektor in der finalen DLL mit einer langen realen Hostsession fehlt.
5. **Speicherbudget:** 16MiB/64MiB begrenzen rohe eingebettete WAV-Daten, nicht den gesamten Prozess-RAM. Float-Decoding, Sinc-Kernel, vorbereitete Ersatzkits, gespeicherte Zustände und ausklingende alte Assets können zusätzlichen Speicher benötigen. Peak Working Set bei maximalem Kit, raschen Samplewechseln und Recall ist separat zu messen.
6. **Resamplingqualität:** Vorhanden sind DC-/Amplituden-/Stereo-Prüfungen, einzelne Sinusbänder und 16x-Kernel-Unterdrückung. Das belegt die geprüften Frequenzen, keinen vollständigen spektralen Qualitätsbericht für alle Raten, Pitches und Transienten.
7. **Performance:** Der bestehende 64-Voice-/16x-Test misst Offline-Renderblöcke auf einem geteilten CI-Rechner, einschließlich einiger Drain-/Leerlaufblöcke. P95/P99/Max sind unten aus dem tatsächlichen Lauf übernommen. Die 20-Sekunden-Grenze ist ein großzügiger Hang-/Regressionsschutz. Sie ist keine Echtzeitgrenze. Bei48k/256 beträgt die Frist 5.333ms; selbst Messwerte darunter garantieren weder Scheduling noch CPUreserve im DAW-Projekt. Paced ASIO, mehrere Plugininstanzen, gleichzeitige GUI/Import/Automation und Langzeittests bleiben Owner-Aufgaben.

### Noch nicht bestätigte Risiken – keine erfundenen Fehler

- Normale Row-Mix-/Pitch-/Mute-/Sample-/Swing-/Humanize-Edits benachrichtigen andere offene Views nicht durchgängig via `notifyViews`; Quellstellen Controller:660,670,694,742,755,769,825. Ob ein zweiter Editor dadurch sichtbar veraltet bleibt, wurde nicht durch einen realen Invalidierungs-/Repaint-Test nachgewiesen. **NOT TESTED**, vor Mehrfachview-Freigabe gezielt prüfen. Die geprüften Mehrfachview-Recalls sind kein Nachweis jeder normalen Liveänderung.
- `views_` wird als rohe Viewzeigerliste durchlaufen. Ein konkreter Host-Reentry, der während eines Resize-/Refresh-Callbacks eine View entfernt, ist nicht abgedeckt. Ein UAF wird nicht ohne solche Reproduktion behauptet.
- `TriggerBatch::sort` ist für gleiche `(sampleOffset,row)` nicht stabil. Unterschiedliche gleichzeitige Trigger derselben Row könnten bei Choke/Capacity vom Gleichstandsvertrag abhängen. Kein beobachteter Audio-Gegenfall in dieser Prüfung; ein adversarialer Sequencer+MIDI-/Blockpartitionstest fehlt. Keine willkürliche Sortierpolitik oder alloziierendes stable_sort ergänzt.
- Kein vollständiges Fuzzing von State-/WAV-/FLAC-Grenzen und keine vollständige Windows-Sanitizer-/Application-Verifier-Kampagne. Explizite beschädigte Fixtures und OOM-Stellen ersetzen diese nicht.
- Die einzigen registrierten Hostautomationsparameter sind die bestehende Pattern-Auswahl (ParamID1000; A–H mit Legacy-Quantisierung). Die übrigen Regler sind gespeicherte Controller-Kommandos. Es werden weder eine nicht implementierte vollständige Reglerautomation noch eine neue Parameterspezifikation stillschweigend zugesagt.

**Bewusst bestätigte Semantik, kein neuer Fehler:** Mute/Solo unterbindet neue Trigger, laufende One-Shot-Tails bleiben erhalten; Rowgain/Pan/Pitch/Output gehören zum gestarteten Voicezustand. Ein deaktiviertes AUX fällt auf MAIN zurück, soweit MAIN vorhanden ist. Transportstart/Locate setzt eine neue Phase am aktuellen Hoststart gemäß vorhandener Produktpolitik; dies ist musikalisch vom Eigentümer abzunehmen. Undo für reine Stepänderungen setzt keine 1T-Länge auf 2T zurück: `PatternUndo` benutzt Länge 0 als „nicht geändert“; U09 prüft dies. Diese zunächst untersuchten Hypothesen werden nicht als bestätigte Bugs aufgeführt.

## 8. Tatsächliche Windows-QA und zugängliche Belege

- Gezielter neuer Lauf: [#39676937642](https://github.com/challanger2000/125A-Drumator/actions/runs/39676937642), Job [119489144405](https://github.com/challanger2000/125A-Drumator/actions/runs/39676937642/job/119489144405), SHA `826c6d8b0a3c0d038678061d7ad97092e37927e3`.
- **Ergebnis: SUCCESS**, gestartet **2026-10-21T21:12:58Z**, abgeschlossen **2026-10-21T21:16:20Z**. Der Lauf prüft exakt den oben genannten Korrekturcommit.

| Schritt | Ergebnis / Nachweis |
|---|---|
| Exaktes SDK/Submodule | SUCCESS, Workflow prüft SHA und Submodulstatus |
| Configure / MSVC x64 Release | SUCCESS |
| Build Testaggregate, Integration, Audio-/Editorhost, Drumator, Validator/moduleinfotool | SUCCESS |
| CTest | **22/22**, echte Annotation: `100% tests passed, 0 tests failed out of 22`; darin der absichtlich erwartete Song-Negativtest |
| Native tatsächliche Integration | **43 benannte Fälle, 0 Fehler**, echte aggregierte Annotation unten |
| Offizieller Steinberg Validator | **47 bestanden, 0 fehlgeschlagen**, echte Result-Zeilen-Annotation |
| Standardbundle / moduleinfo / exakter Dateibaum | SUCCESS laut ausgeführtem Workflow mit prüfenden Rückgabecodes |
| Paketierter Editorhost | SUCCESS; native HWND-/Zoom-/Resize-Prüfung zusätzlich als echte Z02-Annotation |
| Paketierter Audiohost einschließlich T01/T02/T04 und neuer T28/T29 | SUCCESS; anhand des tatsächlich erfolgreichen Hostprozess-Rückgabecodes und der überprüften zwingenden Aufruf-/`require`-Pfade. Einzelne T28/T29-Logzeilen sind wegen der unten beschriebenen Ausgabe-/Downloadgrenze nicht separat zugänglich. Dies wird nicht als vollständige Einsicht in deren stdout ausgegeben. |
| VSTGUI-Rendering-Upload | SUCCESS; Artefakt **12382017644**, 634424 Bytes |
| VST3-Bundle-Upload | SUCCESS; Artefakt **12381882711**, 1513354 Bytes |
| Eigene Ansicht/ZIP-/Binärkontrolle der neuen Artefakte | **NOT TESTED**: auch offizieller Artefaktdownload durch Proxy blockiert; keine angeblich angesehenen aktuellen Bilder |
| Studio One | **OWNER HOST TEST – nicht ausgeführt** |

**Tatsächliche Offline-Messung aus Checks-Annotation:** 64 Voices, 16x, 256 Frames/48k: **p95=4.7974ms, p99=7.403ms, Max=8.892ms**. Frist **5.33333ms**. P99 und Maximum überschreiten diese Frist auf dem CI-Rechner; der Test ist gleichwohl grün, weil seine 20s-Grenze nur ein Hangschutz ist. Keine Behauptung eines Realtime-PASS. Ein Konfidenzintervall, reproduzierte Messreihe auf Eigentümerhardware oder ein paced Studio-One-/ASIO-Nachweis liegen nicht vor.

- Baseline-Lauf #39657454315 wurde nicht erneut ausgelöst. Keine Linux-Kompilierung. Lokales `cmake -P` kontrollierte ausschließlich eindeutige Generierungsanker; dies ist kein Build-/Ausführungstest.
- Vollständige GitHub-Joblogs werden über einen offiziellen Azure-Blob-Redirect bereitgestellt, der in dieser Umgebung CONNECT403 liefert. Öffentliche Job-HTML enthält nur Metadaten und ersetzt keine Logs. Diese Einschränkung wird nicht verborgen.
- Deshalb publizieren die Tests zusätzlich tatsächliche Ergebniszeilen als offizielle Workflow-Annotations. CTest versteckt erfolgreiche stdout normalerweise; der Workflow liest deshalb ausschließlich wirkliche Notice-Zeilen aus `build/Testing/Temporary/LastTest.log` und die reale CTestzusammenfassung aus `build/ctest-functional.log`. Der Original-CTestexitcode wird vor der Ausgabe gesichert und unverändert berücksichtigt. Validatorausgabe wird aufgezeichnet und die echte Result-Zeile übernommen; der Originalexitcode bleibt ausschlaggebend. Paketierte Tests melden ebenfalls erreichte Prüfpunkte, deren Notices jedoch teilweise nicht als einzelne GitHub-Annotation erfasst wurden: ältere stdout-Ausgaben verwenden teils ein literales `\\n` statt eines Zeilenumbruchs und können das nachfolgende `::notice` an eine bestehende Zeile anhängen. Deshalb liegen für diesen Lauf **keine separat abrufbaren T28-/T29-Notices** vor. Ein späteres isoliertes Logging-Cleanup sollte einen Zeilenanfang für Workflow-Kommandos sicherstellen. Der Voice-Ausgabetext ist ebenfalls nur ein Gruppenlabel: tatsächlich gibt es 16 benannte Aufrufe plus den expliziten Empty-Instrument-Block, zusammen 17 Gruppen; es werden daraus keine zusätzlichen CTests abgeleitet. Es wurde kein zusätzlicher funktional unveränderter Windows-Lauf allein zur kosmetischen Wiederholung dieser Logs ausgelöst. Der erfolgreiche abschließende Hostprozess ist durch den Workflowexitcode belegt; seine verpflichtenden Aufrufe und Orakel wurden im Quelltext geprüft. Keine hartkodierten PASS-Zeilen im Workflow.
- Die Funktionsprüfungen wurden weder ausgelassen noch durch Annotationen ersetzt. Das reale Native-Integrationsexecutable meldet seine ausgeführten Fallnamen sowie Fehlerzahl gesammelt; bei Fehler bleibt der Prozess fehlschlagend. Der Bericht benennt ausdrücklich, wenn nur API-Schrittstatus und keine detaillierten Ergebniszeilen vorliegen.

### Testinventar und direkt überprüfbare Evidenz

[Checks-Annotations des tatsächlichen Jobs](https://api.github.com/repos/challanger2000/125A-Drumator/check-runs/119489144405/annotations?per_page=100). Dies ist die Quelle für die oben zitierten Result-Zeilen, nicht eine Ableitung aus Commitnachrichten.

Die 22 registrierten CTests (CMake und tatsächliche erfolgreiche Zusammenfassung):

```text
drumator_song_chain_tests
drumator_song_chain_negative_tests  [WILL_FAIL: prüft wirksame Release-Checks]
drumator_compact_layout_tests
drumator_row_control_tests
drumator_detail_geometry_tests
drumator_row_geometry_tests
drumator_ui_hit_tests
drumator_pattern_timeline_tests
drumator_overlay_hit_tests
drumator_pattern_undo_tests
drumator_velocity_lane_tests
drumator_recall_ui_tests
drumator_recall_transaction_tests
drumator_core_tests
drumator_voice_tests
drumator_wav_tests
drumator_exchange_tests
drumator_trigger_tests
drumator_atomic_tests
drumator_visible_rows_tests
drumator_kit_tests
drumator_integration_tests
```

`EditorGeometrySmoke` und `PackagedAudioHostSmoke` laufen anschließend im Paket-/Validator-Workflow und werden nicht zusätzlich als CTests gezählt.

### Tatsächlich erfasste native Fallresultate (Checks API)

```text
Executed native integration cases:
T03 controlled actual bank PASS
T04 controlled real restore gate PASS
O01 zero snapshot different PASS
O01 zero snapshot equal PASS
O01 zero snapshot ABA PASS
O01 zero busy different PASS
O01 zero busy equal PASS
O01 zero busy ABA PASS
O01 audio snapshot different PASS
O01 audio snapshot equal PASS
O01 audio snapshot ABA PASS
O01 audio busy different PASS
O01 audio busy equal PASS
O01 audio busy ABA PASS
Z01 full Controller allocation injection PASS
Z01 legacy Controller PASS
Z02 native attached views PASS
VSTGUI precision controls PASS
VSTGUI native pixel render set PASS
UX direct pattern length PASS
P18 native pattern duplicate menu PASS
P19 1T/2T copy-paste-duplicate duration PASS
T28 rate-switch transport clock reset PASS
RANDOM one-shot existing notes PASS
VELO 100 reset existing notes PASS
V16 1T hidden velocity preservation PASS
V17 1T row FILL/CLEAR hidden-step protection PASS
UX Velocity empty-step editing PASS
UX paged Song board PASS
S09 Song entry signed extreme edit deltas PASS
S10 Song count signed extreme edit deltas PASS
D11 control signed extreme deltas PASS
UX compact Song viewport PASS
N10 processor rename OOM PASS
N11 controller metadata transactional commit PASS
N12 real WAV sample folder previous/next PASS
Z03 Win32 name editor PASS
FA02 MIDI/host arbitration during actual kit restore PASS
FA02 zero-frame valid-offset arbitration PASS
FA03 new-kit Song fallback during publication PASS
FA04 sample path failures and real View drop callbacks PASS
FA05 modal menu gutters PASS
FA06 minimum-height host refusal menus PASS
Total failures: 0; Studio One NOT RUN
```

Diese 43 Fälle sind innerhalb eines einzigen CTests ausgeführt worden; sie werden nicht als 43 zusätzliche CTests gezählt. Der FA02-Fall enthält intern 16 Interleavings, der separate Zero-Frame-Fall zwei weitere. Weitere Daten wie Allokationsstellenzahl oder X/Y-Blockzähler werden hier nicht erfunden, wenn sie nur im nicht zugänglichen stdout stehen.

## 9. Verbleibende Abnahme- und Testaufgaben

**Studio One wurde in dieser Prüfung nicht ausgeführt.** Vor einer finalen V1/GUI-Phasenentscheidung sind wenigstens folgende protokollierten Tests notwendig; Hostversion, Windows-Build, ASIO-Treiber, Rate/Blockgröße, Plugin-SHA und Ergebnis jeweils festhalten:

1. Saubere Installation/Scan des exakt geprüften Standardbundles; neues Projekt, mehrfach Editor öffnen/schließen; keine Crash-/Hänge-/Leaking-Symptome.
2. Explorer- und Studio-One-Browser-D&D getrennt: WAV/FLAC, Unicode/Leerzeichen/URI-Sonderzeichen, jede sichtbare Zeile und Seite,100/150%, HiDPI; offene Menüs müssen sperren. Fehlerhafte/nicht unterstützte/fehlende Datei darf das alte Sample nicht ersetzen. Reale Dateiauswahl, `<`/`>`-Umlauf und Alphabetfolge kontrollieren.
3. Sequencer ab Takt 1/Takt 2/mitten im Projekt, Stop/Restart an gleicher Position, Vor-/Rücksprung, DAW-Loop und Tempoänderungen;1T/2T-A–H und Song 8/16/32 über mehrere Songseiten. Zweite-Bar-Daten nach1T und Wiederöffnung kontrollieren.
4. MIDI 36–51 und 24–31, gleiche/ABA-Auswahl, Patternautomation mit Zwischenwerten und MIDI-Gleichständen; Hostanzeige/Controller/GUI/gehörte Auswahl nach Save/Recall identisch. Dichte Automation und Kitwechsel unter laufender Wiedergabe, auch Zero-Frame-Flushes soweit der Host sie erzeugt.
5. Alle 16 Ausgänge mit eindeutig unterschiedlichen Stereoquellen, MAIN deaktiviert, AUX ein/aus, mehrere Instrumente auf einem AUX, Zuweisungswechsel und Projekt-Reopen. Kein unerwartetes Doppelsignal; definierter Fallback kontrollieren.
6. Alle vorhandenen Bedienfamilien der Matrix physisch durchgehen: Step/Lane/FILL/Copy/Paste/Duplicate/Clear/Undo/Random/Reset, Grenzen, Ctrl-/Doppelklick, Shiftfein, abgeschnittene Namen, Empty/Disabled-Zustände.100/150%, echte OS-DPI und Hostverweigerung sowie kompakter 300px-Fallback. Mehrere Views und normale Liveänderungen ausdrücklich einschließen.
7. Während einer Velocitygeste bzw. eines nativen Namensedits Kit/Projekt laden; danach Move/MouseUp/Cancel/Enter/Escape/Blur und Schließen auslösen. Abgelehnter Recall muss alte Geste/Undo erhalten, erfolgreicher Recall muss alte Callbackwirkung verwerfen.
8. Gespeicherter Zoom einschließlich 120/130 und DZM1-Vorprojekten, Wiederöffnung, Hostablehnung und mehrere Views. Keine teilweise angewandte Transformation oder irreführende Präferenzanzeige.
9. Reales Dirty-/Save-/Undo-Verhalten: akzeptierte dauerhafte Änderung markiert Projekt, Vorschau allein nicht; geladene Projekte behalten Samples/Patterns/Routing/Namen/Master. Je ein vorhandenes echtes Altprojekt ergänzend zu synthetischen Codecfixtures öffnen.
10. ASIO-Langzeitlast: 44.1/48/96k,64/128/256/512 Frames, 16 Samplezeilen, 64 Stimmen/Choke, hoher Pitch, mehrere Instanzen, paralleler GUI-/Kit-/Samplewechsel. Xruns, Aussetzer, CPUspitzen, p95/p99/max und Peak Working Set messen. Offline-Durchsatz niemals als garantierte Echtzeitfähigkeit deklarieren.

**Priorisierte Nacharbeiten vor endgültiger Freigabe:** Hoch: Echtzeitprofil/ASIO-Last und echte Hostautomation/Recall/Dirty; Mittel: Mehrfachview-Repaint/Reentry und simultane Same-Row-Trigger-/Choke-Blockpartition; ergänzend: Formate/State-Fuzzing, Peak-RAM und Hostinput/DPI-Abnahme. Ein Confirmed-Bug-Status wird diesen fehlenden Nachweisen nicht unterstellt. Ergibt eine dieser Prüfungen einen Fehler, erst reproduzieren und korrigieren, dann erneut gezielt validieren.

Weitere gezielte automatische Nacharbeiten: >256 unterschiedliche Patternereignisse innerhalb eines tatsächlichen positiven Prozessorblocks (einschließlich Host-/MIDI-Kollision und Ausgabeparameter), nicht nur die bereits geprüfte Timeline-Hilfsklasse; normales Mehrfachview-Invalidieren/Reentry, simultane gleichzeilige MIDI-/Sequencertrigger mit Choke und verschiedenen Blockteilungen, breitere Dateiformat-/State-Fuzzfixtures, Asset-/Allocator-/Lockinstrumentierung im finalen Paket. Diese Punkte sind als fehlender Nachweis bzw. Risiko ausgewiesen, nicht ohne Gegenbeispiel als bestätigte neue Produktfehler.

## 10. Änderungen und Übergabe

| Commit | Inhalt | Freigabestatus |
|---|---|---|
| `826c6d8b0a3c0d038678061d7ad97092e37927e3` | FA01–FA06, gezielte echte Windows-/paketierte Regressionen, Q01–Q03, unverfälschte QA-Diagnostik | Windows SUCCESS, Run#39676937642 |
| Dokumentationscommit dieses Berichts | Vollständiger Bericht, aktueller AGENTS-Einstieg und Ergebnisübergabe an Issue #6 | Separater Dokumentationscommit; kein erneuter Produktbuild |

Es wurden keine Änderungen an `main`, Version, Pluginidentität, SDK, finalen Bitmaps oder Branding vorgenommen. Die spätere grafische Gestaltung und eine V1-Freigabe bleiben eine separate Eigentümerentscheidung nach der Host-Abnahme und erneuter Bewertung dieser Befunde.

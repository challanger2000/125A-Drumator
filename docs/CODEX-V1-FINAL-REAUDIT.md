# 125A Drumator – unabhängiger V1-Final-Reaudit

**Prüfdatum:** 23.10.2026, Europe/Berlin.

**Geprüfter Produktcommit:** `8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289`.

**Ausgangsbranch:** `fix/studio-one-acceptance-20261022`.

**Ablagebranch:** `audit/codex-v1-final-20261023`.

**Auftrag:** gesamter vorhandener Funktionsumfang einschließlich WAV-Editor; unabhängige Prüfung, keine Produktreparaturen und keine neuen Actions-Läufe.

## 1. Gesamturteil und Beweismaßstab

**Technische V1-Freigabe: NEIN.** Der aktuelle Stand behebt mehrere zuvor unabhängig gefundene Fehler nachweislich. Der existierende Windows-Lauf für genau diesen Commit besteht mit **23/23 CTests, 78/78 nativen Integrationsfällen und 47/47 Validatorprüfungen**. Diese Ergebnisse werden anerkannt. Sie decken die nachfolgend beschriebenen zusätzlichen WAV-Editor- und Zustandswechsel jedoch nicht ab.

Dieser Reaudit identifiziert **14 konkrete Quellbefunde: 1 BLOCKER, 4 HIGH, 8 MEDIUM und 1 LOW**. Der BLOCKER ist ein durch die Aufrufkette belegter Speicherfehler: Nach Übernahme einer WAV-Bearbeitung und anschließendem Leeren der Samplezeile kann der Processor seinen gesamten Zustand nicht mehr serialisieren. Weitere schwere Befunde betreffen einen veralteten WAV-Entwurf, die Veröffentlichung des Vorhörsamples, überlagerte Vorhörstimmen und einen nicht abgefangenen Allokationsfehler an der VST3-Nachrichtengrenze.

**Keine neue Laufzeitreproduktion dieser 14 Befunde wird behauptet.** Ihr Nachweis besteht aus konkreten Eingaben, Aufrufreihenfolgen und den unveränderten Produktpfaden. Zehn passende Regressionen wurden ergänzt, aber in diesem Audit weder mit MSVC kompiliert noch ausgeführt. Die Freigabe scheitert bereits an den eindeutigen Quellpfaden; ein grüner älterer Test ist kein Gegenbeweis für einen anderen, nicht geprüften Zustandswechsel.

Verwendete Status:

| Status | Bedeutung in diesem Bericht |
|---|---|
| **PASS – benannter Testfall** | Für genau diesen Produktcommit ist ein geeigneter erfolgreicher bestehender Test belegt; die Implementierung wurde zusätzlich geprüft. Keine universelle Zusage. |
| **FAIL – Quellnachweis** | Die konkrete Eingabe beziehungsweise Verschachtelung führt nachweislich durch einen fehlerhaften Produktpfad. Neue Windows-/DAW-Ausführung wird gesondert als nicht durchgeführt ausgewiesen. |
| **TEILWEISE** | Teilverhalten ist implementiert beziehungsweise getestet, die weitergehende Zusage ist damit nicht bestätigt. |
| **NICHT NACHGEWIESEN / OFFEN** | Geeigneter Ausführungsnachweis fehlt, insbesondere Studio One/Studio Pro, ASIO und physische Maus-/Browserabläufe. Ein Risiko wird dadurch nicht zum bestätigten Absturz oder Dropout. |

Der LOW-Befund betrifft die derzeitige Bedienungsdokumentation. Die Zählung bedeutet daher nicht „14 ausgeführte fehlerhafte Audiotests“. Allgemeine Risiken und fehlende Abnahmen werden separat geführt und nicht zusätzlich als bestätigte Fehler gezählt.

## 2. Herkunft, Regeln, Umfang und Grenzen

Zuerst wurden `challanger2000/125A-Engineering/START-HERE.md` und die dort einschlägigen Entwicklungs-, Repository-, DSP-, GUI-, QA-, Echtzeit-, Release- und Forschungsregeln gelesen; Regelstand **`9854275cdad1d35476397d874ba95efc4f2b184a`**. Danach die projektspezifischen `AGENTS.md`, SDK-Baseline, V1-Scope, bisherigen unabhängigen Audits, Reparatur-/Integrationsberichte und beide WAV-Editor-Dokumente. Die vorgeschriebene Bestätigung „verstanden“ erfolgte vor der Prüfung. Maßgeblich ist der tatsächliche aktuelle Quellcode, nicht eine PASS-Tabelle des Autors.

Der aktuelle GitHub-Branchkopf wurde mit dem angeforderten vollständigen SHA abgeglichen. Die Untersuchung erfolgte in einem separaten Worktree auf genau diesem Objekt. Der Produktstand wird nicht mit einem möglicherweise veralteten lokalen Remote-Tracking-Ref verwechselt. `CMakeLists.txt` und Pluginmetadaten nennen weiterhin **0.1.0**; „V1“ bezeichnet den Abnahmeauftrag, keine in diesem Audit geänderte oder veröffentlichte Versionsnummer. Es wurde kein veröffentlichter FULL-Baselinebestand konstruiert oder ersetzt.

Untersucht wurden:

- Die vorhandenen 41 Core-Header, die VST3-Adapter einschließlich Factory, Controller, Processor, Editor, View, FLAC-Adapter und IDs sowie aktive GUI-Ressourcen und Build-/SDK-Einbindung.
- WAV-/FLAC-Einlese-, Sample-Exchange-, Voice-, SRC-, Mix-, Choke-, Routing-, Transport-, Pattern-, Song-, Automations-, Recall-, Undo- und Editorpfade, einschließlich neuer WAV-Bearbeitung und Vorhörengine.
- Bestehende Core-, State-, Geometrie-, native Integrations-, paketierte Host- und Mutationstests. Deren konkrete Orakel und Schnittstellen werden von den in Abschnitt 8 verbleibenden Abdeckungslücken getrennt.
- Alle im aktiven Custom-View vorhandenen Kontrollfamilien; Abschnitt 7 führt ihre Zuordnung bis zu Processor/DSP und Speicherung auf. Wiederholte Zeilen-/Stepkontrollen sind mit ihren vollständigen Indexbereichen erfasst. **Das ist kein behaupteter physischer Klicktest aller Kombinationen.**

Unveränderte Prüfgrenzen:

- **0 neue GitHub-Actions-Läufe**, kein lokaler Linux-/C++-Build, kein Pluginrepair, kein Release, keine Versionsänderung. Lokal liefen ausschließlich lesende Analysen, Git-Diff-Prüfungen und CMake-Skript-Quellgenerierung für die Testkopien.
- Kein Studio One/Studio Pro, kein ASIO-Gerät, kein Abhören, keine echte Explorer-/DAW-Browserbedienung. Ebenso keine eigenständige Prüfung im 125A PluginTester. Alle entsprechenden Abnahmen bleiben offen.
- Die Windows-Baseline wurde über Actions-/Checks-API einschließlich individueller Testannotations geprüft. Vollständige offizielle Log-/Artefaktdownloads sind hier durch den Cloud-Proxy nicht zuverlässig zugänglich. Insbesondere werden keine abgeschnittenen Detaildiagnosen ergänzt oder erfunden.
- Native Tests benutzen die tatsächlichen Produktklassen und SDK-/Win32-Pfade in einem synthetischen Host. Testinstrumentierung ist kein Ersatz für ein unverändertes DLL-/DAW-Lastprofil. Normale C++-Allokationszähler sind kein lückenloser Heap-, Lock-, Scheduler- oder Pagefault-Tracer.

Alle Produktzeilen dieses Berichts beziehen sich auf **8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289**, nicht auf spätere Reparaturen. GitHub-Quelllinks sind auf diesen SHA fixiert.

## 3. Tatsächlich vorhandene Windows-Evidenz und frühere Reparaturen

Ausgewerteter bestehender Lauf: [Actions #39938196621](https://github.com/challanger2000/125A-Drumator/actions/runs/39938196621), Job/Check **120362845347**, exakt Produktcommit **8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289**.

| Ebene | Vorhandenes Ergebnis | Was damit belegt ist / Grenze |
|---|---|---|
| Windows x64 Configure/Compile/Link | PASS | Der damalige Produkt-/Teststand kompiliert, nicht automatisch jede Funktion. |
| CTest | **23/23 PASS** | Bestehende registrierte Testprogramme; kein Ergebnis der neuen FA23-Fälle. |
| Native Integration | **78 einzeln als PASS protokollierte Fälle; 0 Fehler** | Darunter RE01–RE11, RM01–RM13, WE01 und die aktuellen Controller-/View-/Statefälle. |
| Steinberg Validator | **47 PASS, 0 FAIL** | Offizieller Validator im vorhandenen Workflow; kein Studio-One-Nachweis. |
| Paketierter Editorhost | PASS | Reales DLL/Win32-Fenster, Zoom 100/150 und Resize-Ablehnung; keine vollständige WAV-Modalbedienung. |
| Paketierter Audiohost | PASS | Einschließlich der unten konkret benannten T01/T02/T04/T28–T30-Fälle. |
| F14, 64 Stimmen, 16×, 48 kHz / 256 Frames | p95 **3,5791 ms**, p99 **3,6974 ms**, Maximum **3,7898 ms**, **0/180** Fristüberschreitungen | Frist 5,33333 ms; begrenzte Offline-CI-Stichprobe, keine garantierte Echtzeitfähigkeit. |
| F15, 64 Stimmen, 48 kHz / 256 Frames, je 32 Blöcke | p99/Maximum: 1× **0,0788 ms**, 4× **1,9123 ms**, 16× **3,8856 ms** | Bei n=32 entspricht nearest-rank p99 dem Maximum; keine belastbare Schätzung seltener Ausreißer. |
| Neue FA23-01–FA23-10 | **NICHT KOMPILIERT / NICHT AUSGEFÜHRT** | Nur Quellgenerierung und statische Testprüfung; kein neuer Actions-Lauf gestartet. |
| Studio One / ASIO / Hörvergleich | **NICHT DURCHGEFÜHRT** | Keine Aussage „aussetzerfrei“, „klanglich abgenommen“ oder „V1 freigegeben“. |

Die native Sammelannotation enthält alle 78 Fallabschlüsse, ihre nachfolgenden Detaildiagnosen sind ab RE09 abgeschnitten. Daher sind RE09/RE10 als bestandene Fälle belegbar, aber hier keine vollständigen fristspezifischen Messreihen beziehungsweise Importzähler daraus ableitbar. Die oben genannten F14/F15-Werte stammen aus den separaten, vollständig sichtbaren Voice-Testannotations.

### 3.1 Nachprüfung früherer Fehler – behobene Gegenfälle ausdrücklich anerkannt

| Ursprünglicher Gegenfall | Aktueller Status | Technischer Nachweis und verbleibende Grenze |
|---|---|---|
| V1A01 / Zero-Frame-Stop, alte Stimmen | **PASS, benannter Fall** | SOR03Z und RM01 bestehen; explizite Stopflanke wird vor dem Zero-Frame-Rücksprung verarbeitet. Das neue separate Audition-Lifecycleproblem FA10 wird davon nicht erfasst. |
| V1A02 / ursprünglicher deterministischer VFA02 | **PASS** | RE01: L[80] **0,393701**, gegenüber dem falschen Mischwert **0,196850**. Tatsächlicher Paste zwischen Längenlesen und Patternverbrauch. |
| V1A02 / alter vollständiger Fallback | **PASS** | RE03 prüft unterbrochenen Writer mit alter konsistenter 16er-Generation in PATTERN und SONG; RM02 erkennt die zurückgenommene Reparatur. |
| Früherer Cold-SONG-/Divisor-0-Pfad | **PASS, früherer Fehler behoben** | RE11 mit kaltem und warmem Kontrollpfad besteht; der Produktconsumer respektiert die Cachegültigkeit. Kein erneuter alter Absturzbefund. |
| V1A03 / Einzelstep- und FILL-Änderung eines zweiten Fensters überlebt altes Cancel | **PASS für diese Gegenfälle** | V1A03, RE04 und RM03 bestehen. RE04 vor/nach Cancel **127 → 127**. Der Controller beendet bestehende Gesten vor Peer-/Bulkmutation. FA08 betrifft zusätzlich Undo während einer noch aktiven Geste. |
| V1A04 / Component-only-Recall mit gleichem Dateinamen und anderem WAV | **PASS für den ursprünglichen Fremdherkunftsfall** | V1A04 und RM04 bestehen; Byteidentität wird geprüft, nicht nur der Name. |
| RE05 / eine vollständig abgeschlossene Recallpaarung darf keine Herkunftsfreigabe für nächstes Kit hinterlassen | **PASS, früherer Gegenfall behoben** | Beide ursprünglichen Reihenfolgen protokollieren `next-kit origin cleared ...=YES`. FA07 ist ein anderer Zyklus: unpaariger Component-only-Recall vor neuem vollständigen Controller-first-Recall. |
| R02 / RE06 reentrantes Speichern aus echtem `setDirty` | **PASS für den Importfall** | RE06: neues eingebettetes WAV vorhanden, `origin restored=YES`; Controller-Metadaten werden vor Dirty-Mitteilung vervollständigt. Nicht jeder fremde Hostcallback ist damit geprüft. |
| O01 / Arbitration, Equal, Different, ABA und Zero-Frame | **PASS, native benannte Fälle** | 12 O01-Varianten sowie FA02 und RE07 bestehen. RE07: beide Feedbackordnungen Processor B / Controller 1. Kein pauschaler Studio-One-Recall-PASS. |
| Z01 / Controllerprepare und OOM, alter Recall | **PASS, vorhandene gezielte Tests** | Vollständige Controller-Allokationsinjektion und Legacyfall bestehen; Daten/Gesten/Undo werden erst nach erfolgreicher Vorbereitung invalidiert. FA05 betrifft den später ergänzten Querypfad. |
| Z02 / Zoomtransaktion | **PASS für vorhandene native/paketierte Fälle; Gesamtmatrix TEILWEISE** | Frische DZM3-Instanzen mit vier Zooms, beide Streamordnungen, Fehlerpräfixe, OOM und Reopen: RE02 PASS. Native Views und DLL-Resize-Ablehnung PASS. Kombination mit allen neuen WAV-Modalaktionen und realem Monitorwechsel nicht ausgeführt. |
| Z03 / nativer Win32-Namenseditor | **PASS für vorhandene Pfade** | Z03 PASS; Generation wird vor Kitwechsel logisch invalidiert, native Fokus-/HWND-Bereinigung erfolgt geschützt. Kein neuer nachgewiesener Z03-Produktfehler. |
| T01 | **PASS** | Paketierter State wird nach 2T→1T→2T tatsächlich zurückgelesen, alle A–H/Rows/Tailzellen plus Negativkontrolle. |
| T02 | **PASS** | 15 unterschiedliche AUX-Stereocodes, Nachbarbus- und L/R-Vertauschungen als Negativkontrollen; kein reines „irgendwo nichtnull“. |
| T03 | **PASS für erzwungenes Interleaving** | Tatsächliche Patternbank, Writer/Reader mit kontrollierter Überlappung; kein bloß gleichzeitig gestartetes Hilfsmodell. |
| T04 | **PASS für native und paketierte Restorefälle** | Native Gateprüfung; paketiert 24 X/Y-Restores, kohärente X-Blöcke **39**, Y-Blöcke **35**, Gate-stille Blöcke **21.875**. Belegt Konsistenz unter dieser Last, nicht unterbrechungsfreies Audio während Recall. |
| T28 | **PASS** | Paketiert 48→44,1 kHz, ohne MIDI Step-1-Onset bei PPQ 4, unabhängiger Stereocode, 256 versus 128+128 und Stateerhalt. |
| T29 / altes FA01 | **PASS** | AUX1 mit MAIN null Kanäle, null/nichtnull Pointertabelle, Guard bleibt unverändert, frische Stereo-Signatur Offset 64 samt negativen Orakeln. |
| T30 / >256 Patternbefehle | **PASS** | Paketiert 300 MIDI-Befehle, MIDI-only D / Hosttie G, gerenderte und outputlose Pfade, Rückmeldungen an Offsets 255/256. |
| V1T01 / vollständige kritische Testabdeckung | **TEILWEISE** | Alte Blindstellen durch RE/RM wesentlich verbessert; die neuen FA01–FA13-Zustandswechsel fehlen bisher. Keine Zusage einer lückenlosen Suite. |
| V1T02 / vollständige aktive GUI-Abdeckung | **TEILWEISE** | Aktuelle native Kontroll-, Pixel-, Modal- und Compactfälle vorhanden; ältere geometrische Hilfsmodelle decken den neuen WAV-Editor nicht ab. |
| R01 / synchroner Import, R03 / harte Echtzeit, SOR04 / DAW-Aussetzerfreiheit | **NICHT NACHGEWIESEN** | Synchroner Kontrollpfad bleibt; Lasttests und Mutationen sind nützlich, ersetzen aber weder ASIO-Abnahme noch ein allgemeines Zeitlimit. Siehe Abschnitt 9. |

## 4. Neue konkrete Fehler, nach Priorität

### FA01 – BLOCKER – Sample leeren nach WAV-Bearbeitung verhindert Projektspeicherung

**Status: FAIL – Quellnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** [DrumatorProcessor.cpp:304–325](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorProcessor.cpp#L304-L325), insbesondere 323/324; [KitState.h:133–148](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/core/KitState.h#L133-L148); `DrumatorProcessor.cpp:1136–1166`, insbesondere 1162.

**Ursache:** `ClearSample` leert WAV und Dateiname, lässt aber `kit_.sampleEdits[row]` unverändert. Jeder übernommene Editorentwurf hat `end != 0`. `KitCodec::valid` lehnt einen solchen Edit ohne eingebettetes WAV zwingend ab, auch wenn `encode` mit `validateWav=false` arbeitet. `getState` gibt dann `kResultFalse` zurück.

**Gegenbeispiel/Reproduktion:** WAV laden → WAV EDITOR → Gain 50 % → ÜBERNEHMEN → Samplemenü → Sample leeren → tatsächlichen Processorstate speichern. Das Leeren wird erfolgreich bestätigt; danach scheitert die Serialisierung des **gesamten Plugins**, nicht nur der leeren Zeile. Dafür sind weder Race noch beschädigte Quelldatei nötig. Auch RESET mit anschließender Übernahme erzeugt einen expliziten Edit.

**Auswirkung:** Ein normaler Bedienablauf hinterlässt einen nicht speicherbaren Pluginzustand. Welchen Fehler Studio One bei fehlgeschlagenem `getState` anzeigt beziehungsweise ob es ältere Bytes behält, wurde nicht getestet; ein konkret beobachteter DAW-Projektverlust wird nicht behauptet.

**Empfehlung:** WAV, Name und Edit im selben erfolgreichen Clearcommit auf einen kanonischen Leerzustand setzen. Ablehnungs-/OOM-Pfade müssen das gesamte alte Tupel erhalten; übrige Zeilen, Patterns und Instrumentennamen nicht unbeabsichtigt zurücksetzen.

**Abdeckung:** Bestehende Clear-/Metadatentests verwenden keine zuvor bearbeitete Zeile. Neu **FA23-01**: echter Controllercommit → echtes Clear → `Processor::getState` → Decode und kanonischer Leeredit. Noch unausgeführt. Zusätzlich frische Instanz aus diesen Bytes restaurieren und Fehlerpfade prüfen.

### FA02 – HIGH – Veralteter WAV-Entwurf kann auf ein anderes Sample angewendet werden

**Status: FAIL – Quellnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** `DrumatorView.h:66–72`; [DrumatorView.cpp:948–970](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorView.cpp#L948-L970), `:454–480`; `DrumatorController.cpp:833–849`, `:1083–1089`; [DrumatorProcessor.cpp:186–231](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorProcessor.cpp#L186-L231).

**Ursache:** Der View speichert Zeile, Frames und Edit, aber keine Source-/Editgeneration. Die Commitnachricht enthält nur Zeile und Bearbeitungswerte. Der Processor validiert gegen das **jetzt** in der Zeile liegende Sample. Ein erfolgreicher Sampleimport in einem anderen Fenster aktualisiert die Views, invalidiert aber keinen offenen WAV-Entwurf. Bei gleicher oder ausreichend großer Länge sind alte Werte weiterhin gültig. Auch zwei gleichzeitig offene WAV-Entwürfe derselben Source haben keinen Konfliktschutz gegeneinander.

**Gegenbeispiel:** Fenster A öffnet Source A mit 1.024 Frames und stellt Gain 50 % ein. Fenster B lädt Source B mit anderen Daten, ebenfalls 1.024 Frames, in dieselbe Zeile; der Samplewechsel setzt den gespeicherten Edit zurück. Fenster A drückt anschließend ÜBERNEHMEN. Sein alter Edit wird auf B gespeichert, obwohl A noch die Wellenform von A zeigt. Nachweis ohne unzulässige parallel ausgeführte UI-Threads: serialisierte Callbacks zweier Views genügen.

**Auswirkung:** Falsche Bearbeitung des neuen Samples und stilles Überschreiben zwischen Fenstern. Der vollständige Kit-Recall schließt den Editor bereits korrekt; der Befund betrifft Samplewechsel und konkurrierende Editcommits, nicht eine erneute Behauptung des alten Recallgestenfehlers.

**Empfehlung:** Snapshot, Preview und Commit an stabile Source- und Editrevision binden. Nur einen erwarteten, unveränderten Stand übernehmen; sonst sichtbar ablehnen und neu einlesen. Das Sperren von Drops allein löst den Peer-View-Fall nicht.

**Abdeckung:** WE01 hat keinen offenen Modalentwurf während Samplewechsel. Neu **FA23-05** benutzt den echten View-Gain-/Commitpfad und einen echten Controllerimport als Peeraktion. Noch unausgeführt. Zusätzlich zwei Modalviews mit derselben Source, Clear, Hide/Show und abgelehntem Import prüfen.

### FA03 – HIGH – Vorhöranforderung und Sampleübernahme sind nicht dieselbe Transaktion

**Status: FAIL – deterministischer Quell-Interleavingnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** [DrumatorProcessor.cpp:175–183](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorProcessor.cpp#L175-L183), `:841–842`, `:1076–1080`; `SampleExchange.h`.

**Ursache:** Der Kontrollthread publiziert ein PreparedSample und setzt separat `auditionStart_`. Der Audiothread adoptiert Samples am Blockanfang und konsumiert das Startbit erst nach dem normalen Rendering. Release/Acquire für beide Einzeloperationen verbindet den bereits vergangenen Adoptzeitpunkt nicht mit dem später gelesenen Startbit.

**Zulässige Reihenfolge:** Audio passiert `auditionExchange_.audioAdopt` → UI publiziert den ersten Preview und setzt Start=true → Audio liest Start=true und startet Row 0 mit dem zuvor adoptierten, beim ersten Preview leeren Asset → nächster Block adoptiert das neue Asset, Start ist jedoch schon verbraucht. Die akzeptierte Anforderung wird nie hörbar. Bei vorhandenem alten Preview kann stattdessen noch dessen Sound gestartet werden.

**Auswirkung:** VORHÖREN reagiert abhängig vom Callbackzeitpunkt nicht oder spielt einen veralteten Entwurf. Das ist ein logischer Publikationsfehler, keine Behauptung eines ungeordneten Zugriffs auf freigegebenen Speicher.

**Empfehlung:** Generation beziehungsweise Startkommando an genau das veröffentlichte Asset binden und erst nach dessen tatsächlicher Übernahme ausführen. Eine nach dem Blockbeginn eintreffende Anforderung darf auf den nächsten Block verschoben, aber nicht gegen ein anderes Asset konsumiert werden. STOP/neuere Preview müssen eindeutig geordnet sein; keine Locks/Allokationen als Audiothreadreparatur einführen.

**Abdeckung:** Neu **FA23-03** pausiert den tatsächlichen Processor unmittelbar nach Audition-Adopt und publiziert über den echten Controller. Das Orakel akzeptiert den aktuellen **oder** nächsten Block und verwechselt zulässige Blocklatenz nicht mit einem Fehler. Hook nur in generierter Testkopie; noch unausgeführt. Zusätzlich alte/neue unterscheidbare Entwürfe und STOP zwischen Publish/Adopt prüfen.

### FA04 – HIGH – VORHÖREN stapelt Stimmen statt den One-Shot neu zu starten

**Status: FAIL – Quellnachweis mit eindeutigem PCM-Gegenbeispiel. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** `DrumatorView.cpp:1111–1116` verspricht ausdrücklich Neustart; `DrumatorProcessor.cpp:151–184`, `:1076–1080`; [SampleVoiceEngine.h:306–347](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/core/SampleVoiceEngine.h#L306-L347).

**Ursache:** Nur ein separates STOP setzt die Vorhörengine zurück. Eine weitere Preview setzt Start=true und löst einen zusätzlichen normalen Voice-Launch aus; noch aktive Vorhörstimmen werden nicht beendet. Der Kommentar „separate audition voice“ in der Dokumentation entspricht hier einer zweiten Engine mit bis zu 64 Voices.

**Gegenbeispiel:** Source mit 1.024 Frames konstant L=0,25/R=−0,5 bei Unity. Preview, 256 Frames rendern; ohne STOP nochmals Preview, nächsten Block rendern. Im Plateau werden L=0,5/R=−1 erwartet aus dem fehlerhaften Addieren, statt der beim Neustart korrekten L=0,25/R=−0,5. Gleichartige weitere Klicks erhöhen Pegel und Stimmenzahl. Dies ist vom natürlichen Ende eines kurzen Samples zu unterscheiden.

**Auswirkung:** Unzutreffende Lautstärke beim Vergleichen von Edits, gegebenenfalls Clipping und zusätzliche SRC-Last neben den normalen 64 Voices.

**Empfehlung:** Eindeutigen Vorhör-Neustartvertrag implementieren: bisherige Audition beenden/kurz ausblenden und ausschließlich die neue Generation starten. Normale Pattern-/MIDI-Stimmen müssen erhalten bleiben. Zusammen mit FA03 lösen; ein UI-STOP vor jedem Publish allein ist kein vollständiger Generationsvertrag.

**Abdeckung:** WE01 wiederholt Preview erst nach explizitem STOP; es entdeckt das Stapeln nicht. Neu **FA23-02** prüft zwei noch überlappende tatsächliche Previews mit exakten unterschiedlichen L/R-Werten. Noch unausgeführt. Ergänzen: viele schnelle Klicks, Voicezähler, Ende erreicht, FA03-Interleaving und wechselnder Entwurf.

### FA05 – HIGH – Speicherfehler beim Wellenformquery kann über die VST3-Grenze entweichen

**Status: FAIL – Quellnachweis am gepinnten SDK. Neue Fehlerinduktionsausführung nicht erfolgt.**

**Dateien:** [DrumatorProcessor.cpp:121–141](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorProcessor.cpp#L121-L141), insbesondere 135; `DrumatorController.cpp:772–831`; `DrumatorView.cpp:948–955`. Gepinntes SDK `public.sdk/source/vst/hosting/hostclasses.cpp:153–156`, `:294–299`.

**Ursache:** `SampleEditorQuery` kopiert die vollständigen WAV-Bytes per `IAttributeList::setBinary` in die Nachricht, ohne lokale Exceptiongrenze. Die tatsächlich im Testhost verwendete SDK-Implementierung allokiert im binären Attribute-Konstruktor mit `new char[size]`; `setBinary` fängt `std::bad_alloc` nicht. Auch Controller-Query und View-Open fangen diese Exception nicht ab. Der Controller hält dort zudem einen manuell zu releasenden Messagepointer.

**Gegenbeispiel:** Gültige Row mit WAV; Querynachricht und Rowattribut vorab anlegen; exakt die nächste Allokation beim Aufruf des tatsächlichen `Processor::notify` fehlschlagen lassen. Die WAV-Kopie wirft aus `notify`, statt einen VST3-Fehlercode zurückzugeben. Ob ein konkreter Host das ausnahmsweise abfängt oder beendet wird, bleibt ungetestet; der unsichere Boundarypfad selbst ist im SDK nachvollziehbar.

**Empfehlung:** Allokierende neue Query-/Nachrichtenpfade mit einer definierten Ausnahmegrenze versehen, OOM als `kOutOfMemory`/sichtbaren Fehler behandeln, Nachrichtenbesitz per RAII sichern. Fehler dürfen weder Sample/Edit noch geöffnete Transaktionen teilweise ändern. Fremde SDK-Warnungen nicht als Ersatz für diesen eigenen fehlenden Schutz abschalten.

**Abdeckung:** Z01 injiziert Controller-Recallfehler, nicht diesen späteren Nachrichtenzweig. Neu **FA23-10** ruft die echte Notifyfunktion mit einer vorbereiteten SDK-HostMessage unter Allocation-Failpoint auf. Noch unausgeführt. Weitere Failpoints einschließlich Attributmap, Controllerkopie und View-Aufruf ergänzen.

### FA06 – MEDIUM – WAV-Modalfenster blockiert darunterliegende Dropziele nicht

**Status: FAIL – Quell-/Geometrienachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** `DrumatorView.cpp:314–320`, `:807–810`, [DrumatorView.cpp:909–927](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorView.cpp#L909-L927); `UiCompactLayout.h:9–16`.

**Ursache:** `sampleDropTargetAt` prüft Songeditor und Menüs, aber nicht `sampleEditorOpen_`. Die Modalfläche deckt x=230…1050/y=100…605 ab; Row-0-Sampleziel x=78…272 liegt teilweise darunter.

**Gegenbeispiel:** WAV-Editor öffnen und ein gültiges anderes WAV bei logischer Position **(250,128)** droppen. Der Punkt liegt sichtbar im Modalfenster und gleichzeitig im weiterhin akzeptierten Sampleziel. Die Datei wird in Row 0 geladen. Gewöhnliche Mausklicks werden im Modalzweig korrekt abgefangen; D&D nutzt einen anderen Eintrittspfad.

**Empfehlung:** Modale Sperre für alle Drag-/Dropphasen konsistent anwenden, Vorschaucursor und tatsächliche Annahme identisch behandeln. Keine vorhandenen Daten verändern, wenn Ziel gesperrt ist. FA02 muss trotzdem gegen Peer-Views abgesichert werden.

**Abdeckung:** FA04/FA05 der bestehenden Suite betreffen Pfadfehler beziehungsweise andere Menüs. Neu **FA23-04** öffnet den echten WAV-Modalview und liefert ein echtes Filepackage an Enter/Drop. Noch unausgeführt. Weitere Grenzen, 100/130/150 %, DPI und verschiedene sichtbare Rowzahlen ergänzen.

### FA07 – MEDIUM – Unpaariger Recall lässt die Herkunft des nächsten vollständigen Recalls verschwinden

**Status: FAIL – Quellnachweis; neuer Nachfolgezyklus, nicht der bereits behobene RE05-Gegenfall.**

**Dateien:** [DrumatorController.cpp:161–165](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorController.cpp#L161-L165), `:221–239`.

**Ursache:** Zwei unscoped Boolmarker sollen die Zuordnung von Controller- und Componentstreams abbilden. Jeder unpaarige Component-Recall setzt `componentStatePendingController_`. Ein späterer Controllerstream wird damit als Abschluss dieses alten Recalls behandelt, auch wenn er bereits zu einem neuen Projekt gehört. Seine explizite Herkunft bekommt dann keine Freigabe für den folgenden Componentstream.

**Gegenbeispiel:** X aus `kick.wav` laden und beide Streams speichern → Component-only Y mit gleichem Namen, aber anderem WAV-Inhalt in Processor und Controller restaurieren → vollständiges X in zulässiger Reihenfolge Controllerstate(X), Processorstate(X), Controller-Componentstate(X) restaurieren. Der X-Controllerstream stellt den korrekten X-Pfad wieder her, disarmt aber wegen Y die Freigabe. Der folgende X-Componentstream vergleicht gegen Y-Identität und löscht X-Pfad. Das eingebettete Audio bleibt korrekt; `<`/`>`-Ordnernavigation verliert ihre Grundlage.

**Empfehlung:** Herkunft an Asset-/Stateidentität beziehungsweise einen belastbaren Restorevertrag binden, nicht an einen zeitlich unbeschränkten „noch ein Stream erwartet“-Marker. DZM3-Kompatibilität und erlaubte Component-only-Recalls erhalten. Keine „Reparatur“, die alle gleichnamigen fremden Sources wieder akzeptiert.

**Abdeckung:** RE02/RE05 prüfen frische oder vollständig gepaarte Zyklen. Neu **FA23-06** kombiniert tatsächlichen Processorrestore mit dem zusätzlichen unpaarigen Y und nachfolgendem vollständigen X. Noch unausgeführt. Umgekehrte Reihenfolge, mehrere unpaarige Zyklen und fehlende Originaldatei ergänzen.

### FA08 – MEDIUM – Undo während Velocity-Geste vermischt Patterninhalt und alte Länge

**Status: FAIL – Quell-/Aliasingnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** `DrumatorController.cpp:461–485`, [DrumatorController.cpp:527–532](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorController.cpp#L527-L532), `:674–685`; `PatternUndo.h`.

**Ursache:** `undoCurrentPattern` nimmt einen Pointer auf `undo_.snapshot(p)` und die Länge als Wert für `replacePattern`. Dort wird zunächst die aktive Geste beendet. Dieses Ende überschreibt denselben Undo-Snapshot, auf den der übergebene Datenparameter referenziert. Die bereits als Funktionsargument kopierte Länge bleibt dagegen die ältere. Es handelt sich um sequenzielles Aliasing; kein paralleler UI-Thread ist erforderlich.

**Gegenbeispiel:** A hat 32 Steps und Step 1=60; ein 16er-B mit Step 1=90 wird nach A gepastet. Undo enthält nun das ursprüngliche 32er-A. Neue Velocity-Geste auf dem aktuellen 16er-A beginnen und Step 1 auf 80 setzen. In einem zweiten Fenster Undo auslösen, bevor Mouse-up der ersten Geste kommt. Beim Gestenabschluss wird das Undo-Datenarray durch den 16er-Vorgestenzustand (90) ersetzt, die zuvor gelesene Undo-Länge bleibt 32. Ergebnis ist ein Mischzustand 90/32 statt eines vollständigen Undo-Eintrags.

**Empfehlung:** Zuerst aktive Geste definiert abschließen, danach den vollständigen Undo-Eintrag aus Steps und Länge als unveränderlichen lokalen Wert erfassen und anwenden. Ob Undo die neue Geste oder den älteren Paste zurücknehmen soll, muss konsistent festgelegt sein; keinesfalls unterschiedliche Generationen mischen. Ablehnung darf den Eintrag nicht verbrauchen.

**Abdeckung:** Neue **FA23-07** erwartet Undo der zuletzt abgeschlossenen Geste auf 90/16 und prüft den tatsächlichen Processorstate mit. Noch unausgeführt. Falls ein anderer Undo-Vertrag beschlossen wird, muss dessen kohärente Alternative ebenfalls explizit getestet werden; das alte Aliasverhalten ist keine zulässige Alternative.

### FA09 – MEDIUM – Resampling bezieht Daten außerhalb der START-/END-Auswahl ein

**Status: FAIL bezüglich einer isolierten, harten Trim-Auswahl – Quellnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** [SampleVoiceEngine.h:213–258](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/core/SampleVoiceEngine.h#L213-L258), `:268–281`; Kernelkoeffizienten `:48–69`.

**Ursache:** Voiceposition und Ende werden auf den Edit beschränkt, der Sinc-Faltungskern prüft dagegen nur die Grenzen des gesamten Originalsamples. Bei Nicht-Unity-Rate fließen entfernte Prefix-/Suffixdaten in die Samples der ausgewählten Region ein. Die nachträgliche Edit-Hüllkurve entfernt diese Beiträge nicht, wenn ihr Wert in der Auswahl ungleich null ist. Es ist kein Out-of-bounds-Speicherzugriff, sondern ein Zugriff außerhalb der gewählten Audioregion.

**Gegenbeispiel:** Monoquelle 256 Frames, ausschließlich Frame 63=1, alle anderen 0. Auswahl `[64,128)`, keine Fades, Gain 100 %, Ratio 2. Die gesamte ausgewählte Region ist still. Erster Renderpunkt ist 64, 64-Tap-Kern beginnt bei 33; Tap 30 liest trotzdem Frame 63 mit einem Koeffizienten von ungefähr **0,3157**. Das ist aus der Produktformel ableitbar, keine in diesem Audit aufgezeichnete Audiozahl. Dasselbe Problem besteht am END-Rand mit einem außerhalb liegenden Impuls. Bei Ratio 1 ist der Direktpfad dagegen isoliert.

**Auswirkung:** Ein vor START abgeschnittener Attack beziehungsweise ein nach END liegender Transient kann in eine scheinbar stille Auswahl hineinwirken. Für eine Musikeroberfläche mit START/END muss die Randsemantik eindeutig sein. Falls bewusst ein Filter mit Kontext aus dem entfernten Original gewünscht wird, muss diese abweichende Semantik ausdrücklich beschlossen und dokumentiert werden; sie entspricht nicht dem hier geprüften Vertrag einer isolierten Auswahl.

**Empfehlung:** Trim als eigene Sampledomäne für den SRC behandeln, zum Beispiel mit spezifiziertem Padding innerhalb `[start,end)`, und gegen eine unabhängig ausgeschnittene Referenz vergleichen. Original-WAV trotzdem unverändert speichern. Nicht lediglich den Unity-Pfad prüfen.

**Abdeckung:** Vorhandener `testNondestructiveSampleEdits` benutzt nur Ratio 1 und konstantes Material. Neu **FA23-08** prüft wirklichen Renderer bei Ratio 2 mit Prefix-/Suffiximpuls und stiller Auswahl. Noch unausgeführt. Ergänzen: Stereo, 44,1↔48 kHz, Pitch ±, kurze Auswahl, Fades und unabhängiges Referenzpadding.

### FA10 – MEDIUM – Audition überlebt Deaktivierung und setzt beim Reaktivieren fort

**Status: FAIL – Quellnachweis. Neue Windows-Reproduktion nicht ausgeführt.**

**Dateien:** [DrumatorProcessor.cpp:91–101](https://github.com/challanger2000/125A-Drumator/blob/8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289/src/vst3/DrumatorProcessor.cpp#L91-L101), `:601–605`, `:1076–1080`.

**Ursache:** `setProcessing(false)` und `setActive(false)` resetten nur die normalen Voices und normale Previewbits, nicht `auditionVoices_`, `auditionStart_` oder `auditionStop_`. Erst ein erneutes `setupProcessing` setzt Audition zurück. Der Host muss bei unverändertem Setup jedoch nicht zwischen jeder De-/Reaktivierung ein neues Setup senden.

**Gegenbeispiel:** Langes WAV vorhören, ersten 256er-Block rendern → Processing false, Active false → Active true, Processing true → ohne neuen Previewtrigger rendern. Die alte Auditionvoice läuft ab ihrer alten Position weiter. Auch ein vor dem ersten Render anstehendes Startbit kann die Lebenszyklusgrenze überleben.

**Empfehlung:** Im quieszenten Lifecycle alle Auditionvoices und zugehörigen Kommandogenerationen definiert abbrechen. Das ist von Hosttransport-Stop zu unterscheiden: manuelles Vorhören bei gestopptem Transport soll weiterhin möglich sein. Reclamation nicht in einen laufenden Audiocallback verlegen.

**Abdeckung:** WE01 stoppt Preview ausdrücklich, bevor Lifecycleaufrufe erfolgen. Neu **FA23-09** prüft echte De-/Reaktivierung ohne erneutes Setup und verlangt Stille ohne neue Anforderung. Noch unausgeführt; auch noch nicht adoptierte Anforderung und outputlose Phase prüfen.

### FA11 – MEDIUM – Fadegriffe sind im normalen Reset-Zustand unsichtbar

**Status: FAIL – konkrete Zeichen-/Trefferflächenabweichung. Kein neuer Pixeltest ausgeführt.**

**Dateien:** `DrumatorView.cpp:829–843`, `:1055–1065`.

**Ursache/Nachweis:** Bei voller Ansicht und RESET sind START=0, END=N, beide Fades=0. Damit liegen Fade-In-X=284 und Fade-Out-X=994. Gezeichnet werden die gefüllten Griffrechtecke nur für **289≤X≤989**. Beide Griffe fehlen genau im häufigsten Anfangszustand. Die obere Trefferfläche mit Radius 13 bleibt trotzdem aktiv; Linien an den Rändern sind sichtbar, ersetzen aber keine erkennbaren Fadegriffe. Die Anweisung „FADE-GRIFFE OBEN ZIEHEN“ verweist auf nicht gezeichnete Griffe.

**Auswirkung:** Der normale Benutzer muss die versteckte Griffzone kennen; oben und unten an derselben Linie gelten unterschiedliche Funktionen. Das ist kein Beleg, dass Fade grundsätzlich unbedienbar ist, aber ein konkreter Discoverability-/Hitboxfehler.

**Empfehlung:** Randgriffe sichtbar innerhalb der Waveform zeichnen und Zeichen-/Trefferrechtecke gemeinsam definieren. Bei sehr kurzer Auswahl beziehungsweise überlappenden Griffpositionen eine eindeutige sichtbare Auswahl ermöglichen. Keine notwendige Tastenkombination als Ersatz für den fehlenden Griff einführen.

**Abdeckung:** Bestehende Pixeltests öffnen diesen Modalzustand nicht. Erforderlich: echte Modal-Pixel-/Hittests für Reset, Einframe-Datei, beide Kanten und 100/130/150 % plus DPI. Nicht Bestandteil der zehn noch unausgeführten FA23-Fälle.

### FA12 – MEDIUM – Gültige Float-WAVs zeichnen über den Wellenformbereich hinaus

**Status: FAIL – Quell-/Koordinatennachweis. Kein neuer Pixeltest ausgeführt.**

**Dateien:** `WavPcm16Decoder.h:116–120`; `DrumatorView.cpp:813–819`.

**Ursache:** Der Decoder akzeptiert endliche Float-Werte bis Betrag 16. Die Wellenform zeichnet unverändert `y=278−peak*96`, ohne eigenen Clip auf das Signalpanel und ohne Amplitudenskalierung. Das Panel liegt bei y=175…382. Der normale Viewclip beschränkt auf den gesamten View, nicht auf dieses Panel.

**Gegenbeispiel:** Gültige Float-WAV mit bipolar ±4 innerhalb derselben Envelopebins. Linien reichen rechnerisch von y=−106 bis y=662 und überzeichnen unter anderem Titel, Beschriftungen und Schaltflächen. Selbst ±2 reicht über das Panel hinaus. Eine konstante +4-Datei allein ist kein gutes Pixelorakel, weil deren Nulllängenlinie vollständig außerhalb des Views liegen kann; bipolarer Inhalt macht die Überschneidung eindeutig.

**Empfehlung:** Darstellung auf das Signalpanel clippen und einen nachvollziehbaren Anzeigemaßstab verwenden. Den tatsächlich zulässigen Audioinhalt dabei nicht normalisieren oder wegclippen. Clipped-/Headroomanzeige und Gainfeedback sollten verständlich sein.

**Abdeckung:** Erforderlich: echte Modalpixel mit ±1/±4/±16, Mono/Stereo und Zoomstufen; Kontrollpixel außerhalb des Panels müssen unverändert bleiben. Bisher kein solcher Test; keine tatsächliche Screenshotausführung behauptet.

### FA13 – MEDIUM – Fehlertexte des WAV-Editors werden vom eigenen Modalfenster verdeckt

**Status: FAIL – Quell-/Draworder-Nachweis. Kein neuer Pixeltest ausgeführt.**

**Dateien:** `DrumatorView.cpp:682–691`, `:807–883`, `:992`, `:1035`, `:1115–1128`; `UiCompactLayout.h:9`, `:28–41`.

**Ursache:** Preview-, Commit-, Zoom- und Scrollfehler werden nur in `status_` geschrieben. Dieser Text wird vor dem Modalfenster im normalen Footer gezeichnet. Bei vier sichtbaren, nicht expandierten Rows ist gridBottom=484, Statusrechteck **(414,489)–(1016,518)**. Die danach gezeichnete Modalfläche **(230,100)–(1050,605)** verdeckt es vollständig. Der Modaleditor hat keinen eigenen Fehlertext.

**Gegenbeispiel:** WAV-Editor öffnen, im realen Fehlerpfad einen abgelehnten Preview-/Commitrequest erzeugen, zum Beispiel Processorverbindung verlieren oder einen mittlerweile unpassenden Entwurf übergeben. `status_` enthält einen Fehler, der Benutzer sieht nur ein stehengebliebenes Modalfenster. Gleiche verdeckte Rückmeldung bei fehlgeschlagener Waveformaktualisierung.

**Empfehlung:** Fehlerzustand innerhalb des Modaleditors sichtbar zeichnen, mit eindeutigem Erfolg/Ablehnung und erhaltendem Entwurf. Eine interne Statusstringassertion genügt nicht; der Text muss nach Draworder und Clip tatsächlich sichtbar sein.

**Abdeckung:** Kein vorhandener echter Modal-Fehlerpixeltest. Ergänzen: kontrollierte Ablehnung über tatsächlichen Controller/Processor, sichtbares Modalfehlerfeld, unveränderte Daten und weiter bedienbares ABBRECHEN bei 100/130/150 % und unterschiedlichen Rowzahlen.

### FA14 – LOW – WAV-Bedienungsdokumentation beschreibt den aktuellen Editor falsch

**Status: FAIL – Dokumentabgleich.**

**Dateien:** `docs/WAV-EDITOR-IMPLEMENTATION.md:5`, `:16–24`; ergänzend `docs/WAV-EDITOR-INTERNAL-AUDIT-20261023.md:9–34` als zeitlich fortgeschriebener interner Bericht.

**Ursache/Nachweis:** Die Implementierungs-/Abnahmeanleitung nennt Fadebuttons, OK und fehlendes Scrollen. Der aktuelle View hat Fadegriffe, ÜBERNEHMEN, getrennte VORHÖREN/STOP-Tasten und einen proportionalen Scrollbalken. Der interne Bericht korrigiert den alten Toggle in einem Supplement, seine historischen Teilstände dürfen jedoch nicht als Beschreibung des aktuellen Standes oder als Beweis eines weiterhin fehlenden CI-PASS zitiert werden.

**Empfehlung:** Aktuelle Benutzer-/QA-Anleitung nach bestätigter Reparatur konsistent auf die tatsächlichen Bedienelemente und ihre Semantik umstellen; historische Aussagen klar datieren. Die Anleitung darf den Vorhör-Neustart erst nach tatsächlicher Behebung von FA03/FA04 versprechen.

**Abdeckung:** Dokumentreview erforderlich, kein Audiotest. Dieser Audit ändert ausschließlich diesen neuen Bericht und unabhängige Tests, nicht bestehende Produktdokumente.

## 5. Weitere technische Prüfung – positive Ergebnisse und verbleibende Grenzen

### 5.1 Audioengine, Resampling, Stimmen, Mix und Routing

`SampleVoiceEngine.h` hat feste Voicekapazität 64, validierte Trigger und keine dynamische Voiceallokation im Renderloop. Der Unity-Rate-Pfad kopiert Integerpositionen direkt; andere Raten verwenden vorbereitete phasenquantisierte Blackman-Sinc-Kerne mit ratioabhängig 32/64/128/256 Taps. Pitch und Source-/Outputrate stecken im Kernelratio. `PreparedSample` bereitet PCM/Kern außerhalb `process()` vor; alte Voice-Owner werden über `SampleExchange` bis zum letzten Nutzer gehalten und auf dem Kontrollthread entsorgt. Bestehende Lifetime-/Stress-/Alias- und Kapazitätstests bestehen. **FA03/FA04/FA09/FA10 sind zusätzliche Fehler beziehungsweise Verträge dieser neuen Audition-/Editpfade, kein Beleg gegen den kompletten normalen Exchange-Lifetimepfad.**

Choke wird erst nach erfolgreicher Suche eines freien Voiceplatzes angewendet; ein kapazitätsbedingt abgelehnter Hit würgt bestehende Stimmen nicht ab. Mono wird auf beide Kanäle abgebildet, Stereo erhält getrennte Kanalwerte. Pan ist Stereo-Balance mit Unity in der Mitte, kein zusätzlicher Centerboost. Mute/Solo, Gain, Pan, Choke und Bus werden beim Voice-Launch wirksam; bereits klingende Tails behalten ihre kopierten Werte. Das ist vorhandene Semantik, keine belegte Echtzeitautomation dieser Tails. Note-off beendet den One-Shot nicht; bei Kapazität 64 werden weitere Hits abgelehnt statt automatisch gestohlen. Diese Produktentscheidungen müssen für Nutzer klar sein.

16 feste Stereoausgänge, davon MAIN plus 15 optionale Stems; mehrere Rows auf gleichem Bus werden summiert. Bei gültigem aktivem Zielbus wird ausschließlich dort geschrieben, bei nicht verfügbarem Stem auf MAIN zurückgefallen. Alle optionalen Busse werden wegen der bisherigen Studio-One-Erfahrung als `kMain` statt `kAux` registriert; das verändert nicht die eigene Busindexzuordnung. Arrangementverhandlung akzeptiert genau die feste Stereoanordnung. **T02/T29, Voice-Routing-Tests und paketierte Guard-/Negativorakel bestätigen benannte Fälle**, auch MAIN mit null Kanälen und AUX-only. Eine reale Studio-One-Busaktivierung wurde hier nicht ausgeführt.

Master betrifft MAIN und aktive Stems mit derselben vorbereiteten Glättung; Audition wird MAIN vor Master zugemischt. Audition verwendet derzeit keine Row-Mute/Solo-/Volume-/Pan-/Buswerte, wohl aber Row-Pitch und Master. Ob dies der gewünschte Vorhörvertrag ist, benötigt eine Produktentscheidung; ohne eine gegenteilige spezifizierte Zusage ist es hier **kein zusätzlicher Routingfehler**. Durch FA04 können insgesamt normale 64 plus bis zu 64 zusätzliche Vorhörvoices arbeiten; die alte reine 64-Voice-Lastdiagnostik deckt diese Kombination nicht ab.

NaN/Inf-/Rangeprüfungen sind an WAV-, SampleEdit-, Tempo-, Pitch-, Pan- und Setupgrenzen vorhanden. Endliche Float-WAV-Headroomwerte bis ±16 und Editgain bis 4 sind zulässig; Summen dürfen über 0 dBFS liegen, es gibt keine versteckte Normalisierung/Begrenzung. Das ist kein Grund, die Audiodaten zur Behebung von FA12 zu beschneiden. Separate Vollast-/Denormalmessungen über sämtliche Lifecycle- und GUIkombinationen fehlen. Keine neue allgemeine Overflow-/NaN-/Denormalfreiheit wird aus dem Build abgeleitet.

### 5.2 Sampleimport, Pfade und Originaldaten

WAV akzeptiert validierte RIFF-/Format-/Datenchunks, unterstützte PCM-Bittiefen und 32-Bit-Float, Mono/Stereo, begrenzte Frames und Containergrößen. FLAC wird begrenzt in Float-WAV für denselben Sample-/Statepfad konvertiert. „Original-WAV bleibt erhalten“ ist für direkt importierte WAV-Bytes zutreffend; beim FLAC-Import wird **konvertiertes WAV**, nicht das ursprüngliche FLAC-Containerfile, eingebettet. Edits sind nicht destruktiv; sie speichern Metadaten neben dem WAV.

Dateifehler, ungültige Strings/URLs, UTF-8-/Windows-Pfadbehandlung, alphabetische Navigation mit Umlauf, Ablehnung und Erhaltung des vorigen Samples sind in bestehenden N12-/FA04-/Decoderfällen mit tatsächlichen Dateien geprüft. Die neue WAV-Modalität wurde nachträglich ergänzt und fehlt in deren Sperrmatrix: FA06. Unterschiedliche Dateien mit gleichem Namen werden für die Herkunft anhand WAV-Identität unterschieden, nicht anhand Name allein; FA07 zeigt einen noch nicht geprüften Folgezyklus.

Die in `KitState.h` gesetzten Budgets für Einzel-WAV und gesamte WAV-Payload sowie dekodierte Framegrenzen sind echte obere Eingabegrenzen. Sie sind **kein Beweis**, dass der Spitzen-RAMbedarf nur so hoch ist: Containerkopie, dekodierte Kanalvektoren, PreparedSample, Kern, alte aktive Sources, Queryattribute und serialisierter State können zeitweise gleichzeitig existieren. Fehlererhaltung ist bei den ursprünglichen Import-/Recalltests belegt, aber nicht beim neuen Query-OOM FA05.

### 5.3 Sequencer, Pattern, Song, MIDI und Automation

Der Scheduler ist PPQ-basiert, arbeitet mit Audiooffsets, expliziter Tempo-/Positionsgültigkeit und 16/32er Patternlängen. 1T ist 16, 2T ist 32; die Tailsteps 17–32 bleiben beim Wechsel erhalten. Pattern A–H werden über den bestehenden historischen Normalisierungsvertrag quantisiert; nicht auf eine neue inkompatible Achtelrasterung umstellen. T01/T28, RE01/RE03/RE11, 300-Command-T30, vollständige PCM-RE08 und Host/MIDI-Arbitration bestätigen die konkreten Fälle. Swing/Humanize benutzen begrenzte, deterministische Schedulingwerte. Daraus folgt kein pauschal hörpsychologischer Timing-PASS für jede Loop-/Tempoautomation.

Hostautomatisierbarer VST3-Parameter ist **Pattern, ParamID 1000**. Row-/Master-/Swing-/Humanize-/Editwerte werden über eigene Nachrichten und Kitstate verwaltet, nicht als zusätzliche automatisierbare VST3-Parameter exponiert. Die Zuordnung im GUI ist vorhanden; eine Behauptung „alle Regler sind hostautomatisierbar“ wäre falsch. Das wird als aktuelle Funktionsgrenze dokumentiert, nicht als neu erfundene Anforderung ausgelegt.

Patterncopy/paste enthält Steps und Dauer; Duplicate sucht ein tatsächlich leeres Ziel und überschreibt keine belegten Patterns. Clear-/Filloperationen auf 1T schützen versteckte Tailsteps. Random Velocity ist eine einmalige Änderung vorhandener Noten, nicht fortwährendes Audio-Randomisieren; ein separates historisches Randomwertfeld ist weiterhin rückwärtskompatibel im State. Bestehende P18/P19/V16/V17/RANDOM-Fälle prüfen diese Unterschiede. Undo hat zusätzlich den neuen kohärenten Eintragsfehler FA08.

Song hat begrenzte 64 Einträge, eigenes paginiertes Board und validierte 8/16/32-Eintragslängen. Patterndauer/Patterninhalt werden als Generation gelesen; bei unterbrochener Veröffentlichung wird der letzte **gültige** komplette Cache genutzt. Der frühere kalte Cachefehler ist behoben. RE03/RE11 und FA03 der bisherigen Suite prüfen warmen/kalt/neues-Kit-Fallback, nicht automatisch jede denkbare dynamische Chainmutation mitten in einem großen Block. Solche zusätzlichen Hoststresstests bleiben in der Matrix offen.

### 5.4 VST3, Projektzustand, Synchronisation und Lifecycle

Factory-/Controllerzuordnung, Instrumentenklassifizierung, MIDIinput, Float32-Sampleformat, feste Stereooutputs und `kInfiniteTail` sind im Adapter vorhanden. Float64 wird ausdrücklich abgelehnt, nicht irrtümlich als implementiert ausgegeben. 47/47 Validator und paketierte Hostfälle liefern dafür SDK-Nachweis innerhalb ihres Umfangs.

Aktueller Kitcontainer ist **DM16** mit SampleEdit-Metadaten; ältere DM4–DM15 und frühe Gridformate werden über ihre jeweiligen Pfade migriert. DM15 ohne Edit bleibt Vollsample. Decoder/Stateprüfungen validieren Größen, Checksummen, Namen, Werte und abgeschnittene Streams vor dem Commit. Undo und Gesten werden nach erfolgreicher Controller-Vorbereitung invalidiert, nicht schon bei abgelehnten Recalls. `PreparedKitNames` schützt die Stringvorbereitung. Das neue Clear-Tupel FA01 verletzt anschließend die ansonsten vorausgesetzte Kitinvariante.

Voll-Kit-Recall bereitet Assets vor und benutzt `KitRestoreGate`: Kontrolle darf auf laufendes Audio warten, **Audio wartet nicht auf den Kontrollmutex**, sondern gibt in der Gatephase Stille aus. Release/Acquire und Revisionswechsel verbinden veröffentlichte Daten und Cacheinvalidierung. T04 beweist alte/neue kohärente Generationen im benannten Versuch. Die 21.875 stillen Gateblöcke sind absichtlich dokumentiert und dürfen nicht als „unterbrechungsfrei“ ausgegeben werden. Einzelne Samplepublikation erhält alte Stimmen über Owner-Lifetime; ein kompletter Recall beendet alte normale Voices an der nächsten gültigen Revision.

Für Patternautomation schützt ein Generations-/CAS-Vertrag neue Hostereignisse gegenüber Restore, einschließlich Equal/ABA und Zero-Frame. Controller berücksichtigt spätere Hostrückmeldung auch vor Componentstate. Die aktuellen O01/RE07-Tests bestehen. Der separate Audition-Publish FA03 hat diesen Vertrag nicht geerbt.

Host-Dirty wird nach vollständigem Controllercommit gemeldet; RE06 bestätigt den zuvor fehlerhaften Import-/reentranten Savepfad. `IMessage`-Queries schreiben derzeit synchron Attribute der Anfrage als Antwort. Das funktioniert mit dem verwendeten direkt verbundenen SDK-Testhost. Ob ein entkoppelnder Host-/Bridge-Proxy Änderungen am eingehenden Objekt an den Sender zurückspiegelt, wird dadurch nicht universell belegt. Dies ist ein **Kompatibilitätsrisiko**, kein hier nachgewiesener Studio-One-Fehler; für universelle Unterstützung eine explizite Antwortnachricht beziehungsweise einen spezifizierten Queryvertrag erwägen.

### 5.5 GUI, Zoom, native Texteingabe und Zugänglichkeit

Die aktive Oberfläche ist ein Custom-`DrumatorView`, keine Sammlung automatisch verdrahteter UIDESC-Knobs. Leere `control-tags` der Ressource sind deshalb für diese Controls kein fehlendes Binding. Der native Namenseditor benutzt Tag 1001 mit eigener Generation und echtem Win32-Editorlebenszyklus. Numerische Mausgesten und Stepzellen laufen über die handgeschriebenen Hitbereiche; aktive Layoutquelle ist `UiCompactLayout`.

Userzoom **100/120/130/150 %** und Host-DPI werden getrennt kombiniert. `DrumatorEditor::setZoomChecked` vergleicht den realen Frame-Transform und korrigiert den Editorwert bei Ablehnung. Die Recalllogik lehnt mehrfache nötige Viewzoomänderungen ab, wenn kein atomarer Mehrviewvertrag möglich ist; sie setzt nicht stillschweigend einzelne Views um. Fensterhöhe/Rows verwenden Compact-Paging bei Hostablehnung. Der neue WAV-Editor verlangt bei zu geringer Höhe eine erfolgreiche Erweiterung auf logisch 620 Pixel und öffnet bei Ablehnung nicht. Bei einem leeren Sample wird vor der Resizeanfrage abgebrochen. Diese Quellpfade sind sinnvoll; **nicht jeder neue Modal-Resize-/DPI-Mischfall ist ausgeführt**.

Nach vier Rows erfolgt Paging; Rowzahl 1…16 und optionale Velocityhöhen bestimmen die Geometrie. Aktuelle native FA05/FA06 prüfen Menü-Gutters und alle 9 Fill-/16 Outputoptionen selbst bei minimaler Viewhöhe. SA01 prüft Invalidierung einer zweiten View. Die neue WAV-Oberfläche hat dennoch FA06/FA11–FA13, die diese früheren Tests nicht erreichen. Die folgenden Matrizen nennen jeweils Nachweis und Lücke statt „GUI alles PASS“.

## 6. Vollständige Funktions-/QA-Matrix

**Q** = Quellpfad geprüft, **W** = vorhandener Windows-Nachweis am geprüften SHA, **N** = neuer Test nur implementiert, **H** = echte Hostabnahme fehlt. Ein W gilt nur für die genannte Teilmenge.

| Bereich / verlangtes Szenario | Ergebnis | Konkrete Evidenz / notwendige Ergänzung |
|---|---|---|
| WAV PCM/Float, unterstützte Bitbreiten, Mono/Stereo | TEILWEISE Q/W | WavDecoderTests und tatsächliche Imports; GUI-Floatheadroom FA12, sehr kurze echte Modaldateien noch H. |
| FLAC, fehlerhafte/zu große Dateien | TEILWEISE Q/W | Begrenzter FLAC→WAV-Adapter, bisherige Importfehlerprüfung; breites beschädigtes FLAC-Korpus und Peakmemory unter realer DAW offen. |
| Originaldatenerhalt nach Trim/Fades/Gain | PASS für WE01-Statefall | WAVbytes bleiben separat; DM16-Recall geprüft. Stale-Entwurf FA02 und Clear FA01 zusätzlich FAIL. |
| START/END ziehen, Grenzen und Einframe | TEILWEISE Q/W | Modellgrenzen validiert, Unity-DSP getestet; tatsächliche Modalhitmatrix nicht W, Nicht-Unity-Rand FA09. |
| Fade-In/Out, keine Überlappung | TEILWEISE Q/W | SampleEdit.valid und Renderer-Unityrampen geprüft; fehlende sichtbare Resetgriffe FA11, kombinierte SRC-/Mausfälle offen. |
| Gain 0/100/400 % | TEILWEISE Q/W | Q begrenzt 0…4; Modell prüft 0, Grenze >4 und Unity-DSP/.5. Echte UI 0/100/400 plus PCM/Recall nicht komplett W. |
| WAV-ZOOM +/−, GANZES WAV, Scroll | TEILWEISE Q | Intervalle begrenzt, 1-Frame-Minimum; aktueller Schiebebalken vorhanden. Vollsamplequery je Bewegung, FA13 bei Fehlern; reale Last-/Pixelprüfung H. |
| AUTO START | TEILWEISE Q | Threshold auf 256 Envelopebins, kein samplegenauer Transientendetektor; bei hohem Zoom sichtbarer Bereich. Referenzfixtures mit Stille/Transienten noch nötig. |
| RESET, ABBRECHEN, ÜBERNEHMEN | TEILWEISE Q/W | WE01 APIcommit/State-Neutralität; tatsächliche Modalbuttons nicht vollständig W; FA01/FA02/FA13. |
| Preview bei Stop/Playing | PASS für WE01-APIfall | Getrenntes PCM bei beiden Kontexten und explizitem STOP W. Kein kompletter Preview-Gesamt-PASS. |
| Erneutes Preview, schnelle Wechsel | FAIL Q/N | FA03/FA04; FA23-02/03 noch nicht ausgeführt. |
| STOP, Close und Deaktivierung | TEILWEISE Q/W; FA10 FAIL | Explizites STOP und native Viewzyklen W; active-Audition-Deaktivierung nicht abgesichert. |
| Samplegain/Pitch/Stereo-Pan | PASS für vorhandene Voicefälle | SampleVoiceTests, tatsächliche PCM-Signaturen; Vorhörpolicy separat, Trim-SRC FA09. |
| 64 Stimmen, Choke und Kapazität | PASS für benannte Core-/SA02-Fälle | Freiplatz vor Choke, Blockteilung, reale Sequencer/MIDI-Chokepartition. 64+Audition-Last und DAW H. |
| Mute/Solo, versteckte Rows | PASS für benannte Policy-/Statefälle | Neue Voices berücksichtigen Masken; alte Tails behalten Launchmix. Reale Musikererwartung/Host H. |
| Mastergain/-Reset/-Glättung | TEILWEISE Q/W | Validierter 0…200%-Wert, MasterRamp, Präzisionsgesten; neue Editor-/Previewkombination H. |
| MAIN/AUX, frei zugeordnet, mehrere Rows pro Bus | PASS für T02/T29/Corefälle | Verschiedene Stereocodes, echte DLL, Vertauschungsnegativen. Studio-One-Aktivierung H. |
| Einzel-Samplewechsel während Audio | PASS für RE09/RM13/PACING-Fälle | Reale Imports, Owner-Lifetime und hörbar unterscheidbare neue Quelle; kein DAW-Dropoutfreibrief. |
| Komplettkit während Audio | PASS für T04-Konsistenz; Kontinuität OFFEN | Alte/neue vollständige Generationen; Gate erzeugt ausdrücklich Stille. Audition-Spezialzustände ergänzen. |
| 16/32, A–H, Tailerhalt | PASS für T01/RE01/RE03/RE11/P19 | Tatsächlicher Rücklesestate und deterministische Audiointerleavings, kalt/warm. |
| Copy/Paste/Duplicate/Clear/Undo | TEILWEISE Q/W | Alte sichere Ziele/Dauern geprüft; FA08 Undo-in-Geste FAIL. |
| Fill 2/4/8/All/Clear und Rhythmusoptionen | PASS für aktuelle FA06/V17-Fälle | 9 reale Menüaktionen, 1T-Tails geschützt; Mehrview-Fill-Cancel RE04 inzwischen PASS. |
| Velocity malen, Cancel, Peer-Views | TEILWEISE Q/W | Owner-/Generationsschutz in alten Fällen PASS; Undo-Spezialfall FA08. |
| Einmaliges Random Velocity | PASS für RANDOM/Velo-Fälle | Nur vorhandene Steps, Stateänderung statt permanente Randomaudiofunktion. |
| Song-/Chain-Modus und Paging | TEILWEISE Q/W | UX/S09/S10/RE03/RE11/FA03; sehr lange Live-Chain-/Tempo-/Loopkombination H. |
| MIDIpattern, >256 Events, Hosttie | PASS für T30/FA02/RE07 | Ereignisordnung, Offsets und Rückmeldung geprüft. Nicht alle MIDI/Hostkombinationen behauptet. |
| PPQ 0/4/beliebig, Locate, Stop/Play, Ratewechsel | TEILWEISE Q/W | T28, SOR03/SOR03Z, TORTURE, RE08/RE09; neuer Audition-Lifecyclefehler, echte DAW H. |
| Swing/Humanize, blockpartitioniertes Timing | TEILWEISE Q/W | Core-/Schedulergrenzen und Teilungsfälle; umfassende Referenzzeitlinie aller Tempo-/Loopgrenzen H. |
| VST3 Klassifizierung/Busse/Float32 | PASS Validator + Hostfälle | 47/47; Float64 bewusst nicht unterstützt. |
| Patternautomation/Flush/Restore/ABA | PASS für O01/FA02/RE07/T30 | Pattern ParamID 1000; andere GUIwerte nicht als Hostparameter vorhanden. |
| Dirty und reentrantes Save | PASS für RE06 | Vollständiges Streampaar beim Import; keine universelle beliebige Hostreentranzbehauptung. |
| DM16-/Altformat-Recall, beschädigter State | TEILWEISE Q/W | Migration/Checksum/Präfix/OOMtests; FA01 ungültiger nachträglicher Kitstate, FA07 Folgezyklen. |
| Native Name, Enter/Escape/Blur/Recall | PASS für Z03 | Echte HWND-/Generationstests; reale DAWfokusketten H. |
| GUIzoom 100/130/150 %, DPI, Resizeablehnung | TEILWEISE Q/W | RE02 vier Zooms; native/paketierte Ablehnungsfälle. Neuer WAV-Editor nicht umfassend in diesen Tests. |
| D&D, Filepicker, Ordnernavigation | TEILWEISE Q/W | Reale Controller-/Viewpfade und Pfadnegative; WAV-Modalsperre FA06 FAIL, reale Browser H. |
| Alle Labels/Status-/Klickflächen | TEILWEISE Q/W | Aktive Draw-/Inputmatrix unten; FA11–FA14 und fehlende Modalpixel verhindern Gesamt-PASS. |
| Keine Audioallokation/-locks/-frees | TEILWEISE Q/W | Normaler Audiopfad fest/vorbereitet; Testzähler in benannten Fällen. Gesamter Heap-/OS-/DAWpfad nicht vollständig getraced. |
| Audiofrist/Aussetzerfreiheit/Denormals | NICHT NACHGEWIESEN | Begrenzte F14/F15/RE10-Diagnostik; kein ASIO, keine universelle Worst-Case-Garantie. |

## 7. End-to-End-Matrix sämtlicher aktiver Kontrollfamilien

Die folgende Matrix erfasst die aktive Darstellung und Mauszuordnung, Controller-/Nachrichtenfunktion, hörbare beziehungsweise gespeicherte Wirkung und Testlage. Zeilen gelten jeweils für **alle angegebenen Indizes**, auch wenn nur vier Rows beziehungsweise 16 Songslots gleichzeitig sichtbar sind. Keine Controls werden allein aufgrund ihrer Beschriftung als verdrahtet angenommen. Alle Quellen liegen unter `src/`; `V` steht im Folgenden für `vst3/DrumatorView.cpp`, `C` für `vst3/DrumatorController.cpp`, `P` für `vst3/DrumatorProcessor.cpp`.

| Kontrolle / vollständiger Indexbereich | Zeichen-/Ereignispfad | Controller → Processor/DSP | State / Automation / konkrete QA |
|---|---|---|---|
| Patternbuttons A–H | V Headerdraw :522–559; Mouse :1213–1259 | `changePattern` → Param1000; Hosttimeline/selected → Scheduler | kit.selected und Hostparameter; RE01/RE07/T30 W, echte alle Zoomclicks H |
| 1T / 2T | V Header und direkte Länge | `togglePatternLength` → `PatternLength` → gemeinsame Patterngeneration | patternLengths, Tail bleibt; T01/P19/RE01/RE03 W |
| Patternmenü: Copy, Paste, Clear, Undo, VELO VARIIEREN, VELO 100, Duplicate | V Popup :775–781, :1149–1159 | C Patternfunktionen → `PatternReplace`/Patternlänge | Grid+Dauer, Clipboard/Undo transient; P18/P19 W, Undo FA08 |
| Song ein/aus und Songeditor öffnen | V Header; dedicated Overlay :696ff | C Songconfig → SongMailbox → Mapping im Scheduler | enabled/chain; Song-/UX-/Coldtests W |
| Global Swing | V global knob, drag/reset/precision | C `adjustSwing` → `Swing` → Scheduler | kit.swing, kein eigener Hostparameter; Core/native Teilfälle W |
| Global Humanize | derselbe globale Kontrollpfad | C `adjustHumanize` → `Humanize` → deterministische Offsets | kit.humanize; Core/State W, volle Hörmatrix H |
| Master | V global knob, drag/reset | C `adjustMasterVolume` → `MasterVolume` → MasterRamp alle Buses | kit.masterVolume; State/Präzisionsfälle W |
| GUIzoom 100/120/130/150 % | V Header-Zoommenü :767–774/:1140–1148, eigener Editortransform | C Zoompräferenz → checked Framezoom/Resize, kein Audiokommando | DZM3, RE02/Z02 W, WAV-/DPI-Mischmatrix H |
| Instrumentselect/Nummer Rows 0–15 | V Rowdraw :600ff, UiCompactLayout.select | lokale Row-/Velocityauswahl | kein DSP-/Hostparameter, Indexfeedback; native UI Teilfälle W |
| Instrumentname Rows 0–15 | V native CTextEdit/Tag1001, Enter/Escape/Blur | C rename → `RowName`; P validiert/übernimmt | displayNames unabhängig Dateiname; Z03/N10/N11 W |
| Samplefeld/Dateiauswahl Rows 0–15 | V sample Rect, `load`, CNewFileSelector; D&D anderer Eintritt | C loadWav → `LoadPcm16Wav` → PreparedSample/Exchange | WAV+Name+Herkunft; N12/FA04 W, FA02/FA06/FA07 |
| Samplemenü Rows 0–15: Laden, WAV EDITOR, Umbenennen, Entfernen | V Popup :760ff, :1150ff | Filepicker / openSampleEditor / editName / clearSample | gespeicherte Source; Clear FA01, Modal FA02ff |
| Sample `<` / `>` Rows 0–15 | UiCompactLayout.previous/next; V Rowinput | C stepSample → sortierte gültige WAV/FLAC → loadWav | neuer Sourcepfad; N12 W, Mehrzyklus FA07 |
| PLAY Rows 0–15 | UiCompactLayout.preview; V Rowinput | C previewSample → `PreviewRow` → normale Trigger | transient; benannte Audio-/Previewfälle W, nicht mit WAV-Audition verwechseln |
| FILL Rows 0–15, alle 9 Optionen | V Fillpopup, UiCompactLayout.popupTop | C fillRow (einschließlich Clear) → PatternReplace → reale Patternbank | Grid/Undo, versteckte Tails erhalten; V17/FA06/RE04 W |
| Mute und Solo je Row 0–15 | V M/S, aktive Farbe aus C | C toggleRowGate(row,false/true) → RowGate/Masken → neue Voice-Launches | kit.muted/solo; Policy/State W, Tails Launchsemantik |
| Output je Row 0–15, Ziele 0–15 | V Outputpopup zwei Spalten | C setRowOutput → `RowOutput` → Voice.outputBus | kit.output; 16 reale Menüziele FA06, T02/T29 W |
| Choke je Row 0–15, Gruppen 0–4 | V Chokepopup | C setRowChoke → `RowChoke` → capacity-then-choke | kit.choke; Voice/SA02 W |
| Volume je Row 0–15 | V Knobslot0 | C adjustRowMix(row,false,delta) → `RowMix`/AtomicRowMix → Launchgain | kit.gain, kein Hostparameter; SampleVoice/State/Precision W |
| Panorama je Row 0–15 | V Knobslot1 | C adjustRowMix(row,true,delta) → RowMix → Balance im Launch | kit.pan; Stereo-/Statefälle W |
| Pitch je Row 0–15 | V Knobslot2 | C adjustRowPitch → `RowPitch` → vorbereiteter SRC | kit.pitch; Ratios/Samplewechsel W, harte Echtzeit H |
| Random-Velocity-/VELO-Aktion je Row 0–15 | V vierter Rowkontrollplatz, aktuelle Buttonfunktion | C randomizeRowVelocity → PatternReplace; Legacyknob → adjustRandomVelocity/RowRandomVelocity | vorhandene Noten, gespeichertes Grid; RANDOM/VELO W |
| Stepbuttons je A–H × Rows0–15 × Steps0–31 | V Stepdraw/-hit via stepX622/19px | C toggle/setStepVelocity → `StepEdit` → AtomicPatternBank | Grid; Core/Actual-Controllerfälle W, kein behaupteter physischer 4.096-Zellen-Klicktest |
| Velocityexpand je Row 0–15 | V Toggle x1233…1266, variable Höhe | lokales velocityExpanded; Resize/Compactlayout | Anzeigezustand; aktuelle Compact-/Velocitytests, alle DPI-Capturepfade H |
| Velocitybars je Pattern/Row/Step | V onMouseDown/Moved/Up/Cancel :1493ff | C begin/endStroke, setStepVelocity mit Owner/Generation | Grid+Undo; Peer/Recallfälle W, Undo-in-Stroke FA08 |
| +ZEILE / −ZEILE | V Footer, RowResizeTransaction | C VisibleRows, hide-Sicherheitsprüfung; P sichtbare Maske | kit.visible; VisibleRows/nativecompact W |
| Rowseiten `<` / `>` und Bereichstext | V Footer :685–695 | lokale firstRow/compactPageRows, kein Audiokommando | Anzeige, keine verschobene Sourcezuordnung; Compact/native W |
| Songslots0–63, 16 pro Seite; Add-Slot | V Songboard, UiSongLayout | Auswahl/Insert via C Songconfig | chain; UX paged/compact W |
| Songseiten `<`/`>` | V Songboard | lokale songPage, Auswahl erhalten | lokale Anzeige; UX Songtests W |
| Song-Pattern A–H für gewählten Slot | V Songmodalbuttons | C chain entry pattern → Songconfig | gespeicherte chain; Song/S09/S10 W |
| Songlängen 8/16/32 je Slot | V Songmodal | C entry steps → Songconfig → SongRead mapping | chain; Validierung/Core/RE03/RE11 W |
| Songentry-/Countänderung, Hinzufügen/Entfernen, Schließen | V Songmodal, begrenzte Add/Removebereiche | C begrenzte Count-/Entryaktionen → SongMailbox | chain, Close lokal; Extreme Deltas S09/S10 W |
| WAV-Editor Titel/Frames-/Gain-/Markertexte | V :807–863 | Snapshot embeddedWave + SampleEdit | temporärer Draft; derzeit keine vollständige Modalpixelprüfung W; FA11–FA13 |
| WAV START / END | V :821–827, :1049–1075, :996–1006 | lokaler Draft; bei Commit SampleEditorCommit → PreparedSample.edit | DM16; Unity DSP W, tatsächliche Mausmatrix H, FA02/FA09 |
| WAV Fade-In / Fade-Out | V :829–843, :1055–1065, :1010ff | Draft, Summe ≤Auswahl; SampleEdit.amplitude im Renderer | DM16, Core/Unity W; unsichtbare Startgriffe FA11 |
| WAV Gainregler 0–400 % und Wertanzeige | V :867–876, :972–977, :1080ff | Draftgain; Preview/Commit als Floatbits validiert | DM16; API .5 W, echte 0/1/4-Matrix H; FA23-05 nutzt echten .5-Klick N |
| WAV ZOOM +, ZOOM −, GANZES WAV | V :864–866, :979–995 | Fensterintervall, erneuter Embedded-WAV-Query | nicht im Kit, kein Audioedit; Q, tatsächliche UIlast/Fehlerfeedback FA13 |
| WAV horizontaler Scrollbalken | V :846–854, :1022–1037, MouseDrag3 | neues sichtbares Intervall, erneuter Query | lokaler Viewzustand; Q, keine ausgeführte native Scrollmatrix |
| WAV AUTO START | V :877, :1090ff | Schwellenwert auf sichtbaren 256 Bins, setzt Draftstart | erst Commit speichert; Q, samplegenaue Erkennung nicht versprochen |
| WAV RESET | V :878, :1080ff | Draft auf volle Länge/Fades0/Gain1 | kein Sourceverlust; Modell W, resultierendes Clear FA01 |
| WAV VORHÖREN | V :879, :1111–1116 | C previewSampleEdit → EditorPreview → separate Engine → MAIN+Master | transient, nicht Dirty; WE01 W, FA03/FA04 FAIL |
| WAV STOP | V :880, :1117–1118 | C stopSampleEditPreview → EditorPreviewStop → Audioreset | transient; WE01 W, Publish/Stop-Reihenfolge ergänzen |
| WAV ABBRECHEN | V :881, :1119–1122 | PreviewStop; Draft schließen, resize | gespeicherter Edit unverändert; APIneutrales WE01 W, Modalclick H |
| WAV ÜBERNEHMEN | V :882, :1123–1128 | C commitSampleEdit → P SampleEditorCommit → Exchange+kit.edit | Dirty/DM16; WE01 API W, FA01/FA02/FA13 |
| Status-/Ablehnungszeile | V :691; Import-/Inputstatus | Ergebnis aus echten C/P-Aktionen | kein Stateparameter; normale Statusfälle, Modalverdeckung FA13 |

### 7.1 Geometrieprüfung 100/130/150 % und unterschiedliche Instrumentzahlen

Aktive logische Breite ist 1.280. Bei Userzoom 100/130/150 % und DPI 100 % entspricht dies 1.280/1.664/1.920 physischen Pixeln. Host-DPI multipliziert zusätzlich; die logischen Hitbereiche bleiben Viewkoordinaten. Eine bloße Multiplikation der Zeichenrechtecke beweist noch keine korrekte OS-Capture-/Dragkoordinate. Die bestehenden Z02/RE02-Tests prüfen reale Frames und Restore, aber nicht alle neuen WAV-Gesten.

| Situation | Quell-/Geometrieergebnis | Ausführungslage |
|---|---|---|
| Rowzahl 1,2,3,4; Velocity ein/aus | UiCompactLayout.header108/baseRow94/velocity30/footer54; Höhe passt sich an beziehungsweise Compactfallback | Vorhandene Compact-/Row-/Nativefälle W; keine lückenlose physische Klickkombinatorik |
| Rowzahl5…16 | Maximal4 Rows pro Seite; firstRow geht in Sample-/Step-/Namenzuordnung ein | Vorhandene Pagingfälle W; alle Indizes source-geprüft |
| Alle32 Steps | x622…1230; Breite19, Abstand zum Velocitytoggle x1233 | Sourcehit/-draw konsistent; 1T deaktiviert sicht-/bedienbar Steps16…31 ohne deren Daten zu löschen |
| Minimale Viewhöhe300, Popup zu hoch | popupTop begrenzt einschließlich Headerüberlagerung | FA06 alle9 Fill- und16 Outputoptionen W |
| WAVmodal bei kurzer normaler Rowansicht | fordert logisch620 Höhe an, öffnet bei Hostablehnung nicht | Q; tatsächliche alleZoom-/DPI-/Ablehnungsfolgen H |
| WAVmodal 100/130/150 % | Rechteck230…1050/100…605 bleibt innerhalb logisch1280×620 | Q, keine neue gerenderte 3-Zoom-WAVmatrix; FA11/FA12/FA13 skalieren mit |
| WAVmodal D&D | (250,128) ist abgedeckt und dennoch Sampleziel | FA06 Q, FA23-04 N |
| Mehrere Views mit unterschiedlicher Rowhöhe/Zoom | notifyViews invalidiert/recomputet zugängliche Seiten; atomar nicht mögliche Zoomrecalls werden abgelehnt | Native Mehrview-/SA01/Z02 W; WAVdrafts FA02 |

## 8. Tests: Aussagekraft, neue Regressionen und fehlende Negativkontrollen

### 8.1 Was die vorhandenen Tests tatsächlich abdecken

Die heutigen RE-/RM-Fälle sind deutlich stärker als der Stand der älteren Berichte. **RM01–RM13 sind im aktuellen Lauf individuell PASS**: Kontrollen bestehen zuerst, relevante Mutationen werden anschließend vom jeweiligen Orakel erkannt. Dazu zählen unterdrückter Zero-Frame-Stop, alte Länge, übersprungener Peercommit, bloßer Dateiname statt Assetidentität, verlorenes/doppeltes/um ein Sample verspätetes MIDI, ausgelassener Präfix, akzeptierter Import-No-op, aktiver Dropout und gezielte Deadline-Diagnostikinjektion. Der ursprüngliche VFA02 und die frische DZM3-Matrix wurden beibehalten.

TORTURE prüft inzwischen den Anfangsbereich statt nur einen Plateaupunkt; PACING kontrolliert akzeptierte tatsächliche Sampledaten; RE08 benutzt einen unabhängigen aperiodischen Stereo-PCM-Verlauf. Diese konkreten alten Blindstellen sind damit verbessert. Ein bestimmter getöteter Mutant beweist aber keine vollständige Mutationsabdeckung aller Sequencer-, Loop-, GUI- oder Audiofehler.

Der neue WAVtest **WE01** (`tests/integration/WindowsIntegration.cpp:283–355` am Produktcommit) prüft echten Controller/Processor, Embedded-Snapshot, Preview unter Stop/Playing, explizites STOP, Commit, DM16-Recall und drei normale native Attach-/Detachzyklen. **Er öffnet den WAV-Modaleditor nicht über dessen Menü.** Vor jeder Wiederholung wird STOP gesetzt. Daher kann er weder FA04, FA06 noch fehlende Handles/Modalstatus erkennen. Der Kommentar „CANCEL“ im Test entspricht einer direkten Stopmethode und Stateprüfung, nicht einem tatsächlich geklickten ABBRECHEN.

`SampleEditTests.cpp` prüft Modellgeometrie/Envelope und Grenzen. Der Teil „CANCEL preserves saved state“ vergleicht zwei lokale SampleEdit-Werte und ruft kein Controller-/View-Cancel auf. `SampleVoiceTests.cpp:550–575` prüft echten DSP mit Edit, aber ausschließlich Ratio1. `KitStateTests.cpp` prüft Codec/Migration, nicht die nachgelagerte ClearSample-Operation. Diese Tests sind sinnvoll, dürfen jedoch nicht als ausgeführte End-to-End-Beweise der fehlenden Fälle umbenannt werden.

Mehrere ältere `UiRowGeometry`-/`UiRowControls`-/`UiHeaderHitZones`-/`UiOverlayHitZones`-/`VelocityLane`-Tests prüfen weiterhin frühere Hilfsmodelle. Für die aktive Oberfläche sind `UiCompactLayout`, `UiSongLayout` und der tatsächliche View entscheidend. Nur ein Include eines alten Headers ist kein Nachweis, dass dessen getestete Rechtecke aktiv verwendet werden. Aktuelle native Precision-/Pixel-/Menu-/Compacttests ergänzen Teile davon, nicht automatisch die neue Modaloberfläche.

### 8.2 In diesem Audit hinzugefügte unabhängige Tests

Datei **`tests/integration/FinalAudit20261023.inl`**, eingebunden und registriert in `WindowsIntegration.cpp`. Keine bestehenden Assertions abgeschwächt oder alten Regressionen entfernt. Beobachtung des privaten Modalzustands erfolgt über die bereits ausschließlich für Testkopien bereitgestellte `IntegrationAccess`-Freundschnittstelle. Der einzige neue zeitliche Hook steht hinter dem tatsächlichen Audition-Adopt in der generierten Processor-Testkopie; Produktdateien werden nicht verändert.

| Registrierung | Zugehöriger Befund | Tatsächlicher Testpfad und Orakel | Status |
|---|---|---|---|
| FA23-01 | FA01 | Echter Editcommit/Clear, realer getState, Decode: leerer Sourcezustand muss serialisierbar sein | **NICHT KOMPILIERT / NICHT AUSGEFÜHRT** |
| FA23-02 | FA04 | Zweimal echte Preview ohne STOP; getrennte exakte L/R-PCMwerte dürfen sich nicht verdoppeln | dito |
| FA23-03 | FA03 | Audio nach tatsächlichem Adopt pausiert, UI publiziert Preview; spätestens nächster Block muss korrekte PCM enthalten | dito |
| FA23-04 | FA06 | Echter Menü-Open des Modalviews, reales Filepackage bei (250,128); Ablehnung und unveränderte eingebettete Bytes | dito |
| FA23-05 | FA02 | Echter Modal-Gainklick, Peerimport, alter Modalcommit; neues Sample muss seinen Resetedit behalten | dito |
| FA23-06 | FA07 | Wirkliche Controller-/Processorstreams, unpaariges Y vor vollständigem Controller-first X; explizite X-Herkunft bleibt | dito |
| FA23-07 | FA08 | Paste 32→16, aktive Velocitygeste, Undo; Steps und Dauer derselben Undo-Generation im echten Processorstate | dito |
| FA23-08 | FA09 | Wirklicher VoiceRenderer bei Ratio2, stille Auswahl mit außenliegendem Prefix-/Suffiximpuls | dito |
| FA23-09 | FA10 | Echte Preview, Lifecycle-De-/Reaktivierung ohne neues Setup; ohne neue Anforderung Stille | dito |
| FA23-10 | FA05 | Vorbereitete SDK-HostMessage, OOM beim tatsächlichen Notifyquery; kein Exceptionescape, definierter OOMcode, Source bleibt | dito |

Die neuen Tests spezifizieren das gewünschte sichere Verhalten und sollen beim fehlerhaften Produktstand rot werden. **Es wird weder ein bereits beobachteter FAIL noch ein erfolgreicher MSVC-Build behauptet.** Da kein Windowslauf autorisiert ausgelöst wurde, kann die Kompilierbarkeit zusätzlicher Teststellen erst im später gebündelten Windowslauf bestätigt werden. Die Tests verwenden keine ersetzten State-/Audioalgorithmen.

Lokal nachgewiesen: `git diff --check` und die CMake-Skriptgenerierung der tatsächlichen `qa-source` mit eindeutigen Ankern bestehen. Das ist eine Vorprüfung von Instrumentierungsankern, **keine C++-Syntax-/Link-/Laufzeitprüfung**. Ein grüner ursprünglicher Suite-Lauf wird nicht als Ergebnis dieser geänderten Suite wiederverwendet.

### 8.3 Vor einer erneuten Abnahme ergänzen

1. Neue zehn Fälle zunächst auf Windows kompilieren und gegen den hier auditierten Stand ausführen; rote Fälle mit unverändertem Orakel sichern. Danach gezielte Reparaturen getrennt, keine Anpassung des Orakels auf fehlerhaftes Produktverhalten.
2. Nach Reparatur kritische Gegenmutationen: beim Clear Editreset auslassen; Asset-/Editgeneration ignorieren; Start vor Adopt konsumieren; alte Auditionvoice behalten; OOMboundary entfernen. Jeder korrespondierende Test muss wieder scheitern. Das ist in diesem Audit **noch nicht ausgeführt**.
3. Modal-Draw-/Hit-Integration für FA11–FA13 mit echtem nativen View, nicht nur lokalen Zahlen oder `status_`-Vergleich. Gain 0/100/400, START/END/Fades, Scroll, Ganzansicht, AUTO START, RESET, echte Cancel-/Commitklicks und Close abdecken.
4. Kreuzmatrix Sourcewechsel/Kitwechsel/Editcommit mit 2 Views, gleicher/verschiedener Framelänge, abgelehntem Recall/Import/OOM, mehreren vorherigen Recallzyklen. Unveränderte WAVbytes, Metadaten, Undo und alte Gesten gemeinsam prüfen.
5. Preview generationiert mit STOP, Adopt, Close, Recall, Zero-Frame, outputlosem Block und Lifecycle; normale Pattern-/MIDI-PCM muss unverändert bleiben. Gleichzeitig normale64Voices plus Audition unter SRC-Last diagnostizieren.
6. Referenzfixtures für Trim-/SRC-Randsemantik, Fadezeiten und Gainextreme bei 1Frame/kurz/lang, Mono/Stereo, 44,1/48/96kHz und Pitchgrenzen. Mit ausgeschnittener unabhängiger Referenz vergleichen; bloße Nullen-/Plateauproben genügen nicht.
7. Echte Host-/Browser-D&D-, DPI-/Monitorwechsel- und Resizeablehnungsketten prüfen. Ein automatisierter Mausklick auf eine private Viewmethode ersetzt keine Explorer-/DAW-Weiterleitung.

## 9. Risiken, Performance und notwendige Hostpraxis

### 9.1 Synchrone Kontrollpfade und Spitzen-RAM

**R01 bleibt offen als Dropoutursache.** Dateizugriff und WAV-/FLAC-Decodierung liegen weiterhin synchron auf dem Controller-/Nachrichtenpfad. Der Controller validiert WAV und der Processor decodiert beim Vorbereiten erneut. Bei bearbeiteten Kits kann selbst `KitCodec::valid(...,false)` wegen der Editvalidierung erneut decodieren. Die Payloadbudgets begrenzen Input, nicht Anzahl gleichzeitiger Kopien.

WAV-Zoom/Scroll ruft wiederholt `sampleEditorSnapshot` auf. Jede Aktualisierung fordert das **gesamte** WAV als binäres Attribut an und decodiert es erneut, obwohl nur ein sichtbarer Bereich für 256 Bins gebraucht wird. Drag-Scroll kann diesen Pfad pro Mouse-moved-Ereignis auslösen. Das ist quellseitig ein Aufwand proportional zum Originalfile und zusätzliche Allokation/Kopie, kein kostenloser Viewportwechsel. Eine konkrete UIlatenz oder ein dadurch verursachter ASIO-Dropout ist hier **nicht gemessen**. Sinnvolle spätere Optimierung: generationsgebundener unveränderlicher Waveform-/PCMcache auf Kontrollseite, kleine Queryantworten, Ratelimit bei Bewegungen; kein Umbau im Audit.

### 9.2 Durchsatz ist keine harte Echtzeitgarantie

Aktuelle F14/F15 und RE09/RE10 sind brauchbare Lastdiagnostik. Die vollständig sichtbare F14-Stichprobe hat 0/180 Fristüberschreitungen; sie enthält Auslauf-/Leerlaufblöcke und umfasst einen geteilten CIrunner. Frühere Läufe hatten bei anderen Messbedingungen deutlich andere Werte. Daraus folgen weder eine statistische Obergrenze aller künftigen Callbacks noch ASIO-Aussetzerfreiheit.

Bei 64 Frames/48kHz beträgt die Frist **1,3333 ms**, bei 128 **2,6667 ms**, bei 256 **5,3333 ms**. Eine gemessene 256er-Zeit darf nicht ohne tatsächliche Messung auf eine 64er-Frist übertragen werden. Umgekehrt beweist ein begrenztes p99 keinen nie überschrittenen Maximalwert. Bei 32 Blöcken ist p99 nur das Stichprobenmaximum. Kernelthroughput, tatsächliche Processorcallbackdauer, Windows-Scheduler-Wake-Latenz und Host-/ASIO-Overruns sind unterschiedliche Messgrößen.

Für RE09/RE10 ist der individuelle PASSabschluss vorhanden; die abgeschnittenen Details erlauben keine vollständige quantitative Tabellenrekonstruktion. Nicht behauptet werden „alle Buffersizes ohne Deadline-Miss“, „keine Pagefaults“, „kein Lock jemals“, „keine Denormals“ oder „aussetzerfrei bei 128 Stimmen inklusive Preview“.

### 9.3 Konkreter Studio-One-/Studio-Pro-Abnahmeplan nach Reparatur

**Alle folgenden echten Praxistests sind in diesem Audit NICHT DURCHGEFÜHRT.** Sie erst an einem eindeutig bezeichneten, gebündelt Windows-geprüften Reparaturcommit ausführen; 47/47 Validator ist dafür Voraussetzung, nicht Ersatz.

| Praxisfall | Durchführung / Orakel | Aufzeichnen |
|---|---|---|
| Save-Sicherheit | Bearbeitetes WAV clearen; alle Rows/Patterns/Edits speichern, DAW komplett schließen, frisch laden | Kein Savefehler; alle verbleibenden Daten identisch, geleerte Row kanonisch leer |
| Quellen-/Mehrviewkonflikt | Zwei Pluginfenster; Edit offen, andere Source laden, anderen Edit committen, dann alter Commit/Cancel/Close | Keine stille Fremdänderung, sichtbare Konfliktmeldung, normale Pattern weiter |
| Preview-Reaktion | Stop und Playing; lange Datei, rasche Previewwiederholung, wechselnde Edits, STOP, Editorclose und Plugin-Deaktivierung | Einmalige richtige Source, keine Pegeladdition/verwaiste Wiedergabe, Hauptpattern unbeeinträchtigt |
| Modalbedienung | 100/130/150 %, 1/4/16 Rows, Velolane ein/aus, Monitor100/150/200 % DPI | Sichtbare Fadegriffe, jeder Button erreichbar, Trefferfläche=Darstellung, sichtbare Fehler |
| D&D | Explorer und echter DAWbrowser, WAV/FLAC, Unicodepfade, ungültige URLs; offene WAV-/Song-/Fill-/Outputmenüs | Gesperrte Ziele bleiben unverändert, eindeutige Rückmeldung, richtige Instrumentzeile |
| Edit-DSP | Sehr kurze/1Frame- und lange Mono/Stereo-Dateien, Trims, Fades, Gain0/100/400, 44,1/48/96kHz | Referenzaufnahme/Nullvergleich bei Unity, definierte SRC-Randsemantik, Original erhalten |
| Multi-Out | MAIN plus alle aktivierten Stems, zwei Rows auf einem Bus, Panextreme, deaktivierte Stems, save/reopen | Einmaliges Signal am richtigen Stereoausgang, keine vertauschten Kanäle/verdoppelten Sends |
| Transport | Start PPQ0/4/beliebig, Loopgrenze auch mitten im Block, Locate vor/zurück, Stop/Play ohne UIpause, Tempoautomation | Vollständige PCM-/Triggerzeitleiste, keine alten Tails, keine verlorenen/doppelten Hits |
| Recall/Automation | Beide Stateordnungen, Component-only zwischen vollständigen Recalls, Samplegleichname mit anderen Daten, Patternautomation Equal/ABA | Auswahl, Anzeige, hörbares Pattern, Sourcepfad und gespeicherter Zustand stimmen überein |
| Last/ASIO | Reale Zielhardware, 64/128/256/512 Samples, 64 normale Voices, hoher Pitch/SRC; gleichzeitig Importe, Scroll/Zoom, Preview, Loop/Transport | Callbackverteilung und Maxima, ASIO-Overruns, Dropoutmarker, RAM/UIlatenz; keine nur gemittelte CPUanzeige |

Praktisch nicht alle OS-/Hostpfade automatisierbar bedeutet nicht automatisch „Plugin fehlerhaft“. Ein fehlender Test wird als offen markiert; die oben konkret belegten Fehler müssen vor der Abnahme dennoch gezielt behoben werden.

## 10. Priorisierte Reparatur- und Nachweisreihenfolge

1. **FA01 – zuerst Speicherfähigkeit herstellen.** Kanonischer Clearcommit für Source+Edit; tatsächlicher State-/Fresh-Recall-Nachweis einschließlich abgelehnter Operationen. Kein V1-Testpaket mit diesem bekannten nicht speicherbaren Normalablauf zur Abnahme erklären.
2. **FA02–FA05 – Source-/Editgeneration, Auditionkommando und Exceptiongrenze.** Konflikte sichtbar ablehnen; Previewgeneration erst nach passender Adoption starten, vorhandene Audition definiert ersetzen, OOM sicher zurückgeben. Nicht durch UI-Buttondisable allein verdecken.
3. **FA06/FA10 – Modalität und Lifecycle vollständig machen.** Alle D&Dphasen sperren, quieszenten Auditionzustand bei Deaktivierung bereinigen, normale Transportpreview erlauben.
4. **FA07/FA08 – Folgezyklen und Undo-Tupel sichern.** Herkunft an tatsächliche Identität binden; Gesten vor Erfassen eines unveränderlichen Undo-Eintrags beenden. Alte RE02/RE04/RE05/RE06 nicht entfernen.
5. **FA09 – Trimvertrag und Referenz-DSP festlegen.** Isolierte Auswahl gegenüber ausgeschnittener Referenz absichern; keine Änderung der Originaldaten. Falls andere Randsemantik gewünscht, vor Freigabe eindeutig entscheiden und testen.
6. **FA11–FA14 – Musikerbedienung und Rückmeldung.** Sichtbare Rand-Fadegriffe, Waveformclip/Anzeigeheadroom, Modalfehlertext, zutreffende Anleitung. Echte Modaltests bei 100/130/150 % und DPI ergänzen.
7. **Alle neuen Gegenfälle auf Windows einmal gebündelt ausführen**, vorher Testkompilierbarkeit verifizieren; anschließend mit gezielten Reparaturmutationen die Orakelwirkung belegen. Ein zusätzlicher Lauf wurde in diesem Audit nicht ausgelöst. Keine vorhandenen roten Tests abschwächen.
8. **Danach echte Studio-One-/ASIO-Abnahme** nach Abschnitt 9 durchführen und Ergebnis an exakten Produktcommit/Pluginbinary binden. Keine Freigabe aus Offline-Durchsatz, bloßem Build oder Validator ableiten.

## 11. Auditablage und Unverändertheit des Produkts

Dieser Audit ergänzt nur diesen Bericht und unabhängige Tests unter `tests/integration/`. Produktquellen unter `src/`, `resource/`, Produkt-CMake, SDKpatches, Workflow, Version und bestehende Testorakel bleiben gegenüber 8fc58325fc5fd51fe76e2d6ea526ec9a08fa5289 unverändert. Für die gezielte Interleavingkontrolle wird nur eine generierte Testkopie instrumentiert, niemals die ausgelieferte VST3.

Die vorhandene Workflowkonfiguration `.github/workflows/core-ci.yml:3–13` reagiert auf Push ausschließlich nach `develop/v1.0.0` beziehungsweise `fix/studio-one-acceptance-20261022` mit den dort aufgeführten Pfaden oder auf expliziten manuellen Dispatch. **Der separate Auditbranch löst durch diesen Push keinen automatischen Windows-QA-Lauf aus.** Kein Workflowdispatch, kein Merge und keine Änderung auf main oder dem Abnahmebranch sind Bestandteil dieses Auftrags.

**CODEX AUDIT ABGESCHLOSSEN.**

**Befunde: 1 BLOCKER / 4 HIGH / 8 MEDIUM / 1 LOW.**

**V1 technisch freigabefähig: NEIN.** Die alten benannten Reparaturen sind größtenteils tatsächlich bestätigt; die neuen konkreten Save-, Draft-, Audition-, Ausnahme- und Bedienpfade sowie fehlende Hostabnahmen verhindern dennoch eine belastbare V1-Freigabe. Die endgültige grafische Gestaltung ist keine Reparatur dieser technischen Zustandsverträge.

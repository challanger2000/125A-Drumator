# 125A Drumator – unabhängige Nachprüfung Z01–Z03

**Prüfdatum:** 20.10.2026 · **Prüfer:** Codex · **Modus:** ausschließlich Audit und Bericht.

**Gesamturteil: OFFEN. Keine technische V1-Freigabe.** Die drei früher beanstandeten Implementierungsstellen wurden tatsächlich und sinnvoll korrigiert. Für keinen dieser drei alten Gegenfälle wird hier ein fortbestehender Produktfehler behauptet. Der weitergehende Auftrag – vollständige Fehlertransaktion im echten Controller/Host, angehängte Windows-Views und native Editorlebensdauer – ist durch die verfügbaren Tests jedoch noch nicht ausreichend nachgewiesen. Unverändert bestehen konkrete Schwächen der Testorakel T01/T02 sowie die Nachweislücken T03/T04 und O01.

## 1. Verbindlicher Prüfstand und Bedeutung der Statusangaben

| Gegenstand | Tatsächlich verwendeter Stand |
|---|---|
| Repository und Entwicklungsbranch | `challanger2000/125A-Drumator`, `develop/v1.0.0` |
| Neuester Entwicklungs-HEAD bei Beginn und vor Berichtsveröffentlichung | `f6f1933980f8d8c688c20c295cacda5a00a3a848` |
| Reparierter Produkt- und Testcode | **`62f10a4edade31217c90a32eba3d4e39eefc54ee`** |
| Vorgänger des Reparaturdiffs | Dokumentationsstand `7740c6b890bd41fe865019f3aeabb9a17df9b302`, Produktbasis `d984188457882a4463b9263593d88f7bf1fffb71` |
| Produktbaum `src` in Reparaturcommit und aktuellem HEAD | identisch: `5dff4f865a0a3825d63064fdf508c88b29b65816` |
| Testbaum `tests` in Reparaturcommit und aktuellem HEAD | identisch: `c2e54536d77ee35977564bc99e87f54900f9c4d3` |
| Produktversionsangabe in CMake/Packaging | weiterhin `0.1.0`; dieser Audit ändert keine Version |
| Engineering-Regeln | `125A-Engineering/main` auf `83970e48b93b95250dceaa92dfe585266bca07b9` |
| SDK | Steinberg VST3 SDK 3.8.1, `v3.8.1_build_84`, Commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| Gepinntes VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| Neuer Bericht | `docs/audits/CODEX-RECHECK-Z01-Z03-2026-10-20.md` |
| Separater Auditbranch | `audit/codex-recheck-z01-z03-2026-10-20`, ab aktuellem Dokumentations-HEAD |

`f6f1933` ergänzt gegenüber `62f10a4` ausschließlich `AGENTS.md`, `README.md` und den Z01–Z03-Reparaturbericht. Es wurde somit kein älterer Produktstand anstelle des aktuellen Codes untersucht. Alle folgenden Produktzeilen beziehen sich auf den unveränderlichen vollständigen SHA `62f10a4edade31217c90a32eba3d4e39eefc54ee`.

Gelesen und angewandt wurden [START-HERE](https://github.com/challanger2000/125A-Engineering/blob/83970e48b93b95250dceaa92dfe585266bca07b9/START-HERE.md), README, Engineering-Guardrails einschließlich der neuen Z01–Z03-Regeln, die einschlägigen Standards zu AI-Engineering, Repositoryarbeit, Pluginverhalten, GUI/DSP und Echtzeit-/Release-QA sowie die projektspezifischen [AGENTS.md](https://github.com/challanger2000/125A-Drumator/blob/f6f1933980f8d8c688c20c295cacda5a00a3a848/AGENTS.md). Der ursprüngliche [RC01–RC04-Audit](https://github.com/challanger2000/125A-Drumator/blob/8c8babab94379b85a7d608a769d2bde8abc5854e/docs/audits/CODEX-RECHECK-RC01-RC04-2026-10-20.md) und der aktuelle [Reparaturbericht](https://github.com/challanger2000/125A-Drumator/blob/f6f1933980f8d8c688c20c295cacda5a00a3a848/docs/CODEX-Z01-Z03-REPAIR-2026-10-20.md) wurden mit der Implementierung abgeglichen.

**PASS** bedeutet einen ausreichend begründeten Nachweis für die jeweils ausdrücklich begrenzte Aussage. **FAIL** bezeichnet einen konkret nachweisbaren Fehler. **OFFEN** bedeutet fehlenden entscheidenden Nachweis; es ist kein behaupteter Produktfehler. Die Gesamtstatus in der nächsten Tabelle beziehen sich auf den vollständigen Prüfauftrag pro Z-Befund. Eine im Quellcode bestätigte Beseitigung der früheren Ursache wird zusätzlich ausdrücklich festgehalten und nicht wegen ausstehender Hostpraxis als weiterhin defekt bezeichnet.

## 2. Entscheidung pro Befund

| Befund | Abschlussstatus für den vollständigen Auftrag | Bereits bestätigt | Noch fehlender entscheidender Nachweis |
|---|---|---|---|
| **Z01 – transaktionaler Kit-Recall** | **OFFEN** | Die früheren Stringallokationen nach Grid-/Gestenänderung sind in eine vorgelagerte Vorbereitung verschoben. Der eigene Datencommit verwendet feste Daten und `noexcept`-Stringswaps. Auch Legacy-Defaultlabels werden vorbereitet. Die alte konkrete Ursache ist behoben. | DLL-/Controller-Fault-Injection mit realem State, Undo, Gesten, mehreren Views und nachgelagerter SDK-Benachrichtigung; Verhalten bei getrennter Prozessor-/Controllerübernahme. |
| **Z02 – Zoom-/Resize-Konsistenz** | **OFFEN** | Beide alten ungesicherten Rollbackpfade wurden entfernt. Mehr als eine zu ändernde View wird vor jeder Änderung abgelehnt. Es bleibt ein geprüfter Zoomaufruf. Das unsichere SDK-Zoommenü ist deaktiviert. Die alten konkreten Gegenfälle sind beseitigt. | Reale angehängte Frames, kontrollierte `IPlugFrame`-Ablehnungen, DPI, gespeicherter Zoom und Wiederöffnung. Mehrfach-View-Zoom ist bewusst eingeschränkt, nicht vollständig implementiert. |
| **Z03 – native Namenseditor-Lebensdauer** | **OFFEN** | Tokeninvalidierung geht nativen Callbacks voraus. `setFocusView(nullptr)` und erforderlichenfalls `looseFocus()` schließen den Plattformeditor; derselbe Schutz besteht beim Entfernen der View. Die frühere reine Sichtbarkeitsänderung ist ersetzt. | Tatsächliche HWND-/Fokus-/Tastaturprüfung unter Windows, mehrere Views, inaktiver Frame, Recall und verspätete Ereignisse. |
| **T01 – Erhalt Steps 17–32** | **FAIL des Testnachweises** | Der Test prüft für die Erhaltung weiterhin seine eigene Eingabefixture. | Aus dem Plugin zurückgelesener State bzw. hörbares Step-20-Ereignis nach echtem 1T→2T-Umschalten. |
| **T02 – vollständige AUX-Isolation** | **FAIL des Testnachweises** | Benachbarte Ausgangsvertauschungen können innerhalb der Toleranz bleiben. | Trennscharfe Ausgangs- und L/R-Orakel mit Negativkontrollen. |
| **T03 – tatsächlich überlappende Patternersetzung** | **OFFEN** | Kohärenz wird für erfolgreich gelesene Snapshots geprüft. | Eine Mindestzahl erzwungener überlappender Lese-/Schreibfälle ist nicht abgesichert. |
| **T04 – atomare Komplettkit-Übernahme** | **OFFEN** | Echte Prozessorkonkurrenz wird angestoßen, Rückgaben und endliche Audiodaten werden geprüft. | Verschiedene vollständige Kitgenerationen und blockweise Audio-/State-Orakel; keine reine Gleichkit-/Finite-Prüfung. |
| **O01 – Automation gegen Restore** | **OFFEN** | Der Busy-Zweig versieht Hostereignisse mit einer Generation. Die Z-Reparaturen ändern diesen Bereich nicht. | Deterministischer Nachweis des normalen/Zero-Frame-Interleavings und klarer Linearisation einschließlich Gleichwert/ABA. |

**Neue bestätigte Produktfehler aus dem Z01–Z03-Reparaturdiff: keine.** Das ist keine Bescheinigung vollständiger Fehlerfreiheit. Die nachfolgend beschriebenen SDK-/Host- und Mehrfach-View-Risiken bleiben entsprechend gekennzeichnet. Es wird kein weiterer Reparaturauftrag aus einer bloß denkbaren Ausnahme abgeleitet.

## 3. Vorgehen und tatsächliche Ausführungsnachweise

Untersucht wurden sämtliche acht gegenüber `7740c6b` geänderten Implementierungs-/Testdateien sowie deren echte Aufrufketten in Controller, View, Editor, Processor, Kitcodec, Undo, Editgeneration, SDK und Win32. Der Diff verändert weder Audioengine noch Stateformat, Multi-Out oder bestehende Testdateien. Zusätzlich wurden die unveränderten T01–T04-/O01-Stellen erneut gelesen.

Es gab **keine Produktcodeänderung, keine Teständerung, keinen Linux-Build, keinen neu gestarteten Actions-Lauf und keinen Studio-One-Test**. Es wurde kein Windows-Executable in dieser Linux-Umgebung ausgeführt. Temporäre, vom Checkout getrennte Python-Modelle dienten zur gezielten Simulation von Fehlerstellen und Zustandsfolgen. Sie sind keine Ausführung des VST3-Produkts.

| Nachweis | Ergebnis | Aussagegrenze |
|---|---|---|
| Reparaturdiff und Quell-/SDK-Pfadanalyse | durchgeführt | Belegt Implementierung und Reihenfolge, keinen nativen Lauf. |
| Symbolische Namens-Allokationsfehler | **240 Modellfälle ohne Änderung des alten modellierten Gesamtzustands** | Vier Pfadklassen: modern, DMR1, DMR2, DMR3; je 1/2/3 Views; modern 32, Legacy je 16 symbolische Namensfehlerstellen. Keine Zählung aller tatsächlichen Codec-/DLL-Allokationen. |
| Erfolgreicher Recall mit alten Folgeereignissen | **96 Modellfälle ohne Änderung des neuen modellierten Kits** | Vier Pfadklassen × drei Viewzahlen × Move/Up/Cancel/Close/Undo/Enter/Escape/Fokusverlust. Native Ereigniszustellung wird nicht ausgeführt. |
| Zoomvorgaben und Ablehnungen | **360 Modellfälle konsistent** | 0–3 Views, vorhandene Faktoren 1/1,5, Präferenz 1/1,5, Ziel 1/1,5, DPI 1/1,5/2 und Annahme/Ablehnung. Modell der Quellpolicy; kein `CFrame`-/Win32-/Hosttest. |
| Negativkontrollen der alten drei Fehler | alle drei alten Fehlermodelle verletzen das jeweilige Sollorakel | Belegt, dass die Modelle die früheren Gegenfälle unterscheiden; kein Ersatz für Produkt-Negativkontrollen. |
| Bestehender Windows-Lauf [#39347442164](https://github.com/challanger2000/125A-Drumator/actions/runs/39347442164) | öffentlicher Run und Windows-Job als erfolgreich bestätigt, vollständiger Commit passt | Kein neu ausgeführter unabhängiger Test. |
| 17/17 CTests, Validator 47/47, paketierte Smokes | im Reparaturbericht angegeben, erfolgreicher zugehöriger Run bestätigt | Vollständige Einzellogs verlangen auf der öffentlichen Jobseite Anmeldung; Einzelzahlen hier nicht unabhängig aus Volltextlogs nachgezählt. |
| Echter angehängter Windows-/Studio-One-Lauf | **nicht durchgeführt** | Kein nativer PASS. |

Die 17 CTests umfassen laut CMake **16 reguläre Einträge und einen erwarteten Negativtest** (`drumator_song_chain_negative_tests`, `WILL_FAIL`). Paketierter Editor-/Audio-Smoke und Validator sind zusätzliche Workflow-Schritte. Diese Kategorien werden nicht addiert oder als 17 native GUI-Integrationstests dargestellt.

Öffentlich bestätigt sind außerdem Job `118383345828`, Artefakt-ID `12231034358` und der von GitHub angezeigte Digest `sha256:f79be0f06b64fb4ad340a1ef139818c898325f710f8c2df75a5f74eab2d81424`. Das Artefakt wurde in dieser Nachprüfung nicht heruntergeladen oder ausgeführt; der angezeigte Digest ist keine eigene Binärprüfung.

## 4. Z01 – Transaktion und Ausnahmesicherheit

### 4.1 Alte Ursache ausdrücklich behoben

Im alten Audit folgte die allokierende Übernahme von `customNames_`/Labels erst auf die Invalidierung alter Edits und die Änderung des Grids. Eine spätere `bad_alloc`-Rückgabe konnte deshalb ein bereits verändertes Kit als abgelehnt melden.

Die aktuelle Reihenfolge in [Controller.cpp:129](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L129) lautet jetzt:

1. Lokale Kandidaten erzeugen, Eingabepuffer lesen und `KitCodec::decode` ausführen.
2. `stagedNames.prepare(candidate.displayNames, candidate.names)` **vor** jeder Liveänderung ausführen, Zeile 142.
3. Bei `bad_alloc` aus dieser Vorbereitung `kOutOfMemory` zurückgeben, Zeile 143. Codec-intern abgefangene Speicherfehler können als `false`/`kResultFalse` ankommen; auch das liegt vor der Liveänderung.
4. Erst danach Generation/alte Gesten invalidieren, Grid und feste Metadaten übernehmen und vorbereitete Strings swappen, Zeilen 146–166.
5. Native Fokusbereinigung und Views erst nach der Übernahme benachrichtigen, Zeilen 171–179.

[PreparedKitNames.h:14](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/core/PreparedKitNames.h#L14) kopiert die Namen in lokale Arrays. Die internen vorbereiteten Arrays werden erst nach allen Kopien geswappt. [Zeile 22](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/core/PreparedKitNames.h#L22) deklariert den Commit `noexcept` und prüft `std::is_nothrow_swappable_v<Names>` statisch. Live-Strings werden über `swap` übernommen. Die übrigen eigenen Commitdaten sind feste Arrays, skalare Werte und eine feste SongChain; `PatternUndo::reset` setzt Flags. Es findet dort keine erneute Namenskopie statt.

[Controller.cpp:52](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L52) und [View.cpp:329](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L329) invalidieren nur eigene Felder. Die Viewmethode ist jetzt `noexcept`; SDK-Sichtbarkeits-, Fokus- und Zeichenaufrufe stehen ausdrücklich erst in `finishTransientEditsAfterRecall`. Damit ist die vorher im Invalidierungspfad mögliche Plattformarbeit entfernt.

**Begrenzte Quellaussage: PASS.** Ein Fehler an den konkret verschobenen Stringkopien kann das alte Grid, Namen, Undo, Clipboard, Samplepfade und Editgeneration nicht mehr teilweise verändern. Der alte Z01-Gegenfall wird ausdrücklich geschlossen.

### 4.2 Legacy und abgelehnte Eingaben

[Controller.cpp:183](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L183) bereitet auch die Defaultlabels vor. `"LEER - WAV LADEN"` kann die MSVC-SSO-Grenze überschreiten; diese potenzielle Allokation steht jetzt vor der Tokeninvalidierung. DMR2/DMR3 werden erst vollständig in ein lokales Grid gelesen und validiert, bevor Zeile 200 Änderungen beginnt. DMR1 stellt anschließend das definierte Factorygrid her. Unbekannte Magics und ungültige/abgeschnittene Daten führen vor dem Commit zur Ablehnung.

Ein unnötiger Nebeneffekt ist lediglich die Vorbereitung von Legacylabels auch vor der späteren Ablehnung eines unbekannten Legacy-Magics. Sie verändert keinen Livezustand und begründet keinen Datenverlustbefund.

Bei abgelehnter Vorbereitung bleiben die alten Stroke-/Rename-Tokens absichtlich gültig. Ein anschließender Mauszug oder Undo soll weiter das **alte** Kit bearbeiten dürfen. Nach erfolgreichem Recall dagegen werden Tokens und Undo invalidiert. Diese beiden Sollzustände dürfen in einem Test nicht verwechselt werden.

### 4.3 Was der neue echte Allokationstest beweist

[RecallTransactionTests.cpp:14](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/RecallTransactionTests.cpp#L14) ersetzt tatsächlich `operator new` und wirft an der konfigurierten Stelle. Das ist eine deutliche Verbesserung gegenüber einem bloßen manuell geworfenen Hilfsmodell. Die 32 Fehlerpositionen in Zeilen 38–47 entsprechen den erwarteten 16 Customnamen- und 16 Samplelabelkopien dieser Fixture. Zeilen 49–57 erzwingen OOM während des anschließenden `PreparedKitNames::commit` und prüfen alle übernommenen Strings.

Die Grenzen sind jedoch erheblich:

- Aufgerufen wird ausschließlich `PreparedKitNames::prepare/commit`, nicht `Controller::setComponentState`.
- Das separat angelegte `KitEditGeneration`-Objekt wird der getesteten Klasse gar nicht übergeben. Die unveränderte Generation bei Fehler belegt daher keine richtige Position der Invalidierung im Controller.
- Die Fehlerzweige kontrollieren nur `liveCustom[0][0]` und `liveLabels[15][0]`. Ganze Arrays, Grid, Parameter, Samplepfade, Undo, Clipboard und Viewtokens werden dabei nicht beobachtet.
- 100 Byte lange Customnamen erzwingen Allokationen, überschreiten aber die echte DM14/DM15-Labelgrenze von 64 Byte. Für die generische Klasse ist das zulässig; eine gültige Produktfixture mit etwa 40 Byte muss zusätzlich eingesetzt werden.
- Der Defaultlabel-Positivtest in Zeilen 58–63 injiziert keinen Fehler in die Legacyvorbereitung.
- Ein eigenes Test-EXE-`operator new` überwacht nicht automatisch die Allokationen eines separat geladenen VST3-Moduls. Die Fault-Injection muss nachweislich den tatsächlich ausgeführten Controller erreichen.

**Erforderlicher Nachweis, Priorität HOCH:** Windows-Integrationstest der realen Controllerinstanz mit gültigem DM15 und repräsentativen DMR1/2/3- sowie DMR4–DM14-Migrationsfixtures. Jede erreichte Allokation bis zum Commit gezielt scheitern lassen. Vorher/Nachher den vollständigen fachlichen Zustand vergleichen und anschließend beweisen, dass bestehende alte Gesten und Undo bei echter Ablehnung noch funktionieren. Erfolgreicher Recall muss dagegen alte Folgeereignisse in allen Views blockieren. Die alte Produktversion muss als Negativkontrolle an der früheren Fehlerstelle scheitern.

### 4.4 Benachrichtigungen und ganze VST3-Projekttransaktion bleiben offen

Die `bad_alloc`-Fänge um native Bereinigung und `notifyViews` melden einen bereits übernommenen Kitstate nicht mehr fälschlich als abgelehnt. Das ist korrekt. Es ist dennoch kein universeller Ausnahmebeweis für den gesamten VST3-Aufruf:

- [Controller.cpp:171](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L171) und Zeile 223 rufen nach dem Commit die SDK-Basismethode `EditController::setParamNormalized` außerhalb dieser Fänge auf.
- Im gepinnten SDK folgt daraus `Parameter::setNormalized → FObject::changed → UpdateHandler::triggerUpdates`. Bei registrierten Dependents kann der Updatehandler dynamische Benachrichtigungsdaten verwenden. Es ist daher unzulässig, allein wegen des `noexcept`-Stringswaps den **gesamten** Hostaufruf als garantiert nichtwerfend zu bezeichnen.
- Für den aktuellen selbst gezeichneten Patternbereich wurde kein konkreter normaler Host-/GUI-Pfad nachgewiesen, der an dieser Stelle eine solche Ausnahme erzeugt. Die UIDESC besitzt keine Pattern-CControl-Bindung. **Dies ist eine gezielt zu testende Grenze, kein neuer bestätigter Produktfehler.**
- Controller und Processor werden über getrennte VST3-Aufrufe geladen. Ein erfolgreicher `Processor::setState` und ein anschließend wegen OOM abgelehntes `Controller::setComponentState` sind keine gemeinsame C++-Transaktion. Der neue Helfer stellt keinen hostweiten Rollback bereit. Wie der Host einen Fehler behandelt und GUI, hörbare Auswahl und gespeicherten Zustand wieder zusammenführt, muss getestet werden.

Im [Processor.cpp:771](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorProcessor.cpp#L771) werden alle 16 neuen Samples vor der Exchange-Publikation vorbereitet. [Zeile 870](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorProcessor.cpp#L870) serialisiert die Kontrollseite und sperrt den Restore gegen reguläre Audioadoption. Dieser unveränderte Weg wurde mitgelesen; der neue Test führt ihn nicht aus. Die Z-Reparatur rechtfertigt deshalb keine neue pauschale Behauptung atomarer Host-/Audio-/GUI-Übernahme.

**Empfehlung:** Den bereits guten Prepare→Commit→Notify-Aufbau beibehalten. Zunächst die echten Fehlerpfade messen und einen expliziten Postcommit-/Hostfehlervertrag testen. Keine weitere spekulative Architekturänderung aus diesem Audit ableiten. **Z01 insgesamt: OFFEN**, konkrete alte Stringcopy-Ursache behoben.

## 5. Z02 – Zoom, Hostablehnung, DPI und mehrere Views

### 5.1 Beide alten Rollbackgegenfälle sind im Code beseitigt

[Controller.cpp:84](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L84) liest den DZM1-Zoomstate vollständig, zählt tatsächliche Viewabweichungen und prüft **vor** einer Änderung [ZoomRecallPolicy.h:6](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/core/ZoomRecallPolicy.h#L6). Ab zwei erforderlichen Viewwechseln wird `kResultFalse` zurückgegeben. Die vorherige Schleife „View A ändern, View B scheitert, Rückrollback A ungeprüft“ existiert nicht mehr.

[View.cpp:359](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L359) macht höchstens einen `setZoomChecked`-Aufruf. Der vormals anschließende zweite Höhenresize und sein ungesicherter Rückrollback sind entfernt. `zoomMatches` kontrolliert Editorcache **und** tatsächliche Frame-Transformation gegen `effectiveScale()`. Ein bereits gleicher Cache bei abweichendem Frame wird abgelehnt, statt einen Erfolg zu fingieren.

[DrumatorEditor.h:41](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorEditor.h#L41) setzt bei erkannter Resizeablehnung den vom SDK vorzeitig geänderten Zoomcache zurück. Der gepinnte SDK-Pfad lautet:

`setZoomFactor → CFrame::setZoom → CFrame::setSize → VST3Editor::beforeSizeChange → requestResize → IPlugFrame::resizeView`.

Im SDK verändert `CFrame::setSize` bei einem gewöhnlichen negativen `beforeSizeChange` noch nicht die Viewgröße. `CFrame::setZoom` restauriert dann die alte Transformation. Für genau diesen regulären Ablehnungspfad passt die zusätzliche Cachekorrektur des Drumators dazu. Die alten zwei Gegenbeispiele werden nicht als fortbestehende Bugs weitergeführt.

**Begrenzte Quell-/Policyaussage: PASS.** Ein vollständiger angehängter Windows-PASS folgt daraus nicht.

### 5.2 Gespeicherter Zoom und Mehrfach-View-Vertrag

- Keine registrierte View: gültiger State setzt nur `preferredZoom_`; `createView` übernimmt ihn später über `setZoomChecked`, [Controller.cpp:235](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L235).
- Alle Views entsprechen dem Ziel: Präferenz kann ohne Hostresize übernommen werden.
- Genau eine View weicht ab: nur diese wird geändert; Präferenz erst nach Erfolg.
- Mindestens zwei Views weichen ab: vollständige Ablehnung vor Änderung. Beispiel: zwei Views auf 100 %, gespeicherter State 150 % → beide bleiben 100 %, Präferenz bleibt alt, Rückgabe false.
- Ein einzelner Headerbutton verändert weiterhin seine eigene View und merkt den Faktor als Präferenz. Er synchronisiert nicht automatisch alle anderen Views. Das kann unterschiedliche aktuelle Zooms ergeben und ist vom bewusst abgelehnten simultanen Statewechsel zu unterscheiden.
- Das geerbte SDK-Kontextmenü bleibt mittels `setAllowedZoomFactors({})` deaktiviert, Controller Zeile 240. Die sichtbaren 100/150-Buttons bleiben im Viewpfad Zeilen 860–872 aktiv.

Die Ablehnung mehrerer notwendiger Fensteränderungen ist eine **bewusste funktionale Einschränkung** zur Vermeidung des vorherigen Teilwechsels. Dieser Audit deklariert sie nicht als überraschende Regression. Sie muss im Hostpfad jedoch als wirkliche Ablehnung behandelt werden; die Aussage „Mehrfach-View-Zoomrecall vollständig unterstützt“ wäre falsch.

### 5.3 DPI und verbleibende Hostnachweise

[DrumatorEditor.h:13](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorEditor.h#L13) verwirft ungültige DPI-Faktoren. Angeheftet kombiniert es Host-DPI und Benutzerzoom, prüft `CFrame::setZoom` und stellt bei false den alten DPI-Cache zurück. Vor Attach passt es das gemeldete Rechteck an. Die Zoomprüfung vergleicht gegen die kombinierte effektive Skalierung; Benutzerzoom 150 % bei DPI 150 % muss somit eine Transformation von 2,25 liefern.

Der vorhandene [EditorGeometrySmoke.cpp:47](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/EditorGeometrySmoke.cpp#L47) erstellt den echten Editor, prüft `getSize` und Pre-Attach-DPI. Er ruft **kein** `attached`, kein `setFrame`, keinen kontrollierten Resizehost und keine Zoom-State-Wiederherstellung auf. Auch die drei `zoomRecallIsAtomic`-Assertions im neuen Test üben keine Viewtransformation aus.

**Erforderlicher Windows-Integrationstest, Priorität MITTEL:** Echte Controller-/Editorinstanz mit HWND und skriptbarem `IPlugFrame`. Vorher/Nachher jeweils Preference über `getState`, Editorrechteck, native Clientgröße, Frame-Transformation und tatsächliche Klickkoordinaten vergleichen. Matrix: 100/150 %, Host-DPI 100/150/200 %, 0/1/2 Views, gleiche/verschiedene Ausgangszooms, gültiger/defekter State, Host akzeptiert/lehnt ab, View schließen/wiederöffnen und Projekt neu laden. Bei absichtlicher Mehrfach-View-Ablehnung muss **jede** Beobachtung unverändert bleiben.

Zusätzlich Plattformfehler nach positivem Hostresize und reentrante Hostantworten separat testen. Die Analyse des gewöhnlichen negativen Resize-Returns ist kein Beweis, dass jede Win32-/SDK-Ausnahme atomar abgefangen wird. Ein konkreter neuer Plattformfehler wurde hier nicht ausgeführt oder behauptet.

**Z02 insgesamt: OFFEN.** Alte unkontrollierte Rückrollungen behoben; echte Host-/DPI-/Wiederöffnungsabnahme fehlt.

## 6. Z03 – Tatsächliche native Editorlebensdauer

### 6.1 Vollständige relevante Aufrufkette

Die frühere reine `setVisible(false)`-Änderung konnte ein Win32-EDIT-HWND mit Fokus weiterleben lassen. Die jetzige Lösung wurde bis zur gepinnten Plattformimplementierung verfolgt:

1. [View.cpp:295](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L295): `editName` setzt Zeile und Generation, positioniert den CTextEdit und ruft `takeFocus` auf.
2. SDK `CTextEdit::takeFocus` erzeugt den Plattformeditor und verknüpft den Framefokus.
3. SDK `Win32TextEdit` erzeugt über `CreateEditControl` ein sichtbares Child-HWND und setzt den nativen Fokus.
4. Erfolgreicher Kitcommit invalidiert zuerst alle Viewtokens über `discardTransientEditsForRecall`.
5. [View.cpp:338](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L338) beendet anschließend den Framefokus über `setFocusView(nullptr)`. Falls ein Plattformeditor ohne passenden aktiven Framefokus übrig ist, wird ausdrücklich `looseFocus()` aufgerufen.
6. SDK `CTextEdit::looseFocus` verschiebt die Plattformreferenz, übernimmt gegebenenfalls den letzten Text, gibt die Plattformreferenz frei und meldet Fokusverlust.
7. SDK `Win32TextEdit::~Win32TextEdit` restauriert den WndProc und ruft `DestroyWindow(platformControl)` auf.

Maßgebliche SDK-Dateien am gepinnten VSTGUI-Commit: [ctextedit.cpp:378](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/controls/ctextedit.cpp#L378), [cframe.cpp:1194](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L1194), [win32textedit.cpp:115](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32textedit.cpp#L115). Es wurden die tatsächlichen SDK-Dateien gelesen, nicht eine angenommene API.

Der zusätzliche `getPlatformTextEdit`-/`looseFocus`-Fallback ist relevant: `CFrame::setFocusView` kann bei inaktivem Frame nur den vorgemerkten Fokus ändern. Allein dieser Aufruf wäre dann kein hinreichender Beweis, dass ein bestehender Plattformeditor geschlossen ist.

### 6.2 Späte Textübernahme und Entfernen der View

Beim Fokusverlust kann das SDK noch `updateText` und `valueChanged` auslösen. Der [Viewguard in Zeile 312](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L312) verwirft das Ereignis, weil `editingNameRow_ == kRows` und die Generation bereits ungültig ist. Die Invalidierung steht **vor** jedem nativen Fokusaufruf, nicht erst dahinter.

[View.cpp:279](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorView.cpp#L279) setzt auch beim Entfernen erst den Renamezustand ungültig, beendet dann Frame-/Plattformfokus, entfernt den Listener und die Textview. Damit wird das zuvor fehlende Lifecycleende tatsächlich implementiert. Bei einem abgelehnten Recall wird dieser Vorgang gar nicht begonnen: alter Name, aktiver Editor und gültiger Token bleiben erhalten.

**Begrenzte Quellaussage: PASS.** Die alte Ursache „logische View verborgen, natives HWND absichtlich nicht geschlossen“ besteht in diesen erfolgreichen Recall-/Remove-Pfaden nicht mehr. Eine weitere Beschädigung von Namen durch einen alten Renamecallback wurde nicht nachgewiesen.

### 6.3 Enter, Escape und echte Windows-Ereignisse

Im SDK laufen Return/Escape und `WM_KILLFOCUS` über `platformOnKeyboardEvent`, `platformLooseFocus` beziehungsweise die reguläre CTextEdit-Tastaturbehandlung. Enter beendet eine gültige aktuelle Eingabe; Escape soll den alten Text erhalten. Der Drumator muss nach einem erfolgreichen Kitwechsel beide alten Ereignisse verwerfen und danach einen frischen gültigen Namensedit wieder zulassen.

**Erforderlicher nativer Regressionstest, Priorität MITTEL:** Echten Editor an ein Win32-Parent-HWND anhängen und das tatsächlich erzeugte EDIT-HWND identifizieren. Typen, Fokus und Generation beobachten. Danach Kit X→Y (auch 16→4 Instrumente), Enter/Escape/Fokuswechsel, inaktiven Frame, Editor schließen, Wiederöffnung und zwei Views ausführen. Nach erfolgreichem Recall muss das alte HWND zerstört sein (`IsWindow` false), der native Fokus darf dort nicht bleiben, und kein alter Text darf per Callback im neuen Kit landen. Nach abgelehntem Recall muss der alte Editor weiter gültig bedienbar sein. Ein neuer Edit im neuen Kit muss normal gespeichert werden.

Callback-Injektionen dürfen keine C++-Methoden auf bereits zerstörten Objekten aufrufen: Das wäre eine ungültige Testfixture. Echte Nachrichten müssen über noch gültige HWND-/SDK-Wege oder klar definierte noch lebende Listener zugestellt werden. Bei OOM während der Postcommit-Fokusbereinigung außerdem prüfen, ob UI/Fokus wieder benutzbar sind; bloß abgefangene Ausnahme ersetzt diesen Nachweis nicht.

Keiner der vorhandenen CTests erzeugt dieses native EDIT-HWND. Der geometrische Smoke hängt die View nicht an. **Z03 insgesamt: OFFEN**, Implementierungsfehler behoben, native Lebensdauer noch nicht praktisch bestätigt.

## 7. Unveränderte Testbefunde, nach Priorität

### T04 – HOCH: Komplettkit-Konsistenz wird im Konkurrenztest nicht beobachtet – OFFEN

**Datei:** [tests/PackagedAudioHostSmoke.cpp:1544](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/PackagedAudioHostSmoke.cpp#L1544), insbesondere 1579–1588.

**Ursache:** Sechs Restores derselben gespeicherten Fixture werden neben einer echten Audioschleife ausgeführt. Das Ergebnisorakel kontrolliert Rückgaben, endliche Samples und mindestens einen Audioblock. Es unterscheidet weder alte/neue Kitgenerationen noch gemischte Samples, Patterns und Mixparameter. Stille während des Restoregates ist ebenfalls endlich und zulässig.

**Gegenbeispiel zum Beweisanspruch:** Ein hypothetisch immer still rendernder Prozessor mit erfolgreichen Rückgaben kann die abschließende Finite-/Blockzahlprüfung dieses Teiltests bestehen. Ebenso könnte eine falsche Generation unentdeckt bleiben, wenn beide geladenen Kits identisch sind. Das ist kein Nachweis, dass der aktuelle Prozessor tatsächlich still oder inkonsistent ist.

**Empfehlung/Regression:** Zwei hörbar und im State verschieden codierte Kits X/Y verwenden. Nach definierten Kontrollpunkten jeden freigegebenen Block anhand mehrerer Instrumente, Patternpositionen, Ausgänge und Parameter einer vollständigen Generation zuordnen. Erzwungene Pausen vor/nach Publikation; zulässige Gate-Stille getrennt zählen und begrenzen. Rückgelesenen State und Controlleranzeige ergänzen. Der Test muss an absichtlich gemischter Generation scheitern.

### T01 – MITTEL: Selbsterhaltende Eingabefixture statt Plugin-State – FAIL des Nachweises

**Datei:** [tests/PackagedAudioHostSmoke.cpp:933](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/PackagedAudioHostSmoke.cpp#L933), Vorbereitung Zeile 911.

**Ursache:** Der behauptete Erhalt der Schritte 17–32 wird durch `patternKit.grid[...20] == 95` geprüft. `patternKit` ist die lokale Eingabefixture; sie wird vom Plugin nicht zurückgeschrieben.

**Konkretes Gegenbeispiel:** Das Plugin könnte Step 20 intern auf 0 setzen, während der lokale Fixturewert 95 bleibt. Diese Assertion besteht trotzdem. Die davor gemessenen Bar-2-Audiosummen prüfen andere Timingfragen und ersetzen gerade dieses Erhaltungsorakel nicht.

**Empfehlung/Regression:** Nach realen PatternLength-Nachrichten 2T→1T→2T den tatsächlichen Komponentenstate speichern und dekodieren; Step 20 muss weiter 95 sein. Zusätzlich das Step-20-Audio im passenden Zeitfenster prüfen. Eine Negativkontrolle, die nur interne Steps 17–32 löscht, muss den Test rot machen. Der Audit behauptet keinen nachgewiesenen tatsächlichen Stepverlust im aktuellen Produkt.

### T02 – MITTEL: AUX-Toleranz kann Ausgangsvertauschung akzeptieren – FAIL des Nachweises

**Datei:** [tests/PackagedAudioHostSmoke.cpp:1485](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/PackagedAudioHostSmoke.cpp#L1485), Orakel [Zeile 1535](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/PackagedAudioHostSmoke.cpp#L1535).

**Ursache:** Benachbarte Testamplituden unterscheiden sich um 0,015, die zulässige absolute Abweichung beträgt 0,03. Die Fixture besitzt außerdem keine unabhängige L/R-Kennung.

**Konkretes Gegenbeispiel:** Zwei benachbarte AUX-Buffers vertauschen. Beide Pegelfehler betragen ungefähr 0,015 und bleiben kleiner als 0,03. Ein L/R-Tausch gleicher Testsignale ist ebenfalls unsichtbar. Die pauschale Meldung vollständiger Isolation ist damit stärker als das geprüfte Orakel.

**Empfehlung/Regression:** Eindeutige zeitliche/spektrale Kennungen pro Ausgang sowie verschiedene L/R-Werte verwenden oder jede Row einzeln mit engen, begründeten Pegelgrenzen messen. Zusätzlich kollidierende Rowzuweisungen, aktive/deaktivierte Ausgänge und MAIN-Doppelung separat prüfen. Absichtliche Nachbarbus-/Kanaltäusche müssen scheitern. Es wird keine tatsächliche aktuelle Routingvertauschung behauptet.

### T03 – MITTEL: Konkurrenzprüfung kann ohne beobachteten Überlappungsfall bestehen – OFFEN

**Datei:** [tests/AtomicPatternTests.cpp:160](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/AtomicPatternTests.cpp#L160), insbesondere Zeile 190.

**Ursache:** `verified` zählt erfolgreich geprüfte konkurrierende Snapshots, wird aber am Ende nur mit `(void)verified` verworfen. Wenn der Writer vor dem ersten Readerdurchlauf fertig ist oder alle konkurrierenden Snapshotversuche abgewiesen werden, können null überlappende Prüfungen genügen.

**Gegenbeispiel:** Writer vollständig ablaufen lassen, dann Reader starten. Die Endprüfung kann korrekt bestehen; der kritische Interleavingpfad wurde trotzdem nicht geprüft.

**Empfehlung/Regression:** Kontrollierte Barrieren und definierte Writerpausen; Mindestzahl tatsächlich überlappender erfolgreicher Snapshots erzwingen. Jeder Snapshot muss einer vollständigen X-/Y-Generation entsprechen; Scheduler darauf ausführen. Thread-Schedulingglück allein ist kein Testgate. Das ist eine Abdeckungslücke, kein nachgewiesener aktueller torn Pattern-Read.

## 8. O01 – Automation-/Restore-Arbitration bleibt offen

**Risikopriorität: HOCH. Kein neuer bestätigter Produktfehler.**

**Dateien:** [AtomicPatternSelection.h:13](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/core/AtomicPatternSelection.h#L13), [Processor.cpp:455](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorProcessor.cpp#L455), [Processor.cpp:871](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorProcessor.cpp#L871).

Der Busy-Zweig benutzt `hostSubmit` und erhöht die Generation bei jedem gültigen Hostpunkt. Der normale Zero-Frame-Pfad Zeile 497 und normale Auswahlübernahmen Zeilen 679/688 benutzen dagegen `store`, das die oberen Generationsbits erhält. Restore nimmt einen Snapshot vor Eintritt in das Kontrollgate und führt später `restoreIfUnchanged` aus.

Ein minimales Modell illustriert die Nachweislücke:

```python
snapshot = (9 << 3) | 2       # Generation 9, Pattern C
store = lambda s, p: (s & ~7) | p
host  = lambda s, p: (((s >> 3) + 1) << 3) | p
regular = store(store(snapshot, 5), 2)  # C -> F -> C
busy    = host(host(snapshot, 5), 2)
assert regular == snapshot             # ABA nicht als neues Ereignis erkennbar
assert busy != snapshot
```

Ein gleichwertiger normaler Hostpunkt kann ebenfalls keine neue Generation erzeugen. **Das Modell beweist nur die Eigenschaft des Atomworts.** Ob ein Hostpunkt vor dem zulässigen Restore-Linearisationspunkt liegt und daher korrekt vom Restore überholt werden darf oder nach dem gewünschten Vorrangvertrag erhalten bleiben müsste, wird dadurch nicht entschieden. Eine reale unzulässige Ereignisreihenfolge im gepackten Prozessor wurde in diesem Audit nicht ausgeführt. O01 wird deshalb nicht in einen unbelegten FAIL umbenannt.

**Erforderlicher Test:** Echter `Processor::process/setState`-Pfad mit kontrollierten Pausen vor Snapshot, zwischen Snapshot und Gate, nach Publikation und vor CAS; Zero-Frame und normale Audioblöcke; gleichwertige Werte, C→F→C, Zwischenwerte und mehrere Parameterpunkte. Erwartete hörbare Auswahl, Hostparameter, Controlleranzeige und gespeicherte Auswahl explizit nach dem dokumentierten Vorrangvertrag prüfen. Erst danach über eine gezielte Änderung an Hostgeneration oder Snapshotgrenze entscheiden.

Z01–Z03 ändern weder diese Atomik noch den Prozessor. Aus ihren Tests darf kein O01-PASS abgeleitet werden.

## 9. Zusätzliche Grenzen und Regressionen

**Mehrfach-View-Gestenbesitz:** Generationen stoppen alte Kitgesten. Der Controller besitzt dennoch nur einen globalen `strokePattern_`/`strokeBefore_`-Satz, [Controller.cpp:494](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/src/vst3/DrumatorController.cpp#L494). Der neue Helfertest beweist keinen per-View-Gestenbesitz bei überlappenden aktuellen Gesten. Eine gezielte Prüfung zweier gleichzeitiger Viewgesten mit Cancel/Close ist sinnvoll; ein neu verursachter Datenverlust wird daraus hier nicht behauptet.

**Postcommit-UI-Wiederherstellung:** Abgefangene `bad_alloc` während Fokusbereinigung oder Relayout lässt das neue Kit als übernommen gelten. Das verhindert die alte falsche Ablehnung. Die UI muss anschließend aber tatsächlich wieder brauchbar sein. Ein Test muss Fenster, Fokus und Geometrie beobachten und eine Erholung durch frischen Edit/erneutes Öffnen belegen.

**Echtzeitsicherheit:** Die neuen Stringvorbereitungen bleiben im Controller-/Hoststatepfad; der Reparaturdiff fügt keine Audio-Callback-Allokation ein. Das ist kein Langzeit-RT-Nachweis. Der unveränderte [SampleVoiceTests.cpp:324](https://github.com/challanger2000/125A-Drumator/blob/62f10a4edade31217c90a32eba3d4e39eefc54ee/tests/SampleVoiceTests.cpp#L324) gibt offline p95/p99/Max aus, hat aber als CI-Gate lediglich eine großzügige 20-Sekunden-Gesamtgrenze. 256 Samples bei 48 kHz ergeben 5,333 ms Blockbudget. Ein bestandener Durchsatztest beweist nicht, dass jede Callbackdeadline eingehalten wurde. Aus den nicht zugänglichen Volltextlogs werden hier keine Zahlen erfunden.

**Keine neu belegte Regression:** Der untersuchte Diff ändert keine Samplecodec-, MIDI-, Routing-, Format- oder DSP-Implementierung. Daraus folgt begrenzt, dass die Z-Reparatur dort keine direkten Codeänderungen einführt. Es folgt keine erneute Vollfreigabe aller unveränderten Produktfunktionen.

## 10. Reproduzierbarkeit der Auditmodelle und Grenzen der Simulation

Die eigene Simulation bildet jeweils ausschließlich die im Quellcode geprüfte Reihenfolge nach. Für die 240 Namensfehlerfälle wurde der gesamte modellierte alte Zustand tief kopiert: alle Patternzellen, Namen/Labels, Pfade, Auswahl, sichtbare Rows, Mixdaten, Längen, Undo, Clipboard, Stroke-/Generation und alle Viewtokens. Jeder symbolische Fehler wird ausschließlich während der lokalen Namensvorbereitung ausgelöst. Die entscheidende Struktur lautet:

```python
before = deepcopy(live)
try:
    prepared_names = []
    for site in range(number_of_name_copies):
        if site == fault_site:
            raise MemoryError(site)
        prepared_names.append(new_name)
    # Erst hier: Generation invalidieren, feste Daten übernehmen, Namen swappen.
    commit(live, prepared_names)
except MemoryError:
    assert live == before
```

Die 96 Erfolgsfälle invalidieren zuerst alle alten Viewtokens und modellieren dann alte Folgeereignisse. Die 360 Zoomfälle zählen zu ändernde Views vor jeder Änderung; mehr als eine erforderliche Änderung oder eine einzelne Hostablehnung muss Viewliste und Präferenz unangetastet lassen. Ziel-/DPI-Faktoren werden als Modellwerte verglichen; es werden keine tatsächlichen Pixel oder nativen Rechtecke gemessen.

Die Gegenmodelle der alten Version ändern vor dem Namensfehler bereits Grid/Generation, lassen nach Rückrollablehnung einen Faktor 1,5 zurück bzw. verstecken nur das logische Textfeld bei weiter lebendem HWND. Alle drei verletzen das entsprechende Sollorakel. Ergänzend wurden die konkreten T01-/T02-Orakelgegenbeispiele und das O01-ABA-Modell ausgeführt.

Diese Modelle helfen, widersprüchliche Statusaussagen und offensichtliche Reihenfolgefehler aufzudecken. Sie beweisen weder echte `std::string`-Allokationszählung noch Codecfehler, VST3-ABI-Ausnahmeverhalten, Win32-Nachrichtenreihenfolge, Threadraces, native Zerstörung oder tatsächliche Studio-One-Funktion. Insbesondere werden die **240 symbolischen Fälle nicht als 240 erfolgreiche Controller-Fault-Injection-Tests ausgewiesen**.

## 11. Konkrete notwendige Nacharbeiten und Studio-One-Abnahme

Die folgenden Schritte sind Empfehlungen. Dieser Audit hat sie nicht implementiert oder gestartet.

1. **HOCH – Z01-Integration:** Den realen Windows-Controller einschließlich gültiger Statefixtures, Undo/Gesten, Samplepfaden und aller Views mit gezielter Allokationsfehlerinjektion prüfen. Initialisierungsallokationen von den relevanten Recallstellen trennen. SDK-Benachrichtigungen und getrennte Prozessor-/Controllerfehler explizit einschließen. Belegbare alte Zustände müssen bei Ablehnung erhalten bleiben; erfolgreiche Übernahme darf durch Postcommit-UI-Arbeit nicht nachträglich als zurückgerollt gelten.
2. **HOCH – T04 und O01:** Verschiedene vollständige Kitgenerationen sowie deterministische Automation-/Restore-Interleavings im echten Prozessor testen. Stille im Gate und tatsächliche neue Audiogeneration unterscheiden. Erst nach einem konkreten verletzten Sollvertrag Produktcode korrigieren.
3. **MITTEL – T01/T02-Testorakel korrigieren:** Tatsächlichen Plugin-State statt Fixture lesen; Ausgangs-/Kanalzuordnung mit trennscharfen Signaturen prüfen. Bekannte falsche Implementierungen müssen die Tests nachweislich rot machen.
4. **MITTEL – T03 deterministisch machen:** Erzwungene Überlappung und nichtnull Zahl geprüfter konkurrierender Snapshots als Gate.
5. **MITTEL – Z02/Z03 native Windows-Suite:** Angehängte HWNDs, skriptbares `IPlugFrame`, Resizeablehnung, tatsächliche Frame-Transformation, Fokus und `DestroyWindow` beobachten. Reale Controller-/Viewmethoden aufrufen; zusätzliche isolierte Policytests reichen nicht.
6. **Studio One / Studio Pro, ausdrücklich noch nicht durchgeführt:** Exaktes Paket zu `62f10a4` installieren und SHA dokumentieren. Mit laufender Wiedergabe zwei deutlich verschiedene Kits laden; während Velocityziehen und Namenseingabe Recall, danach Move/Up/Cancel/Undo/Enter/Escape und Fokuswechsel ausführen. 16→4 Rows und ältere Projektfixtures einschließen. Nach abgelehntem Recall alte Eingabe fortsetzen. Projekt speichern, DAW schließen, neu öffnen und hörbare Samples, A–H-Pattern, Namen, Mixer und Zuordnung vergleichen.
7. **Zoompraxis:** Benutzerzoom 100/150 %, Windows-DPI 100/150/200 %, Monitorwechsel, Editor mehrfach öffnen/schließen und gespeicherte Projektpräferenzen prüfen. Hostablehnungen zusätzlich kontrolliert im Testhost erzeugen; die DAW muss sie nicht von sich aus zuverlässig provozieren. Bei mehreren Views den bewusst eingeschränkten Recallvertrag dokumentieren.
8. **Audio/Hostgate:** Reale Automation gegen Kitwechsel, Multi-Out/Main, Mute/Solo/Choke sowie paced CPU- und Callbackausreißer bei typischen Blockgrößen prüfen. Ein erfolgreiches offline Smoke-/Validator-Ergebnis ersetzt diese praktische Abnahme nicht.

**Liste derzeit nachgewiesen erforderlicher Produktreparaturen aus diesem Z-Diff: keine neue.** Nachgewiesen verbesserungsbedürftig sind insbesondere T01/T02; die übrigen genannten Punkte benötigen zunächst belastbare Integrationsnachweise. Es wäre fachlich falsch, die behobenen drei Ursachen wieder pauschal als defekt zu deklarieren oder vorsorglich neue Produktvarianten zu verlangen.

## 12. Abschließendes technisches Urteil

**Z01: OFFEN. Z02: OFFEN. Z03: OFFEN.** Jeweils ist die zuvor konkrete Implementierungsursache behoben; offen bleibt die im Auftrag verlangte vollständige Integration und Fehlerabnahme. **T01/T02: FAIL des Testnachweises; T03/T04/O01: OFFEN.** Ein neuer bestätigter Produktfehler in den drei Reparaturen wurde nicht gefunden.

**Ist eine belastbare funktionale V1-Freigabe jetzt möglich? NEIN.** Ausschlaggebend sind die fehlenden echten Controller-/Allocator- und nativen Windows-Nachweise, unzureichende kritische Testorakel sowie die offene Automation-/Restore-Abnahme. Ein vollständiges Studio-One-PASS liegt nicht vor. Die sinnvolle nächste Arbeit ist gezielte Verifikation dieser vorhandenen Reparaturen; aus diesem Bericht folgt kein Auftrag zu erneuten spekulativen Reparaturen oder zur endgültigen GUI-Implementierung.

Nur dieser Bericht wird auf dem separaten Auditbranch bereitgestellt. Produktdateien, vorhandene Tests, `develop/v1.0.0` und `main` bleiben unverändert; es werden keine Actions angestoßen.

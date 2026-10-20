# 125A Drumator – unabhängiger Abschlussaudit RC01–RC04

**Prüfdatum:** 20.10.2026, Europe/Berlin. **Urteil: keine funktionale V1-Freigabe.**

Die vier Reparaturen sind konkret im Code umgesetzt. Die früheren alten Pattern-/Rename-Callbacks werden nach einer **erfolgreichen** Stateübernahme nun an mehreren Stellen blockiert; das unsichere SDK-Zoommenü ist deaktiviert, die Dropdiagnose trennt Ziel und Pfad. Diese Verbesserungen werden anerkannt. Es bleiben jedoch drei konkret nachvollziehbare Fehlerpfade beziehungsweise unvollständige Abschlüsse: **Z01 HOCH**, **Z02 MITTEL**, **Z03 MITTEL**. Außerdem fehlen produktnahe Windows-GUI-/Fehlerpfadtests; T01–T04 und O01 sind nicht geschlossen.

| Ursprungsbefund | Ergebnis dieser Nachprüfung | Begründung |
|---|---|---|
| **RC01 – Velocity/Recall** | **FAIL im geforderten vollständigen Fehlervertrag** | Der ursprüngliche erfolgreiche-Recall-Gegenfall ist im Code abgesichert. Eine erst während der Zustandsübernahme auftretende Allokationsablehnung kann aber bereits Daten und Gesten verändert haben: Z01. Mehrfach-View-Gestenbesitz bleibt zusätzlich unvollständig verifiziert. |
| **RC02 – Zoom** | **FAIL für vollständigen Rollbackvertrag** | SDK-Menü tatsächlich deaktiviert; gleicher Präferenzwert prüft nun echte Viewzustände. Fehlgeschlagene Rückrollungen werden weiterhin ignoriert, tatsächlicher Zoom und gespeicherte Präferenz können auseinanderlaufen: Z02. |
| **RC03 – Namen** | **TEILWEISE** | Alte Rename-Callbacks werden nach erfolgreichem Recall durch Row-/Generationprüfung abgewehrt. Das Ausblenden beendet den nativen Windows-Texteditor und Fokus jedoch nicht: Z03. Ein weiterer Namensdatenverlust trotz dieser Guards wird hier nicht behauptet. |
| **RC04 – Dropdiagnose** | **PASS für die eingegrenzte Diagnose-/Ablehnungslogik** | Zielblockierung und ungeeigneter Pfad werden korrekt unterschieden; abgewiesene Ziele/Payloads erreichen keinen Sampleladeaufruf. Native Explorer-/Hostbrowser-/Zoomabnahme bleibt ausdrücklich nicht durchgeführt. |

**PASS bezeichnet ausschließlich den genannten nachgewiesenen Teilvertrag.** Ein Quellcodebeweis oder ein Pythonmodell ist keine neu ausgeführte Windows-/Studio-One-Prüfung. FAIL bezeichnet nachfolgend einen konkret beschriebenen fehlerhaften Kontrollfluss unter seiner angegebenen Bedingung; die jeweiligen nativen Reproduktionen werden getrennt gekennzeichnet. Risiken ohne gesicherten Produktgegenfall stehen unter OFFEN.

## 1. Geprüfte Identität und Quellen

| Gegenstand | Wert |
|---|---|
| Repository | `challanger2000/125A-Drumator` |
| Entwicklungsbranch | `develop/v1.0.0` |
| **Exakt geprüfter Produkt-/Testcommit** | **`d984188457882a4463b9263593d88f7bf1fffb71`** |
| Dokumentationsstand / Elternstand des neuen Auditbranches | `7740c6b890bd41fe865019f3aeabb9a17df9b302` |
| Vorheriger Produktstand | `dbd9106f7af48b23ad4874a1a4c74395baa14173`; davor liegender Dokumentationscommit `57b902b505bd9bf04110391af0313a7719ec6bda` hat identischen Produktcode |
| `src`-Tree von `d984188` und `7740c6b` | identisch: `cd7edffe523b65844a1404000280c10620f39d99` |
| `tests`-Tree von `d984188` und `7740c6b` | identisch: `63c46eca502616fd2b5a38f0e677f6d2fdb04932` |
| Plugin-Metadatenversion | `0.1.0`; keine unterstellte veröffentlichte FULL-V1 |
| Engineering-Regeln | `125A-Engineering`, `29bd83e9cfc1c7f46ac1a60a35a9341d310ba08b` |
| SDK | VST3 `v3.8.1_build_84`, `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| Zugehöriges VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| Vorhandener Windowslauf | `39341195970`, Windowsjob `118362886423` |
| Zielbranch | `audit/codex-recheck-rc01-rc04-2026-10-20` |
| Einzige neue Repositorydatei | `docs/audits/CODEX-RECHECK-RC01-RC04-2026-10-20.md` |

Gelesen und angewendet wurden `START-HERE.md`, die aktuellen Engineering-Guardrails einschließlich des neuen RC01–RC04-Eintrags, die in der vorherigen Prüfung bereits vollständig berücksichtigten unveränderten einschlägigen Standards/QA-Regeln sowie die projektspezifischen `AGENTS.md`. Die neuen Engineering-/Reparaturbehauptungen wurden als Prüfauftrag, nicht als unabhängiger PASS-Nachweis verwendet. Der Auftrag benennt ausdrücklich einen Entwicklungsstand, keine neue Releasebasis oder Besitzerfreigabe.

Der vollständige Reparaturdiff gegen den unmittelbar vorangehenden Dokumentationsstand betrifft **acht Implementierungs-/Test-/Builddateien**. Diese Dateien und ihre vollständigen Controller-/View-/SDK-Aufrufpfade wurden geprüft. Processor, Codec, bestehende Audiohost-/Atomic-Tests und die offenen T-/O-Befunde wurden zusätzlich gezielt abgeglichen. `7740c6b` ergänzt gegenüber `d984188` ausschließlich `AGENTS.md`, `README.md` und den neuen Reparaturbericht. Produkt- und Testbäume sind identisch. Sämtliche Produktzeilenlinks unten sind auf `d984188` festgeschrieben.

### 1.1 Tatsächlich ausgeführte Prüfungen und Grenzen

- Git-Identität, Branchstände, Produkt-/Testtree-Identität, vollständiger Reparaturdiff und Inhaltsprüfsummen wurden abgeglichen.
- Der neue CTest und die bestehenden von T01–T04/O01 betroffenen Prüfpunkte wurden am tatsächlichen Quellcode gelesen. SDK-Fokus-, Win32-Textedit-, Kontextmenü- und Resizeverträge wurden am exakt gepinnten SDK geprüft.
- Zusätzliche **Python-Quellguards und Zustands-/Fehlermodelle** wurden außerhalb der Repositoryarbeitskopie ausgeführt: 216 Kombinationen alter Callbackarten nach erfolgreicher Übernahme, drei Fehlerpfadmodelle und die vollständige Dropklassifikation. Diese Modelle bilden untersuchte Verzweigungen ab; sie sind **keine Ausführung des C++-Plugins**, kein Codec-, HWND- oder DAWtest. Die Modellkombinationen werden nicht als 216 voneinander unabhängige native Regressionen verkauft.
- Der vorhandene Run und Job wurden über ihre öffentlichen GitHubseiten gelesen. Vollständige Einzellogs sind dort anmeldepflichtig. Einzelne Testzahlen werden daher als gemeldete Ergebnisse mit unterstützendem Run-/Workflow-Nachweis bezeichnet, nicht als hier erneut gelesenes Gesamttranskript.
- Auf dieser Linuxumgebung wurden gemäß Windows-only-Regel **keine C++-Produkt-/Linuxbuilds, keine neuen Windowsbuilds und keine neuen Actions** ausgeführt. **Studio-One-/Studio-Pro-Praxistests: nicht durchgeführt.** Kein neues Bundle wurde installiert oder bitgenau auditiert.
- Ausschließlich der Bericht wird committet. Keine Änderungen an Code, vorhandenen Tests, Workflows, `develop/v1.0.0`, `main` oder finaler GUIgestaltung.

## 2. RC01 – erfolgreiche und abgelehnte Recalls, alte Gesten und mehrere Views

### 2.1 Der ursprüngliche erfolgreiche-Recall-Fehler ist gezielt korrigiert

[Controller:50](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L50) invalidiert zuerst Controller-Stroke und dessen Token, erhöht dann die gemeinsame Editgeneration und verwirft transienten Viewzustand in **allen registrierten Views**. Der Aufruf liegt jetzt:

- bei DM4–DM15 nach vollständiger erfolgreicher Dekodierung und vor dem ersten Gridwrite [Controller:142](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L142)–[Controller:149](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L149);
- bei DMR2/DMR3 nach vollständig validierten Gridbytes [Controller:181](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L181)–[Controller:186](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L186);
- bei DMR1 nach Header-/Auswahlprüfung und vor der Erzeugung des Factorygrids [Controller:187](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L187)–[Controller:194](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L194).

Der View-Reset [View:323](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L323) setzt `laneDragActive_=false`, Dragtoken0, EditingRow auf ungültig, Nametoken0 und schließt das Fillmenü. Er ruft **kein** `endVelocityStroke(false)` auf. Damit wird der alte Patternsnapshot beim Recall selbst nicht in das neue Kit zurückgeschrieben. Moderne und Legacypfade resetten außerdem die Undohistorie.

Späte MouseMove/Up/Cancel werden durch Aktivitäts- beziehungsweise Generationprüfungen abgewehrt [View:1049](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L1049), [View:1074](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L1074), [View:1082](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L1082). `removed()` sieht nach erfolgreichem Viewreset keinen aktiven alten Stroke mehr [View:279](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L279). Selbst ein späterer direkter Controller-Strokeabschluss findet den bereits ungültigen `strokePattern_`; zusätzlich prüft [Controller:483](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L483) die Generation. Es wurde kein fortbestehender unmittelbarer alter Snapshot→neues Kit-Pfad aus dem vorherigen RC01 allein nach einer normal erfolgreich beendeten Übernahme gefunden.

**Nachvollziehbarer positiver Ablauf:** KitX `[20,64]`, aktiver Stroke → erfolgreiches KitY `[90,115]` → alte Move/Up/Cancel/Close/Undo-Ereignisse. Neue Gridwerte bleiben `[90,115]`. In den zusätzlichen Ablaufmodellen wurden drei Statepfadfamilien, Rowzahlen1/4/16, eine bis drei Views und acht Arten alter Abschlüsse kombiniert:216 Quellflussmodelle. Sie setzen die erfolgreiche Stateübernahme voraus und testen keine echten Streams oder Windowscallbacks.

### 2.2 Ablehnung ist in zwei Klassen zu trennen

**Vor der Invalidierung:** beschädigter/trunkierter DM-Stream, ungültige Legacygridbytes, ungültige Auswahl/Rowzahl, nicht unterstützte Magics sowie fehlgeschlagene anfängliche Allokation/Dekodierung kehren vor `invalidateTransientEditsForRecall()` zurück. Die vorhandene gültige Geste wird auf diesen Pfaden nicht explizit invalidiert. Diese Verbesserung ist nachprüfbar.

**Nach Beginn der Übernahme:** Stringkopien und weitere Arbeit können noch nach der Invalidierung scheitern. Der moderne `catch(std::bad_alloc)` deckt auch diesen Bereich ab. Die pauschale Aussage „jeder abgelehnte Recall lässt laufende Edits und alten Zustand unangetastet“ ist deshalb nicht erfüllt. **Z01** beschreibt den konkreten Fehlerpfad. Der neue Generationstest behandelt diese Klasse überhaupt nicht.

### 2.3 Mehrere Views: nach Recall besser abgesichert, Gestenbesitz weiterhin offen

Alle zum Zeitpunkt des Aufrufs registrierten Views werden invalidiert. Eine neu angelegte View beginnt ohne aktiven Stroke und übernimmt für neue Edits die aktuelle Generation. Nach Recall kann deshalb auch eine alte View A nicht allein mit ihrem nun auf0 gesetzten Token einen neuen Stroke von View B derselben neuen Kitgeneration beenden. Das ist ein sinnvoller zusätzlicher Schutz.

Davon getrennt liegt weiterhin **ein einziger Stroke-Snapshot im Controller**, während jede View eigene Aktivitätsflags hat. Zwei überlappende oder verspätet abgeschlossene Gesten innerhalb derselben Kitgeneration sind durch eine Kitgeneration allein nicht eindeutig einem Besitzer zugeordnet. `beginVelocityStroke()` kann einen bisherigen Stroke abschließen; alte Viewflags werden dadurch nicht ebenfalls gelöscht. Ein späterer Abschluss einer noch als aktiv betrachteten View kann damit einen inzwischen anders begonnenen Controllerstroke adressieren.

Das ist ein **offener Mehrfach-View-/Ereignisordnungsfall**, hier kein als tatsächlich nativ reproduziert ausgegebener Datenverlust. Es muss mit realer Maus-Capture-/Fensterreihenfolge und kontrollierten verzögerten Callbacks geprüft werden. Windows liefert nicht beliebige konkurrierende Maustastensequenzen; ein künstliches Hilfsmodell allein genügt nicht als Nachweis einer normalen Studio-One-Reproduktion. Empfohlen wird ein expliziter Gestentoken/Besitzer zusätzlich zur Kitgeneration oder das explizite Beenden aller beteiligten Viewgesten bei Übernahme einer neuen Geste.

## 3. RC02 – Zoomwege, Präferenz und fehlgeschlagene Rollbacks

### 3.1 Konkret korrigierte Teile

`createView()` setzt nun `setAllowedZoomFactors({})` [Controller:213](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L213)–[Controller:219](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L219). Im gepinnten SDK wird das geerbte „UI Zoom“-Menü nur bei **nicht leerer** Liste angeboten [SDKEditor:859](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L859). Der ursprünglich beanstandete ungeprüfte Menüweg ist damit bewusst deaktiviert. Die zwei sichtbaren100-/150%-Tasten bleiben erhalten und benutzen `setZoomChecked()`; nur bei Erfolg wird die Präferenz gemerkt [View:843](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L843)–[View:850](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L850). Eine erneut erzeugte View bekommt die gespeicherte Präferenz.

`Controller::setState()` kehrt nicht mehr nur wegen gleicher Präferenz früh zurück. Es erfasst die tatsächlichen vorherigen Editorfaktoren und verwendet `zoomMatches()`, das auch die Frame-Transformation gegen den effektiven Userzoom×DPI-Faktor prüft [Controller:89](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L89)–[Controller:105](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L105), [View:333](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L333)–[View:340](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L340). Dadurch wird der frühere triviale „Präferenz100%, View150%, lade100%“ nicht mehr blind bestätigt.

Der Headerpfad behandelt normale Hostablehnung und der spezielle Editoroverride Host-DPI-Anfragen nachvollziehbar. Das ist jedoch kein Beleg, dass mehrere aufeinanderfolgende Hostanfragen oder jede Rückrollung erfolgreich sind.

### 3.2 Verbleibender Fehler

[Controller:101](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L101) ignoriert das Ergebnis der Rückrollung zuvor veränderter Views. [View:347](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L347) ignoriert das Ergebnis des lokalen Rückzoomens, nachdem das anschließende Höhenrelayout fehlgeschlagen ist. Außerdem werden dabei Compact-/Lanezustände verändert, ohne einen vollständigen alten Layoutsnapshot wiederherzustellen. **Z02** dokumentiert konkrete kontrollierte Hostantworten, bei denen die Methode zwar Fehlschlag meldet, ihre angenommene Rückkehr zum alten sichtbaren Zustand aber nicht erreicht.

Ein Plugin kann keinen Host zwingen, einen Resize zurück zu akzeptieren. Die notwendige Korrektur ist deshalb keine unzulässige erzwungene Fenstergröße, sondern ein definierter und geprüfter Umgang mit **teilweise ausgeführten** Zustandswechseln, inklusive tatsächlicher Faktoren, Persistenz, Diagnose und Wiederherstellungspolitik.

**Erforderliche Native-Matrix:** gleicher/anderer gespeicherter Zoom, eine/mehrere Views, Annahme des ersten Zooms und Ablehnung des nachfolgenden Layouts, Ablehnung einer bereits nötigen Rückrollung, DPI-/Monitorwechsel, Schließen/Reopen und anschließendes Speichern. Der vorhandene Geometriesmoke prüft nur Preattach-DPI und kein solches Hostprotokoll.

## 4. RC03 – Namen, Generationen und nativer Fokus

Der Beginn eines Namensedits speichert nun die Kitgeneration [View:297](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L297). `valueChanged()` verlangt dieselbe Control, eine gültige EditingRow und die aktuelle Generation [View:305](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L305). Beim erfolgreichen Recall werden Row und Token **vor** dem Ausblenden invalidiert. Alte Enter-/Fokus-/Commitcallbacks können dadurch nicht mehr einfach den ausstehenden Namen in das neue Kit schreiben. Der eigentliche Schutz ist auf zwei Bedingungen gestützt: ungültige Row und unpassende Generation. Ein Wechsel16→4 Rows ist für diesen Guard nicht anders als4→4.

Der Fokuslistener setzt die Flags zusätzlich zurück. Diese Verbesserung wird nicht mit dem früheren unveränderten Renamefehler gleichgesetzt. **Es wurde in dieser Nachprüfung kein einfacher Rename-Datenverlust trotz dieser Flags nachgewiesen.**

Der neue `discardTransientEditsForRecall()` setzt aber lediglich die logische View auf unsichtbar. Im exakt gepinnten Win32-SDK bedeutet das **nicht**, dass das native EDIT-HWND verschwindet oder seinen Fokus verliert. **Z03** beschreibt diesen getrennten Lifecyclefehler. Die erfolgreiche Generationprüfung ersetzt die ordnungsgemäße Beendigung des nativen Eingabefelds nicht.

Für Escape, Enter und Fokusverlust ist zwischen bewusstem Commit einer aktuellen Bearbeitung und Abbruch einer alten Kitbearbeitung zu unterscheiden. Beim gezielten Schließen des nativen Felds können noch SDK-Änderungs-/Fokuscallbacks auftreten. Die bereits richtige Reihenfolge „erst Token ungültig machen“ muss dabei erhalten bleiben. Das Feld erst zu schließen und danach die Tokens zu invalidieren würde den alten Datenverlust wieder ermöglichen.

## 5. RC04 – Dropziele, Pfade und Sampleerhalt

[View:238](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L238) bestimmt Zielzulässigkeit und Pfaderkennung getrennt und verwendet die drei Ergebnisse des neuen Helfers [Drop:3](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/DropFeedback.h#L3). Die Priorität ist:

| Ziel erlaubt | geeigneter Dateipayload | Diagnose / Wirkung |
|---|---|---|
| nein | nein | Ziel blockiert, `DragOperation::None` |
| nein | ja | Ziel blockiert, kein irreführender fehlender Pfad |
| ja | nein | ungeeigneter/fehlender WAV-/FLAC-Pfad, `None` |
| ja | ja | Pfad erkannt, `Copy`; tatsächlicher Import entscheidet später |

Bei blockiertem Ziel wird die Pfadextraktion absichtlich nicht benötigt. Das ist mit der Priorität „Ziel zuerst“ konsistent. `onDragMove()` benutzt dieselbe Ziel-/Pfadfreigabe. `acceptFileDrop()` prüft Ziel und Payload erneut; alle Ablehnungen kehren **vor** `loadPath()` zurück [View:702](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L702)–[View:720](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L720). Diese Wege ändern nur Status/Redraw und ersetzen keine Samples.

„Erkannter WAV-/FLAC-Pfad“ ist kein Versprechen, dass die Datei existiert oder ihre Audiodaten decodierbar sind. Diese Prüfungen erfolgen im unveränderten Importpfad mit dessen eigenen Fehlermeldungen und Kandidatenübernahme. Der native CF_HDROP-Patch wurde von dieser Reparatur nicht verändert. Ein neuer nativer Datei-/Hostbrowser-/Fehlererhaltstest wurde hier nicht ausgeführt; der eingegrenzte Diagnosefehler RC04 ist im Quellvertrag geschlossen.

## 6. Verbleibende konkrete Fehler, nach Priorität

### Z01 – HOCH: Abgewiesener moderner Recall kann bereits Daten, Undo und aktive Edits verändert haben

**Quellen:** [Controller:135](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L135)–[Controller:173](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L173), insbesondere Invalidierung145, Gridwrite146, Stringkopie148, Undoreset149, spätere Labelkopien166–168 und `bad_alloc`-Catch173. Datentypen/Längen: [Kit:33](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/KitState.h#L33), [Kit:75](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/KitState.h#L75).

**Ursache:** Der Code bereitet den decodierten Kandidaten vor, beginnt aber danach eine nicht vollständig ausfallsichere Übernahme. `customNames_=candidate.displayNames` kopiert16 `std::string`s; ebenso können `labels_`-Zuweisungen später Speicher anfordern. Der umfassende `catch(std::bad_alloc)` liefert auch bei einem Fehler **nach** Invalidierung und Gridwrite `kOutOfMemory`, ohne alte Felder und laufende Bearbeitungen wiederherzustellen.

**Konkretes Fehler-/Reproduktionsszenario:**

1. Benutzte Instanz mit KitX, laufender Velocity-/Namebearbeitung und gegebenenfalls vorhandener Undohistorie. Alte Customnamen sind leer/kurz.
2. Gültiger DM15-Kandidat KitY mit einem anderen Grid und einem64-Byte-Displaynamen; Dekodierung und deren Allokationen erfolgreich abschließen.
3. In einer Allocator-Fault-Injection die danach erforderliche Stringkopie nach `invalidateTransientEditsForRecall()` scheitern lassen, etwa die erste `customNames_`-Kopie auf einer frisch zuvor leeren Zeichenkette.
4. Der Controller meldet `kOutOfMemory`. Seine Generation wurde trotzdem erhöht, alle Views haben gültige alte Edits verloren und `grid_` enthält bereits KitY. Namen/Mix/Undo können noch vonKitX stammen. Bei späterer `labels_`-Allokationsablehnung ist noch mehr des neuen Zustands bereits übernommen.

**Nachweis:** Dies ist ein aus dem tatsächlichen C++-Exception-/Zuweisungspfad bestätigter Fehlerfall. Das zusätzliche Faultmodell zeigt beispielsweise `result=kOutOfMemory`, neue Generation2, neues Grid`[90,115]`, alte NamenX und verworfenen Stroke. **Eine echte C++-/Windows-Allokationsfehlerinjektion wurde hier nicht ausgeführt.** Es wird kein gewöhnlicher beschädigter-Stream-Fail behauptet: Diese kehren früher zurück und erhalten den alten Zustand.

**Abgrenzung:** Gemeint ist hier die eigene Controller-Transaktion. `setComponentState()` ersetzt nicht den separaten Prozessor-`setState()`-Aufruf. Eine Rückrollung bereits vom Prozessor akzeptierter Daten kann diese Controllerfunktion allein nicht garantieren; die Hostfixture muss die Koordination beider Schnittstellen getrennt prüfen.

**Einordnung:** Stringkopien während der Übernahme bestanden schon vor dieser Reparatur. Die neuen Invalidationen sind richtig für einen **tatsächlich erfolgreichen Commit**, machen aber die behauptete Erhaltung laufender Edits bei *jedem* abgelehnten Recall zusätzlich unvollständig. Das ist kein Beleg, dass der alte Snapshot nach einem normal erfolgreichen Recall noch zurückgeschrieben wird.

**Gezielte Empfehlung:** Sämtliche potenziell allokierenden Controllerdaten einschließlich Labels und Customnamen vollständig vor dem Commit vorbereiten. Den entscheidenden Modellwechsel mit geeigneten `noexcept`-Moves/Swaps durchführen. Tokens vor jeder möglichen alten Editcallbackwirkung invalidieren, aber den transaktionalen Commitpunkt klar von potenziell scheiternder GUI-/SDK-Benachrichtigung trennen. Ein nach diesem Commit auftretender Benachrichtigungsfehler darf nicht als unveränderte Ablehnung behandelt werden. Keine weiter kopierenden Felder oder fehlschlagenden Nebenarbeiten nach einem angeblich ausfallsicheren Commit verstecken.

**Notwendige Regression:** Tatsächliches Controller-/Prozessorpaar mit unterschiedlichem KitX/Y, begonnenen Gesten in mehreren Views, modernen und Legacyfixtures; sämtliche eigenen Allokationsstellen nacheinander ausfallen lassen. Auf jeder als abgelehnt zurückgegebenen Übernahme vollständige Gleichheit von altem Controllerzustand, Generation, laufenden Editflags und zulässiger Audio-/Projektantwort fordern. Normale erfolgreiche Übernahme und ursprünglichen RC01/RC03-Positivschutz mitprüfen. Im neuen Helfertest gibt es hierfür keine entsprechende Operation.

### Z02 – MITTEL: Fehlgeschlagener Zoom-Rollback wird ignoriert und kann einen partiellen Wechsel zurücklassen

**Quellen:** [Controller:95](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L95)–[Controller:105](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp#L105), [View:342](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L342)–[View:350](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L350), geprüfter Editorpfad [Editor:41](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorEditor.h#L41); SDK [SDKEditor:600](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L600), [SDKFrame:248](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L248).

**Ursache:** Frühere tatsächliche Viewfaktoren werden jetzt korrekt erfasst. Ihre Wiederherstellung wird aber nur versucht, nicht ausgewertet. Der Rückrollpfad kann vom Host genauso abgewiesen werden wie der Hinweg. Das betrifft das lokale Rückzoomen nach abgelehntem Höhenrelayout und die Controller-Rückrollung vorher erfolgreich veränderter Views.

**Reproduktion mit kontrolliertem, tatsächlich angehängtem Testhost:**

- **Zwei Views:** Präferenz100%, ViewsA/B100%. A nimmt150% einschließlich Layout an. B lehnt den neuen Zoom ab. Die folgende Rückrollanfrage fürA auf100% wird ebenfalls abgelehnt. Der Code ignoriert dieses `false`, meldet den Statewechsel insgesamt als fehlgeschlagen und lässt Präferenz100%, tatsächlichesA150%, B100% zurück.
- **Eine View:** Ersten Framezoom auf150% annehmen, direkt folgendes `adjustEditorHeight()` ablehnen, Rückzoom auf100% ebenfalls ablehnen. Auch dessen Ergebnis wird ignoriert; der lokale Fehlerpfad kann mit Livezoom150% und weiterhin gespeicherter100%-Präferenz enden.

**Nachweis:** Die erlaubte Hostantwortfolge und ignorierten Ergebnisse sind am Produkt-/SDKcode belegt; die Quellflussmodelle ergeben die angegebenen unterschiedlichen Faktoren. **Kein native-HWND-/Studio-One-Resizeversuch wurde hier durchgeführt.** Es wird ausdrücklich anerkannt, dass die Methode `kResultFalse` statt eines falschen vollständigen Erfolgs liefert. Der verbleibende Defekt ist der nicht behandelte partielle sichtbare Zustand und Persistenz-/Rollbackvertrag, nicht ein ungeprüftes erneutes SDK-Kontextmenü.

**Gezielte Empfehlung:** Rollbackergebnisse und reale Faktoren nach jeder fehlgeschlagenen Phase auswerten; einen definierten partiellen Fehler-/Wiederherstellungszustand und eine konsistente Persistenzpolitik für eine/mehrere Views implementieren. Redundante Resizephasen reduzieren, vollständigen alten Layoutzustand berücksichtigen und Wiederherstellung nur als erfolgt behandeln, wenn sie verifiziert ist. Ein Host kann nicht zum Rückresize gezwungen werden; weder illegale Fensteränderungen noch pauschales Erfolgsmelden sind eine geeignete Lösung. Diese Grenze muss im Fehlervertrag und in der Diagnose sichtbar bleiben.

**Notwendige Regression:** Native Fake-`IPlugFrame`-Fixture mit einer nach Aufrufnummer und View unterschiedlichen Annahme-/Ablehnungsfolge. Nach jedem Pfad Editorcache, Frame-Transformation, View-/Hostgröße, Lane/Compact und gespeicherte Präferenz prüfen; erneutes Speichern/Reopen sowie gleichen/anderen Folge-Recall einbeziehen. Die Fehlerstelle war schon als offener Rollbackfall im vorherigen Audit genannt und wurde nicht vollständig geschlossen.

### Z03 – MITTEL: Recall blendet die View aus, beendet aber den nativen Windows-Namenseditor nicht

**Quellen:** [View:323](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L323)–[View:331](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp#L331); SDK `CView::setVisible` [SDKView:1041](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cview.cpp#L1041), `CTextEdit::looseFocus` [SDKText:378](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/controls/ctextedit.cpp#L378), `Win32TextEdit` [SDKWinText:65](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32textedit.cpp#L65), [SDKWinText:118](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32textedit.cpp#L118), [SDKWinText:163](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32textedit.cpp#L163); Frame-Fokusabschluss [SDKFrame:1194](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L1194).

**Ursache:** `nameEdit_->setVisible(false)` verändert im gepinnten `CView` ausschließlich das Sichtbarkeitsflag und Invalidierungsrechtecke. `CTextEdit` hat keinen dazu passenden `setVisible`-Override, der seinen Plattformeditor schließt. Der Windowseditor ist ein separat erzeugtes `WS_CHILD|WS_VISIBLE`-EDIT-HWND. Sein `DestroyWindow()` erfolgt beim Plattformeditor-Abbau im Fokusabschluss, nicht beim Umschalten des gemalten Viewflags. Win32-`updateSize()` ist hier sogar noch ein nicht implementierter Stub; ein gewöhnliches Relayout ersetzt keinen verlässlichen Fokusabschluss.

**Konkrete Reproduktion:** Shift+Name öffnet das native Feld. Während der Fokus dort bleibt, Hostzustand programmgesteuert mit gleicher Rowzahl/Höhe recallen, ohne Fenster zu entfernen oder einen anderen Fokus anzufordern. Der Produktpfad setzt EditingRow/Token auf ungültig und die logische View auf unsichtbar; der native Editorbesitzer und Framefokus werden jedoch nicht beendet. Das alte EDIT-HWND kann weiter sichtbar und fokussiert sein und Eingaben abfangen, bis ein tatsächlicher Fokusabschluss/Enter/Escape oder Editorentfernung erfolgt. Bei geänderter Rowzahl kann es zusätzlich am nicht mehr passenden alten Ort stehen.

**Nachweis:** Die konkrete Trennung zwischen Sichtbarkeitsflag, HWND und Fokuslebensdauer ist aus dem exakt gepinnten Windows-SDK nachgewiesen. Der Zustandsnachweis ergibt `logicalVisible=false`, aber weiterhin bestehenden nativen Editor/Fokus. **Kein echter Windowsfensterlauf wurde hier ausgeführt.** Die neuen Tokens verhindern den einfachen alten Renamecommit; dieser Befund behauptet deshalb keine nachgewiesene Beschädigung neuer Namen trotz Guard.

**Gezielte Empfehlung:** Nach der bereits richtigen Invalidierung von Row/Token den nativen Fokus-/Plattformeditor über den vorgesehenen SDK-Abschluss tatsächlich beenden beziehungsweise kontrolliert entfernen. Reentrante `valueChanged`-/Fokuscallbacks müssen weiterhin alte Edits verwerfen. Bei inaktivem Frame, mehreren Views und erneutem Start einer Namensbearbeitung sauber mit beiden SDK-Fokuszuständen umgehen; eine bloße zusätzliche `setVisible(false)`-Variante reicht nicht.

**Notwendige Regression:** Tatsächlich angehängtes Win32-`CTextEdit`; vor/nach erfolgreichem Recall HWND-Existenz/Sichtbarkeit und nativen/Framefokus prüfen. Enter, Escape, echter Fokusverlust,1/4/16 Rows, erneutes Nameedit und Close testen. Neue Namen/Statebytes dürfen sich durch alte Abschlusscallbacks nicht verändern. Ein Fehlerrecall muss das gültige bisherige Feld und dessen Eingabe erhalten. Die Generationshelperprüfung kann HWND-/Fokuslebensdauer nicht nachweisen.

## 7. Neue Tests: tatsächlich geprüfter Vertrag und Lücken

`tests/RecallUIContractTests.cpp` inkludiert ausschließlich `KitEditGeneration.h` und `DropFeedback.h`, keine Controller-/View-/Hostimplementierung. Der CMake-Target bindet den Core-Interface-Target ein. Seine `CHECK`-Prüfungen bleiben in Release aktiv; ein neuer `assert`-/`NDEBUG`-Leerpass ist hier nicht die Ursache.

| Teststelle | Tatsächliche Aussage | Nicht geprüft |
|---|---|---|
| [RecallTest:6](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L6)–[RecallTest:16](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L16) | Token ist zunächst gültig; `onSuccessfulRecall()` macht alte Tokens ungültig;0 ist ungültig. | Kein Komponentenstate, kein Stream, keine Codecvalidierung, keine Controller-/Viewübernahme, keine Mausgeste und kein Callback. |
| [RecallTest:9](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L9)–[RecallTest:10](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L10) „Failed recall“ | Derselbe zuvor gültige Token wird **ohne dazwischen ausgeführte Operation** erneut geprüft. | Es gibt keinen fehlgeschlagenen Recall. Weder früh abgewiesener Stream noch spätes `bad_alloc` werden ausgeführt. |
| [RecallTest:19](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L19)–[RecallTest:22](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp#L22) | Vollständige boolesche Truth-Tabelle der Dropklassifikation. | Kein echtes `DragEventData`, keine View, kein WAV-/FLAC-Payload, kein Samplecommit oder Dateisystemfehler. |
| Neue Zoomprüfung | Keine im neuen Test vorhanden. | Weder `setState` mit mehreren Views noch Hostablehnung oder fehlgeschlagener Rückrollback. |
| Native Texteditprüfung | Keine im neuen Test vorhanden. | Kein HWND, SDK-Fokus, Hide/Destroy, Enter/Escape, Slot-/Generationwiederverwendung. |

Der Reparaturbericht legt diese Grenze ausdrücklich offen; dieser Audit unterstellt ihm keinen behaupteten nativen Studio-One-PASS. Der grüne Helfertest ist für seine engen Datenstrukturen sinnvoll. Seine Ergebnisse schließen die im Auftrag verlangten Controller-/View-/Host-Gesamtverträge jedoch nicht.

### 7.1 Weiterhin offene Tests T01–T04

Die betreffenden Testdateien wurden im RC-Reparaturcommit nicht verändert. Die früheren Lücken bestehen an denselben Stellen fort:

| ID | Priorität / Status | Quellnachweis und erforderliche Korrektur |
|---|---|---|
| **T01** | MITTEL / **FAIL als Beleg des behaupteten Plugin-Datenerhalts** | [HostTest:933](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/PackagedAudioHostSmoke.cpp#L933) liest die unveränderte lokale `patternKit.grid`-Fixture für Step20. Den tatsächlich gespeicherten/geladenen Prozessorstate auslesen und decodieren oder die zweite Patternhälfte wieder hörbar prüfen. Ein Löschen im Plugin könnte an der lokalen Prüfung vorbeigehen. |
| **T02** | MITTEL / **TEILWEISE für Routingisolation** | [HostTest:1535](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/PackagedAudioHostSmoke.cpp#L1535) erlaubt±0,03 bei0,015 Pegelabstand benachbarter Busse. Ein benachbarter Busswap liegt innerhalb der Toleranz; gleiches L/R-Material deckt keinen Kanalswap ab. Eindeutige Stereo-/Zeitsignaturen und begründete Grenzwerte verwenden. |
| **T03** | MITTEL / **TEILWEISE für tatsächliche Konkurrenzbeobachtung** | [AtomicTest:190](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/AtomicPatternTests.cpp#L190) verwirft `verified` ohne Mindestbedingung. Ein Lauf kann ohne erfolgreich überprüften überlappenden Snapshot bestehen. Synchronisierte Start-/Interleavingbarrieren und positive Beobachtung verlangen; kein bloßes letztes Endbild. |
| **T04** | HOCH für Atomizitätsfreigabe / **TEILWEISE** | [HostTest:1544](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/PackagedAudioHostSmoke.cpp#L1544) lädt sechsmal denselben Kitstate; [HostTest:1587](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/PackagedAudioHostSmoke.cpp#L1587) fordert Erfolg, finite Samples und positive Blockzahl. All-null-Ausgabe und endliche Mischgenerationen können bestehen. Zwei unterschiedlich signierte Kits, Automation/MIDI und pro Block erlaubte Gesamtsignaturen plus maximale Gate-Stille prüfen. |

Diese Aussagen betreffen die **Tests**, nicht neu nachgewiesene echte Busvertauschungen oder tatsächliche Audio-Mischgenerationen. Sie sind schon im vorherigen Audit genannt und durch diese punktuellen RC-Fixes nicht erledigt.

### 7.2 Was künftig als Produktregression benötigt wird

1. **State-/Editfixture:** echtes Controller-/Prozessorpaar, mindestens zwei instanziierte Views und unterschiedlich belegte Kits. Moderne/Legacy-Stateübernahme, absichtlich abgelehnte Streams und gezielte Allokationsfehler; Move/Up/Cancel/Close/Undo nach jedem Commitpunkt. Ein anderer unberührter Step muss als Datenverlustoracle dienen.
2. **Native Windowsfixture:** echtes HWND und VSTGUI-Attach. Für Textedit eigene HWND-Existenz/Sichtbarkeit und Fokus prüfen, nicht nur `CView::isVisible`. Für Zoom echten `IPlugFrame` mit sequenziell wechselnden, pro View steuerbaren Antworten verwenden.
3. **Dropfixture:** gültiger WAV-/FLAC-Payload im freien/gesperrten Bereich, ungeeigneter Payload im freien Bereich und fehlgeschlagener Import. Diagnose, `DragOperation`, Rückgabewert, alte Samplebytes, Audio und Dirty getrennt prüfen.
4. **Negativnachweise:** Eine absichtlich fehlende Invalidierung, falsche Generation, nicht ausgewertete Rückrollung, lokale statt tatsächlicher Stateprüfung oder vertauschte Busse müssen den jeweils zuständigen Test gezielt scheitern lassen. Keine Akzeptanzgrenzen lockern, nur um grüne Gesamtzahlen zu erzielen.
5. **Lifecycle-/Fehlervertrag:** Vorher definieren, welche Operation wann wirklich committed ist und was ein Fehlschlag nach bereits angenommenen Hostresizes bedeutet. Eine Sammlung isolierter Helfer kann keinen solchen Gesamtvertrag ersetzen.

## 8. O01 – möglicher Automation-/Restore-Konflikt

**Status OFFEN, Priorität HOCH für die unabhängige Verifikation.** Die RC-Reparaturen verändern weder `AtomicPatternSelection` noch den Prozessorpfad. Die neue `KitEditGeneration` betrifft Controller-/UIedits und ist keine Reparatur der Audio-/Host-Arbitrierung.

**Konkrete Stellen:** [Selection:13](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/AtomicPatternSelection.h#L13) normales `store()` erhält die oberen Generationsbits; [Selection:22](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/AtomicPatternSelection.h#L22) `hostSubmit()` erhöht sie. [Processor:871](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorProcessor.cpp#L871) nimmt einen Auswahl-Snapshot **vor** Eintritt des Restoregates, [Processor:886](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorProcessor.cpp#L886) versucht `restoreIfUnchanged()`.

**Zu prüfender Ablauf:** SnapshotA → normales neues HosteventA, beziehungsweise A→G→A, vor vollständiger Sperre des laufenden Audioblocks → RestoreKitB. Die gepackten Bits können am Ende wieder genau dem ursprünglichen Snapshot entsprechen. Der CAS kann deshalb den Kitwert übernehmen, obwohl ein als später betrachtetes gleichwertiges Hostevent verarbeitet wurde. Das einfache Bit-/Ablaufargument bleibt gültig; es beweist allein noch keinen Verstoß gegen einen eindeutig spezifizierten Hostprioritätsvertrag.

**Entscheidende offene Frage:** Soll ein Hostevent nach Beginn der Stateoperation oder erst nach deren Gate-/Commitlinearisation Vorrang haben? Bei unterschiedlichen Linearisationen ist nicht dieselbe Auswahl der erwartete Endzustand. Der Kommentar „kann neuere Hostautomation nicht überschreiben“ ist stärker als der bloße Test auf unveränderte Auswahlbits. Diese Semantik muss festgelegt werden.

**Erforderlich:** Deterministischer Barrieretest an tatsächlichem `Processor::setState/process`, mit bereits laufendem Audioblock, Snapshot/Gate/Endcommit als kontrollierten Punkten. Gleichwert-, ABA-, Busy-, Nullframe- und Normalpfad; Auswahl, Audio, gespeicherter State und Controllerbeobachtung vergleichen. Erst danach gegebenenfalls Generation-/Prioritätslogik gezielt ändern. Keine erneute Latchvariante allein aufgrund eines vereinfachten Helfermodells; der historische `PatternRestoreLatch` ist weiterhin kein aktiver Produktpfad.

**Kein zusätzliches neues O01-FAIL oder PASS wird hier ohne diese Produktreproduktion behauptet.** Die native Integration wurde nicht ausgeführt und ein geeigneter vollständiger Test wurde in der Reparatur nicht ergänzt.

## 9. Windows-Build #39341195970: verifizierbarer Umfang

Unabhängig abgerufen wurden die öffentliche [Runseite](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970) und die [Windows-Jobseite](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970/job/118362886423).

| Aussage | Evidenz dieser Nachprüfung |
|---|---|
| Richtiger Commit/Branch | `d984188`, `develop/v1.0.0`, passender Commitbetreff und Workflowlauf307. |
| Status | Run **Success**, Windowsjob **succeeded**. Run3m2s, Job2m59s, ein Artefakt. |
| Artefakt | Öffentliche Seite nennt `125A-Drumator-M11-Full-Kit-Stress-QA`,1,43MB und Digest `sha256:b93bff4157bffc61d001cc5f3ebe5cca639bf19ee9cac736c2e4f5d23c1e114c`. Das sind GitHub-Metadaten, keine hier selbst heruntergeladene und gehashte DLL/ZIP. |
| 16/16 CTests,47/47 Validator, Paket-Smokes | Im Reparaturbericht gemeldet; passender erfolgreicher Run und Workflow mit Fehlerprüfung unterstützen die Angaben. Vollständige Einzelergebnislogs waren öffentlich mit „Sign in to view logs“ gesperrt und wurden nicht erneut ausgelesen. |
| Registrierte CTests | Quelltext registriert16: **15 reguläre plus ein absichtlich fehlschlagender SongChain-Negativtest mit `WILL_FAIL`**. Das ist kein sechzehnfacher nativer UI-Funktionsnachweis. |
| Editor-/Audio-Smokes | Bestehende separate Workflowaufrufe; der Editor-Smoke bleibt ohne echten Fensterattach. Der Audiohost besitzt Produktorakel, aber die oben genannten T-Lücken. |
| Eigene neue Build-/Hostläufe | **Keine.** Keine neue Action, kein neuer Windows-/Studio-One-Lauf. |

Die im Run sichtbare Node.js-Actionwarnung ist kein neuer Produktfehler und wird nicht als Reparaturauftrag für fremde Workflowaufgaben verwendet. Ein grün beendeter Build ist relevante Regressionsevidenz, schließt aber die nativen Fokus-/Rollback- und Postcommitfehler nicht.

## 10. Reproduzierbare Quellflussnachweise und offene native Durchführung

Die zusätzlichen Hilfsprüfungen wurden außerhalb der Repositoryarbeitskopie gehalten; bestehende Tests wurden nicht verändert. Das Verhalten wird hier absichtlich nicht als „C++-Tests bestanden“ etikettiert.

```python
# Z01: tatsächliche Reihenfolge im modernen Controllerpfad, als Fehlermodell
state = dict(epoch=1, grid=[20, 64], names=['X'], stroke=True)
try:
    state['stroke'] = False             # invalidateTransientEditsForRecall
    state['epoch'] += 1
    state['grid'] = [90, 115]           # grid_=candidate.grid
    raise MemoryError('customNames_ copy')  # hier gezielt angenommener Speicherfehler
except MemoryError:
    result = 'kOutOfMemory'
assert result == 'kOutOfMemory'
assert state['grid'] == [90, 115] and state['names'] == ['X']
assert state['epoch'] == 2 and state['stroke'] is False

# Z02: definierte zulässige Hostantwortfolge im Zweiview-Rollbackmodell
preferred = 1.0
actual = [1.0, 1.0]
actual[0] = 1.5                         # ViewA nimmt angeforderten Zoom an
view_B_accepts = False
rollback_A_accepts = False
if not view_B_accepts:
    if rollback_A_accepts:
        actual[0] = 1.0
    result = 'kResultFalse'
assert result == 'kResultFalse' and actual == [1.5, 1.0] and preferred == 1.0

# RC04: vollständiger eingegrenzter Klassifikationsvertrag
classify = lambda target, path: ('blocked' if not target else
                                'invalid' if not path else 'valid')
assert classify(False, True) == 'blocked'
assert classify(False, False) == 'blocked'
assert classify(True, False) == 'invalid'
assert classify(True, True) == 'valid'
```

| Unabhängige Zusatzprüfung | Ergebnis / Einschränkung |
|---|---|
| Sourceguards für drei erfolgreichen Controller-Statezweige | Invalidierung jeweils nach zugehöriger Validierung, vor Modellwrite vorhanden. |
|216 Kombinationsmodelle alter Callbacks nach erfolgreichem Recall | Keine Modellmutation neuer Werte durch alte Move/Up/Cancel/Close/Undo/Nameabschlüsse; keine tatsächlichen Formate decodiert, kein Windowsevent ausgeführt. |
| Frühe Decodeablehnung | Quellpfade kehren vor Invalidierung zurück; bestehende Tokens werden dort nicht verändert. |
| Z01: Fehler nach Beginn der Übernahme | Fehlschlag bei bereits verändertem Grid/Generation/Stroke im Modell nachvollzogen. Echte Allocator-Injektion offen. |
| Z02: ein-/mehrviewiger Rückrollfehler | Zurückbleibende unterschiedliche tatsächliche Faktoren/Präferenz im Modell nachvollzogen. Echte Hostfixture offen. |
| Z03: SDK-Sichtbarkeit vs. Fokuslebensdauer | SDK-Quellvertrag hält natives EDIT-HWND/Fokus unabhängig vom Viewflag. Echter HWND-/Fokuslauf offen. |
| RC04: vier boolesche Kombinationen plus Ablehnungsreihenfolge | Ziel-/Pfadklassifikation korrekt; keine Ladefunktion vor bestandenen Guards. Native Payload-/Datei-/Sampleerhaltmatrix offen. |

## 11. Konkrete Nacharbeiten und Abnahmekriterien

**Priorisierte Produktkorrekturen:**

1. **Z01 HOCH:** Controller-Stateübernahme einschließlich Namen/Labels und transienter Editgeneration in einen nachvollziehbaren ausfallsicheren Commit überführen. Potenziell scheiternde Vorbereitung vorziehen, Rückgabe-/Benachrichtigungsfehler nach dem Commit konsistent behandeln. Alte gültige Edits bei tatsächlich abgelehntem Recall erhalten.
2. **Z02 MITTEL:** fehlgeschlagenen lokalen und Mehrfach-View-Rollback nicht ignorieren. Tatsächliche Faktoren/Layoutzustände auswerten, einen definierten partiellen Fehler-/Persistenzvertrag und überprüfbare Wiederherstellung einführen.
3. **Z03 MITTEL:** nach Tokeninvalidierung den nativen Texteditor/Fokus wirklich abschließen; späte SDKcallbacks müssen weiter unschädlich sein.

**Erforderliche Verifikation, keine unbewiesenen weiteren Reparaturen:**

4. Neue **echte Controller-/View-/Hostregressionen** für Erfolg, frühe Ablehnung und spätes Scheitern. Mehrfach-View/Gesteneigentümer und native Capture-/Fokusreihenfolge kontrollieren. Ein weiterer Test nur des Generationszählers schließt das Problem nicht.
5. Testorakel **T01–T04** wie beschrieben korrigieren beziehungsweise verstärken und durch geeignete Negativfälle belegen.
6. **O01** mit festgelegter Linearisation und deterministischem Produktinterleaving entscheiden; erst bei belegtem Fehler gezielt synchronisieren.
7. Native Windows-/Studio-One-Matrix durchführen und reale Echtzeitfähigkeit gesondert nachweisen. Vorhandene unpaced Durchsatz-/Finite-Smokes und20s-Laufzeitgrenzen sind kein Deadlinebeweis. Callbackallokationen, p95/p99/max, Deadlineüberläufe, Gate-Stille, Peak-RAM und mehrere Instanzen messen.

### 11.1 Studio-One-/Studio-Pro-Läufe, die weiterhin nicht durchgeführt sind

- Gehaltene Velocitygeste inKitX, erfolgreicher moderner/Legacy-RecallKitY, danach alte Move/Up/Cancel/Close/Undo; andere unberührte Steps und Audio-/Statebytes kontrollieren. Fehlgeschlagene Streams und unter Speicherfehlerinjektion abgelehnte Übernahmen gesondert prüfen.
- Mindestens zwei offene Views, Wechsel des aktiven Fensters und kontrollierte verzögerte Abschlüsse; kein fremder Stroke darf durch den Abschluss einer anderen View zurückgesetzt werden. Erst native Reihenfolge nachvollziehen, dann als Fehler oder PASS einstufen.
- Nameeingabe offen, gleicher/anderer Kitstate mit1/4/16 Rows: HWND verschwindet, Fokus ist zurückgegeben, kein alter Name wirkt auf neues Kit; Enter/Escape/Klick außerhalb und erneuter Editbeginn funktionieren.
- 100-/150%-Tasten, gleichwertiger/abweichender Controllerstate, Reopen/Projektneustart, DPI-/Monitorwechsel sowie Annahme/Ablehnung mehrerer Resizephasen. Entferntes SDK-Zoommenü darf nicht erscheinen; tatsächliche und gespeicherte Werte müssen dem definierten Fehlervertrag entsprechen.
- WAV-/FLAC-Drop aus Explorer und Hostbrowser bei Song/Fill offen/zu, freiem Bereich daneben und Compactpaging. Zielblockierung/ungeeigneter Pfad/Importfehler klar unterscheiden; Samplebytes und Audio bei Ablehnung erhalten.
- Bestehende MIDI-/Automation-, Multi-Out-, Transport- und Realtimegates des Gesamtaudits am exakt später korrigierten Bundle; vollständiger Projektneustart und Besitzerabnahme.

**Abschließendes technisches Urteil:** Die RC-Fixes sind sinnvolle gezielte Reparaturen und schließen mehrere ursprüngliche Quellpfade. Die vollständigen Verträge für abgelehnte Zustandsübernahmen, native Textedit-Lebensdauer und gescheiterte Zoomrückrollungen sind noch nicht erfüllt beziehungsweise nicht hinreichend abgesichert. Die technische Grundlage erhält daher **keine funktionale V1-Freigabe**. Eine finale Gestaltung ist durch diesen Audit nicht beauftragt. Eine spätere Freigabe benötigt die konkret benannten Produktkorrekturen und tatsächliche Integrationsnachweise; weitere grüne Helfertests allein reichen nicht.

## 12. Geprüfter Reparaturdiff und feste Referenzen

| Reparaturdatei | Zeilen | SHA-256-Präfix |
|---|---:|---|
| [CMakeLists.txt](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/CMakeLists.txt) | 251 | `0f5b8210b14c` |
| [src/core/DropFeedback.h](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/DropFeedback.h) | 9 | `4108ec0dac48` |
| [src/core/KitEditGeneration.h](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/core/KitEditGeneration.h) | 16 | `e2bde401ae11` |
| [src/vst3/DrumatorController.cpp](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.cpp) | 768 | `952de17489ba` |
| [src/vst3/DrumatorController.h](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorController.h) | 144 | `01cb99a99294` |
| [src/vst3/DrumatorView.cpp](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.cpp) | 1088 | `90d99796a8c8` |
| [src/vst3/DrumatorView.h](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/src/vst3/DrumatorView.h) | 70 | `ad920312e2d0` |
| [tests/RecallUIContractTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/tests/RecallUIContractTests.cpp) | 24 | `a6f5181b39c0` |

- [Vorheriger unabhängiger RC-Ausgangsaudit](https://github.com/challanger2000/125A-Drumator/blob/cc3a2ef495b9a282ee03308f5560ed4569b00525/docs/audits/CODEX-RECHECK-A01-A09-2026-10-19.md)
- [Reparaturbericht, exakt beauftragter Dokumentationsstand](https://github.com/challanger2000/125A-Drumator/blob/7740c6b890bd41fe865019f3aeabb9a17df9b302/docs/CODEX-RC01-RC04-REPAIR-2026-10-20.md)
- [Engineering-Einstieg](https://github.com/challanger2000/125A-Engineering/blob/29bd83e9cfc1c7f46ac1a60a35a9341d310ba08b/START-HERE.md)
- [Aktuelle Guardrails](https://github.com/challanger2000/125A-Engineering/blob/29bd83e9cfc1c7f46ac1a60a35a9341d310ba08b/ENGINEERING-GUARDRAILS.md)
- [Projektspezifische Regeln am Produktcommit](https://github.com/challanger2000/125A-Drumator/blob/d984188457882a4463b9263593d88f7bf1fffb71/AGENTS.md)
- [Vorhandener Windowslauf](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970)

Die Veröffentlichung auf `audit/codex-recheck-rc01-rc04-2026-10-20` enthält ausschließlich diese Berichtsdatei. Der Bericht-Commit ist vom geprüften Produktcommit getrennt und behauptet keinen neuen Build. Entwicklungs- und Mainbranch bleiben unverändert.

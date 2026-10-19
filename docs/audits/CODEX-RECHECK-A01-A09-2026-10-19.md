# 125A Drumator – unabhängige Nachprüfung A01–A09

**Prüfdatum: 20.10.2026, Europe/Berlin.** Der Dateiname `CODEX-RECHECK-A01-A09-2026-10-19.md` und der Auditbranch mit Datum19 entsprechen ausdrücklich dem Auftrag; sie sind keine Rückdatierung der Durchführung.

## 1. Urteil und Umfang

**Funktionale V1-Reife: NEIN.** Die Reparaturen enthalten echte Verbesserungen. Das destruktive 256-Ereignis-Limit ist im aktuellen Produktpfad entfernt, ein MIDI-Ausgabeparameter ist hinzugekommen, und mehrere konkrete GUI-Fehler sind korrigiert. Dennoch sind nicht alle neun Reparaturen vollständig geschlossen:

- **3 PASS:** A02, A03, A09 für den jeweils ausdrücklich eingegrenzten ursprünglichen Logikfehler.
- **4 TEILWEISE:** A01, A04, A05, A06. Korrekturen sind vorhanden; vollständige End-to-End-/native Verifikation fehlt, bei A04 besteht zusätzlich der verwandte neue Recallfehler RC03.
- **2 FAIL:** A07, A08 wegen konkret nachgewiesener verbleibender beziehungsweise neu eingeführter Fehlerpfade.
- **Vier zusätzliche konkrete Befunde:** RC01 **HOCH**, RC02 **MITTEL**, RC03 **MITTEL**, RC04 **NIEDRIG**. Ihre Ursachen, Aufrufreihenfolgen, Grenzen und Regressionen stehen in Abschnitt4.

Ein Windows-Success oder die Anzahl grüner Tests ist keine funktionale V1-Freigabe. Dieser Bericht ist eine unabhängige **Nachprüfung der neun Reparaturen und ihrer Nebenwirkungen**, keine Behauptung, alle im früheren Gesamtaudit offenen Risiken seien nun erledigt. Der Besitzer entscheidet über eine spätere Veröffentlichung; dieser Audit erteilt keine eigenmächtige Freigabe.

Es wurden **kein Produktionscode, keine bestehenden Tests, keine Workflowdateien, kein `develop/v1.0.0` und kein `main` verändert**. Ausschließlich dieser neue Bericht wird auf dem beauftragten neuen Auditbranch committet. Es wurde keine neue GitHub Action ausgelöst.

## 2. Exakte Quellen und Beweisklassen

| Gegenstand | Festgeschriebene Identität |
|---|---|
| Repository | `challanger2000/125A-Drumator` |
| **Geprüfter Reparatur-/Produktcommit** | **`dbd9106f7af48b23ad4874a1a4c74395baa14173`** |
| Erster Reparaturcommit | `e3218ffaa27669106233cf2c8ceb98d10da4b75f` |
| Ausgangscode des ursprünglichen Audits | `ee712dd094e28fdc9bd6da9917f249aa7ef7b185` |
| Commit des ursprünglichen Auditberichts | `3ba3286938c55f89e379ef4eef2a4b89d7c7eb3f`, Branch `audit/codex-drumator-2026-10-19` |
| Entwicklungs-HEAD bei dieser Nachprüfung | `57b902b505bd9bf04110391af0313a7719ec6bda` |
| Reparaturbericht tatsächlich vorhanden ab | `57b902b505bd9bf04110391af0313a7719ec6bda`, `docs/CODEX-FINDINGS-REPAIR-2026-10-19.md` |
| `src`-Tree in `dbd9106` und `57b902b` | identisch: `e57b67d41663c99079f506e15d338c834b09ffe6` |
| `tests`-Tree in `dbd9106` und `57b902b` | identisch: `007b8efc3f5c7307c065b687088c3020ef0e6d4d` |
| Plugin-Metadatenversion | `0.1.0`; keine hier belegte freigegebene Windows-FULL-V1 |
| Engineering-Basis | `125A-Engineering` `b98fe97b474edd22cf8f8d7f9b0380576b97cd34` |
| SDK | VST3 `v3.8.1_build_84`, `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| Gepinntes VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| Neuer Berichtbranch | `audit/codex-recheck-a01-a09-2026-10-19`, ab Dokumentationsstand `57b902b`, dessen Produkt-/Testbäume exakt dem beauftragten Commit entsprechen |

Der Reparaturbericht existiert **nicht innerhalb `dbd9106`**, sondern erst im nachfolgenden Dokumentationscommit. Der Diff danach betrifft ausschließlich `AGENTS.md`, `README.md` und den Reparaturbericht. Es wurde deshalb die Dokumentation aus `57b902b`, aber konsequent der Produkt-/Testcode aus `dbd9106` bewertet. Alle Produktzeilenlinks dieses Berichts sind auf `dbd9106` festgeschrieben.

Gelesen wurden der verbindliche Engineering-Einstieg, die aktuellen Guardrails einschließlich des neuen A01–A09-Eintrags, einschlägige Standards für Professional Plugin Behavior, GUI, AI-Assisted Engineering, Repository-/Releaseprovenienz und Release-/Realtime-QA sowie die projektspezifischen `AGENTS.md`. Die Aussage „verified scoped correction“ in den Engineering-Guardrails wurde **nicht als unabhängiger Nachweis übernommen**. Sie ist genau wie der Reparaturbericht eine zu prüfende Behauptung. Die Regeln zur Owner-Freigabe und Releaseprovenienz bleiben getrennt; der ausdrücklich benannte Entwicklungsstand ist keine unterstellte kommerzielle FULL-Basis.

### 2.1 Was hier tatsächlich geprüft/ausgeführt wurde

- Vollständiger Diff der zwei Reparaturcommits gegen `ee712dd`: **14 Dateien, 296 hinzugefügte und40 entfernte Zeilen**. Zusätzlich wurden die vollständigen betroffenen Aufrufpfade, bestehende Integrationsprüfungen, Rückmeldungs-/Statepfade und der exakt gepinnte SDK-Code untersucht.
- Unabhängige Python-Quellguards und Referenz-/Ablaufmodelle: **200 Timeline-Referenzfälle**, **201 Master-Startwerte**, **17.632 Compact-Geometriekombinationen** und **3.080 Song-Button-Rand-/Lückenpunkte**. Zustandsgegenbeispiele zu RC01–RC03 wurden anhand der tatsächlichen Verzweigungen nachvollzogen. Konkrete Ergebnisse und kleine Reproduktionen stehen weiter unten.
- Diese Python-Prüfungen sind **keine Ausführung des C++-Plugins**, kein neu gebauter CTest und kein nativer Windows-PASS. Sie dienen der unabhängigen Logik-/Geometriekontrolle. Die Modelle wurden gegen die aktuellen Quellstellen abgeglichen; bestehende Tests wurden weder geändert noch als neu ausgeführt dargestellt.
- Auf der verfügbaren Linuxmaschine wurden gemäß Windows-only-Projektregel **kein Plugin und keine C++-Tests gebaut**. Kein Wine-/Windows-/Studio-One- oder nativer VSTGUI-Lauf wurde ausgeführt. Es gibt keine hier neu gemessene Audio-Deadlinezeit und keinen neu verifizierten Bundlehash.
- Öffentlich lesbare Run- und Jobseiten des vorhandenen Windowslaufs wurden geprüft; vollständige Logs waren dort anmeldepflichtig. Die Unterscheidung steht in Abschnitt5.

### 2.2 Statusbedeutung

**PASS** bedeutet hier: Der im Befund benannte konkrete Logikfehler ist anhand der echten Implementierung und der benannten unabhängigen Referenz-/Quellnachweise geschlossen. Das ist ausdrücklich keine pauschale Aussage „in jeder DAW nativ bestanden“. **TEILWEISE** bedeutet implementiert, aber der verlangte Gesamtpfad ist unvollständig verifiziert. **FAIL** bedeutet einen nachvollziehbaren fehlerhaften Produktpfad mit konkretem Gegenbeispiel. Risiken ohne hinreichenden Nachweis erhalten separat **OFFEN**, keine erfundene FAIL-Reproduktion.

## 3. Einzelbewertung sämtlicher Reparaturen

| ID | Ergebnis | Konkreter Nachweis | Verbleibende Grenze |
|---|---|---|---|
| **A01 MIDI→Host/Controller/GUI** | **TEILWEISE** | [P:561](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L561) erzeugt Ausgabequeue für Parameter1000; [P:676](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L676)/[P:686](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L686) melden MIDI mit ursprünglichem Sampleoffset und `pattern/7`. [Host:1160](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1160) prüft Offset64/B und gibt den Wert manuell an den echten Controller zurück. | Die tatsächliche Hostweiterleitung zur offenen View ist nicht getestet. Gemischte spätere/frühere Hostwerte, Fehler bei `addPoint`, Wiederanlauf nach fehlendem Ausgabepuffer und Recallkonkurrenz fehlen. |
| **A02 mehr als256 Patternpunkte** | **PASS für den ursprünglichen destruktiven Overflow** | Neue vorallokierte Offsettimeline [Timeline:13](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/core/PatternEventTimeline.h#L13), im Setup angelegt [P:435](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L435), pro Block zurückgesetzt [P:533](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L533). Kein `editCount=0`-/256er-Abbruch mehr. Zeitordnung wird durch aufsteigende Offsets bestimmt, Host gewinnt Ties.257 verschiedene gültige Offsets innerhalb512 Frames bleiben erhalten; unabhängige Referenzvergleiche stimmen überein. | Kein allgemeiner Echtzeit-/Grenzlast-PASS. Der Pakettest hat258 Eingabeeinträge, aber nur250 verschiedene gewinnende Offsets; echte512-/1024-Frame-Integrationsmatrix ergänzen. |
| **A03 Masterdefault** | **PASS für Resetwert und Zuordnung** | [V:736](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L736) berechnet bei Strg jetzt `100-current`; der echte Masterdefault ist100. Alle201 erlaubten ganzzahligen Startwerte0…200 ergeben100. Normal±5 bleibt erhalten. | Reale Strg+Links-Ereigniszustellung/Host-Dirty ist nicht neu nativ ausgeführt. |
| **A04 Name Enter/Escape** | **TEILWEISE** | [V:287](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L287) registriert `ITextEditListener`; [V:306](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L306) versteckt/resettiert auch bei Fokusende ohne Textänderung. Gepinnter SDK-Abschluss ruft diesen Listener unabhängig von `valueChanged` auf. | Kein nativer Fokus-/Unicode-/Lifecycle-Regressionslauf. Zusätzlich neuer Recallrest RC03; „Namenseditor vollständig sicher“ ist nicht bestätigt. |
| **A05 Drop unter Overlays** | **TEILWEISE** | [V:259](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L259) verwendet gemeinsamen sichtbaren Zieltest; Enter/Move und Drop rufen ihn auf [V:237](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L237), [V:246](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L246), [V:677](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L677). Song blockiert Drops; Fill blockiert nur die verdeckte Fläche. Die ursprünglichen Punkte werden korrekt abgewiesen. | Test prüft nur Helfer. Kein aktueller nativer Datei-/Transform-/Paging-/Commit-Erhaltstest am tatsächlichen View-/Hostpfad; zusätzlich falsche Ablehnungsdiagnose RC04. |
| **A06 Compact-Fallback** | **TEILWEISE** | [V:339](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L339) bestimmt Pagegröße aus bestehender Höhe über [Compact:9](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/core/CompactRowViewport.h#L9); Lane wird bei zu geringer Höhe geschlossen. Altes357px-Gegenbeispiel behält nur eine Lane-Row.300px ergeben drei geschlossene Rows.17.632 rechnerische Kombinationen behalten Status/Footer innerhalb300…850px. | `getViewSize()` wurde nicht gegen echten HWND-/Frameclient und Hostablehnung gemessen. Kein angeschlossener Resize-/DPI-/Zoom-/Pagingtest. |
| **A07 Velocity-Gestenundo** | **FAIL** | Normaler Stroke hat jetzt einen Vorzustand [C:457](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L457), Zwischenwerte überschreiben ihn nicht [C:494](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L494). **Modernes State-Recall verwirft den Stroke aber nicht**, [C:135](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L135)–[C:160](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L160), anders als Legacy [C:187](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L187). Cancel/Close/Undo kann alte Patterns in ein neues Kit schreiben: RC01. | Kein neuer tatsächlicher Controller-/View-Stroketest; bisheriges `PatternUndoTests.cpp` bleibt unverändert und kennt diesen Lifecycle nicht. |
| **A08 offener Zoom-Recall** | **FAIL für vollständige Synchronisierung** | Neuer View-Aufruf [C:87](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L87)/[V:312](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L312) korrigiert den einfachen100→150-Recall. Der geerbte Kontextmenü-Zoom aktualisiert jedoch weder Präferenz noch sicheren Rollback; [C:85](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L85) kann danach eine notwendige Korrektur überspringen: RC02. | Mehrfach-View-/Rückrollback und Hostablehnungen nicht getestet; Rückgabefehler beim Rollback werden teilweise ignoriert. |
| **A09 unsichtbare Buttonlücken** | **PASS für die gemeldeten Lücken** | Songpattern prüft zusätzlich die50px-Breite [V:770](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L770), Songlänge die100px-Breite [V:773](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L773). Footer1014…1024 löst weder Reset noch Erfolgsmeldung aus [V:883](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L883).3.080 unabhängige Rand-/Gapvergleiche stimmen mit den gezeichneten Rechtecken überein. | Kein neuer nativer Dispatcher-/DPI-Test für alle Controls; weitere42-Gruppen-GUIabnahme bleibt eigener Gate. |

### 3.1 A01: tatsächlich verbesserter Rückkanal, keine vorschnelle Vollfreigabe

Die ursprüngliche fehlende Ausgabe ist korrigiert. `reportMidi()` legt die Hostqueue nur bei Bedarf an und meldet den selektierten Wert mit dem Eventoffset. Der Audiothread greift dafür nicht auf die View zu und ruft weder `beginEdit` noch einen blockierenden eigenen UI-Kanal auf. Eine Hostquelle am gleichen Offset verdrängt die MIDIquelle bereits in der Timeline; damit wird für den verdrängten MIDIpunkt keine falsche Ausgabe generiert. Ohne laufenden Transport durchläuft der normale positive Audioblock ebenfalls die Auswahl-/Ausgabetimeline. Das ist eine fachlich passende Verbesserung.

Die Prüfung [Host:1176](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1176) ist jedoch eine **explizite Testhost-Simulation**: Sie liest den gemeldeten Wert und ruft selbst `ctrl->setParamNormalized()` auf. Sie beweist keine automatische Weiterleitung durch Studio One und keine tatsächliche sichtbare A–H-Markierung oder anschließende Stepbearbeitung. Der Controllerpfad kann die View invalidieren [C:50](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L50), aber es wird im Test keine angehängte native View bedient.

**Noch nötige Gegenfälle:** MIDI24…31,0/letzter Frame, mehrere MIDIwerte, gleiche Offsets mit mehreren Hostwerten; MIDI-B@64 gefolgt von Host-A@128 und die umgekehrte Reihenfolge; Ende des Blocks und gespeicherte Auswahl. Die Ausgabe enthält derzeit nur MIDIwerte, keine nachfolgenden Hostwerte. Je nach Hostzusammenführung muss vermieden werden, dass eine später an die UI weitergereichte ältere MIDIausgabe die neuere Hostauswahl überschreibt. Das ist **ein konkreter erforderlicher Integrationstest, kein hier nachgewiesener Studio-One-Fehler**. Hostseitige Kenntnis der eigenen Automation darf nicht ignoriert werden; blindes Echo sämtlicher Hostpunkte ist ebenfalls keine ohne Prüfung zu empfehlende Reparatur.

`addParameterData()` kann fehlschlagen, `addPoint()`-Erfolg wird ignoriert [P:570](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L570). Ein `nullptr`-Ausgabekanal ist zulässig behandelt, erzeugt aber keinen späteren Nachholabgleich. Dafür sind Hostvertrag, Retry-/Endwertstrategie und Negativtests erforderlich. Es wird daraus ohne getesteten Hostfehler kein weiterer bestätigter A01-FAIL konstruiert.

**Erforderlicher End-to-End-Test:** echte verbundene Instanzen mit Hostbrücke, aktuelle GUIauswahl und nächstes Editziel, hörbare verschiedene Patterns, gespeicherter Prozessorwert und Controllerparameter. Die Bridge muss die tatsächliche zeitliche Priorität prüfen und darf nicht nur den gewünschten Einzelwert manuell setzen.

### 3.2 A02: Overflow und Reihenfolge tatsächlich anders gelöst

Die Timeline reserviert `maxSamplesPerBlock` Punkte **in `setupProcessing()`**, nicht in `process()`. Jeder Punkt enthält Pattern und Quelle. Nach Eingabe aller Quellen wird über die Sampleoffsets aufsteigend iteriert. Damit ist eine zusätzliche sortierte Ereignisliste nicht nötig. Mehrere Änderungen desselben Parameters am selben Frame können auf den letzten gültigen Gewinner reduziert werden: Für ein diskretes Pattern kann zwischen zwei Ereignissen am selben Sample kein weiteres Sample hörbar werden. `host||point.source!=kHost` erhält die Hostpriorität auch dann, wenn MIDI nachträglich eingefügt wird.

**Unabhängiger Vergleich:** Für200 deterministische Fälle mit Blockgrößen1/32/256/512/1024, Eingabemengen0/1/255/256/257/512/2048/10000, ungültigen Offsets/Patterns und beliebiger Quellenmischung wurde die Timeline gegen einen anders formulierten Referenzentscheid verglichen: pro Offset gültige Ereignisse gruppieren, nach Quellenpriorität und Eingabereihenfolge den Gewinner bestimmen. Keine Abweichung im Modell. Das ursprüngliche257-Punkte-Beispiel behält alle257 Gewinner. Sourceguards prüfen die Bindung dieser Logik an den aktuellen Processor und den Wegfall der alten Verwerfungsstelle.

**Aktuelle Testaussage:** `PatternEventTimelineTests.cpp` [TimelineT:7](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PatternEventTimelineTests.cpp#L7) prüft257 verschiedene MIDIoffsets als Helfer; [TimelineT:14](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PatternEventTimelineTests.cpp#L14) prüft2048 Änderungen **am selben Offset**, nicht2048 verschiedene Zeitpunkte. Der Pakettest [Host:1181](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1181) erzeugt250 Hostpunkte an0…249 und acht MIDIpunkte an0…7. Die acht MIDIpunkte kollidieren mit Hostpunkten und werden durch diese verdrängt:258 Rohereignisse,250 Timeline-Gewinner. Das ist ein sinnvoller Nachweis gegen den alten Rohereignis-Cap, aber kein Paketnachweis für257 tatsächlich verschiedene Patternintervalle.

**Noch nötig:** echter Processor mit512/1024 Frames,257/512 wechselnden Gewinnern, Stateendwert, mehrere hörbare Grenzsignaturen, gleiches Ereignisband in verschiedenen Blockpartitionierungen. Callbackallokationen und Deadlineverhalten messen. Bei `numSamples>maxSamplesPerBlock` wird korrekt ein Fehler statt Reallokation gewählt [P:533](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L533); ein Host muss den zuvor vereinbarten Maximalwert einhalten. Speicherallokationsfehler beim Setup separat prüfen.

**Leistungsgrenze:** Die neue Timeline hat begrenzten Speicher proportional zur vereinbarten maximalen Blockgröße, aber Scanaufwand proportional zur aktuellen Blockgröße. Viele Gewinner erzeugen viele Schedulersegmente samt kohärenten Patternkopien. „Kein256-Limit mehr“ und „keine eigene Timeline-Allokation im Callback“ sind belegbare Eigenschaften; „jede gültige Eventdichte hält sicher die Audio-Deadline“ ist damit nicht bewiesen.

### 3.3 A03–A06 und A09: konkrete Korrekturen anerkennen

- **A03:** Der alte Rechenfehler ist beseitigt.0,50,100,200 und alle Zwischeninteger ergeben100. Es gibt keinen Grund, den alten „Ctrl macht0“-Befund weiterzuführen.
- **A04:** Der neue Listener schließt die ursprüngliche Lücke, weil das SDK zuerst seinen Textabschluss erledigt und dann `onTextEditPlatformControlLostFocus` unabhängig von einer Textänderung aufruft. Listener wird bei `removed()` abgemeldet [V:275](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L275). Der neue Recallrest RC03 betrifft die Bindung der ausstehenden Bearbeitung an eine Kitgeneration, nicht denselben unveränderten Escapezweig.
- **A05:** Der tatsächliche View gibt sich über `getDropTarget()` zurück [VH:33](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.h#L33); die operative Ereigniskette endet in den neuen Viewprüfungen. Die daneben noch vorhandene Adapterklasse ist keine notwendige zweite operative Registrierung. Ihre bloße Existenz wäre kein Beweis für D&D; geprüft wurde der echte SDK-/Viewweg. Der funktionierende native Windows-CMake-Patch bleibt vorhanden und wurde durch diese Reparatur nicht neu erfunden. VSTGUI wandelt die Koordinaten im Container unter Berücksichtigung seiner inversen Transformation um; ein zusätzlicher pauschaler Zoomteiler wäre keine gerechtfertigte Reparatur.
- **A06:** Bei357px und offener Lane passt jetzt eine Row; deren Status endet logisch bei346, innerhalb357. Bei300px passen drei geschlossene Rows, Statusende294. Bei Höhe unter dem Minimum eines vollständigen Footer-/Rowlayouts liefert der Helfer0; die View erzwingt dann mindestens eine Row. Dieser außerhalb des unterstützten300px-Minimums liegende200px-Fall ist kein neu bewiesener Fehler eines legalen Hostfensters. Tatsächliche Clientgröße, effektive Skalierung und Rückrollback bleiben nativ zu prüfen.
- **A09:** `(167,290)` und die Längengaps werden nicht mehr zu mutierenden Songaktionen. Der Footerleerklick x1019 erzeugt keine falsche Resetbestätigung mehr. Bestehende größere Zelltreffflächen für32 Steps/64 Songzellen sind von den explizit gezeichneten diskreten Buttonabständen zu unterscheiden.

## 4. Neu bestätigte Fehler und erforderliche Reparaturen

Alle folgenden Nachweise sind **SOURCE-CONFIRMED** mit expliziter gültiger Aufrufreihenfolge. Die Zustands-/SDK-Abfolge ist nachvollziehbar; ein hier nicht durchgeführter nativer DAWlauf wird nicht behauptet.

### RC01 – HOCH: Aktiver Velocity-Stroke überlebt modernes Kit-Recall und kann neue Patterns überschreiben

**Betroffene Stellen:** modernes Recall [C:135](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L135)–[C:160](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L160), nur im Legacyzweig vorhandenes Invalidieren [C:187](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L187), Strokeabschluss [C:463](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L463)–[C:471](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L471), View-Cancel/Remove [V:273](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L273), [V:1043](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L1043), MouseUp [V:1036](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L1036).

**Einordnung:** Neue Regression der A07-Implementierung. Der normale Drag-Undo-Fehler ist korrigiert, die neue transiente Strokehistorie wird aber nicht in allen Statepfaden invalidiert.

**Technische Ursache:** `beginVelocityStroke()` hält `strokePattern_` und eine komplette512-Byte-Patternkopie `strokeBefore_`. Beim DM4–DM15-Recall werden `grid_`, Namen, Längen und Undo ersetzt/resettiert. `strokePattern_`/`strokeBefore_` bleiben jedoch aktiv. Der Kommentar „A recalled kit invalidates an in-progress GUI stroke“ steht ausschließlich im nachfolgenden Legacyzweig. `refreshFromHost()` verwirft auch die aktive Viewgeste nicht. Ein späteres `endVelocityStroke(false)` schreibt den alten kompletten Snapshot mit `PatternReplace` in den aktuell geladenen Prozessor; `endVelocityStroke(true)` legt den alten Snapshot als neue Undohistorie an.

**Konkrete Reproduktion:**

1. KitX: PatternA enthält etwa Step1=20 und Step11=64. Velocitygeste beginnen, Step1 auf80 ziehen, Maustaste noch halten.
2. Host ruft erfolgreich `setState`/`setComponentState` für KitY auf, während derselbe Editor bestehen bleibt. KitY hat A: Step1=90, Step11=115. Das kann als serialisierte UI-/Hostcallbackfolge passieren; es braucht keinen ungeschützten gleichzeitigen C++-Zugriff.
3. **Cancel oder Editor schließen:** `onMouseCancel` beziehungsweise `removed` ruft `endVelocityStroke(false)`; das gesamte PatternA aus KitX wird in KitY geschrieben. Auch der gar nicht im Stroke bearbeitete Step11 wird115→64 überschrieben.
4. **Alternativ MouseUp, dann Undo:** Der Strokeabschluss erfasst als Undo-Vorzustand weiter KitX. Nach einem bewusst leeren Undo beim Recall entsteht erneut eine Historie, die Daten des vorherigen Kits zurückschreibt.

**Nachweis:** Im nachvollzogenen Modell: neues Kit `[90,115]`; nach Cancel beziehungsweise Commit+Undo **`[20,64]`**. Im Produkt sind exakt die hierfür erforderlichen fehlenden Invalidierungen und der erfolgreiche Bulk-Nachrichtenpfad vorhanden. Keine Sampledatei wird dafür beschädigt; betroffen sind Pattern-/Projektinhalte.

**Empfohlene Reparatur:** Alle erfolgreichen Komponentenzustandsübernahmen müssen laufende Gesten **verwerfen**, ohne den alten Snapshot auf das neue Kit zurückzurollen. Controller- und Viewseite gemeinsam invalidieren, etwa über eine Kit-/Editgeneration oder einen expliziten gültigen Gestentoken. MouseMove/Up/Cancel/Remove dürfen nach Recall keine Edits oder Historie der alten Generation mehr anwenden. Nur `strokePattern_` im modernen Zweig zurückzusetzen genügt nicht vollständig: `laneDragActive_` und `laneDragPattern_` bleiben sonst aktiv und ein späterer Move kann neue Daten weiterbearbeiten.

**Erforderliche Regression:** Tatsächlicher Controller/Prozessor plus View oder Eventfixture: Stroke inKitX → modernes DM15-RecallKitY → je Move/MouseUp/Cancel/Editorclose/Undo. Patternbytes, Audio und gespeicherten State vonKitY exakt vergleichen; besonders einen unberührten Step unterschiedlich belegen. Dieselbe Generationstrennung mit Legacy-Recall und abgelehntem/defektem Recall prüfen: ein fehlgeschlagener Recall darf die gültige alte Geste nicht unbemerkt beschädigen. Keine bloße isolierte `PatternUndo`-Modellprüfung.

### RC02 – MITTEL: Geerbtes VSTGUI-Zoommenü umgeht Präferenz und Resizeprüfung; A08-Frühabbruch konserviert die Abweichung

**Betroffene Produktstellen:** Aktivierung der zulässigen Zoomfaktoren [C:200](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L200)–[C:205](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L205), Präferenzsetter [CH:88](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.h#L88), eigene Buttonbehandlung [V:813](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L813), State-Frühabbruch [C:83](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L83)–[C:95](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L95), geprüfter Editorpfad [E:41](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorEditor.h#L41). **Gepinnter SDK-Code:** [SDKEditor:859](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L859), [SDKEditor:600](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L600), [SDKEditor:1574](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L1574), [SDKDelegate:99](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.h#L99), [SDKFrame:248](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/cframe.cpp#L248).

**Einordnung:** Bereits vorhandener zusätzlicher Zoom-Eingabepfad, der von der A08-Reparatur nicht abgedeckt wird; nicht als ausschließlich durch `dbd9106` eingeführte Menüfunktion darstellen. Der neue Gleichheitsfrühabbruch lässt einen gültigen Recall diese bereits mögliche Abweichung überspringen.

**Technische Ursache:** `setAllowedZoomFactors({1.0,1.5})` aktiviert im regulären VSTGUI-Editor das Rechtsklickmenü „UI Zoom“. Dessen SDK-Kommando ruft direkt `VST3Editor::setZoomFactor()` auf, **nicht** `DrumatorEditor::setZoomChecked()` und **nicht** `Controller::rememberZoom()`. Der SDK-Callback `onZoomChanged` bleibt im aktuellen Controller der leere Default. `setZoomFactor()` ignoriert das Ergebnis von `CFrame::setZoom()`; bei Ablehnung setzt der Frame seinen Transform zurück, der Editorcache kann auf dem angeforderten Faktor bleiben. Der neue Controllerstate-Zweig prüft zunächst `requested==preferredZoom_` und kehrt dann zurück, ohne einen Editorzustand anzusehen.

**Konkrete Reproduktion ohne Mehrfach-View:**

1. Controllerpräferenz100%, bestehender Editor100%.
2. Über das tatsächlich vom SDK angebotene Rechtsklickmenü150% auswählen, Host akzeptiert. Editor/Frame150%, gespeicherte Präferenz weiterhin100%.
3. Controllerstate mit100% laden. `requested==oldZoom` ist wahr; `setState()` bestätigt Erfolg, der offene Editor bleibt150%.
4. Zusätzlich geht beim Speichern/Schließen/Wiederöffnen die über das Menü getroffene150%-Auswahl verloren. Bei Hostablehnung des Menüzooms ist auch die aus N02 bekannte Cache-/Transformabweichung wieder möglich, obwohl die eigenen Headerbuttons geprüft werden.

Ein weiterer möglicher Einstieg ist ein zweiter offener Editor: Zoom der ersten View ändert die gemeinsame Präferenz, nicht automatisch die zweite. Ein Recall mit derselben Präferenz wird dann ebenfalls vorzeitig übersprungen. Dieses Mehrfach-View-Verhalten muss ausdrücklich definiert werden; das Ein-Editor-Menügegenbeispiel benötigt diese Zusatzannahme nicht.

**Nachweis:** Gelesener SDK-Kommandopfad und fehlender Produkt-Override; Modell `preferred=1.0`, `editor=1.5`, geladen1.0 → Early Return, Editor bleibt1.5. Kein tatsächlicher Kontextmenüklick wurde hier ausgeführt. Für den Resizeablehnungsfall ist der SDK-Rollback in `CFrame::setZoom` exakt vorhanden, kein vermutetes SDK-Verhalten.

**Empfohlene Reparatur:** Sämtliche angebotenen Zoomwege auf dieselbe geprüfte Produkttransaktion führen; SDK-Menükommando passend abfangen oder dessen Zoomhandling bewusst vereinheitlichen. Erfolgreiche echte Änderungen müssen die gespeicherte Präferenz konsistent aktualisieren; Ablehnung muss Cache/Frame/Präferenz erhalten. `onZoomChanged` allein naiv zu verwenden ist nicht ausreichend, weil der Basispfad den Callback auch nach einem fehlgeschlagenen Framezoom auslösen kann. Im State-Recall tatsächliche betroffene Editoren prüfen statt allein Präferenzgleichheit als Beweis zu verwenden. Rollbackfehler nicht ignorieren.

**Erforderliche Regression:** Angehangener Editor, echte SDK-Kontextmenükommandos100/150%, erfolgreicher und abgelehnter Hostresize; danach DZM1-State mit gleicher und anderer Präferenz laden, getState/getSize/Transform/Markierung/Reopen prüfen. Mehrfach-View und Ablehnung einer späteren Rückrollback-Anfrage ergänzen. Der aktuelle Preattach-DPI-Smoke prüft keinen dieser Wege.

### RC03 – MITTEL: Nicht abgeschlossene Namensbearbeitung ist nicht an das recalled Kit gebunden

**Betroffene Stellen:** Namensbeginn/Commit [V:282](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L282)–[V:304](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L304), Refresh nach Recall [V:323](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L323), Stateübernahme/Benachrichtigung [C:135](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L135)–[C:160](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L160), Renameannahme [C:229](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L229), Prozessor [P:152](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L152)–[P:164](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp#L164).

**Einordnung:** Neu entdeckter verbleibender Lifecyclefehler im Umfeld A04, nicht nachweislich erst durch die Listenerreparatur eingeführt. Die ursprünglichen Escape-/unveränderten-Enter-Abschlüsse sind korrigiert; dieser andere Eingang darf dabei nicht übersehen werden.

**Technische Ursache:** Der Namenseditor hält `editingNameRow_` und seinen bisherigen Text im Frame-Child. Eine erfolgreiche Komponentenzustandsübernahme ersetzt die Controllerdaten und ruft `notifyViews`/Relayout auf. `refreshFromHost()` invalidiert aber weder das Textfeld noch dessen ausstehende Bindung an das alte Kit. Der spätere `valueChanged()` prüft nur dieselbe Control und `editingNameRow_<16`; `renameRow()` und der Prozessor akzeptieren ebenfalls jede Row0…15. Es gibt keinen Abgleich mit einer Kitgeneration.

**Konkretes Fehlerszenario:** Name aufRow1 inKitX beginnen und einen neuen Text eintippen, ohne ihn abzuschließen. Host lädt programmgesteuert KitY in derselben offenen Instanz, ohne vorher nativen Fokusverlust zu erzwingen; gleiche Zeilenzahl/Höhe ist ausreichend. Der Feldinhalt und die EditingRow bleiben aufKitX gebunden. Beim späteren Enter/Fokusverlust wird der alte ausstehende Text jetzt als **Rename fürKitY** gesendet und mitgespeichert. Variante: zuvor sichtbareRow8 bearbeiten, danach KitY mit vierRows laden; spätere Eingabe kann sogar die jetzt versteckteRow8 umbenennen, da nur die16-Slot-Grenze geprüft wird.

**Nachweis:** Fehlender Abschluss im Refresh-/Recallpfad und gültiger späterer Renamepfad sind im Quellcode bestätigt. Das Szenario setzt einen Recall bei weiter bestehendem nativen Editfokus voraus; eine solche Callbackreihenfolge ist zulässig und wurde hier **nicht in Studio One ausgeführt**. Es wird nicht behauptet, jeder manuelle Presetklick halte den Fokus fest: Ein vorheriger natürlicher Fokusverlust kann den Gegenfall verhindern.

**Empfohlene Reparatur:** Bei erfolgreichem Kit-/Komponentenstatewechsel ausstehende Namensbearbeitung gezielt verwerfen oder generationengeprüft abschließen, bevor sie auf neue Inhalte wirken kann. Keine späte native Benachrichtigung darf alte Eingaben auf das neue Kit anwenden. Nicht bei jeder gewöhnlichen Parameterautomation pauschal alle Textfelder abbrechen. Gewünschten Recall-/Abbruch-/Dirtyvertrag festlegen.

**Erforderliche Regression:** Tatsächlich angehängtes `CTextEdit`: Textedit offen → erfolgreicher moderner Recall mit gleicher/kleinerer Rowzahl → Enter/Escape/Fokusverlust. Neue displayNames und Statebytes bleiben ohne neue bewusste Bearbeitung unverändert; kein Rename-/Dirtyevent aus der alten Generation. Zusätzlich abgelehnter Recall, Schließen und Listenerabmeldung. Der reine Text-/Kitcodec-Test reicht nicht.

### RC04 – NIEDRIG: Overlayablehnung meldet fälschlich einen fehlenden Dateipfad

**Betroffene Stelle:** [V:237](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L237)–[V:244](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L244), insbesondere [V:239](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L239)–[V:242](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L242). **Einordnung:** Diagnosefehler, durch die neue gemeinsame Prüfung in A05 eingeführt; die eigentliche Overlayblockierung ist dabei korrekt.

**Ursache/Szenario:** `hasPath` enthält jetzt sowohl Zielzulässigkeit als auch Dateipfaderkennung. Bei einem gültigen WAV-/FLAC-Payload auf dem geöffneten Song-/Fillbereich scheitert bereits die Zielprüfung; die Pfadextraktion wird wegen Kurzschluss nicht ausgeführt. Der falsche Zweig meldet dennoch ausdrücklich „EREIGNIS EMPFANGEN, ABER KEIN WAV/FLAC-DATEIPFAD“. Der gültige Pfad ist nicht die Ursache. Das kann weitere Fehlersuche gerade bei Browserdrops in die falsche Richtung lenken. Der Status kann je nach Overlay verdeckt sein; dies ändert den falschen Diagnosewert nicht.

**Nachweis:** `targetAllowed=false`, gültiger Dateipayload, daraus `hasPath=false` und die genannte Meldung. Die Verzweigung ist direkt im Quellcode nachvollziehbar; es wird damit kein erneuter Fehler des nativen CF_HDROP-Patches oder ein durchgelassener Import behauptet.

**Empfohlene Reparatur/Regression:** Zielzulässigkeit und erkannten Pfad getrennt bewerten; bei Overlay/Außenbereich einen zutreffenden Ablehnungsgrund liefern, bei ungültiger Datei den fehlenden/ungeeigneten Pfad melden. Test mit demselben gültigen Payload im freien und verdeckten Bereich sowie mit tatsächlich fehlendem Pfad; Rückgabewert und Diagnose getrennt prüfen. Kein zusätzlicher Importversuch beim abgewiesenen Ziel.

## 5. Vorhandener Windows-Build #39335925418

Unabhängig abgerufen wurden die öffentliche [Runseite](https://github.com/challanger2000/125A-Drumator/actions/runs/39335925418) und die [Windows-Jobseite](https://github.com/challanger2000/125A-Drumator/actions/runs/39335925418/job/118345283080).

| Aussage | Was dieser Auditor tatsächlich sehen konnte |
|---|---|
| Run gehört zum beauftragten Commit | `dbd9106`, Branch `develop/v1.0.0`, passender Commitbetreff, Workflowlauf306. |
| Gesamtergebnis | **Success**, Windowsjob **succeeded**,19.10.2026; Run3m4s, Job3m1s, ein Artefakt. |
| Vollständige Einzellogs | **Nicht unabhängig lesbar** über die öffentlichen Seiten: „Sign in to view logs“. Keine Umgehung und keine erfundenen Transkripte. |
|15/15 CTests,47/47 Validator, Paket-Smokes | Im Reparaturbericht/Engineering gemeldet. Der exakt zugehörige Workflow ruft CTest, Validator und beide Smokes mit Fehlerprüfung auf; der beobachtete Erfolg ist unterstützende Evidenz. Die konkreten15-/47-Einzelausgaben wurden hier nicht neu ausgelesen. |
| Eigener erneuter Build/Test | **Nicht ausgeführt.** Keine neuen Actions, kein Windowsbundle installiert oder bitgenau vermessen. |
| GUI-/DAW-/Deadlinefreigabe | **Nicht** durch Run-Success abgedeckt. |

Der Quelltext registriert jetzt15 CTests: **14 reguläre Tests plus ein absichtlicher SongChain-Negativtest mit `WILL_FAIL`**. Die neue Timeline- und Overlay-CTestquelle verwendet stets aktive `CHECK`s. Der CMake-Suitetarget sammelt über den Namensfilter auch diese neuen Tests ein [Build:129](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/CMakeLists.txt#L129). Ein im Release durch `NDEBUG` leerer neuer Test wurde hier nicht gefunden. Editor- und Audio-Host-Smoke sind separate Aufrufe, nicht zusätzliche native GUI-Interaktionstests innerhalb der15.

Die sichtbare Jobannotation betrifft die Node.js-Laufzeit der verwendeten Actions. Sie ist kein nachgewiesener Produktcompilerfehler und kein Grund, ungefragt Workflows umzubauen. Die Produkt-/Testfehler dieses Berichts werden nicht aus dieser Annotation abgeleitet.

## 6. Kritische Bewertung der neuen und unveränderten Regressionen

### 6.1 Neue Tests: was sie wirklich tun

| Test / Zeilen | Geeigneter Nachweis | Noch nicht abgedeckt |
|---|---|---|
| `PatternEventTimelineTests.cpp:6–19` |257 MIDIoffsets als Helfer, Host/MIDI-Tie, letzter Hostwert,2048 gleiche Offsets, Reset und ungültige Inserts. | Tatsächlicher Processor mit mehr als256 gewinnenden Sampleoffsets, alternierende hörbare Intervalle, vollständiger Endstate, Callbackallokationen, negative Setup-/Outputqueuefehler. |
| `PackagedAudioHostSmoke.cpp:1160–1179` | Echter Prozessor liefert B@64 in der richtigen VST3-Outputqueue; echter Controller akzeptiert manuell weitergereichten Wert. | Hostautomatik, offene GUI/Redraw/Editziel, mehrere Rückmeldungen und spätere Hostautomation, State-/Timingvergleich am Blockende. |
| `PackagedAudioHostSmoke.cpp:1181–1205` |258 Rohereignisse überschritten den alten gemeinsamen Cap; nachfolgendes B ist hörbar. | Nicht258 verschiedene Offsets; nur250 Gewinner. Keine512-/1024-Frame-Reihe, keine Assertion des final gespeicherten Patternwerts in diesem neuen Block. |
| `UiOverlayHitZonesTests.cpp:6–10` | Fünf sinnvolle Helferpunkte für normalen Drop, Song, Fill, freie Fläche daneben und untere Gridkante. | Keine echte View, kein `IDataObject`, WAV/FLAC-Datei, Importannahme/-fehler, Compact-Rownummer oder100/150%-Transform. |
| `VisibleRowsTests.cpp:52–61` | Rechnerische Pagegrößen für300/357/792/200 Pixel. | Keine Host-/Frame-/Childclientmessung, tatsächliches Lane-Schließen/Paging oder Resizeablehnung. |
| A03/A04/A07/A08/A09 | Änderungen im Produkt nachvollziehbar. | **Keine neuen passenden nativen Event-/Stroke-/Recall-/Menütests.** Windowskompilation beweist die neue API-/Listener-Verwendung, nicht die Bediensemantik. |

Eine Textsuche in den aktuellen Tests findet keine direkten Aufrufe von `beginVelocityStroke`, `endVelocityStroke`, `applyZoomFromState` oder den neuen Textedit-Fokuscallbacks. Der Controllerzoom-Test [Host:128](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L128) führt seinen Roundtrip aus, bevor eine View angehängt ist. `EditorGeometrySmoke` bleibt ein Factory/getSize/Preattach-DPI-Test; er ist kein angeschlossener Editortest mit einem ablehnenden `IPlugFrame`.

**Wichtige Präzisierung:** Die neuen Timeline- und Pakettests sind nicht wertlos. Sie treffen den alten Overflow-/fehlenden-Ausgabe-Fehler tatsächlich besser als frühere Tests. Ihre behauptete Reichweite muss trotzdem auf das tatsächlich ausgeführte Szenario begrenzt bleiben. Die GUIreparaturen besitzen überwiegend Kompilations- und Quellnachweise, keine umfassende Interaktionsabdeckung.

### 6.2 Alte Testschwächen T01–T04 weiterhin vorhanden

Die neun Reparaturen ändern die im unabhängigen Gesamtaudit beschriebenen vier Orakellücken nicht:

- **T01 / MITTEL:** [Host:933](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L933) prüft für DM15-Datenerhalt weiter die lokale Fixture `patternKit.grid`, nicht die tatsächlich recalled Pluginantwort. Prozessorstate zurücklesen oder die wieder eingeblendete zweite Hälfte hörbar messen.
- **T02 / MITTEL:** [Host:1535](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1535) akzeptiert noch±0,03 bei Pegelabstand0,015 benachbarter AUX-Ziele. Ein benachbarter Busswap kann unentdeckt bleiben. Unabhängige Stereo-/Zeit-Signaturen und sinnvoll begründete Toleranz verlangen.
- **T03 / MITTEL:** [AtomicT:190](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/AtomicPatternTests.cpp#L190) verlangt weiterhin keine positive Mindestanzahl `verified` im konkurrierenden Snapshottest. Deterministische Barrieren und tatsächlich beobachtete kohärente alte/neue Generationen ergänzen.
- **T04 / HOCH für behauptete Restore-Atomizitätsfreigabe:** [Host:1544](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1544) stellt sechsmal denselben State wieder her und fordert endliche Ausgabe/positive Blockzahl [Host:1587](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp#L1587). Durchgehende Nullen oder endliche unzulässige Mischgenerationen können bestehen. Zwei deutlich unterschiedliche Kits, Automation, erzwungene Überlappung und zulässige pro-Block-Gesamtsignaturen prüfen.

Das sind bestätigte **Testschwächen**, keine neu behaupteten realen Busvertauschungen oder nachgewiesenen Audio-Races. Sie gehören zur weiteren technischen Absicherung, auch wenn A02/A03/A09 als konkrete Logikfehler geschlossen werden können.

### 6.3 Noch erforderliche Gegen-/Negativtests

1. **RC01 höchste Priorität:** tatsächliche Controller-/Prozessornachrichten mit begonnener Geste, modernem Recall und späten Move/Up/Cancel/Remove/Undo. Nicht nur `PatternUndo` separat prüfen.
2. **RC02:** geerbte Rechtsklick-Zoomkommandos einschließlich gleicher Präferenz, nicht nur die Headerbuttons und nicht nur Preattach-DPI.
3. **RC03:** echtes native Textfeld mit weiterhin offenem Fokus über Recall, gleiche/kleinere Rowzahl, Abbruch ohne Rename/Dirty des neuen Kits.
4. **A01:** Blockendzustand von Audio/Prozessorstate/Controller/View gegen gemeinsame erwartete Ereigniszeitachse, Host vor/nach MIDI und gleicher Offset; volle/fehlende Ausgabequeue, abgewiesenes `addPoint`, Editor zu/offen. Keine UI-Arbeit in den Callback einführen.
5. **A02:** Produktblock512/1024 mit257/512 verschiedenen Gewinnern und hörbarem Onsetorakel; äquivalentes Ereignisband mit variabler Blockpartitionierung.255/256/257-Grenzfälle bei mehreren Quellmischungen, Endwert speichern/recallen.
6. **A05/A06:** Native Windowsfixture mit HWND/Attach, kontrolliert annehmendem/ablehnendem `IPlugFrame`,100/150%, zusätzlichen DPI-Faktoren,1/4/8/16 Rows, Lane an/aus und Compactpaging. Echte Datei-Drops in freie/überdeckte Bereiche; jeder abgelehnte Drop erhält Samplebytes und Audio.
7. **A07/A08 Fehlerpfade:** abgelehnte Nachrichten, Cancel-Rollbackabweisung, bereits laufende Geste, Recallfehler, spätes Text-/Mausevent; Mehrfach-Views und fehlgeschlagener Rückrollback dürfen keinen stillen Erfolg vortäuschen.
8. **A03/A09 echte Eingabeprüfung:** Strg+Links auf beide Mastertasten und negative Gapklicks am wirklichen Dispatcher; Anzeige, Dirty, State und Audio gemeinsam beobachten. Quellcode-PASS nicht mit nativer Durchführungsbestätigung verwechseln.

## 7. Weiterhin offene Risiken: ausdrücklich keine neuen bestätigten Produktfehler

### O01 – Gleichwertige/ABA-Hostautomation gegen Restore

**Priorität HOCH für die Verifikation; Status OFFEN.** Der frühere Gesamtaudit-R01 bleibt unverändert. `AtomicPatternSelection::store()` erhöht die Generation im normalen Audiozweig nicht; `hostSubmit()` im Busyzweig tut es. Restore liest vor Eintritt des Gates einen Snapshot. Gleiche-Auswahl-/ABA-Events zwischen Snapshot und Gate müssen an **diesem tatsächlichen Produktpfad** mit Barrieren und festgelegter Linearisation geprüft werden. Die beiden Reparaturcommits ändern diese Klasse nicht. Weder der alte unbenutzte `PatternRestoreLatch` noch der neue MIDI-Outputkanal schließen diesen Fall. Ohne deterministische Produktreproduktion wird daraus hier kein zusätzliches FAIL konstruiert.

### O02 – Realtime, Kit-Busy-Stille und Timelinekosten

**Priorität HOCH für V1; Status OFFEN.** Das bestehende Sample-/Assetmodell vermeidet eine offensichtliche neue Timelineallokation im Callback; neue Vorallokation läuft beim Setup. Lockfreiheit ist weiterhin nicht gleich Wait-Freiheit: die Auswahl-CAS-Schleifen besitzen keine feste Versuchsgrenze. Kitrestore bereitet während des Busy-Fensters Assets vor; der gesperrte Audiopfad gibt Nullen aus und verarbeitet Hostauswahl, aber keine MIDI-Noten. Maximale Stummdauer, Notenpolitik und Verhalten großer Kits sind keine durch Success bewiesene unterbrechungsfreie Wiedergabe.

Der bestehende Voice-Test [VoiceT:323](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/SampleVoiceTests.cpp#L323) druckt p95/p99/max für einen unpaced Corelastlauf, fordert aber nur `elapsed<20s` [VoiceT:332](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/SampleVoiceTests.cpp#L332). Bei48kHz/256 Frames beträgt die Audiodeadline5,333ms. Es wurden hier keine aktuellen p95-/p99-/Maxwerte unabhängig eingesehen oder neu gemessen. Eine großzügige Gesamtlaufzeitgrenze und8192 finite Hostblöcke bleiben Durchsatz-/Stabilitätsevidenz, keine garantierte Callbackdeadline.

**Erforderlich:** gepacete komplette Callbackmessung, Deadlineüberschreitungen/Dropouts/Allokationshooks, mehrere Sampleraten/Blockgrößen und Instanzen,16 Outputs, viele Auswahlpunkte, GUI-/Kitlast; CPU/Powerprofile und Instrumentierungsaufwand mitprotokollieren.

### O03 – UI-Rollback und transiente Bearbeitungen über mehrere Views

**Priorität MITTEL; Status OFFEN über RC01–RC03 hinaus.** [C:89](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp#L89) ignoriert das Ergebnis, wenn bereits geänderte Views auf den alten Zoom zurückgesetzt werden sollen. [V:317](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp#L317) ignoriert ebenfalls den Rückgabewert beim Rücksetzen und verändert über `adjustEditorHeight` den Compact-/Lanestatus. Ein tatsächlich ablehnender Rückrollback muss getestet werden; der Bericht behauptet keinen hier nativ erzeugten Dauerdesync dieses zusätzlichen Pfads.

Eine Strokehistorie liegt global im Controller, aktive Mouseflags liegen je View. Multi-View-Bedienung und Automation während einer fest auf ein Pattern gebundenen Geste benötigen einen klaren Vertrag: Welches Pattern zeigt die Oberfläche währenddessen, welche Instanz darf commit/cancel auslösen? Ein bloßer globaler Snapshot ist keine vollständige Mehrfenster-Transaktionsarchitektur. Nicht jede theoretische Folge wird deshalb bereits als neuer bestätigter Fehler gemeldet.

### O04 – Unveränderte Import-/Routing-/Ressourcengates

**Priorität MITTEL; Status OFFEN.** Der Gesamtaudit benannte außerdem unbekannte FLAC-Gesamtlänge/Spätkorruption, Peak-RAM statt bloßem16-/64-MiB-Dateibudget, Tempoloops/Locates, echte Multi-Out-Aktivierung und Owner-QA. Diese neun Reparaturen ändern weder FLAC-/WAVdecoder noch Routing-/DSPalgorithmen. Ihre Nichtänderung ist kein neuer Fehler, aber auch keine nachträgliche PASS-Bestätigung. Der ursprüngliche Gesamtaudit bleibt hierfür die Checkliste; eine V1-Freigabe benötigt gesonderte Evidenz.

## 8. Reproduzierbare Kurzgegenbeispiele und Ergebnisse

Die Hilfsprüfungen wurden außerhalb der Repositoryarbeitskopie gehalten und werden nicht als neue Tests committet. Die folgenden Modelle sind bewusst als **Python-/Ablaufmodelle** ausgewiesen. Die Produktregressionen sind anschließend an den echten C++-Pfaden zu ergänzen.

```python
# RC01: durch aktuelles modernes Recall nicht invalidierter Stroke
before_X = [0] * 512
before_X[0], before_X[10] = 20, 64
stroke_before = before_X.copy()       # beginVelocityStroke
kit_Y = [0] * 512
kit_Y[0], kit_Y[10] = 90, 115          # erfolgreicher moderner Recall
# strokePattern_ ist im modernen Zweig weiter aktiv.
cancel_result = stroke_before.copy() # end(false) -> PatternReplace
assert (cancel_result[0], cancel_result[10]) == (20, 64)
assert (kit_Y[0], kit_Y[10]) == (90, 115)   # erwarteter, aber verlorener Zustand

# RC02: Kontextmenü umgeht Produktpräferenz, gleicher State wird übersprungen
preferred, actual_editor = 1.0, 1.5
requested = 1.0
if requested != preferred:           # entspricht der aktuellen Frühabbruchlogik
    actual_editor = requested
assert actual_editor == 1.5          # geladen wurde100%, Editor bleibt150%

# A03: korrigierter Masterdefault
for current in range(201):
    assert max(0, min(200, current + (100-current))) == 100

# A06: korrigierter ursprünglicher357px-Lanefall
fit = min(4, int((357-108-220)/29))
assert fit == 1
assert 108 + fit*29 + 209 <= 357      # Status vollständig im Fenster

# A09: Gap zwischen A undB ist jetzt außerhalb der gezeichneten A-Breite
x = 167
index = int((x-115)/56)
assert index == 0
assert not (x < 115 + index*56 + 50)  # keine Songmutation
```

| Unabhängige Hilfsprüfung | Ergebnis |
|---|---|
|200 Timelinefälle gegen prioritäts-/reihenfolgegestützten Referenzentscheid | Alle Modellfälle konsistent mit der neuen Quelllogik. |
| Ursprünglicher257-Offset-Fall,512 Frames |257 Gewinner, letztes PatternB; keine Verwerfung des ganzen Blocks. |
| Masterstartwerte0…200 | Alle201 ergebenDefault100. |
| Compacthöhe300…850 × Lanezustände ×1…16 Rows |17.632 Fälle: rechnerisches Status-/Footerende innerhalb der angenommenen tatsächlichen logischen Höhe. |
| Songbuttonränder/-gaps, Viertelpixelschritte |3.080 Punkte: Dispatcherentscheidung entspricht den gezeichneten diskreten Pattern-/Längenrechtecken. |
| RC01 modernes Recall + Strokeabschluss | Neues `[90,115]` wird im Gegenmodell zu altem `[20,64]` überschrieben. |
| RC02 Kontextmenü + gleicher Controllerstate | Geladen100%, tatsächlicher Editor bleibt150%. |
| RC03 pendingName + Recall | EditingRow/Control bleiben gebunden; spätes gültiges Rename kann das neue Kit ändern. Native Fokusfolge noch auszuführen. |
| Tatsächlich neu ausgeführte C++-/Windows-/Studio-One-Tests | **Keine.** |
| Neu gestartete Actions | **Keine.** |

## 9. Konkrete Korrekturliste und Abnahmefolge

**Produktkorrekturen, die aus den bestätigten Befunden folgen:**

1. **RC01 / HOCH:** laufende Velocitygeste und Snapshot generationssicher bei jeder erfolgreichen modernen/Legacy-Stateübernahme verwerfen; späte Viewevents blockieren. Kein Zurückrollen der alten Geste ins neue Kit.
2. **RC02 / MITTEL:** SDK-Kontextmenü, Headerzoom und geladene Zoompräferenz über denselben geprüften, gespeicherten Zoompfad führen. Gleichheitsfrühabbruch darf inkonsistente Editoren nicht überspringen. Ablehnung und Reopen konsistent behandeln.
3. **RC03 / MITTEL:** Namenseditor an die Kitgeneration binden; bei erfolgreichem Recall alte ausstehende Eingaben beenden/verwerfen und späte Commitcallbacks abweisen.
4. **RC04 / NIEDRIG:** Overlay-/Zielablehnung von tatsächlich fehlendem Dateipfad in der D&D-Diagnose trennen.

**Verifikation und gegebenenfalls davon abhängige weitere Korrekturen:**

5. Produktregressionen fürRC01–RC04 und A01-Hostbrücke/A02-dichte Gewinner einführen. Die vier alten Testorakel T01–T04 stärken. Dabei bestehende Tests nicht lockern oder bloß passend zur Implementierung nachprogrammieren.
6. Tatsächlich angehängte Windows-GUI testen, insbesondere Namefokus, vollständige Velocitygeste, ursprüngliche und geerbte Zoomwege, Hostresize-Ablehnung, Compact, Drop und Buttonlücken. Erst danach native GUI-PASS vergeben.
7. O01 mit deterministischen Produktbarrieren und definierter Ereignispriorität entscheiden. Erst bei geklärtem Vertrag/Resultat eine gezielte Synchronisationsänderung beauftragen. Keine spekulative erneute Latchvariante.
8. Gepacete Realtime-/Ressourcenmatrix, echte Studio-One-Routing-/Transport-/Recallpraxis und Ownerabnahme abschließen; exakte Bundle-/Commitprovenienz dokumentieren.

**Kein bereits bestätigter weiterer Reparaturauftrag** folgt allein aus dem gemischten MIDI-/Host-Rückmeldungsrisiko oder einem theoretischen Rückrollback. Diese Fälle benötigen zuerst die genannten Tests. Ebenso ist eine erneute Änderung der als PASS eingegrenzten A02-/A03-/A09-Logik ohne neuen Nachweis nicht angezeigt.

### 9.1 Letzte Studio-One-/Studio-Pro-Prüfungen

- MIDI-A–H mit geöffnetem Editor und laufender Automationsspur: hörbare Auswahl, Parametertext, Highlight, nächster Stepedit und Speichern stimmen überein; Host/MIDI vor/nach/gleichzeitig.
- Velocitygeste halten, programmatisch anderes Kit recallen, dann weiterbewegen/loslassen/abbrechen/Editor schließen: keine alte Generation verändert neues Kit. Ein normales Drag bleibt genau ein Undo.
- Nameedit mit geändertem/unverändertem Text: Enter, Escape, Klick außerhalb, Recall bei erhaltenem Fokus, Rowzahländerung, Reopen. Keine alte Eingabe im neuen Kit.
- Beide Headerzoomtasten **und** Rechtsklick-„UI Zoom“ verwenden; anschließend gleicher/anderer Controllerstate, Projektneustart, Resizeablehnung,100/150% und DPI-/Monitorwechsel.
- Drag WAV/FLAC aus Explorer und Hostbrowser bei Song/Fill offen/zu, freier Fläche daneben, Compactpage und mehreren Rowzahlen. Kein verdecktes Ziel und kein Verlust eines vorherigen Samples bei Ablehnung.
- Strg+Master± aufDefault100, alle Song-/Footerlücken inert; angenommene Änderungen dirty, abgelehnte/No-ops nicht.
- Gespeichertes Projekt mit16 Rows, Multi-Out, Song und Pitch/SRC nach vollständigem Hostneustart prüfen; separate Timing-/Realtimegates aus dem Gesamtaudit nachweisen.

## 10. Geprüfte Reparaturdateien und feste Referenzen

Die folgenden14 Dateien bilden den vollständigen Reparaturdiff. Sie wurden samt den zugehörigen Aufrufpfaden geprüft. Der insgesamt archivierte Produkt-/Test-/Build-/Ressourcenbestand umfasst39 Quelldateien,16 Testdateien und fünf weitere Dateien; seine60 Dateien wurden zur Identität gehasht. Daraus wird keine neue vollständige Laufzeitprüfung sämtlicher unveränderter Funktionen behauptet.

| Datei | Aktuelle Zeilen | SHA-256-Präfix |
|---|---:|---|
| [CMakeLists.txt](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/CMakeLists.txt) | 243 | `f8b1d5c7a75a` |
| [src/core/CompactRowViewport.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/core/CompactRowViewport.h) | 32 | `14db7de65211` |
| [src/core/PatternEventTimeline.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/core/PatternEventTimeline.h) | 36 | `90ec49738912` |
| [src/core/UiOverlayHitZones.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/core/UiOverlayHitZones.h) | 10 | `028661c6d9e4` |
| [src/vst3/DrumatorController.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.cpp) | 750 | `388f9db57bcc` |
| [src/vst3/DrumatorController.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorController.h) | 139 | `4ebc0f4e9925` |
| [src/vst3/DrumatorProcessor.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.cpp) | 906 | `b2f729881960` |
| [src/vst3/DrumatorProcessor.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorProcessor.h) | 126 | `1395a51a210e` |
| [src/vst3/DrumatorView.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.cpp) | 1048 | `7c5c33f472d0` |
| [src/vst3/DrumatorView.h](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/src/vst3/DrumatorView.h) | 65 | `4e8e318030b3` |
| [tests/PackagedAudioHostSmoke.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PackagedAudioHostSmoke.cpp) | 1684 | `0e28c0507923` |
| [tests/PatternEventTimelineTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/PatternEventTimelineTests.cpp) | 21 | `6a355a264e25` |
| [tests/UiOverlayHitZonesTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/UiOverlayHitZonesTests.cpp) | 12 | `4990eca2220b` |
| [tests/VisibleRowsTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/tests/VisibleRowsTests.cpp) | 133 | `e952f5ede2cf` |

**Feste Dokument-/Regelreferenzen:**

- [Ursprünglicher unabhängiger Gesamtaudit](https://github.com/challanger2000/125A-Drumator/blob/3ba3286938c55f89e379ef4eef2a4b89d7c7eb3f/docs/audits/CODEX-INDEPENDENT-AUDIT-2026-10-19.md)
- [Nachfolgender Reparaturbericht, festgeschriebener Dokumentationscommit](https://github.com/challanger2000/125A-Drumator/blob/57b902b505bd9bf04110391af0313a7719ec6bda/docs/CODEX-FINDINGS-REPAIR-2026-10-19.md)
- [Verbindlicher Engineering-Einstieg](https://github.com/challanger2000/125A-Engineering/blob/b98fe97b474edd22cf8f8d7f9b0380576b97cd34/START-HERE.md)
- [Engineering-Guardrails einschließlich zu überprüfender Reparaturbehauptung](https://github.com/challanger2000/125A-Engineering/blob/b98fe97b474edd22cf8f8d7f9b0380576b97cd34/ENGINEERING-GUARDRAILS.md)
- [Projektspezifische Regeln am geprüften Produktcommit](https://github.com/challanger2000/125A-Drumator/blob/dbd9106f7af48b23ad4874a1a4c74395baa14173/AGENTS.md)
- [Vorhandener Windowslauf, keine neue Auslösung](https://github.com/challanger2000/125A-Drumator/actions/runs/39335925418)

Der Bericht-Commit enthält ausschließlich `docs/audits/CODEX-RECHECK-A01-A09-2026-10-19.md`. Er ist **nicht** der geprüfte Produktcommit und erhebt keinen Anspruch auf einen neuen Windows-Testlauf. Die Dokumentation ist für die anschließende Bewertung und gezielte Korrekturentscheidung bestimmt.

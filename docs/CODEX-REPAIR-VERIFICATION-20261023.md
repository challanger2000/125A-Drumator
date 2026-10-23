# 125A Drumator – unabhängige FINAL Repair Verification

**Prüfdatum:** 23.10.2026, Europe/Berlin.

**Repository:** `challanger2000/125A-Drumator`.

**Geprüfter Produktcommit:** `34a52da622553629008ce62520715d39f4915046`.

**Geprüfter Entwicklungsbranch:** `fix/studio-one-acceptance-20261022`.

**Auditbranch:** `audit/codex-repair-verification-20261023`.

**Originalbericht:** [CODEX-FINAL-REAUDIT-20261023.md auf 0cb997f734c145dcf34ab12d0e0f86f0dcce4a31](https://github.com/challanger2000/125A-Drumator/blob/0cb997f734c145dcf34ab12d0e0f86f0dcce4a31/docs/CODEX-FINAL-REAUDIT-20261023.md), damals geprüfter Produktstand `6879d829afabbe5d21be2fea17d58b018f77d5cc`.

## 1. Ergebnis und Nachweisgrenze

**Weitere technische Reparaturen sind erforderlich: JA. Keine V1-/Releasefreigabe.**

Von FR01–FR11 ergeben sich **7 PASS, 1 FAIL und 3 TEILWEISE**. Die sieben PASS beziehen sich auf die jeweils abgegrenzte ursprüngliche Ursache, nicht auf eine pauschale DAW-Freigabe. Die ursprüngliche FR01-Gegenprobe besteht inzwischen, die zusätzliche Stopgeneration macht die gesamte Start-/Stoptransaktion aber noch nicht atomar. Zwei weitere zulässige Interleavings bleiben fehlerhaft.

Die nachfolgend dokumentierten zusätzlichen beziehungsweise verbleibenden Befunde sind:

- **RV01 – HIGH:** Alter Previewstart kann weiterhin nach STOP feuern; ein neuerer Start kann alternativ verloren gehen.
- **RV02 – MEDIUM:** Nicht zugestelltes STOP wird verschwiegen; GUI meldet Erfolg beziehungsweise schließt, obwohl Preview weiterläuft.
- **RV03 – HIGH:** OOM im Hostpattern-Bestätigungsquery kann nach bereits erfolgtem Controller-Recallcommit entweichen.
- **RV04 – MEDIUM, Testbefund:** Der neue FR05-Pixeltest erkennt eine vollständige Wiederherstellung des ursprünglichen falschen View-Fadeblocks nicht zuverlässig.
- **RV05 – LOW, Testbefund:** Der FR10-Randtest trifft auch im alten fehlerhaften Layout einen gültigen Hitbereich.

**Alle RV-Produktgegenfälle sind Quellnachweise, keine in diesem Audit ausgeführten nativen Fehlerreproduktionen.** RV04/RV05 sind analytische Orakelgegenbeispiele; ich habe keine Produktmutanten gebaut oder ausgeführt. Es werden keine tatsächlichen neuen Windowsfehler, Hostabstürze, Speicherlecks oder ASIO-Dropouts behauptet.

Dieser Audit hat **keine Produkt- oder Testdatei verändert**, keinen C++-/Linux-/Windows-Build ausgeführt und keinen Actions-Lauf gestartet. Nur diese neue Berichtsdatei wird auf dem separaten Auditbranch committet und veröffentlicht.

## 2. Grundlagen, Herkunft und vorhandene Windows-QA

Zuerst wurde erneut `125A-Engineering/START-HERE.md` geöffnet. Der Regelstand ist unverändert `9854275cdad1d35476397d874ba95efc4f2b184a`; die bereits im vorherigen Audit gelesenen einschlägigen Engineering-, DSP-, GUI-, QA-, Repository-, Forschungs- und Brandingregeln gelten fort. Projekt-`AGENTS.md` ist gegenüber dem vorherigen geprüften Stand unverändert. Bestätigung „verstanden“ erfolgte vor der eigentlichen Reparaturprüfung.

Geprüft wurden der vollständige Reparaturdiff, der unveränderte umgebende Controller-/Processor-/Viewpfad, die neuen Tests und ihre Instrumentierung sowie die Gegenüberstellung zum Originalbericht. Der angeforderte SHA wurde explizit verwendet; keine stillschweigende Umstellung auf einen anderen HEAD. Die tatsächliche Produktversion bleibt `0.1.0`. Kein veröffentlichter FULL-/Demo-Release wird aus diesem Entwicklungsstand abgeleitet.

Ausgewertet wurde der bestehende [GitHub-Actions-Lauf #40009835904](https://github.com/challanger2000/125A-Drumator/actions/runs/40009835904), Job/Check **120607058358**. Seine `head_sha` ist exakt **34a52da622553629008ce62520715d39f4915046**, Status `completed`, Ergebnis `success`.

| Ebene | Tatsächlich vorhandener Nachweis | Aussagegrenze |
|---|---|---|
| Windows x64 Configure/Compile/Link | PASS | Kompilierbarkeit des damaligen Produkt-/Teststands |
| CTest | **23/23 PASS** | Bestehende Testprogramme, keine Vollständigkeitsgarantie |
| Native Integration | **96 einzeln als PASS protokollierte Fälle, 0 Fehler** | 88 vorherige Fälle plus 8 neue FR-Fälle |
| FA23-01–FA23-10 | **10/10 PASS** | Alte konkrete Gegenfälle; zusätzliche RV-Fälle fehlen |
| Neue FR-Fälle | **8/8 PASS** | FR01, FR02, FR03, FR08, FR04, FR05, FR06, FR10; Orakelprüfung siehe unten |
| Offizieller Steinberg Validator | **47/47 PASS** | Kein Studio-One-Praxistest |
| Paketierter Editorhost | PASS | Einschließlich benannter nativer Zoom-/Resizeprüfungen |
| Paketierter Audiohost | PASS | Einschließlich T01/T02/T04/T28/T29/T30 |
| Neu ausgeführte native Audittests | **KEINE** | Keine Veränderung vorhandener Tests, kein neuer Build |
| Tatsächlich ausgeführte FR-Reparaturmutationen | **NICHT NACHGEWIESEN** | Die vorhandenen RM01–RM13 betreffen ältere Ursachen, nicht automatisch die neuen FR-Reparaturen |
| Studio One/Studio Pro, ASIO, 125A PluginTester | **NICHT DURCHGEFÜHRT in diesem Audit** | Keine Releasefreigabe daraus ableiten |

Die Ergebnisse stammen aus der Actions-/Checks-API mit individuellen Fallabschlüssen. Die nachfolgende Detaildiagnose der Sammelannotation wird abgeschnitten; keine nicht sichtbaren Einzelmessreihen oder Leakzähler werden daraus erfunden.

**Statusmaßstab:** PASS bestätigt den benannten reparierten Vertrag mit konkreter Quell-/Testbegründung. TEILWEISE bedeutet implementierte Korrektur, aber relevante verbleibende Fehler oder unzureichende Regressionsabsicherung. FAIL bezeichnet einen konkreten noch fehlerhaften Produktpfad. NICHT NACHGEWIESEN wird ausdrücklich für fehlende Ausführungs-, Mutations- und Hostnachweise verwendet; das ist kein implizites PASS.

## 3. Tabelle FR01–FR11

| Befund | Gesamtstatus | Unabhängige technische Begründung | Test-/Mutationsbewertung |
|---|---|---|---|
| **FR01 – Preview/STOP-Reihenfolge** | **FAIL** | Die Generation verwirft den im ursprünglichen Test bereits konsumierten alten Start korrekt. Generation und Flags bilden aber weiterhin keine gemeinsame Transaktion: RV01-A lässt einen alten Start nach STOP durch, RV01-B verliert einen neueren Start. | Neuer echter Processor-Interleavingtest PASS, jedoch nur Pause nach Startübernahme. Interleavings vor Startübernahme beziehungsweise innerhalb STOP fehlen. |
| **FR02 – OOM, Nachrichten und Cleanup** | **TEILWEISE** | Die ursprünglichen Sampleeditor-Snapshot-/Preview-/Applystellen fangen nun auch Messageallokation ab; lokale Owner releasen in Rückgabe- und Exceptionpfaden. STOP fängt OOM ab und releast, verschweigt aber fehlgeschlagene Zustellung. Andere relevante Nachrichten-/Recallpfade bleiben ungeschützt, insbesondere RV03. | 16 Failpoints je vier Methoden bestehen gegen Exceptionescape. Test ignoriert Rückgaben und misst keine Message-/Heapfreigabe oder vollständige Zustandsneutralität. |
| **FR03 – Legacy-Samplepräsenz** | **PASS** | `samplePresent_.fill(false)` wird im erfolgreichen gemeinsamen DMR1/2/3-Commit gesetzt. Abgelehnte Legacydaten erreichen diesen Commit nicht. | Drei tatsächliche Controller-Legacyformate in zuvor beladener Instanz geprüft. Entfernen der neuen Zeile würde die `!hasSample(0)`-Assertion verletzen; tatsächlicher Mutationslauf nicht durchgeführt. |
| **FR04 – Waveformgeometrie** | **PASS** | Derselbe `UiWaveEditorLayout::waveform`-Bereich beginnt nun bei y=155 und wird für Hintergrundzeichnen und Mousehit verwendet. | Native Mouseaktion bei (500,165) verändert wirklich den Draftstart; Rücknahme der Hitgrenze auf 175 würde diesen benannten Test scheitern lassen. Alle Zoom-/DPIränder bleiben zusätzliche Abnahme. |
| **FR05 – Live-Fade/DSP-Endpunkt** | **TEILWEISE** | Die ursprüngliche separate falsche Endpunktformel wurde entfernt. Der View verwendet nun `SampleEdit::amplitude(floor(sourceFrame))` wie die Engine und berücksichtigt dabei auch Gain. Die Korrektur ist quellseitig richtig. | Der neue Pixeltest PASS ist gegenüber dem vollständigen alten Viewblock nicht trennscharf: RV04. Kein neuer tatsächlicher PCM-/View-End-to-End-Abgleich für den Einframefall. |
| **FR06 – unsichtbare Modalfehler** | **PASS** | Sprachabhängiger Teilstringfilter wurde entfernt; jeder nichtleere Status wird nach dem Modalhintergrund im Modal gezeichnet. | Native Pixel unterscheiden leeren Status von `ZOOM: WAVEFORM UNAVAILABLE`. Rücknahme des alten Filters würde diesen Test scheitern lassen. Test setzt Status direkt; echte Queryfehlerroute nicht zusätzlich ausgeführt. |
| **FR07 – verbliebene deutsche GUItexte** | **PASS** | Die konkret beanstandeten Song- und Legacystrings sind durch englische Bezeichnungen ersetzt; geprüft an aktiven Zeichenpfaden. Keine Änderung von Dateinamen, Benutzerlabels oder Statewerten. | Quell-/Stringabgleich. Kein neuer vollständiger Localizationkatalog-/Textbreiten-/Hosttest; kein erfundener automatisierter FR07-Fall. |
| **FR08 – importierter Pfad nicht speicherbar** | **PASS** | `loadWav` lehnt >4096 UTF-8-Bytes vor Zugriff und Mutation ab; damit ist der Import mit bestehendem Controller-Save-/Recalllimit konsistent. | 4097-Byte-Eingabe wird als Rejected geprüft, altes WAV bleibt bytegleich. Keine echte Windows-Langpfaddatei oder positive 4096-Grenz-/Saveprobe; das wird nicht behauptet. |
| **FR09 – unwirksamer Modal-Drop-Test** | **PASS** | Positiv- und Modalkontrolle benutzen nun beide das wirklich gültige Sampleziel (150,128). Positivkontrolle prüft Annahme und tatsächliche neue eingebettete Bytes. | Korrigierter FA23-04 PASS. Entfernen allein des WAV-Modalguards würde den gültigen negativen Drop zulassen und dessen Assertion verletzen. Dieser Mutationslauf wurde nicht ausgeführt. |
| **FR10 – Regler-Zeichen-/Hitmittelpunkte** | **TEILWEISE** | Zeichnen und Mouseinput verwenden jetzt dieselben `globalKnobCenters`; ursprüngliche doppelte Zeichenmittelpunkte entfernt. | Neuer Randtest besteht aber auch mit den alten Zeichenmittelpunkten: RV05. Kein Pixel-/Hit-Konsistenznachweis aller drei Regler. |
| **FR11 – veraltete WAV-Anleitung** | **PASS** | Aktuelle Anleitung beschreibt direktes EDIT, Fadegriffe, Gain, Zoom/Fit/Scroll, Preview/Stop, Reset/Cancel/Apply und benennt entfernte Aktionen. FLAC-Konvertierung und fehlendes Time-Stretching sind korrekt abgegrenzt. | Dokument-/Quellabgleich. Die Anleitung ist kein Nachweis, dass STOP in allen Fehlerpfaden funktioniert; RV01/RV02 bleiben maßgeblich. |

PASS bei FR07/FR11 ist ein nachvollziehbarer Quell-/Dokumentabgleich, kein behaupteter GUI-Praxislauf. Die Einschränkungen der einzelnen Tests werden nicht hinter dem Status verborgen.

## 4. Detailprüfung der elf Reparaturen

### 4.1 FR01 – genauer Wirkungsbereich der neuen Generation

Produkt: [src/vst3/DrumatorProcessor.cpp:153–157](https://github.com/challanger2000/125A-Drumator/blob/34a52da622553629008ce62520715d39f4915046/src/vst3/DrumatorProcessor.cpp#L153-L157), **861–863, 1097–1103**; `src/vst3/DrumatorProcessor.h:113–114`.

STOP erhöht zuerst `auditionStopGeneration_`, löscht dann `auditionStart_` und setzt zuletzt `auditionStop_`. Audio liest zuerst die Generation und nimmt danach das Startbit. Vor dem Launch wird die Generation erneut verglichen.

Der neue `testFr01StopAfterStartConsumed` in `tests/integration/FinalAudit20261023.inl:155–170` pausiert Audio mit dem Hook aus `ReauditInstrumentation.cmake:6–7` **nach** Startübernahme. Der UI-Thread sendet einen vollständigen Stop, bevor Audio fortgesetzt wird. Der Generationsvergleich unterscheidet genau diese Reihenfolge korrekt. Aktueller und nächster Block werden auf getrennte L/R-Stille geprüft; der bestehende Lauf bestätigt PASS.

Für einen hypothetischen Mutanten, der den neuen Generationsvergleich einfach weglässt, ist dieses Orakel geeignet: Das vorgemerkte alte Sample würde erneut hörbar. **Eine tatsächlich ausgeführte neue FR01-Mutation ist jedoch nicht nachgewiesen.** Vor allem untersucht der Test nicht, ob das Lesen von Generation und Startbit selbst eine konsistente Anforderung erfasst. Zwei konkrete Gegenfälle stehen in RV01.

CANCEL und Editor-Schließen benutzen denselben Controller-Stopweg (`DrumatorView.cpp:365–371, 1214–1225`). Die neue FR01-Prüfung klickt weder CANCEL noch schließt sie eine View während des Interleavings. Die gemeinsame Route begründet Relevanz, ersetzt aber keine eigene native Ereignisprobe.

### 4.2 FR02 – Ausnahmegrenzen und Besitz

Produkt: `src/vst3/DrumatorController.cpp:786–921`.

In Snapshot, Apply und Preview sind nun getrennt abgesichert:

1. `allocateMessage`, einschließlich eines geworfenen `std::bad_alloc` aus dem SDK-Host.
2. Attributanlage, Message-ID und Sendepfad innerhalb des nachfolgenden Tryblocks.
3. Automatisches `release()` durch den lokalen `MessageOwner` beim normalen Return und beim Stackunwinding.

Apply behandelt einen nach erfolgreichem Commit fehlschlagenden Viewrefresh gesondert: Ein nachträglicher `bad_alloc` in `notifyViews` macht den bereits bestätigten Commit nicht wieder zum fehlgeschlagenen Edit. Das ist eine sinnvolle Trennung von Modellcommit und optionaler Anzeigeaktualisierung.

`stopSampleEditPreview` hat einen Catch und ein manuelles `release()` nach dem Catch. Für die geschützten OOMfälle bleibt ein bereits zugewiesener Messagepointer damit auf dem Cleanupweg. Es wird kein Double-release in diesen neuen Methoden nachgewiesen. Ein tatsächlich gemessener Ressourcenbilanznachweis fehlt; die Bewertung der Freigabe ergibt sich aus dem Quellpfad.

Der neue Test `FinalAudit20261023.inl:171–199` injiziert je 16 Failpoints in Preview, Apply, Snapshot und Stop. Er prüft im Wesentlichen, dass kein `bad_alloc` entweicht, und am Schluss, dass das WAV nicht leer ist. Er prüft **nicht**:

- dass ein fehlgeschlagener Stop tatsächlich beendet oder sichtbar abgelehnt wird;
- dass Message-/Attributobjekte vollständig freigegeben wurden;
- dass alle möglicherweise erreichbaren Allokationsstellen erfasst wurden;
- dass Rückgabewert, angenommener Commit und vollständiger State übereinstimmen;
- dass Grid, SampleEdit, Undo und aktive Stimmen bei Ablehnung korrekt erhalten bleiben;
- den Hostpattern-Bestätigungsquery oder weitere Row-/MIDI-/Recall-Nachrichtengrenzen.

Die Rückgaben der drei Boolmethoden werden im Test verworfen. Ein ausschließlich immer mit false zurückkehrender Ersatz könnte diesen OOM-Einzelfall ebenfalls bestehen; die alten erfolgreichen WE-/FA23-Fälle verhindern zumindest eine so offensichtliche allgemeine Funktionsabschaltung in der Gesamtsuite. Das macht den neuen OOMtest selbst noch nicht vollständig.

RV02 und RV03 begrenzen deshalb den Gesamtstatus. Insbesondere liefert die neue Exceptionabsicherung keinen Beweis, dass sämtliche Controller-/Processorpfade OOM-sicher wären. Auch unveränderte Methoden wie `previewSample` und `clearSample` enthalten weiterhin nicht durchgehend geschützte Messageallokation/Attributanlage (`DrumatorController.cpp:922–944`). Sie dürfen nicht als durch diese Reparatur bereits abgedeckt bezeichnet werden.

### 4.3 FR03 – Legacyformat und Samplepräsenz

Produkt: `src/vst3/DrumatorController.cpp:283–337`, neue Synchronisation **318**. Test: `tests/integration/FinalAudit20261023.inl:201–215`.

Die gemeinsame erfolgreiche DMR1/2/3-Commitphase setzt nun alle Samplepräsenzbits zurück. Die Bedingung ist an den tatsächlichen Legacyformatpfad gebunden, nicht an die gerade angezeigte Row. DM16 mit eingebetteten Samples benutzt weiterhin seine bisherige, datenabhängige Präsenzinitialisierung. Die Änderung ist damit mit vorhandenen Samples und modernen States vereinbar.

Der native Test beginnt pro Format mit einem vorhandenen WAV und `hasSample(0)==true`, ruft den echten Controller-Legacyrestore und verlangt anschließend false. Die primitive ursprüngliche Ursache wird direkt getroffen. Der Test ruft aber nicht zusätzlich `Processor::setState(legacy)` und kontrolliert nicht alle 16 Rows sowie die resultierenden nativen EDITpixel. Das ist eine zusätzliche Integrationslücke, kein Grund, die nun richtige Controllerzuweisung als weiterhin fehlerhaft darzustellen.

### 4.4 FR04 – gemeinsame Wellenformgeometrie

Produkt: `src/core/UiWaveEditorLayout.h:13`; `src/vst3/DrumatorView.cpp:859, 1166–1193`. Tests: `tests/UiCompactLayoutTests.cpp:64–70` und `tests/integration/FinalAudit20261023.inl:250–257`.

Der Bereich ist nun `{284,155,994,382}`. Zeichnung und `waveform.hit` referenzieren dieselbe Definition. Modelltests überprüfen obere/untere Grenzen, der native Test sendet einen echten Klick in den früher toten 20-Pixel-Streifen. Die tatsächliche Änderung des Draftstarts ist stärker als eine reine `Rect::hit`-Assertion.

Die testweise Rücknahme der Hitgrenze auf 175 würde die konkrete Klickassertion verletzen. Eine nur zurückgenommene Zeichenreferenz könnte durch den Mausfall allein unentdeckt bleiben; daher bleibt eine vollständige Render-/Hit-Kreuzmatrix sinnvoll. Der native neue Fall läuft bei Standardzoom, nicht auf allen vier Zoom-/DPIkombinationen. Gemeinsame Sourcegeometrie ist bestätigt; kein vollständiger Studio-One-Geometrie-PASS.

### 4.5 FR05 – gemeinsame Hüllkurve, aber unzureichendes Pixelorakel

Produkt: `src/vst3/DrumatorView.cpp:876–895`; unveränderter Vertrag `src/core/SampleEdit.h:26–33`; Engineaufruf `src/core/SampleVoiceEngine.h:270–273`.

Der View ruft jetzt dieselbe diskrete Hüllkurvenfunktion wie Audio mit dem ganzzahligen Sourceframe auf. Damit ist der beanstandete Fade-Out-Endpunkt ohne separate Off-by-one-Formel implementiert. Gain wird nun in der Wellenform ebenfalls berücksichtigt. Original-WAV, gespeicherter Edit, Audiokern und Projektformat werden dabei nicht geändert. Graphisches Clipping innerhalb des Panels bleibt eine Anzeigeentscheidung, kein Beschneiden der gespeicherten Audiodaten.

Der neue Pixeltest `FinalAudit20261023.inl:258–274` verwendet den echten nativen Viewrenderer, setzt den Draft jedoch direkt über `IntegrationAccess`. Er rendert kein tatsächliches Audio für diesen konkreten Einframefall. Er bestätigt aktuell passende Pixelbilder, ist aber gegenüber dem vollständigen alten Viewblock blind: RV04. Deshalb TEILWEISE hinsichtlich der geforderten belastbaren Regressionsabsicherung, nicht die Behauptung, die alte Endpunktformel sei noch vorhanden.

Peak-Bin-/Pixelaggregation und die Multiplikation mit einer Hüllkurve am jeweiligen Pixel-Sourceframe sind weiterhin eine visuelle Approximation. Die Anzeige ist keine samplegenaue PCM-Exportreferenz sämtlicher Transienten innerhalb eines Bins. Für kurze/extreme Bereiche sind unabhängige PCM- und sichtbare Referenzorakel erforderlich.

### 4.6 FR06 – Fehlertext wird nicht mehr nach Wortfragmenten unterdrückt

Produkt: `src/vst3/DrumatorView.cpp:971–973`. Test: `tests/integration/FinalAudit20261023.inl:275–282`.

Der alte Filter ist tatsächlich entfernt. Ein nichtleerer Query-/Zoom-/Scrollfehler kann unabhängig von seiner Formulierung im nach dem Modalhintergrund gezeichneten Textfeld erscheinen. Der native Vergleich mit leerem Status zeigt, dass der beanstandete Text den Pixelbereich wirklich verändert.

Der Test setzt den Text direkt; er ruft keine tatsächliche fehlgeschlagene Zoom-/Scrollquery auf. Für die eng abgegrenzte alte Zeichenursache ist er dennoch geeignet: Wiederherstellen des früheren Filters unterdrückt genau diesen Text. Eine echte Fehlerroute mit erhaltenem Entwurf und bedienbarem CANCEL sollte ergänzt werden. Das jetzige Zeichnen beliebiger Statusmeldungen kann auch alte Informationstexte anzeigen; das ist ein Verbesserungs-/Statusmodellpunkt, kein hier belegter Datenfehler.

### 4.7 FR07 – Englischkorrektur

Produkt: `src/vst3/DrumatorView.cpp:673–676, 750–761`.

`ALT RT` wurde `LEGACY RT`; Songkopf und Seitennavigation heißen jetzt `SONG CHAIN`, `SELECT SLOT, THEN PATTERN AND LENGTH`, `< PREVIOUS` und `NEXT >`. Die im Originalbericht konkret aufgeführten deutschen Zeichenstrings sind in diesen aktiven Pfaden nicht mehr vorhanden. Die Änderung betrifft Zeichenketten, nicht Pattern-/Songdaten, Benutzerlabels oder WAV-Dateinamen.

Kein neuer automatisierter FR07-Localizationfall existiert. Das PASS ist ein konkreter Quell-/Stringabgleich. Lesbarkeit sämtlicher längerer englischer Labels, Popupbreiten, Legacyanzeigen und alle Hostskalierungen sind dadurch nicht zusätzlich nativ abgenommen.

### 4.8 FR08 – konsistente Pfadgrenze

Produkt: `src/vst3/DrumatorController.cpp:1067–1071`, bestehender Save-/Recallvertrag **108–122, 139–148**. Test: `tests/integration/FinalAudit20261023.inl:216–225`.

Die neue unbedingte Prüfung `path.size()>4096u` liegt vor Dateizugriff und allen Änderungen. Der Controller kann daher über diesen Importpfad keinen neuen Herkunftspfad annehmen, den sein bestehendes DZM3-Format wegen genau dieser Länge nicht speichern könnte. Vorhandene gültige Controllerstates haben bereits die entsprechende Obergrenze; Parameter-/Stateformat wurde nicht geändert.

Der Test verwendet eine 4097-Byte-Eingabe mit sehr langem einzelnen Dateinamen, keine reale gültige Windows-Verzeichniskette. Er verlangt ausdrücklich `Rejected`, nicht bloß irgendeinen Fehler, und bytegleichen bisherigen Sourceinhalt. Ohne neue Prüfung würde die Dateisystemprüfung stattdessen einen anderen Fehlerstatus liefern. Das schützt den frühen Limitguard. Eine tatsächliche positive Windowsdatei an der 4096-Grenze mit Save-/Fresh-Recall wird nicht geprüft und bleibt offen.

### 4.9 FR09 – D&D jetzt mit positivem gültigem Ziel

Produkt: `src/vst3/DrumatorView.cpp:357–363`; `src/core/UiCompactLayout.h:14–18`. Test: `tests/integration/FinalAudit20261023.inl:47–76`.

Der frühere Punkt x=250 wurde durch x=150 ersetzt. Der Test beweist zunächst ohne Modal, dass Enter einen Copydrop anbietet, Drop angenommen wird und die eingebetteten Bytes tatsächlich auf die unterscheidbare Ersatzquelle wechseln. Eine zweite Instanz öffnet den echten Editor und verlangt bei demselben gültigen Rowpunkt Ablehnung und unveränderte Source.

Wird ausschließlich `sampleEditorOpen_` aus der Sperre entfernt, bleiben Song-/Popup-/Fillzustand in dieser Fixture frei und der gültige Drop kann ausgeführt werden. Das Orakel würde dann entweder am Annahmestatus oder am veränderten WAV scheitern. Dies ist eine nachvollziehbare statische Mutationsprognose, **kein tatsächlich gestarteter Guard-entfernter Windowslauf**.

Der Punkt liegt links außerhalb der gezeichneten Modalbox, aber innerhalb des normalen Sampleziels. Das ist gerade eine geeignete Prüfung der gesamten modalen Eingabesperre. Echte Explorer-/Studio-One-Browserweiterleitung, alle Dragphasen und Zoomtransformationen sind separate offene Hostprüfungen.

### 4.10 FR10 – gemeinsame Reglerzentren

Produkt: `src/vst3/DrumatorView.cpp:597–601, 1343–1345`; `src/core/UiCompactLayout.h:20`.

Beide Pfade verwenden jetzt `globalKnobCenters` mit 78/176/1180. Die ursprüngliche Zweipixelabweichung der Zeichenmittelpunkte ist damit beseitigt. Eine absichtlich etwas größere symmetrische Hitfläche als der sichtbare Kreis ist weiterhin zulässig.

Der neue Test `FinalAudit20261023.inl:283–293` verwendet jedoch x=78+16,5=94,5. Dieser Punkt war bereits im alten Hitbereich `[60,96)` gültig. Das Wiederherstellen nur der alten Zeichenzentren lässt seine Eingaben und Controllerassertion vollständig unverändert: RV05. Eine tatsächliche Pixel-/Hit-Relation aller drei Regler und mehrere Zooms fehlen. Deshalb TEILWEISE als Reparaturverifikation trotz korrekter Sourcevereinheitlichung.

### 4.11 FR11 – aktuelle Anleitung

Datei: `docs/WAV-EDITOR-IMPLEMENTATION.md:1–16`.

Die Anleitung ist tatsächlich neu geschrieben und entspricht den vorhandenen direkten Aktionen. AUTO START und früherer Menüeditor werden ausdrücklich als entfernt beschrieben. FLAC→WAV, nichtdestruktive Bearbeitung, kein automatisches BPM-/Time-Stretching und offene Studio-One-Abnahme werden korrekt getrennt.

Der alte Dokumentationsbefund ist damit behoben. Ein Quell-/Dokumentreview genügt für diese alte Textabweichung; es ersetzt keine Funktionstests der beschriebenen STOP-/CANCEL-Verträge. Deren aktuell verbleibende Probleme werden nicht durch die Anleitung geheilt.

## 5. Neue beziehungsweise verbleibende konkrete Befunde

### RV01 – HIGH – Generation und Flags sind weiterhin keine atomare Previewtransaktion

**Bezug:** FR01. **Nachweis:** statische zulässige Threadfolgen; keine neue native Ausführung.

**Quellen:** `src/vst3/DrumatorProcessor.cpp:153–157, 194–195, 861–863, 1097–1103`; `src/vst3/DrumatorProcessor.h:113–114`.

#### Gegenfall A: alter Start wird trotz verarbeitetem STOP gestartet

Anfang: altes Previewasset A ist publiziert, Start=true, Stop=false, Generation=0.

| Reihenfolge | UI/Controller→Processor | Audiothread |
|---|---|---|
| 1 | STOP beginnt und führt nur `fetch_add` aus: Generation=1. UI wird vor `auditionStart_.store(false)` unterbrochen. | |
| 2 | | Liest Generation=1 in `auditionGeneration`. |
| 3 | | Übernimmt das noch alte Start=true in `fireAudition` und adoptiert A. |
| 4 | UI setzt Start=false und Stop=true; STOP kehrt vollständig zurück. | |
| 5 | | Konsumiert Stop=true und setzt Voices zurück. |
| 6 | | Vergleicht 1==1, hält den alten Start für gültig und startet A. |

Danach ist Stop=false und kein weiterer Stop für den nächsten Block vorgemerkt. Die alte Wirkung kann nach bereits verarbeitetem STOP erneut eintreten. Acquire/Release macht die Einzeloperationen sichtbar, fasst aber nicht die drei UI-Operationen zu einer Transaktion zusammen. Es gibt hierfür keinen unerlaubten parallelen UI-Aufruf; ein UI- und ein Audiothread genügen.

#### Gegenfall B: neuer Start nach abgeschlossenem STOP verschwindet

1. Audio liest Generation=0 und wird vor dem Startbit-Exchange unterbrochen.
2. UI führt STOP vollständig aus: Generation=1, Start=false, Stop=true.
3. Derselbe UI-Thread führt anschließend einen neuen gültigen PREVIEW B aus: Asset B publiziert, Start=true.
4. Audio übernimmt Start=true, adoptiert B, konsumiert Stop=true.
5. Audio verwirft den **neuen** Start wegen 0!=1.
6. Start und Stop sind danach beide false; im folgenden Block kommt kein neuer Trigger.

Hier wurde ein nach STOP akzeptierter neuer Preview dauerhaft verworfen. Ein zulässiges Verschieben auf den nächsten Block findet nicht statt. Das ist eine neue Nebenwirkung der Generationsreparatur, getrennt vom wieder verbleibenden alten Start in Gegenfall A.

**Auswirkung:** STOP/CANCEL/Close beziehungsweise schnelle STOP→PREVIEW-Folgen sind nicht vollständig geordnet. Keine Speicherfreigabe-/Out-of-bounds-Behauptung; es ist ein logischer Kommandokonsistenzfehler.

**Empfehlung:** Start/Stop/Gültigkeitsgeneration müssen als ein gemeinsam veröffentlichter, eindeutig zugeordneter Kommandostand übernommen werden. Das Asset benötigt eine passende Identität; ein Startbit ohne eigene Generation genügt nicht. Ein begrenztes SPSC-/Mailboxverfahren mit klarer Latest-/Cancel-Semantik erwägen; keine Audioallokation, Locks oder unbegrenzte Wiederholschleifen einführen. Bloßes Verschieben des `fetch_add` löst nicht automatisch beide Gegenfälle.

**Erforderliche Regressionen:** Zusätzliche deterministische Hooks innerhalb STOP nach Generationsänderung und zwischen Audiogenerationslesen und Startübernahme. Beide tatsächlichen Controller-/Processorfolgen, unterschiedliche Sources A/B, aktueller und nächster Block, STOP/CANCEL/Close sowie unverändertes normales Patternaudio prüfen. Den bestehenden FR01-Test unverändert behalten.

### RV02 – MEDIUM – OOM oder fehlgeschlagene Stopzustellung wird als erfolgreicher Stop behandelt

**Bezug:** FR02 und STOP-Vertrag. **Nachweis:** konkreter Fehlerpfad, nicht nativ ausgeführt.

**Quellen:** `src/vst3/DrumatorController.cpp:909–921`; `src/vst3/DrumatorView.cpp:365–371, 1214–1225`; neuer OOMtest `tests/integration/FinalAudit20261023.inl:192–198`.

**Ursache:** `stopSampleEditPreview` gibt void zurück. Bei null Messagepointer, OOM oder abgelehntem `sendMessage` wird keine Stopwirkung bestätigt und keine Zustellung nachgeholt. Trotzdem setzt die View `PREVIEW STOPPED` beziehungsweise schließt beim CANCEL/APPLY. Die neue Catchlogik verhindert die Exception korrekt, behandelt aber die fehlgeschlagene Aktion nicht sichtbar.

**Konkretes Gegenbeispiel:** Lange Audition A starten und einen Teil rendern. Beim nächsten echten STOP die Messageallokation fehlschlagen lassen, etwa mit dem vorhandenen threadlokalen Failpoint 0. Der Catch kehrt normal zurück, kein Processor-Stop wurde gesendet. Ohne Plugin-Deaktivierung läuft A in folgenden Blöcken weiter, während die View Erfolg behauptet oder bereits geschlossen ist.

Die im Kommentar erwähnte spätere Deaktivierung ist kein Ersatz für STOP: Ein Benutzer kann nur das Pluginfenster schließen, während das Plugin im Host weiterhin aktiv verarbeitet wird. Auch ein einfach abgelehntes Senden wird derzeit ignoriert.

**Einordnung:** Der alte Code war bei geworfenem OOM ebenfalls nicht korrekt; er konnte die Exception auslassen. Das neue Catch ist ein echter Fortschritt, aber noch kein sicherer Stopfehlervertrag. Es wird nicht behauptet, ein vormals zuverlässig zugestellter Stop sei durch das Catch zerstört worden.

**Empfehlung:** Zustell-/Ausführungserfolg explizit rückmelden, Misserfolg sichtbar behandeln und einen zuverlässigen begrenzten Abbruchpfad vorsehen. Reservierte/allocationsarme Stopnachricht beziehungsweise geordnete ausfallsichere Cancelanforderung prüfen. CANCEL/Close darf nicht allein aus normaler Methodenrückkehr auf erfolgte Audiobeendigung schließen. Keine Allokation/Retry-Blockade im Audiothread als Lösung.

**Erforderlicher Test:** Aktive tatsächliche Preview, gezielter OOM/null/Send-Reject beim Stop, danach reales PCM und sichtbaren Status prüfen. Dass lediglich keine Exception entweicht, reicht nicht. Dieser Fall würde den jetzigen neuen FR02-Test trotz weiterlaufender Preview bestehen lassen.

### RV03 – HIGH – Ungefangener Patternquery-OOM nach dem Recallcommit

**Bezug:** weiter offene OOM-/Restoreintegration, keine neu durch den Reparaturdiff eingeführte Zeile.

**Quellen:** `src/vst3/DrumatorController.cpp:69–92, 219–281, 324–327`; `src/vst3/DrumatorProcessor.cpp:115–120`. Gepinntes SDK `public.sdk/source/vst/hosting/hostclasses.cpp:40–46, 108–117, 221–226`.

**Ursache:** Nach gültiger Vorbereitung wird im Controller bereits neuer Kitinhalt übernommen, Undo/Gesten werden invalidiert und Namen/Pfade gesetzt. Bei `hostPatternPending_==true` folgt erst danach `processorConfirmsPattern`. Diese Hilfsmethode allokiert eine Hostmessage, setzt ihre ID und ruft den Processorquery ohne Catch/RAII auf. Auch dessen neues Antwortattribut kann allokieren.

Der verwendete SDK-Host erzeugt IMessage mit `new HostMessage`; `setMessageID` benutzt `new char[len]`, und `setInt` kann einen Mapknoten anlegen. Ein gezielter OOM hier kann aus `Controller::setComponentState` entweichen, obwohl dessen Modellcommit schon erfolgt ist. Der Kommentar „Nothrow commit phase“ sichert diesen nachfolgenden Querypfad nicht ab.

**Konkretes Gegenbeispiel:**

1. Gültiges altes Kit und tatsächliche Hostpattern-Rückmeldung setzen, sodass `hostPatternPending_` true ist.
2. Gültiges neues Componentstate einlesen; Vorbereitungsallokationen gelingen lassen.
3. Ausschließlich die IMessageallokation oder ID-/Antwortattributanlage des nachfolgenden `Drumator.QueryPattern` fehlschlagen lassen.
4. Exception verlässt den Recall nach bereits geändertem Modell; Parameternachführung und Viewabschluss können ausfallen. Bei spätem Fehler fehlt zusätzlich das manuelle `msg->release()`.

Dies ist kein behaupteter tatsächlich beobachteter DAW-Absturz. Der Exceptionpfad und die bereits davor erfolgte Mutation sind im Quellcode eindeutig. Der gleiche Bestätigungsquery ist auch im Legacyabschluss erreichbar.

**Warum grüne Tests das nicht widerlegen:** Der vorhandene Z01-OOMtest erzeugt Undo/Clipboard/Velocitygeste, aber in dieser Fixture keine zusätzliche Hostpattern-Rückmeldung vor Recall. RE07/O01 prüfen die Hostpatternordnung ohne genau diesen Allokationsfehler. Der neue FR02-Test ruft nur die vier Sampleeditormethoden auf. Die Kombination Hostfeedback + Restore + Query-OOM ist nicht abgedeckt.

**Empfehlung:** Den allokierenden Bestätigungs-/Antwortpfad absichern und seinen Zeitpunkt in den Restorevertrag einbeziehen. Falls der Query für eine konsistente Auswahl nötig ist, vor dem irreversiblen Commit vorbereiten beziehungsweise bei Fehlschlag eine definierte, nachweisbar konsistente Policy verwenden. Nicht einfach nach bereits erfolgtem Commit „Recall abgelehnt“ zurückgeben oder neuere Hostautomation blind überschreiben. Gemeinsamen Messagebesitz auch hier verwenden.

**Erforderlicher Test:** Echte Hostpattern-Rückmeldung, tatsächlicher vollständiger sowie Legacy-Recall, kontrollierter Failpoint nur auf dem Patternquery, beide Feedbackordnungen. Exceptionfreiheit, zurückgegebener Status, Controller-/Processor-Endauswahl, Undo/Gesten, sichtbarer Zustand und Messagebilanz gemeinsam prüfen.

### RV04 – MEDIUM – FR05-Pixeltest kann die vollständige alte Fadeursache übersehen

**Art:** nachgewiesenes analytisches Testgegenbeispiel; keine Produktmutation ausgeführt.

**Quellen:** `tests/integration/FinalAudit20261023.inl:258–274`; alter Viewblock in Produktcommit `6879d829afabbe5d21be2fea17d58b018f77d5cc`, `src/vst3/DrumatorView.cpp:876–899`.

Der Test vergleicht zwei Drafts mit identischen START/END/Fades. Nur Gain unterscheidet sich: erst 1, dann 0. Im vollständig alten Zeichenblock wurde Gain jedoch überhaupt nicht in die Wellenform eingerechnet. Wird dieser alte Block als Wiederherstellung der ursprünglichen FR05-Ursache verwendet, erzeugen **beide falsch gezeichneten Drafts dasselbe Bild**. Die Assertion `faded==silentPixels` bleibt dann wahr.

Die Gegenkontrolle setzt anschließend den gesamten Edit auf volle Länge zurück. Damit ändern sich nicht nur Audiofläche, sondern auch START-/END-/Fadeindikatoren innerhalb des untersuchten Ausschnitts. `full!=silentPixels` kann also weiterhin erfüllt sein und beweist nicht isoliert das korrekte Zeichnen der einframe-stillen Auswahl. Der eigentliche Fehler wurde durch die voneinander abhängigen Referenzen verdeckt.

Ein engerer anderer Mutant, der nur einen Endpunkt verändert, Gain aber weiterhin korrekt berücksichtigt, könnte vom Test durchaus erkannt werden. Das ist nicht gleichbedeutend mit Erkennung des vollständig ursprünglichen Viewfehlers.

**Empfehlung:** Referenzsilence unabhängig vom getesteten Gain-/Fadezeichenpfad erzeugen. Zum Beispiel gleiche Marker-/Overlaygeometrie bei tatsächlich nullwertiger Source versus nichtnullwertiger Source mit einframe-stummem Edit, mit abgegrenzten Signalpixeln. Dazu echten Processor-/Engine-PCMoutput des gleichen Edits messen und die ursprüngliche Zeichenformel als gezielte Negativkontrolle verwenden.

**Erforderlicher Nachweis:** Reproduzierbarer roter Test mit vollständig altem separatem Fadeblock, positiver korrigierter Lauf, unveränderte Marker-/Sourcepositionen sowie 1/2/kurze Frames, Mono/Stereo und relevante Ratios. Der aktuelle CI-PASS bestätigt nur die aktuelle Bildgleichheit, keinen solchen Mutationkill.

### RV05 – LOW – FR10-Maustest kann alte Zeichenzentren nicht erkennen

**Art:** analytisch nachgewiesene fehlende Trennschärfe.

**Quellen:** `tests/integration/FinalAudit20261023.inl:283–293`; `src/core/UiCompactLayout.h:20`; alter und aktueller `src/vst3/DrumatorView.cpp:597–601`.

Der Test klickt x=78+16,5=94,5. Der alte Ereignispfad hatte bereits den Hitbereich `[78−18,78+18)`, also `[60,96)`. Die Eingabe liegt darin. Die Reparatur hat gerade die **Zeichenmittelpunkte**, nicht diesen Hitbereich geändert.

Bei Wiederherstellung der alten Zeichenmittelpunkte 80/178/1178 bleibt der neue Testeingabepfad daher unverändert erfolgreich. Seine Assertion misst nur veränderten Swing, nicht die sichtbare Kreisposition. Der im Originalbericht angegebene alte tote Rand um x=96,5 wird nicht geprüft; nach Verschieben des gezeichneten Kreises ist dieser konkrete Punkt außerdem nicht mehr derselbe sichtbare Rand.

**Empfehlung:** Sichtbare Geometrie unabhängig aus nativen Zeichen-/Pixelinformationen oder einem überprüften Rendervertrag erfassen und gegen die Ereignisgeometrie vergleichen. Alle drei Regler und beide Ränder einbeziehen. Ein aus derselben Layoutkonstante berechneter beliebiger erfolgreicher Klick ersetzt diesen Vergleich nicht.

**Erforderlicher Test:** Rücknahme nur der drei Drawmittelpunkte muss eine Render-/Hit-Konsistenzassertion verletzen, während korrekte aktuelle Zeichnung und zulässig vergrößerte symmetrische Hits bestehen. Keine tatsächliche solche Mutation wurde in diesem Audit ausgeführt.

## 6. Unabhängige Regressionsprüfung außerhalb der elf Einzelfälle

Der Produktdiff von `6879d829...` nach `34a52da...` betrifft ausschließlich fünf Produktdateien: `UiWaveEditorLayout.h`, `DrumatorController.cpp`, `DrumatorProcessor.cpp`, `DrumatorProcessor.h` und `DrumatorView.cpp`. Die übrigen Unterschiede sind Testcode und WAV-Dokumentation. Dieser begrenzte Diff wurde vollständig gelesen, nicht aus Commitbeschreibungen erschlossen.

| Bereich | Quell-/Testprüfung und Ergebnis | Verbleibende Grenze |
|---|---|---|
| Normale Sample-/Voice-/SRCengine | `SampleVoiceEngine.h`, `SampleExchange.h`, WAVdecoder und FLACadapter bytegleich zum vorherigen Stand. Alte Sample-/Voice-/FA23-Fälle bestehen. | Keine neue Echtzeit-/Hörabnahme; Auditionkommando RV01 betrifft hörbares Zusatzsignal. |
| MIDI, Choke, Swing, Humanize | Scheduler, TriggerBatch und Kernpolicies unverändert; SA02, RANDOM, SOR-, RE-/RM-Fälle PASS. | Nicht jede reale DAW-/Hostkombination ausgeführt. |
| Pattern A–H, Länge, Undo, Song | AtomicPatternBank, SongChain und zentrale Sequencerlogik unverändert; RE01/RE03/RE11, T01 und P19 bestehen. | Hostfeedback-/OOMkombination RV03 ist ein zusätzlich offener Restorepfad. |
| MIDI-/Hostautomation und Restore/ABA | Alte Arbitrationlogik unverändert und O01/RE07/T30 PASS. | Diese Fälle enthalten nicht den Patternquery-OOM. |
| Multi-Out | Busverhandlung, Aktivierung und Routingkern unverändert; T02/T29 mit unterscheidbaren Stereocodes und negativen Orakeln PASS. | Kein echter Studio-One-Mixer-/Bus-Reopenlauf. |
| Samples/Patterns/Projektformate | KitState, Codec, IDs und Pluginversion unverändert; Legacy-Präsenzreset bewusst ergänzt, Pfadlimit konsistent. | FR03-Test isoliert Controller; echte vollständige Legacy-Doppelrestoreprobe ergänzen. |
| WAV-Editor | Änderung an Darstellung, Hitbereich, Status und Ausnahmebehandlung. Bestehende WE-/FA23-Fälle und neue FR-Fälle PASS. | RV01/RV02 und die GUIorakellücken verhindern Gesamt-PASS. |
| Zoom/DPI/Editor | DrumatorEditor, UIDESC, Assets und bisherige Zoompolicies unverändert; RE02/Z02 und V18 PASS. | Neue WAV-Maus-/Pixeltests überwiegend bei 100 %, echte 120/130/150-%-Hostbedienung offen. |
| Branding | SVGmaster/BrandLogoGeometry und Einbindung unverändert. | Keine neue interaktive visuelle Abnahme behauptet. |
| Echtzeit/Audiothread | Neue Operationen sind atomare Generationszugriffe; keine neue direkte Audiothreadallokation oder Mutex in diesem Diff. | Logische Atomizität RV01 bleibt fehlerhaft; lockfrei allein ist keine Fristgarantie. |

**Kompatibilität:** Keine neue Stateversion, keine veränderte ParamID, keine Neuinterpretation gespeicherter Pattern-/Samplebytes. Der Pfadguard verweigert zusätzliche neue Eingaben, die ohnehin nicht mit dem bestehenden Controllerformat speicherbar waren. Texte und Waveformdarstellung verändern keine gespeicherten Audioassets. Das bestätigt Quellkompatibilität des Diffs; es ist keine Behauptung eines vollständigen erneuten DAW-Projektvergleichs aller historischen Formate.

**Nachgewiesene neue Nebenwirkung:** Das neue Generationsverfahren kann eine nach abgeschlossenem STOP eintreffende frische Preview verschlucken (RV01-B). **Kein Nachweis** einer durch diese Reparaturen neu verursachten normalen DSP-, MIDI-, Choke-, Pattern- oder Multi-Out-Regression. „Nicht nachgewiesen“ wird nicht in die Zusage umgedeutet, dass keine andere Regression existieren kann.

## 7. Aussagekraft der bestehenden QA und Performance

Die acht neuen FR-Registrierungen sind in `tests/integration/WindowsIntegration.cpp:1955–1962` tatsächlich vorhanden. Ihre Checks sind aktive, exceptionbasierte Assertions und keine in Release entfernten C-`assert`-Ausdrücke. Die vorhandenen RM01–RM13 mutieren ältere Fehlerursachen. Für die neuen STOP-/OOM-/Pixel-/Layoutreparaturen wurden keine entsprechenden neuen ausgeführten Mutationkills nachgewiesen.

Die Instrumentierung benutzt eindeutige CMake-Anker in generierten Testkopien. Der FR01-Hook liegt hinter dem echten Startbit-Exchange, nicht in einem Ersatzmodell. Das ist eine geeignete native Gegenprobe für genau diesen Zeitpunkt. Es gibt aber keinen Hook für die beiden zusätzlich nötigen RV01-Reihenfolgen. Ein bestandenes einzelnes Interleaving beweist nicht alle möglichen Interleavings.

Die neuen nativen Pixeltests sind ein Fortschritt gegenüber bloßen Layoutzahlentests. FR05 und FR10 zeigen jedoch konkret, warum native Ausführung allein noch kein unabhängiges, trennscharfes Orakel garantiert. FR06 setzt Status direkt; FR03 setzt nur Controllerstate; FR08 verwendet keine echte lange Windowsdatei. Diese Grenzen sind im Status berücksichtigt.

### 7.1 Tatsächlich sichtbare Leistungswerte des vorhandenen Laufs

| Diagnose | Wert |
|---|---|
| F14, 64 Voices, 16×, 48 kHz / 256 Frames, 180 Blöcke | p95 **4,1877 ms**, p99 **4,3716 ms**, Maximum **5,3148 ms**, **0/180** über Frist |
| Blockfrist 48 kHz / 256 | **5,33333 ms** |
| F15, je 32 Blöcke | p99/Maximum 1× **0,0848 ms**, 4× **2,3249 ms**, 16× **4,3997 ms** |
| Paketierter T04 | 24 Restores; kohärente X-Blöcke **38**, Y-Blöcke **39**, Gate-stille Blöcke **36.260** |

Das F14-Maximum liegt rechnerisch nur etwa **18,5 µs** unter der dortigen Blockfrist. Das ist eine abgeleitete Differenz aus den angezeigten Messzahlen, kein gemessener ASIO-Sicherheitsabstand. Die Stichprobe enthält auch Auslauf-/Leerlaufblöcke, nutzt einen geteilten CIrunner und ist keine Worst-Case-Garantie. Bei n=32 ist nearest-rank p99 das beobachtete Maximum.

Da der normale Voice-/SRCkern unverändert ist und Runnerbedingungen streuen, ergibt sich aus diesen Zahlen **kein Nachweis einer durch die Reparaturen verursachten DSP-Verlangsamung**. Ebenso darf das gemessene 0-Miss-Ergebnis nicht als allgemeine Echtzeitfreigabe dienen. 64-Frame-Fristen und Audiointerface-/Windows-Schedulerlatenzen sind damit nicht bewiesen.

Die Gate-stillen Restoreblöcke sind ein dokumentierter Konsistenzmechanismus, kein unterbrechungsfreier Kitwechsel. Synchroner Import, mehrfaches Decodieren/Kopieren und vollständiger Waveformquery bei Zoom/Scroll bleiben unverändert relevante Last-/UIlatenzrisiken. Keine tatsächlichen dadurch verursachten Studio-One-Aussetzer wurden hier gemessen.

## 8. Priorisierte erforderliche Nacharbeiten

1. **RV01 / HIGH:** Start-/Stopgeneration und Anforderung atomar beziehungsweise eindeutig gekoppelt veröffentlichen. Beide neuen Interleavings plus ursprüngliche FR01-/FA23-03-Fälle gemeinsam absichern. Aktuelles und nächstes PCM getrennt prüfen, Sources unterscheidbar machen.
2. **RV03 / HIGH und FR02:** Den verbleibenden Hostpattern-/Restorequery in einen ausnahmesicheren Commitvertrag aufnehmen; Messagebesitz, Queryfailure und Controller-/Processor-Auswahl konsistent behandeln. Bereits erfolgte Mutation nicht nachträglich als vollständig abgelehnten Recall ausgeben.
3. **RV02 / MEDIUM:** Fehlgeschlagene Stopzustellung sichtbar und wirksam behandeln. Aktive Stimme unter gezieltem OOM/null/Send-Reject prüfen, auch bei CANCEL und Editor-Schließen.
4. **RV04/RV05:** Die beiden neuen GUIorakel so ergänzen, dass eine Rücknahme der vollständigen ursprünglichen Ursache tatsächlich erkannt wird. Keine Abschwächung vorhandener Tests und keine Sollwertänderung auf fehlerhaftes Verhalten.
5. **OOMnachweise vervollständigen:** Statt willkürlicher fixer Failpointzahl tatsächliche Allokationsstellen zählen, frische Fixtures und valide positive Kontrollen verwenden; Message-/Attributbilanz, Rückgabestatus und vollständigen State prüfen. Andere relevante Controllerentrypoints nicht stillschweigend als geschützt deklarieren.
6. **Gezielte verbleibende Integration:** Volle Legacy-Processor-/Controllerrestorefolgen, gültige Windows-Pfadgrenzen mit Save/Fresh-Recall, echte fehlschlagende Waveformqueries und Render-/Hitmatrix bei 100/120/130/150 % plus DPI.
7. **Nach einem später autorisierten Reparaturstand gebündelte Windows-QA und echte Hostabnahme:** vorhandene RE-/RM-/FA23-/FR-Regressionen erhalten, neue Gegenfälle prüfen, danach Studio One/Studio Pro/ASIO mit Stop/Play/Loop, Multi-Out, D&D, Editorclose und Projekt-Reopen. In diesem Audit wird dafür kein Lauf gestartet.

## 9. Abschlussentscheidung und Ablage

**Weitere technische Reparaturen erforderlich: JA.** FR01 ist trotz bestandener ursprünglicher Gegenprobe nicht vollständig behoben. Die drei geschützten Sampleeditor-OOMmethoden sind deutlich verbessert; Stopfehlersemantik und der Patternquery beim Restore bleiben jedoch offen. Die Sourcekorrekturen von FR03/FR04/FR06/FR07/FR08/FR09/FR11 sind nachvollziehbar bestätigt. FR05/FR10 sind quellseitig korrigiert, ihre neuen Testorakel erfüllen die geforderte adversariale Absicherung noch nicht.

**Keine V1-Freigabe und kein Studio-One-/ASIO-PASS.** Der grüne Windows-Lauf bleibt gültige Evidenz für seine 96 benannten Integrationsfälle, 23 CTests und 47 Validatorprüfungen. Er widerlegt nicht die hier konkret gezeigten zusätzlichen Fehlerpfade.

Dieser Commit ergänzt ausschließlich `docs/CODEX-REPAIR-VERIFICATION-20261023.md` auf einem separaten Auditbranch mit dem geprüften Produktcommit als Parentbasis. Kein Produktcode, Test, Asset, Workflow, bestehender Bericht oder Versionswert wird verändert. Entwicklungsbranch und main werden nicht beschrieben oder gemergt; kein Pull Request wird erstellt.

Die unveränderte `.github/workflows/core-ci.yml:3–12` startet auf Push nur für `develop/v1.0.0` beziehungsweise `fix/studio-one-acceptance-20261022` mit Quell-/Test-/Buildpfadfilter oder bei manuellem Dispatch. Die reine Berichtsablage auf `audit/codex-repair-verification-20261023` erfüllt diese Pushbedingungen nicht; kein Dispatch wird ausgelöst.

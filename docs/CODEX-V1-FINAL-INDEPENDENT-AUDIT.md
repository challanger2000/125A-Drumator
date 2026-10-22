# 125A Drumator – unabhängiger V1-Abschlussaudit

**Prüfdatum: 22.10.2026. Ergebnis: KEINE technische V1-Freigabe.**

Drei zusätzliche Windows-Gegenproben reproduzieren Fehler im unveränderten Produktcode: einen musikalisch inkonsistenten Pattern-Paste unter Audioverarbeitung, das Überschreiben einer fremden View-Änderung durch Velocity-Cancel und eine überlebende Stimme nach einem Zero-Frame-Stop. Ein weiterer Fehler der Herkunftspfad-Zuordnung ist durch den Quellcode belegt. Die gemeldeten Studio-One-Aussetzer beim Samplewechsel sind damit **nicht abschließend erklärt oder als behoben bestätigt**.

Es wurden **keine Produktkorrekturen**, Ressourcenänderungen, Versionsänderungen, Merges oder Releases vorgenommen. Die zusätzlichen Tests und dieser Bericht liegen ausschließlich auf dem Audit-Branch.

## 1. Identität, Quellen und Nachweisgrenzen

| Gegenstand | Tatsächlich geprüfter Stand |
|---|---|
| Repository | `challanger2000/125A-Drumator` |
| Beauftragter Reparaturbranch | `fix/studio-one-acceptance-20261022` |
| Geprüfter **Produktcode** | [`4f6a1318b11ab1d09c8132ad7f65517f3f6b0714`](https://github.com/challanger2000/125A-Drumator/tree/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714) |
| Zugehöriger PR | [#8](https://github.com/challanger2000/125A-Drumator/pull/8), offen; nicht gemergt |
| Ausgangspunkt der Reparaturen | `f00fac2f8ce6279cba159c22d6aea9b858f7b5c6` / zuvor Windows-geprüfter Programmstand `7598252828d709c23a5bed33756c085a4d052a08` |
| Vorhandene vollständige Windows-QA | [#39778743465](https://github.com/challanger2000/125A-Drumator/actions/runs/39778743465), Job `119824279897`, exakt auf `4f6a131…`, **SUCCESS** |
| Audit-Branch | `audit/codex-v1-final-independent-20261022` |
| Erster zusätzlicher Teststand | [`546cd4fb515a410f7ff2b70922dd94db6cdb5b64`](https://github.com/challanger2000/125A-Drumator/commit/546cd4fb515a410f7ff2b70922dd94db6cdb5b64) |
| Abschließender zusätzlicher Teststand | [`0833f8b069bdd1cae859a7eeeb438ad6409ba06d`](https://github.com/challanger2000/125A-Drumator/commit/0833f8b069bdd1cae859a7eeeb438ad6409ba06d) |
| Engineering-Grundlage | `125A-Engineering`, `origin/main` = `9854275cdad1d35476397d874ba95efc4f2b184a`; aktuelles START-HERE, Guardrails, einschlägige Standards, QA, Research und Roadmap gelesen; **verstanden** |
| Projektregeln | `AGENTS.md`, SDK-Baseline, V1-Scope, aktuelle Implementierung, Regressionen und Studio-One-Issue [#7](https://github.com/challanger2000/125A-Drumator/issues/7) |
| Zielplattform | Windows x64, MSVC Release, C++23, VST3 SDK 3.8.1, SDK-SHA `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` mit zugehörigen Submodulen |
| Tatsächliche Programmversion | Weiterhin `0.1.0` in CMake und `PluginIDs.h`; „V1.0.0“ im Auftrag ist keine bereits veröffentlichte Binärversion |

**Quellzeilen in diesem Bericht beziehen sich auf `4f6a131…`.** Testzeilen der neuen Datei beziehen sich auf `0833f8b…`. Die Audittest-Commits verändern ausschließlich vier Dateien unter `tests/integration/`. Ein Vergleich von `src/`, `resource/`, `CMakeLists.txt` und `.github/` gegen `4f6a131…` ergibt **keinen Unterschied**. `develop/v1.0.0` und `main` wurden nicht verändert.

Prüfmethode: systematischer Quellreview der eigenen Core-/VST3-Dateien und der vorhandenen Testtargets; Verfolgung von GUI-Aktion, Controller, Nachricht, Processor, Audio und State; Kontrolle der Testorakel; gezielte Gegenbeispiele mit tatsächlichen Klassen unter Windows. Bestehende Audit-PASS-Aussagen wurden nicht als Beweis übernommen.

Die neuen nativen Tests kompilieren die aktuellen Controller-, View- und Processor-Implementierungen gegen das echte SDK. Nur generierte **Testkopien** erhalten Beobachtungszugriffe und definierte Unterbrechungspunkte. Es gibt keine Ersatz-Sampler-/Controller-Implementierung. Diese Tests sind dennoch **keine Ausführung der paketierten DLL in Studio One**. Die Bibliotheks-/DLL-Hosttests des vorhandenen Laufs werden gesondert ausgewiesen.

Vollständige Actions-Logdownloads scheitern hier am verwalteten Downloadpfad mit „Forbidden“. Deshalb wurden Job-/Schrittstatus und die **tatsächlichen Checks-Annotations** abgefragt. Compilerdiagnosen oder nicht zugängliche CTest-Einzelresultate wurden nicht erfunden. Die hochgeladenen Artefakte wurden in diesem Audit nicht nach Download binär oder visuell unabhängig untersucht.

### Statusbedeutung

- **PASS:** Der ausdrücklich benannte Fall ist anhand geeigneten Quellcodes und tatsächlich ausgeführter Tests bestätigt.
- **FAIL:** Ein Fehler ist durch konkrete Ausführung oder einen eindeutigen Quellgegenbeweis belegt. Die Beweisart steht dabei.
- **OFFEN / NICHT NACHGEWIESEN:** Eine weitergehende Zusicherung fehlt oder das Ergebnis ist nicht abschließend.
- **NICHT TESTBAR:** In der verfügbaren Umgebung nicht ausführbar, insbesondere echtes Studio One/ASIO. Dies ist kein PASS.

Ein PASS eines begrenzten Falls bedeutet keine universelle Fehlerfreiheit des gesamten Teilbereichs.

## 2. Gesamtentscheidung und Reparaturstatus

**Ist die technische Grundlage jetzt zur V1-Freigabe bereit? NEIN.** Zunächst die bestätigten Fehler beheben, gezielt regressieren und anschließend die noch betroffenen Studio-One-Fälle prüfen. Der erfolgreiche Ausgangsbuild bleibt ein gültiger Nachweis seines damaligen Testumfangs; er widerlegt die zusätzlichen Gegenbeispiele nicht.

| Bereich | Ergebnis | Begründung |
|---|---|---|
| **SOR01 – neue Steps Velocity 100** | **PASS, normaler Step-Klick/Controllerpfad** | `Controller.cpp:821–823` setzt `0 → 100`, belegte Werte werden ausgeschaltet. Tatsächlicher Controller-Test prüft auch `37 → 0 → 100` und andere Rows/Patterns. Vorhandener View-Handler `View.cpp:1182–1194` führt den normalen Klick hierhin. Shift-Klick, FILL und der alte Factory-Rhythmus haben eigene, unveränderte Verträge; deren 127/32 sind kein Gegenbeweis zum normalen Klick. Kein neuer interaktiver Studio-One-Test behauptet. |
| **SOR02 – Navigation nach Projekt-Reopen** | **PASS für vollständigen Zwei-Stream-Recall; FAIL für sichere Herkunftszuordnung allgemein** | VFA01 zerstört Quellinstanzen und prüft in frischen Instanzen beide Reihenfolgen, alle vier Zooms sowie echte Prev/Next-Dateien. Component-only-Recall mit gleichem Basename kann einen falschen alten Ordner behalten: **V1A04**. Fehlende Originaldateien verhindern Embedded-Playback nicht, garantieren aber keine Navigation. |
| **SOR03 – Transport** | **FAIL, teilweise repariert** | Die neue Jump-Erkennung entfernt alte Stimmen bei beobachteter PPQ-Diskontinuität; vorhandener nativer Jump-Test besteht erneut. Zero-Frame-Stop vor zeitlich kontinuierlichem Restart wird dagegen übergangen: **V1A01**. Eine vollständige Behebung des Owner-Fehlers in Studio One ist nicht nachgewiesen. |
| **SOR04 – unterbrechungsfreier Samplewechsel** | **OFFEN / NICHT NACHGEWIESEN als DAW-Zusicherung** | 40-Wechsel-Test besteht seinen begrenzten Exchange-Fall. Zusätzlicher kompletter WAV-Import hält die unveränderte Row in vier kleinen Lastfällen samplegenau. Der Steuerthread bleibt synchron für Dateizugriffe, Decode, Kopien und gegebenenfalls Sinc-Vorbereitung belegt. Keine Aufnahme, Deadline-Spur oder erfolgreiche Nachabnahme des Owner-Fehlers in Studio One. |
| **DZM3 – Format und Legacy-Zoom** | **PASS für geprüfte Migration/Transaktion** | DZM1/DZM2 weiterhin lesbar; DZM3 mit vier Zoomcodes und 16 längenbegrenzten Pfaden. VFA01 prüft frische Zwei-Stream-Instanzen, alle Trunkierungen seines States, NUL, ungültigen Zoom, ersten Pfad-Allokationsfehler und Reopen. Legacy- und native DPI-/Ablehnungsfälle bestehen. Mehrfach-Resize-Recall wird weiterhin bewusst vollständig abgelehnt. |
| **Mehrfachfenster** | **FAIL im Edit-Protokoll, Teilfunktionen PASS** | Registrierung, Benachrichtigungen, Repaint, Paging und Reopen bestehen V18/SA01. Das gemeinsame Velocity-Stroke-Undo überschreibt unabhängige Änderungen: **V1A03**. Normale Benachrichtigung löst diesen Konflikt nicht. |
| **Multi-Out** | **PASS für vorhandene automatisierte Routingfälle** | Feste 16 Stereo-Busse, Aktivierung, freie Zuordnung, Summierung mehrerer Rows, Fallback auf MAIN und Routing-Recall geprüft. Paketiertes T02 nutzt 15 unterschiedliche AUX-Stereocodes mit Bus-/L/R-Vertauschungs-Negativen; T29 prüft AUX bei MAIN mit null Kanälen. Kein neuer 16-Out-Studio-One-Lauf durchgeführt. |
| **Pattern/Song/FILL/Undo** | **FAIL für gleichzeitigen musikalischen Paste; übrige benannte Regressionen PASS** | P18/P19, V16/V17, Song-Paging und Extremdeltas bestehen; Steps 17–32 bleiben in den geprüften Bearbeitungen erhalten. Daten und Dauer sind im Audio-Snapshot nicht gemeinsam atomar: **V1A02**. |
| **MIDI/Automation** | **PASS für benannte normalen und Pattern-Restore-Fälle; Gesamtzusage OFFEN** | Note-On 24–31 für A–H bei SONG OFF; 36–51 für One-Shots; Note-Off absichtlich ignoriert. T30, FA02 und O01 prüfen tatsächliche Processor-Ereignisse, Priorität, Offset/Zero-Frame/ABA und Feedback. Das ist kein Nachweis aller Host-Proxy-/GUI-Interleavings oder verlustfreier Drum-Note-Übernahme während eines gegateten Voll-Kit-Recalls. |
| **WAV/FLAC, Löschen, Namen, Preview, D&D** | **PASS für benannte Quell-/Windows-Fälle, weitere Grenzen OFFEN** | Decoder-/Fehlererhaltung, echte WAV-Dateinavigation, Preview-/Clear-Kommandos, nativer Namenseditor, tatsächliche View-Drop-Callbacks und Overlay-Sperren geprüft. Reales Studio-One-OLE-Browser-D&D und schwierige FLAC-/Dateisystemfälle sind davon getrennt. |
| **Studio One / ASIO / finale Klangabnahme** | **NICHT TESTBAR hier** | Keine eigene Studio-One-Ausführung und keine ASIO-Aufnahme. Owner-Beobachtungen in Issue #7 sind externe Befunde, kein Codex-PASS für diesen Reparaturstand. |

## 3. Bestätigte Produktfehler, nach Priorität

### V1A01 – HOCH – Zero-Frame-Stop wird ignoriert; alte Stimme läuft nach Restart weiter

**Status: FAIL, unter Windows mit tatsächlichem Processor reproduziert.**

**Quelle:** [`src/vst3/DrumatorProcessor.cpp:579`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/vst3/DrumatorProcessor.cpp#L579), besonders `642–655`, im Vergleich zu Transportverarbeitung `668–682` und Jump-Erkennung `737–758`. **Funktion:** `Processor::process`.

**Ursache:** Ein Block mit `numSamples == 0` bearbeitet Parameter und kehrt zurück, bevor `ProcessContext::kPlaying`, `wasPlaying_` und `haveExpectedPpq_` berücksichtigt werden. Meldet genau dieser Callback den Stop, bleibt das alte Playing-/Clock-/Voice-Modell erhalten. Wenn der nächste Playing-Block an `expectedNextPpq_` beginnt, erkennt auch die neue SOR03-Jump-Prüfung keinen Übergang.

**Konkrete Reproduktion VFA05:**

1. Echtes Kit mit leerem Pattern und geladenem Stereo-Sample `(+0,25; −0,5)`; 48 kHz, 256 Frames.
2. Playing-Block bei PPQ 4, einmal MIDI-Note 36; eine aktive Stimme.
3. `process` mit **null Frames**, explizit gestopptem Context und PPQ `4 + 256/24000`; keine weitere MIDI-Note.
4. Wieder Playing, 256 Frames, dieselbe PPQ – genau der zuvor erwartete nächste Blockanfang.

**Erwartet:** Der explizite Stop beendet die alte Stimme entsprechend der bereits vorhandenen Stop-Policy. Das leere Pattern und fehlende neue MIDI-Noten liefern beim Restart Stille.

**Gemessen:** Nach dem Stop noch **1 aktive Stimme**; Restart `L[80] = 0,250000`, `R[80] = −0,500000`. Ein Kontrollfall mit einem normalen 256-Frame-Stop beendet die Stimme und startet danach still. Der alte Sound ist folglich kein neuer Sequencerhit. Bei langen Samples kann die unerwünschte Fortsetzung wesentlich länger dauern als in dieser kurzen Fixture.

**Test:** [`FinalIndependentAuditCases.inl:117`](https://github.com/challanger2000/125A-Drumator/blob/0833f8b069bdd1cae859a7eeeb438ad6409ba06d/tests/integration/FinalIndependentAuditCases.inl#L117), Testname `VFA05 zero-frame Stop then continuous-PPQ restart`, zusätzlicher Windows-Lauf #39788692366: **FAIL**.

**Warum vorhandene Tests vorbeigingen:** Der neue SOR03-Coretest ruft den zustandslosen `StepScheduler` mit einem gestoppten Clock auf. Er kann das persistente `Processor::wasPlaying_` nicht prüfen. T25 verwendet einen Stop mit einem vollen Audioblock; der neue Jump-Test erzwingt PPQ-Diskontinuität. Beide umgehen genau diesen Gegenfall.

**Empfohlene Reparatur:** Explizite gültige Transportübergänge auch vor den Zero-Frame-Rückgaben im zugelassenen Audio-Gate-Pfad verarbeiten. Für Busy-Gate-Fälle eine klar definierte, sichere Übergabe des Stop-Ereignisses vorsehen; dort nicht ungeschützt auf die gerade installierten nichtatomaren Audiostrukturen zugreifen. Keine generelle Phasenrücksetzung bei jedem Flush und kein Stop allein aufgrund eines fehlenden optionalen Contexts einführen.

**Erforderliche Regressionen:** Null Frames mit/ohne Output-Bus, expliziter Stop/fehlender Context, kontinuierlicher und gesprungener PPQ-Restart, lange Stimmen, SONG ON/OFF, 44,1/48 kHz und 128/256/512 Frames; normaler Stop als Positivkontrolle und Stille ohne neue Trigger als Negativorakel.

### V1A02 – HOCH – neuer Patterninhalt wird mit alter musikalischer Dauer gerendert

**Status: FAIL, deterministisch erzwungenes Interleaving des tatsächlichen Controllers und Audioprozessors.**

**Quellen:** [`DrumatorProcessor.cpp:344`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/vst3/DrumatorProcessor.cpp#L344), insbesondere `360–365`, Audio-Längenread `764–766`, späterer Patternread `772–796`; [`src/core/AtomicPatternBank.h:37`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/core/AtomicPatternBank.h#L37); `Controller::replacePattern`, `Controller.cpp:413–437`.

**Ursache:** Der Writer serialisiert Steps und Dauer zwar für den **Stateexport** mit `kitMutex_`, der Audiothread verwendet diesen Mutex aber zu Recht nicht. Er liest die Dauer einmal separat, danach die Steps mit einer eigenen Pattern-Generation. Die Dauer gehört nicht zu dieser Generation und auch nicht zum Audio-Patterncache. Eine neue zusammenhängende musikalische Änderung kann so mit einer alten Dauer erklingen. Stärkere Speicherreihenfolge einzelner atomarer Loads/Stores allein macht daraus keine gemeinsame Transaktion.

**Reproduktion VFA02:** Pattern A bisher 32 Steps und still. Pattern B: 16 Steps, Step 1 Velocity 100, gespeicherter inaktiver Step 17 Velocity 50. B kopieren, A wählen. A über einen vollständigen ersten Takt kontinuierlich rendern. Im nächsten Block pausiert nur die generierte Testkopie direkt nach dem Längenread. Nun echten Controller-Paste B→A vollständig abschließen und Audio fortsetzen.

**Erwartet:** Zulässig ist eine vollständige alte Generation (in der Fixture Stille) oder eine vollständige neue Generation (1T, Step 1 mit 100). Ein vorhandener Hidden-Step darf nicht durch eine gemischte Generation hörbar werden.

**Gemessen:** Gespeicherter tatsächlicher State ist korrekt `Länge=16`, Step 1=100, Hidden-Step 17=50. Im Audio entsteht aber `L[80] = 0,196850`, exakt `0,5 × 50/127`. Die neue 1T-Generation hätte `0,393701`; die alte Generation wäre still. **Keine der beiden vollständigen Generationen erklärt das gemessene Ergebnis.**

**Test:** [`FinalIndependentAuditCases.inl:80`](https://github.com/challanger2000/125A-Drumator/blob/0833f8b069bdd1cae859a7eeeb438ad6409ba06d/tests/integration/FinalIndependentAuditCases.inl#L80). Erster und abschließender Windows-Lauf: **FAIL**. Im abschließenden Testorakel sind ausdrücklich sowohl die vollständige alte als auch die vollständige neue Generation erlaubt. Die kontrollierte Warteoperation existiert ausschließlich in der generierten Testkopie und ist keine behauptete Produktions-Echtzeiteigenschaft.

**Warum vorhandene Tests vorbeigingen:** P19 prüft gespeicherte Cells und Dauer nach abgeschlossenen Commands; T03 kontrolliert nur die Stepbank-Generation. Keine dieser Prüfungen nimmt die musikalische Dauer in denselben Audio-Snapshot auf.

**Empfohlene Reparatur:** Dauer und Steps desselben Patterns unter einem gemeinsamen Versions-/Snapshotvertrag publizieren und lesen. Der Audio-Fallback muss ebenfalls die dazugehörige Dauer behalten. Reine PatternLength-Änderungen, kombinierte PatternReplace-Nachrichten und Song-Lesewege müssen denselben Vertrag beachten. Kein blockierender Audiomutex und keine Callback-Allokation.

**Erforderliche Regressionen:** Erzwungene Reads vor/während/nach 16→32 und 32→16, normale Daueränderung, Copy/Paste/Duplicate/Undo bei laufendem Audio, Song und Patternmodus, unabhängige Stereo-/Velocity-Signaturen. Nur ganze alte/neue Generationen akzeptieren; gemischte Kontrollgeneration muss sicher scheitern.

### V1A03 – MITTEL – Velocity-Cancel überschreibt bestätigte Änderungen einer anderen View

**Status: FAIL im tatsächlichen Controller-/View-Protokoll. Praktische Erreichbarkeit unter der konkreten Studio-One-Mauscapture ist separat OFFEN.**

**Quellen:** [`DrumatorController.cpp:617`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/vst3/DrumatorController.cpp#L617), insbesondere `624–634` und `656–658`; `Controller.h:125–131`; [`DrumatorView.cpp:1281`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/vst3/DrumatorView.cpp#L1281), außerdem `324–325` und `1273–1278`.

**Ursache:** Ein einziges Controller-weites Stroke-Snapshot enthält das gesamte Pattern. Es besitzt einen Kit-Generationstoken, aber keinen View-/Gesture-Eigentümer und keine Konflikterkennung für gewöhnliche zwischenzeitliche Edits. Cancel schreibt den vollständigen alten Snapshot zurück. Gewöhnliche Änderungen desselben Patterns während des Strokes werden zudem nicht einzeln in die Undo-Historie aufgenommen.

**Reproduktion VFA03:** Zwei echte angehängte VSTGUI-Views mit demselben Controller. Erste View beginnt Velocity-Geste auf Row 1. Zweite View aktiviert über ihren tatsächlichen Step-Handler einen zuvor leeren Step in Row 2. Controller **und gespeicherter Processor-State** enthalten dort 100. Dann `onMouseCancel` der ersten View.

**Erwartet:** Die erste Geste darf ihre eigene Velocity zurücknehmen, aber nicht den unabhängig bestätigten Step der zweiten View löschen.

**Gemessen:** Fremder Step **100 → 0**; eigene Velocity korrekt zurück auf 60. Die Löschung gelangt über PatternReplace auch zum Processor. Es handelt sich um einen Datenverlustpfad, nicht nur um fehlenden Repaint.

**Test:** [`FinalIndependentAuditCases.inl:153`](https://github.com/challanger2000/125A-Drumator/blob/0833f8b069bdd1cae859a7eeeb438ad6409ba06d/tests/integration/FinalIndependentAuditCases.inl#L153), beide zusätzlichen Windows-Läufe: **FAIL**.

**Beweisgrenze:** Die Testfenster sind tatsächlich nativ angehängt; Mouse-/Cancel-Handler werden gezielt direkt aufgerufen. Die normale OS-Mauscapture kann bestimmte manuelle Klickreihenfolgen verhindern oder früher canceln. Ein exakt so durchgeführter Studio-One-Bedienversuch wurde nicht behauptet. Das gemeinsame Edit-Protokoll selbst nimmt die gültigen Aufrufe jedoch an und verliert den bestätigten Fremdedit; es verfügt über keine eigene Sicherung.

**Empfohlene Reparatur:** Eindeutige Gesture-/View-Eigentümerschaft und Konfliktregel festlegen. Beispielsweise eine aktive Geste sicher abschließen, bevor ein fremder Edit angenommen wird, und verspäteten Cancel des alten Eigentümers ablehnen; alternativ nur die tatsächlich eigenen geänderten Cells zurücknehmen. Kitgeneration allein genügt nicht. Undo-Semantik für weitere Views mitprüfen.

**Erforderliche Regressionen:** Peer-Step/Fill/Paste zwischen Begin und Cancel/Close, zwei nacheinander begonnene View-Gesten, verspäteter Cancel, Erfolgs-/Fehl-Recall, tatsächlich über Win32 zugestellte Fokus-/Capturewechsel. Unabhängige Edits dürfen weder gelöscht noch irrtümlich in fremdes Undo aufgenommen werden.

### V1A04 – MITTEL – gleicher Dateiname wird fälschlich als identische Sampleherkunft behandelt

**Status: FAIL durch eindeutigen Quellgegenbeweis; noch kein zusätzlicher Windows-Gegenfall ausgeführt.**

**Quelle:** [`DrumatorController.cpp:187`](https://github.com/challanger2000/125A-Drumator/blob/4f6a1318b11ab1d09c8132ad7f65517f3f6b0714/src/vst3/DrumatorController.cpp#L187), `setComponentState`, `190–196`; Navigation `825–855`.

**Ursache:** Ein bestehender Ursprungspfad bleibt beim neuen Component-State erhalten, wenn allein sein Basename mit `candidate.names[row]` übereinstimmt. Weder eingebettete WAV-Bytes noch eine Asset-/Restore-Identität werden verglichen. Gleicher Basename bedeutet bei Drum-Sample-Bibliotheken häufig unterschiedliche Dateien.

**Konkreter Quellgegenfall:**

1. Aktuelles Kit X stammt aus `C:/Library-X/kick.wav`; dieser Pfad steht in `samplePaths_[0]`.
2. Ein anderer gültiger Component-/Kit-State Y enthält ebenfalls den Namen `kick.wav`, aber andere eingebettete WAV-Daten aus Library Y.
3. Host/Preset lädt Y über `setComponentState`, ohne einen passenden Controller-State nachzuliefern. Controller-State ist ein separater Zustand; Component-only-Recall darf keine falsche Herkunft erfinden.
4. Der Vergleich in Zeile 195 ist gleich; der alte Library-X-Pfad wird **nicht** geleert. `<`/`>` navigiert folglich weiter in Library X.

**Erwartet:** Y darf eine unbekannte Herkunft haben und trotzdem eingebettet spielen. Eine nicht belegte Herkunft aus X darf nicht als Herkunft von Y erscheinen.

**Tatsächliches, aus der Bedingung zwingendes Verhalten:** Falscher alter Ordner bleibt erhalten. Embedded-Audio wird beim Recall nicht verloren; erst Navigation lädt aus dem falschen Zusammenhang. Bei vollständigem passenden DZM3-/Component-Stream-Paar kann der anschließend restaurierte richtige Pfad das Problem überdecken.

**Warum vorhandene/zusätzliche Tests vorbeigehen:** Die SOR02-Tests laden dasselbe Kit erneut. Auch VFA01 prüft absichtlich vollständige zueinander passende Zwei-Stream-Projekte. Beides kann eine inkorrekte Identitätsheuristik bei einem anderen Component-only-Kit nicht erkennen.

**Empfohlene Reparatur:** Herkunft an eine belastbare Sample-/Restore-Identität binden; unbestätigte alte Herkunft beim Wechsel anderer eingebetteter Daten verwerfen. Dabei die bereits bestandenen beiden Restore-Reihenfolgen erhalten. Nicht einfach beide Streams in einer festen Reihenfolge voraussetzen und keine Dateisystemzugriffe in den Audio-Callback verlagern.

**Erforderlicher Regressionstest:** Zwei existierende Ordner mit gleichnamigen, unterschiedlich codierten WAVs und unterschiedlichen Nachbarn; X laden, Component-only-Y restaurieren; falsche Navigation ausschließen, Y-Audio erhalten. Danach vollständige passende DZM3-Recalls in beiden Reihenfolgen, gleiche Assetdaten, andere Daten, fehlende Dateien und WAV/FLAC-Namen abdecken.

## 4. Sampleimport und Echtzeitsicherheit – vollständiger Pfad

### 4.1 Tatsächlicher Ablauf

1. **View/OLE/Dateiauswahl:** `View.cpp:106–252` extrahiert einen echten UTF-8-Pfad; `313–319` sperrt überdeckte Ziele; `825–843` ordnet die sichtbare Row zu. `load` bei `846–861` öffnet den Dateidialog modal. `extractSamplePath` fragt bereits bei Drag-Enter/Move `is_regular_file` ab: auch dieser UI-Pfad kann auf langsamen Laufwerken blockieren.
2. **Controller-Datei-I/O:** `Controller.cpp:860–885` prüft Extension/Dateigröße und liest die gesamte Datei synchron. Grenzen: 16 MiB pro Nachricht/Datei. Bei FLAC erfolgt zunächst die Konvertierung in `886–890`.
3. **Erster WAV-Decode:** `892–894` dekodiert den kompletten WAV-Puffer zur Vorprüfung in planar float. Dieser PCM-Puffer wird nicht an den Processor übergeben und bleibt bis zum Ende von `loadWav` lebendig.
4. **Nachrichtenkopie:** `907–921` erzeugt die VST3-Nachricht und schreibt `WavBytes` binär. Der hier verwendete SDK-HostAttributeList-Pfad hält die Daten als eigene Kopie; ein echter Host darf eine Proxy-Kommunikation einsetzen. VST3 `IConnectionPoint::notify` ist laut gepinntem `ivstmessage.h:85–87` ein UI-Thread-Aufruf.
5. **Processor-Kopie und Vorbereitung:** `Processor.cpp:408–443` kopiert die Originalbytes nochmals für den späteren Projekt-State. Unter dem **control-only** `kitMutex_` liest er die aktuelle Stimmung und ruft `PreparedSample::fromWav` auf.
6. **Zweiter vollständiger WAV-Decode:** `SampleExchange.h:41–50` dekodiert denselben WAV-Inhalt nochmals; danach entsteht das ratioabhängige Sinc-Kernel. FLAC durchläuft vorher zusätzlich Decoder, Zwischen-float und WAV-Erzeugung (`FlacImport.cpp:23–79`).
7. **Veröffentlichung:** `SampleExchange.h:131–150` übergibt den fertig vorbereiteten unveränderlichen Zeiger atomar; ein noch nicht übernommener Pending-Zeiger kann auf der Steuerseite zurückgezogen werden. Controller ändert Labels/Pfad erst nach akzeptierter Nachricht.
8. **Audioblock:** `Processor.cpp:690–691` übernimmt komplette Assets. `audioAdopt` dereferenziert Pending-Pointer erst nach atomarer Besitzübernahme. Alte Voices behalten Sample-/Kernel-Zeiger und Owner-Token. Normale einzelne Samplewechsel rufen **weder** `voices_.reset()` **noch** das Voll-Kit-Restore-Gate auf.
9. **Freigabe:** `SampleExchange.h:195–214` gibt nicht mehr referenzierte Assets über eine begrenzte SPSC-Queue zurück. Tatsächliches Löschen und die Pending-/WAV-/PCM-Destruktion erfolgen auf der Steuerseite, nicht in `process`. Bei Queue-/Retire-Druck wird Übernahme auf spätere Blöcke verschoben.

### 4.2 Bewertung der Unity-Optimierung

`SampleVoiceEngine.h:41–45` spart die Kernelvorbereitung ausschließlich bei **exakt** `playbackRatio == 1.0`. Im Renderer verzweigt `218–224` dann auf direkte PCM-Lesezugriffe. Stimmen starten bei Position null und schreiten bei Ratio 1 ganzzahlig fort. In den tatsächlich erreichbaren Samplegrößen ist das numerisch konsistent; der Unity-PCM-Test prüft Stereo/Mono, Beginn und Fortsetzung.

**PASS für diesen begrenzten Optimierungsvertrag.** Kein neuer Zugriff auf eine leere Koeffiziententabelle im tatsächlichen Unity-Renderer gefunden. Bei anderem Source-/Host-Rate-Verhältnis oder Pitch bleibt die Sinc-Vorbereitung aktiv. Die Optimierung beseitigt weder Datei-I/O noch doppelten WAV-Decode oder Nachrichten-/Statekopien. Deshalb ist „Samplewechsel jetzt generell aussetzerfrei“ daraus nicht ableitbar.

### 4.3 Zusätzlicher kompletter Importtest VFA04

[`FinalIndependentAuditCases.inl:170`](https://github.com/challanger2000/125A-Drumator/blob/0833f8b069bdd1cae859a7eeeb438ad6409ba06d/tests/integration/FinalIndependentAuditCases.inl#L170) schreibt echte Stereo-float-WAVs mit **16.777.212 Bytes** (vier Bytes unter 16 MiB), Source-Raten 44,1/48 kHz. Echte `Controller::loadWav`-Aufrufe importieren diese bei Output 44,1/48 kHz und Row-Pitch 0/+12. Parallel rendert ein eigener Actor den tatsächlichen Processor mit wechselnden 64/128/256 Frames. Ein zweiter tatsächlicher Processor ohne Imports dient als unabhängiger Kontinuitätsvergleich für eine unveränderte hörbare Row. Eine beschädigte Ersatz-WAV wird abgelehnt, ohne ihren Herkunftspfad zu ersetzen.

Der Actor verwendet synthetisches `sleep_until`-Pacing. Gemessen wird die Laufzeit von `process`, nicht die Wartezeit. C++-`operator new`-Aufrufe auf diesem Actor werden nur über den eigentlichen produktiven `process`-Aufruf gezählt. Weder Timer noch Ergebnisformatierung werden in den Produktcode eingebaut.

**Abschließender Lauf #39788692366, tatsächlich gemessene Werte:**

| Output / Pitch | Import Source 44,1 kHz | Import Source 48 kHz | Audioblocks | PCM-Abweichungen / C++-new in process | process p95 / p99 / Max |
|---|---:|---:|---:|---:|---:|
| 44,1 kHz / 0 st | 43,2468 ms | 46,5844 ms | 26 | 0 / 0 | 0,0478 / 0,0678 / 0,0678 ms |
| 44,1 kHz / +12 st | 45,9508 ms | 45,9631 ms | 28 | 0 / 0 | 0,0378 / 0,0497 / 0,0497 ms |
| 48 kHz / 0 st | 45,2764 ms | 45,0819 ms | 30 | 0 / 0 | 0,0227 / 0,0315 / 0,0315 ms |
| 48 kHz / +12 st | 45,3556 ms | 46,0747 ms | 30 | 0 / 0 | 0,0162 / 0,0166 / 0,0166 ms |

Der erste zusätzliche Lauf #39787836364 lieferte bei identischem Produktcode rund **79,9–86,1 ms** pro Import; auch dort keine PCM-Abweichung und kein beobachteter C++-new-Aufruf im Callback. Diese Unterschiede zwischen Runs werden **nicht** als Leistungsverbesserung durch einen Codefix ausgegeben.

**PASS:** In diesen kurzen nativen WAV-Fällen geht keine Probe der unveränderten hörbaren Row verloren. Der reale Importpfad umfasst Controller, Decode, IMessage, Processor-Vorbereitung und Adoption.

**NICHT NACHGEWIESEN:** Der Test misst keine ASIO-Deadline, keinen UI-/Host-Proxy-Handshake in Studio One, keine Windows-Scheduler-Lateness und keine Ausfallquote eines Audiointerfaces. Er enthält nur 26–30 kleine Lastblocks je Fall; p99 ist bei so wenigen Messpunkten der größte Wert. Es wird eine Row ersetzt, während **eine andere** Row klingt; eine hochpolyphone, gepitchte Ersatz-Row und deren neue Hits sind hier kein vollständiger Abnahmefall. Es werden keine `malloc`-/OS-/Host-internen Allocatoren oder sämtliche Freigaben gehookt. FLAC, langsame Datenträger und alle negativen Pitches sind in diesem neuen Timingtest nicht enthalten.

### 4.4 Antwort auf die entscheidende Aussetzerfrage

**Kann der einzelne Samplewechsel intern den Audiothread auf einen Datei-/Decode-Mutex warten lassen?** Im geprüften eigenen Callbackpfad wurde dafür kein direkter Aufruf oder Lock gefunden; der normale Austauschpfad ist dafür getrennt. VFA04 bestätigt die Trennung in seinen Fällen.

**Kann er die Host-Steuerung/Kommunikation lange synchron beschäftigen? JA.** Der Controller lädt und dekodiert synchron, `sendMessage` muss den Processor-Pfad einschließlich zweitem Decode und Kernelvorbereitung abarbeiten; Steuer-Mutex, große Kopien und Freigaben gehören dazu. Gemessen wurden ungefähr **43–86 ms** für nahe am Limit liegende WAVs in diesen CI-Runs. Ein konkreter Studio-One-Proxy kann zusätzlichen Aufwand haben; der Test benutzt direkte Komponentenverbindung.

**Beweist dies den vom Owner gehörten Dropout oder dessen Behebung? NEIN.** Zwischen UI-Blockierung und tatsächlichem ASIO-Aussetzer muss der Host-/Scheduler-/Speicherdruckpfad gemessen werden. Aus den kleinen Callbackzeiten im Test folgt keine Garantie. Aus 43–86 ms Steuerlatenz folgt umgekehrt nicht, dass genau diese Zeit als Audio-Stille auftrat.

**Empfehlung R01, hohe Nachweispriorität:** Den Owner-Fall mit laufender Aufnahme und protokollierten Blockzeiten/Overruns sowie Ladezeit reproduzieren. Dateilesen, Controller-Vorprüfung, Nachrichtenkopie, Processor-Decode, Kernel und Reclaim separat außerhalb von `process` messen. Erst danach gezielt doppelte Arbeit reduzieren bzw. eine begründete Loader-/Kommunikationsänderung planen. Keine spekulative Thread-Architektur-Reparatur aus diesem Audit ableiten.

### 4.5 Weitere RT-/Speichergrenzen

- **Voll-Kit-Recall ist ein anderer Pfad:** `Processor.cpp:1043–1047` öffnet das Busy-Gate **vor** `publishRestoredKit`; diese Funktion erstellt alle neuen Assets einschließlich Decode/Kernel. Währenddessen geben Audiocallbacks absichtlich Stille aus. Auch ein später abgelehnter Prepare-Versuch kann vorher Gate-Stille erzeugen. Alte Kitdaten können dabei logisch erhalten bleiben, ohne dass die Audiowiedergabe lückenlos war. Das ist kein Beweis für einen Gatefehler beim normalen einzelnen WAV-Wechsel.
- Gate-denied-Blöcke erhalten Pattern-Automation/MIDI-A–H, aber sammeln die eigentlichen Drum-Note-Ons 36–51 nicht für später. Die Zusage „kein MIDI-Ereignisverlust unter allen Zustandswechseln“ ist somit **nicht** belegt. Für unterbrechungsfreien Live-Kitwechsel müsste dieser Vertrag gesondert entschieden und getestet werden.
- Source-WAV-/Kitlimits begrenzen gespeicherte Bytes, nicht den gesamten RAM. Beispiel, **abgeleitet, nicht Peak-RAM-gemessen**: Nahe 16 MiB PCM8-mono expandieren zu annähernd 64 MiB float-PCM; doppelte Decodes können davon zwei Puffer gleichzeitig halten, zusätzlich Original-/Message-/Statekopien und bisherige Assets. Ausklingende alte Samplegenerationen bleiben absichtlich länger erhalten. Der 64-MiB-Kitdeckel ist keine 64-MiB-Prozess-RAM-Garantie.
- Pitchänderungen bereiten die gesamte Row erneut aus ihren WAV-Bytes vor (`Processor.cpp:273–297`). Bei laufenden Mausgesten kann wiederholter Decode-/Kernelaufwand die UI belasten. Keine diesbezügliche Studio-One-Latenzqualifikation vorhanden.
- Der Callback nutzt eigene feste Voice-/Trigger-/Retire-Strukturen und keine eigenen Datei-/Decoderaufrufe oder blockierenden Mutex-Scopes. Dennoch sind lockfreie CAS-Schleifen kein formaler Wait-free-Beweis, und Callbackkosten hängen auch von Host-Ereignisdichte und externen Queue-Implementierungen ab.
- Dichte Host-Patternautomation bei laufendem Sequencer erzeugt viele `scheduleRange`-Aufrufe. In Songmode können diese jeweils mehrere Pattern-Snapshots lesen. Ein vollständiger Deadline-Nachweis bei dieser Last zusätzlich zu 64 Stimmen/hoher Ratio fehlt.

## 5. DSP, Routing, Zustand und Bedienoberfläche

### 5.1 Audioengine

Geprüft wurden Ratio aus Source-/Output-Rate und `exp2(st/12)`, Ratio-Limit 16, tapabhängige Sinc-Tabellen (32/64/128/256), Phasenwahl, Zero-Padding an Grenzen, Integer-Unity-Pfad, endliche Decoderwerte, Stereo-Balance, 64 Voices, beschränkte Choke-Fades und Retire-Besitz.

Die vorhandenen Tests unterscheiden unter anderem exaktes Unity-PCM, 2x-Hoch-/Niederfrequenz, einen 16x-Stopbandpunkt, Stereo/Choke, verworfenen Choke bei vollen 64 Voices, Blockpartition und Lifetime beim Sampletausch. SA02 führt weiterhin den alten Sortierkomparator als ausschließlich testseitige Negativkontrolle aus; auch in den Auditläufen reproduziert dieser bei 16 gleichzeitigen MIDI-Hits die alte PCM-Divergenz. Der korrigierte `(offset,row,arrival)`-Comparator besteht seine Fälle erneut.

**PASS für diese Fälle. OFFEN für vollständige Resampling-Klangqualität:** Es fehlen eine breitbandige, unabhängig ausgewertete Rate-/Pitch-Matrix, Impuls-/Transienten-/Passbandmessung und feste reale Drums für eine musikalische Abnahme. Zwei Tontests sind keine vollständige Aliasing-Qualifikation.

Bestehende Stimmen behalten ihre Launch-Werte für Gain/Pan/Output; Mute/Solo filtert neue Trigger. Der Voice-Test verlangt sogar ausdrücklich, dass Mute ausklingende One-Shots nicht tötet. Diese bewusst getestete Semantik wurde hier nicht heimlich als „Mute stoppt alle laufenden Samples sofort“ umgedeutet. Falls diese andere Produktsemantik gewünscht wird, ist das vor einer Korrektur zu entscheiden.

### 5.2 Multi-Out

`Processor.cpp:36–86` deklariert 16 Stereo-Busse, zusätzliche Busse als `kMain` und initial nicht aktiv; `setBusArrangements` akzeptiert nur die tatsächlich angebotenen 16 Stereo-Arrangements. Aktivierung ist davon unabhängig. `900–914` reicht nur verfügbare aktive Stem-Pointer an die Engine weiter. `SampleVoiceEngine.h:265–271` schreibt entweder in den zugeordneten Stem oder in MAIN; kein absichtliches doppeltes Signal.

Die Outputzuordnung ist beim Voice-Start festgehalten; mehrere Rows dürfen denselben Bus summieren. Die Table wird im Kit gespeichert/restauriert und im Controller angezeigt. Die bereits in Issue #7 beschriebenen Studio-One-Erfolge MAIN/OUT02/OUT05 sind Owner-Nachweise auf dem dort benannten Stand. Für den jetzigen Stand wurde daraus kein neuer persönlicher DAW-Test gemacht.

### 5.3 Sequencer, Pattern und MIDI

Der Scheduler leitet Positionen aus absoluter PPQ ab, arbeitet mit halboffenen Blocks und berechneter Rundungstoleranz; Swing verzögert ungerade Sechzehntel, Humanize arbeitet reproduzierbar. Coretests vergleichen 44,1/48/96/192-kHz-/Tempo- und Blockpartitionen, kombinierte Swing/Humanize-Offsets und Song-Vorlauf. Der Processor setzt eine transportbezogene Phase, erkennt PPQ-Jumps und übernimmt geordnete Host-/MIDI-Patternpunkte.

Die musikalische Interpretation lautet: 16/32 aktive Steps pro Pattern; gespeicherte inaktive zweite Takte werden beim Edit nicht pauschal gelöscht. Song-Einträge haben 8/16/32 Steps und mappen gegen die jeweilige Patternlänge. Das sind geprüfte konkrete Verträge, keine allgemeine DAW-Loop-Freigabe. V1A01/V1A02 bleiben Gegenbeispiele.

Pattern-Automation verwendet weiterhin `lround(clamp(v,0,1)*7)` und Controllertext A–H. Die stabilen IDs bleiben erhalten; im aktuellen Plugin ist **Pattern** der exponierte VST3-Parameter. Die übrigen Custom-Knöpfe arbeiten über persistierte Edit-Nachrichten. Nicht vorhandene VST3-Automationsparameter für jeden Mixerknopf werden nicht als getestet ausgegeben. MIDI-OUT des Sequencers ist laut Issue #7 eine gewünschte neue Funktion, kein bereits implementierter bestehender Ausgang.

### 5.4 Projektzustand und Restore

Aktuelles Component-Format ist **DM15**, Controller-Format jetzt **DZM3**. Das eine ersetzt das andere nicht. KitCodec hält Größen-/Längen-/Werteprüfungen, Hash, WAV-Validierung und Prepare-before-Commit ein; aktuelle und ältere Formate werden kontrolliert migriert. Vorhandene Kit-State-Tests rekonstruieren die alten Layouts, korrigieren Größen/Hashes und prüfen tatsächliche Decodeergebnisse. Nicht jeder Legacy-State wird dabei zusätzlich in einem echten alten Studio-One-Song geöffnet.

Controller-Namen werden vor dem Modelcommit vorbereitet; Kitgeneration invalidiert alte Velocity-/Texttokens bei erfolgreichem Recall. Native Fokus-/HWND-Abschlüsse und injizierte Controller-Allokationsfehler werden in Z01/Z03 tatsächlich ausgeführt. Fehlender oder korrupter Kandidat darf keine erfolgreiche Modellübernahme vortäuschen. Die neuen DZM3-Pfade werden vor dem Zoomcommit eingelesen und bei Fehlern nicht partiell geswappt. V1A04 ist eine **Identitätslücke**, keine Verletzung dieser Speichertransaktion.

**O01:** Die tatsächlichen Processor-Tests prüfen Hostpunkte vor/während Restore, gleiches Pattern und ABA, mit Audio und Zero-Frame. FA02 ergänzt MIDI-Patternpriorität und Busy-Gate-Fälle. Diese Fälle bestehen erneut. Sie synchronisieren den Controller für ihren Endvergleich gezielt mit dem vom Processor zurückgelesenen Endstate. Ein realer Host, der gleichzeitig Controller-Parameterfeedback und einen ursprünglichen Component-State auf anderer Reihenfolge zustellt, ist dadurch nicht vollständig simuliert: **OFFEN**, kein universeller Host-/GUI-Restore-Race-PASS.

**Weiterer offener Reentranzfall R02:** `sendEditMessage` meldet `setDirty(true)` bereits in `Controller.cpp:65–70`, bevor der aufrufende Controller z.B. bei `loadWav:926–928` eigene Labels/Pfade abschließend umgestellt hat. Ein reentrant sofort speichernder Host könnte zwischen neuem Processor- und altem Controller-State lesen. Das ist hier **nicht dynamisch reproduziert** und wird als Risiko, nicht als zusätzlicher bewiesener DAW-Fehler gewertet. Ein echter `IComponentHandler`/`IComponentHandler2`-Callbacktest mit Speichern während setDirty fehlt. Eine spätere Reparatur sollte Hostbenachrichtigungen grundsätzlich nach vollständig konsistentem Commit prüfen.

### 5.5 GUI-/Control-Trace-Matrix

Die folgende Matrix beschreibt die aktuelle kompakte View mit vier Rows/Seite, nicht eine alte 16-Row-Layoutidee. Sie ist Quell-/Testtraceability, **keine behauptete vollständige visuelle Studio-One-Abnahme**.

| Sichtbares Element / Eingabe | Tatsächlicher Weg / State | Nachweis und Grenze |
|---|---|---|
| A–H / Shift+A–H; 1 TAKT / 2 TAKTE | `View.cpp:946–973` → Patternparameter bzw. PatternLength | Nativer Direktlängenfall, Hosttext, T01/T28/T30 und gespeicherte Längen; V1A02 schränkt gleichzeitige Transaktionen ein |
| PATTERN: Kopieren, Einfügen, Löschen, Rückgängig, Velo variieren, Velo100, Duplizieren | `883–893` → entsprechende Controller-Patterntransaktion | P18/P19, V16/V17; ein Undo-Level pro Pattern, Copy/Paste inklusive Dauer; kein Überschreiben belegter Duplicate-Ziele |
| SONG ON/OFF, SONG EDIT, Seitentasten, Eintrag, Patternwahl, 1/2/2-Taktlänge, +/- Eintrag | `957–965`, `993–1053` → SongConfig | Echte View-Song-/Compacttests und S09/S10; UI-Auswahl ist Editziel, kein separat nachgewiesener Audio-Playhead |
| ZOOM-Menü 100/120/130/150 | `874–882` → checked Editor-Zoom → preferredZoom | DZM3/Legacy-/DPI-/Hostrefusal-/Reopen-Tests; geerbtes SDK-Kontextmenü durch leere allowedZoomFactors deaktiviert |
| SWING, HUMANIZE, MASTER mit Ctrl-/Doppelklick-Reset und Shift-Feindrag | `975–990`, `1228–1237` → Controller-Edits | Precisiontest, Extremdeltas, Master-Ramp G01/G02; kein neuer DAW-Automations-PASS für Customknöpfe |
| Row-Nummer/Auswahl, VELO auf/zu, Seiten < >, +ZEILE/-ZEILE | `1056–1138` → lokaler Viewzustand / VisibleRows | Compactfallback, SA01, V18; Guard für befüllte letzte Row; keine neue scrollbarbasierte Oberfläche implementiert |
| Sample-Text, Shift-Umbenennen, Alt-Löschen; „…“-Menü | `1140–1149`, `914–924` → load/rename/clear | Native Textedit-/Recalltests, echte WAV-Imports, Clear-Nachricht; Dateidialog tatsächlich modal, kein eigener Studio-One-Klicktest |
| Sample < > / PLAY | `1151–1163` → stepSample / PreviewRow | N12/VFA01 echte Dateien und tatsächlicher Processor; Preview soll Host nicht dirty markieren |
| Row FILL: alle neun Varianten einschließlich CLEAR | `929–944` → fillRow/PatternReplace | V17 liest gesamte Processor-Grid zurück und prüft Undo sowie versteckte Steps |
| M, S, OUT MAIN/02…16, CHOKE0…4 | `1172`-Umfeld sowie Popuphandler `895–912` → RowGate/RowOutput/RowChoke | SA01, Precision, Choke-/Routing- und Recallfälle; Output-/Choke-Popupgrenzen gegen Zeichnung verglichen |
| VOL, PAN, PITCH, RANDOM VEL / bei Altprojekten RANDOM RT | `1197–1220` → RowMix/RowPitch/Random | One-shot ändert nur bestehende aktive Notes; Legacy-RT bleibt bei nichtnull sichtbar; Precision-/Random-/Hidden-Bar-Tests |
| 32 Step-Tasten pro Row, davon 16 ggf. inaktiv | `1182–1194` → toggle/cycleStepVelocity | Normal-Klick jetzt100; Shift-Zyklus separat; inactive Steps werden nicht versehentlich editierbar |
| Velocity-Lane, Ctrl-Erase, MouseUp/Cancel/Close | `1105–1128`, `1239–1287` → Stroke/StepEdit | Recallgeneration und One-Gesture-Undo geprüft; **V1A03** entdeckt fehlende Peer-Konfliktregel |
| Drop auf Samplefeld | `getDropTarget() = this`, `313–319`, `825–843` → loadWav | Echte View-Drop-Callbacks, WAV-Pfade/Fehler und Overlaytests; nicht gleichbedeutend mit realem OLE-D&D aus Studio-One-Browser |
| Statuszeile und Anzeige deaktivierter/ausgewählter Controls | `draw`, gemeinsame Compactslots, `loadPath` | Quellvergleich Zeichen-/Hitbereiche, vorhandene native Renderfälle; aktuelle Artefakte hier nicht visuell nach Download untersucht |

Sämtliche Step-Reihen verwenden dieselben Handler und Compact-Maße; 100/150 sowie Zwischenzooms skalieren über den tatsächlichen Frame-Transform/DPI-Pfad. Die ältere reine Geometrie-Testsuite ist dafür alleine unzureichend, siehe V1T02. Caption-Fontbreiten, Unicode-Grapheme und optische Qualität aller aktuellen Skalierungs-/DPI-/Mehrfensterkombinationen sind hier nicht vollständig visuell abgenommen. Es wurde keine finale GUI gestaltet.

## 6. Kritische Bewertung vorhandener Tests

### V1T01 – MITTEL – Reparaturtests decken die gemeldeten Gesamtprobleme nur teilweise ab

**Status: FAIL für die Behauptung vollständiger Abdeckung; die einzelnen begrenzten Testfälle bleiben gültig.**

- `tests/CoreTests.cpp:166–184`: neuer SOR03-Stoptest prüft `StepScheduler`, keinen Stateful Processor, keine Voicecleanup und keinen Hostfeedbackpfad. Ein völlig defekter Processor-Stop könnte bei diesem Test grün bleiben. VFA05 liefert nun einen konkreten Gegenbeweis zur bisherigen Gesamtannahme.
- `tests/integration/WindowsIntegration.cpp:1541–1561`: neuer Jump-Test prüft eine aktive Stimme nach PPQ 4→8. Gut gegen die konkret zuvor stehengebliebene Stimme; keine Zero-Frame-Stop-/continuous-PPQ-Prüfung und keine vollständige Song-/Blockgrößenmatrix.
- `tests/SampleExchangeTests.cpp:126–166`: 40 kleine Wechsel erfolgen **sequenziell im selben Testthread**. Er ersetzt eine ungetriggerte Row0, während Row1 mit 48-kHz/Unity klingt. Kein Dateisystem, Controller, Host-Proxy, FLAC, paralleler Loader oder Deadline-Test. Der Test ist sinnvoll, aber sein PASS darf nicht die Owner-Dropoutmeldung schließen.
- `tests/integration/WindowsIntegration.cpp:1439–1469`: SOR02 restauriert die beiden Streams in derselben Controller-/Processor-Instanz. Das ist kein Reopen-Nachweis. VFA01 ergänzt jetzt tatsächlich frische Instanzen; die Identitätslücke V1A04 bleibt.
- P19 und T03 erfassen State-/Cell-Atomizität, aber nicht die gemeinsame musikalische Audio-Generation aus Dauer und Cells. VFA02 schließt diese Nachweislücke als derzeit roter Test.

**Empfehlung:** Die Gegenproben als verbindliche Regressionen übernehmen und die Claims der bestehenden Tests auf ihren realen Prüfumfang begrenzen. Vorhandene Tests nicht entfernen; passende Integration ergänzen und nach jeder Korrektur unverändert bekannte Fehlerkontrollen erhalten.

### V1T02 – NIEDRIG bis MITTEL – mehrere grüne GUI-Tests prüfen nicht mehr verwendete Layoutmodelle

**Quellen:** `tests/UiRowControlsTests.cpp`, `UiDetailGeometryTests.cpp`, `UiRowGeometryTests.cpp`, `UiHeaderHitZonesTests.cpp`, `UiOverlayHitZonesTests.cpp`, `VelocityLaneTests.cpp`; außerdem alte feste Koordinaten in `VisibleRowsTests.cpp:80–87` und CompactRowViewport-Prüfungen.

Die aktuelle View verwendet für diese Hauptwege `UiCompactLayout`. Eine Suche nach den entsprechenden alten `Ui…::`-/`VelocityLane::`-Aufrufen in `src/vst3` findet deren tatsächliche alte Hit-/Layoutberechnungen nicht mehr; vorhandene Includes machen daraus keine Nutzung. Beispielsweise liegt der heutige SONG-Button bei x910…991, während der alte Header-Hittest x540…638 prüft. `VisibleRowsTests` erklärt vertikale Patternbereiche 38…68, die heutige Patternreihe liegt bei y52…82.

**Status: Nachweislücke bestätigt, kein damit bewiesener aktueller Geometriefehler.** Diese Tests können unverändert PASS liefern, während aktuelle Buttons falsch lägen. Echte Precision-/Menu-Gutter-/Native-Render-/Compact-Tests liefern ergänzende aktuelle Evidenz; sie rechtfertigen aber keine pauschale Aussage „jeder alte GUI-CTest prüft die aktuelle Oberfläche“.

**Empfehlung/Regression:** Anforderungen jedem tatsächlich verwendeten Zeichen-/Hitpfad zuordnen. Aktuelle View-Eventtests an sichtbaren Rändern, Zwischenräumen, Popups, 1/4/16 Rows, 100/120/130/150 und DPI ergänzen; absichtliche aktuelle Hitboxverschiebung muss scheitern. Historische reine Helpertests kenntlich machen, nicht als aktuelle GUI-Vollabnahme zählen.

### 6.1 Standalone-/CTest-Inventar

Im beauftragten Ausgangslauf #39778743465 sind **22/22 CTests** tatsächlich erfolgreich. Die zusätzliche unabhängige Windows-QA enthält nun rote Gegenfälle. In diesem Bericht werden die nicht zugänglichen individuellen CTest-Resultate der zusätzlichen Läufe nicht als „21/22“ rekonstruiert; sicher belegt sind Build-PASS, der fehlgeschlagene Regressionsschritt und die vollständige native Fallzusammenfassung.

| Testtarget-Gruppe | Was geprüft wird / Evidenzgrenze |
|---|---|
| core | Samplescheduler, Swing/Humanize, Origin, Song, Master-Ramp, Pack-Gain/Pan, Fills, Folder-Helper; Stateful-Transport zusätzlich erforderlich |
| voice | PCM/Sinc, Mix/Pan, 64 Voices, Choke, Lebensdauer, Outputsumme, Stress; DSP-Core, kein vollständiger Hostbenchmark |
| wav | PCM8/16/24/32, float/extensible, Chunkfolge/Padding, Bounds, ungültiges Format/NaN und Erhalt des alten Decodes |
| exchange | Besitz/Adoption/Withdraw/wrong-rate/Reclaim und Live-Row-Kontinuität; kein echter Datei-/Hostwechsel allein |
| trigger | Pitch-/Velocity-/Offsetbounds, Sortierung, Mute/Solo, Random-Partitionierung, 2048er Cap mit erhaltener Kapazität |
| atomic | Gridvalidierung, Generationen, MIDI-/Host-ABA, Stateexport; tatsächliche kontrollierte Überlappung in nativem T03 |
| visible_rows | Rowhide/solo, Restoregate-/Selection-Policy, Hilfsmodelle für Resize/Dirty; keine native Gesamtprüfung allein |
| kit | DM15 und ältere Layouts, Labels/Routing/Längen, Hash/Trunkierung, 16 Rows, Sourcebyte-Limits/Fehlererhaltung |
| song_chain + song_chain_negative | Musikalische Ketten, negative Positionen; bewusster aktiver Fehler mit CTest WILL_FAIL, kein ausgeschaltetes Release-assert |
| pattern_timeline, pattern_undo | Host-/MIDI-Tie, >256 Punkte, Grid-/Dauerundo; begrenzte echte Coreklassen |
| recall_ui, recall_transaction | Token-/Diagnostic-/Nothrow-/OOM-Helper; Controller-/HWND-Gates zusätzlich im Integrationstest |
| compact_layout | Aktuelle Compact-/Songslots und Grenzen; nicht optische Studio-One-Abnahme |
| row_control, detail_geometry, row_geometry, ui_hit, overlay_hit, velocity_lane | Teilweise historische Modelle, siehe V1T02 |
| integration | Aktuelle Controller/View/Processor/Win32-Pfade, definierte Race-/OOM-/Reopen-/Native-Renderfälle und neue Audittests |

Gewöhnliche laufzeitabhängige `assert(...)` wurden in den Acceptance-Quellen nicht gefunden; die Checks sind auch unter Release aktiv. Das ersetzt nicht die fachliche Kontrolle ihrer erwarteten Werte. Die vorhandenen Negativkontrollen für Routing, T01-State, SA02 und Song-Assertions sind beizubehalten.

### 6.2 DZM3-Orakel: was an den nachgebesserten Erwartungen richtig ist

DZM1 enthält 4-Byte-Magic + 1-Byte-Zoom, Code1 bedeutet150. DZM2 enthält dieselbe 5-Byte-Hülle, Codes0/1/2/3 bedeuten100/120/130/150. **DZM3 enthält anschließend 16 uint16-Längen und die jeweiligen Pfadbytes.** Daher:

- Leerer DZM3-State: `5 + 2*16 = 37` Bytes.
- 150% in neu gespeichertem DZM3: Code **3**, nicht der alte DZM1-Code1.
- Ein eingelesener DZM1/DZM2-Zustand wird beim neuen Speichern in DZM3 ausgegeben. Byteidentität mit dem alten 5-Byte-Stream wäre ein falsches Testorakel.
- Nichtleere Pfade erhöhen die Größe. Die 37-Byte-Erwartung ist nur für ausdrücklich leere Fixtures richtig, nicht für jedes Projekt.

Die Reparaturen in `PackagedAudioHostSmoke.cpp:204–222` und `WindowsIntegration.cpp:447–461` stimmen mit diesem Vertrag überein. Dass alte Tests hierfür verändert wurden, ist an diesen Stellen sachgerecht; es beweist alleine noch keinen vollständigen Sampleordner-Recall. Dafür gibt es nun VFA01.

## 7. Tatsächlich ausgeführte Windows-QA

### 7.1 Vorgegebener vollständiger Lauf

[#39778743465](https://github.com/challanger2000/125A-Drumator/actions/runs/39778743465), [Job119824279897](https://github.com/challanger2000/125A-Drumator/actions/runs/39778743465/job/119824279897), Produkt/Teststand `4f6a131…`:

- SDK-Bezug/-Prüfung, Configure und Windows-x64-MSVC-Build **SUCCESS**.
- **22/22 CTests** laut tatsächlicher Summary.
- Darin **48 benannte native Integrationsfälle**, **0 Fehler**.
- Offizieller Steinberg Validator **47 bestanden, 0 Fehler**.
- Tatsächlich paketierte VST3-Editor-/Audiohost-Smokes **PASS**, einschließlich T01/T02/T04/T28/T29/T30-Annotations.
- Bundle-Artefakt ID `12425454033`, 1.515.212 Bytes; Native-Renderings ID `12425029921`, 634.424 Bytes, Uploads erfolgreich. Dies sind **Artefaktarchive**, keine hier eigenständig verifizierten DLL-Hashes oder Release-ZIPs.

### 7.2 Erster unabhängiger Gegenlauf

[#39787836364](https://github.com/challanger2000/125A-Drumator/actions/runs/39787836364), [Job119855578252](https://github.com/challanger2000/125A-Drumator/actions/runs/39787836364/job/119855578252), `546cd4f…`:

- Windows-Build **PASS**; keine Compilerreparatur erforderlich.
- Nativer Integrationstest: **50 PASS, 2 FAIL** von 52 benannten Fällen.
- VFA01 frischer DZM3-Recall und VFA04 vollständiger Live-WAV-Import **PASS**.
- VFA02 gemischte Pattern-/Längengeneration und VFA03 Peer-Cancel **FAIL** mit den oben angegebenen Zahlen.
- Regressionsschritt insgesamt **FAIL**. Nachfolgender Paket-/Validator-/Upload-Schritt **SKIPPED**; kein neuer 47/47-PASS daraus behauptet.

### 7.3 Abschließender unabhängiger Gegenlauf

[#39788692366](https://github.com/challanger2000/125A-Drumator/actions/runs/39788692366), [Job119858605375](https://github.com/challanger2000/125A-Drumator/actions/runs/39788692366/job/119858605375), `0833f8b…`:

- Windows-Build **PASS**.
- **53** benannte native Integrationsfälle, **50 PASS / 3 FAIL**.
- Alle 48 vorhandenen nativen Fälle bleiben PASS; VFA01 und VFA04 erneut PASS.
- VFA02 und VFA03 erneut FAIL; zusätzlich **VFA05 FAIL** mit aktiver alter Stimme und gemessener Stereoausgabe.
- Die Ergänzung erlaubt bei VFA02 ausdrücklich alte **oder** neue vollständige Generationen und prüft zusätzlich den bisher fehlenden Zero-Frame-Transportpfad. Dies war ein gezielter neuer Gegenlauf, kein unveränderter Wiederholungsbuild in der Hoffnung auf Grün.
- Regressionsschritt **FAIL**; paketierte neue Hosttests, Validator und Artefaktupload **SKIPPED**.

Auszug der tatsächlichen Checks-Annotations des abschließenden Laufs:

```text
VFA01 fresh DZM3 two-stream recall PASS
VFA02 pattern length and grid audio transaction FAIL:
  actual audio combines new pasted grid with old 32-step length
VFA03 peer edit survives velocity cancel FAIL:
  cancel in first real View erased the second View's committed step
VFA04 complete live sample import PASS
VFA05 zero-frame Stop then continuous-PPQ restart FAIL:
  zero-frame transport Stop ignored; old voice survives continuous-PPQ restart
Total failures: 3; Studio One NOT RUN

VFA02 observed L[80]=0.196850; coherent 1T expected=0.393701;
  mixed new-grid/old-32-length=0.196850
VFA03 peer step before Cancel=100, after=0
VFA05 zero-frame Stop active voices=1; restart L/R=0.250000,-0.500000;
  full-block Stop control clears voice and restarts silent
```

Es gab keine lokalen Linux-C++-Builds. Die lokale CMake-Prüfung führte nur das Generierungsskript für eindeutige Testanker aus, ohne Configure/Compile eines Linux-Plugins. Für die abschließende Dokumentation wird kein weiterer Actions-Lauf ausgelöst.

## 8. Vorhandene Performance-Messungen richtig einordnen

Aus #39778743465:

| Diagnose | Tatsächliches Ergebnis | Aussagegrenze |
|---|---|---|
| F14, 64 Voices, Ratio16, 48k/256 | p95 **4,0915 ms**, p99 **4,3000 ms**, Max **4,5022 ms**, Frist **5,3333 ms** | Offline-Enginebenchmark mit Drain-/Idleblöcken; kein kompletter DAW-Callback |
| F15, 64 aktive Voices, 32 Blocks, Ratio1 | p99/Max **0,0751 ms** | Direkter PCM-Pfad; „taps=32“ im Log bedeutet hier keine ausgeführte 32-Tap-Faltung |
| F15, gleiche aktive Last, Ratio4 | p99/Max **2,1289 ms** | 128 Taps, kurze Offline-Stichprobe |
| F15, gleiche aktive Last, Ratio16 | p99/Max **4,2258 ms** | 256 Taps; bei 32 Messpunkten fällt nearest-rank-p99 auf Max |

Im ersten unabhängigen zusätzlichen Lauf #39787836364 war bei **demselben Produktcode** F14 p95 **5,4607 ms**, p99 **5,9304 ms**, Max **6,0257 ms**. Damit liegen beobachtete Renderzeiten über der 5,3333-ms-Frist. Es wurde keine genaue Overrunanzahl veröffentlicht. Der Test verlangt nur eine großzügige Gesamtlaufzeit unter 20 Sekunden; ein einzelner Deadline-Miss ist darin kein entsprechendes hartes Fail-Kriterium.

Das ist ein weiterer Grund, Durchsatz, Offline-Regression und garantierte Echtzeit auseinanderzuhalten. CI-Runnerscheduling, CPU und Systemlast sind hier nicht kontrolliert identisch. Weder lässt sich aus dem schnelleren vorhandenen Lauf eine Garantie ableiten, noch aus dem langsameren Lauf eine quantifizierte Studio-One-Ausfallrate. Der zusätzliche Live-Importtest enthält viel geringere Voice-Last und ersetzt den 64-Voice-/Ratio16-Deadline-Gate nicht.

**R03 – OFFEN, hohe Freigabepriorität:** Gepacete Langzeitmessung auf dem Zielsystem bei 44,1/48 kHz und kleinen Blocks, einschließlich vieler aktiver Instrumente, gepitchter Stimmen, GUI, Automationsdichte, Dateiimport und ASIO-Dropoutzählung. CPU/OS/Powerzustand/Interface/Buffer dokumentieren. Ein p95-/p99-/Max-Ausdruck allein ohne Fristprüfung und reale Ablaufmessung genügt nicht.

## 9. Noch erforderliche Korrekturen und Nachweise

| Priorität | Arbeit | Abnahmekriterium |
|---|---|---|
| HOCH | **V1A01**: expliziten Zero-Frame-Stop korrekt übernehmen | VFA05 und normale Stop-/Outputless-/Recall-Fälle grün; keine alten Stimmen beim kontinuierlichen Restart |
| HOCH | **V1A02**: gemeinsame musikalische Pattern-Generation aus Länge und Steps | VFA02 in beiden Richtungen grün; alte/neue Generation zulässig, gemischte verboten; Song/Undo/Copy ebenfalls geprüft |
| MITTEL | **V1A03**: View-/Gesture-Eigentümerschaft und Undo-Konfliktregel | Peer-Änderung überlebt Cancel/Close; verspätete Events ändern keine fremde Geste; Win32-Capturefolge zusätzlich geprüft |
| MITTEL | **V1A04**: sichere Sampleherkunft beim Component-only-Kitwechsel | Gleichnamige andere Samples behalten keine fremde Ordnerzuordnung; beide vollständigen Restore-Reihenfolgen weiterhin grün |
| HOCH, Nachweis | **SOR04/R01/R03**: Owner-Dropout mit tatsächlichem Host messen | Aufnahme und Timing zeigen weder fehlende Proben noch ASIO-Overruns im definierten Lastfall; Ursache und etwaige Optimierung messbar begründet |
| MITTEL, Testqualität | **V1T01/V1T02**: Grenzen alter Helper-/Geometrietests dokumentieren, aktuelle Eventtests vervollständigen | Keine vollständige GUI-/Transport-/Recall-Freigabe nur aufgrund veralteter Koordinaten oder zustandsloser Modelle |
| MITTEL, Risiko | **R02/O01**: Host-Reentranz und ursprünglicher Controller-/Component-Recall unter Feedback | Tatsächlicher Handler-/Hostablauf statt nachträglichem Zwangsabgleich des Controllers; keine inkonsistent gespeicherten Streampaarungen |
| Weiter offen | FLAC-Stress, Peak-RAM, realer alter Song, Klang-/Resampling-Matrix, lange Lebenszyklen/Denormals | Jeweils definierte positive/negative Cases und echte Nachweise; nicht aus Validator-PASS ableiten |

Die bestehenden positiven und negativen Tests bleiben erhalten. Die roten Audittests dürfen nicht durch Abschwächung auf „aktuelles Verhalten erwartet“ grün gemacht werden. VFA02 lässt bereits beide kohärenten Übergabestrategien zu. Erst gezielte Korrekturen, anschließend eine gebündelte vollständige Windows-QA einschließlich der paketierten DLL-Pfade.

## 10. Gezielt notwendige Studio-One-Nachabnahme

Keine vollständige Wiederholung sämtlicher laut Issue #7 bereits erfolgreich bedienter Controls verlangen. Für diesen Reparaturstand und die künftigen Korrekturen gezielt:

1. **Transport:** In einem Song mit langen, unterscheidbaren Drumtails Stop/Play mitten im Takt, unmittelbar am Taktende, Start ab Takt2, Locate vor/zurück und DAW-Loop. Audio aufnehmen, damit alte Tails, Doppeltrigger, verschobene Onsets oder Stille erkannt werden. Falls ein Hosttrace möglich ist, Anzahl/Framezahl der Stopcallbacks und PPQ-Folge erfassen. Das Zero-Frame-Gegenbeispiel nicht ohne Trace als bereits bewiesene Ursache des Owner-Symptoms darstellen.
2. **Samplewechsel:** Den konkreten Owner-D&D-Fall wiederholen, bei 44,1/48 kHz und tatsächlich verwendeten kleinen ASIO-Blocks; Original-/Ersatzsample inklusive Source-Rate, Format, Größe und Pitch dokumentieren. Unveränderte Instrumente und die ersetzte klingende Row getrennt aufzeichnen. Kleine WAV, nahe Limit, FLAC, ungültige Datei, schneller Prev/Next-Wechsel; Dateidialog-/Host-UI-Stall und hörbare Audiolücke separat messen.
3. **Recall/Ordner/Zoom:** Projekt schließen und Host neu öffnen; vollständiger neuer Plugin-State mit nichtleeren Pfaden, alle vier Zooms und 100/150-DPI-Zielkombinationen. Bei vorhandenen Originalen Prev/Next, bei umbenanntem/fehlendem Ordner korrektes Embedded-Playback und ehrliche Fehlermeldung. Nach V1A04 auch ein anderes gleichnamiges Kit/Preset prüfen.
4. **Gleichzeitiger Paste/Undo:** Während Playback zwischen 1T/2T kopieren/einfügen, mit absichtlich belegten versteckten Steps im zweiten Takt; keine einzelnen falschen Akzente oder Zwischengenerationen. Eigene Aufnahme vor/nach Bearbeitung vergleichen.
5. **Zwei Fenster:** Aktuellen Native-View-Nachweis um tatsächliche Host-Fokus-/Capture-/Schließreihenfolge ergänzen. Fremde Stepänderungen während/um eine Velocity-Geste müssen erhalten bleiben; Reopen und Resize-Ablehnung dürfen keine Daten ändern.

Studio One, ASIO-Deadline und musikalische Owner-Abnahme bleiben **NICHT TESTBAR in dieser Cloud-Ausführung**. Ein erfolgreicher Abschluss dieser gezielten Nachabnahme und behobene rote Regressionen sind Voraussetzung einer Freigabe; es wurde kein Release und keine endgültige Vintage-GUI freigegeben.

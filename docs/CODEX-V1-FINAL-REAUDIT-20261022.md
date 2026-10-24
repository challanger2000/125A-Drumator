# Unabhängige Drumator-V1-Nachprüfung – 22.10.2026

Prüfobjekt: `challanger2000/125A-Drumator`, Branch `fix/studio-one-acceptance-20261022`, **`72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb`**. Dieser vollständige SHA wurde am Prüfanfang gegen die GitHub-Branch-API verifiziert. Prüfdatum: **22. Oktober 2026** (UTC). Prüfplattform für ausführbaren C++-Code: Windows x64/MSVC Release, VST3-SDK 3.8.1 Build 84 (`3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, VSTGUI `5db272256172557818b6158cf0bb2c4410bddb25`). Keine Linux-Builds.

Alle nachfolgenden Produktzeilen beziehen sich auf diesen SHA. Die Auditdateien liegen ausschließlich auf `audit/codex-v1-final-reaudit-20261022`. Produktcode, Ressourcen, Versionsnummern und Entwicklungsbranches wurden nicht geändert. Mutation bedeutet hier eine absichtliche Fehlerumschaltung **nur in generierten Testkopien**, niemals in der ausgelieferten VST3.

## 1. Gesamturteil und Statusdefinition

**V1 technisch freigabefähig: NEIN.**

Die Behebung eines konkreten alten Fehlers ist von der Freigabe des gesamten Features zu unterscheiden. Auch ein nachweislich behobener Zero-Frame-Stop beweist weder vollständige Hostkompatibilität noch garantierte Echtzeitfähigkeit. Insbesondere ist auch der kalte SONG-Consumerpfad nach der Cache-Reparatur zu prüfen: Eine passende Grid-/Dauergeneration darf niemals durch eine ungültige Nulldauer ersetzt werden. Die ursprünglichen Gesamtzusagen zu Mehrfenster-Gesten, Sampleherkunft und Studio-One-Aussetzerfreiheit müssen getrennt bewertet werden.

- **PASS:** Der benannte, begrenzte Fall ist durch geprüfte Implementierung und einen geeigneten erfolgreich ausgeführten Test belegt. Keine universelle Fehlerfreiheitsbehauptung.
- **FAIL:** Ein konkreter Gegenfall oder ein ausgeführter überlebender relevanter Mutant widerlegt die betreffende Produkt- bzw. Testzusage. Ein Testlücken-FAIL ist kein Beweis für denselben Fehler im unveränderten Produkt.
- **NICHT NACHGEWIESEN:** Keine geeignete erfolgreiche Ausführung für die weitergehende Zusage; insbesondere keine Studio-One-/ASIO-Abnahme. Ein Risikoszenario wird ausdrücklich von einem beobachteten Produktfehler getrennt.

## 2. Grundlagen, Unabhängigkeit und Grenzen

Zuerst wurden `125A-Engineering/START-HERE.md`, die vorgeschriebenen Engineering-/QA-Regeln und Projekt-`AGENTS.md` gelesen. Engineering-Regelstand: `9854275cdad1d35476397d874ba95efc4f2b184a`; Bestätigung „verstanden“ vor der Arbeit. Geprüft wurden der tatsächliche Reparaturdiff und seine aufgerufenen Controller-, View-, Processor-, State-, Scheduler-, Exchange-, Voice- und SDK-Pfade, keine bloße Übernahme von Committexten oder PASS-Tabellen.

Die Befundkennungen R01–R03 beziehen sich in diesem Bericht auf den nachfolgend verlinkten unabhängigen Vorbericht; andere ältere Projektpläne verwenden dieselben kurzen Kennungen teilweise für andere Sachverhalte.

Vorbericht: [`docs/CODEX-V1-FINAL-INDEPENDENT-AUDIT.md`](https://github.com/challanger2000/125A-Drumator/blob/00a03bd90848c30092fcb455950b8cdf42113bef/docs/CODEX-V1-FINAL-INDEPENDENT-AUDIT.md), vollständiger Auditstand `00a03bd90848c30092fcb455950b8cdf42113bef`; damals geprüfter Produktstand `4f6a1318b11ab1d09c8132ad7f65517f3f6b0714`. Der ursprüngliche deterministische VFA02 wurde aus Testcommit `0833f8b069bdd1cae859a7eeeb438ad6409ba06d` übernommen; lediglich der C++-Funktionsname wurde geändert. Auch dessen vollständige Fresh-Instance-DZM3-Matrix wurde erneut übernommen.

Die neuen Fälle benutzen die tatsächlichen Klassen und echten Win32/VSTGUI-Editoren des bestehenden Integrationstesttargets. `IntegrationAccess` beobachtet Zustand; es ersetzt die Produktlogik nicht. Exakte, eindeutige CMake-Anker erzeugen instrumentierte Testkopien und brechen bei fehlenden/mehrdeutigen Ankern ab. Der zusätzliche Pattern-Hook liegt nach dem äußeren Längenlesen, der bestehende Writer-Hook mitten in der tatsächlichen Veröffentlichung. Das ist ein deterministisches Interleaving und kein bloß zufällig gleichzeitig gestarteter Threadtest.

Wichtige Grenzen:

- Auf dem Cloud-Arbeitsplatz wurde ausschließlich die CMake-Quellgenerierung mit `cmake -P` geprüft; **kein lokales C++ gebaut oder ausgeführt**.
- Der Windows-Testhost ist ein synthetischer nativer Host. Studio One/Studio Pro, ASIO-Treiber, Audiointerface, reale DAW-Browserdrops und eine interaktive Mehrfenster-Mausbedienung wurden nicht ausgeführt.
- Die Mehrfenster-Gegenprobe verwendet zwei tatsächlich angehängte Views und die echten View-Callbacks. Eine verspätete Callback-Reihenfolge ist damit geprüft; Windows-Capture-/Fokusweiterleitung als kompletter physischer Bedienvorgang ist dadurch nicht zusätzlich belegt.
- Normale `new`-/`new[]`-Aufrufe werden threadlokal gezählt. Das ist kein vollständiger Heap-/Lock-/Pagefault-Tracer; direkte `malloc`, fremde Allokatoren, alle Alignments und Betriebssystemlatenzen werden nicht lückenlos erfasst.
- Erfolgreiche CTest-, Validator-, Native- und DAW-Tests werden nicht gleichgesetzt. Dokumentierte Baseline-Ergebnisse stammen aus der GitHub-Checks-API. Vollständige offizielle Log-/Artefaktdownloads sind in dieser Umgebung durch den Cloud-Proxy blockiert; diese Grenze wird nicht durch erfundene Logaussagen ersetzt.

## 3. Tatsächliche Windows-Baseline

Geprüfter existierender Lauf: [#39818578233](https://github.com/challanger2000/125A-Drumator/actions/runs/39818578233), Job/Check `119963448370`, exakt **`72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb`**:

| Nachweis | Ergebnis | Aussagegrenze |
|---|---|---|
| SDK-Checkout, Configure, Windows-x64-Build | PASS | Kompilation/Linken; kein Funktionsbeweis allein |
| CTest | **22/22 PASS** | Unterschiedlich starke Tests, siehe unten |
| Native Integration | **53 Fälle, 0 Fehler** | Alte Tests enthalten die hier untersuchten Blindstellen |
| Offizieller Steinberg Validator | **47/47 PASS** | Kein DAW-Praxistest |
| Paketierte Audio-/Editorhost-Smokes | PASS | Synthetische Hostpfade, keine Studio-One-Abnahme |
| PACING, 48 kHz/256, 500 Blöcke | 0 PCM-Abweichungen, 0 Fristüberschreitungen; max. **0,1726 ms** | Kleine Ersatzdateien, unveränderte Row, kein Vollast-/ASIO-Beweis |
| TORTURE, 96 Zyklen | stale=0, missing-or-doubled=0, unexpected-PCM=0 | Jeweils nur PCM-Probe 80, leeres Pattern |
| F14, 64 Stimmen/16×, 48 kHz/256 | p95 **5,5059 ms**, p99 **5,6943 ms**, max. **5,9993 ms** | Frist **5,33333 ms**, **132/180 = 73,3333 %** Überschreitungen |
| F15, 64 Stimmen, 256 Frames, je 32 Messungen | p99/max: 1× **0,0712 ms**, 4× **2,8454 ms**, 16× **5,5073 ms** | Kleine Offline-Stichproben, keine harte Echtzeitgarantie |
| Studio One / ASIO | **NICHT DURCHGEFÜHRT** | Keine Aussetzerfreiheit oder V1-Reife daraus ableiten |

Die F14-Messung enthält auch Auslauf-/Leerlaufblöcke. Die 132 Überschreitungen dürfen weder auf „alle Blöcke aktiv“ umgedeutet noch als tatsächlich beobachtete ASIO-Dropouts bezeichnet werden. Der Test hat bewusst eine 20-Sekunden-Hanggrenze und diagnostische Fristzähler, keine harte Echtzeit-Freigabegrenze.

## 4. Ergebnisübersicht

| Befund / Zusage | Status | Nachweis und Grenze |
|---|---|---|
| **V1A01 – expliziter Zero-Frame-Stop** | **PASS, benannter Processorfall** | Tatsächlicher SOR03Z-Test im geprüften Baselinelauf erfolgreich; Stop wird vor dem Frührücksprung verarbeitet. Kill-Ergebnis der zusätzlich eingebauten RM01-Mutation nicht abrufbar, keine Gesamt-DAW-Zusage. |
| **V1A02 – ursprünglicher Mischsnapshot VFA02** | **PASS** | Unverändert übernommener deterministischer RE01 ergibt links **0,393701**, nicht **0,196850**. Auch RE03 mit warmem PATTERN-/SONG-Fallback besteht. |
| **V1A02 – Gesamtconsumer einschließlich kaltem SONG-Start** | **FAIL im statischen Ausführungspfad; native RE11-Reproduktion NICHT NACHGEWIESEN** | Ungültiger Null-Längencache kann den Default überschreiben und Divisor 0 an `SongRead::mapStep` liefern. Kritischer zusätzlicher Quellbefund; **kein tatsächlich beobachteter Windows-/DAW-Absturz oder nativer Exitcode behauptet**. |
| **V1A03 – Mehrfenster-Gesten insgesamt** | **FAIL** | RE04: echter Peer-FILL zunächst 127, nach altem Cancel **0**. Einzel-Step-Owner-Reparatur funktioniert im Baseline-Einzelfall, erfasst Bulkmutationen jedoch nicht. |
| **V1A04 – sichere Herkunft über Restorezyklen** | **FAIL** | RE05: Controller→Component-Kontrolle löscht fremde Herkunft; nach Component→Controller bleibt die alte Herkunft beim nächsten gleichnamigen fremden Kit erhalten. |
| **R01 – synchroner Import / Dropoutursache** | **NICHT NACHGEWIESEN als behoben** | Synchrones I/O, FLAC-Konvertierung, wiederholtes WAV-Decodieren und Kopieren bleiben vorhanden. Keine abgeschlossene Ursache-/Studio-One-Aussetzerprüfung. |
| **R02 – Reentranz beim Host-Dirty** | **FAIL im nachgebildeten Hostcallback** | RE06 speichert tatsächliche Streams innerhalb `setDirty`: neues eingebettetes WAV vorhanden, neue Herkunft fehlt beim frischen Restore. Kein Studio-One-Autosave-Nachweis. |
| **R03 – garantierte Echtzeitfähigkeit** | **NICHT NACHGEWIESEN** | Baseline F14: 132/180 über Frist; Auditlauf 3: 0/180. Große Runnerstreuung, keine ASIO-Ausführung und keine belastbare universelle Tail-Grenze. |
| **SOR04 – aussetzerfreier Samplewechsel** | **NICHT NACHGEWIESEN** | Begrenzter bisheriger PACING-Fall besteht; neue Vollast-RE09-/RM13-Einzelergebnisse nicht abrufbar. Kein echter DAW-/ASIO-Test. |
| **R02/O01 – vollständiger Host-/Controller-/Restorevertrag** | **O01: NICHT NACHGEWIESEN** | Processor-Generation/ABA im Baselinefall abgesichert. RE07 ergänzt ursprünglichen Incoming-State und beide Feedbackordnungen; individuelles Resultat nicht verfügbar. Quellseitige Divergenzgefahr bleibt offen. |
| **V1T01 – vollständige wirksame kritische Regressionen** | **FAIL als Vollabdeckungszusage** | Bulk-/Recall-Folgefehler trotz grüner bestehender Tests; belegbare Orakelblindstellen. Kein erfundener Mutations-Kill-/Survival-Nachweis. |
| **V1T02 – aktive GUI vollständig durch Geometrietests abgedeckt** | **FAIL als Abdeckungszusage** | Teilweise historische Modelle, native aktuelle Teilabdeckung vorhanden. Gesamte Zoom-/DPI-/Capture-/Hostkombinatorik nicht vollständig geprüft. |
| **Fresh-Instance-DZM3, vier Zooms, beide Streamordnungen** | **PASS für RE02** | Tatsächlich vollständig durchgelaufene Matrix inklusive realer Navigation, fehlerhafter Präfixe/NUL/Zoom, OOM und Reopen. Folgende fremde Kits sind ein separater FAIL, siehe V1A04. |
| **Studio One / Studio Pro / ASIO-Abnahme** | **NICHT NACHGEWIESEN** | Hier nicht durchgeführt. |


## 5. V1A01 – Zero-Frame-Stop und alte Stimmen

Produkt: [`src/vst3/DrumatorProcessor.cpp:581–590`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorProcessor.cpp#L581-L590), normaler Stop-/Jump-Pfad `:694–710` und `:748–771`; Test `tests/integration/WindowsIntegration.cpp:1699–1720` am unveränderten Prüfstand.

Die explizite Stop-Flanke wird jetzt vor dem Parameter-/Zero-Frame-Frührücksprung verarbeitet: Stimmen, Playing-Zustand, erwartete PPQ und Previewbits werden zurückgesetzt. Ein fehlender optionaler ProcessContext wird dabei korrekt nicht als ausdrücklich gemeldeter Stop interpretiert.

Gegenfall: One-Shot MIDI 36 bei PPQ 4 starten; genau einen `numSamples=0`-Callback mit `state=0` zustellen; am zeitlich kontinuierlich erwarteten PPQ ohne neuen MIDI-Hit weiterrendern. Der Test muss sowohl `active()==0` unmittelbar nach dem Stop als auch stummes PCM danach prüfen. Eine zusätzliche absichtliche Umschaltung unterdrückt ausschließlich den neuen Stop-Pfad; derselbe Test muss dann scheitern. Die Resultate stehen in der Ausführungstabelle.

Der Gate-denied-Pfad kehrt weiterhin vor dieser Stelle zurück. Ein erfolgreicher Voll-Kit-Recall invalidiert alte Stimmen über die Kitrevision; daraus wird keine Vollabdeckung sämtlicher abgelehnter Restore-/Stop-Überlagerungen abgeleitet. Ein zusätzlicher Hostfall „Stop ohne irgendeinen beobachtbaren Callback und Restart am exakt erwarteten PPQ“ ist für das Plugin ohne weitere Hostinformation nicht sicher unterscheidbar und kein Beweis gegen diese konkrete Reparatur.

## 6. V1A02 – behobener Mischsnapshot, neue Cold-SONG-Absturzregression

### 6.1 Neuer Fehler – ungültige Cachedauer wird als Modulo-Divisor verwendet

**Priorität: KRITISCH; statischer Quellbefund. Laufzeitreproduktion RE11: NICHT NACHGEWIESEN.** Ein Modulo mit Divisor 0 ist undefiniertes Verhalten und kann den gesamten DAW-Prozess beenden. Dieser neue Fehler ist vom behobenen ursprünglichen VFA02-Mischsnapshot zu unterscheiden. Der globale FAIL eines Windows-Laufs beweist für sich allein keinen beobachteten Division-durch-null-Absturz.

Produkt: `src/vst3/DrumatorProcessor.h:104–106` initialisiert den neuen Längencache mit Null; `src/vst3/DrumatorProcessor.cpp:696–701` invalidiert die Caches beim ersten Block einer Kitrevision. `:777–778` setzt zunächst korrekt eine sichere Defaultdauer 32 für ungültige Caches. **`:797` überschreibt diese jedoch bedingungslos mit der noch uninitialisierten Cachedauer 0**, wenn der erste Snapshot abgelehnt wird und SONG aktiv ist. Die Nicht-SONG-Abbruchbedingung `:793–795` greift dann nicht. Der Song-Schleifendurchlauf `:832–843` korrigiert diesen Wert nur bei einem erfolgreichen Snapshot oder bereits gültigem Cache. **`:823` berechnet schließlich `result.step % lengths[result.pattern]` mit Divisor 0.** Der Scheduler ruft das Mapping vor dem Lesen der Stepvelocities auf (`src/core/SequencerCore.h:117–126`); auch ein ansonsten stummes Pattern schützt nicht.

**Deterministischer Gegenfall RE11:**

1. Frische echte Processor-/Controllerinstanz, SONG aktiv, eine Chainposition A, A=16 Steps, Clipboard B=32 Steps; Audio vorbereiten, aber noch keinen Playing-Block rendern.
2. Auf dem UI-Thread einen echten Paste B→A beginnen und innerhalb von `AtomicPatternBank::replacePattern` bei ungerader Version pausieren.
3. Parallel den ersten tatsächlichen Playing-Audioblock bei PPQ 4 ausführen. Sowohl der erste aktive-Pattern-Read als auch der SONG-Read von A sehen die ungerade Version und lehnen ab.
4. `audioPatternValid_[0]` ist false, `audioPatternLengthCache_[0]` ist 0; `lengths[0]` wird 0; die Chain referenziert A; am Blockanfang wird das Modulo ausgeführt.
5. Der identische Fall nach 375 Cache-Warmup-Blöcken ist die negative Ablaufkontrolle: Er darf nicht abstürzen und bestätigt, dass der Testaufbau selbst funktioniert.

Die implementierte RE11-Prüfung verwendet einen eigenen Windows-Kindprozess, mit demselben ausführbaren Testprogramm, echten Produktklassen und ohne Schutz-/Ersatzimplementation am Modulo. Der Elternprozess erfasst den nativen Exitcode, statt den übrigen Audit mitzureißen. Ein nativer Fehler `EXCEPTION_INT_DIVIDE_BY_ZERO` hat Wert `0xC0000094` / 3221225620. Ausführungsergebnis siehe Abschnitt 14.

**Ursache/Einordnung:** Neue Regression der gemeinsamen Dauer-/Gridcache-Reparatur: „ungültiger Cache“ wird für eine einzelne Dauerübernahme nicht beachtet. Der vorherige Code hatte den zusätzlichen Null-Längencache nicht. Die neu hinzugefügten zufälligen Banktests und unser zunächst vorbereiteter RE03 prüfen diesen kalten Consumerpfad nicht; RE03 wärmt bewusst den Fallback auf. Ein PASS des ursprünglichen deterministischen VFA02 reicht deshalb für die Gesamtfreigabe von V1A02 nicht.

**Konkrete Reparaturempfehlung:** Nur bei gültigem vollständigem Cache dessen Dauer übernehmen; für noch nicht verfügbare SONG-Patterns eine explizite sichere Policy verwenden, etwa stummes Grid plus gültige Defaultdauer, und niemals 0 an das Songmapping weitergeben. Sämtliche im selben Schedulingabschnitt konsumierten Grid-/Dauerpaare müssen gültig und konsistent bleiben. Keine aus dem Writer einzeln nachgelesene Dauer als Umgehung, denn das würde den alten Mischfehler wieder einführen.

**Regression:** RE11 kalt und warm, erstes Playing nach längerem Stop, frischer Recall, Sample-Ratewechsel, alle A–H als aktive/Chainpatterns, unterbrochener erster Snapshot und abgebrochene Veröffentlichung. Ursprünglichen RE01 und warmen RE03 unverändert behalten; ein zurückgenommener Cachevaliditätsguard muss die kalte Gegenprobe wieder töten. Ein späterer Guard darf den warmen Fallback nicht stummschalten.

### 6.2 Tatsächlich implementierte gemeinsame Generation und ursprünglicher Gegenfall

Produkt:

- [`src/core/AtomicPatternBank.h:37–52`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/core/AtomicPatternBank.h#L37-L52): Längenbyte und 512 Zellen unter demselben Version-Paar lesen, Ergebnis nur bei gleicher gerader Version übernehmen.
- `AtomicPatternBank.h:59–83`: Writer-Mutex ausschließlich im Kontrollpfad; Längen-/Bulkänderungen innerhalb derselben ungeraden/geraden Version.
- [`src/vst3/DrumatorProcessor.cpp:776–798`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorProcessor.cpp#L776-L798), Songpfad `:831–843`: Audio konsumiert Paar und behält passende Cache-Dauer bei abgelehntem neuem Snapshot.
- `Processor.cpp:1081–1084`: auch Kit-Recall veröffentlicht jede Patternscheibe zusammen mit ihrer Länge.
- `src/vst3/DrumatorProcessor.h:102–106`: getrennte audioeigene Dauer-/Grid-Caches mit gemeinsamer Gültigkeit.

**Originaler deterministischer VFA02, jetzt RE01:** altes A leer/32 Steps, kopiertes B mit 16 Steps, Step 1=100 und verborgenem Step 17=50. 375 Blöcke × 256 Frames bei 48 kHz/120 BPM etablieren PPQ 4 ohne Transport-Jump. Audio pausiert exakt nach dem äußeren Längenlesen; der UI-Thread führt den echten Paste aus; dann rendert Audio weiter. Zulässig sind vollständig alt (0) oder vollständig neu (links `0,5*100/127 = 0,393700787…`). Der alte Fehler ergab `0,5*50/127 = 0,196850394…` und ist eindeutig verschieden. Das versteckte Step-Byte bleibt absichtlich erhalten, damit ein falsch verwendeter 32er-Zyklus hörbar unterscheidbar wird.

**RE03:** zusätzlich Writer innerhalb eines 16→32-Paste pausieren; Audio muss bei gleichzeitigem Zugriff den alten vollständigen 16er-Cache verwenden, in PATTERN **und SONG**. Eine nur auf erfolgreiche Snapshot-Reads geprüfte Reparatur könnte diesen Fallback übersehen.

Der neue vorhandene `tests/AtomicPatternTests.cpp`-Threadtest ist nützlich für den Bank-Baustein, ersetzt diese Fälle jedoch nicht: 10.000 Writeriterationen erzwingen weder das konkrete Processor-Interleaving noch die Fallback-/Songverwendung. Die Audio-Reads bleiben begrenzt und nehmen den Writer-Mutex nicht. Ein x64-Erfolg ist kein nebenbei erteilter Portabilitätsbeweis für andere Zielarchitekturen.

## 7. V1A03 – verbleibender Datenverlust bei Bulk-Edits aus Peer-Views

**Priorität: HOCH**, weil bereits angenommene Patternbearbeitung gelöscht werden kann. Klassifikation: unvollständige Reparatur des ursprünglichen Fehlers, kein Beweis, dass die neue Einzel-Step-Reparatur unwirksam ist.

Produkt:

- [`src/vst3/DrumatorController.cpp:659–664`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorController.cpp#L659-L664): Fremd-Owner-Sicherung nur bei `setStepVelocity`.
- [`DrumatorController.cpp:428–453`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorController.cpp#L428-L453): `replacePattern` berücksichtigt eine laufende Gesture nicht.
- `DrumatorController.cpp:640–651`: Cancel setzt weiterhin einen vollständigen älteren Patternsnapshot zurück.
- `src/vst3/DrumatorView.cpp:929–944`: echter FILL-Menüpfad; `:1281–1288`: echter Cancel-Callback.

**Reproduktion RE04:** Zwei tatsächliche Pluginviews öffnen. In View 1 eine Velocity-Geste in Row 1 beginnen. In View 2 Row 2 → FILL → FILL ALL ausführen. Controller und zurückgelesener Processorstate müssen zunächst Row-2-Step-1=127 belegen. Danach den verspäteten Cancel von View 1 zustellen. Er darf den bestätigten Peer-Fill nicht entfernen.

**Ursache:** Der Owner-Schutz wird für einzelne Steps und neue Gesten ausgeführt, aber FILL/Paste/Clear/Variationen/Undo erreichen den Bulkpfad. `strokeBefore_` bleibt gültig und der alte Owner kann später das gesamte Pattern zurückschreiben. Die vorhandene neue Regression prüft ausschließlich den Fremd-Step-Klick, der den Schutz tatsächlich durchläuft.

**Empfohlene Reparatur:** Alle Patternmutationen durch eine einheitliche Gesture-/Transaktionsschicht führen. Vor jeder fremden Bulkänderung die vorherige Gesture nach klarer Policy abschließen bzw. invalidieren. Ein verspäteter Cancel darf ausschließlich seine noch gültige Transaktion zurücknehmen. Die Identität darf nicht nur an einem einzigen Setter geprüft werden. Restore-Grenzen und Undo-Dauer müssen erhalten bleiben.

**Erforderliche Regressionen:** Peer-Step und Peer-FILL, Paste, Clear, VELO reset/randomize sowie Undo; jeweils spätes Cancel, MouseUp, weitere Mausbewegung und Editor-Schließen; beide Views als Owner, 1T/2T, unveränderte fremde Rows/Patterns und Processorstate prüfen. Direkte View-Callbacks durch einen ergänzenden echten Win32-Capture-/Fokusfall absichern. Keine bestehende rote Prüfung durch „Undo ist global“ oder einen neuen Erwartungswert wegdefinieren.

## 8. V1A04 – Herkunftsfreigabe überlebt einen abgeschlossenen Recall

**Priorität: MITTEL.** Folgen sind falsche Ordnernavigation und ein danach unbeabsichtigt geladenes Sample; das aktuelle eingebettete PCM wird durch die Herkunftsmetadaten allein nicht verändert.

Produkt:

- [`src/vst3/DrumatorController.cpp:140–143`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorController.cpp#L140-L143): jeder erfolgreiche Controller-State setzt `pathsRestoredBeforeComponent_=true`.
- `DrumatorController.cpp:199–212`: nächster Component-State überspringt dann bei gleichem Basename die Identitätsprüfung und verbraucht das Flag.
- `DrumatorController.cpp:947–951`: direkter Sampleimport setzt korrekte WAV-Identität und löscht das Flag.
- `DrumatorController.cpp:846–877`: Prev/Next navigiert tatsächlich anhand dieses Ordners.

**Was repariert wurde:** Direkt nach einem echten WAV-Import verwirft ein abweichender gleichnamiger Component-only-Recall die alte Herkunft anhand der WAV-Byteidentität. Die neue bestehende Regression testet genau diesen Fall.

**Verbleibender Gegenfall RE05:**

1. Tatsächliche Datei X/`kick.wav` mit PCM (+0,25; −0,5) laden; Component- und DZM3-Controllerstream speichern.
2. Ein vollständiges passendes Projekt in Reihenfolge **Component → Controller** wiederherstellen. Der zweite Aufruf setzt das Flag, obwohl das Streampaar bereits vollständig ist.
3. Ein unabhängiges Component-only-Kit Y laden, ebenfalls `kick.wav`, jedoch mit PCM (−0,125; +0,375).
4. Der alte X-Ordner muss verschwinden. Das noch scharfe Flag erlaubt stattdessen einmalig die falsche Herkunft.
5. Kontrollfall **Controller → Component** desselben vollständigen Projekts: das Flag wird verbraucht und der nächste Y-Recall kann korrekt geprüft werden.

Die dynamische RE05-Gegenprobe isoliert den tatsächlichen `Controller::setComponentState`-Pfad mit gültigen unterschiedlichen Kitbytes; sie spielt Y nicht zusätzlich im Processor ab. Ein vorheriger `Processor::setState(Y)` ändert das private Controllerflag nicht. Die vollständige Zwei-Stream-Wiederherstellung in frischen Instanzen wird gesondert in RE02 ausgeführt. Die Aussage über den falschen Controllerpfad ist somit nachgewiesen, kein neuer Hörtest eines vollständigen DAW-Presetwechsels.

**Ursache:** Eine globale Einmal-Berechtigung ist keine Zuordnung zweier Streams zu demselben Restorevorgang. Die neue Reparatur beseitigt die Basename-Heuristik nur außerhalb dieses stale Flags.

**Empfohlene Reparatur:** Herkunft anhand einer expliziten Asset-/Restoreidentität an das tatsächlich passende Kit binden, auch über frische Instanzen und beide Streamreihenfolgen. Ein abgeschlossenes Paar darf keine Erlaubnis für den nächsten Recall hinterlassen. DZM3 enthält bislang nur Pfade, keine gespeicherte WAV-Identität; eine belastbare Lösung muss dieses Kompatibilitätsproblem gezielt behandeln. Ein bloßes willkürliches Umsortieren der Hostaufrufe reicht nicht.

**Erforderliche Regressionen:** Beide Ordnungen, frische Instanzen, identisches/abweichendes WAV unter identischem Namen, danach mehrere Component-only-Recalls; abgelehnte/abgeschnittene States, Legacy-DZM1/2, leere Rows, fehlende Originaldatei und anschließendes Prev/Next. Eingebettetes PCM und Herkunft getrennt prüfen.

## 9. R02 – inkonsistentes Streampaar bei reentrantem Host-Save

**Priorität: MITTEL** für den nachgebildeten synchronen Hostcallback. Hostabhängige Reentranz; keine Behauptung, dass Studio One exakt diesen Ablauf ausführt.

Produkt: [`src/vst3/DrumatorController.cpp:71–76`](https://github.com/challanger2000/125A-Drumator/blob/72ae6a7aaa8beff0877a7f7fb5cc5dde0566ddeb/src/vst3/DrumatorController.cpp#L71-L76), Sampleimport `:942–951`, Controller-State `:87–103`; Processorannahme `src/vst3/DrumatorProcessor.cpp:434–447`.

`sendEditMessage()` sendet die Änderung zum Processor und meldet sofort `IComponentHandler2::setDirty(true)`. Erst nach dessen Rückkehr übernehmen die aufrufenden Controllerfunktionen ihre eigenen Daten. Beim Sampleimport sind Processor-WAV und Dateiname daher bereits neu, während Controllerpfad/Label/Identität noch alt sind.

**RE06:** Ein echtes Objekt implementiert die beiden SDK-Schwesterinterfaces `IComponentHandler` und `IComponentHandler2`; keine falsche Interfacevererbung, kein Ersatzmodell. In seinem `setDirty(true)` werden synchron die tatsächlichen Processor- und Controllerstreams gespeichert. Danach wird eine frische Instanz ausschließlich aus diesen gespeicherten Bytes rekonstruiert. Das neue eingebettete WAV muss vorhanden sein und die neue Herkunft muss dazu passen. Der Callbackzeitpunkt darf nicht erst nach der Rückkehr künstlich nachgestellt werden.

**Mögliche Auswirkung:** Herkunft fehlt oder zeigt auf das vorherige Sample; andere Editaufrufer haben ebenfalls ein Fenster zwischen Processorannahme und Controllercommit. Der Test beweist den konsistenten bzw. inkonsistenten Zustand im legal aufgerufenen Interfacecallback, nicht die Häufigkeit eines DAW-Autosaves an dieser Stelle. Die im Vorbericht rein hypothetische Reentranz wird damit gezielt dynamisch untersucht.

**Empfohlene Reparatur:** Host-Dirty erst nach vollständig erfolgreichem lokalem Commit melden; Peer-Versand/Annahme und Hostbenachrichtigung trennen. Keine speicherrelevanten Controllerdaten nach der für den Host sichtbaren Commitbenachrichtigung offenlassen. Rückweisungen und Benachrichtigungsfehler dürfen bereits bestätigte Daten nicht partiell zurückrollen.

**Regressionen:** Tatsächlich reentrant beide Streams bei Samplewechsel, Clear, Rename, Patternreplace und Undo speichern; frischer Restore beider Ordnungen, korrekte Werte und Metadaten. Ein kontrollierter Nested-Save muss scheitern, wenn Dirty wieder vor den Commit verschoben wird.

## 10. O01 – Processor-Arbitration gegenüber Host-/Controller-Reihenfolge

Processorpfade: `src/core/AtomicPatternSelection.h:19–37`; `src/vst3/DrumatorProcessor.cpp:508–567` und `:1063–1085`. Controllerpfade: `src/vst3/DrumatorController.cpp:65–69` und `:231–232`.

**Belegter Teil:** Ein Host-/MIDI-Ereignis erhöht die gemeinsame Generation selbst bei gleichem sichtbarem Wert bzw. ABA. Restore darf diesen späteren Audioparameter nicht überschreiben. Die bestehenden Zero-/Audio-/Busy-/Snapshot-Fälle und FA02 prüfen dafür tatsächliche Processorpfade. Das ist eine sinnvolle, eng begrenzte Absicherung.

**Offener Gesamtvertrag:** Der Controller setzt bei `setComponentState` weiterhin den Wert des eingehenden Kitstreams. Die bestehenden Endvergleiche richten den Controller nachträglich anhand des **vom Processor zurückgelesenen** Endstates aus. Ein Host, der noch den ursprünglichen eingehenden Stream liefert, ist damit nicht simuliert.

**Gegenprobe RE07:** Eingehendes Kit selektiert G. Während des tatsächlichen gegateten Processor-Restores nimmt ein Audio-Actor einen Zero-Frame-Hostpunkt B an. Processor- und Controller-Stateaufrufe bleiben auf dem UI-Thread serialisiert; nur `process()` läuft parallel. Anschließend beide zulässigen Testreihenfolgen vergleichen:

- Host-Controllerfeedback B **nach** `Controller::setComponentState(original G)`: Endzustände können übereinstimmen.
- Host-Controllerfeedback B **vor** `Controller::setComponentState(original G)`: Der Controller kann auf G zurückfallen, während der Processor wegen seiner Generation B spielt und als Endstate B speichert.

Das SDK schreibt für `IComponent::setState` UI-Thread auch im Processing-Zustand vor (`pluginterfaces/vst/ivstcomponent.h:198–204`), für Controller-Stateaufrufe ebenfalls UI-Thread (`ivsteditcontroller.h:472–482`). Ein Test mit gleichzeitig auf zwei Threads aufgerufenen Controller-/State-Methoden wäre deshalb kein geeigneter Beweis; die finale RE07-Gegenprobe hält diese Grenze ein.

**Bewertung: NICHT NACHGEWIESEN.** Die Implementierung lässt die beschriebene Divergenz zu; das individuelle RE07-Ausführungsergebnis ist über die verfügbaren Nachweise jedoch nicht abrufbar. Daher kein dynamischer FAIL und keine Behauptung eines tatsächlich abgehörten B/angezeigten G in diesem Audit. Ob ein bestimmter Host diese Rückmeldeordnung verwendet und danach keine weitere Synchronisation liefert, benötigt zusätzlich Hostnachweis. Processor-Arbitration-PASS und universeller Host-/GUI-PASS sind unterschiedliche Aussagen.

**Empfehlung:** Reihenfolge-/Generationsvertrag für Restore und nachlaufende Parameterzustellung festlegen; den Controller mit der tatsächlich akzeptierten finalen Auswahl auf UI-sicherem Weg synchronisieren, ohne im Audiothread GUI-Nachrichten zu senden. Nicht einfach jeden Audio-Event durch das alte Kit überschreiben. Beide Feedbackordnungen, gleiche Werte, ABA, Zero-Frame und abgelehnte Recalls im selben produktnahen Test absichern; danach Studio One mit aufgezeichneter Automation und Projekt-Reopen prüfen.

## 11. Qualität der vier neuen Audio-/Transport-Testcommits

### 11.1 `444503c` – Fristdiagnostik

`tests/SampleVoiceTests.cpp:328–393` zählt korrekt `duration > 256/48000*1000` und gibt Anzahl/Anteil zusätzlich zu den nearest-rank-Perzentilen aus. Das verbessert die Transparenz erheblich. Es bleibt ein Offline-Enginebenchmark mit 180 Messungen über fünf Zyklen, enthaltenen Drainblöcken, ohne GUI/Import/ASIO; `elapsed<20s` bleibt die eigentliche harte Grenze.

Das zusätzliche PCM-Prüfen im bestehenden SOR03-Jump-Test (`tests/integration/WindowsIntegration.cpp:1722–1742` im Prüfstand) prüft nur endlich und Betrag ≤ 0,25 an Position 80. Völlige Stille würde die PCM-Ungleichung allein bestehen; der zusätzliche aktive-Stimmen-Zähler hilft, ist jedoch kein unabhängiger korrekter Audio-Wert.

**Bewertung:** PASS für die korrekt ausgewiesene Fristdiagnostik, NICHT NACHGEWIESEN für Echtzeitfähigkeit. Keine Schwächung des Tests ist nötig, um dies ehrlich zu dokumentieren.

### 11.2 `313c19a` – PACING-Samplewechsel

`tests/integration/WindowsIntegration.cpp:1587–1650` am Prüfstand:

- Echte Controllerimports, echter laufender Processor, 500 Blöcke und Vergleich sämtlicher Samples der unveränderten Row sind ein realer Fortschritt gegenüber einem reinen Exchange-Helpertest.
- Referenz und Prüfobjekt benutzen jedoch **dieselbe** Engine. Gemeinsame Fehler können unsichtbar bleiben; dies ist keine unabhängige mathematische PCM-Referenz.
- MIDI 37 triggert Row 2 in jedem 256er-Block; Row 1 wird zwölfmal umgeladen, aber **nie getriggert**. Ein Processor, der alle zwölf Importe einfach mit OK quittiert und ignoriert, kann den Vergleich unverändert bestehen.
- Die Ersatzdateien sind jeweils 8.236 Byte, nicht nahe dem 16-MiB-Limit. Die zwölf Wechsel starten nach Block 4 und können sämtlich am Testanfang erledigt sein. Es gibt keinen harten Überlappungsnachweis für die eigentliche Veröffentlichungsstelle.
- Kein Playing-Transportkontext und kein gleichzeitig laufender Sequencer-/Loop-/Automationsfall; rund 2,67 Sekunden synthetisches Pacing sind kein Langzeittest.
- Nur die Dauer des getesteten `process` wird gemessen. Referenzrender, Systemwecklatenz, verzögerter Callbackbeginn und Nachholen nach `sleep_until` zählen nicht zur gemeldeten Callbackzeit.
- Fristüberschreitungen werden absichtlich nur ausgegeben. Der Schlussassert prüft Blocks, PCM und Fehlerstatus, **nicht** `misses==0`. Das ist ein Diagnosevertrag, kein Frist-Gate.

**Negativproben:** Nur geänderte Instanz stummschalten; importierte Samples akzeptieren, aber nicht veröffentlichen; acht echte 10-ms-Verzögerungen in `process` nach einem Import erzwingen. Kontrollausführung muss jeweils vorher grün sein. Ein grüner bewusst diagnostischer Fristfall wird nicht als fehlerhafte Deadline-Erkennung bezeichnet: seine Zähler müssen die eingefügten Stalls ausweisen, seine Funktion darf nach bisherigem Vertrag trotzdem PASS melden.

**Erforderliche Verbesserung:** Gewechselte Row tatsächlich triggern und neue Werte gegen eine unabhängige Referenz prüfen; alte weiterlaufende Stimmen getrennt nachweisen; echte Last-/Überlappungscounter und Render-/Wake-Latenzen getrennt messen. RE09 ergänzt einen solchen hörbaren Lastfall, ersetzt aber die DAW-Abnahme nicht.

### 11.3 `cdc0166` / `72ae6a7` – TORTURE-Transport

`tests/integration/WindowsIntegration.cpp:1544–1586` am Prüfstand:

- Leeres Pattern, explizites MIDI 36, Velocity 1, Offset 0. Laut tatsächlichem `src/core/TriggerBatch.h:25–39` ist 36 → Row 1 korrekt; Velocity 1 → 127. Für die Float32-Datei bei Unity-Rate sind +0,25/−0,5 an Position 80 fachlich richtig.
- Fehlender gesamter MIDI-Trigger und zwei identische gleichzeitige Trigger sind an diesem konstanten PCM-Punkt unterscheidbar. Die entsprechende Mutation muss rot werden.
- Titel/Committext „exact onset“ ist weitergehend als das Orakel: Kein Assert liest Position 0 oder die erwartete Anstiegsstelle. Ein um ein Sample verspäteter Hit ergibt bei der konstanten Datei an Position 80 denselben Wert.
- Auch absichtlich genullte Samples 0–31 können bei identischem Punkt 80 unsichtbar bleiben.
- Die zusätzlichen Restart-/Locate-Prüfungen erfolgen nach ausdrücklichem Stop ohne neue Hits und mit leerem Pattern. Der PPQ-Sprung trifft bereits stumme Stimmen. Das prüft Wiederauferstehung, aber keinen laufenden Sequencer bei einem DAW-Loop.
- Es werden PPQ 4/8 und Stop/Play real auf den Processor angewandt, jedoch kein vollständiger Host-Cycling-Vertrag mit Cycle-Flags/-Grenzen, musikalischer Phase über variable Blockgrößen, dichten Patterns und aktiven Tails simuliert.

**Erforderliche Verbesserung:** Komplette unabhängige PCM-Zeitlinie einschließlich Triggeroffset, erlaubter Stille und aller Samples prüfen. Sequencer- und MIDI-Trigger getrennt, aktive Tails bei Rück-/Vorwärtssprüngen, Loopgrenzen mitten im Block und verschiedene Blockpartitionen testen. RE08 prüft zusätzlich einen aperiodischen Stereoquellverlauf ab MIDI-Offset 17 über zwei unterschiedlich lange Blöcke; verschobener, fehlender, doppelter und unterbrochener Ton müssen jeweils erkannt werden.

## 12. R01, R03 und SOR04 – Audioaussetzer, Import und Echtzeit

### 12.1 Kontrollpfad bleibt synchron und aufwendig

`src/vst3/DrumatorController.cpp:881–955` führt Dateimetadatenzugriff, vollständiges Einlesen, gegebenenfalls FLAC→WAV, WAV-Vordecodierung, Nachrichtenkopie und synchronen Versand auf dem Aufrufthread aus. Der Processor kopiert die Nachricht (`DrumatorProcessor.cpp:410–413`), nimmt einen **Kontrollthread-Mutex** (`:417`), decodiert erneut und bereitet den Sinc-Kernel (`:420–425`, `src/core/SampleExchange.h:32–60`) vor. Danach Reclaim, Publication und Serialisierungsdatencommit.

Das ist keine nachgewiesene Audiothread-Allokation und darf nicht so bezeichnet werden. Es kann aber die UI merklich blockieren und unter Last Speicher-/Cache-/Betriebssystemdruck erzeugen. FLAC hat zusätzlich Konvertierung und anschließend die WAV-Decodes. Der vom Namen her „Pcm16“-Nachrichtenpfad unterstützt tatsächlich mehr WAV-Formate; der Name allein ist kein Formatbeweis.

Die SDK-Referenz `public.sdk/source/vst/vstcomponentbase.cpp:119–125` ruft das verbundene `notify()` auf; die offizielle Hosting-`connectionproxy.cpp:67–76` prüft den UI-Thread und delegiert synchron. Die nativen Direktverbindungen sind für den getesteten synchronen Fall passend, beweisen aber nicht das Kosten-/Reentranzverhalten jeder Hostbrücke.

### 12.2 Eigentum und Audiopfad

`src/core/SampleExchange.h:131–150`: Release-Publication und CAS-basierter Withdraw, keine doppelte Pointerübernahme. `:155–190`: Audio nimmt exklusives Eigentum vor Dereferenzierung, adoptiert nur passende Ausgangsrate, lässt bestehende Stimmen ihre alten Assets behalten. `:195–213`: nur nicht mehr referenzierte Assets in begrenzte SPSC-Reclaimqueue; Löschung außerhalb Audio. `src/core/SampleVoiceEngine.h:155–163` und `:206–275`: Stimmen behalten Owner-/Kernel-/Samplezeiger; Austausch ist kein gewöhnlicher Reset.

Das begrenzte Muster ist im Code nachvollziehbar; die neuen Tests sollen speziell alte weiterlaufende Stimmen **und** später hörbar neue Samples belegen. `process()` nimmt den `kitMutex_` nicht. `KitRestoreGate.h` lässt Audio bei belegtem Restore sofort in den Stillepfad laufen, statt den Kontrollthread abzuwarten. **Nicht blockierend heißt hier nicht unterbrechungsfrei:** Beim Voll-Kit-Recall wird absichtlich Stille gerendert. Gegenüber einem einzelnen Row-Samplewechsel ist das ein eigener Vertrag.

### 12.3 Zusätzliche Lastdiagnostik dieses Audits

RE09: 48 kHz/64 Frames, zwei lange unterschiedliche Stereoassets, 64 gleichzeitige Stimmen mit Pitch +24 (Ratio 4), zwölf tatsächliche Controllerimports je 2.400.044 Byte; nachgewiesene laufende Callbacks während Import. Alte Stimmen müssen ihren alten konstanten Wert behalten; nach einem kontrollierten PPQ-Rücksprung und frischen Hits muss das neue Sample hörbar den unabhängig berechneten Summenwert ändern. Zero-Frame-Stop, stiller Restart und erneuter Hit sind eingeschlossen. Ein absichtlicher Import-No-op muss diese zusätzliche Prüfung töten.

Die gezielte Veröffentlichungs-/Abschlussbarriere liegt außerhalb `process()`. Sie darf nicht als durchgängige Echtzeitmessung ausgegeben werden. Die Sinc-Randtransiente wird für die konstanten Plateauorakel ausdrücklich bis Sample 19 ausgenommen; RE08 prüft separat den gesamten Unity-Rate-Verlauf. Dieser Audit verschweigt diese eigenen Testgrenzen nicht.

RE10: tatsächlicher kompletter Processor, 64 aktive Stimmen, Ratio 4 und 16, 44,1/48 kHz und 64/128/256/512 Frames; je 32 Callbackzeiten, PCM-/Stimmen-/normaler-C++-Allokationsprüfung. Zeiten werden mit `steady_clock` gemessen, nearest-rank p95/p99/Maximum und exakte Blockfrist/Misszahl ausgewiesen. Bei 32 Werten ist p99 das beobachtete Maximum, keine präzise Schätzung einer seltenen Tail-Latenz.

**Für RE09/RE10 sind individuelle Ergebnisse nicht abrufbar; deshalb kein PASS und keine hier erfundenen Last-Perzentile oder PCM-/Allokationszähler.** Die unten tatsächlich verfügbaren F14/F15-Werte stammen aus dem separaten Voice-CTest. Jede künftig abrufbare RE09/RE10-Messung gilt für einen geteilten Windows-CI-Runner, instrumentierte Testkopien und einen begrenzten Versuch. Es gibt keine statistische Obergrenze aller künftigen Callbacks. Aus kleinerem p95, Offline-Durchsatz oder „0 Misses in 32 Werten“ folgt keine garantierte Echtzeitfähigkeit.

### 12.4 Reparatur- und Abnahmereihenfolge

1. Den echten Owner-Dropout reproduzieren und eine Audioaufnahme mit Callbackfristen, ASIO-Overruns, Samplerate, Puffer, CPU, Powerprofil, Interface und Hostversion verbinden.
2. Dateilesen, FLAC-Konvertierung, beide WAV-Decodes, Nachrichtenkopie, Kernelvorbereitung und Reclaim separat messen. Erst danach eine konkrete Optimierung bzw. einen begründeten Loader-/Kommunikationsumbau planen.
3. Audioqualität und PCM-Äquivalenz bei jeder Performanceänderung separat sichern. Keine pauschale Tapreduktion oder reduzierte Testlast, nur um den Fristzähler grün zu machen.
4. Kleine Buffer mit 64 Stimmen/hoher Ratio, Live-Samplewechsel, dichtem MIDI, GUI und Loop/Transport unter ASIO abnehmen. Funktionsfälle und Echtzeitgrenze getrennt veröffentlichen.

## 13. V1T01 / V1T02 und noch fehlende Regressionen

**V1T01:** Neue Stateful-Stop-, Peer-Step- und Herkunftstests schließen reale Teile der alten Lücken. Der zusätzliche deterministische Paste-/Audiofall war aber nicht Bestandteil des Reparaturbranches; die neue Bankloopprüfung beweist nicht die Verwendung im Processor. Die neue PACING-/TORTURE-Prüfung hat die oben beschriebenen, gezielt angegriffenen Orakelgrenzen. Ein erfolgreicher Gesamtbuild ist deshalb kein Vollabdeckungsnachweis.

**V1T02:** Die historischen `tests/UiRowGeometryTests.cpp`, `UiRowControlsTests.cpp`, `UiDetailGeometryTests.cpp`, `UiOverlayHitZonesTests.cpp` bzw. zugehörigen alten Layoutmodelle prüfen weiterhin nicht vollständig die tatsächliche Compact-GUI. `src/vst3/DrumatorView.cpp:287–319`, `:460–502`, `:557–607` und `:929–944` verwenden `UiCompactLayout`, variable Zeilenhöhen und Seiten. Teilweise noch vorhandene Includes alter Header sind kein Nachweis, dass deren Geometrie aktive Hitflächen bestimmt. Aktuelle native Präzisions-/Menü-/Mehrfenstertests sind wertvoll, aber keine erschöpfende Prüfung sämtlicher Controls in allen Zoom-/DPI-/Instrumentenzahlkombinationen.

Zusätzlich erforderlich:

- Vollständige UI-Ereigniskette inklusive Windows-Capture, Fokusverlust, Cancel, späten Listenern und Reentranz beim Hostcallback; nicht nur direkte Methodenaufrufe.
- Transaktionen über jeden konkurrierenden Patternmutator und Undo; Daten-/Längen-/Generationserhalt auch bei abgelehnter Nachricht.
- Mehrere komplette Restorezyklen mit beiden Streamordnungen und anschließendem Component-only-Recall; Metadaten und tatsächlich hörbares PCM getrennt prüfen.
- Sequencerloop-/Phasentests mit echten Patternnoten, Rücksprüngen in aktive Tails, variablen Blockgrößen, Swing/Humanize und exaktem gesamten PCM- bzw. Triggerorakel.
- FLAC sowie WAV nahe 16 MiB beim Livewechsel; reproduzierbar beschädigte Datei, erschöpfte Reclaimkapazität und gezielte OOM-/Nachrichtenablehnung bei gleichzeitigem Rendering. Die frühere allgemeine Import-QA wird nicht als neuer Vollast-FLAC-Test ausgegeben.
- Langzeit-ASIO-Ausführung auf dem Zielsystem einschließlich aufgezeichneter Audioaussetzerzählung. Deadline-Diagnostik bleibt gesondert sichtbar.

## 14. Tatsächlich ausgeführte unabhängige QA und Mutationen

### 14.1 Auditcommits und drei vorhandene Läufe

| Stand | Windows-Lauf | Tatsächlich verfügbarer Nachweis |
|---|---|---|
| **`fba6526cc73e80210caad228e2a1a0f8323d7182`** – gebündelte unabhängige Fälle/Mutationen | [#39823072597](https://github.com/challanger2000/125A-Drumator/actions/runs/39823072597), Check `119979129188` | Windows-Build **SUCCESS**, CTestschritt **FAIL**. Einzelausgaben beweisen RE01–RE06 wie unten; keine vollständige native Fallübersicht über die verfügbare API. Validator/Paketierung übersprungen. |
| **`eec0978caf9fe0a0ae8ba41cf73860567fe44ea3`** – UI-Thread-gerechter RE07 und gebündelte Diagnostik | [#39824145312](https://github.com/challanger2000/125A-Drumator/actions/runs/39824145312), Check `119982839451` | Windows-Build **SUCCESS**, CTestschritt **FAIL**. Bei Abschluss 0 verfügbare Checkannotations; keine abgeleiteten Einzelergebnisse. Validator/Paketierung übersprungen. |
| **`6aa15030917888a8e6258935fe2c7e5388ad76ee`** – zusätzlicher kalter SONG-Kindprozessfall | [#39824973014](https://github.com/challanger2000/125A-Drumator/actions/runs/39824973014), Check `119985765293` | Windows-Build **SUCCESS**, **21/22 CTests PASS, 1 FAIL**. API liefert F14/F15 und Gesamtzählung, **keine vollständige native Fall-/Mutations-/Kindprozessausgabe**. Validator, Renderarchive und Paketartefaktupload **SKIPPED**. |

Die zweite Ausführung folgte der konkreten SDK-/Diagnostikkorrektur im Testaufbau, die dritte dem neu gefundenen kalten SONG-Pfad. Es gab keine Produktänderung und kein künstlich grünes Ergebnis. Auf ausdrückliche Anweisung des Owners wird der Audit mit diesen vorhandenen Erkenntnissen abgeschlossen: **keine weiteren Windows-Läufe, keine Änderungen/Abschwächungen der unabhängigen Tests.**

Die Checks-API ist in den letzten Läufen unvollständig. Im ersten Lauf fehlen nach den frühen Einzelfallnotices weitere Meldungen; die im Test ergänzte Aggregation lieferte in den verfügbaren API-Daten ebenfalls keinen vollständigen Native-Report. Ob Annotation-Limits, Übertragung oder ein anderer Laufzeitpfad dafür verantwortlich sind, wurde nicht abschließend geklärt. Der offizielle volle Logdownload scheitert an HTTP 403/„Forbidden“ des Cloud-Proxys. **Aus globalem CTest-FAIL werden weder eine bestimmte native Absturzursache noch erfolgreiche Einzeltests oder eine Mutations-Killrate rekonstruiert.** Vorhandene vollständige Originalausgaben können später diese offenen Felder schließen, ohne jetzt erneut einen kostenpflichtigen Build zu starten.

### 14.2 Individuell belegte zusätzliche Ergebnisse

| Fall | Status | Konkreter vorhandener Nachweis |
|---|---|---|
| **RE01 / original VFA02** | **PASS** | `VFA02 observed L[80]=0.393701; coherent 1T expected=0.393701; mixed new-grid/old-32-length=0.196850`. Die tatsächliche Paste-/Audioverschachtelung ist erreicht. |
| **RE02 / Fresh DZM3** | **PASS** | Abschlussnotice bestätigt frische Instanzen, alle vier Zooms, beide Ordnungen, reale Navigation, sämtliche abgeschnittenen Präfixe/NUL/ungültigen Zoom, OOM und Reopen; diese Notice steht erst nach allen Assertions. |
| **RE03 / warmer Fallback** | **PASS** | `actual Pattern/Song fallback: old complete 16-step generation survives paused 32-step writer PASS`. Beide Zweige tatsächlich durchlaufen. |
| **RE04 / Peer-FILL** | **FAIL** | `before Cancel=127 after=0`. Die echte bestätigte Row-2-Änderung wurde gelöscht. |
| **RE05 / Herkunftsflag** | **FAIL** | `next-kit origin cleared after Controller-first full project=YES`; nach `Component-first ...=NO`. Reihenfolge-Gegenprobe unterscheidet die Ursache. |
| **RE06 / reentrant Dirty** | **FAIL** | `NEW embedded WAV retained; origin restored=NO`. Neues Processor-Audio und alte Controllerherkunft werden zusammen gespeichert; frischer Restore geprüft. |
| **RE07 / Hostmirror** | **NICHT NACHGEWIESEN** | Test implementiert und Windows-kompiliert, individuelles Ausführungsergebnis nicht verfügbar. Nicht zum bestätigten RE04–RE06-Fehlerpaket hinzuzählen. |
| **RE08 / vollständige unabhängige PCM-Zeitlinie** | **NICHT NACHGEWIESEN** | Implementiert/kompiliert; keine hier verfügbare Einzelfallausgabe. |
| **RE09 / 64 Stimmen und hörbarer Liveimport** | **NICHT NACHGEWIESEN** | Implementiert/kompiliert; keine erfundenen Überlappungs-/PCM-/Allokationszähler. |
| **RE10 / tatsächliche Processor-Fristenmatrix** | **NICHT NACHGEWIESEN** | Implementiert/kompiliert; keine individuellen Messreihen abrufbar. |
| **RE11 / kalter SONG mit warmem Kontrollkind** | **NICHT NACHGEWIESEN als Runtime-Reproduktion** | Im letzten Windows-Stand kompiliert und registriert; weder warmer Kontrollabschluss noch kalter Exitcode individuell verfügbar. Statischer Fehlerpfad separat in Abschnitt 6.1. |

Der letzte native Programmeinstieg registriert 53 vorhandene Fälle plus RE01–RE11 und RM01–RM13. Das ergibt **77 registrierte Top-Level-Fälle**, ausdrücklich **keine Behauptung von 77 vollständig ausgeführten oder bestandenen Fällen**.

### 14.3 Mutationstests: Implementierung und derzeitige Nachweisgrenze

Alle Umschaltungen liegen in `tests/integration/ReauditInstrumentation.cmake:3–26` und generierten Testkopien. Ein `ReauditMutationScope` aktiviert jeweils genau einen Fehlertyp; die Programme werden nicht als Plugin ausgeliefert. `expectMutationKilled` führt zuerst den unveränderten Kontrolltest aus und zählt eine bereits rote Kontrolle ausdrücklich nicht als Mutationserkennung. Relevante Meldungen würden `KILLED` oder `SURVIVED` samt Ursache ausgeben. Die entsprechenden Einzelergebnisse sind in den vorhandenen Checks **nicht enthalten**.

| Fall | Absichtlich injizierter Fehler / Kontrollzweck | Abschlussstatus |
|---|---|---|
| RM01 | Neue Zero-Frame-Stopbehandlung unterdrücken | **NICHT NACHGEWIESEN**, kein behaupteter Kill |
| RM02 | Nach neuer Gridgeneration alte äußere Cachedauer behalten; original VFA02 als Orakel | **NICHT NACHGEWIESEN** |
| RM03 | Fremd-Step-Owner-Sicherung auslassen | **NICHT NACHGEWIESEN** |
| RM04 | Herkunft wieder nur nach Basename behalten | **NICHT NACHGEWIESEN** |
| RM05 / RM06 | TORTURE: MIDI-Hit vollständig auslassen bzw. verdoppeln | **NICHT NACHGEWIESEN**; erwartete Erkennung ist quellanalytisch begründet, nicht als ausgeführter Kill ausgegeben |
| RM07 | TORTURE: MIDI um genau ein Sample verschieben | **NICHT NACHGEWIESEN als ausgeführter Survivor**; der konstante Punkt-80-Vergleich ist quellanalytisch blind dafür |
| RM08 | TORTURE: erste 32 PCM-Samples jedes Blocks nullen | **NICHT NACHGEWIESEN als ausgeführter Survivor**; unveränderte Probe 80 prüft diese Samples nicht |
| RM09 | PACING: alle Row-Imports mit OK quittieren und ignorieren | **NICHT NACHGEWIESEN als ausgeführter Survivor**; gewechselte Row wird im Originaltest nicht getriggert |
| RM10 | Nach Import ausschließlich die geänderte Processorinstanz stummschalten | **NICHT NACHGEWIESEN** |
| RM11 | Unabhängiges komplettes PCM-Orakel mit Delay/Nullprefix/fehlendem/doppeltem Hit angreifen | **NICHT NACHGEWIESEN**, kein erfundener vierfacher Kill |
| RM12 | Acht echte 10-ms-Callbackstalls; PACING muss sie diagnostisch ausweisen, hat aber absichtlich kein Deadline-Gate | **NICHT NACHGEWIESEN**, keine behaupteten acht beobachteten Misses |
| RM13 | Ignorierte Sampleübernahme muss im neuen hörbaren 64-Voice-Fall auffallen | **NICHT NACHGEWIESEN** |

Die Quellkritik an den Orakeln bleibt gültig: konstantes Sample 80 kann keinen exakten Offset oder ein früheres Nullsegment beweisen; ein nie getriggertes geändertes Sample beweist keine erfolgreiche hörbare Übernahme. **Die dynamische Mutationsaufgabe ist wegen der fehlenden vollständigen Einzelresultate nicht abgeschlossen.** Die Tests bleiben unverändert für die spätere Logauswertung bzw. den nächsten ausdrücklich freigegebenen QA-Stand erhalten.

### 14.4 Tatsächlich neue verfügbare Leistungswerte aus Lauf 3

Unverändertes `drumator_voice_tests` / Check `119985765293`:

| Messung | Beobachteter Wert |
|---|---|
| F14, 64 Stimmen/16×, 48 kHz/256 | p95 **2,7349 ms**, p99 **2,7703 ms**, max **2,8738 ms**, Frist **5,33333 ms**, **0/180 Misses** |
| F15, 64 Stimmen/1×, 32 Blöcke | p99/max **0,0382 ms** |
| F15, 64 Stimmen/4×, 32 Blöcke | p99/max **1,4470 ms** |
| F15, 64 Stimmen/16×, 32 Blöcke | p99/max **2,7624 ms** |

Der kontrastierende Baselinewert 132/180 versus jetzt 0/180 bei unverändertem Produkt zeigt gerade, warum einzelne Shared-Runner-Durchläufe nicht als garantierte Echtzeitqualifikation taugen. Kein Leistungsgewinn wurde diesem Audit zugeschrieben: **Produkt-DSP blieb identisch.** Keine RE10-Matrix, kein ASIO-Overrunzähler und keine Audioaufnahme werden damit ersetzt.

### 14.5 Wiederholung und Erhaltung für die weitere Arbeit

Nicht jetzt erneut ausgeführt; für einen später ausdrücklich autorisierten Windows-Stand:

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON -DDRUMATOR_BUILD_VST3=ON -DVST3_SDK_ROOT=<exakter SDK-Pfad>
cmake --build build --config Release --target drumator_test_suite --parallel 2
ctest --test-dir build -C Release --output-on-failure
# Die originale CTest-Registrierung startet den tatsächlichen nativen Host:
.\build\Release\drumator_integration_tests.exe .\resource
# Isolierte kalte/warme SONG-Fälle; kalt kann den Kindprozess beenden:
.\build\Release\drumator_integration_tests.exe .\resource --reaudit-song-warm
.\build\Release\drumator_integration_tests.exe .\resource --reaudit-song-cold
```

Originale vollständige stdout-Ausgabe, `build/ctest-functional.log` und `build/Testing/Temporary/LastTest.log` sichern; sowohl fehlende Einzelfälle als auch unerwartete Harnessfehler auswerten. Rote Tests nicht abschwächen, erwartete Produktfehler nicht als erfolgreiche Kontrollen zählen. Eine zusätzliche Actions-Ausführung ist **nicht Teil dieses Abschlussauftrags**.


## 15. Priorisierte notwendige Nacharbeiten

1. **KRITISCH – V1A02/Cold SONG:** Modulo durch 0 bei konkurrierendem Paste im ersten SONG-Audioblock verhindern; RE11 kalt/warm, RE01 und RE03 zusammen absichern.
2. **HOCH – V1A03:** Mehrfenster-Bulkänderungen vor alten Gesture-Rollbacks schützen; Datenverlust-Gegenprobe RE04 bestehen, alle Mutationswege und Undo ergänzen.
3. **HOHE Freigabepriorität – R01/R03/SOR04:** Reale Dropoutursache und ASIO-Fristeinhaltung im benannten Lastbereich nachweisen. Die existierenden Fristüberschreitungen sind mit einem pauschalen Echtzeit-PASS unvereinbar; sie belegen nicht automatisch dieselbe Missquote auf dem Zielsystem.
4. **MITTEL – V1A04:** stale Herkunftsfreigabe nach abgeschlossenem Component→Controller-Recall beseitigen; RE05 und beide vollständigen Restoreordnungen samt Nachfolgekit bestehen.
5. **MITTEL – R02:** Dirty-Benachrichtigung hinter den vollständigen Controllercommit verlegen bzw. Transaktionsvertrag so gestalten, dass reentrantes Speichern konsistente Streams sieht; RE06 als Regression.
6. **MITTEL – O01, Nachweis offen:** Zuerst originales RE07-Ergebnis auswerten; Audio-/Controller-Endauswahl bei ursprünglichem Componentstream und beiden Feedbackordnungen konsistent machen; keine nachträgliche Zwangssynchronisation im Test als Ersatz für Produktverhalten. Tatsächlichen Studio-One-Ablauf separat nachweisen.
7. **MITTEL – V1T01, Mutationsabschluss offen:** Originale RM01–RM13-Ergebnisse auswerten; relevante Transport-/Importmutationen durch unabhängige, vollständige Orakel erfassbar machen. Die bestätigten roten Gegenproben und die noch nicht individuell belegten Audittests unverändert erhalten.
8. **NIEDRIG bis MITTEL – V1T02:** Historische GUI-Modelltests eindeutig kennzeichnen und aktive Compact-View-Eventabdeckung vervollständigen.

## 16. Konkrete letzte Studio-One-/Studio-Pro-Abnahme

Keine der folgenden Prüfungen wird hier als ausgeführt gemeldet:

- Host-, Windows-, CPU-, Powerprofil-, Interface-/ASIO-Version, 44,1/48 kHz und 64/128/256/512 Frames dokumentieren; tatsächlichen Plugincommit und installierten Paket-Hash festhalten.
- Lange aktive Samples: Play → Stop nur über Zero-Frame-Flanke, normaler Stop, schneller Stop/Play, Start an unterschiedlichen Takten; alte Tails dürfen nicht ungewollt weiterlaufen. Loop-/Locate-Rücksprung während aktiver Stimmen, nicht nur bereits stummem Instrument.
- Dichte Sequencerpatterns und MIDI getrennt aufnehmen; Patternlängen-/Patternwechsel, Swing/Humanize und Automationspunkte an Block-/Loopgrenzen. Ganze Aufnahme auf fehlende, doppelte und versetzte Hits untersuchen.
- Zwei echte Pluginfenster: Velocity-Geste offenhalten, im Peer FILL/Paste/Clear/Undo, dann Captureverlust/Cancel/Schließen; ausschließlich die beabsichtigte Transaktion darf zurückgenommen werden.
- Projekt X/`kick.wav` und Kit Y/`kick.wav` mit verschiedenen Inhalten, vollständiges Projekt und anschließend Component-only-Preset/Kits; Browserpfeile müssen zur richtigen Herkunft gehören. Eingebettetes Audio muss auch ohne Originaldatei erhalten bleiben.
- Automation während Projekt-/Presetrestore; Hostanzeige, GUI, gespeicherter Zustand und hörbares Pattern abgleichen, auch Zero-Frame und A→B→A.
- Laufende Audioaufnahme bei tatsächlichen WAV-/FLAC-Wechseln nahe Importgrenze, 64 Stimmen und hoher Ratio, gleichzeitig Loop/GUI/Automation. ASIO-Overruns und Callbackbeginn/-ende separat zu Importzeiten protokollieren. Einmaliger erfolgreicher Drop ohne Last reicht nicht.
- 100/150 % und gespeicherte Zooms, unterschiedliche DPI/Monitore, Resizeablehnung, Schließen/Reopen und 4/16 Instrumente; echte OS-/DAW-Browserdrops auf Rows und bei geöffneten Menüs.

Die endgültige funktionale V1-Freigabe setzt behobene nachgewiesene Fehler und ausreichende Host-/Echtzeitnachweise voraus. Dieser Audit enthält weder eine Produktreparatur noch eine Freigabe aufgrund grüner Actions allein.

## 17. Abschließende Freigabeentscheidung und Übergabe

**V1-Freigabe: NEIN.** Bereits die drei dynamisch belegten Fehler RE04–RE06 schließen die funktionale Freigabe aus. Der ursprüngliche VFA02 ist nachweislich behoben; das ist ausdrücklich anerkannt. Der zusätzliche statische Cold-SONG-Befund ist vor einer Freigabe zu beseitigen bzw. mit vollständiger Originalausgabe der unveränderten Gegenprobe zu klären. Fehlende Runtime-/Mutations-/Hostnachweise bleiben offen und werden nicht grün geschätzt.

Die vorhandenen unabhängigen Tests bleiben auf **`6aa15030917888a8e6258935fe2c7e5388ad76ee`** unverändert erhalten. Dieser Abschluss ergänzt ausschließlich den Bericht. Kein Produktionscode repariert, kein Entwicklungsbranch verändert, keine Versionsänderung, kein Release und kein zusätzlicher Windows-Lauf nach dem Abschlussauftrag. Der Pull Request dient der gesammelten Prüfung von Bericht und Audittests; er ist keine automatische Freigabe oder Aufforderung, erwartete oder ungeklärte rote Tests zu übergehen.

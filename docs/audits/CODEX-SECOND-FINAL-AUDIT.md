# 125A Drumator – zweiter unabhängiger V1-Abschlussaudit

Prüfdatum: **22.10.2026 (Europe/Berlin)**. Repository `challanger2000/125A-Drumator`, ausschließlich `develop/v1.0.0`.

**Ergebnis: Technisch bereit für die Studio-One-Abnahme – JA. Zwei bestätigte Produktfehler behoben. Windows-QA SUCCESS. Keine V1-/endgültige GUI-Freigabe und kein erfundener Studio-One-PASS.**

Geprüfter finaler Programm-/Testcommit: **`7598252828d709c23a5bed33756c085a4d052a08`**. [Erfolgreiche abschließende Windows-QA #39695738761](https://github.com/challanger2000/125A-Drumator/actions/runs/39695738761): **22/22 CTests, 46 benannte native Integrationsfälle ohne Fehler, 47/47 Steinberg Validator, tatsächliche paketierte Audio- und Editorhosttests PASS.** Der folgende Dokumentationscommit ändert weder Programm noch Tests.

| Produktbefund | Priorität | Ausgangsstand | Finaler Status |
|---|---|---|---|
| SA02: gleichzeitige Choke-Trigger ändern PCM mit der Blockgröße | P1 / HOCH | Tatsächlich im Windows-Prozessor reproduziert | PASS: alle positiven Partitionen identisch; ursprünglicher Komparator reproduziert den Fehler bei 16 gleichzeitigen MIDI-Noten |
| SA01: normale Controlleredits aktualisieren andere Views nicht vollständig | P2 / MITTEL | Konkreter fehlender Benachrichtigungs-/Viewportpfad | PASS: beide nativen HWNDs, 100/150%, sämtliche benannten Edits und erreichbare Peer-Seiten |

**Bilanz neuer bestätigter Produktfehler:** P0: 0 / P1: 1 / P2: 1 / P3: 0; beide korrigiert. Davon getrennt: ein eigener Testcompilerfehler und ein falsch präparierter Test-Row-Entfernungsfall, beide berichtigt. Kein bestätigter Produktbefund dieses Auftrags bleibt ungelöst.

## 1. Herkunft, Auftrag und Methode

Der unabhängig per Fetch und Remote-Abfrage bestätigte Ausgangs-HEAD ist **`72ee3c37cd0500f0eb618bfc1d36b7357d11360c`**. Er entspricht dem vom Eigentümer genannten Stand. Der vorherige Bericht `docs/audits/CODEX-FINAL-FUNCTIONAL-RECHECK-2026-10-21.md` wurde als Gegenprüfliste verwendet, seine PASS-Aussagen nicht als Ersatz einer aktuellen Quell- oder Ausführungsprüfung.

Gelesen und angewandt: `125A-Engineering/START-HERE.md`, vorgeschriebene Lesereihenfolge und anwendbare Engineering-, QA-, DSP-, GUI-, Repository-/Baseline-Standards, danach Drumator `AGENTS.md`, V1-Scope und `docs/VSTGUI-HANDOFF.md`. Engineering-HEAD blieb seit dem vorherigen Audit unverändert **`9854275cdad1d35476397d874ba95efc4f2b184a`**. Das Projekt ist weiterhin ein nicht freigegebener Entwicklungsstand, Version **0.1.0**. Es wurde keine hypothetische kommerzielle FULL-/Demo-Releasebaseline erfunden.

`main` wurde nicht geändert und steht auf **`7f5b4a7baba2371d9cc42c12406191ecbf3fe32f`**. Keine Versionsänderung, kein Release, keine neuen Funktionen, keine finale GUI-Gestaltung, keine fremden proprietären Komponenten. Kein Linux-Build. Verwendet wird das fest gepinnte offizielle VST3-SDK **3.8.1**, SHA **`3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`**, mit seinen zugehörigen Submodulen/VSTGUI.

### Beweismaßstab

- **PASS:** Die genannte abgegrenzte Prüfung hat mit überprüfter Prüflogik und tatsächlicher Ausführung bestanden; keine Universalgarantie.
- **FAIL:** Konkret belegter Fehler; Fehlerursache und Gegenbeispiel werden angegeben.
- **OFFEN / NOT TESTED:** Kein ausreichender Ausführungsnachweis oder nicht abgedeckte Variante.
- **OWNER HOST TEST:** Prüfung im tatsächlichen Studio One fehlt. Ein synthetischer Host ist dafür kein Ersatz.

Die Analyse betrachtet Quellcode und Übergänge: Controller → Nachricht → Processor → Atomic-/Sample-/Patternzustand → tatsächlicher Render sowie native Views/Hostgrößen. Das Testorakel und mögliche falsche Positivresultate wurden ausdrücklich mitgeprüft. Ein Kompilierfehler im neuen Testcode wird nicht als Produktfehler bezeichnet. Nicht jedes Risiko wird zu einem Bug umetikettiert.

## 2. Untersuchte Produktionskomponenten

| Bereich | Relevante Quellen | Gegenprüfung und Ergebnisumfang |
|---|---|---|
| WAV/FLAC und Speicherlimits | `src/core/WavPcm16Decoder.h`, `src/vst3/FlacImport.cpp`, `KitState.h`, Controller::loadWav/stepSample, Processor::notify | Kandidaten vor Commit; RIFF-/Metadatengrenzen, Mono/Stereo und PCM/Float; vorhandene echte FLAC-/WAV-Fixtures, korrupte Daten, Limits und vorheriger Klang geprüft. Kein umfassendes Codec-Fuzzing. |
| Assetbesitz, aktiver Samplewechsel | `SampleExchange.h`, `PreparedSample`, Processor::publishRestoredKit/setupProcessing | Übergabe durch atomare Besitzübernahme, aktive/retired Assets, Reclaim nur Kontrollseite; alte normale Sample-Tails bleiben am alten Asset. Recall ist eigener Gate-/Generationswechsel. |
| Voice-/Pitch-/Sampleratenpfad | `SampleVoiceEngine.h`, SincKernel, PreparedSample::fromWav | Direkter PCM-Pfad und Interior-FIR-Optimierung unabhängig analysiert; weiterhin Pitch-/Rate-FIR, Choke, Stereo, Stimmenlimit, Launch-Mix und Routing. |
| Clock, Sequencer und Song | `SequencerCore.h`, `SongChain.h`, Processor::process, SongMailbox | PPQ-/Samplegrenzen, Takt-2-Start/Locate/Loop, 1T/2T, 8 Patterns, Songlängen, Swing/Humanize, Fallback nach Recall und Sampleoffset-Auswahl. |
| MIDI-/Hostereignisse | `TriggerBatch.h`, `PatternEventTimeline.h`, `AtomicPatternSelection.h`, `PatternQuantization.h`, Processor::process | Noten 36–51, Pattern 24–31, Legacy-Quantisierung, Hosttie, Zero-/Outputless-/Render-/Restore-Pfade und tatsächliche Merge-Reihenfolge. |
| Routing und Mixer | Processor::initialize/setBusArrangements/activateBus/process; Voice routing, AtomicRowMix, MasterGainRamp | MAIN + 15 Stereoausgänge, deaktiviertes MAIN, gemeinsame AUX-Ziele, Stereo-Identität, Fallback, Mute/Solo/Choke, Masterrampe und Speicherzustand. |
| Editing | Controller Pattern/Velocity/Fill-/Undo-Funktionen, PatternUndo/Operations/RowFill, AtomicPatternBank | Copy/Paste/Duplicate ohne belegte Ziele zu überschreiben, aktive Patternlänge, zweite-Bar-Erhalt, Undo/Cancel, Random einmalig vs Legacy Playback-Random. |
| State / Threading | KitCodec, Processor/Controller::setState/getState/setComponentState, KitRestoreGate und Mailboxes | Versionierte Zustände bis DM15, Altfixtures DMR1–14, Reject/OOM, atomarer Kitwechsel, Automation-/Recall-Generationen und echte konkurrierende Renderfälle. |
| GUI und Host | Controller/View/Editor, UiCompactLayout/UiSongLayout, UIDESC/Factory/IDs | Jede bestehende Bedienfamilie aus der ersten Matrix erneut mit tatsächlichen Handlern abgeglichen; insbesondere normale Mehrfachview-Edits, Zoom, native Namensedits, D&D und Editor-Lebensdauer. Keine Designänderung. |

Die ausschließlich nach `72ee3c3` vorgenommenen Produktänderungen werden unten einzeln aufgeführt. Die funktionierende Resampling-/PCM-Optimierung wurde nicht vorsorglich umgeschrieben.

## 3. Neue Produktbefunde und Reparaturen (nach Priorität)

### SA02 – P1 / HOCH – Gleichzeitige Choke-Trigger hängen von der Hostblockgröße ab

**Ursache:** Im Ausgangsstand vergleicht [TriggerBatch::sort, Zeile 71](https://github.com/challanger2000/125A-Drumator/blob/72ee3c37cd0500f0eb618bfc1d36b7357d11360c/src/core/TriggerBatch.h#L71) ausschließlich `sampleOffset` und `row`. `std::sort` ist nicht stabil. Sequencer- und MIDI-Trigger derselben Row können bei gleichem Zeitpunkt abhängig von der Gesamtzahl/Verteilung anderer Events umsortiert werden. Der zuletzt akzeptierte Trigger einer Chokegruppe bleibt ungechokt; eine andere Gleichstandsreihenfolge verändert daher den verbleibenden Klang bzw. seine Velocity.

**Praktisches Gegenbeispiel / echter roter Nachweis:** Der reale Processor erzeugt 16 Sequencer-Row-Trigger am Downbeat; Row 0 trägt ein Sample und Choke 1. Gleichzeitige MIDI-Noten derselben Row besitzen unterschiedliche Velocities. Ein späterer Trigger auf leerer Row 15 liegt bei Sample 200. Eine DAW-Blockaufteilung in 256 bzw 128+128 Frames ändert die absoluten Ereigniszeiten nicht, wohl aber die Sortierlisten. Der tatsächlich ausgeführte Test in [Windows #39695007293](https://github.com/challanger2000/125A-Drumator/actions/runs/39695007293) meldet:

```text
SA02 actual sequencer/MIDI choke partition FAIL:
SA02 same absolute sequencer/MIDI/choke events produce different audio at 256 versus 128+128
```

S16 allein konnte dies nicht erkennen: Es übergibt der Stimmenengine bereits gleich geordnete Listen und durchläuft den fehlerhaften Merge/Sortierpfad nicht.

**Gezielte Korrektur in `7598252828d709c23a5bed33756c085a4d052a08`:** [SequencerCore.h:54](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/core/SequencerCore.h#L54) ergänzt ein internes, standardmäßig 0 initialisiertes Ankunftsordinal. [TriggerBatch.h:16](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/core/TriggerBatch.h#L16) vergibt es beim Zusammenführen; [Sortierung:75](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/core/TriggerBatch.h#L75) ordnet nach Offset, Row und zuletzt Ankunft. Die bisherige Eingangsreihenfolge bleibt somit bei allen Listengrößen erhalten. Keine alloziierende `stable_sort`, keine neue Stealing-/Choke-Politik, keine Änderung von Host-/MIDI-Patternprioritäten. Das Ordinal ist weder Stateformat noch VST3-ABI; Trigger werden nicht als rohe Zustandsbytes gespeichert. Der fest begrenzte 2048-Event-Puffer wächst um 8 KiB pro Prozessor; keine Callbackallokation, die Sortierkomplexität bleibt O(n log n).

**Positive und negative Regression:** [SecondAuditCases.inl:76](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/tests/integration/SecondAuditCases.inl#L76) prüft sechs Dichten mit 1/8/15/16/17/20 gleichzeitigen MIDI-Noten, alle L/R-PCM-Samples exakt über beide Partitionen und unabhängig erwartete Stereo-/Velocitywerte nach dem Chokefade. Ein Nichtstille-Orakel verhindert triviale Gleichheit. Eine ausschließlich in generierte Testquellen eingefügte Negativkontrolle verwendet im tatsächlichen Processor wieder den **exakten ursprünglichen Komparator**. Sie muss mindestens eine Blockgrößendivergenz erzeugen, während die korrigierte Sortierung alle Fälle bestehen muss. Das ist ein kontrollierter Eingriff in genau die verursachende Operation und kein paralleles Ersatzmodell. Testflag/Legacyzweig existieren nicht im ausgelieferten Plugin. Die tatsächliche Negativkontroll-Dichte wird unten aus der letzten Windows-Annotation dokumentiert.

**Status: PASS für die Korrektur.** Der finale reale Windows-Prozessor liefert bei allen sechs Dichten identisches PCM über die Blockteilungen; der ursprüngliche Komparator erzeugt in derselben Pipeline **genau eine Divergenz, bei 16 gleichzeitigen MIDI-Noten**. Tatsächliche Annotation:

```text
SA02 actual Processor PCM PASS; original-comparator negative divergences=1;
simultaneous MIDI counts=16
```

Damit ist das Ausbleiben des Fehlers keine Folge eines wirkungslosen oder stummen Tests. Der vorher gemeldete FAIL stammt zusätzlich aus dem echten unveränderten Sortierpfad des zweiten Laufs.

### SA01 – P2 / MITTEL – Normale Edits lassen andere offene Views veralten

**Ursache im Ausgangsstand:** `src/vst3/DrumatorController.cpp`, Funktionen `adjustRowMix` (:755), `adjustRowPitch` (:742), `toggleRowGate` (:769), `adjustSwing` (:670), `adjustHumanize` (:660), `clearSample` (:694), `loadWav` (:825) sowie `addRow`, `hideLastEmptyRow`, `restoreVisibleRows` (:542–581). Nach akzeptierter Prozessornachricht wurde das gemeinsame Controllermodell geändert, aber keine Benachrichtigung sämtlicher registrierten Views ausgelöst. Die aufrufende View zeichnet sich selbst neu; die zweite erhält dadurch keinen Invalidierungsauftrag.

**Praktisches Gegenbeispiel:** Zwei Editorfenster derselben Instanz öffnen, im ersten Mute klicken oder Gain/Pan ändern. Der gemeinsame Controller-/Prozessorzustand ändert sich, während die zweite View ihren bisher gezeichneten Zustand ohne neues Paint behalten kann. Zufällige spätere Expose-/Resize-Ereignisse können das Problem verdecken. V18 hätte den Fehler nicht erkannt, weil sein Orakel nur Viewanzahl/Zeiger und erfolgreiche `setParamNormalized`-Rückgaben prüft.

**Verwandter Zeilenzahlpfad:** Ein kurzes zweites Fenster muss nach einer Änderung der gemeinsamen Instrumentenzahl alle neuen Instrumente weiter per Paging erreichen können. Bloßes Invalidieren ohne Anpassung seiner sichtbaren Zeilen würde unter Umständen eine größere Zeilenanzahl in seine unveränderte kleine Clienthöhe zeichnen.

**Korrektur in `ef0b1de92c4ee7fac4d160373407a9b93cd79919`:** [Controller.cpp:549](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/vst3/DrumatorController.cpp#L549), [Controller.cpp:660](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/vst3/DrumatorController.cpp#L660), [View.cpp:448](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/src/vst3/DrumatorView.cpp#L448). Alle genannten akzeptierten Änderungen rufen erst nach ihrem erfolgreichen Modellcommit `notifyViews(false)` auf. Abgelehnte Änderungen tun dies nicht. `DrumatorView::refreshFromHost` (:448) berechnet im nicht-resizenden Aktualisierungspfad aus der tatsächlichen logischen Clienthöhe die passende kompakte Seitenanzahl neu. Die auslösende View behält ihre bestehende Resize-/Rollback-Transaktion; andere Views lösen keine zusätzlichen Hostgrößenanfragen aus. Keine Audio-/Stateformatänderung und kein neuer Polltimer.

**Gezielte Regression:** [tests/integration/SecondAuditCases.inl:3](https://github.com/challanger2000/125A-Drumator/blob/7598252828d709c23a5bed33756c085a4d052a08/tests/integration/SecondAuditCases.inl#L3), `testSecondAuditViewInvalidation`. Zwei echte attached Win32-/VSTGUI-Fenster bei 100/150%. `GetUpdateRect` kontrolliert deren tatsächliche nativen Neuzeichnungsbereiche, nachdem `ValidateRect` alte Bereiche gelöscht hat. Positivkontrolle `other->invalid()` muss dieses Orakel auslösen. Der echte erste View-Mauscallback für M und anschließende Controller-Edits für Solo/Gain/Pan/Pitch/Swing/Humanize/Sample-Clear/echten WAV-Import müssen beide HWNDs invalidieren. 1→2→3 Rows, kompakte Peer-Seite, Erreichbarkeit der dritten Row, Hide/Rollback sowie ein abgelehnter Out-of-range-Edit werden geprüft. Das ist ein echter View-/Controller-/Windows-Pfad, keine Registrierungssimulation.

**Nachweisgrenze:** Der Ausgangsfehler ist durch den ursprünglichen fehlenden Benachrichtigungspfad konkret belegt. Ein beabsichtigter Vorher-Ausführungstest kam wegen des unten dokumentierten eigenen Testcompilerfehlers nicht bis CTest. Deshalb wird kein roter funktionaler Vorherlauf erfunden. Die korrigierte Prüfung hat in Run#39695738761 bestanden. Dies bestätigt den beschriebenen Nachher-Pfad; die Beweisgrenze des ursprünglichen Vorherlaufs bleibt ausdrücklich bestehen.

## 4. Neue Ergänzungen T30, S16, V18 und PCM kritisch bewertet

| Ergänzung | Unabhängige Bewertung des Ausgangstests | Maßnahme / Beweisgrenze |
|---|---|---|
| T30 |300 MIDI-Patternkommandos laufen durch den echten Processor, jedoch nur outputless; das einzige Orakel war abschließend Pattern G, das bereits der Hostpunkt allein erzwingt. Selbst wenn sämtliche MIDI-Kommandos verschwinden, könnte diese Erwartung bestehen. Es wurde keine tatsächliche Rückmeldung jedes Zwischenoffsets kontrolliert. | MIDI-only muss jetzt D ergeben (letzter von 300 Kommandos), mit Hosttie G. Jeweils echter Render- und Outputless-Pfad. Frische Ausgabeparameterqueues müssen sämtliche 256 bzw 255 chronologischen Punkte mit unabhängig berechneter Patternfolge enthalten. Damit genügt ein Host-only-Ergebnis nicht mehr. |
| S16 | Sinnvoller exakter PCM-/Voicecount-Vergleich der Engine bei überlappenden Chokes und verschiedenen Blockteilen. Als Nachweis der vorgeschalteten Sortierung reicht er nicht. Ein ignorierter Choke könnte diesen Gleichheitstest alleine ebenfalls bestehen; die separaten Fade-/Kapazitätstests bleiben notwendig. | Originaltest erhalten; SA02 ergänzt den tatsächlichen Prozessormerge. Keine beliebige neue Choke-/Stealing-Politik. |
| V18 | Belegt in zwei nativen Views Registrierung, Removal, Reopen und normalen Patternbenachrichtigungspfad ohne offensichtlichen Crash. Er prüft nicht, ob beide Views tatsächlich aktualisiert werden, nicht jede normale Änderung und keinen reentranten Close innerhalb eines Hostcallbacks. | Originaltest erhalten; SA01 kontrolliert native Invalidierung bei realen Edits und zugängliches Paging. Vollständiger Host-Reentry weiterhin nicht automatisch bewiesen. |
| PCM-Direktpfad | `ratio==1.0` wird exakt verglichen, nicht mit einer willkürlichen Näherung. Position startet bei 0 und wird pro Outputsample um exakt1 erhöht; bei maximal erlaubter Samplelänge bleibt diese ganzzahlig und exakt darstellbar. Deshalb ist für diesen Fall kein interpolierender FIR nötig. Die maximale erlaubte Samplelänge liegt weit unter 2^53; ganzzahlige Positionen bleiben somit exakt. Stereo/Mono, Gain/Pan, Velocity, Choke und Routing bleiben hinter der Abtastung erhalten. | P01 prüft unabhängig erwartete PCM-Samples einschließlich erster/letzter Frames, Fortsetzung und Mono. Kein Produktfehler gefunden. Der Direktpfad gilt nur bei exakt1; bei anderem Pitch oder Rateverhältnis bleibt der Sinc-Pfad. |
| FIR-Interior-Optimierung | `first>=0 && first+taps<=frames` bedeutet, dass alle ursprünglichen Tap-Indizes gültig sind. Nur dann entfallen die wiederholten Randprüfungen. Sonst bleibt das ursprüngliche Zero-Padding erhalten; Koeffizienten, Tapreihenfolge und doppelte Akkumulation sind unverändert. | Keine neue Filtertopologie, keine Änderung allein aufgrund eines Mikrobenchmarks. Bestehende DC-/Stereo-/Alias-/Pitch-/Rate-/Hosttests erneut auswerten. Kein vollständiger Frequenz-/Transientensweep behauptet. |
| F15 | Identische64-Voice-/256-Frame-Engineworkloads bei 1x/4x/16x, jede Messung fordert64 aktive Voices.32 Messblöcke sind eine kleine Offline-Stichprobe; bei Nearest-Rank ist p99 hier identisch zum Maximum. Reine Engine, kein gepaceter kompletter VST3-/ASIO-Callback. | Als Diagnose geeignet, als garantierter Realtime-PASS ungeeignet. Literale\n-Ausgabe korrigiert, damit jede Ratio als eigene tatsächliche Notice verfügbar wird. Keine Statistik erfunden. Bei1x nennt das Log die vorbereiteten 32 Kernel-Taps, der tatsächlich aktive Direktpfad führt aber keine 32-Tap-Faltung aus. |

### Eigener Testcompilerfehler, sauber getrennt

Der erste neue Windows-Lauf [#39694354286](https://github.com/challanger2000/125A-Drumator/actions/runs/39694354286), Commit **`a9837824f08ad95e6e079194bc5eecb2078b3ec4`**, scheiterte beim Build. **CTest, Validator und paketierte Hosttests wurden dort nicht ausgeführt.** Die vom Eigentümer aus dem Originallog gelieferten Fehler lauten:

```text
PackagedAudioHostSmoke.cpp(1846,52): error C2220
PackagedAudioHostSmoke.cpp(1846,52): warning C4389: '==': signed/unsigned mismatch
vstgui/lib/ctexteditor.cpp(1967,18): warning C4267: size_t -> int
```

Der neue Test verglich den `uint32_t`-Rückgabewert von `patternFromNormalized` mit einem `int`-Ternärausdruck. Die Konstante ist nun typgerecht unsigned; keine Abschaltung von `/W4 /WX`. Die getrennte Upstream-C4267 war nicht der gemeldete Abbruchgrund. Dieser selbst eingeführte Buildfehler ist kein zusätzlicher Drumator-Produktbefund. Die Diagnose wurde anhand der konkreten Meldung bestätigt; ein blinder Wiederholungslauf wird nicht als Testnachweis ausgegeben.

### Zweiter Lauf: Produkt- und Testfehler unterschieden

[#39695007293](https://github.com/challanger2000/125A-Drumator/actions/runs/39695007293) kompiliert erfolgreich und führt CTest tatsächlich aus: **21/22 CTests**, zwei fehlgeschlagene Unterfälle im einzigen nativen Integrations-CTest. SA02 ist der oben konkret reproduzierte Produktfehler. SA01 scheiterte mit „actual accepted edit failed“: das Fixture `namedKit` belegte auch unbenutzte Rows mit nichtleeren Namen. `Controller::canHideLast` lehnt eine solche Row korrekt ab. Das Fixture muss die unbenutzten Sample-/Customnamen tatsächlich leeren; die Schutzregel des Produktes wird nicht aufgeweicht. Das aktuelle Fixture tut dies. Es werden weder zwei fehlgeschlagene CTests noch ein weiterer Datenverlustfehler behauptet. Alle übrigen gemeldeten nativen Fälle bestanden in diesem Lauf; Validator/Host-Paketprüfungen waren wegen CTestfehlern noch übersprungen.

## 5. Funktions- und Abdeckungsmatrix des gesamten vorhandenen V1-Umfangs

Die benannten automatisierten Fälle sind im finalen Windows-Stand ausgeführt und bestanden. PASS gilt nur für die beschriebene abgegrenzte Prüflogik; Studio One bleibt separat.

| Funktion | Konkreter Nachweis / erneut geprüfter Übergang | Status / Grenze |
|---|---|---|
| Leeres Kit, One-Shot-Wiedergabe, Preview im Stop | Voice empty; echter paketierter Host, Preview-/outputless Lifetime | PASS (Automatik); echte Hostbedienung OWNER HOST TEST |
| PCM mono/stereo + WAV8/16/24/32/Float/Extensible | Decoder-Goldwerte; P01 sourcegenaue Samples; echte Hostamplituden | PASS (Automatik); RF64/Mehrkanal nicht unterstützt |
| FLAC, korrupt/zu groß, fehlgeschlagener Wechsel | Echter Decoder/Audiofixture, deklarierte Framegrenzen, Candidate-Reject/FA04 | PASS (Automatik); beliebige Dateien nicht exhaustiv gefuzzt |
| Samplewechsel bei aktiven Stimmen | Exchange old/new Owner, retired Referenzen und Drain/Reclaim | PASS für definierte Sequenzen; lange Realhostlast offen |
| Kit-Recall gleichzeitig mit Audio/Automation | T03/T04/O01, FA02/FA03, reale Gates und X/Y-Audiocodes | PASS für erzwungene Interleavings; keine allgemeine Race-Freiheitsgarantie |
| Sampleratenwechsel/Pitch/Sinc/PCM | P01, DC/Stereo/Alias, M35, echter T28 bei PPQ4 plus Blockpartition | PASS (Automatik); breiter Qualitäts-/ASIO-Sweep OWNER HOST TEST |
| 64 Voices, Überlast, Choke, Mute/Solo | Capacity/Recovery/N09/Fade/Gate, S16, tatsächlicher SA02-Merge | PASS (Automatik); Tails bleiben bei Mute absichtlich erhalten |
| 32-Step-Timing, Takt 2, Stop/Restart/Locate/Loop | Core Sampleoffset-Goldwerte/Whole-Split; paketierter Transport und T28 | PASS (Automatik); echter DAW-Transport/Tempoänderungen OWNER HOST TEST |
| Swing/Humanize/Random Playback | Mehrere Raten/Tempi, identische absolute PPQ bei Split, unabhängiges Random-Zeitgegenbeispiel | PASS (Automatik); musikalischer Höreindruck OWNER HOST TEST |
| Patterns A–H, 1T/2T, Song 64 Slots8/16/32 | T01 echte gespeicherte zweite Bar, Core/Host ABA, native Seiten/Count/Length | PASS (Automatik); realer Songworkflow OWNER HOST TEST |
| Copy/Paste/Duplicate/Clear/Fill/Undo/Velocity | Native Controller-Commit-/Duration-/Inactivebar-/Cancel-/Recalltests, FA05/FA06 | PASS für Szenarien; alle physischen Zellen einzeln NOT TESTED |
| MIDI36–51, 24–31, Automation/Zwischenwerte/Offsets | Trigger-Ranges, echter A01/Hosttie, FA02/O01, verstärkter T30 | PASS (Automatik); tatsächliche DAW-Ausgabeparameter-/GUI-Rückführung OWNER HOST TEST |
| MAIN+15 AUX, gemeinsame Busse, deaktiviertes MAIN | Echte Stereo-Signaturen/T02, T29 null/nichtnull MAIN-Tabelle, F08, Assignment/SharedBus | PASS (Automatik); Studio-One-Busaktivierung OWNER HOST TEST |
| Gain/Pan/Pitch/Master/Choke/Grenzen | Voice-Goldwerte, Hostpersistenz, Masterrampe, native Precision/Reset, SA01 Peerfeedback | PASS (Automatik); Launchwerte gelten für neu gestartete Stimmen |
| WAV-/FLAC-Picker, Ordnernavigation, D&D | Echte Ordner/Dateien, FA04 Callbacks/OOM/UTF8 und erhaltenes Kit; Win32Drop-Patch geprüft | PASS für Adapter; Explorer/DAW-Browser und echter Dialog OWNER HOST TEST |
| State DM15 / Migration DMR1–14 / Reject | Codec korrigierteCRC-/Truncation-Gegenfälle, frische Hostinstanzen, Z01/OOM | PASS für konkrete Fixtures, nicht jede historische DAW-Datei |
| Normale Mehrfachview-Edits und Zeilenzahl | V18 Removal/Reopen plus SA01 echte HWND-Invalidierung, 100/150 und kompaktes Paging | PASS (Automatik); Host-Reentry/Langzeitsession offen |
| Zoom100/120/130/150, DPI, Hostrefusal | Attached Z02, DZM1/2, reale Transformation/Wiederöffnung und atomare Mehrfachview-Ablehnung | PASS für Testhost; physische Host-/OS-Skalierung OWNER HOST TEST |
| Namenseditor, alter Callback nach Recall, Editorclose | Echtes Win32 EDIT, Z03 Enter/Escape/Blur, abgelehnter/akzeptierter Recall; FA04 Namen/Pfade | PASS (Automatik); echte DAW OWNER HOST TEST |
| VST3-IDs/Lifecycle, Float32, Tail, Busverhandlung | Offizieller Validator, echte paketierte Hosts, Lifecycle-/Rate-/Statepfade | PASS (Automatik); Float64 absichtlich nicht unterstützt |
| Hostdirty, Preview ohne Dirty, Projekt-Reopen | sendEditMessage-/HostEditDirty-Vertrag und gespeicherter Prozessor-/Controllerzustand | Quell-/Automatikteil; tatsächlicher Projektstern/Save/Hostundo OWNER HOST TEST |
| RT-Allokation/Locks/I/O/Deadline | Quellprüfung Callback/Gate/Exchange, vorallokierte Timeline, Offline F14/F15 | OFFEN als umfassender Allocation-/Lock-/Paced-ASIO-Nachweis |

**Per-Control-Traceability:** Die konkrete vorhandene Zeichen-/Hit-Geometrie und vollständigen dynamischen Indexbereiche sind in der [ersten vollständigen Auditmatrix](CODEX-FINAL-FUNCTIONAL-RECHECK-2026-10-21.md) dokumentiert und wurden gegen den aktuellen Quellpfad abgeglichen. Sie bleiben unverändert: Header A–H/1T/2T/Patternaktionen/Song/Zoom/Swing/Humanize/Master; je Row 0..15 Auswahl, Samplefeld, Menü, `<`/`>`, PLAY, 9 Fills, M/S, 16 Outputs, 5 Chokes, VOL/PAN/PITCH, One-Shot Random bzw Legacy RT-Regler, 32 Steps und 32 Velocitybalken; Song 64 Einträge mit Seiten/Pattern-/Längenwahl; Instrument+/−/Paging und nativer Namenedit. Maßgeblich `UiCompactLayout`, `UiSongLayout`, `View::draw/onMouse*`, anschließend Controllerkommando und Processor-Nachricht/State. Keine interaktiven UIDESC-Tags werden vorgetäuscht: dies ist eine Customdraw-View. Alte isolierte Geometriehelfer werden nicht als heutiger UI-PASS gewertet. Für diese zweite Prüfung ergänzt SA01 den vorher unbewiesenen normalen Peer-Feedback-Pfad; die übrigen bestehenden Native-/Hostfälle wurden erneut ausgeführt und sind oben abgegrenzt bewertet.

**Kein physischer Komplettklick-PASS:** Direkte logische VSTGUI-Mauscallbacks, Win32-Textedit-Ereignisse und native Offscreen-Renderings ersetzen keine vollständige tatsächliche Studio-One-Mausbedienung aller 4096 Patternzellen bei allen Skalierungen. Die neue HWND-Invalidierungsprüfung beweist den Repaintauftrag, nicht die visuelle Abnahme jeder Beschriftung oder den proprietären Hostinput.

## 6. Windows-QA, Commits und tatsächliche Messwerte

| Stand | Commit / Lauf | Ergebnis |
|---|---|---|
| Ausgangsstand | `72ee3c37cd0500f0eb618bfc1d36b7357d11360c`, [#39688505771](https://github.com/challanger2000/125A-Drumator/actions/runs/39688505771) | SUCCESS; echte Annotation 22/22 CTests, 47/47 Validator; neue Testorakel erst nach eigener Prüfung eingeordnet |
| Neue Gegenprüfungen, unveränderter Produktcode | `a9837824f08ad95e6e079194bc5eecb2078b3ec4`, [#39694354286](https://github.com/challanger2000/125A-Drumator/actions/runs/39694354286) | BUILD FAIL im eigenen Test, keine Funktionsergebnisse |
| SA01 und typgerechte Testkorrektur | `ef0b1de92c4ee7fac4d160373407a9b93cd79919`, [#39695007293](https://github.com/challanger2000/125A-Drumator/actions/runs/39695007293) | BUILD PASS, CTest 21/22: SA02 Produktfehler und SA01-Fixturefehler, Paket/Validator übersprungen |
| Stabile Triggerreihenfolge und korrekt leere Row-Fixture | `7598252828d709c23a5bed33756c085a4d052a08`, [#39695738761](https://github.com/challanger2000/125A-Drumator/actions/runs/39695738761) | SUCCESS: 22/22 CTests, 46 native Fälle, 47/47 Validator, paketierte Hosts |

Der Ausgangslauf misst F14 Offline bei 48k/256/64 Voices/16x: p95 **4.1313ms**, p99 **4.3548ms**, Maximum **4.3941ms**, Frist 5.33333ms. F15 meldet bei 1x **0.0761ms**, 4x **2.174ms**, 16x **4.2465ms** jeweils p99=Max bei 32 Blöcken. Diese Werte stammen aus der gespeicherten tatsächlichen Ausgangsannotation, keine zugesicherte DAW-Frist. Zwischen verschiedenen CI-Läufen darf ein reiner Zahlenvergleich nicht als kontrolliertes Optimierungsexperiment ausgegeben werden.

Die vollständigen offiziellen Joblogs werden hier weiterhin durch den Cloud-Proxy beim Azure-Blob-Redirect blockiert; keine Umgehung. Deshalb werden zugängliche echte Checks-Annotations, Job-/Schrittstatus und tatsächliche Testorakel gemeinsam ausgewertet. Originalcompilerdiagnosen wurden vom Eigentümer nachgereicht und sind oben wörtlich erfasst. Die bereits im ersten Audit eingeführte Notice-Ausgabe wurde am Zeilenanfang abgesichert; CTest-/Validator-Exitcodes bleiben unverändert ausschlaggebend. Kein hartkodierter PASS im Workflow, keine Testabschaltung, keine erfundenen Studio-One-Ergebnisse.

### Abschließende tatsächliche Resultate und Belege

[Job #119552536694](https://github.com/challanger2000/125A-Drumator/actions/runs/39695738761/job/119552536694), [direkt lesbare Checks-Annotations](https://api.github.com/repos/challanger2000/125A-Drumator/check-runs/119552536694/annotations?per_page=100). Run-Metadaten: Start **22.10.2026 01:48:09 MESZ**, Abschluss **01:51:58 MESZ** (UTC+2). Geprüfte SHA exakt `7598252828d709c23a5bed33756c085a4d052a08`.

- Offizieller SDK-/Submodulcheck, Configure und MSVC-x64-Release-Build: SUCCESS.
- **CTest 22/22**. Darin ein absichtlich erwarteter Song-Chain-Negativtest (`WILL_FAIL`); dies wird nicht als unbemerkter Produktfehlschlag interpretiert.
- **46 native Fallnamen, 0 Fehler** innerhalb eines CTests, nicht 46 zusätzliche CTests.
- **Offizieller Steinberg Validator: 47 bestanden, 0 fehlgeschlagen.**
- Paketierter Editorhost: natives HWND, echte Attach-/Remove-/Zoom-/Resizeprüfungen PASS.
- Paketierter Audiohost: Ende aller verpflichtenden `require`-Prüfungen erreicht, PASS. T28/T29/T30 und die übrigen unten zitierten Einzelfälle sind diesmal zusätzlich als einzelne tatsächliche Notices zugänglich.
- Native BMP-Renderings erzeugt und hochgeladen: Artefakt **12390751119**, 634424 Bytes. Standard-VST3-Bundle: Artefakt **12389959159**, 1514575 Bytes. Der Workflow prüft das tatsächlich zusammengestellte Bundle, seine drei nichtleeren Dateien und moduleinfo vor Validator/Hosttests.
- Eigene nachträgliche ZIP-/Binär-/Bildansicht dieser Artefakte: **NOT TESTED**, offizielle Downloads weiterhin vom Proxy blockiert. Keine Aussage, die aktuellen Bilder selbst angesehen zu haben.
- Tatsächlicher Studio-One-/ASIO-Praxistest: **OWNER HOST TEST – nicht durchgeführt.**

Aus den tatsächlichen Annotations, unveränderte Messwerte:

```text
F15 64 voices, 16x, taps=256, 48k/256, 32 blocks: p99=4.3182ms max=4.3182ms (offline CI, not realtime qualification)
F15 64 voices, 4x, taps=128, 48k/256, 32 blocks: p99=2.1536ms max=2.1536ms (offline CI, not realtime qualification)
F15 64 voices, 1x, taps=32, 48k/256, 32 blocks: p99=0.1133ms max=0.1133ms (offline CI, not realtime qualification)
F14 64-voice 16x offline block times: p95=4.0995ms p99=4.4873ms max=4.4938ms; 48k/256 deadline=5.33333ms (CI runner, NOT Studio One real-time evidence)
```

F14 liegt in dieser Offline-Messung mit seinem gemessenen Maximum unter 5.33333ms. Dies ist kein Hard-Realtime-Nachweis. Die Stichprobe enthält auch Drain-/Leerlaufblöcke. F15 hält dagegen alle 64 Stimmen über seine 32 Messblöcke aktiv, misst aber ebenfalls nur die Engine und keinen gepaceten gesamten Hostcallback. Weder Messreihe beweist fehlende ASIO-Xruns auf Eigentümerhardware.

Zugängliche tatsächliche Integrationsmarker:

```text
T04 PASS: 24 X/Y restores; coherent X blocks=55, Y=53, gate silent=29541
T02 PASS: 15 distinct AUX stereo codes, neighbor-bus and L/R swap negatives
T29/FA01 PASS: real packaged AUX1 with MAIN zero channels, null/non-null pointer table, untouched MAIN guard, fresh offset64 stereo signature and oracle negatives
T30 PASS: 300 MIDI commands, MIDI-only D and host-tie G, rendered/outputless paths and all 255/256 feedback offsets
T28 PASS: packaged 48k->44.1k, no-MIDI Step-1 onset at PPQ4, independent stereo code, 256 versus 128+128, sample/state preservation
T01 PASS: actual returned state after 2T/1T/2T, all A-H/rows tail cells and erased-step negative
SA02 actual Processor PCM PASS; original-comparator negative divergences=1; simultaneous MIDI counts=16 
```

T04s viele Gate-Stille-Blöcke entstehen im ungebremsten synthetischen Audioactor während der Kontrollarbeit. Sie dürfen nicht in „so viele reale verlorene DAW-Blöcke“ umgerechnet werden. Das bestehende Recall-Fallback ist Stille während exklusiver Zustandsübernahme; es wird kein nahtloser, unhörbarer Kitwechsel zugesagt. Beim normalen einzelnen Sampletausch dagegen behalten bereits gestartete Stimmen ihr altes Asset. Diese unterschiedliche Politik im Host abhören.

Vollständige tatsächlich gemeldete Native-Fallliste:

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
V18 multi-view native lifecycle PASS
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
SA01 two-view native invalidation after row and global edits PASS
SA02 actual sequencer/MIDI choke partition PASS
Total failures: 0; Studio One NOT RUN
```

FA01–FA06 aus dem ersten Audit bleiben durch die abgegrenzten aktuellen Regressionen bestätigt: FA01/T29 nullkanaliges MAIN; FA02 MIDI-/Host-/Zero-Frame-Recall; FA03 neuer Song-Fallback; FA04 Importpfad-Ausnahmen; FA05 Menülücken; FA06 Mindesthöhe/Hostablehnung. Auch T01–T04/O01/Z01–Z03 sind tatsächlich erneut ausgeführt worden. Dies ersetzt weiterhin keine unbegrenzte Race-/Host-/Fuzzing-Garantie.

## 7. Verbleibende Risiken – begrenzt, nicht künstlich verlängert

- **Paced Realtime/ASIO:** Offline-Perzentile und 32-Block-Diagnostik sind kein harter Deadlinebeweis. Reale Treiber, GUI/Import, mehrere Instanzen und Langzeitlast sind noch zu messen; die CI-Hanggrenze20 s ist kein RT-Akzeptanzkriterium.
- **Gesamt-RAM:**16 MiB pro rohe WAV/64 MiB im Kit begrenzen nicht PCM-Decoding, Kernel, Ersatzkits und ausklingende alte Assets zusammen. Es liegt keine vollständige Peak-Working-Set-Qualifikation vor.
- **Begrenzte Nebenläufigkeitsbeweise:** T03/T04/O01/FA02/FA03 prüfen reale erzwungene Fenster. Sie beweisen keine mathematische Race-/Wait-Free-Freiheit aller erlaubten Hosts. Pattern-CAS verwendet Wiederholungen; lockfrei bedeutet nicht formal fristgarantiert.
- **Host-Reentry:** Das Entfernen einer View exakt innerhalb eines reentranten Resize-/Refresh-Hostcallbacks ist weiterhin nicht abschließend geprüft. V18 prüft normales Remove/Reopen zwischen Benachrichtigungen; SA01 fordert bewusst keine zusätzlichen Peer-Resizes an.
- **Codecs/State:** Gezielt beschädigte Fixtures und OOM-Injektion vorhanden, aber keine exhaustive WAV-/FLAC-/State-Fuzzing- oder vollständige Windows-Sanitizer-Kampagne.
- **Automationsumfang:** Pattern ParamID1000 ist die registrierte Hostautomation. Die übrigen gespeicherten Regler sind Controller-Kommandos. Keine unimplementierte Vollautomation aller Knöpfe zugesagt.
- **Physische GUI/D&D:** Reale Browserpayloads, Hostzoom/DPI, Dialoge und subjektive Lesbarkeit sind Owner-Hosttests. Vorhandene historische D&D-Bestätigung ersetzt keine Behauptung einer hier ausgeführten aktuellen Studio-One-Session.

Diese Grenzen sind gezielte Abnahmeaufgaben. Ein Audit ohne zusätzliche reproduzierbare Bugs wäre ausdrücklich zulässig; es werden keine hypothetischen Fehler repariert oder neue Feature-/Optimierungsprojekte erfunden.

## 8. Kurze Studio-One-Abnahmecheckliste

Die getestete Build-SHA und Host-/Windows-/ASIO-Version sowie Rate/Blockgröße protokollieren:

1. Neues Projekt, Plugin-Scan, Editor mehrfach öffnen/schließen; zwei Views und 100/150%-Zoom/HiDPI. M/S, Regler, Samplewechsel und Instrumentseiten in beiden Fenstern kontrollieren; Namenseditor Enter/Escape/Blur/Recall.
2. WAV/FLAC über Dateiauswahl, Explorer und Studio-One-Browser laden; Unicodepfade, falsche Datei und offene Menüs; `<`/`>`-Navigation sowie erhaltener alter Klang nach Reject.
3. Takt 1/Takt 2, Locate, Stop/Restart und Loop; A–H, 1T/2T, Song 8/16/32; gleichzeitige MIDI-/Hostauswahl und dichte Automation, auch während Kit-/Projekt-Recall.
4. Alle 16 Stereoausgänge mit unterscheidbaren Signalen; MAIN aus, AUX an/aus, gemeinsame Zielbusse und Recall. Choke bei dichten Hits abhören;64/128/256/512-Blockgrößen vergleichen.
5. Projekt nach Pattern-/Velocity-/Fill-/Undo-/Routing-/Sample-/Namensänderungen speichern, schließen und erneut laden. Hostanzeige, gehörtes Ergebnis, Dirty und Preview-ohne-Dirty kontrollieren. Ein echtes Altprojekt ergänzen.
6. Reale ASIO-Last mit 16 geladenen Rows, dichtem MIDI/Choke, Pitch und paralleler GUI/Import prüfen. 44.1/48/96k und kleine/große Blöcke; Xruns, p95/p99/Max sowie RAMspitzen festhalten. Keine Schlussfolgerung allein aus durchschnittlicher CPU.

### Umfang der tatsächlich integrierten Änderungen

Die Produktdiff gegenüber `72ee3c3` betrifft ausschließlich `SequencerCore.h`, `TriggerBatch.h`, `DrumatorController.cpp` und `DrumatorView.cpp`. `SampleVoiceEngine.h`, Version/CMake, IDs, SDK und Ressourcen sind gegenüber dieser Basis unverändert. Die ergänzten Test-/Diagnoseänderungen sind gezielt auf die genannten Gegenfälle begrenzt. Die drei dokumentierten Actions-Läufe waren kein wiederholtes Ausführen eines unveränderten grünen Standes: eigener konkret diagnostizierter Buildfehler, tatsächlicher Produkt-/Fixture-Gegenfall, abschließende korrigierte Gesamt-QA. Nach deren Erfolg wird kein weiterer Build für den Bericht ausgelöst.

## 9. Freigabeentscheidung und Übergabe

**Technisch bereit für die Studio-One-Abnahme – JA.** Begründung: die zwei konkret bestätigten Produktfehler sind gezielt korrigiert; die neue Sortier-Negativkontrolle erkennt weiterhin den ursprünglichen Fehler; die tatsächlichen Controller-/Windows-/Prozessorprüfungen und die vollständige vorhandene Windows-QA sind grün. T30 kann die untersuchten MIDI-Verluste jetzt erkennen, S16 wird durch den echten Merge-Pfad ergänzt und V18 durch tatsächliches Peer-Repaint/Paging.

Es gibt keinen Befund, der eine weitere spekulative Umbau- oder Testserie vor dem nächsten realen Hostschritt rechtfertigt. Die offengelegten Mess-/Hostgrenzen bleiben bestehen. **Eine V1-/Vintage-GUI-Freigabe wird nicht erteilt.** Die endgültige Gestaltung bleibt unangetastet; zuerst die kurze echte Studio-One-Abnahme und danach Eigentümerentscheidung. Bei einem dort tatsächlich reproduzierten Fehler gezielt nachbessern, statt vorsorglich funktionierende Bereiche umzubauen.

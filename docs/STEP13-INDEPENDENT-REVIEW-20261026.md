# 125A Drumator – Schritt 13: unabhängige Übernahmeprüfung (26.10.2026)

**Status: Entwicklungskandidat. V1-FINAL nicht freigegeben. Studio One / ASIO / 125A Plugin Tester: NOT RUN.**

## Verifizierte Übergabe

- Repository: `challanger2000/125A-Drumator`; Branch: `gui/ppg-blue-background-20261024`.
- Ausgangs-HEAD: `fe01de080c2acdec61e5478aed0854d31dc7a224`. GitHub Actions [#40364959472](https://github.com/challanger2000/125A-Drumator/actions/runs/40364959472) hat für **exakt diesen Commit** SUCCESS gemeldet; Windows-Job: Build, CTest, SDK-Validator, Packaged-Audio-/Editor-Smokes erfolgreich. 23/23 CTests und 47/47 Validator sind der belegte **Ausgangsstand**.
- Die Entwicklungs-Version bleibt 0.1.0; die abgenommenen GUI-/SONG-/Sample-Editor-Grafiken sind unverändert.
- Die Baseline ist **kein veröffentlichtes FULL/V1-Release**. `125A-Engineering/RELEASE-REGISTER.md` enthält keinen Drumator als ausgeliefertes Finalprodukt; keine RELEASE-Freigabe aus Dateinamen abgeleitet.
- Die Regeln aus `125A-Engineering/START-HERE.md`, den relevanten Engineering-Standards, `AGENTS.md` sowie dem bestehenden [S13-Audit](STEP13-TECHNICAL-AUDIT-IN-PROGRESS.md) wurden als Grundlage gelesen. Dies ist eine gezielte unabhängige Nachprüfung der S13-Meter-Reparaturen, **kein behaupteter neuer Vollaudit**.

## Unabhängige Quellprüfung der S13-Reparaturen

| Anforderung | Kontrollierter Code/Test | Bewertung vor neuem Windows-Lauf |
|---|---|---|
| Messwerte über VST3-Verbindung | `Processor::notify(MeterRequest)`, `Controller::notify(MeterSnapshot)`; S13-Integration mit absichtlich unauflösbarer lokaler `MeterId` | Source-Pfad vorhanden; real getrennter Host-Prozess **NOT RUN** |
| Mehrere Editoren | `DrumatorView::pollMeter` liest gemeinsam Controller-Momentaufnahme; S13 zwei native Views | Testorakel vorhanden; Windows-PASS am Ausgangscommit |
| Peak über lauten und stillen Block | `OutputMeter::accumulate/observe` und S13 Transienten-Orakel | Testorakel vorhanden; Windows-PASS am Ausgangscommit |
| Mehrere aktive Outputs, exakt 0 dBFS | S13 unabhängiger `OutputMeter::observe`-Test sowie 11I tatsächlicher Processor-PCM-Render | Testorakel vorhanden; echtes Studio-One-Multi-Out **NOT RUN** |
| GUI-/Nachrichten-Ausnahmegrenzen | `Controller::requestMeterSnapshot/clearMeterClips`, `Processor::notify`, S13 Alloc-Fault | Ursprüngliche Exception-Abwehr vorhanden, aber unten dokumentierte Datenverlustlücke |
| Transport, Cold-SONG, Cycle, Rücksprung, 16 Stems, Recall | A10–A26/RE09–RE11/PackagedHost und S13-Bericht | Automatisierte Testpfade vorhanden; realer Studio-One-8.1.2-Host **NOT RUN** |

### Neuer konkreter Befund S13-08 – verlorene Transienten bei fehlgeschlagener Meter-Antwort

**Ursache:** Beim S13-Ausgangscommit entnahm `Processor::notify("Drumator.MeterRequest")` die MAIN-L/R-Peaks durch `exchange(0)`, *bevor* der Response-Transport erfolgreich sein konnte. Scheiterte danach ein `IAttributeList::setInt` oder `sendMessage` (Fehlerstatus oder C++-Exception), waren beide aus dem Audiothread gemessenen Peaks aus dem Transport-Akkumulator gelöscht. Der alte S13-Test injizierte ausschließlich einen Fehler während des **Controller-Anforderungsaufbaus** und konnte diesen späteren Fehler nicht entdecken.

**Korrektur (dieser Commit):** Die MAIN-Peaks werden zunächst nur atomar gelesen. Erst nach `sendMessage(reply)==kResultOk` werden genau diese Werte durch bedingtes CAS auf null verbraucht; zwischenzeitlich vom Audiothread veröffentlichte **höhere** Werte bleiben für die nächste Anfrage erhalten. Dadurch entsteht weder ein zusätzlicher positiver Schreiber am Audio-Akkumulator noch ein Lock, Heapzugriff oder Hostaufruf in `process()`. CLIP bleibt weiterhin ein separater Sticky-Latch.

**Regression:** `test13MeterTransportAndMultiEditor` injiziert anschließend sequenziell bis zu 32 tatsächliche C++-Allokationsfehler in `Processor::notify` inklusive später SDK-Nachrichtenattribute. Fehlgeschlagene Antworten dürfen keine Peaks entfernen oder Exceptions aus dem Host-Callback entweichen lassen. Der positive Kontrollpfad fordert anschließend nachweislich die korrekte Controller-L/R-Antwort und die einmalige erfolgreiche Konsumtion. Fehlerfälle, bei denen gar keine spätere Allokation erreicht wird, zählen **nicht** als ausreichende Abdeckung. Der unveränderte Mehrfach-Editor-/CLIP-/Decay-Test bleibt aktiv.

**Abnahme:** Die Tests müssen auf Windows für den exakten neuen HEAD ausgeführt und im Job nachgewiesen werden; vor dem Build ist dies **NOT RUN**. `kResultOk` bestätigt nur die lokale Host-Annahme der Nachricht, keine garantierte spätere Zustellung über einen externen IPC-Prozess. Ein echtes Mehrprozess-Host- und Unterbrechungs-/Recovery-Szenario bleibt offen.

## Offene reale Freigabeprüfungen – alle NOT RUN

1. **Studio One 8.1.2 Pro auf Windows 11/ASIO:** Stop/Play, Cycle an/aus, nahtlose Sample-Tails über Loop, Start ab Takt 2, tempo-/blockgrößenabhängiges Timing und 1-/2-Takt-Pattern-Song-Übergänge, unter hörbarer 16-Output-Last.
2. **Multi-Out:** MAIN und DRUM 2–16 im Host aktivieren, routen und in einem gespeicherten Song erneut laden; Instrument-Zuordnung sowie parallele CLIP-Anzeigen getrennt prüfen.
3. **Realer Projekt-Recall:** SAVED Kit mit eingebetteten Samples, Song A–H, Misch- und Master-Reglern, Zoom und Audio-Ausgängen; Studio One schließen, neu öffnen und Audio-/Pattern-/GUI-Zustand sowie Sample-Navigation vergleichen.
4. **Mehrprozess-IPC:** echte getrennte Audio-/Controller-Adressräume, Editor-Mehrfachinstanz, wiederholte Polls, Stop, Reset, Verbindungsabbruch/-aufbau, ungeordnete/verspätete Antworten. Der Registry-Null-Test ersetzt dies ausdrücklich nicht.
5. **125A Plugin Tester:** gegen das tatsächlich für Windows gebaute `.vst3` aus dem neuen Commit, mit nachvollziehbarem Report und Versions-/Binäridentität. Kein Tester-PASS ohne ausgeführtes Tool.
6. **Abschlusskontrolle:** den gesamten S13-Bericht gegen Code, Nachweise und noch offene Negativpfade freigeben lassen. Kein automatisiertes PASS setzt eine Owner-/V1-Freigabe voraus.

## Scope und Statusregeln

Nur `OutputMeter.h` (Kommentar-Klarstellung), `DrumatorProcessor.cpp` (gezielter Transport-Fix), `WindowsIntegration.cpp` (Test) und dieser Bericht. Keine DSP-, Sequencer-, GUI-, State-, Parameter-, Lizenz- oder Versionsänderung. **Der Windows-Lauf für diesen neuen Commit ist zunächst NOT RUN und darf erst mit eindeutig zugeordnetem grünen CI-Nachweis auf PASS geändert werden.** GitHub Actions wurden durch einen gebündelten, nicht spekulativen Code-/Testcommit ausgelöst; weitere Runs nur bei konkretem Bedarf.

## Windows-QA-Nachlauf 1 – bewiesene noch offene Exception-Lücke

- [Actions #40368521712](https://github.com/challanger2000/125A-Drumator/actions/runs/40368521712), Commit `6c2a0755bb226d61bdce39058b82f1862ffba965`: **FAIL**. MSVC-Build SUCCESS, **22/23 CTests**, `drumator_integration_tests` FAIL: `S13 Processor meter reply fault escaped host notify`. Validator/Packaged-Bundle wegen fehlgeschlagener CTests **SKIPPED**.
- **Konkrete Folgerung:** Der erste korrigierte Reply-Verbrauch war quelllogisch ausreichend gegen spätere `setInt`-/Send-Fehler, aber die ältere Ausnahmegrenze erfasste noch nicht den potentiell allokierenden `reply->getAttributes()`-Aufruf unmittelbar nach dem Nachrichtenaufbau. Dadurch konnte eine injizierte `std::bad_alloc` aus `Processor::notify` entweichen.
- **Nachbesserung:** Die Attribute-Getter-Grenze erhält ein eigenes C++-Exception-Guard mit dokumentiertem Rückgabefehler. Auch `Controller::notify(MeterSnapshot)` schützt jetzt das Attribut-Decoding als extern aufrufbare VST3-Hostgrenze. Die bestehende Test-Injektion bleibt strikt, und die Fehlermeldung nennt bei einem erneuten Testversagen den jeweiligen Allokationsindex.
- Der Nachlauf für den exakten Nachbesserungs-Commit ist **NOT RUN**, bis eine neue Windows-Actions-Run-ID mit belastbarem Ergebnis vorliegt. Keine Umbenennung des fehlgeschlagenen Vorlaufs in PASS.

## Windows-QA-Nachlauf 2 – unabhängig ausgewerteter Erfolg (26.10.2026)

- **Exakter getesteter Codecommit:** `b53603f84fd44002f3b7c12a8238af15d56769bb`.
- [Actions #40369818428](https://github.com/challanger2000/125A-Drumator/actions/runs/40369818428): **SUCCESS**, 23/23 CTests, inklusive S13 Mehrfach-Editor-/Missing Registry-/Ausnahme-/Peak-Regression, 47/47 offizieller Steinberg Validator, paketierte Audio-/Editor-Smokes PASS; Windows-Runner, nicht Studio One.
- Echte Logbelege: `S13 IPC meter with absent registry and two native editors PASS`, `A26 combined 16 active buses, 64 voices, 12 imports, Cycle Stop and PCM PASS`, `100% tests passed, 0 tests failed out of 23`, `Result: 47 tests passed, 0 tests failed`.
- F14 48 kHz/256 Frames/64 Voices/16×: p95=0,6251 ms, p99=0,671 ms, Maximum=0,7159 ms, 0/180 Deadline-Misses auf dem geteilten CI-Runner. **Keine ASIO-Garantie und kein kontrollierter CPU-Before/After-Vergleich.**
- Windows Build-Artefakt: `125A-Drumator-M11-Full-Kit-Stress-QA` (ID `12671344093`, Actions-Archiv-Digest `sha256:719ccf182aa83eecfce7493e0a8a0a08f3fa3c8525e6d56e9485e8e52dcae11e`), archiviert mit diesem geprüften Run. Archiv-Digest ist kein Hash der enthaltenen VST3-DLL.

## Separates 125A Plugin Tester Gate

Um ohne erneute vollständige Compilation das **tatsächlich gebaute VST3-Bundle** zu prüfen, wurde `.github/workflows/125a-plugin-tester-qa.yml` als einmalig bei ihrer eigenen Änderung auslösbare, vom Core-Build getrennte Windows-QA eingerichtet. Sie prüft die exakte Actions-Baseline `b53603f`, lädt ausschließlich das nachweislich erfolgreiche VST3-Archiv aus Run #40369818428, prüft Identität und den SHA-256 der **offiziellen 125A Plugin Tester v0.2.10 Release-EXE** (`b0f386422d107ac6fa8b15c3e1e86113f52b013e277fc94b16d28b929438bfea`) und führt `--portable-test` mit eigenem Berichtsordner aus. Berichte werden unabhängig vom Testausgang hochgeladen. Vollständige 125A-Tester-Abnahme erst nach Auswertung des separaten Runs. Dies ersetzt keinen realen Studio-One-/ASIO-Hosttest.

### Erster 125A Tester-Actions-Nachlauf – Auswertungsinfrastruktur

[Workflow #40370763768](https://github.com/challanger2000/125A-Drumator/actions/runs/40370763768) hat das erfolgreich gebaute VST3 aus Drumator-Run #40369818428 geladen (passender Archiv-Digest), den v0.2.10 Tester mit geprüftem EXE-Hash gestartet und tatsächliche QA-/JSON-/Validator-/I-O-/Editor-Lifecycle-Berichte erzeugt (Report-Artefakt #12671514089). Die nachgeschaltete Wrapper-Prüfung meldete jedoch `Tester report is missing a complete result summary`, weil der PowerShell-Regex **doppelte Backslashes** enthielt. Damit ist **der Workflow FAIL**. Keine Tester-PASS-Aussage ohne korrekt erfassten Exitcode und Reportklassifikation. Eine gezielte Korrektur der Skript-Escapes löst nur den kurzen Tester-Workflow erneut aus; Drumator-Binary und seine 23/23+47/47 Windows-Kernaudits bleiben unverändert.

## Abschluss der separaten 125A-Tester-Prüfung – 26.10.2026 (maßgeblich)

**Exakt geprüfter Drumator-Produktcode bleibt** `b53603f84fd44002f3b7c12a8238af15d56769bb` (Version **0.1.0**, Entwicklungskandidat). Der jüngere Dokumentations-/CI-Branch-HEAD ist nicht mit einer erneut gebauten VST3-Binärdatei zu verwechseln. Die gesonderte Tester-Prüfung wurde ohne Wiederholung des vollständigen Produkt-Builds durchgeführt.

- **Eigenständiger echter Windows-Lauf:** [125A Plugin Tester #40371483389](https://github.com/challanger2000/125A-Drumator/actions/runs/40371483389), **SUCCESS**. Eingangsartefakt [Windows-Core-QA #40369818428](https://github.com/challanger2000/125A-Drumator/actions/runs/40369818428), ID `12671344093`, heruntergeladen und vom Actions-Downloader mit SHA-256 `719ccf182aa83eecfce7493e0a8a0a08f3fa3c8525e6d56e9485e8e52dcae11e` geprüft.
- **Tester:** offizielle 125A Plugin Tester v0.2.10 Portable-EXE, Release-SHA-256 `b0f386422d107ac6fa8b15c3e1e86113f52b013e277fc94b16d28b929438bfea` erfolgreich bestätigt. Der Workflow benutzte `--portable-test` gegen die originale paketierte `125A Drumator.vst3`.
- **Original-Tester-Ergebnis im Windows-Joblog und TXT/JSON:** `RESULT: 58 PASS / 0 WARNING / 0 FAIL`, `RELEASE: PASS` im Tester-eigenen Report, Exitcode `0`, JSON-Status `PASS`. Dies ist eine **technische Tester-Klassifikation, keine Freigabe des 125A Drumator V1-FINAL**.
- Getestete Pfade umfassen VST3-Laden, Komponenten und Controller, 16 Stereo-Ausgänge und MIDI, Roundtrip/Frischinstanz-State, 50/50 gültige Sample-Rate/Blockgrößen-Setup-Kombinationen, tatsächliche Host-Matrix, mehrfachen Editor-Lifecycle, I/O- und Event-Checks, überlastungs- und NaN-bezogene Checks sowie die separat integrierte Steinberg-Validierung.
- Exakte Tester-Diagnostik als [Actions-Artefakt #12673550021](https://github.com/challanger2000/125A-Drumator/actions/runs/40371483389/artifacts/12673550021), Archiv-SHA-256 `c2e9d0a385b99a4d171697626a2a689c70d7a2a636f0a70928c29117b30f92c4`.
- Vorläufe [#40370763768](https://github.com/challanger2000/125A-Drumator/actions/runs/40370763768) (fehlerhafte Report-Regex-Escapes) und [#40371070628](https://github.com/challanger2000/125A-Drumator/actions/runs/40371070628) (ungültige YAML/PowerShell-Zeichenkette) bleiben historische **FAIL**, nicht Produktfehlerbelege. Der korrigierte Workflow `125a-plugin-tester-qa.yml` wurde im erfolgreichen Run #40371483389 vollständig ausgeführt.

### Unabhängige technische Schlussbewertung des S13-Berichts

Die reparierten S13-Meter-Anforderungen haben jetzt nachvollziehbare **Windows-automatisierte** Positiv-/Negativbelege. Bei der externen Nachkontrolle wurde zusätzlich **S13-08** als ungetesteter Peak-Verlust bei fehlgeschlagener IPC-Antwort gefunden, mit eigenem Test erfasst und nach zunächst nachgewiesenem FAIL auf `b53603f` korrigiert. Für die übrigen dokumentierten Transport-, SONG-, 16-Bus-, Realtime-, Recall- und Sample-Tests gelten die tatsächlich in der bestehenden Suite ausgeführten Windows-Orakel; bereits positive Tests wurden nicht grundlos neu geschrieben. Das Prüfergebnis reicht für den **Windows-geprüften Entwicklungskandidaten** mit getrenntem Tester-PASS.

**NOT RUN / weiterhin blockierende Hostfreigaben:** Studio One 8.1.2 Pro unter Windows 11 mit realem ASIO-Gerät; Stop/Play und Cycle im eigenen Projekt, Start ab Takt 2, tatsächlich durch den Host geroutete 16 Ausgänge und langfristige Last; Save/Close/Reopen einschließlich SONG, Kit und Samples; echte Host-IPC mit physisch getrennten Controller-/Audioprozessen einschließlich Verzögerung/Abbruch/Recovery. Die Registry-Null-Negativkontrolle emuliert *keinen* unabhängigen Prozess. Keine V1-FINAL-/Gumroad-Freigabe ohne real ausgeführte und dokumentierte Host-Abnahme und Besitzerentscheidung.

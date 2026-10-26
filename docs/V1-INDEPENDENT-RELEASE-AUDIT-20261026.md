# 125A Drumator — unabhängiger V1-Release-Audit (Befundstand, KEIN FINAL)

**Auditdatum:** 26.10.2026  
**Untersuchte Quellrevision:** `1ee12c6c3d10f11333e98e69edba77c8b6f69e5a`  
**Branch:** `gui/ppg-blue-background-20261024`  
**Pluginversion:** `0.1.0`, unveröffentlichter V1-Kandidat  
**Vorgehen:** lesender GitHub-Quellcode-/Test-/Workflow-Review und Kontrolle der **vorhandenen** GitHub-Actions-Protokolle. Keine Quellreparaturen, neue Funktionen, neue CI-Läufe oder Release-Aktionen. Dieser Dokumentationscommit ist ausdrücklich **nicht** Bestandteil des geprüften Produkt-HEAD.

## 1. Freigabeentscheidung und Nachweisgrenze

**V1-FINAL: NICHT FREIGEGEBEN.** Der geprüfte HEAD besitzt einen aktuellen erfolgreichen Windows-Core-/VST3-Lauf, jedoch keinen vollständig durchlaufenen unabhängigen 125A Plugin Tester **auf genau dieser Binärdatei**, keinen belegten real getrennten Processor-/Controller-Prozess und keinen Studio-One-8.1.2-/Windows-11-/ASIO-Besitzertest.

Diese Prüfung umfasst eine unabhängige Kontrolle der Repository-Struktur und der kritischen Implementierungs-, State-, Scheduler-, Audio-, GUI-, Test-, Paket- und Workflow-Pfade. Sie ist **kein Nachweis einer lückenlosen formalen Zeilenprüfung jeder Quellcodedatei** und keine eigene dynamische Windows-/ASIO-Ausführung. Nicht inspizierte einzelne Codepfade oder unerprobte Host-Modelle gelten ausdrücklich **nicht** als fehlerfrei. Bewertung: technisch fortgeschrittener, Windows-geprüfter Entwicklungsstand, aber **kein belastbar final abgenommener V1-Release**.

## 2. Verbindliche Ausgangsbasis

Beachtet: `125A-Engineering/START-HERE.md`, Engineering-README, die einschlägigen Release-/Realtime-/Repository-/DSP-/GUI-/Source-Baseline-Vorgaben, dieses Repositories `AGENTS.md`, `README.md`, `docs/V1-SCOPE.md`, `docs/STEP13-INDEPENDENT-REVIEW-20261026.md`, `docs/V1-FOUR-CORRECTIONS-QA-20261026.md`, `docs/SONG-START-REST-SILVER-GUI-QA-20261026.md`. Die älteren Statusabschnitte in `AGENTS.md` sind historische Evidenz, **nicht** die aktuelle Testergebnis-Baseline.

Quellcode-/Pfadkontrolle mit besonderer Tiefe:
- `src/core/`: `SequencerCore.h`, `SongChain.h`, `AtomicPatternBank.h`, `AtomicPatternSelection.h`, `PatternEventTimeline.h`, `TriggerBatch.h`, `SampleVoiceEngine.h`, `SampleExchange.h`, `WavPcm16Decoder.h`, `SampleEdit.h`, `KitState.h`, `KitRestoreGate.h`, `OutputMeter.h`, `UiSongLayout.h`, `UiCompactLayout.h`, `UiWaveEditorLayout.h`, `PatternOperations.h`, `PatternUndo.h`, `RowFill.h`, `Utf8Path.h` und angrenzende Kontroll-/Geometriepfade.
- `src/vst3/`: `DrumatorProcessor.cpp/.h`, `DrumatorController.cpp/.h`, `DrumatorView.cpp`, `FlacImport.cpp`, `PluginIDs.h`.
- `tests/`: CTest-Registrierung, `SongChainTests.cpp`, `KitStateTests.cpp`, `TriggerBatchTests.cpp`, `SampleVoiceTests.cpp`, Integrationsfälle in `WindowsIntegration.cpp` und `FinalAudit20261023.inl`, Protokolle des paketierten Audio-/Editor-Hosttests, Testkonzept in `tests/integration/README.md`.
- `CMakeLists.txt`, `.github/workflows/core-ci.yml`, `.github/workflows/125a-plugin-tester-qa.yml`, gebündelte Ressourcenliste und vorhandene Nachweise für echte VSTGUI-Offscreen-Pixel.

Es wurden **nicht** alle möglichen Windows-Host-Aufrufsequenzen dynamisch erprobt und **nicht** alle Projektdateien vollständig Zeile für Zeile neu verifiziert. Das ist eine offen ausgewiesene Auditgrenze, kein PASS.

## 3. Aktuelle, exakt zuordenbare Nachweise

**[Windows-Core-/VST3-Run #40399811503](https://github.com/challanger2000/125A-Drumator/actions/runs/40399811503)**: `head_sha=1ee12c6c3d10f11333e98e69edba77c8b6f69e5a`, SUCCESS. Job-Originalprotokoll: **23/23 CTests**, **47/47 Steinberg Validator**, paketierte Windows-DLL mit Audio-/MIDI-/Multi-Out- und Editor-Smokes PASS. Die neueste `GUIFINAL`-Testgruppe meldet SONG START/REST-Hitbereiche und native 100/130-%-Renderings PASS. Weitere Testevidenz: SONG START mit BAR2/BAR4, REST/Tails, Cycle/Tempo, DM17/DM16, 16 aktive Stereo-Busse und 64 Stimmen, gespeicherte Samples, Negativzustände und Restore-Proben. Archivierte Artefakte: `12688172737` (paketiertes VST3), `12687543646` (native VSTGUI-Renderings).

**Performance-Evidenz ausschließlich offline im CI-Runner:** F14, 48 kHz/256 Frames, 180 Blöcke: p95 0,6366 ms; p99 0,6863 ms; max. 0,7185 ms, nominelle Blockfrist 5,3333 ms, 0/180 Offline-Fristüberschreitungen. **Keine** ASIO-/Studio-One-Hard-Realtime-Garantie.

**[Tester-Run #40395721734](https://github.com/challanger2000/125A-Drumator/actions/runs/40395721734)**: 125A Plugin Tester V0.3.2, **58 PASS / 0 WARNING / 0 FAIL**; bewusst gegen das Paket der Revision `989f454e9ed00ef175b1a2c1c25927d56c0aa134` gepinnt. **Dieser erfolgreiche Tester-Lauf prüft nicht die letzte DLL von `1ee12c6`.** Der neue CORE-Run ersetzt den separaten Tester nicht.

Die genaue Aussagekraft bleibt pro Oracle begrenzt: Tests mit realen PCM-Werten, getrennten Kanälen, Status-/Rollback-Prüfungen und bewussten Negativkontrollen liefern wesentlich stärkere Evidenz als reine Funktionsaufrufe. Die GUI-Offscreen-Bilder stammen aus nativer VSTGUI-Renderung; ein direkter verlässlicher Screenshot eines unter Studio One laufenden Editor-HWND wurde nicht nachgewiesen. Keine Behauptung eines real getrennten Hostprozesses.

## 4. Priorisierte technische Befunde

### V1-A01 — HIGH — Anfrage/Rückantwort-Protokoll bei getrennter VST3-Hostverarbeitung nicht robust

**Typ:** statisch nachgewiesene Architekturabhängigkeit; tatsächlicher Fehler in einem Mehrprozess-Host noch nicht ausgeführt.  
**Quellen:** [`DrumatorProcessor.cpp`, ca. Zeilen 202–253](../src/vst3/DrumatorProcessor.cpp), [`DrumatorController.cpp`, ca. 940–1000](../src/vst3/DrumatorController.cpp), außerdem Processor ca. 197–211 und Controller ca. 189–212.

`Drumator.SampleEditorQuery`: der Controller sendet ein `IMessage` und liest nach `sendMessage(msg)` die vom Processor **in das empfangene Anfrageobjekt** geschriebenen Attribute (`Wave`, `Start`, `End`, Fades, Revision) wieder aus dem ursprünglichen `msg`. Analog liest `processorConfirmsPattern()` den vom Processor an **derselben Anfrage** ergänzten `Selected`-Wert. Dies funktioniert nur, wenn die Host-Verbindung die Identität/Mutabilität des Anfrageobjekts über den Rückweg erhält. Die VST3-Schnittstelle definiert eine Nachrichtenübergabe über `IConnectionPoint`; Hosts dürfen einen Proxy zwischen die Komponenten schalten. Ein mutierter Aufrufparameter ist **kein spezifiziertes Reply-Protokoll**. Die Meterkommunikation verwendet demgegenüber korrekt `MeterRequest` plus eigens versandtes `MeterSnapshot`.

**Technische Reproduktion:** Host-Proxy, der `IMessage` samt Attributen vor `notify` in einen separaten Speicherraum kopiert, nur das `tresult` zurückgibt und nur separat gesendete Nachrichten zurückstellt; Sample laden und Editor öffnen. Der Processor verarbeitet die Anfragekopie erfolgreich; der Controller erhält in seinem Ursprungsobjekt kein `Wave` und `sampleEditorSnapshot()` liefert false. Bei `QueryPattern` fehlt `Selected`, wodurch die Host-Pattern-Arbitration auf den Fallback zurückfällt. Ein echter Studio-One-/Mehrprozess-Fail wurde **nicht** behauptet.

**Release-Auswirkung:** unter unterstützten getrennten Host-Prozessmodellen können Sample-Editor/Pattern-Reconciliation versagen. **Blocker für eine allgemeine Mehrprozess-Kompatibilitätsbehauptung.**

**Pflichtmaßnahme:** Korrelation-ID plus eigenständiges `SampleEditorReply`/`PatternReply`, Fehler-, Timeout- und veraltete-Revision-Semantik festlegen; negativen Proxy-Test mit *kopierten* Nachrichten ausführen. Weder ein zusätzlicher In-Process-Test noch ein Validator-PASS beseitigt dieses Risiko. VST3-Hinweis: [Steinberg IConnectionPoint](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1Vst_1_1IConnectionPoint.html).

### V1-A02 — HIGH — Unabhängiger Tester-Workflow prüft weiterhin dauerhaft den alten Artefaktstand

**Typ:** nachgewiesener Test-/Provenienzdefekt, kein unmittelbar nachgewiesener DSP-Fehler.  
**Quelle:** [`.github/workflows/125a-plugin-tester-qa.yml`, Zeilen 1–25, 95–110](../.github/workflows/125a-plugin-tester-qa.yml).

Der Workflow verifiziert explizit Run `40394744591` / Quell-SHA `989f454...`, lädt das alte VST3-Bundle und verlangt die alte DLL-SHA `9a8b11bc...`. Sein `push.paths`-Filter reagiert nur auf Änderungen an dieser Workflow-Datei; Änderungen an `src/`, `tests/` oder `resource/` lösen diesen Tester nicht automatisch aus. Der jüngste Tester-PASS ist deshalb **nicht** dem HEAD `1ee12c6` zuzuordnen.

**Reproduktion:** Werte für `run-id`, Quell-SHA und DLL-SHA im Workflow mit den beiden Actions-Läufen vergleichen; unabhängig von aktuellen Source-Commits wird die ältere Binärdatei geladen.

**Release-Auswirkung:** Eine grüne unabhängige Tester-Karte kann fälschlich als Test der aktuellen DLL verstanden werden. **Release-Gate offen.**

**Pflichtmaßnahme:** vor Freigabe provenance-sichere Verknüpfung mit dem exakten aktuell paketierten Build und dessen Binär-Hash herstellen; danach den bestehenden V0.3.2-Tester **einmal gezielt** gegen das neue Paket ausführen und TXT/JSON/Exitcode archivieren. Keine Notwendigkeit, vor dieser Korrektur blind neue Builds zu starten.

### V1-A03 — MEDIUM — Stiller Verlust von MIDI-/Step-Ereignissen bei ausgeschöpftem Trigger-Puffer

**Typ:** aus Quellcode und vorhandenem Grenzwerttest belegte Funktionseinschränkung; kein alltäglicher 120-BPM-Fehler nachgewiesen.  
**Quellen:** [`TriggerBatch.h`, Zeilen 15–40](../src/core/TriggerBatch.h), [`DrumatorProcessor.cpp`, ca. 1318–1330 und 1350–1385](../src/vst3/DrumatorProcessor.cpp), [`tests/TriggerBatchTests.cpp`, Zeilen 63–69](../tests/TriggerBatchTests.cpp).

Feste Trigger-Kapazität: `kRows * maxBoundariesPerBlock = 16 * 128 = 2048`. `appendStep()` und `appendNoteOn()` melden Überlauf korrekt durch `false`. Der Processor verwirft diesen Rückgabewert bei Step-Scheduling, MIDI-Note-ons und Preview; es gibt weder ein nach außen prüfbares Overflow-Flag noch eine kontrollierte Fehler-/Prioritätsstrategie.

**Reproduktion:** im Testhost 2049 gültige Note-ons im selben Audio-Block auf MIDI-Noten 36–51 schicken; die 2049. Note wird nach Füllen des Arrays still verworfen. Der vorhandene Unit-Test bestätigt nur die Kapazitätsgrenze, **nicht** eine sinnvolle Integrationsreaktion. Bei vielen dicht belegten Sequencer-Boundaries konkurrieren die MIDI-Hits um denselben Speicher.

**Auswirkung:** nicht-berichtete fehlende Hits bei Extremdichten/MIDI-Flood, kein nachgewiesener Speicherüberlauf. Für V1 mindestens eine bewusste Limit-/Prioritäts-/Diagnosespezifikation mit integriertem Negativtest festlegen; keine Heap-Vergrößerung im Audiothread.

### V1-A04 — MEDIUM — Host-Automation nur für Pattern A–H als VST3-Parameter verfügbar

**Typ:** bestätigte Funktionsgrenze; ob weitere Automation V1-Pflicht ist, erfordert eine explizite Scope-Entscheidung.  
**Quellen:** [`DrumatorController.cpp`, Zeilen 311–320, 900–950, 1170–1230](../src/vst3/DrumatorController.cpp), [`PluginIDs.h`](../src/vst3/PluginIDs.h), [`docs/V1-SCOPE.md`](V1-SCOPE.md).

Der Controller registriert in `initialize()` ausschließlich den `Pattern`-Parameter `kPatternParam=1000`. Master Volume, Swing, Humanize, Row-Gain/Pan/Pitch, Mute/Solo, Choke, Output und SONG-Konfiguration werden über private `IMessage`-Kontrollkommandos geführt und **nicht** als DAW-automatisierbare VST3-Parameter veröffentlicht. Persistenz über Kit-State ist keine Automation. Die aktuelle Sample-genaue Host-Pattern-Automation ist dagegen ausdrücklich vorhanden.

**Reproduktion:** öffentliche VST3-Parameterenumeration im Host; außer `Pattern` erscheinen keine genannten Knöpfe. **Kein falscher Anspruch**, dass diese Regler in Studio One bisher als Automation getestet seien. V1-Dokumentation präzisieren oder, falls Automation dieser Regler zugesagt war, als funktionsblockierenden Scope-Defekt behandeln. Neue Automationsparameter würden Kompatibilität und Host-Parameter-IDs berühren und dürfen **nicht** in diesem Audit eigenmächtig ergänzt werden.

### V1-A05 — MEDIUM — Taktartenannahme 4/4 im SONG-START-Mapping

**Typ:** ausdrücklich dokumentierte, statisch bestätigte funktionale Einschränkung, nicht überraschender Codefehler.  
**Quellen:** [`DrumatorProcessor.cpp`, ca. 1297–1317](../src/vst3/DrumatorProcessor.cpp), [`docs/SONG-START-REST-SILVER-GUI-QA-20261026.md`](SONG-START-REST-SILVER-GUI-QA-20261026.md).

`songOriginStep=(songStartBar-1)*16`; der Scheduler rechnet Host-PPQ über `4 Steps/Quarter`. Bei 3/4, 6/8 oder Taktwechseln entspricht die angezeigte BAR-Nummer nicht notwendig dem Hosttakt. Getestete BAR2/BAR4-Fälle sind 4/4-Beispiele.

**Auswirkung:** falscher musikalischer SONG-Start bei abweichender Hosttaktart. Vor V1 **4/4 als bindende Funktionsgrenze in Bedienungsanleitung/UI und QA** kennzeichnen oder echte Time-Signature-Abbildung implementieren und neu testen. 4/4-only als ausdrücklich akzeptierter Produktumfang ist kein automatischer Blocker.

### V1-A06 — MEDIUM — Build-Zulieferkette nicht vollständig bytegenau abgesichert

**Typ:** Reproduzierbarkeits-/Supply-Chain-Risiko, kein nachgewiesener kompromittierter Build.  
**Quellen:** [`.github/workflows/core-ci.yml`, Zeile 21](../.github/workflows/core-ci.yml), [`CMakeLists.txt`, Zeilen 215–224](../CMakeLists.txt), `AGENTS.md` SDK-Baseline.

Der SDK-Checkout nennt `v3.8.1_build_84` und `--recursive`, prüft im Workflow aber nicht noch einmal die in der Repo-Policy genannte exakte Commit-SHA `3cdf9ca5...` und Submodul-IDs. `dr_flac.h` wird von einer Commit-URL per TLS bezogen, **ohne `EXPECTED_HASH`**. Das ist besser als ein Floating-Master, aber keine vollständig unabhängig byteverifizierte Zulieferbasis.

**Maßnahme:** vor FINAL Commit-/Submodul-SHAs sowie SHA256 von `dr_flac.h` in CI fest verifizieren. Kostenneutral in der nächsten ohnehin begründeten Buildprüfung integrierbar.

### V1-A07 — HIGH (Release-Nachweislücke) — Studio One / ASIO / echte Host-Prozessmodelle nicht abgenommen

**Typ:** NICHT GEPRÜFT, kein behaupteter Bug.  
**Quellen:** oben genannte QA-Dokumentationen, jüngster Actions-Run und `AGENTS.md`.

Noch nicht durch die vorhandenen Windows-Proben ersetzbar: echtes Studio One 8.1.2 Pro mit Windows 11 und ASIO, 44,1/48 kHz, verschiedene Hostbuffer, Wiedergabestart ab BAR1/BAR2/BAR4, Pause/Stop/Play, Cycle-Sprünge innerhalb und zwischen Blöcken, Tempowechsel, Wechsel A–H über GUI/MIDI, SONG-REST mit nachklingenden Sample-Tails, Drag-and-drop WAV/FLAC bei Playback, 16 aktivierte Stereo-Busse, Projekt Speichern/Schließen/Neuladen, 100/130-%-VSTGUI samt echten Klickbereichen und langes Playback unter realer CPU-Last. Reale getrennte Processor-/Controller-Prozesse ebenfalls nicht belegt.

**Maßnahme:** definierte Anwender-Abnahmematrix mit Audioaufnahmen, genauen Repro-Schritten und Projektfiles; festgestellte Fehler vor Release beheben. F14 Offline-CPU ist kein ASIO-Stabilitäts-PASS.

### V1-A08 — LOW — Rendernachweis ist nicht dasselbe wie tatsächliche Studio-One-Fensterdarstellung

**Typ:** UI-Nachweislücke, nicht nachgewiesener Layoutfehler.  
**Quelle:** [`docs/SONG-START-REST-SILVER-GUI-QA-20261026.md`](SONG-START-REST-SILVER-GUI-QA-20261026.md), aktueller `GUIFINAL`-Test und Renderartefakt `12687543646`.

Die nativen Offscreen-Renders und hitboxbezogenen Integrationstests belegen Ressourcenzugriff, bestimmte Pixel/Geometrie und Eingabe-Verknüpfungen; sie beweisen nicht alle Interaktions- und Skalierungszustände im angehängten Studio-One-HWND. Direkte Anwender-Sichtprüfung bleibt offen. Der bisherige QA-Bericht benennt dieses Limit korrekt.

## 5. Positiv geprüfte Architekturpfade (keine allgemeine Fehlerfreiheitszusage)

- **Realtime-Kern:** `SampleVoiceEngine::render`, `TriggerBatch`, `StepScheduler`, `SampleExchange::audioAdopt/audioCollect` und `KitRestoreGate` besitzen begrenzte Arrays bzw. vorab aufgebaute Ressourcen. In den untersuchten Audio-Hauptpfaden kein unmittelbarer WAV-Dateizugriff, kein `kitMutex_` und keine explizite eigene `new`-/`vector::resize`-Operation innerhalb von `process()`. Das ist eine **Quellcodeeigenschaft der untersuchten Pfade**, kein umfassendes ThreadSanitizer-/Hard-Realtime-Zertifikat. Der 64-Voice-Deckel ist eine **absichtliche Ressourcenbegrenzung**; darüber werden Hits verworfen, was die Benutzerabnahme unter langen Tails berücksichtigen muss.
- **Timing/SONG:** 8 Patterns, 16/32 Schritte, SONG-Entry-Längen 8/16/32, REST-Sentinel, absolute BAR-Position, PPQ/Cycle-Segmentierung, atomarer Patternbank-Snapshot, MIDI/Host-Priorität sind im Code vorhanden. SONG-Prüfungen erzeugen/prüfen stellenweise echte Samples, nicht nur Funktionsrückgaben.
- **State:** `KitCodec::decode` baut Kandidaten vor dem Commit; DM17 speichert Start-Bar und REST, DM16 und ältere Formate werden erkannt. Controller- und Processor-Restore nutzen vorbereitete Kandidaten/Schutz gegen gemischte Audio-Generationen. Der vorhandene Host-Smoke prüft korrupte/abgeschnittene States und wiederholte Wiederherstellung. **Kein** eigener Beweis für jedes ältere Format in jeder möglichen DAW-Reihenfolge.
- **Samples:** WAV-Decoder prüft RIFF, Format, Kanalzahl, Nutzdatenlängen und nicht-endliche Float-Samples; FLAC-Import begrenzt Ausgabeframes. Audio übernimmt immutable vorbereitete Samples, Audio löscht keine Besitzerobjekte; alte Voice-Tails können auf die vorherige Probe verweisen. Ein perfektes Ergebnis für jede absichtlich korrumpierte Datei oder jede Speichermangelsituation ist **nicht** daraus ableitbar.
- **Paket/GUI:** aktueller Workflow prüft das exakt zusammengestellte VST3 mit `moduleinfo.json`, `Drumator.uidesc`, den genannten Knob-/Screw-/Faceplate-/Logo-Ressourcen, Validator und gepacktem Audio-/Editor-Test. Nachweis gilt für die paketierte CI-DLL, nicht für eine möglicherweise ältere lokal installierte Kopie.
- **Unit-Test-Qualität:** sinnvolle überprüfte PCM-/State-/Status-Oracles und begrenzte negative Ausführungsproben vorhanden. Dennoch entgehen In-Process-Tests, die `IMessage` by-reference übergeben, der Host-Proxy-Grenze A01. Der TriggerBatch-Grenztest prüft `false` korrekt, nicht die Processor-Gesamtreaktion A03. Die Log-Annotation `PASS` bezeichnet immer nur die ausgeführte Testmatrix.

## 6. Reparatur-/Verifikationsfolge (noch NICHT umgesetzt)

1. **P0:** A01 getrennte Nachrichten-Protokollierung/Antwortkorrelation und eigenständigen serialisierenden Proxy-Negativtest planen; Schnittstellen und State-Pairing vor Implementierung festlegen.
2. **P0:** A02 Tester-Workflow auf SHA-identische **aktuelle** Build-Artefakte ausrichten, Hash/GitHub-Run-Provenienz gegenprüfen; dedizierten Tester dann gezielt ausführen.
3. **P0 Release Gate:** A07 Studio-One-/ASIO-/Mehrprozess-Abnahme unter dokumentierten realen Bedingungen durchführen. Bis dahin kein V1 FINAL.
4. **P1:** A03 Overflow-Policy (MIDI, Sequencer, Preview) präzisieren, konkreten Processor-Überlauf-/Grenztest ergänzen, ohne den Audiothread zu allozieren.
5. **P1 Scope:** A04 explizit festhalten, welche VST3-Parameter in V1 automatisierbar sein müssen; bei tatsächlich zugesagtem Umfang vor Release beheben. A05 4/4-only dokumentieren oder vollständig neue Taktartenentwicklung separat planen.
6. **P2:** A06 Hash-/SDK-Pinning in bestehende CI-Sicherung integrieren, A08 finale reale Editor-Sichtprüfung ausführen. Bestehende Tests nur punktuell dort ergänzen, wo diese Befunde die konkrete Lücke begründen.

**Kein Reparatur-/GUI-Commit, kein Release-Merge, kein neuer Windows-Run im Rahmen dieses Audits.** Kein Bug wird allein wegen eines vorherigen PASS als geschlossen markiert. Nach Korrekturen sind SHA-genaue Retests erforderlich; das Ergebnis dieses Berichts gilt für `1ee12c6`, nicht automatisch für spätere Commits.

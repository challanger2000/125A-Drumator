# 125A Drumator — unabhängige Nachprüfung der V1-Audit-Reparaturen

**Prüfdatum:** 26.10.2026  
**Berichtsbasis:** `docs/V1-AUDIT-REPAIRS-20261026.md` bei `ce0211ddcf64b167e045d5ccbb26e7fc9fb115e7`  
**Tatsächlich geprüfter Produktcommit:** `7da6d5a4fd42a76fd8a561b9637ee8ebdd810c13`  
**Methode:** getrennte statische Kontrolle des Produktquellcodes, der Negativ-/Regressionstests und der Original-Joblogs/Artefakte von [GitHub Actions #40410436596](https://github.com/challanger2000/125A-Drumator/actions/runs/40410436596). **Keine Codeänderung, keine neuen Builds, keine lokale DAW-Ausführung.** Kein Anspruch auf formale Vollständigkeit sämtlicher Zeilen/Hostmodelle.

## Entscheidung nach Befund

| ID | Urteil | Unabhängig geprüfte Begründung |
| --- | --- | --- |
| **A01** | **PARTIALLY CLOSED** | Separate `SampleEditorReply` / `PatternReply` mit RequestId und `addRef/release` ersetzen tatsächlich die mutierte Anfrage. Der kopierende und verzögernde Proxy-Test einschließlich Restore und Hostautomations-Vorrang besteht. **Aber** die übrige Wellenform-Bedienung ist nicht vollständig asynchron weitergeführt (Details unten). |
| **A02** | **CLOSED** | Der 125A Tester wird durch `needs: windows` im selben Buildrun nachgeschaltet. Run, Artefakt und geprüfte DLL sind anhand der SHA- und Artifact-Nachweise eindeutig miteinander verknüpft; keine alte Drumator-DLL fest verdrahtet. |
| **A03** | **PARTIALLY CLOSED** | Die 2048er Kapazitätsgrenze, Ablehnung weiterer *gültiger* Trigger, kumulative atomare Zählung und `TriggerDrops` per Meter-Reply sind nachgewiesen. Der Eingangspfad iteriert aber nach Kapazitätserschöpfung weiter über **alle** Host-MIDI-Ereignisse ohne feste Obergrenze: die worst-case Audiocallback-Arbeit ist damit nicht strikt beschränkt. |
| **A04** | **CLOSED** | Dokumentiert und in `Controller::initialize` bestätigt: nur Pattern A–H ist regulärer VST3-Automationsparameter. Weitere Regler bleiben UI-/projektzustandsbasiert, ohne neue Automation. |
| **A05** | **CLOSED** | 4/4-Scope und 16 Sechzehntel je Takt sind in `docs/V1-SCOPE.md` ausdrücklich erklärt; SONG START rechnet entsprechend `(startBar-1)*16`. Keine Behauptung einer allgemeinen Taktartenunterstützung. |
| **A06** | **CLOSED** | Windows-Workflow validiert SDK-HEAD `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` und kontrolliert rekursive Git-Submodule auf ausgecheckte, zum jeweiligen Gitlink passende Revisionen. CMake lädt `dr_flac.h` vom gepinnten Commit und erzwingt `EXPECTED_HASH SHA256=111144e778f55738db6851cb226015c419e00d04b916a09506d4856d9cff945c`. |
| **A07** | **OPEN** | Echte Studio-One-8.1.2-/ASIO-Praxisabnahme ist nicht erfolgt und wird hier nicht simuliert. |
| **A08** | **OPEN** | Reale visuelle Abnahme im Anwenderhost bei 100/130 % ist nicht erfolgt; Offscreen-Renderings sind kein Ersatz. |

## Noch offene technische Ursachen — keine bloßen CI-Warnungen

**A01-1 (funktionsrelevant): Wellenformaktionen ohne asynchrones Ergebnis.** In [`DrumatorView.cpp` Zeilen 1759–1780 und 1816–1842](../src/vst3/DrumatorView.cpp) erwarten `zoomSampleEditor()` und `scrollEditorWave()` unmittelbar ein erfolgreiches `sampleEditorSnapshot()`. Bei einem Host, der die Antwort erst später liefert, endet die Aktion mit `ZOOM: WAVEFORM UNAVAILABLE` bzw. `SCROLL: WAVEFORM UNAVAILABLE`. Die spätere Reply ruft [`onSampleEditorReady()` (1718–1723)](../src/vst3/DrumatorView.cpp) auf, dieser verarbeitet aber nur `pendingEditorRow_` des **ersten Editor-Öffnens** und setzt die ausstehende Zoom-/Scroll-Aktion nicht fort. Dass der Ein-Fenster-Proxytest das verzögerte *Öffnen* besteht, falsifiziert diesen Gegenfall nicht. Auch Edge-Autoscroll beim Markerziehen nutzt dasselbe synchron erwartete Snapshot-Ergebnis.

**A01-2 (Mehrfenster): einzige globale Anfrageposition.** [`Controller::sampleEditorSnapshot()` (ca. 1022–1054)](../src/vst3/DrumatorController.cpp) hat nur `waveRequestPending_` und `waveRequestRow_`. Während eine Antwort für Zeile A aussteht, erzeugt die Snapshot-Anfrage einer anderen View für Zeile B **keine** eigene Query und liefert zunächst false; [`sampleEditorRequestPending(B)`](../src/vst3/DrumatorController.h) bleibt false. Die zweite View registriert deshalb keine verzögert abzuschließende Öffnung. Der Proxytest verwendet für diesen Pfad nur eine View. Eine beim letzten `unregisterView()` nicht freigegebene `waveReply_` hält außerdem ihre WAV-Binärdaten bis zur nächsten Anfrage/Invalidierung weiter im Speicher, obwohl `editorWaveCache_` geleert wird.

**A01-3 (Fehlergrenze):** In [`Controller::notify()` (PatternReply/SampleEditorReply, ca. 94–137)](../src/vst3/DrumatorController.cpp) sind `getAttributes`, `getInt/getBinary` und `addRef` nicht durchgängig gegen mögliche Host-/SDK-Ausnahmen geschützt; die bestehende MeterSnapshot-Verarbeitung besitzt dagegen einen äußeren `try/catch`. Ein OOM-/fehlerhafter Host-Replay an dieser ABI-Grenze bleibt ungetestet. Keine Aussage, dass diese Ausnahme im bisherigen CI aufgetreten sei.

**A03-1 (Realtime-Obergrenze):** [`DrumatorProcessor.cpp` ca. 1393–1409](../src/vst3/DrumatorProcessor.cpp) verarbeitet mit `for(i < data.inputEvents->getEventCount())` weiterhin sämtliche vom Host übergebenen Noten, selbst wenn `TriggerBatch::kCapacity` bereits erreicht wurde. `appendNoteOn()` lehnt danach zwar neue Einträge ab und zählt sie allokationsfrei, doch die Kosten steigen mit der Host-Ereignismenge statt einer festen Audioblock-Grenze. 2200 MIDI-Ereignisse im Test belegen die Zählfunktion, **nicht** eine harte Worst-Case-Deadline unter adversarialem Input. Die 2048er Speichergrenze selbst ist korrekt.

## Unveränderte, verifizierte QA-Evidenz

- **Windows-Run:** [#40410436596](https://github.com/challanger2000/125A-Drumator/actions/runs/40410436596), Jobs `windows` und `independent-plugin-tester / tester` **SUCCESS**, Produkt-SHA `7da6d5a4fd42a76fd8a561b9637ee8ebdd810c13`.
- **CTests:** **23/23 PASS**; darin `A01 independent copied and delayed Processor/Controller IPC PASS`, `A03 actual processor 2200-event bounded overflow PASS`. **Steinberg Validator: 47/47 PASS**. Paketierter Audio-/Editorhost: PASS.
- **125A Plugin Tester v0.3.2:** **58 PASS / 0 WARNING / 0 FAIL**, Exitcode 0, JSON PASS.
- **Build-Artefakt:** `12692093334` (Archiv-SHA256 `41e5886c6997a8e969c639c3f76903863448e59c08345b92926f6bcc33025760`); **getestete DLL-SHA256** `8e1b8d1815d2529a35ccc6417d15416637c488933469bfba91a4f82c9f17e89a`. Unabhängige Tester-Evidenz: Artifact `12692158314`.
- **A06-Restgrenze:** Die genannten *Quell-Abhängigkeiten* sind gepinnt und im Lauf geprüft. Nicht behauptet wird eine bitidentische Reproduzierbarkeit der gesamten Toolchain: `windows-2022` und `actions/*@v4` sind keine SHA-fixierten unveränderlichen Compiler-/Action-Images.

**Empfehlung ohne Implementierung:** A01-1/A01-2 samt Test für **verzögertes Zoom/Scroll und zwei verschiedene View-/Zeilenanfragen** sowie A03-1 per festem Realtime-Arbeitsbudget separat prüfen. Bereits geschlossene A02/A04/A05/A06 nicht erneut entwickeln.

**Freigabe:** **KEINE V1-FINAL-FREIGABE.** Technisch verbleiben A01 und A03 **PARTIALLY CLOSED**; A07/A08 bewusst **OPEN**. Keine Quellcodeänderung, keine neu gestarteten Windows-Actions und kein Merge nach `main`.

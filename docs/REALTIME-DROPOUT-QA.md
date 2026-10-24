# Drumator V1 — Audioaussetzer: gezielte Abnahme (22.10.2026)

## Status
Die vier Audits V1A01–04 sind auf dem Reparaturbranch durch Windows-QA abgedeckt. **Die drei in Studio One gehörten Audioaussetzer sind dadurch noch nicht abschließend erklärt oder behoben.** Keine Release-Freigabe ohne den tatsächlichen Host-Nachweis.

## Fehlerklassen getrennt messen
- **Audioengine liefert falsche Samples:** Ausgang mit konstantem Kontrollsound oder reproduzierbaren Hits mit unabhängigem Referenzrender vergleichen; Nullstrecken, Doppeltrigger, verschobene Onsets und alte Sampletails getrennt zählen.
- **Deadline verpasst:** Pro Callback Samplezahl, Sollfrist, Laufzeit p95/p99/max und Anzahl Fristüberschreitungen messen. Ein Offline-Test ohne Host-/ASIO-Pacing liefert hierfür nur diagnostische Hinweise.
- **Steuerthread blockiert:** WAV-/FLAC-Dateizugriff, Controller-Decode, IMessage-Kopie, Processor-Decode/Prepare und Rückgabe getrennt aufzeichnen. UI-Latenz darf nicht als bewiesener Audiobufferverlust bezeichnet werden.

## Pflichtszenarien
1. 44,1 kHz und 48 kHz, jeweils 64/128/256/512 Frames: Wiedergabe im Loop, Rücksprung, Stop mit und ohne Null-Frame-Callback, Stop→Play mit kontinuierlichem oder neuem PPQ, Start ab Takt 2, Locate in Songmitte.
2. Gleicher Samplewechsel mehrfach bei aktiver eigener Instrumentenzeile und bei unabhängig klingender Zeile; kurze und lange WAVs, FLAC, Source-Rate gleich/abweichend, Pitch 0/+12/-12; realen Controller→Processor-Nachrichtenweg verwenden.
3. Viele gleichzeitig aktive Voices/Rows, 16 Stereo-Outs soweit aktivierbar, stark gepitchte Sounds; mindestens 60 Sekunden gepacete Aufnahme in Studio One und dokumentierte ASIO-Overruns.
4. Negative Kontrollen: bei absichtlich doppeltem Loop-Trigger oder unterdrücktem Stop muss das Sample-/Timing-Orakel FAIL melden. Nicht die Zahl der erfolgreich abgeschlossenen Callback-Aufrufe allein zählen.

## Automatische Messung
F14 in `tests/SampleVoiceTests.cpp` protokolliert jetzt zusätzlich die **Anzahl der 48k/256-Deadline-Verfehlungen** über seine 180 Offline-Blöcke. **Eine beliebig schnelle CI-Maschine wird dadurch nicht zum ASIO-Testgerät.** Über 5,333 ms liegende Einzelzeiten werden ausdrücklich ausgewiesen, aber auf einem unkontrollierten Shared Runner nicht ohne Vergleichsbasis als deterministische Code-Regression gewertet.

## Ergebnisdisziplin
- Windows-CI-Ergebnis gilt nur für seine konkret ausgeführten Tests.
- Für eine Aussage „keine Audioaussetzer in Studio One“ sind zusätzlich Hostaufnahme und nachvollziehbare Frist-/Pufferstatistik erforderlich.
- Solange mindestens einer der drei Owner-Fehler reproduzierbar ist oder seine Ursache nicht überprüft wurde: **SOR04/R01/R03 = OFFEN; Release = NEIN.**

## A11 – regulärer VST3-Cycle-Rücksprung mit ausklingenden Samples (24.10.2026)

Bisher löste jeder abweichende Host-PPQ-Block `voices_.reset()` aus; damit wurden auch beim echten Cycle-Rücksprung laufende One-Shot-Tails abgebrochen. Die Korrektur behält Stimmen ausschließlich bei einem mit `kCycleActive|kCycleValid` markierten und geometrisch gegen die zuletzt erwartete PPQ-Position geprüften Wrap. Sonstige Locates und explizite STOPs behalten ihr Reset-Verhalten.

A11 rendert den echten VST3-Processor mit 2048-Frame Stereo-FLOAT32-WAV gegen einen 1280-Frame-Cycle. Der alte Tail und der neue Trigger müssen am Loop-Start gemeinsam L=0.25/R=-0.5 ergeben. Bei einem unmarkierten Locate oder unpassendem Cycle-Ende muss nur der neue Hit L=0.125/R=-0.25 hörbar sein. Die Tests prüfen außerdem vor dem Sprung den unveränderten Tail und die aktive Stimmenzahl. Bestehende SOR03/A10-Prüfungen laufen weiterhin. **Windows-Testergebnis nur nach Prüfung des zugehörigen exakten Commits; Studio One/ASIO weiterhin NOT RUN.**

## A12 — Rücksprung mitten im Audioblock (24.10.2026)

A11 deckte nur einen exakt blockgrenzigen Cycle ab. Bei nicht passend geteilten Audioframes kann der vorherige Block die Cycle-Endposition um einige Samples überschreiten; der folgende Host-`projectTimeMusic` beginnt dann bei `cycleStart + overshoot`. Der A11-Vergleich auf exakt `cycleStart` ließ einen solchen gültigen Wrap fälschlich als Locate erscheinen und setzte Stimmen hart zurück.

Die zusätzliche Prüfung erkennt nun das begrenzte Overshoot-Verhältnis `previousExpectedPpq - cycleEnd == newPpq - cycleStart`; sie verlangt die aktiven/gültigen VST3-Cycle-Flags, endliche geordnete Grenzen und einen zum vorherigen Maximalblock passenden Overshoot. Andere Locates, ungültige Cycle-Flags/-Grenzen und explizite STOPs setzen die Voices weiterhin zurück.

A12 verwendet den echten Windows-Processor: 48 kHz, 256 Frames, Cycle-Beginn bei PPQ 4 (Takt 2), Cycle-Ende nach 1152 Sampleframes (innerhalb des fünften Audioblocks), 2048-Frame Stereo-WAV. Vor dem Wrap ist das Signal über **alle** Frames L=0.125/R=-0.25; danach ergeben alter Tail plus neu gestarteter MIDI-Hit L=0.25/R=-0.5, bei drei Negativkontrollen nur L=0.125/R=-0.25. Ein ausdrücklich gemeldeter STOP muss den PCM-Ausgang vollständig stummschalten. Ältere A10/A11/SOR03 müssen weiterhin PASS sein. **Noch kein Windows-QA-PASS behauptet, bis der exakte Commit erfolgreich gebaut und getestet wurde.** Studio One/ASIO NOT RUN.

## A13 – sequenzierter Step innerhalb eines nicht gesplitteten VST3-Cycle-Blocks (24.10.2026)

Der Steinberg-VST3-ProcessContext-Vertrag erlaubt explizit, eine Loop-Grenze nicht als separaten Host-Audioblock auszugeben. Der alte Drumator-Processor führte den StepScheduler stets anhand einer einzigen linearen Host-PPQ-Zeit für den gesamten Callback aus. Damit konnte der neue Step an der inneren Cycle-Naht fehlen, obwohl die alten Sampletails bei A12 weiterliefen.

Die Korrektur unterteilt **nur das Sequencer-Scheduling** an einem bestätigten, gültigen Cycle in Sampleframe-Abschnitte; nach der Naht verwendet der Scheduler wieder die Cycle-Startposition und den Cycle-Ursprung. Schon gestartete Sample-Voices werden nicht zurückgesetzt. Es entstehen keine Audio-Callback-Allokationen, Locks oder Dateioperationen. Die Anzahl behandelter innerer Wraps ist durch den vorhandenen StepScheduler-Hardcap begrenzt; exotisch kurze Host-Cycles außerhalb dieser Grenze bleiben ein gesonderter Nichtfreigabe-Randfall.

A13 verwendet einen unveränderten Grid-Step 0 ohne externe MIDI-/Preview-Events, 48 kHz, 256 Frames und einen Cycle ab Projekt-PPQ 4, dessen Ende bei Sample 1152 liegt. Im fünften Callback muss der zweite Kick **genau ab Sample 128** zusätzlich zum Tail klingen (L/R 0.25/-0.5 gegenüber 0.125/-0.25 davor). Der folgende Callback muss beide Sample-Voices ohne Lücke erhalten. Ohne kCycleValid oder bei NaN-Grenze wird kein zusätzlicher Step ausgelöst, bei echtem PPQ-Locate werden alte Stimmen verworfen. A12 wird um denselben programmierten Schritt plus echten MIDI-Hit und ausdrücklichen STOP ergänzt. Windows-QA wird **erst nach** geprüftem Build/Test/Validator als PASS bezeichnet; Studio-One-/ASIO-Praxistest ist weiterhin offen.

## A14–A16 und RM19–RM21: Folgeregressionen der Cycle-Reparatur (24.10.2026)

**A14:** Der echte Processor rendert Pattern-A/B-Auswahl per MIDI-Programmumschaltung exakt 64 Frames vor und genau auf dem inneren Cycle-Rücksprung bei Frame 128; zusätzlich rendert er den autonomen Song-Modus über denselben Rücksprung. Die Voll-PCM-Referenzen unterscheiden die beiden Sample-Quellen, den vorherigen Tail und die neuen Stimmen.

**A15:** Bei aktivierter instrumentbezogener Random-Velocity ist die zu variierende Position die wirkliche musikalische PPQ *nach* einem inneren Cycle-Rücksprung, nicht die linear fortgeschriebene PPQ des kompletten Hostblocks. Ein 6000-Frame-Cycle bei 48 kHz/120 BPM beginnt bei PPQ 4 (Takt 2) und springt mitten im Hostblock zurück. Exakte 1024 Stereo-Samples des ersten und zweiten Umlaufs werden verglichen, mit positiven hörbaren Kontrollwerten. Die korrigierte `TriggerBatch::varyVelocity` erhält Cycle-Daten mit Standardwerten, wodurch alle bisherigen Aufrufer und normale Nicht-Cycle-Blöcke unverändert bleiben. Keine Heap-Allokationen, Locks oder Dateioperationen im Audio-Callback.

**A16:** Bei Host-Cycles kürzer als eine Callback-Länge kann ein 256-Frame-Block mehrfach die Cycle-Grenze durchlaufen. Die bisherige Prüfung nach A12 verglich nur den *unreduzierten* Overshoot und setzte beim darauffolgenden Callback legitime ausklingende Voices zurück. Nun wird der erwartete Offset modulo Cycle-Länge mit dem Host-PPQ abgeglichen; absolute Puffergrößen-/Zeitgrenzen und gültige Cycle-Flags bleiben verpflichtend. 64-Frame-Cycles werden mit vier präzisen Step-Retriggern pro Block und acht gleichzeitig klingenden Sample-Voices über zwei Callbackblöcke vollständig in Stereo geprüft, inklusive ungültiger Cycle-Grenze als Negativkontrolle.

**RM19–RM21:** Der generierte *Test*-Processor kann die drei Reparaturen separat absichtlich umgehen: Cycle-Split, PPQ-Remapping der zufälligen Velocity und Mehrfachloop-Erkennung. Für jede Mutation muss dieselbe native PCM-Regression erst ohne Mutation PASS und mit Mutation FAIL melden. Der produktive VST3-Code wird von diesen Mutationen nicht verändert. **PASS-Einstufung erst nach exakt zugeordnetem Windows-CI-Ergebnis; Studio One/ASIO NOT RUN.**

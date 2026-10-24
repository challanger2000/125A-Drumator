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

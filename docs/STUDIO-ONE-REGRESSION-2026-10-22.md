# Studio-One-Regressionsplan (22.10.2026)

Status: Testvorgaben, keine behaupteten PASS-Ergebnisse. Siehe Issue #7.

## R01 Step Velocity
Neue Steps müssen Velocity 100 statt 127 erhalten. Erneuter Klick deaktiviert den Step. Vorhandene Velocity, Pattern-Undo, Fill, Random und Kit-Recall bleiben unverändert. Echte Controller- und GUI-Pfade testen.

## R02 Sample-Navigation nach Wiederherstellung
WAV A und B aus einem Ordner laden; Prev/Next funktioniert. Prozessor- und Controller-State speichern, Instanzen zerstören, in neuer Instanz wiederherstellen: Ordnernavigation muss mit bestehenden Originaldateien erneut funktionieren. Ältere Kit-Formate behalten funktionierenden Embedded-Audio-Recall auch ohne Ursprungsdatei. Fehlender Pfad darf Playback nicht verhindern. Keine Dateisystemzugriffe im Audio-Callback.

## R03 Stop/Play während Song
Audio-Processor mit Host-PPQ ungleich null starten; stoppen, mit gleicher PPQ wieder starten; Test mit Null- und mehreren Stop-Blöcken, validem/invalidem Clock, 44,1/48 kHz und 128/256/512 Frames. Sample-offset-genaue Trigger und Blockpartition-Determinismus prüfen. Keine zusätzliche Phasenrücksetzung ohne reproduzierbaren Gegenbeweis.

## R04 Samplewechsel während Audio
Während Audioverarbeitung WAV wechseln, übrige Instrumente müssen ohne Dropouts laufen. Processor-Laufzeiten p95/p99/max sowie Worst-Case gegen Blockfrist messen. Original und Ersatzsample, beschädigte Datei und Lastfall prüfen. Echtzeit- und Host-Test nicht mit Offline-CTest gleichsetzen.

## R05 Undo und Texte
PATTERN-Menü mit wirklich gerenderten Labels/Hitboxen prüfen, Undo-Semantik nach FILL CLEAR verifizieren oder bewusst als nicht unterstützt deklarieren. Deutsche Beschriftungen korrigieren. Aktuelle Owner-PASS-Ergebnisse nicht erneut durchtesten.

## Freigabe
Fixes und Regressionsnachweise, dann gebündelte Windows-QA und Steinberg Validator, unabhängiger Codex-Audit, anschließend nur gezielte Studio-One-Nachtests. Kein V1 FINAL vor Nutzerfreigabe.

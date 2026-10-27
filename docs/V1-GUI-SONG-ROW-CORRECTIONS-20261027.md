# Drumator 0.1.0 — SONG REMOVE LAST / ROW-Menü, Windows-QA

**Datum:** 27.10.2026 (Europa/Berlin)  
**Entwicklungsbranch:** `gui/ppg-blue-background-20261024`  
**Geprüfter Quellstand:** `a14d9e67470ad0af94a8250baca13fa3a15fb70e`  
**Version:** `0.1.0` — **kein V1-FINAL, kein Merge nach main**.

## Geltungsbereich

Ausgangsbasis: Dokumentations-HEAD `482d8774a8bd6e101f3d5ce0472ab43345d49c50` nach dem erfolgreichen A01-/A03-Produktbuild. Diese Korrektur verändert ausschließlich die SONG-Chain-Bedienung, das ROW-Kontextmenü, die notwendige ROW-CLEAR-Controlleraktion und betreffende UI-/Regressionstests. Keine Audio-Engine-, Transport-, Pattern-IPC-, Mix-/Routing-, Sample-Decoder- oder GUI-Materialänderungen.

### SONG CHAIN / REMOVE LAST

- Die SONG-Kacheln werden nur bis `index < songCount()` gezeichnet und sind **nur** zur Auswahl bestehender Einträge verwendbar. Das zuvor anklickbare `+ ENTRY`-Feld und dessen automatische Erzeugung sind entfernt.
- `ADD ENTRY` ist der **einzige** Hinzufügen-Befehl. `REMOVE LAST` verarbeitet unabhängig von der Auswahl stets `adjustSongCount(-1)`, mit gültiger Auswahl und zum neuen letzten Eintrag passender Seite.
- Keine neue leere Navigationsseite bei genau 16/32/48 vorhandenen Einträgen; `UiSongLayout::canNext()` ist dafür streng `count > first(nextPage)`.
- Grenzen **1 bis 64** unverändert. Bestehende REST-/A–H-Daten und ihre Laufzeiten bleiben erhalten.

### ROW-Button / FILL

- Ein kleiner **ROW**-Button links neben dem ersten VOL-Drehregler pro Instrument, bestehende Metallbutton-Optik und exakter Hitbereich `x=28..94, y=rowTop+73..105`; die Instrumenten-Auswahl ist auf ihren oberen, eigentlichen Select-Button begrenzt, damit der untere ROW-Button nicht abgefangen wird.
- Eigenes ROW-Menü mit **COPY ROW / PASTE ROW / CLEAR ROW**. COPY/PASTE benutzen weiter die vorhandene Pattern-übergreifende Zeilen-Zwischenablage und die Undo-fähige Transaktion.
- **CLEAR ROW** setzt **alle 32 gespeicherten Steps/Velocity-Werte** der aktuellen Pattern-/Instrumentenzeile zurück, auch das verborgene zweite Taktsegment eines 1-Bar-Patterns. `Controller::replacePattern()` bewahrt Undo und lässt alle anderen Patterns/Zeilen, Sampledaten, Sounds, Mix, Choke und Output unverändert.
- FILL bietet ausschließlich seine **acht rhythmischen Fills**. Der vorherige CLEAR-Menüpunkt und ROW COPY/PASTE sind entfernt. Es wurden keine neuen Audiofunktionen ergänzt.

## Regressionen und tatsächliche QA-Evidenz

| Prüffeld | Messpunkt / assertierter Vertrag | Ergebnis |
|---|---|---|
| SONG Hinzufügen | Bei exakt 16 keine leere Folgeseite, ADD erzeugt Nr. 17 und wählt Seite 2; Klick auf leere Kachel und Raster-Zwischenraum ist wirkungslos; danach ADD Nr. 18 | PASS |
| SONG Entfernen | REMOVE LAST bei 18/17/16; korrekter Tail unabhängig von Auswahl, Zeilenanzahl, Auswahl und Page nach Grenzübergang; bei 1 keine Entfernung; bei 64 kein ADD und anschließend korrekt 63; REST sowie Recall unverändert | PASS |
| ROW Hitbox | Tatsächlicher VSTGUI-Mausklick links neben VOL öffnet ROW statt Select; alle drei Menüaktionen über echte Mauskoordinaten | PASS |
| Pattern-übergreifendes Clipboard | COPY Row aus A mit allen 32 Zellen, PASTE in andere Zeile von B; andere Patterns/Zeilen bleiben bytegenau, UNDO stellt Ziel wieder her | PASS |
| CLEAR ROW | Löschung aller 32 Werte einschließlich versteckter Steps bei 1-Bar; Vergleich Controller und serialisierter Processor-Zustand, alle WAV-/Kanal-/Gain-/Pan-/Pitch-/Choke-Daten unverändert, UNDO funktioniert | PASS |
| FILL | Alle acht bestehenden Rhythmusaktionen mit separater referenzierter 16-Step-Golden-Matrix, keine COPY/PASTE/CLEAR mehr im Menü; alte Trefferflächen sind inert | PASS |
| Grenzfälle Menüs | Gutter-Negativtests für FILL, ROW, ZOOM bei 100/150; alle acht FILLs, drei ROWs und 16 Output-Ziele im 300px-Host-Refusal-Fenster tatsächlich klickbar | PASS |
| Native GUI | Echte VSTGUI-Offscreen-Renderings `v1-row-menu-100.bmp`, `v1-row-menu-130.bmp`, `song-12a-varied-chain-100.bmp`, `song-12a-varied-chain-130.bmp`; tatsächlicher Zustand `zoomMatches(1.0/1.3)`; die vier Bilder nach Herunterladen geöffnet und visuell geprüft: keine offensichtlichen Kollisionen/abgeschnittenen ROW-/SONG-Beschriftungen | PASS (Offscreen) |
| Windows gesamt | 23/23 CTests, Steinberg Validator 47/47, paketierte VST3-Audio-/Host-Smokes | PASS |
| 125A Plugin Tester v0.3.2 | 58 PASS / 0 WARNING / 0 FAIL, Exitcode 0, JSON PASS; geprüfte DLL stammt exakt aus dem nachgelagerten gleichen Windows-Run | PASS |
| Studio One/ASIO | Tatsächlicher Anwenderhost und subjektive endgültige GUI-Abnahme | NOT RUN, offen |

### Nachvollziehbare Builds (auch Fehlschläge)

- Commit `d73086bf0487ce6e463e0f2ac0c54382d3c98488`: [Actions #40424243064](https://github.com/challanger2000/125A-Drumator/actions/runs/40424243064) **FAIL in Compilation**, `UiCompactLayout.h` referenzierte `rowKnobCenters` vor der Deklaration. Keine CTests/Validator-Ergebnisse für diesen Stand.
- Commit `5f4a53fd1a0a9e0313e4f50d16481af354707b58`: [Actions #40424620327](https://github.com/challanger2000/125A-Drumator/actions/runs/40424620327) **FAIL CTest**; neuer Code kompiliert, aber drei ältere Testorakel waren noch an voriger FILL-CLEAR-/SONG-Leerseiten-Spezifikation ausgerichtet. Ergebnis 21/23 CTests PASS, 2 FAIL; unabhängiger Tester übersprungen. Nur die Tests wurden aktualisiert.
- **Exakt getesteter Commit `a14d9e67470ad0af94a8250baca13fa3a15fb70e`: [Actions #40425541707](https://github.com/challanger2000/125A-Drumator/actions/runs/40425541707) — SUCCESS für beide Jobs.** Windows 23/23 CTests, Validator 47/47, paketierter Host PASS und unabhängiger Plugin Tester v0.3.2 58/0/0.

**Provenienz und Artifacts:**

- VST3 Windows QA ZIP Artifact **12699159401**, Archiv-SHA256 `8f3026976a87173a01effd404e25de2b7027d49084f11bbe2eb1a196d4703760`.
- Exakte VST3-DLL `125A Drumator.vst3/Contents/x86_64-win/125A Drumator.vst3`, SHA256 **`0af3046c826bd03bf02eccca5cec35f827e8ed610eb5f6acdf317b7628189b7b`**; lokal direkt gegen den QA-ZIP-Inhalt berechnet und mit dem unabhängigen Tester-Joblog verglichen.
- Native Rendering Artifact **12698389889**, Archiv-SHA256 `7afa05493b6e173aba1fa5358254fbfad0a8d0dbe9c0f630a4ac411d7967f69e`.
- Unabhängiges Plugin-Tester-Artifact **12699523020**, Archiv-SHA256 `bb204b51706ccd1f44ca1daa5d7cad2eec4cc3faec7ce906226688347446d4d8`.

**Einschränkung:** Native Offscreen-Renderings bei 100/130 % ersetzen keine tatsächliche Studio-One-8.1.2-Abnahme. Realtime-ASIO-Aussetzerfreiheit ist dadurch nicht bewiesen. **Keine eigenmächtige V1-FINAL-Freigabe.**

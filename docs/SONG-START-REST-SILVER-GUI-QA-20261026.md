# 125A Drumator — SONG START, REST und Silber-Logo

**26.10.2026 · Windows-Entwicklungskandidat V0.1.0 · keine V1-FINAL-Freigabe**

## Baseline und Commit-Provenienz

- Repository: `challanger2000/125A-Drumator`; Branch: `gui/ppg-blue-background-20261024`.
- Vom Auftrag bestätigter, unveränderter Ausgangs-HEAD: `9585891aa9435a03c75189696431f32988f6a82a`.
- Implementierung, DM17-State und Windows-Regressionen: Commits `f22b56a` bis `368d7a3b88da0956563f7a850755b314b2f785fa`.
- QA-/Screenshot-Nacharbeit: `d651631`, `c5c12a3` (absichtlicher FAIL, verworfen als Screenshot-Evidenz), `54c491f`, `989f454e9ed00ef175b1a2c1c25927d56c0aa134` (exakter final paketierter Windows-VST3-Build).
- Exakter unabhängiger Tester-V0.3.2-Artefakt-Pin, ohne Core-Neubau: `433c9d2e2538a476e265cf7c7a97ced31a5788f9`.
- Keine Änderungen an `main`, keine V1-FINAL-Markierung, `0.1.0` unverändert.

### Geänderte Dateien gegenüber der vorgegebenen Baseline

`.github/workflows/core-ci.yml`, `.github/workflows/125a-plugin-tester-qa.yml`, `CMakeLists.txt`,
`src/core/SongChain.h`, `src/core/KitState.h`,
`src/vst3/DrumatorProcessor.h`, `src/vst3/DrumatorProcessor.cpp`,
`src/vst3/DrumatorController.h`, `src/vst3/DrumatorController.cpp`,
`src/vst3/DrumatorView.cpp`,
`tests/SongChainTests.cpp`, `tests/KitStateTests.cpp`,
`tests/integration/WindowsIntegration.cpp`, `tests/EditorGeometrySmoke.cpp`,
sowie dieses Dokument. Die früheren vier Funktionskorrekturen aus `docs/V1-FOUR-CORRECTIONS-QA-20261026.md` wurden nicht erneut implementiert.

## Fachliche Implementierung

**SONG START:** Ein einstellbarer Starttakt `BAR 1` (Default, zulässig 1..9999), `-`, `+` und Reset `BAR 1` in der vorhandenen kompakten SONG-Ansicht. Der Controller sendet den Starttakt als Teil der bestehenden `Drumator.SongConfig`-Transaktion. Der Processor übernimmt Kette, Enable und Starttakt kohärent in seine lockfreie `SongMailbox`. Der SONG-Scheduler leitet die Kettenposition **direkt von der musikalisch absoluten PPQ-Position** ab, relativ zu `(StartBar - 1)*16` Sechzehntelschritten, **nicht** relativ zu `phaseOriginStep_` beziehungsweise dem DAW-Transportstart. Vor dem Start liefert das SONG-Mapping eine stille REST-Position; die bisherige PATTERN-Transportphasenlogik bleibt unverändert. Bei DAW-Positionssprung oder aktivem Cycle wird die Kettenposition wieder aus der tatsächlichen Host-PPQ-Position berechnet. Der Starttakt ist Bestandteil des gespeicherten Kit-/Plugin-States.

**REST:** Eigenständiger SONG-Entry-Sentinel `SongChain::kRest=8` (keine Erweiterung der acht musikalischen Patterns A–H), wählbar in der SONG-Ansicht. Dieselben zulässigen Dauern 8, 16 oder 32 Steps (½, 1, 2 Takte) und Ketten-/Wrap-Mathematik wie reguläre Einträge; das audioseitige Pattern-Read gibt für REST **keine neuen Step-Trigger** aus. Bereits aktive Sample-Voices werden dadurch nicht beendet. Alle acht echten Patterns bleiben musikalisch nutzbar.

**State-Migration:** Neuer `DM17`-Format-Magic mit einem hinter den bisherigen DM16-Feldern gespeicherten `uint16`-Starttakt vor der Prüfsumme. Vorgängerversionen DM16 und ältere unterstützte Kit-Formate werden weiter geladen; bei ihnen wird `BAR 1` benutzt. Der neue REST-Sentinel wird nur im neuen State geschrieben. Alte Projekt-Routing-/Pattern-/Sample-Daten werden unverändert rekonstruiert. Format bleibt transaktional validiert (Fehler dürfen einen bestehenden validen Zustand nicht überschreiben).

**GUI-Ressourcen:** Das bereits vorhandene unveränderte Original `resource/Metallisches DRUMATOR-Logo.png` und `resource/125A_Drumator_Faceplate.png` werden ausdrücklich in `Contents/Resources` kopiert, mittels exakter Installationsbaum-Liste und obligatorischer Mindestgröße geprüft. Die gültigen PNG-Header und Abmessungen werden am **ausgelieferten** VST3-Bundle geprüft. Kein neu generiertes Markenbild, keine Änderung an Farben, Bedienelementen, Grundlayout oder Zoomstufen.

## Nachvollziehbare Windows-QA

| Gate | Evidenz | Resultat |
|---|---|---|
| Voller Windows-MSVC-Build | [Actions #40394744591](https://github.com/challanger2000/125A-Drumator/actions/runs/40394744591), Quellcommit `989f454e` | **PASS** |
| Alle CTests | derselbe Lauf | **23/23 PASS, 0 FAIL** |
| Neu SONG START und REST | echte native Processor-/Controller-/GUI-Integration plus `SongChainTests` und `KitStateTests` | **PASS** absolute BAR2-/BAR4-Positionen, Pre-roll-Grenze, REST/Tail-/Folgepattern-PCM, BAR+/-/Reset-UI, Cycle und Tempo, DM17-Recall, DM16-Migration |
| Steinberg SDK 3.8.1 Validator | derselbe Lauf | **47 PASS / 0 FAIL** |
| Paketierter VST3-Editor | derselbe Lauf | **PASS** echte DLL laden, HWND attach/detach, GUI-Size/Zoom/Host-Refusal; kein Ersatztext als „Screenshot“ fingiert |
| Paketierter VST3 Audio-Host | derselbe Lauf | **PASS**, inklusive Audio/MIDI/16 Stereo-Busse, Sample-Recall, Transport/Loop, T02/T04 |
| Silber-Logo im Bundle | derselbe Lauf | **PASS** PNGs mit exakten Namen: Produktlogo **2172×724**, Faceplate **1671×941**, beide als tatsächliche native VSTGUI-Bitmaps dekodiert; offscreen Bildpixel der silbernen Header-Beschriftung geprüft |
| Unabhängiger **125A Plugin Tester V0.3.2** | [Actions #40395721734](https://github.com/challanger2000/125A-Drumator/actions/runs/40395721734), `989f454e`-DLL-SHA gepinnt | **58 PASS / 0 WARNING / 0 FAIL**, JSON PASS, Exit 0 |

**Downloads/Evidenz**

- [Installierbares VST3, Artefakt 12687160591](https://github.com/challanger2000/125A-Drumator/actions/runs/40394744591/artifacts/12687160591): ZIP SHA256 `c71da22a75ae9550eb21f783bececf447ae7fa99e9b67e165388c27e9739970d`, DLL SHA256 `9a8b11bce220109e663c0b75c1967ad9f7c6eced1f82c3a1ae0939755a99ed91`, acht geprüfte ZIP-Mitglieder, CRC-Integrität PASS.
- [Native GUI-Renders, Artefakt 12686575828](https://github.com/challanger2000/125A-Drumator/actions/runs/40394744591/artifacts/12686575828): enthält **`packaged-vst3-resources-silver-100.bmp`**, 1280×734, und weitere originäre VSTGUI-Renders. ZIP SHA256 `0dbd8714f7a784f74c700ad2dde021f8ce4deb686b46e67f291cfe9fa054c686`.
- [Unabhängiger Tester-Report, Artefakt 12685833609](https://github.com/challanger2000/125A-Drumator/actions/runs/40395721734/artifacts/12685833609).

### Wichtige Screenshot-Evidenzgrenze

Ein direkter GDI/`BitBlt`-Screenshot des real angehängten VST3-Editor-HWND in einem **headless GitHub-Windows-Runner** erfasste nur das leere Host-Fenster beziehungsweise keine verlässlich auslesbaren Direct2D-Pixel. Ein solcher Screenshot wurde **nicht** als bestanden ausgegeben. Stattdessen wurde zusätzlich zum **real aus dem installierbaren Bundle geladenen und angehängten VST3-Editor** die native offscreen VSTGUI-Renderfunktion mit **den tatsächlich fertig gepackten PNGs aus `Contents/Resources`** ausgeführt, samt Bitmap-Ladedimensionsprüfung und messbarer Silberpixel- und Dunkelheader-Kontrolle. `packaged-vst3-resources-silver-100.bmp` ist **echtes natives VSTGUI-Rendering der finalen Bundle-Ressourcen**, ausdrücklich **nicht** ein direkter Screenshot des live gepainteten VST3-DLL-HWND. Die weitere direkte Sichtprüfung des VST3 auf dem Anwenderrechner in Studio One bleibt offen.

## Restliche Grenzen / Freigabe

- **NOT RUN** mit der neuen DLL: direkte Anwenderprüfung in **Studio One 8.1.2 Pro / Windows 11 / ASIO**, insbesondere BAR1/BAR2 und direkte Wiedergabe ab BAR4, Stop/Play, Rücksprünge, Cycle, Temposprung, REST mit Sample-Tails, SONG-State-Recall und das silberne Logo im wirklich geöffneten Studio-One-VST3.
- Die Umsetzung nimmt für das Umrechnen von Takt nach PPQ **4/4** an (`16 Steps/Takt`). Wechselnde oder andere Taktarten sind hierfür **nicht** validiert und würden eine gesonderte Tempo-/Time-Signature-Anbindung erfordern. Kein stiller Anspruch auf beliebige Taktarten.
- Die bisherige real getrennte Processor-/Controller-Mehrprozess-IPC-Prüfung bleibt unabhängig von dieser Änderung offen.
- **Keine V1-FINAL-Freigabe** allein aufgrund grüner Actions oder des internen Tester-Labels `RELEASE: PASS`; endgültige Besitzer-/DAW-Abnahme erforderlich.

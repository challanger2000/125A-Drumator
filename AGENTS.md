# 125A Drumator — development instructions

## Aktueller unabhängiger Funktionsaudit — 21.10.2026 (maßgeblich)

**Windows-geprüfter Programm-/Teststand `826c6d8b0a3c0d038678061d7ad97092e37927e3`:** [Windows-QA #39676937642](https://github.com/challanger2000/125A-Drumator/actions/runs/39676937642) **SUCCESS**, **22/22 CTests**, darin **43 native Integrationsfälle ohne Fehler**, **47/47 Steinberg Validator**, paketierte Audio-/Editor-Hosttests erfolgreich. Sechs konkrete Fehler FA01–FA06 wurden korrigiert; T28/T29 und das Random-Velocity-Testorakel wurden verstärkt. [Vollständiger unabhängiger Bericht mit Quellen, Matrix und Beweisgrenzen](docs/audits/CODEX-FINAL-FUNCTIONAL-RECHECK-2026-10-21.md).

**Keine V1-/endgültige GUI-Freigabe.** Studio One wurde nicht ausgeführt. Offline-64-Voice-/16x-Messung: p95 4.7974 ms, p99 7.403 ms, Maximum 8.892 ms bei 5.33333 ms Blockfrist. Keine garantierte Echtzeitfähigkeit; kontrollierte Host-Abnahme und dokumentierte Nacharbeiten bleiben erforderlich. Vollständige Logs/Artefaktdownloads sind hier durch den Cloud-Proxy blockiert; tatsächliche Checks-Annotations und deren Grenzen stehen im Bericht. Die älteren „aktuellen“ Abschnitte unten sind historische Stände.

## Aktueller GUI-Funktionsstand — 20.10.2026 (maßgeblich)

**Geprüfter Quellstand `eaa035bb43cc9fa20682f711b8631a75da0ece4d`:** [Windows-QA #39418839905](https://github.com/challanger2000/125A-Drumator/actions/runs/39418839905) **SUCCESS**; **21/21 CTests**, **47/47 Steinberg Validator**, reale gepackte VST3-Audio-/Editor-Host-Smokes PASS. Das native VSTGUI-Rendering ist als eigenes Artifakt `125A-Drumator-VSTGUI-Native-Renderings` (ID `12262412156`, 9 Renderings) gespeichert und wurde angesehen; das neue VST3-Testpaket ist `125A-Drumator-M11-Full-Kit-Stress-QA` (ID `12262387312`). Kein Ausführungstest in Studio One, **keine endgültige V1-Veröffentlichung und noch kein endgültiger Bitmap-Skin**.

Die technische VSTGUI-Vorlage besitzt gemeinsame Geometrie für 16 Instrumente, 32 Steps und optional 32 inline Velocity-Balken je angezeigtem Instrument; bei Velocity 8 Instrumente/Seite mit geprüfter Navigation und sichtbarer Auswahl. Fünf vergrößerte Präzisionsregler pro ausgewähltem Instrument, genaue Sample-Text-/Pfeil-/Mixer-Hitflächen, lesbare Werte mit Einheiten, 100/120/130/150%-Zoom inkl. DZM1-Kompatibilität und DZM2-Recall. Beschriftungen und Layout wurden in echten Windows-Offscreen-Renderings auf Kollisionen geprüft; ein Kontrastproblem bei deaktivierter 16-Instrumente-Schaltfläche wurde behoben. [Geometrie/Handoff](docs/VSTGUI-HANDOFF.md) und [QA-/UX-Spezifikation](docs/VSTGUI-FOUNDATION-2026-10-20.md) sind die maßgeblichen technischen Bezugspunkte.

**Hinweis:** Die bisherigen Audit-/Buildstatus unterhalb dieses Abschnitts sind **historisch**, nicht aktueller Entwicklungsstand. Studio-One-Praxistest (Reales Sample-D&D, Song, Velocity, Zoom, Projekt-Recall) und abschließende grafische Gestaltung verbleiben als separate Freigabeschritte. Weiterentwicklung nur vom verifizierten aktuellen `develop/v1.0.0`-HEAD; keine unnötigen Actions-Läufe.


## Aktuell – Integrationstest-Ausbau, 20.10.2026

Test-/Buildstand `a0700173a16b16858e402a99a57987af41b7f071`: [Windows #39398941950](https://github.com/challanger2000/125A-Drumator/actions/runs/39398941950) **BUILD FAIL**; CTest, Validator und paketierte Hosttests **SKIPPED**. T01–T04/O01/Z01–Z03-Testverbesserungen sind implementiert, aber nicht erfolgreich Windows-validiert. Eigene `/W4 /WX` bleiben aktiv. Die vorherigen Originalmeldungen LNK2019/LNK1120 belegen die fehlende `setupVSTGUIBundleSupport`-Definition im Test-EXE. Ausschließlich dessen CMake-Anbindung wurde um die originale SDK-Datei `public.sdk/source/vst/vstgui_win32_bundle_support.cpp` ergänzt; Ursache und Plattformbedingungen wurden im gepinnten SDK geprüft. Keine Dummyfunktion, kein Abschalten von Tests. Auch dieser neue Lauf scheitert im Buildschritt; seine konkreten neuen Fehler sind wegen CONNECT 403 beim offiziellen Joblogdownload noch nicht verfügbar. Produktcode/Ressourcen gegenüber `f6f1933` unverändert; letzter erfolgreicher Produktnachweis bleibt `62f10a4`. [Vollständiger Bericht und Testgrenzen](docs/INTEGRATION-QA-T01-T04-O01-Z01-Z03-2026-10-20.md). Keine Studio-One-Ausführung, keine V1-Freigabe. Keine weiteren spekulativen Actions-Läufe; zuerst konkrete Fehlermeldungen aus Job `118548526770` diagnostizieren.

## Aktuell – Z01–Z03-Reparatur am 20.10.2026

**Windows-geprüfter Quellcode `62f10a4edade31217c90a32eba3d4e39eefc54ee`**: [Actions #39347442164](https://github.com/challanger2000/125A-Drumator/actions/runs/39347442164) **SUCCESS**, 17/17 CTests, Steinberg Validator 47/47, paketierter Audio-/Editor-Smoke PASS. Codex-Z01 (transaktionaler Recall), Z02 (einfacher geprüfter Zoom ohne partiellen Rollback) und Z03 (nativer Textedit-Fokusabschluss) sind im Code bearbeitet. [Details und ehrliche Testgrenzen](docs/CODEX-Z01-Z03-REPAIR-2026-10-20.md). Mehrfach-Editor-Zoom-Recall mit mehreren nötigen Fensteränderungen wird bewusst und vollständig **vor** Änderungen abgelehnt. Studio One, O01/T01–T04, finale Grafik und Owner-Freigabe offen. **Keine V1-Freigabe.**

Ältere Standmeldungen unten sind historische Prüfstände.

## RC01–RC04 — aktueller Stand 20.10.2026

**Geprüfter Programmcommit `d984188457882a4463b9263593d88f7bf1fffb71`**, [Windows #39341195970](https://github.com/challanger2000/125A-Drumator/actions/runs/39341195970) **SUCCESS**: 16/16 CTests, Steinberg Validator 47/47, paketierter Editor-/Audio-Host-Smoke PASS. Codex-Recheck RC01–RC04 im Code bearbeitet; [exakter Reparatur-/QA-Nachweis](docs/CODEX-RC01-RC04-REPAIR-2026-10-20.md). SDK-Rechtsklick-„UI Zoom“ bewusst deaktiviert, 100/150-Headerbuttons bleiben. **Studio One/nativer Attach/Recall, O01 und finale GUI weiterhin offen. KEINE V1-FREIGABE.**

Die früheren Abschnitte sind historische Prüfstände.

## Aktueller Entwicklungsstand nach Codex A01–A09 (19.10.2026)

Quellcode `dbd9106f7af48b23ad4874a1a4c74395baa14173`, [GitHub Actions #39335925418](https://github.com/challanger2000/125A-Drumator/actions/runs/39335925418) **SUCCESS**: 15/15 CTests, Steinberg Validator 47/47 und paketierte VST3-Audio-/Editor-Smokes. Alle neun identifizierten Codex-Befunde sind im Quellcode bearbeitet; Details und noch offene **native Studio-One-Gates** in [docs/CODEX-FINDINGS-REPAIR-2026-10-19.md](docs/CODEX-FINDINGS-REPAIR-2026-10-19.md). Keine finale 125A-Drumator-GUI, keine Freigabe als V1; die bisherigen Statusabschnitte darunter sind historische Fixpunkte.

## Stand nach Fehlerkorrektur (19.10.2026)

**Aktueller geprüfter Produktionscode: `b6c40d0c918f92c0d63e343dc72caa0793528fb5`, [Windows Actions #39324490951](https://github.com/challanger2000/125A-Drumator/actions/runs/39324490951) SUCCESS.** 13/13 CTests einschließlich echter Song-Chain-Release-Checks, absichtlichem Negativtest und Header-Hitbox-Test; Steinberg Validator 47/47 PASS; paketierter GUI-/Audio-Host-Smoke PASS. P1-01, P2-03 (UIDESC-Obergrenze), konkrete P2-06-Headerlücken, P3-08 und die zwei eigenen Compilerwarnungen wurden im Quellcode behoben.

**Noch OFFEN:** finale Bitmap-/VSTGUI-Skin-Einbindung gehört dem anderen Chat; angeschlossene Studio-One-GUI/Zoom/Drag-Drop/Fenster-Resize, Langzeit-Realtime, möglicher Host-/Restore-Konkurrenzfall und abschließende Nutzerfreigabe sind **nicht** durch Actions bewiesen. Keine FINAL V1.

Die nachfolgenden älteren Meilenstein- und Auditabschnitte bleiben als **historische Befunde**, nicht als aktueller Stand.

## Status
**M10 VERIFIED Windows host state recall**: source commit `eb11d3f4fdacf557aa48940395616fbeda79caaa`, [Actions #38960037594](https://github.com/challanger2000/125A-Drumator/actions/runs/38960037594) SUCCESS. CTests **8/8 PASS**; official Steinberg Validator **47/47 PASS**; packaged editor 1100x300 PASS; packaged audio-host smoke full DMR4 save → **new VST3 processor/controller instance** → state restore **before** setupProcessing → same FLOAT32 WAV at measured **−0.5 per channel** PASS. PCM16/24/FLOAT32 amplitude, corrupt input, silent unloaded row also PASS. Artifact ID `12072750795` (125A-Drumator-M10-Embedded-Kit-Recall-QA), ZIP `125A_Drumator_M10_Windows_x64.zip` independently inspected; three correct bundle files, no corrupt ZIP members, x64 PE machine 0x8664, strict JSON resources, DLL SHA256 `87d7254382901ccf2d8053e7440bbfdb801618cfd71bda3ca8faf9dbde5b4dd8`. **Studio Pro 8.1.2 project reopen/GUI/drag-drop, all 16 rows loaded concurrently, kit limit handling under 64 MiB, and long-term realtime still need independent QA. Not FINAL.**

M10 embedded kit recall source candidate: DMR4 stores WAV bytes, filenames, visible rows and all patterns in the VST3 project state; host/Windows QA pending. **No M10 PASS until real packaged host state/reload test succeeds.**

**M09 verified Windows QA:** source `c7e8a08eac6ab8fd2392d23151d44d18c2b81586`, [Actions #38956743559](https://github.com/challanger2000/125A-Drumator/actions/runs/38956743559) SUCCESS. Seven CTests **7/7 PASS**, official Steinberg Validator **47/47 PASS**, packaged editor geometry **1100x300 PASS**, actual packaged VST3 host simulation PASS for silence before load, PCM16 +0.50/+0.50, PCM24 +0.25/+0.25 and FLOAT32 -0.50/-0.50 L/R amplitude; corrupt WAV rejection preserves prior sound; unloaded row 2 silent. Artifact `125A-Drumator-M09-4-Rows-Host-Audio-QA` id `12070159256`, downloaded as ZIP, verified exact `125A Drumator.vst3` tree (3 nonempty files), valid JSON resources, PE x64 signature and ZIP integrity. NO claim of real Studio Pro browser drag/drop, GUI plus/minus interaction, recalled sample assets, prolonged paced realtime load or V1 final.

M09 under development: 4 visible rows by default, + expand / guarded minus, DMR3 row-count project state, packaged VST3 host audio smoke. **No Windows M09 PASS claimed until a matching run succeeds.** Prior verified M08 results remain below.

M08 **verified Windows QA** at source commit `73b0430c673ea86d7133ca6ebd75a56c7515c3bd`, [Actions #38952896083](https://github.com/challanger2000/125A-Drumator/actions/runs/38952896083) **SUCCESS**: MSVC Windows x64 compiled, six CTests **6/6 PASS**, official Steinberg Validator **47/47 PASS**, actual packaged editor smoke **1100×600 PASS**. Delivered artifact `125A-Drumator-M08-Windows-x64-WAV-Drop-Audition` (ID 12069382610): ZIP was downloaded, its three expected paths inspected, PE x64 confirmed, `moduleinfo.json` and `Drumator.uidesc` parsed as valid JSON, ZIP integrity clean. In Studio Pro user tests for PCM24/float WAV loading and OS/browser drag/drop **remain unverified**.

**M08 source candidate (2026-10-16):** removes the misleading internal kick (empty sample rows are silent), supports mono/stereo WAV PCM 8/16/24/32-bit, IEEE float32 and common WAVE_FORMAT_EXTENSIBLE variants, and keeps recorded level at center pan (unity per channel, no normalization). GUI distinguishes load failure causes, and VSTGUI file-path drag/drop targets individual rows. **CI, real Studio Pro acceptance and dragged-file confirmation remain unverified until tested.** No release or FINAL label.

**GUI resource fix / verified source:** commit `7097bc5a9a6fa6178ffde7fa706aa9be94de280c`, Windows Actions [#38944959865](https://github.com/challanger2000/125A-Drumator/actions/runs/38944959865), 6/6 CTests PASS, Steinberg Validator 47/47 PASS, and new **editor geometry smoke 1100 × 600 PASS** on the packaged VST3. Downloaded artifact `125A-Drumator-M07-Windows-x64-GUI-PROTOTYPE` (ID 12064249354) has been independently ZIP-verified. Studio Pro GUI rendering and click/WAV playback are NOT yet confirmed.

**Do not substitute validator PASS for UI QA.** When VSTGUI is in a separately compiled static target and Windows bundle mode is disabled, compile bundle resource-base support into that target as well and run the real editor geometry smoke test on the final installed structure.

Windows CI VERIFIED at source commit d1055c1b59aba15b6a7c9d594e5f882b13fd630d, Actions #38923496070: Windows x64 MSVC build PASS, CTest 6/6 PASS, official Steinberg Validator 47/47 PASS on assembled standard Windows VST3 bundle, moduleinfotool PASS; uploaded engineering-only artifact Drumator-M07-GUI-PROTOTYPE-UNTESTED (ID 12054908672). No actual Fender Studio Pro editor/interactive WAV import test, 125A Plugin Tester, sample file project recall or FINAL release.

Milestone 07 GUI source candidate: 16x32 grid, WAV chooser and versioned atomic step state **not yet Windows-built/DAW verified**. Prior Milestone 06 test results remain separately documented below. Sample file contents and references are not yet restored from host state.

**Verified M06 technical build:** source `9f223c0f6c4d8e65a8ce368afd08a0387643af57`, [GitHub Actions #38919501529](https://github.com/challanger2000/125A-Drumator/actions/runs/38919501529) **SUCCESS**: Windows MSVC x64 VST3 build PASS; five CTests **5/5 PASS**; moduleinfotool PASS; official Steinberg Validator **47/47 PASS** on assembled bundle. Engineering-only artifact `Drumator-M06-INTERIM-VST3-UNTESTED` (ID `12053822596`). No DAW WAV-import/GUI/audio test or final release.

DEVELOPMENT / Verified at source commit `8b2b41eb1feb8dca09a325e7d649841a34ce58af`, [Windows Actions #38917602904](https://github.com/challanger2000/125A-Drumator/actions/runs/38917602904): MSVC x64 Release build PASS; CTest **4/4 PASS** (`drumator_core_tests`, `drumator_voice_tests`, `drumator_wav_tests`, `drumator_exchange_tests`); official moduleinfotool creation and validation PASS; assembled Windows VST3 bundle official Steinberg Validator **47/47 PASS**. Engineering-only artifact `Drumator-M03-VST3-UNTESTED`, id `12052789021` (still temporary synthesized kick and no editor). The PCM16 WAV decoder and sample exchange are host-independent tested modules. A **new processor message receiver and MIDI audition adapter are currently under CI validation**, and there is still no user file picker/drag-drop GUI or end-to-end DAW sample-import evidence. No 125A Plugin Tester, real DAW audio/UI QA, owner approval, or FINAL release exists.

## Mandatory first read
Before *any* implementation, audit, DSP, GUI, build or release work:
1. Open `challanger2000/125A-Engineering/START-HERE.md`.
2. Follow its complete reading order and apply all relevant Engineering, Standards, QA, Research and Roadmap documents.
3. Read the current `challanger2000/125A-Branding/README.md` and authoritative master assets before any branded GUI/logo work. Use approved 125A-Knob-Designer assets where applicable.

## Project policy
- Windows 11 x64, VST3 instrument only. No macOS or Linux builds or releases.
- Development branch: `develop/v1.0.0`. Protect stable `main`; no casual implementation directly on `main`.
- Official Steinberg VST3 SDK **3.8.1**, tag `v3.8.1_build_84`, commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, including its **exact tagged submodules**. See `docs/SDK-BASELINE.md`.
- Never use a floating SDK `master`, a guessed SDK API, or separately update VSTGUI without verification.
- Prefer established audio/DSP theory and exact Steinberg VST3 contracts, reproducible measurement and realistic QA; no speculative workarounds or arbitrary thresholds.
- No file I/O, dynamic allocation, blocking locks, or unbounded work inside the audio callback.
- Static product design precedes the **final** branded GUI. Early functional prototype may use plain VSTGUI.
- Keep the user-facing workflow deliberately simple. The current concept is documented in `docs/V1-SCOPE.md`.
- Before any write: verify actual repository, branch HEAD, version and affected files; avoid unnecessary GitHub Actions runs.
- Be exact about status: concept is not implemented; compilation is not QA; QA is not owner-approved release.

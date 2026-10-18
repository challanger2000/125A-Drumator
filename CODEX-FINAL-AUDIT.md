# 125A Drumator – erneuter unabhängiger technischer Abschluss-Audit

**Prüfdatum:** 18.10.2026, UTC.

**Repository / Branch:** `challanger2000/125A-Drumator` / `develop/v1.0.0`.

**Tatsächlich geprüfter Entwicklungscommit:** **`42f468aac2e9d84d6e4f563107fd3871eb3f8d1f`**.

**Pluginversion:** `0.1.0`, weiterhin keine finale V1. Der Dokumentationscommit dieses Berichts ist vom geprüften Implementierungscommit zu unterscheiden.

**Vergleich:** [TECHNISCHER-AUDIT.md][original] mit F01–F17, [erster Abschlussbericht][first-final] mit N01–N05 und [unmittelbar vorheriger Abschlussbericht][previous] mit N06–N09 und Prüfstand `32f68ca7b2c3dc395c5a68efeb991760c373d4ac`. Alle acht danach erfolgten Reparatur-/Testcommits wurden am tatsächlichen Diff und den vollständigen betroffenen Aufrufketten kontrolliert.

## 1. Entscheidung

**Ist die technische Grundlage jetzt bereit für die endgültige Vintage-GUI? NEIN.**

Es bestehen nachvollziehbare Fortschritte:

- **N06:** Die ursprünglichen Pattern-Automationsgrenzen sind wiederhergestellt; ein eigener Parameter gleicht Hosttext und Plainwert daran an.
- **N08:** Der Transportzustand wird beim Kitrevisionswechsel nicht mehr verworfen. Ein neuer Pakettest prüft tatsächlich „Restore → genau ein laufender Block mit Sound → unmittelbar Stop“.
- **N09:** Die Voicekapazität wird vor dem Choke geprüft. Ein neuer Audiocoretest deckt den vorher fehlerhaften Kombinationsfall ab.
- **N02:** Der Zoomcheck vergleicht jetzt die tatsächliche Frame-Transformation. Der im letzten Bericht beschriebene Cachegegenfall ist im Quellpfad beseitigt; eine native Regression fehlt noch.
- **Synchronisation:** Die neue Ein-Atomic-Zulassung löst den früheren Zwei-Atomic-Ordnungsfehler der Eintrittssperre. Ein Test benutzt jetzt die tatsächliche Produktionsklasse mit zwei Threads.

**Nicht abgeschlossen:**

1. **N07 / P1 / FAIL:** `PatternRestoreLatch::finish()` kann einen bereits erfolgreich übernommenen neueren Automationswert mit einem älteren Wert überschreiben. Dies ist ein konkretes Lost-Update-Interleaving; es tritt selbst bei sequentiell konsistenten Atomics auf und benötigt kein schwaches Speichermodell.
2. **N10 / P2 / FAIL:** Die vom Editor geerbte Host-HiDPI-API bestätigt vor Attach einen neuen ContentScale-Faktor, ohne `getSize()` anzupassen. Auch ihr Ablehnungspfad umgeht die neue Zoomtransaktion.
3. **TEILWEISE/OFFEN:** Echte angehängte GUI, Zeilen-/Resize-/Dirtypfade, vollständige Kit-Audiokonsistenz mit geeignetem Oracle, Echtzeitmessungen, SRC-Qualitätsvertrag und mehrere Projekt-/Importgrenzfälle sind weiterhin nicht ausreichend verifiziert.

Die Entscheidung folgt damit nicht allein aus fehlender Studio-One-Praxis: Es bleibt mindestens ein konkreter P1-Funktionsfehler. Ebenso werden korrekt geschlossene Fehler nicht erneut aufgrund bloßer Vorsicht als FAIL bezeichnet.

## 2. Grundlagen, Beweisgrenzen und aktueller Windows-Nachweis

### 2.1 Regeln und Umfang

`125A-Engineering/START-HERE.md` und das aktuelle `AGENTS.md` wurden zuerst gelesen. Engineering-HEAD ist unverändert `5f13a850c065167526519f0249fb7a38b5841a6a`. Die einschlägigen Guardrails und Standards wurden angewandt; AI-Assisted-Engineering, Release-QA und Realtime-QA erneut abgeglichen. Entscheidend bleiben kompatible Automation, genaue Zustands-/Ownershipverträge, geeignete Gegenfälle und echte native GUI-/Realtime-Evidenz. Nur dieser Bericht wird geschrieben; Regeln in anderen Repositories bleiben gemäß Benutzerauftrag unverändert.

Der Quellenbestand wurde aus exakt dem oben genannten Commit gelesen: **26 Produktionsdateien und zehn Testdateien**, UIDESC, CMake, Workflow und Projektdokumentation. Gegenüber dem vorigen Entwicklungsstand sind acht Produktionsdateien und drei Testdateien geändert oder neu; zwei neue Produktionsdateien sind `KitRestoreGate.h` und `PatternRestoreLatch.h`. Ein SHA-256-Inventar unterscheidet unveränderte und geänderte Dateien. Die unveränderten Scheduler-, SampleExchange-, WAV-/FLAC-, Kit- und GUI-Grundlagen wurden in den aktuellen Gesamtpfad einbezogen, nicht allein wegen ihres Alters als bestanden behandelt.

Quellenreferenz: VST3 SDK3.8.1 `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`, public.sdk `586dc5e6c8012c3e4b01c79389375cbe96bdb1da`, pluginterfaces `4f547e8e102b47de4a8b8aaf343c73b700786372`, VSTGUI `5db272256172557818b6158cf0bb2c4410bddb25`. dr_flac ist auf `dfe8377631000664666519fdb83da193fd8037f4` gepinnt. Die lokale SDK-Kopie dient der Quellenprüfung und wird nicht als eingerichtete Windows-Toolchain ausgegeben.

Die bereits anhand `cloud-environment-onboarding:setup` vorbereitete Cloud-Umgebung erlaubt Git-, Quellen- und analytische Prüfungen. Ein nutzbarer Windows-/Studio-One-Ausführungsplatz ist hier nicht vorhanden. Gemäß Projektregel wurden keine Linux-Builds als Ersatz durchgeführt und keine neue Umgebungskonfiguration benötigt.

### 2.2 Statusdefinitionen und tatsächlich ausgeführte Prüfungen

- **PASS:** Der genau bezeichnete Fehlerfall ist durch geeignete konkrete vorhandene Assertions, den zugeordneten erfolgreichen Windowslauf und den abgeglichenen Quellpfad gestützt. Ein enger PASS ist keine globale Plugin- oder GUI-Freigabe.
- **TEILWEISE:** Reparatur vorhanden, aber vollständiger Pfad oder relevanter Gegenfall nicht ausreichend nachgewiesen.
- **FAIL:** Konkreter Fehler durch tatsächliche Implementierung, SDK-Vertrag und nachvollziehbaren Gegenablauf belegt. Ein solcher Quellenbeweis wird nicht als hier ausgeführter DAW-/Binärtest bezeichnet.
- **OFFEN:** Zusätzlicher praktischer Test, Messung oder Verhaltensentscheidung erforderlich.

Hier ausgeführt wurden read-only Git-/Diff-/SDK-/Quellprüfungen und unabhängige Python-Analysen. Darunter: normale/alte Patternabbildungen, Zoomzustand des bisherigen Gegenfalls und alle20 quellreihenfolgeerhaltenden Interleavings eines dreiteiligen `finish` und eines dreiteiligen weiteren `submit`. Ein konkreter Ablauf verliert die neueste Auswahl; Details in Abschnitt5. GUIgrößen und weiterhin unveränderte SRC-Berechnungen wurden gegen die vorherigen Ergebnisse abgeglichen.

**Nicht hier ausgeführt:** C++-Tests, Windows-VST3, native Attach-/Mouse-/Drop-/HiDPI-Tests, Studio One, 125A Plugin Tester, eigener Validatorlauf, Allocation-Profiler oder zeitlich getakteter Audiotest. Keine vorhandene Testdatei wurde verändert. Kein Actions-Build wurde gestartet. Die Python-Interleavings sind ein unabhängiger analytischer Quellengegenbeweis und ausdrücklich keine Ausführung der C++-Produktklasse.

### 2.3 Vorhandener Lauf am tatsächlich geprüften Commit

[Actions-Run #39113340712][run], Workflowlauf #194, verweist auf exakt **`42f468aac2e9d84d6e4f563107fd3871eb3f8d1f`**, Status **Success**, Gesamtdauer **2min45s**. Der [Windows-Job][job] ist erfolgreich, Dauer2min42s. Die öffentliche Zusammenfassung zeigt das Artefakt `125A-Drumator-M11-Full-Kit-Stress-QA`, ca.1,4MB, Digest `sha256:9cdfeb070111666e0f146adec6c181728831f88cf4a254b036f01de76bad54f5`.

Der unveränderte Workflow baut acht CTest-Executables, zwei separate Paket-Smokes und die SDK-Werkzeuge. CTest sowie Validator- und Smokeaufrufe prüfen Fehlercodes. Die hier genau gelesenen verpflichtenden Assertions gelten deshalb als eng begrenzte vorhandene Windows-Evidenz. Die vollständigen Logs sind im verfügbaren Zugriff an eine Anmeldung gebunden. Es werden keine erneut ausgelesenen Einzeltestzahlen oder numerischen Performancewerte behauptet. Das Artefakt wurde hier nicht heruntergeladen oder ausgeführt.

### 2.4 Alle acht Folgecommits

| Commit | Geprüfter tatsächlicher Inhalt |
|---|---|
| `3c324579220050e2b05f3b6f5482f1e4c95df496` | `wasPlaying_` beim Revisionswechsel erhalten; freien Voiceslot vor Choke bestimmen. |
| `dc030bc0fc220a0ee168aa15b6935e5e320cadd3` | Erster Automationserhalt während Restore und gezielter N09-Voice-Audiotest; dieser Zwischenstand wurde später umgebaut. |
| `451aa77038c5ff6e8c5acc8a91e039b74560bd77` | Legacyquantisierung `round(7v)` plus eigener Parameter für Hosttext und Plainwert. |
| `6040a56700f402c279e6ea5c7cd9d5cb560e20b3` | `getTransform().m11` statt Frame-Zoomcache prüfen. |
| `f166d99ba1934d52195bdd1cfe4d6492e49c6a6e` | Echter Paket-Hosttest für Restore, einen laufenden Block und Stop. |
| `3c85e0168253c500a3e552972fd08857704f32a5` | Ein-Atomic-Kitgate und mehrteiliger PatternRestoreLatch. |
| `37e048bb7412bff3a745b26c089986b6c03eaf2d` | Lock-free-Static-Assert und Zwei-Thread-Test der tatsächlichen Gateklasse. |
| `42f468aac2e9d84d6e4f563107fd3871eb3f8d1f` | Tatsächlichen Latch testen, aber nur mit vollständig sequenziellen Methodenaufrufen. |

## 3. Status sämtlicher ursprünglicher Befunde F01–F17

Alle Zeilenlinks zeigen auf den oben angegebenen aktuellen Entwicklungscommit. C=Controller, CH=Controllerheader, P=Prozessor, PH=Prozessorheader, E=Editor, V=View, K=Voice/SRC, S=Exchange, B=KitCodec, F=FLAC, G=Patternquantisierung, D=Kitgate, I=Patternlatch, H=Paket-Hosttest, T=Voice-Tests, R=VisibleRows-Tests, Q=Editor-Smoke, W=Workflow, L=Zeilenpolicy.

| Befund | Status | Aktuelle technische Begründung und Grenze |
|---|---|---|
| **F01 – Childbreite/Clipping** | **TEILWEISE** | [V:96], [V:108], [C:180] und UIDESC verwenden1280. Alle32 Stepfüllungen passen rechnerisch hinein. Die Viewgeometrie blieb seit dem letzten Audit unverändert. [Q:45] prüft weiterhin nur ein ungeöffnetes Editorobjekt; tatsächliche Childgröße und Randklicks fehlen. |
| **F02 – Hitboxen** | **TEILWEISE** | Numerische Lücken [V:376] und A–H-Abstände [V:323] werden korrekt abgewiesen. Pattern-/Zoom-y-Bereiche stimmen nun getrennt überein. [R:70] prüft testlokale Lambdas, nicht die Produktview. Native negative Rand-/Lückenklicktests fehlen. |
| **F03 – Host→GUI/Relayout** | **TEILWEISE** | Registrierung/Abmeldung, Parameterinvalidierung [C:39], [C:48] und State-Relayout [C:136] sind vorhanden. [H:817] erzeugt weiterhin keine attachte Custom-View und beobachtet kein tatsächliches Repaint. State-Recall bei Resize-Ablehnung bleibt ohne bedienbaren Fallback. |
| **F04 – Parameter-Flush** | **TEILWEISE** | Normaler Nullframe-/Nulloutputflush [P:367] wird durch tatsächliche Paketstate-Assertions [H:851], [H:882] abgesichert. Während Restore wird nun ebenfalls gelesen [P:339]; die anschließende Latch-Arbitrierung verliert aber im N07-Gegenfall den neuesten Wert. |
| **F05 – Host Dirty** | **TEILWEISE** | Erfolgreiche Editnachrichten rufen über [C:54] `componentHandler2->setDirty(true)` auf. Der Helpertest [R:51] registriert keinen echten Hosthandler und führt keinen Controlleredit aus. Song-Modified/Autosave, abgelehnte Edits und Zoom-only-Persistenz fehlen als echte Hosttests. |
| **F06 – Legacy-Controllerdefaults** | **TEILWEISE** | DMR1 setzt Factorygrid, DMR1–3 setzen moderne Felder zurück [C:151], [C:160]; Prozessorpfad [P:582] passt dazu. Codec-Migrationstests DMR4–9 ersetzen keinen Controller-/GUItest „moderner verstellter Zustand → DMR1–3 → nächster Edit/Audio“. |
| **F07 – Unsichtbare Solos** | **TEILWEISE** | Direkter Hide bleibt bei Solo gesperrt [C:208]. Recall maskiert versteckte Solobits im Controller und Prozessor [C:126], [P:604]. [H:1056] prüft den bereinigten Paketstate. Tatsächlicher Hide und hörbares sichtbares Instrument nach diesem Recall werden weiterhin nicht geprüft. |
| **F08 – AUX ohne MAIN** | **PASS, ursprünglicher Fehlerfall** | [P:389] erlaubt Null-MAIN, Renderer ebenfalls. Der15-AUX-Pakettest [H:928] prüft positive erwartete L/R-Pegel über16 Busdeskriptoren bei inaktivem MAIN. Keine reine Finite-Assertion mehr. Benachbarte Busverwechslungen sind wegen grober Toleranz allerdings nicht vollständig abgesichert, siehe Abschnitt7. |
| **F09 – Host-DPI** | **TEILWEISE beim Größenfaktor; FAIL im zusätzlichen API-Pfad N10** | [V:100] verwendet den Gesamtfaktor korrekt. Die geerbte `setContentScaleFactor`-Implementierung behandelt Pre-Attach-`getSize` und Resize-Ablehnung nicht vollständig. Das ist unabhängig von der reparierten Benutzerzoomprüfung. |
| **F10 – Resize-Ablehnung** | **TEILWEISE** | Direkter Countrestore [V:348], [V:362], [C:230] löst die belegte versteckte Row ohne normalen Hide aus. Echte Controller-/Hostregression fehlt; [R:36] benutzt weiterhin nur einen Integer. Recall/Attach mit abgelehntem hohen Fenster hat keinen Scroll-/Fit-Fallback. |
| **F11 – Ctrl-Reset** | **TEILWEISE** | Formeln [V:307], [V:314], [V:384], [V:388] ergeben Human/Swing/Pan/Pitch0 und Gain100. Keine nativen Ctrl-Klicks an allen Feldern und Zoomstufen mit Audio-/Stateprüfung. |
| **F12 – UTF-8-Kürzung** | **TEILWEISE** | [V:177] trennt gültige UTF-8-Namen an Codepointgrenzen. Fontbreite und Grapheme werden nicht gemessen; CJK/Emoji/Kombinationszeichen und sehr breite Namen brauchen echte Render-/Cliptests. |
| **F13 – FLAC-Limits** | **TEILWEISE** | Frühe Metadatengrenze,4096-Frame-Blöcke und maximales Ausgabebudget [F:27], [F:36], [F:49]. Bekannter Längenwiderspruch ist gezielt abgedeckt, N03. Unbekannte Länge, reale Ausgabebudgetüberschreitung, Mehrframekorruption, Peak-RAM und Decoderzeit sind nicht vollständig abgesichert. |
| **F14 – SRC/Performance** | **TEILWEISE** | Adaptive32/64/128/256 Taps und der einzelne Ratio16-Aliaspunkt bleiben unverändert. Neue Voicekapazitätsprüfung ändert daran nichts.180 ungepacte Core-Zeitmessungen ohne archivierte Werte oder Overrun-Gate ersetzen keine Echtzeitqualifikation. |
| **F15 – Zoomanzeige/Persistenz** | **TEILWEISE** | DZM1-State [C:67], Übernahme beim neuen Editor [C:171] und Auswahlfarben vorhanden. Der bisherige Cachefehler ist im Code repariert, N02. Kein nativer Reopen-/Ablehnungsnachweis; `setState` [C:74] verändert die Präferenz, aktualisiert aber keine bereits geöffnete Editorinstanz. Zoom-only-Dirty ungeprüft; Host-DPI N10 bleibt FAIL. |
| **F16 – Hosttexte A–H** | **PASS, definierte Parameterabbildung** | `LegacyPatternParameter::toPlain/toString` [C:25] und alle Prozessor-/GUIpfade benutzen dieselbe alte `round(7v)`-Abbildung [G:6]. Zwischenwert-Hosttexte und tatsächliche Prozessor-Flushstates werden geprüft. Keine aktuelle Host-/DSP-Quantisierungsabweichung erkennbar; N07 ist ein separater Restorefehler. |
| **F17 – Win32-Dateidrop-Länge** | **TEILWEISE** | [V:65], [V:277] behandeln die SDK-Länge ohne zwingenden Terminator richtig und weisen innere NUL ab. Kein echter IDataPackage-/Explorer-/Studio-One-Droptest samt Zielrow und150%-Koordinaten. |

Ein enger PASS schließt den bezeichneten Fehler. TEILWEISE bedeutet nicht automatisch falsche Reparatur, sondern fehlenden vollständigen Nachweis. Ungeprüfte Interaktionsketten bleiben ungeprüft, auch wenn ihre Hilfsfunktionen bestehen.

## 4. Status N01–N10 und Einordnung der jüngsten Reparaturen

| Befund | Status | Nachprüfung |
|---|---|---|
| **N01 – Rollback bei belegter versteckter Row** | **TEILWEISE** | Direktes `restoreVisibleRows(previous)` [C:230], [V:351] kennt keine Sample-/Solo-/Step-Hidesperre und repariert den ursprünglichen Quellfehler. Kein tatsächlicher Produktions-Controller-/IPlugFrame-Test; bestehender Integerhelper kann diesen Pfad nicht absichern. |
| **N02 – Zoomcache/Resize-Ablehnung** | **TEILWEISE** | [E:18] liest jetzt die tatsächliche Transformation `m11`. Der frühere Gegenfall „nach gespeicherten150 %, Cache1,0, abgelehnte100 %“ wird dadurch als Ablehnung erkannt; Editorwert wird auf1,5 zurückgesetzt und Präferenz nicht verändert. Es wurde kein nativer Regressionstest ergänzt. Host-ContentScale ist gesondert N10. |
| **N03 – FLAC-Soll/Ist-Länge** | **PASS, bekannter deklarierter Längenwiderspruch** | [F:57] verweigert bekannte unterschiedliche Framelängen vor Outputswap. [H:777] prüft ausdrücklich deklariert256/actual128 und unveränderten vorherigen Output. Geeigneter Wrappertest im erfolgreichen Windowslauf; keine pauschale Freigabe beliebiger korrupter FLACs. |
| **N04 – Header-Randstreifen** | **TEILWEISE** | Pattern[38,68), Zoom[39,69) sind im tatsächlichen Draw-/Mousecode gleich getrennt. Die alten Gegenpunkte sind rechnerisch richtig; der Test [R:70] dupliziert nur Zahlen in Lambdas. Native Produktregression fehlt. |
| **N05 – Aktuelle Pattern-Abbildungsdifferenz** | **PASS, normaler Parametervertrag** | Eigener Parameter und Prozessor benutzen jetzt gemeinsam die Legacyquantisierung. Hosttexte bei Zwischenwerten [H:115], echte Prozessor-Flushstates [H:882] sowie Helperabgleich [R:57]. Nicht allein aus erfolgreichem Build geschlossen. Gesamt-Automation unter Restore bleibt N07 FAIL. |
| **N06 – Alte Automationsschwellen** | **PASS, ursprünglicher Normalwertvertrag** | `round(7v)` ist wiederhergestellt und mit Hosttext/Plainwert verknüpft. Der0,10→A-/0,90→H-Bruch des vorigen Standes besteht nicht mehr; jetzt B/G wie ursprünglich. Tatsächlicher alter Studio-One-Song/Bounce bleibt ein zusätzliches OFFEN-Gate. |
| **N07 – Automation während Restore** | **FAIL / P1** | Busy-Pfad liest die Events jetzt, aber `finish()` kann eine neuere Auswahl mit einer älteren überschreiben. Die jüngste Regression benutzt echte Produktionshelper, jedoch keine überlappenden Latchaufrufe. Quellengegenbeweis in Abschnitt5. |
| **N08 – Stop nach erstem Restoreblock** | **PASS, gezielter Paketfall** | [P:398] behält den aktuellen Transportzustand; Revisionszweig [P:400] setzt ihn nicht zurück. [H:1094] lädt ein Sample, startet genau einen laufenden Block mit gemessener Energie>1 und verlangt im direkt folgenden gestoppten Block L/R=0. Geeignete tatsächliche Prozessorregression. |
| **N09 – Verworfener Choke bei64Voices** | **PASS, gezielter Audiocorefall** | [K:263] prüft Kapazität vor jeder Chokeänderung. [T:252] füllt64 Stimmen mit einer Gruppe1-Stimme und63 Gruppe0-Stimmen, verwirft einen weiteren Gruppe1-Hit, verlangt64 verbleibende Voices sowie erwarteten Pegel8,0. Der alte Fehler hätte63Voices und ca.7,875 geliefert; Toleranz0,08 verdeckt diesen Unterschied nicht. |
| **N10 – Host-ContentScale/HiDPI** | **FAIL / P2, neu konkretisiert** | Geerbte API liefert vor Attach die falsche Größe und ignoriert live eine Resize-Ablehnung. Getrennt vom reparierten Benutzerzoom; Quellen-/Vertragsgegenfälle in Abschnitt7. |

### N06: Aktueller Vertrag im Detail

Für endliche Normalwerte ist die gemeinsame Abbildung `lround(clamp(v,0,1)*7)`. Grenzen sind `(p+0,5)/7`, also ca.0,0714286 /0,2142857 /0,3571429 /0,5 /0,6428571 /0,7857143 /0,9285714. Kanonische GUIwerte bleiben `p/7`. Nichtendliche Werte werden im Helper auf0 abgebildet und im Prozessorinput verworfen.

`LegacyPatternParameter` überschreibt sowohl `toPlain` als auch `toString`. Geerbtes `toNormalized` und `fromString` liefern weiterhin `p/7`; GUI und Prozessor nehmen dieselbe Helperfunktion. Der frühere Konflikt mit SDK-`floor(8v)` ist deshalb nicht mehr vorhanden. Diese Reparatur erhält den ursprünglichen Drumatorvertrag und verändert die Abbildung des kurzlebigen Zwischenstandes zurück. Projekte, die gerade auf dessen `floor(8v)`-Semantik angewiesen wären, dürfen nicht als zusätzlich kompatibel beworben werden. Für die ursprüngliche Kompatibilitätsanforderung ist die gewählte Rückkehr sachgerecht.

**Noch ergänzen:** `normalizedParamToPlain`, `getParamValueByString`, alle exakten Grenzwerte einschließlich `nextafter` beidseitig und hörbare Mehrpunkt-Automation. Der bestehende1001-Punkte-Helpertest bestätigt die Formel, die Pakettests bestätigen ausgewählte echte Controller-/Prozessorpfade. Ein alter echter Song mit hörbarem Vorher/Nachhervergleich wurde hier nicht ausgeführt.

### N08/N09: Grenzen der anerkannten PASS

Der N08-Test erzeugt1024 Quellframes bei48kHz; beim zu diesem Testzeitpunkt eingestellten44,1kHz-Ausgang und neutralem Pitch sind das ca.940,8 Ausgabeframes, somit mehr als ein256-Frame-Block. Seine positive Energieassertion verhindert einen bestandenen Test ohne gestarteten Sound. Die nächste Stillheitsassertion prüft die tatsächliche Stopwirkung. Der geänderte Code ist exakt der relevante Pfad; Loop/Seek und Stop während eines noch aktiven Busyfensters sind zusätzliche Fälle.

N09 verwendet die tatsächliche Voiceengine, kein vereinfachtes Kapazitätsmodell. Abweisung erfolgt nun vor Änderung vorhandener Releases. Die vorhandenen normalen Choketests bleiben bestehen. Das ist ein geeigneter geschlossener Corefehlerfall, jedoch kein neuer Nachweis sämtlicher Studio-One-MIDI-/Stresskombinationen.

## 5. N07: Verbleibender Lost-Update-Fehler

**Priorität P1, FAIL.** Produktpfade: [PatternRestoreLatch.h:14][I:14], [PatternRestoreLatch.h:19][I:19], [DrumatorProcessor.cpp:348][P:348], [DrumatorProcessor.cpp:632][P:632].

`submit()` schreibt nacheinander Pendingwert, Revision und `selected`. `finish()` prüft die Revision, liest Pendingwert und schreibt ihn dann nach `selected`. Der letzte Ausdruck ist keine atomare Lese-/Schreibtransaktion über beide Variablen. Die Gate-Busyphase lässt `submit()` ausdrücklich parallel zum Controlthread laufen. Der Controlthread wartet nach seinem Eintritt nicht auf diese späteren Busy-Aufrufe, bevor er `finish()` ausführt.

### 5.1 Konkreter zulässiger Ablauf

Anfang: KitB wird installiert; Restore-Startrevision0. Ein früher Busy-FlushH wurde schon vollständig eingereicht: `pending=7`, `revision=1`. Die Kitinstallation hat `selected` danach vorübergehend aufB gesetzt.

| Schritt | Thread / tatsächliche Operation | Ergebnis |
|---|---|---|
|1|Control: `finish` liest Revision1, ungleich Start0|Replay wird durchgeführt.|
|2|Control: lädt Pendingwert7 für den anschließenden Store|Lokaler zurückzuschreibender WertH; Control wird unterbrochen.|
|3|Audio: neuer Busy-FlushG schreibt `pending=6`|Neuester HostwunschG.|
|4|Audio: erhöht Revision auf2|Neuer Event ist veröffentlicht.|
|5|Audio: schreibt `selected=6`; `submit` endet|G wurde tatsächlich übernommen.|
|6|Control: setzt den in Schritt2 geladenen alten Wert7 nach `selected`|**H überschreibt das neuere G.**|
|7|Control: Revision erhöhen, Gate öffnen; kein weiteres Hostevent|Nächster normaler Block und getState könnenH verwenden, obwohl letzter HostwunschG war.|

Die C++-Auswertungsreihenfolge erlaubt diese Unterbrechung zwischen Laden des Arguments und dem Store. Keine parallelen `process()`-Aufrufe sind erforderlich: zwei aufeinanderfolgende Audioflushes können während eines einzigen Restorefensters auftreten. Audio und Control halten jeweils ihre Quellreihenfolge ein. Alle beteiligten Werte dürfen sogar mit stärkeren sequentiell konsistenten Operationen betrachtet werden; der Ablauf bleibt möglich. Zusätzliche Acquire-/Releaseflags allein beseitigen den logischen Fehler nicht.

Unabhängig wurden alle20 Ordnungsverschachtelungen der drei `finish`-Operationen und der drei Operationen des letzten `submit` nach einer bereits eingereichten ersten Automation analysiert. Genau die oben dargestellte Ordnung endet mit einem veralteten Auswahlwert. Das ist eine operationale Quellenanalyse, kein behaupteter realer Windows-Lauf oder Timingmesswert.

### 5.2 Weshalb der neue Test trotzdem grün ist

[R:104] verwendet jetzt zwar **die echte Klasse**, führt aber `submit(H)`, simulierten Kitstore und `finish()` vollständig nacheinander aus. Danach prüft er einen weiteren sequenziellen Restore ohne neue Automation. Beide Fälle sind nützlich; beide schließen die kritische Überlappung von Pendingload und Selectionstore aus.

Der Zwei-Thread-Test direkt davor betrifft ausschließlich `KitRestoreGate`, nicht diesen Latch. Der Paket-Konkurrenztest [H:1010] enthält keine Parameterqueues. Kein vorhandener Test koppelt deshalb tatsächliches `setState`, laufenden Busy-Flush und konkurrierendes `finish` mit einer Last-Event-Assertion.

### 5.3 Reparaturempfehlung und notwendige Regression

Die endgültige Patternauswahl muss eine eindeutig linearisierbare Ownership-/Übergabeentscheidung erhalten. Ein späterer erfolgreicher Hostevent darf nicht durch das Zurückschreiben einer früher gelesenen Kopie verloren gehen. Mögliche Architekturentscheidungen sind eine einzelne getaggte Zustandsveröffentlichung mit klarer Generation oder eine Übernahme durch den Audioowner nach abgeschlossenem Kitcommit; die Eignung muss vor Implementierung gegen Flush, getState und fehlenden Folgecallback geprüft werden. Unbegrenzte Compare/Retry-Loops oder Audio-Mutexe sind kein akzeptabler Ersatz.

Notwendig ist eine deterministische Regression genau für „finish liestH → neuer vollständiger FlushG → finish schreibt alte Kopie“. Dazu Fälle vor/nach Kit-Selectionstore, nach letzter Revisionsprüfung, unmittelbar vor Gateöffnung, direkt nach Gateöffnung und vor dem nächsten Restore. Nach Abschluss ohne erneutes Hostevent müssen gespeicherter Zustand, Host/Controller und hörbares Pattern den definierten letzten Wert zeigen. Ein bloßer Stresslauf ohne Last-Event-Oracle oder „kein Crash/kein NaN“ genügt nicht.

## 6. Synchronisation, Kitatomizität und Speicherownership

### 6.1 Die Gate-Reparatur löst den früheren Eintrittsfehler

[KitRestoreGate.h:15][D:15] kodiert Busybit und Audiozähler in **demselben** `atomic<uint64_t>`. Audioeintritt `fetch_add`, Writerbeginn `fetch_or`, Readeraustritt `fetch_sub` und Writerende `fetch_and` stehen in einer gemeinsamen Modifikationsordnung.

- Liegt der Audioeintritt vor dem Busy-RMW, sieht der Writer den vorhandenen Reader und wartet auf dessen Ende.
- Liegt der Eintritt danach, sieht der Audio-RMW das Busybit und betritt den normalen Sample-/Grid-/Mixpfad nicht.
- Der Controlthread gibt nach seinen Schreiboperationen mit Release frei; der nächste zugelassene Audio-RMW übernimmt mit Acquire die Veröffentlichung.
- Mehrere Control-Restores werden in der tatsächlichen Verwendung zusätzlich durch `kitMutex_` serialisiert [P:607]. Die Audioschnittstelle wird entsprechend dem Produktvertrag von einem Audioactor verwendet.

Damit besteht der vorherige Store-Buffering-Gegenfall mit zwei unabhängig beobachteten Atomics nicht mehr. [D:9] fordert für das64-Bit-Atomic statisch Lockfreiheit. [R:77] hält einen vorhandenen Produktionsreader offen, startet einen tatsächlichen Writerthread, prüft Ablehnung neuer Leser und Verbleiben des Writers außerhalb bis zum Readerende sowie erneuten Eintritt nach Freigabe.

**Bewertung des früheren Gate-Einwands: PASS für die Eintritts-/Freigabeprimitive unter dem angegebenen Single-Audio-/serialisierten-Writer-Vertrag.** Das ist eine Quellenbegründung plus geeigneter Produktionstest im vorhandenen Windowslauf. Daraus folgt kein PASS des getrennten Patternlatches, keine garantierte maximale Atomiclatenz und keine vollständige Echtzeit-/Kit-Audioabnahme.

### 6.2 Samples, Grid und Mix: nachvollziehbare Veröffentlichung, noch fehlende Audioabnahme

Während des Gates bereitet [P:513] alle16 frischen Assets vor, nimmt Pendingwerte zurück und veröffentlicht die neuen Pointer. Ein zugelassener normaler Audioblock kann diese per-Row-Installation nicht mehr währenddessen konsumieren. Grid und Mix werden vor Freigabe installiert. Im ersten vollen Audioblock wird wegen der neuen Kitrevision der alte Voicebestand zurückgesetzt [P:400], danach werden Assets adoptiert [P:406].

SPSC-Ownership bleibt unverändert: atomare Übernahme **vor Dereferenzierung** [S:169], Stimmen halten immutable Assets, Rückgabequeue statt Delete im Audio [S:195], Destruktion und Decodierung auf Control. Beim Gesamtkitwechsel werden zurückgegebene Assets auf Control abgeholt; nach dem Voicereset können Retireplätze freigegeben werden. Unter den genannten Annahmen ist kein neuer konkreter Sample-Use-after-free- oder Teilgrid-Gegenfall festgestellt worden. Das wird nicht mit einem vollständigen Laufzeitbeweis verwechselt.

**Gesamter Kit-/Parametervertrag: TEILWEISE, wegen N07 für Auswahlkonsistenz FAIL.** Der vorhandene Paket-Konkurrenztest lädt sechsmal denselben Kitstand, verlangt nur erfolgreiche Aufrufe, finite Samples und insgesamt mindestens einen Audioblock. Eine ausschließlich stumme Ausgabe kann ihn weiterhin bestehen. Alternierende Kits mit unterschiedlichen Samples, Patterns, Gains und Gates und ein genauer Generations-/Pegeloracle fehlen; Wiederanlauf und Überlappung pro Restore sind nicht erzwungen.

### 6.3 Weiterhin offen: Stummfenster und Control-Wartezeit

Das Gate wird vor der vollständigen Kernel-/Assetvorbereitung genommen [P:609]. Während aller Dekodier-/Vorbereitungsarbeiten liefern Busycallbacks Nullausgabe. MIDI wird dabei weiterhin nicht übernommen; nur die Patternautomation wird im Latch behandelt. Ein nachher abgelehnter Vorbereitungskandidat kann bis dahin ebenfalls Stummzeit verursacht haben.

Audio enthält keinen Gate-Spinloop und keinen `kitMutex_`. Der Writer wartet mit `yield()` auf verschwindende Zähler [D:21]; auch abgewiesene Busycallbacks zählen kurz mit. Eine garantierte maximale Control-Wartezeit oder Dauer des Stummfensters ist nicht gegeben. Lockfrei bedeutet hier nicht wartefrei oder unter allen Lasten rechtzeitig. Hostparameter-/Eventmengen werden außerdem weiter ohne eigenes gemessenes Lastbudget durchlaufen.

**Nacharbeit:** Teure Kandidatenvorbereitung vor ein möglichst kurzes, belegbar korrektes Commitfenster verschieben, sofern die Ownership dies erlaubt; MIDI-/Dropoutpolicy definieren. Mit realistischen großen16-Row-Kits und hoher SRC-Ratio Stummblöcke, längste Stummsequenz, verlorene Events, Rückgewinnung, Wiederanlauf und Controlwartezeit messen. Keine hypothetische Laufzeit erfinden und kein Nullausgabepfad als Beweis kontinuierlichen Audios deklarieren.

## 7. Neuer Befund N10 und weitere subsystembezogene Ergebnisse

### N10 – P2 / FAIL: Host-HiDPI umgeht Zoomprüfung und verletzt Pre-Attach-Größenvertrag

Produktstelle: [DrumatorEditor.h:8][E:8] erbt den SDK-Editor ohne Override von `setContentScaleFactor` oder `getSize`. [C:169] erzeugt diese Klasse. Die zuletzt geänderte Prüfung [E:14] betrifft ausschließlich den ausdrücklich aufgerufenen Benutzerzoom.

**Verbindlicher SDK-Vertrag:** [IPlugViewContentScaleSupport][sdk-scale-contract] erlaubt den Aufruf vor oder nach Attach. Auch vor gesetztem `IPlugFrame` muss ein folgendes `getSize()` bereits die zum neuen Faktor passende Größe zurückgeben.

**Gegenfall ohne angehängtes Fenster:** Editor mit Standard100 % erzeugen. Host ruft `setContentScaleFactor(1.5)` vor Frame/Attach auf. Die tatsächlich geerbte [SDK-Implementierung Zeile588][sdk-scale] speichert nur den Faktor und gibt Erfolg zurück, weil kein Frame vorhanden ist. Sie ändert das aus der UIDESC gelesene Rect nicht. Das geerbte [CPluginView::getSize Zeile69][sdk-getsize] kopiert danach genau dieses Rect. Ergebnis bei vierRows: weiterhin **1280×300** statt **1920×450**. Es gibt in der Drumator-Unterklasse keinen kompensierenden Pfad. Das ist ein nachweisbarer SDK-Vertragswiderspruch; ein späteres Resize beim Öffnen heilt die vorherige falsche Größenantwort nicht.

**Zusätzlicher Ablehnungspfad bei offenem Editor:** Der Host fordert einen neuen ContentScale-Faktor1,5 statt1 an. Die geerbte SDK-Methode ändert `contentScaleFactor`, ruft `CFrame::setZoom` auf und ignoriert dessen boolesches Ergebnis. Lehnt der Host das Resize ab, stellt der Frame die bisherige Transformation1 wieder her, der editorseitige Gesamtfaktor bleibt aber1,5 und die Methode meldet Erfolg. `adjustEditorHeight()` verwendet anschließend diesen nicht tatsächlich wirksamen Gesamtfaktor. Der in N02 verbesserte `setZoomChecked` wird hier nicht aufgerufen.

**Einordnung:** Bereits im vorigen Bericht war der geerbte HiDPI-Ablehnungspfad als Risiko genannt. Nun wird der zusätzliche Pre-Attach-Vertragsbruch konkret nachverfolgt. Es handelt sich um unverändert geerbtes SDK-Verhalten; es ist kein Beleg dafür, dass die neue `m11`-Prüfung falsch ist. Eine tatsächlich im konkreten Studio-One-Fenster beobachtete Auswirkung wird nicht behauptet.

**Empfehlung:** Host-ContentScale, Benutzerzoom, tatsächliche Transformation und veröffentlichte Editorgröße in allen Lifecyclephasen konsistent behandeln. Vor Attach die richtige `getSize`-Antwort gewährleisten, live das tatsächliche Resizeergebnis berücksichtigen. Keine pauschale Ablehnung von HiDPI und keine ungeprüfte Änderung des gepinnten SDKs.

**Fehlende Regression:** Tatsächliches Paketeditorobjekt; Interfacequery, ContentScale1/1,5/2 vor `setFrame` und vor/nach Attach, jeweils `getSize`. Anschließend nativer Frame und ablehnender Host für beide Richtungen; Folge-Rowresize, gespeicherter Benutzerzoom und Close/Reopen. Die vorhandenen reinen `getSize`-/Controllerstate-Smokes rufen dieses Interface nicht auf.

### 7.1 Audio/SRC, Stimmen und Hostverhalten

Die SRC-Koeffizientenformel und ihre32/64/128/256-Tap-Auswahl sind unverändert. Die unabhängig berechneten Phase0-Werte für Ratio16 bleiben: Quellfrequenz0,04 etwa−47,678dB,0,06 etwa−92,328dB,0,028125 etwa−1,918dB und0,034375 etwa−14,064dB relativ DC. Berechnung: Blackman-gefensterter Sinc nach [K:35], Float32-Koeffizientenspeicherung, DC-Normalisierung und diskrete Fourierauswertung. Das sind analytische Filterwerte, keine hier gerenderte Plugin-FFT.

Die alten starken Aliasgegenfälle sind verbessert. Der einzelne Test [T:33] bei Ratio16/Phase0/Frequenz0,04 gegen40dB ist weiterhin kein Passband-/Stopband-/Fractional-Ratio-/Phasensweep. Ohne festgelegten Qualitätsvertrag ist keine pauschale „Aliasingfreiheit“ oder Hörfreigabe begründet. Realistische Drums, Stereo, Impulse an Grenzen und Pitchmatrix fehlen als vollständiger Nachweis.

Voicepool64, One-Shot-Retrigger,2ms-Chokefade und Startparameterretention bleiben erhalten. Mute/Solo filtern neue Trigger; laufende Tails behalten Launchgain/-pan/-pitch/-asset. Diese bestehende Policy ist ausdrücklich getestet. Stop ist davon getrennt und setzt Tails zurück, nun inklusive N08. N09 ist gezielt repariert. Voice-/Triggerüberläufe bleiben im Produkt ohne sichtbare Diagnostik, obwohl der Core Kapazitätsablehnungen zählt.

Pan ist Stereo-Balance, Gain linear0…2, Mitte bewahrt den Pegel pro Kanal. Kein Limiter oder versteckte Normalisierung. MIDI-NoteOn36…51 triggert Rows1…16, Velocity1…127; NoteOff wird bei One-Shots ignoriert. Kanalunabhängiges Triggern ist implementiert; keine freie Notenzuordnung oder MIDI-Learn-Oberfläche. Nur Patternparameter1000 ist hostautomatisierbar, die übrigen UI-Felder sind Editnachrichten mit Projektstate.

Der32-Step-/A–H-Scheduler benutzt absolutes PPQ, begrenzte Boundaryarbeit, Swing und deterministisches Humanize. Hostloop/Seek, negative PPQ, geswingte/humanisierte Auswahlgrenzen, mehrere Parameterpunkte im laufenden Block und Offline-/Realtimegleichheit benötigen weitere hörbare und zeitgenaue Assertions. Loop/Seek haben keinen eigenen expliziten Voiceflush; die gewünschte Tailsemantik ist weiter festzulegen.

### 7.2 Performance: keine neuen Messartefakte

[T:276] misst weiterhin180 ungepacte **Core**-Renderaufrufe mit64 Voices, Ratio16,256Taps,48kHz/256Frames.160 Blöcke enthalten aktive Voices,20 sind vollständig leere Drainblöcke; verarbeitete Audiolänge0,96s. p95/p99 sind die171./179. sortierten Werte, Max der180.; Deadline rechnerisch **5,333333ms**.

Die neue Kapazitätsprüfung verändert etwas Arbeit beim Voicefeuern, liefert aber keinen neuen End-to-End-Echtzeitnachweis. Es besteht nur die gesamte `<20s`-Hanggrenze [T:332]. Deadline wird daneben gedruckt, nicht als Limit geprüft; keine Overrunzählung. Timer-/Profiler-Overhead, CPU/Takt/Energieprofil und Systemlast sind nicht dokumentiert. Monoquelle, gemeinsam genutzter Kernel und eine Rate/Blockgröße sind keine vollständige Worst-Case-Matrix.

Der Workflow bleibt bei `ctest --output-on-failure` [W:36]. stdout erfolgreicher CTests wird deshalb nicht regulär ausgegeben. Ein lokales `Testing/Temporary/LastTest.log` wird nicht als Artefakt archiviert; hochgeladen wird nur das VST3-Bundle [W:110]. Die p95/p99/Max-Zahlen des aktuellen Laufs sind im zugänglichen Material somit weiterhin nicht auswertbar. **Keine fehlenden Zahlen werden aus der grünen Run-Zusammenfassung geschätzt.**

Erforderlich bleiben veröffentlichte CSV-/JSON-Messwerte mit Metadaten, vollständiger `IAudioProcessor::process`-Messung, getrenntem Offline-Durchsatz und zeitlich getaktetem Realtimeversuch, mean/p95/p99/max, Overruns, Dropouts, verlorenen Triggern, Callbackallokationen und mehrinstanzigem UI-/Import-/Kitstress. Selbst ein gemessener Maxwert unter einer Deadline ist eine Stichprobenbeobachtung und keine unbedingte Garantie.

### 7.3 Import, RAM, State und Multi-Out

WAV bleibt RIFF Mono/Stereo PCM8/16/24/32 und Float32 mit passenden Extensible-GUIDs. NaN/Inf und Floatwerte über Absolut16 werden verworfen. RF64/RIFX, Float64 oder fremde ValidBits werden nicht als unterstützt ausgegeben. FLAC prüft Metadaten vor Vollausgabe, liest4096 Frames pro Schritt und vergleicht bekannte Soll/Ist-Länge vor Austausch. Unbekannte Länge und beschädigte Mehrframe-/Endfälle brauchen weitere Fixtures; vollständige Korruptionserkennung ist nicht nachgewiesen. Bei Fehlern werden Kandidaten nicht regulär als neues Sample übernommen; Controllerlade-/Drop-End-to-Endfälle fehlen trotzdem.

16MiB je gehaltenem WAV und64MiB für die WAV-Summe sind keine Prozess-RAM-Grenzen. PCM8→Float kann etwa vierfache PCMgröße erzeugen; Preflight, Kopien, alter/neuer Kit, Voices und Retire-/Returnobjekte können gleichzeitig existieren. Jeder nichtleere PreparedSample reserviert1024×256 Floatkoeffizienten=1MiB, also16MiB für16 aktuelle Kernel, auch bei weniger tatsächlichen Taps. Pointerkapazitäten96/128 sind kein Bytebudget. Peak-RAM, Speicherdruckablehnung, UI-Ladezeit und Rückgewinnung bei vielen Wechseln sind weiter offen.

DM10 speichert Samples, Namen,4096 Velocityzellen, ausgewähltes Pattern, sichtbare Rows, M/S, Gain/Pan/Pitch/Choke/Swing/Humanize. DMR4–9 haben Codec-Migrationstests; die tatsächlichen DMR1–3-Controller-/Editorpfade sind nicht entsprechend abgesichert. DZM1-Zoomstate ist ein eigener Controllerstate; Änderung bei bereits offenem Editor wird nicht angewandt. Eine spätere Reparatur muss Legacydefaults und nächsten Edit prüfen, nicht nur Bytes.

Multi-Out bleibt fest: Row1→MAIN, Rows2…16→ihre15AUX-Busse, wenn aktiv und gepuffert, andernfalls MAIN. Der neue15-AUX-Pegeltest bleibt nützlich, seine Sollpegel unterscheiden sich jedoch um0,015 bei erlaubtem Absolutfehler0,03 [H:951], [H:1001]. Eine benachbarte Vertauschung kann bestehen. Für strenge Isolation sind Single-Row-Tests oder eindeutig getrennte Signaturen mit begründeter Toleranz erforderlich. Inaktive Lücken, AUX15 allein, Reaktivierung und Studio-One-Songrecall fehlen als vollständige Hostmatrix. Aus der schwachen Testtoleranz allein wird kein tatsächlicher Routingfehler behauptet.

32-Bit-Audio wird unterstützt,64-Bit korrekt abgelehnt. `setActive(false)`/`setProcessing(false)` setzen Voices zurück; `setupProcessing` baut vorhandene Samples für neue Rate außerhalb aktiven Processings neu auf.48→44,1kHz wird im vorhandenen Pakettest auf positiven Pegel geprüft. Weitere Raten, variable Blockgrößen und wiederholter echter Lifecycle bleiben offen. Unendlicher Tail ist bewusst für den selbstlaufenden Sequencer vorgesehen.

## 8. Gesamte provisorische GUI: aktuelle Kontroll- und Geometriematrix

Dies ist die vollständige **Quellenzuordnung**, keine behauptete native GUI-Abnahme. Es gibt keine individuellen VSTGUI-Controltags: sämtliche folgenden Aktionen werden manuell in einer `DrumatorView` gezeichnet und getroffen. Für jede Zeile `r=0…visibleRows−1` und jeden Step `s=0…31` gelten die Formeln. Damit sind alle bis zu512 Step-Tasten erfasst; kein nicht geprüfter Einzelstep wird als praktisch bestanden ausgewiesen.

Logische Größen: `y=108+29*r`, `xStep=560+21*s`, Breite1280, `H=max(300,172+29*visibleRows)`. Physische Anforderung: logisch × Benutzerzoom × Host-ContentScale. Rechtecke nachfolgend links/oben inklusive, rechts/unten für Hit-Tests überwiegend exklusiv.

| Element / Anzahl | Position, Maße und tatsächlicher Klickbereich | Daten-/Funktionszuordnung, Rückmeldung, State und Auditstatus |
|---|---|---|
| Titel1 | x16…1000, y8…32; Schrift18, links | Anzeige, kein Klick. Keine native Font-/Clippingmessung. |
| Patternhilfe1 | x16…320, y47…67, links | Anzeige `PATTERN A-H / SHIFT+STEP`; kein Parameter. Keine Stepnummern oder Lauflichtanzeige implementiert. |
| Pattern A–H,8 | x340+62p…+54, y38…68;54×30;8px Zwischenräume ohne Aktion | `changePattern` → begin/perform/endEdit Parameter1000 → Hostqueue → Prozessor; Auswahlfarbe aus `currentPattern`; State selected. Aktuelle Abbildung N05 und Legacyvertrag N06 konsistent; Restore-Race N07 bleibt FAIL. Native Audio-/Anzeigeinteraktion OFFEN. |
| Zoom100,1 | x1070…1127,y39…69;57×30 | `setZoomChecked(1)` → rememberZoom → DZM1; Farbe vom Editorwert. N02-Quellreparatur vorhanden, native Ablehnungsprüfung OFFEN; Host-DPI-Pfad N10 FAIL. |
| Zoom150,1 | x1136…1217,y39…69;81×30 | Entsprechend Faktor1,5. N02-Quellreparatur vorhanden; Wiederöffnung/Hostablehnung OFFEN, Host-DPI N10 FAIL. |
| Swinganzeige/−/+,1 | Label16…182,y74…98; Bedienzone185…244, Teilung214 | Klick±5, Ctrl→0, Grenzen0…50; Nachricht `Drumator.Swing`, Prozessoratomik, DM10; Anzeige sofort bei Erfolg. Nur ±-Zone ist bedienbar. Native Reset-/Timingprüfung OFFEN. |
| Humananzeige/−/+,1 | Label275…447,y74…98; Bedienzone448…505, Teilung477 | Klick±10, Ctrl→0, Grenzen0…100; `Drumator.Humanize`, deterministisches Timing/Velocity, DM10. Native Wirkung OFFEN. |
| Zeilennummern,1…16 | x16…47,y…y+26; links | Anzeige; liegt im Sample-Klickbereich16…275 und öffnet daher bei Klick ebenfalls den Dateidialog. Kein unabhängiger Zeilenwahlparameter. |
| Samplelabel/Load/Clear,1…16 | Text52…275,y…y+26; Klick16…275 über volle29px | Klick→WAV/FLAC-Picker→Controllerpreflight→WAV-Nachricht→PreparedSample→nächster Block; Alt→Clear, Steps bleiben. Filename statt unabhängigem Instrumentennamen; Erfolg/Fehler im Footer. Sample/Name im Kitstate. Native Auswahl/Abbruch/Drop/Unicode OFFEN. |
| VOL,1…16 | Text/Klick x280…345; Hit volle29px, Text26px; Halbierung312,5 | ±10 linear, Ctrl100; RowMix→Gain0…2; neuer Voicegain, State; numerische Anzeige. Breiten-/Fontmessung OFFEN. |
| PAN,1…16 | x350…415, Halbierung382,5; übrige y wie VOL | ±10, Ctrl0; RowMix→Balance−1…1, State; Vorzeichenanzeige. Laufende Voices behalten Startbalance. Native Wirkung OFFEN. |
| P/Pitch,1…16 | x420…475, Halbierung447,5 | ±1 Halbton, Ctrl0, Grenzen±24; RowPitch→Assetneuvorbereitung→neue Voices, State; Anzeige mit Vorzeichen. Native Reaktionszeit OFFEN. |
| Mute,1…16 | Gezeichnet478…500,y+3…y+24; Hit478…500 über volle29px | RowGate→Mute-Maske→Filter neuer Trigger, orange Rückmeldung; State. Größerer Hitbereich berührt keinen anderen Rowcontrol. Keine native Abnahme. |
| Solo,1…16 | Gezeichnet507…529,y+3…y+24; Hit über volle29px | RowGate→Solo-Maske, cyan Rückmeldung; State und neue Recallmaskierung. F07 TEILWEISE. |
| Choke,1…16 | Text535…558,y+3…y+24; Hit über volle29px | Klickzyklus0…4→RowChoke→neue Voicegruppe; numerische Anzeige/State. Kein eigener beschrifteter CHOKE-Header und kein eigener Ctrl-Resetpfad; N09 bei voller Kapazität nun gezielt im Voicecore regressiert. Native Zuordnung OFFEN. |
| Step-Tasten,32 je sichtbarer Row | Zelle21×29; Füllung(x+2…x+18,y+4…y+22)=16×18; Viertelrahmen bei s%4=0 | Ganze zugehörige Zelle klickbar; Klick0↔127, Shift0/32/64/96/127; StepEdit→AtomicPatternBank→Scheduler→Voice; Farbe und Velocitybar aus aktuellem Pattern; im State alle8×16×32 Zellen. F01/F02 teilweise; native Tests jedes Steps bei beiden Zooms fehlen. |
| Add,1 | x16…213, yA…yA+29, `yA=113+29*n` | Count+1→Hostresize→direkter alter Count bei Ablehnung; Status bei Max16/Fehler. Keine Datenlöschung. N01 Quellreparatur, native Verifikation fehlt. Rechter Randx=213 ist inklusiv getroffen. |
| Hide,1 | x226…266, gleiche yA | Nur letzte leere/nichtsolo Row; prüft Steps in allen8Patterns. Bei Ablehnung Countrestore. Farbe aus `canHideLast`; Fehlermeldung erwähnt nur Steps/Sample, nicht Solo. Native Prüfung OFFEN; rechter Rand inklusiv. |
| Zeilenzahl1 | x277…455, gleiche yA, links | Anzeige n/16, keine eigene Aktion; State visible. |
| Status/Footer1 | x16…1240,y=147+29*n…+21, links | Import-/Edit-/Resizefehler und Erfolg. Keine eigene Interaktion; keine Messung der tatsächlichen langen Textbreite. |
| Bedienhilfe1 | x545…1240,y74…98, links | Anzeige der Halbflächen-/Alt-Bedienung. Ctrl-Reset/Choke/Outputmapping werden nicht vollständig erklärt; kein automatischer Funktionsnachweis. |

Die größeren Row- und Step-Hitflächen sind eindeutig einer Zeile/einem Step zugeordnet; sie werden nicht allein wegen Abweichung von Glyphen als Fehler deklariert. Headerlücken und falsche Zielzuordnung werden dagegen gesondert geprüft. Kein Scroll-, Drag-, Wheel-, Texteingabe- oder Keyboard-Editingpfad für diese numerischen Anzeigen ist implementiert. Die Engineering-Bedienanforderungen für präzise numerische Bedienung sind vor der Final-GUI verbindlich zu klären und nativ abzunehmen.

### Größen- und Instrumentenzahlmatrix

| Sichtbare Rows | Logische Größe100 % | Größe150 %, ohne zusätzliches DPI | Letzte Stepfüllung, y-Bereich | Add-/Hide-Oberkante | Footerunterkante |
|---|---|---|---|---|---|
|1|1280×300|1920×450|112…130|142|197|
|4|1280×300|1920×450|199…217|229|284|
|5|1280×317|1920×475,5|228…246|258|313|
|8|1280×404|1920×606|315…333|345|400|
|16|1280×636|1920×954|547…565|577|632|

SDK-Resizeanforderungen runden physische Koordinaten mit `floor`; bei5Rows/150 % ist die halbe Pixelhöhe daher nativ nachzuprüfen. Letzter Step rechts: Füllungx1213…1229, Zelle1211…1232, innerhalb1280. Bei16Rows/150 % benötigt das Fenster1920×954 plus Hostrahmen; auf einem1920 breiten Monitor kann dies tatsächlich Ablehnungen oder Platzprobleme auslösen. Mit ContentScale1,5 kämen2880×1431 hinzu. Das sind geometrische Anforderungen, keine Behauptung über das tatsächliche Verhalten eines konkreten Studio-One-Fensters.

**Weiteres offenes Layoutproblem:** Nach gültigem16-Row-Recall kann [C:121] bereits16Rows anzeigen wollen, während [V:88] eine Vergrößerung anfordert. Bei Hostablehnung bleibt die kleine View; es gibt keinen Scroll-/Fit-Fallback. N01 behandelt nur Add/Hide-Rollback und löst diesen Lifecyclefall nicht. Ebenso kann `Controller::setState` die Zoompräferenz ändern, ohne einen bereits geöffneten Editor zu aktualisieren. Beide Wege benötigen echte Hosttests und gegebenenfalls eine definierte Bedienstrategie.

**Noch offene Produktziele:** Eigenständige Instrumentennamen, frei wählbare Outputs, Lauflicht/Stepnummern, Samplepreview, Copy/Paste, Undo und optionaler Vintage-Samplercharakter sind im Draft `docs/V1-SCOPE.md` genannt, aber nicht vollständig implementiert. Der Draft ist ausdrücklich keine gesperrte Spezifikation. Daraus werden keine erfundenen neuen Pflichtbugs abgeleitet; vor „funktionale Entwicklung abgeschlossen“ muss der Owner den tatsächlichen V1-Umfang festlegen.

## 9. Regressionstestbewertung und offene Risikofortschreibung

| Test | Geeigneter Nachweis | Verbleibende Lücke |
|---|---|---|
|N06: [H:115], [H:882], [R:57]|Zwischenwerttexte, echte Prozessor-Flushstates und Legacyformel.|Plain-/String-Rückrichtung, Grenznachbarn, alter Song und hörbare Mehrpunkt-Automation.|
|N08: [H:1094]|Echter Restore, gemessener positiver Sequencerblock, direkter Stop mit L/R=0.|Busy-Stop, Loop/Seek, sonstige Tailentscheidungen.|
|N09: [T:252]|Echte64Voices und Chokeablehnung ohne Voiceverlust; Audiopegel.|Weitere MIDI-/Multi-Out-/Lastkombinationen, keine pauschale Echtzeitabnahme.|
|Gate: [R:77]|Tatsächliche Klasse und zwei Threads; Writer wartet auf zugelassenen Reader; neue Busyreader werden abgewiesen.|Keine vollständige Kitgeneration-/Pegelprüfung, keine Controlwartezeitmessung.|
|Latch: [R:104]|Echte Klasse mit zwei gezielten sequenziellen Arbitrierungsfällen.|Kein einziger überlappender Submit/Finish-Aufruf; N07 bleibt unsichtbar.|
|Paket-Kitstress: [H:1010]|Kein NaN, Rückgabefehler oder offensichtlicher Crash bei sechsmal gleichem Restore.|Nullausgabe kann bestehen; kein eindeutiger Kitwechsel, kein Last-Event-Oracle oder Wiederanlauf pro Restore.|
|N02/F15: [H:127], [Q:45]|Controller-Zoombytes und Ressourcenrect vorhanden.|Keine tatsächliche Transformation, kein HWND/Attach, kein ablehnender Host, kein ContentScale.|
|N01/F10: [R:36]|Generische Integer-Transaktionsreihenfolge.|Kein `restoreVisibleRows`, keine belegte versteckte Row und kein IPlugFrame.|
|N04: [R:70]|Lokale Testprädikate für Header-y-Ränder.|Produkt-Draw/Hitcode wird nicht ausgeführt; reine Produktionsregression kann grün bleiben.|
|F05: [R:51]|Boolhelper führt Callback bei Erfolg aus.|Kein Controller, kein IComponentHandler2, keine Hostmodified-/Autosavewirkung.|
|F07: [H:1056]|Verstecktes Solo im Paketstate nach Recall entfernt.|Keine hörbare sichtbare Row, kein echter GUI-Hide.|
|F08: [H:928]|15AUX liefern Pegel bei Null-MAIN.|Toleranz kann benachbarte Busvertauschung verdecken; inaktive Lücken fehlen.|
|N03/F13: [H:777]|Bekannter Längenwiderspruch abgelehnt, alter Wrapperoutput bleibt.|Unbekannte Länge, echte Oversize-Dekodierung, native Lade-/Dropkette und Ressourcenmessung.|
|F14: [T:33], [T:276]|Ein Aliasfrequenzpunkt, feste Voicegrenze und Offlinezeitdatensatz.|Kein vollständiger SRC-Vertrag, keine archivierten Messzahlen und kein Realtimegate.|
|F03/F17/Legacy|Relevante einzelne Controller-/Codec-/Importpfade vorhanden.|Kein angehängtes Repaint-/Drop-/Legacy-Controller-Audiotestpaket.|

### Frühere Risiken bleiben nachvollziehbar zugeordnet

Die R-Nummern der älteren Berichte waren nicht identisch belegt. Deshalb werden sie hier mit ihrem Ursprungsbericht bezeichnet; es wird kein Risiko durch bloße Umnummerierung für erledigt erklärt.

| Ursprungsbericht / Risiko | Aktueller Status |
|---|---|
|Letzter `CODEX-FINAL-AUDIT`, R01: Zwei-Atomic-Eintritt|**PASS für diesen Einwand:** jetzt Ein-Atomic-Zulassung plus Produktionsklassentest. Separater Latch bleibt N07 FAIL.|
|Letzter `CODEX-FINAL-AUDIT`, R02: Restore-Stummzeit|**TEILWEISE/OFFEN:** Vorbereitung weiterhin im Busyfenster, keine Maximalzeit-/Dropoutmessung.|
|`TECHNISCHER-AUDIT`, R01: gesamtes Kit nicht transaktional|**TEILWEISE:** Gate und Release/Acquire-Veröffentlichung substanziell verbessert; komplette Auswahlkonsistenz wegen N07 nicht gegeben, generationseindeutiger Audiooracle fehlt.|
|`TECHNISCHER-AUDIT`, R02: Echtzeitfreigabe|**OFFEN:** keine archivierten Corezahlen und keine Paced-/End-to-End-Qualifikation.|
|`TECHNISCHER-AUDIT`, R03: Überläufe/Choke|**TEILWEISE:** konkreter Chokefehler N09 geschlossen; Produktdiagnostik und Lastpolicy unvollständig.|
|`TECHNISCHER-AUDIT`, R04: Import-/Pitch-UI/RAM|**OFFEN:** weiter synchron auf Control; Peak-RAM und Reaktionszeit ungeklärt.|
|`TECHNISCHER-AUDIT`, R05: FLAC-Kurzdekodierung|**PASS für bekannte deklarierte Länge via N03; TEILWEISE insgesamt:** unbekannte Länge und breiter Korruptionskorpus fehlen.|
|`TECHNISCHER-AUDIT`, R06: Transport/Tails/Mix|**TEILWEISE:** N08 geschlossen; Loop/Seek und gewünschte Tail-/Mixpolicy weiter abzugrenzen.|
|`TECHNISCHER-AUDIT`, R07: Multi-Out-Hostmatrix|**TEILWEISE:** alle15AUX mit Pegeln, Isolationstoleranz und Hostrekonfiguration unvollständig.|
|`TECHNISCHER-AUDIT`, R08: Robustheitsgrenzen|**OFFEN/TEILWEISE:** Headroom/Subnormal, Speicherdruck, tatsächlicher GUI-/Hostlifecycle und vollständige Raten-/Blockmatrix nicht erledigt.|

## 10. Priorisierte Empfehlungen und objektives GUI-Startgate

1. **P1 N07:** Latch-/Auswahlentscheidung wirklich linearisierbar machen. Den genau dokumentierten Lost-Update-Fall deterministisch regressieren; danach echte überlappende Paketautomation mit eindeutigem letzten Hostwert, State und Audio.
2. **P2 N10:** Host-ContentScale vor/nach Attach samt veröffentlichtem Rect und Ablehnung konsistent behandeln. N02 zusätzlich mit tatsächlicher Transformation, gespeichertem150%-Reopen und ablehnendem nativen Host absichern.
3. **P1/P2 Kit-/Echtzeitvertrag:** Kandidatenvorbereitung, maximale Stummzeit, MIDI-/Eventpolicy und Wiederanlauf definieren und messen. Verschiedene generationseindeutige Kits, keine reine All-zero-/Finiteprüfung.
4. **P2 GUI/Host:** Produktionspfade für Dirty, Add/Hide-Rollback, abgelehntes16-Row-Recallresize, offene Editorstateänderung, alle Ctrlresets und echte Drops nativ prüfen.
5. **P2 Qualität/Kompatibilität:** Grenz- und Audioautomation, DMR1–3-Controllerrecall, vollständige Multi-Out-Isolation, FLAC-/WAV-Ressourcengrenzen und SRC-/Realtime-Messartefakte ergänzen.
6. **V1-Umfang:** Offene Draftfunktionen ausdrücklich entscheiden. Eine neue Skinimplementierung darf keine fehlenden Funktionsverträge oder ungelösten Audiozustände verdecken.

**Startgate:** Keine offenen P1-Funktions-/Synchronisationsfehler; geschlossene P2-Lifecyclefehler mit geeigneter Regression; echte100/150%-Kontrollmatrix; nachvollziehbare Projekt-/Automationskompatibilität; generationseindeutiger16-Row-/Multi-Out-/Kit-Wiederanlauf; zugängliche und ausreichend breite Audio-/Realtimeevidenz. Ownerabnahme und späterer Releasegate bleiben getrennt. Ein Windows-Build/Validator allein erfüllt dieses Gate nicht.

## 11. Konkrete noch ausstehende Studio-One- und native Abschlussfälle

Diese Tests sind **OFFEN**. Die automatisierbaren Fehlerpfade gehören zuerst in den Windows-Paketharness; sie sind nicht ausschließlich durch manuelles Studio-One-Testen prüfbar.

1. **N07 synchronisiert:** Tatsächliches `setState` plus zwei aufeinanderfolgende BusyflushesH/G; letzten Latchstore gezielt verschachteln. Ohne erneuten Event State und anschließende hörbare Patternauswahl messen. Alle Grenzeintritts-/Freigabephasen und folgende Restores einschließen.
2. **HiDPI/Zoom:** ContentScale1/1,5/2 vor `setFrame`, vor/nach Attach und beim Monitorwechsel; jeweiliges `getSize` protokollieren. Gespeicherte150 % öffnen, beide Zoomrichtungen akzeptieren/ablehnen, echten Transform und Hostrect gegen Präferenz/Markierung prüfen; danach Add/Hide.
3. **Jedes GUIelement:**1/4/5/8/16Rows,100/150 %, jeden Step jeder sichtbaren Row, Shiftvelocity, alle A–H, VOL/PAN/Pitch beide Hälften plus Ctrl, Swing/Human, M/S/Choke, Load/Altclear, Add/Hide samt Sperren. Ränder/Lücken negativ prüfen, Aktionen gegen hörbare Wirkung und Save/Reopen protokollieren.
4. **Kit-/Transportpraxis:** Gegensätzliche16-Row-Kits unter laufendem Transport wechseln, Stimmenmaximum und hohe Ratio, großer und fehlerhafter Kandidat. Stummblöcke, verlorene MIDIevents und Wiederanlauf messen. N08 zusätzlich während Busyphase, Loop/Seek und Stop/Resume; dauerhaft gestoppte MIDI-Audition erhalten.
5. **Automation/Legacy:** Alte Projektkurve an0,10/0,90 sowie allen alten Schwellen und ihren Nachbarn; Hosttext, Plainwert, GUI, State und Audio. Mehrere Punkte pro Block, Loop, Offlinebounce, Nullframeflush und Songreopen. DMR1–DM10 aus vorher modern verstelltem Controllerzustand laden.
6. **Import/Erhalt:** WAV PCM8/16/24/32, Float32/Extensible und FLAC mono/stereo; unbekannte Länge, falsche bekannte Länge, beschädigte End-/Mehrframes, nahe Ausgabe-/Datei-/Kitgrenze. Fehlversuch muss bisherigen Klang, Name und Steps bewahren. Pickerabbruch, Explorer und interner Browserdrop mit Unicode/langer Datei bei beiden Zooms getrennt prüfen.
7. **Multi-Out/Gates:** Alle15AUX, MAIN aus, AUX15 allein, Lücken, Reaktivierung und Songrecall. Einzelrows oder orthogonale Signaturen verwenden. Kein Übersprechen und keine MAIN-Doppelung; Mute/Solo-Mischungen und versteckter Solo-Recall.64Voice-Choke auch mit mehreren Gruppen/MIDI.
8. **Zeit/RAM/Lifecycle:** Zielhardware mit Windows-/Studio-One-Version, CPU, Energieprofil, ASIO und Paketcommit/hash dokumentieren.44,1/48/96/192kHz und32/64/128/256/1024Frames;64 lange Stereovoices, mehrere Instanzen und UI-/Import-/Automationsstress. mean/p95/p99/max, Overruns, Dropouts, Callbackallokationen, Triggerdrops und RAMverlauf archivieren. Wiederholtes echtes Attach/Close/Reopen, Deactivate/Activate, Ratenwechsel und offene Editorstateänderung.
9. **Host Dirty:** Bei jedem nichtautomatisierten Edit Modified/Autosave und Projektwiederherstellung prüfen; abgelehnte Edits sowie Zoom-only-Änderung getrennt. Genau eine installierte Pluginversion verwenden.

## 12. Arbeitsprotokoll

Der tatsächliche Remote-HEAD wurde mit dem Benutzer-SHA abgeglichen und vor Veröffentlichung erneut geprüft. Historische Berichte und alle späteren Reparaturcommits wurden berücksichtigt. Quellen-/Testdateien wurden ausschließlich gelesen; lokale Analyseergebnisse liegen außerhalb des Repositorycheckouts. Veröffentlicht wird ausschließlich dieser vollständige Bericht mit Dokumentationscommit und CI-Skip, ohne Quell-/Teständerungen, PR oder neu gestarteten Build.

Der Bericht ersetzt die vorherige Bewertung des Entwicklungscommits32f68ca. Die damaligen Befunde werden weder blind fortgeschrieben noch ohne konkreten Implementierungs-/Testnachweis geschlossen. Eine vollständige praktische Pluginfreigabe wird nicht aus einem Quellenreview abgeleitet. Die verbindliche Entscheidung zum aktuellen Stand lautet **NEIN**.

## Quellen

[original]: https://github.com/challanger2000/125A-Drumator/blob/491f45ebcb3028e607897cf838f5b9e5eba702da/TECHNISCHER-AUDIT.md
[first-final]: https://github.com/challanger2000/125A-Drumator/blob/04e8b285afe0b5b151ef9d4d275201ea0fc144ff/CODEX-FINAL-AUDIT.md
[previous]: https://github.com/challanger2000/125A-Drumator/blob/07253bf258cbb0a30f80ae5c1d08b7c3aec60b20/CODEX-FINAL-AUDIT.md
[run]: https://github.com/challanger2000/125A-Drumator/actions/runs/39113340712
[job]: https://github.com/challanger2000/125A-Drumator/actions/runs/39113340712/job/117633926278
[sdk-scale-contract]: https://github.com/steinbergmedia/vst3_pluginterfaces/blob/4f547e8e102b47de4a8b8aaf343c73b700786372/gui/iplugviewcontentscalesupport.h#L36
[sdk-scale]: https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/plugin-bindings/vst3editor.cpp#L588
[sdk-getsize]: https://github.com/steinbergmedia/vst3_public_sdk/blob/586dc5e6c8012c3e4b01c79389375cbe96bdb1da/source/common/pluginview.cpp#L69

[C:25]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L25
[C:39]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L39
[C:48]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L48
[C:54]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L54
[C:67]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L67
[C:74]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L74
[C:126]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L126
[C:136]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L136
[C:151]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L151
[C:160]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L160
[C:169]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L169
[C:171]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L171
[C:180]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L180
[C:208]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L208
[C:230]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L230
[D:9]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/KitRestoreGate.h#L9
[D:15]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/KitRestoreGate.h#L15
[D:21]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/KitRestoreGate.h#L21
[E:8]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorEditor.h#L8
[E:14]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorEditor.h#L14
[E:18]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorEditor.h#L18
[F:27]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/FlacImport.cpp#L27
[F:36]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/FlacImport.cpp#L36
[F:49]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/FlacImport.cpp#L49
[F:57]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/FlacImport.cpp#L57
[G:6]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/PatternQuantization.h#L6
[H:115]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L115
[H:127]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L127
[H:777]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L777
[H:817]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L817
[H:851]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L851
[H:882]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L882
[H:928]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L928
[H:951]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L951
[H:1001]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L1001
[H:1010]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L1010
[H:1056]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L1056
[H:1094]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/PackagedAudioHostSmoke.cpp#L1094
[I:14]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/PatternRestoreLatch.h#L14
[I:19]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/PatternRestoreLatch.h#L19
[K:35]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/SampleVoiceEngine.h#L35
[K:263]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/SampleVoiceEngine.h#L263
[P:339]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L339
[P:348]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L348
[P:367]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L367
[P:389]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L389
[P:398]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L398
[P:400]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L400
[P:406]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L406
[P:513]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L513
[P:582]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L582
[P:604]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L604
[P:607]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L607
[P:609]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L609
[P:632]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorProcessor.cpp#L632
[Q:45]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/EditorGeometrySmoke.cpp#L45
[R:36]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L36
[R:51]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L51
[R:57]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L57
[R:70]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L70
[R:77]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L77
[R:104]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/VisibleRowsTests.cpp#L104
[S:169]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/SampleExchange.h#L169
[S:195]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/core/SampleExchange.h#L195
[T:33]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/SampleVoiceTests.cpp#L33
[T:252]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/SampleVoiceTests.cpp#L252
[T:276]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/SampleVoiceTests.cpp#L276
[T:332]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/tests/SampleVoiceTests.cpp#L332
[V:65]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L65
[V:88]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L88
[V:96]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L96
[V:100]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L100
[V:108]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L108
[V:177]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L177
[V:277]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L277
[V:307]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L307
[V:314]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L314
[V:323]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L323
[V:348]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L348
[V:351]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L351
[V:362]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L362
[V:376]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L376
[V:384]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L384
[V:388]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorView.cpp#L388
[W:36]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/.github/workflows/core-ci.yml#L36
[W:110]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/.github/workflows/core-ci.yml#L110

[C:121]: https://github.com/challanger2000/125A-Drumator/blob/42f468aac2e9d84d6e4f563107fd3871eb3f8d1f/src/vst3/DrumatorController.cpp#L121

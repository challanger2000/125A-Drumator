# 125A Drumator – unabhängiger technischer Audit, 19.10.2026

## 1. Urteil und verbindlicher Prüfstand

**Ist die technische Grundlage für eine abschließende Vintage-GUI-Integration und V1-Freigabe ausreichend verifiziert? NEIN.**

Der aktuelle Stand besitzt eine deutlich verbesserte technische Grundlage, sinnvolle Ownership für Samples und echte paketierte Audio-Regressionen. Die jüngsten Reparaturen sind überwiegend im tatsächlichen Quellcode nachvollziehbar. Eine vollständige Funktionsfreigabe folgt daraus nicht: Dieser Audit findet **neun konkrete verbleibende Befunde: zweimal HOCH, sechsmal MITTEL, einmal NIEDRIG**. Hinzu kommen ausdrücklich getrennte Konkurrenz-, Ressourcen- und Echtzeitrisiken sowie fehlende native GUI-/Studio-One-Prüfungen. Ein nachgewiesener kritischer Speicherfehler oder DAW-Absturz wurde in diesem Audit nicht festgestellt; die fehlenden Laufzeitprüfungen erlauben ebenso wenig eine allgemeine Absturzfreiheitszusage.

Die grafische Gestaltung durch den anderen Chat kann als eigenständiger Entwurf weitergehen. Die funktionale Bedienarchitektur sollte vor ihrer endgültigen grafischen Festlegung die unten genannten Korrekturen und nativen Regressionen erhalten. Der Bericht beauftragt und implementiert keine Gestaltung.

| Identität | Unabhängig abgeglichener Wert |
|---|---|
| Repository | `challanger2000/125A-Drumator` |
| Geprüfter Entwicklungsbranch | `develop/v1.0.0` |
| **Geprüfter Commit** | **`ee712dd094e28fdc9bd6da9917f249aa7ef7b185`** |
| Letzter als Windows-getestet gemeldeter Produktionscommit | `b6c40d0c918f92c0d63e343dc72caa0793528fb5` |
| `src`-Git-Tree, in beiden Commits identisch | `512254b930741133e01285596acf61c1c1f5dbc7` |
| `tests`-Git-Tree, in beiden Commits identisch | `7cd0a62728e119a635f601f0f25013ef096f2da6` |
| Prüfdatum | 2026-10-19 |
| Plugin-Metadatenversion | `0.1.0` in `CMakeLists.txt`; „V1“ ist hier Entwicklungsziel, keine veröffentlichte Version |
| Berichtsziel | bestehender Branch `audit/codex-drumator-2026-10-19`, ausschließlich diese Markdown-Datei |
| Engineering-Regeln | `125A-Engineering`, `2eb41eadea88c32d774e918ee2d71756f81a4459`, einschließlich `START-HERE.md`, Guardrails und einschlägiger Standards/QA |
| SDK | Steinberg VST3 SDK `v3.8.1_build_84`, `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` |
| Zugehöriges VSTGUI | `5db272256172557818b6158cf0bb2c4410bddb25` |
| Funktionierende D&D-Referenz | Phraseator-Final `b843d5e1756bb4fd89053633fa546cdcf05e5a65` |

`ee712dd` verändert gegenüber `b6c40d0` ausschließlich `AGENTS.md`, `README.md`, den Selbst-Audit und den V1-Funktionsaudit. Deshalb wird exakt derselbe Produkt-/Testquellcode untersucht. Es wird daraus **keine** Identität mit einer lokal installierten Besitzer-DLL oder einer freigegebenen FULL-Veröffentlichung abgeleitet. Der Auftrag ist eine Prüfung des ausdrücklich benannten Entwicklungsstands, keine Implementierung auf einer vermuteten Releasebasis. Die Engineering-Regel zur unabhängigen Release-Provenienz bleibt ein späteres Freigabegate.

Alle Quelllinks und Zeilenangaben dieses Berichts sind auf **`ee712dd` festgeschrieben**, nicht auf einen später veränderlichen Entwicklungsbranch.

## 2. Methode, Status und tatsächliche Beweislage

Gelesen und gegengeprüft wurden `AGENTS.md`, der vollständige Selbst-Audit einschließlich seiner Aktualisierung, `TECHNISCHER-AUDIT.md`, `CODEX-FINAL-AUDIT.md`, spätere Reparaturpfade und der aktuelle Bestand: **37 Dateien unter `src`, 14 unter `tests` sowie CMake, SDK-Patch, Ressourcen und Workflow; zusammen 56 Implementierungs-/Test-/Konfigurationsdateien**. Ältere Aussagen wurden als Hinweise behandelt, nicht als Beweise übernommen. Die Prüfung umfasst Definitionen und vollständige Aufrufketten zwischen View, Controller, Nachrichten, Prozessor und Core. Eine Bestandsliste steht in Abschnitt 13.

**Statusdefinitionen:**

- **PASS:** ausschließlich für einen klar eingegrenzten, durch den benannten Nachweis bestätigten Sachverhalt. Kein pauschaler Plugin-PASS.
- **FAIL:** ein konkreter Fehlerpfad ist aus der tatsächlichen Implementierung nachvollziehbar nachgewiesen. „Quellcode-bestätigt“ bedeutet keine heimlich behauptete Studio-One-Reproduktion.
- **TEILWEISE:** Reparatur/Implementierung vorhanden, aber die produktnahe Verifikation deckt den Gesamtvertrag nicht ab.
- **OFFEN:** ohne zusätzlichen Laufzeit-, Ressourcen- oder Hosttest nicht abschließend bewertbar. Risiken erhalten keine erfundenen Fehlerreproduktionen.

### 2.1 Was in diesem unabhängigen Audit tatsächlich ausgeführt wurde

1. Git-Identität, Remote-Branch-SHAs, Baumidentität und Dokumentationsdiff wurden abgeglichen. Für die untersuchten Dateien wurde ein SHA-256-Inventar erzeugt.
2. Zusätzliche **Python-Quellguards und arithmetische Ablaufmodelle** prüften die Gegenbeispiele A01–A09 und das Risiko R01 gegen die festgeschriebenen Quellstellen. Deren konkrete Ergebnisse stehen bei den Befunden und in Abschnitt 12. Diese Hilfsmodelle sind **keine Ausführung des C++-Plugins**, keine neuen Produktregressionen und kein Windows-PASS.
3. Der gepinnte VSTGUI-Quellcode wurde insbesondere für Texteditor-Fokus, Skalierung und Windows-Dateidaten abgeglichen. Der native D&D-CMake-Patch ist mit der funktionierenden Phraseator-Referenz bytegleich.
4. Die öffentliche [Actions-Laufseite #39324490951](https://github.com/challanger2000/125A-Drumator/actions/runs/39324490951) wurde geprüft: **Success**, passender Produktionscommit `b6c40d0`, Windows-Job und ein Artefakt. Die Jobseite verlangt für vollständige Logs eine Anmeldung. **13/13 CTests, 47/47 Validator und paketierte Smokes sind damit vorhandene, im Projekt/Benutzerauftrag gemeldete Ergebnisse; deren vollständige Einzeltranskripte wurden in diesem Audit nicht unabhängig erneut gelesen.**
5. Die verfügbare Maschine ist Linux. Gemäß Windows-only-Projektregel wurden **kein Linux-Plugin, keine C++-Tests und kein VST3-Bundle gebaut**. Kein Windows-/Studio-One-Prozess, keine native GUI, kein Audiogerät und keine neuen Actions wurden ausgeführt. Ein aktuelles Bundle wurde hier nicht installiert oder bitgenau auditiert.

### 2.2 Was „13/13 Tests“ tatsächlich bedeutet

CMake registriert **zwölf reguläre CTests und einen absichtlich fehlschlagenden SongChain-Negativtest** mit `WILL_FAIL`. Der Negativtest ist eine sinnvolle Prüfung aktiver Release-Assertions; er ist keine dreizehnte unabhängige Produktfunktion. Editor- und Audio-Host-Smoke sind zusätzliche Workflow-Aufrufe. Weder deren Existenz noch der grüne Workflow ersetzt einen angehängten GUI-Test oder Studio-One-Abnahme.

## 3. Priorisierte verbleibende Fehler

Die folgenden neun Befunde sind am aktuellen Quellstand bestätigt. Bei A07 ist zusätzlich der Bedienvertrag einer zusammenhängenden Drag-Geste ausdrücklich benannt. A08 ist eine verbliebene Teilfrage des bereits früher erwähnten Zoom-Recalls, kein neu erfundener historischer Fehler. Native Reproduktionsläufe stehen für diese Befunde noch aus.

| ID | Priorität | Status | Auswirkung |
|---|---|---|---|
| A01 | **HOCH** | **FAIL, Quellcode** | MIDI-Patternauswahl und GUI-/Host-Auswahl laufen auseinander; Bearbeitung trifft ein anderes Pattern als das hörbare. |
| A02 | **HOCH** | **FAIL, Quellcode + Ablaufmodell** | Der 257. gültige Patternpunkt verwirft sämtliche Patternänderungen des Blocks. |
| A03 | MITTEL | FAIL, Quellcode + Rechnung | Strg+Master setzt auf 0 statt auf Neutral-/Defaultpegel 100. |
| A04 | MITTEL | FAIL, Produkt- und SDK-Quellcode | Namenseditor wird bei Escape/unverändertem Enter nicht geschlossen. |
| A05 | MITTEL | FAIL, Quellcode + Geometrie | Drop auf Song-/Fill-Overlay ersetzt ein verdecktes Instrumentensample. |
| A06 | MITTEL | FAIL, bedingter Resizepfad + Geometrie | Compact-Fallback berücksichtigt die tatsächliche Resthöhe nicht; Lane/Status werden abgeschnitten. |
| A07 | MITTEL | FAIL, Gesten-/Undo-Vertrag | Ein Undo nach Velocity-Drag nimmt nur das letzte Mausereignis zurück. |
| A08 | MITTEL | FAIL, offener Editor beim Controller-Recall | Zoompräferenz wird geladen, bestehender Editor behält den alten Zoom. |
| A09 | NIEDRIG | FAIL, Quellcode + Geometrie | Leere Abstände zwischen Song-Auswahltasten verändern Pattern/Länge. |

### A01 – MIDI-Patternwechsel wird nicht zum Controller/Host zurückgeführt

**Dateien/Zeilen:** [P:552](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L552), [P:668](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L668), [C:206](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L206), [V:348](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L348), [V:455](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L455), [V:965](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L965).

**Ursache:** Der Prozessor übernimmt MIDI-Noten 24–31 in seine zeitlich sortierten Patternänderungen und schreibt am Blockende `selected_`. Der Controller bestimmt `currentPattern()` dagegen aus seinem VST3-Parameter `kPatternParam`/1000. Im gesamten aktuellen Prozessor wird `ProcessData::outputParameterChanges` nicht verwendet; ein anderer Prozessor→Controller-Rückkanal für diese MIDI-Auswahl ist ebenfalls nicht vorhanden. Die sichtbaren A–H-Zustände und die Step-Bearbeitung richten sich nach dem Controllerwert.

**Konkretes Fehlerszenario:** Controller/GUI zeigen A. MIDI-Note 25 wählt im Prozessor B. B wird gespielt und als Prozessorauswahl gespeichert; GUI und Hostparameter bleiben auf A. Ein anschließender Step-Klick bearbeitet A, während B hörbar ist. Ein Host, der gar keine Rückmeldung erhält, kann diese Divergenz nicht zuverlässig selbst korrigieren. Im Songmodus ist zwischen bewusst eigenständigem Song-Playhead und editierbarem Pattern zu unterscheiden; dieser Befund betrifft den ausdrücklich vorgesehenen MIDI-A–H-Wechsel im Patternmodus.

**Nachweis:** Getrennte Auswahlquellen und fehlender Rückkanal sind im Produkt nachgewiesen. Der Ablauf ergibt `Prozessor=1/B`, `Controller=0/A`. Der paketierte MIDI-Test [Host:1122](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L1122) prüft Audio/Offsets, nicht übereinstimmende Controller-, Host- und GUI-Auswahl. Kein nativer Mauslauf wurde behauptet.

**Empfohlene Reparatur:** Prozessorseitige MIDI-Auswahl mit korrektem Sampleoffset über die vorgesehenen VST3-Ausgabeparameter melden und deren Controller-/View-Aktualisierung im verbundenen Hostpfad sicherstellen. Falls zusätzlich ein asynchroner UI-Kanal nötig ist, ohne UI-Zugriff, `beginEdit` oder blockierende Nachrichten im Audiothread. Gleichzeitige Hostautomation muss die bereits festgelegte Priorität behalten; keine Rückkopplungsschleife erzeugen.

**Erforderliche Regression:** Tatsächliches verbundenes Controller-/Prozessorpaar mit Host-Feedback, verschiedene hörbare A/B-Patterns, MIDI 24–31 bei Anfang/Mitte/Ende eines Blocks und gleichen Offsets wie Hostpunkte. Audio, gemeldeter Parameter, A–H-Markierung, nächster Step-Edit und gespeicherte Auswahl müssen zusammenpassen. Editor geöffnet und geschlossen prüfen.

### A02 – Patternereignis-Overflow löscht den gesamten Block

**Dateien/Zeilen:** [P:531](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L531), besonders [P:537](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L537), Auswertung [P:668](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L668).

**Ursache:** `patternEdits` hat 256 Plätze. Beim nächsten gültigen Punkt setzt `addPattern()` **`editCount=0; editOverflow=true`**. Damit gehen die bereits gesammelten Punkte verloren, weitere Punkte werden ignoriert. Hostautomation und MIDI teilen sich diese Kapazität. Die feste Kapazität verhindert Allokationen, die gewählte Überlaufreaktion zerstört aber auch den bereits bekannten Endzustand.

**Konkretes Fehlerszenario:** 512 Frames, anfangs A; 257 gültige Hostpunkte an Offsets 0–256 wählen B. Der 257. Punkt leert die Liste. Statt B bleibt A gewählt und hörbar. Eine andere Blockpartitionierung derselben Ereignisse kann funktionieren, weil sie den Grenzwert pro Block nicht überschreitet. Selbst viele identische gültige Punkte lösen den Fehler aus.

**Nachweis:** Ausführung des nachgebildeten Kontrollflusses ergibt `valid=257`, `retained=0`, `expected=B`, `actual=A`. Das ist ein aus dem Produkt abgeleitetes Gegenbeispiel, kein hier ausgeführter VST3-Lastlauf. In den vorhandenen Tests fehlt die Grenze 255/256/257 an diesem Produktpfad.

**Empfohlene Reparatur:** Eine begrenzte, dokumentierte Overflowstrategie verwenden, die zumindest den autoritativen Endzustand erhält und den bereits gültigen Verlauf nicht vollständig verwirft. Vorzugsweise vorhandene sortierte Host-/MIDI-Quellen ohne verlustreiche Gesamtliste zusammenführen; erforderliche Zeitordnung und Gleichoffset-Priorität beibehalten. Keine dynamische Erweiterung oder unbeschränkte Wiederholungen im Callback einführen.

**Erforderliche Regression:** Echte `IParameterChanges`/`IEventList` mit 255, 256, 257 und mehr Punkten; MIDI/Host gemischt, identische und wechselnde Werte, gleiche Offsets, finale gespeicherte Auswahl und Audiosignatur. Dasselbe Ereignisband mit unterschiedlichen Blockgrößen muss dieselbe musikalische Auswahl ergeben. Callback-Allokationen instrumentieren.

### A03 – Strg-Reset am Master schaltet den Ausgang stumm

**Dateien/Zeilen:** [V:699](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L699), speziell [V:700](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L700); Default [Kit:62](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/KitState.h#L62), Audioanwendung [P:738](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L738).

**Ursache:** Für Strg+Klick berechnet die View `amount = -masterVolume()`. `adjustMasterVolume()` addiert diesen Betrag auf den aktuellen Wert: Ergebnis immer 0. Der gespeicherte und initiale Master-Default ist dagegen **100**, entsprechend unverändertem Pegel. Dieser Reset unterscheidet sich unbeabsichtigt vom neutralen Lautstärkereset der Instrumente.

**Fehlerszenario/Nachweis:** Strg+Master-Minus oder -Plus bei 0/50/100/200 ergibt jeweils 0. Bei 100 wird statt Rückkehr zum Neutralwert sämtliches Audio auf MAIN und AUX stumm. Dieser Rechenpfad wurde unabhängig nachvollzogen; Header-Hitbox-Tests prüfen nur Positionen, der Host-Mastertest [Host:249](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L249) umgeht den Mausdispatcher.

**Empfohlene Reparatur:** Den definierten Master-Default gemeinsam verwenden und `default-current` statt `-current` übergeben; Host-Dirty-/Recall-Verhalten erhalten. Soll bewusst „Strg=Stumm“ gelten, müsste das als abweichender Bedienvertrag explizit eingeführt werden; der vorhandene Reset-/Defaultvertrag spricht dafür nicht.

**Regression:** Strg+Links auf beide sichtbaren Mastertasten bei vier Startwerten, 100/150 %, anschließende Audio- und Stateprüfung. Normale ±5-Schritte und Grenzwerte mitprüfen.

### A04 – Escape/Enter ohne Namensänderung lässt das Textfeld über der Samplefläche stehen

**Dateien/Zeilen:** [V:271](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L271), [V:285](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L285), Lifecycle [V:263](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L263). SDK: `vstgui/lib/controls/ctextedit.cpp:247–259,378–418` am oben genannten VSTGUI-Commit.

**Ursache:** Das dynamische `CTextEdit` wird als Frame-Child angelegt. Verbergen und Rücksetzen von `editingNameRow_` geschehen ausschließlich in `valueChanged()`. VSTGUI ruft diese Änderungsbenachrichtigung bei Fokusverlust nur auf, wenn der neue Text vom bisherigen abweicht. Escape stellt den alten Text wieder her; unverändertes Enter verursacht ebenfalls keine Änderung. Für `onTextEditPlatformControlLostFocus` beziehungsweise `kMsgLooseFocus` ist in der Produktview kein Abschlussweg registriert.

**Fehlerszenario:** Shift+Name öffnet das Feld. Escape oder Enter ohne Textänderung entfernt das native Eingabefeld, lässt jedoch das darüberliegende sichtbare/mausempfindliche `CTextEdit`-View und den Editierstatus stehen. Ein normaler Klick auf den Namen erreicht dann nicht zuverlässig wieder den vorgesehenen Sample-Ladedialog. Bei späterem Relayout kann die alte Overlayposition zusätzlich unpassend sein. Ein Absturz wird daraus nicht behauptet.

**Nachweis:** Der Produktpfad und der Änderungsvertrag des exakt gepinnten SDK wurden gegengeprüft. Der Abschluss ohne Textänderung fehlt tatsächlich. Die bestehenden Namen-/Stateprüfungen testen keinen nativen Fokuszyklus.

**Empfohlene Reparatur:** Fokusabschluss separat vom Speichern behandeln, etwa über den vorgesehenen `ITextEditListener`. Overlay und Editierzeile auf jedem Ende, Escape und Abbruch zurücksetzen; Namensänderung nur bei akzeptierter tatsächlicher Änderung senden. Recall/Relayout/Editorentfernung konsistent behandeln.

**Regression:** Shift+Name → Escape; unverändertes Enter; geändertes Enter; Klick außerhalb; Recall, Paging und Editor schließen während der Bearbeitung. Danach muss ein normaler Namensklick wieder laden. Abbruch darf keinen Dirty-Edit oder falsche Zeilenumbenennung auslösen.

### A05 – D&D ignoriert verdeckende Song-/Fill-Oberflächen

**Dateien/Zeilen:** [V:237](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L237), [V:245](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L245), [V:635](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L635), Songdarstellung [V:565](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L565), Mausvorrang [V:713](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L713).

**Ursache:** Die Drop-Annahme berechnet die Instrumentenzeile nur aus der Gridposition. Sie berücksichtigt weder `songEditorOpen_` noch `fillMenuRow_`. Der Songdialog überzeichnet die Instrumentenmatrix und fängt normale Mausklicks ab; Datei-Drops werden weiterhin an die unsichtbare Matrix darunter weitergeleitet. Das gleiche Problem betrifft den vom Fill-Popup verdeckten Teilbereich. DragEnter/Move signalisieren außerdem Copy allein aufgrund eines erkannten Dateipfads, unabhängig vom tatsächlich sichtbaren Ziel.

**Fehlerszenario/Nachweis:** Bei vier Instrumenten Song-Editor öffnen, gültiges WAV/FLAC auf `(700,150)` ablegen. Der Punkt liegt auf der Songoberfläche, wird aber als `floor((150-108)/29)=1` interpretiert: **Instrument 2 wird ersetzt**. Das folgt direkt aus Zeichnung und akzeptiertem Dropbereich. Die Windows-Dateipfad-Reparatur selbst ist korrekt vorhanden; dieser Fehler ist eine zusätzliche Zielzuordnungsfrage.

**Empfohlene Reparatur:** Eine gemeinsame Sichtbarkeits-/Zielprüfung für Enter, Move und Drop verwenden. Bei geschlossenem Instrumentenbereich keinen Drop akzeptieren; alternativ ein klar sichtbares, explizites Sampleziel anbieten. Auf verdeckten Fill-Bereichen ebenfalls keine Hintergrundzeile ändern. Den bewährten Phraseator-/Win32-Adapter unverändert weiterverwenden.

**Regression:** Tatsächlicher Drop bei Song offen, Fill offen, normaler Matrix, Compact-Paging und beiden Zoomstufen. Bei verdecktem Ziel `None/false` und unveränderte Samples, Statebytes und Audio; im normalen Ziel genau die sichtbare Zeile ändern. Fehlgeschlagener Import erhält das vorherige Sample.

### A06 – Compact-Fallback kann Lane und Fehlermeldung abschneiden

**Dateien/Zeilen:** [V:305](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L305), [Compact:7](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/CompactRowViewport.h#L7), Lane [V:520](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L520), Status [V:343](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L343).

**Ursache:** Bei Host-Resize-Ablehnung bleibt die alte Viewhöhe bestehen. Der Fallback reduziert die Instrumentendarstellung aber starr auf höchstens vier Zeilen. Er berechnet nicht, wie viele Zeilen samt offenem Footer in die **tatsächlich verbleibende Höhe** passen. Die Statusmeldung wird unter dem berechneten Grid statt in einem garantierten sichtbaren Bereich gezeichnet.

**Konkretes Gegenbeispiel:** Ein zulässiger Zustand mit einem Instrument und offener Lane hat `108+29+220=357` logische Pixel. Nach Recall eines 16-Instrumente-Kits lehnt der Host die benötigten 792 ab. Compact zeigt vier Zeilen: Gridende 224, Lane-Unterkante **389**, Statusbeginn **412**. Bei verbleibenden 357 Pixeln fehlen mindestens **32 Pixel** der Lane; die Ablehnungs-/Fehlermeldung ist ebenfalls außerhalb. Auch die initiale 300-Pixel-Preattachhöhe genügt nach verweigertem ersten Resize nicht automatisch dem Vierzeilen-Footer. Die auf 850 korrigierte UIDESC-Obergrenze behebt diesen separaten Ablehnungsfall nicht.

**Nachweis:** Produktformeln und unabhängige Geometrierechnung, kein ausgeführter Studio-One-Resize. Zeilen-Paging existiert und wird anerkannt; der Befund lautet nicht mehr „kein Fallback vorhanden“.

**Empfohlene Reparatur:** Sichtbare Zeilenzahl und Footer aus der realen logischen Clienthöhe berechnen oder getrennt scrollen; Status/Fehlermeldung und wesentliche Aktionen erreichbar halten. Das Wechselspiel von Lane, minimaler Größe, Zoom und Host-DPI als einen Layoutvertrag behandeln.

**Regression:** Angehangener Editor mit kontrolliert ablehnendem `IPlugFrame`: 1→16 Instrumente, Lane auf/zu, 100/150 %, zusätzlich DPI-Faktoren; alle sichtbaren Bounds innerhalb der Clientfläche. Kein bloßer Integer-Rollbacktest.

### A07 – Velocity-Drag überschreibt seine eigene einstufige Undo-Historie

**Dateien/Zeilen:** [V:814](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L814), [V:976](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L976), [V:994](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L994), [C:444](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L444), [C:450](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L450), [C:349](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L349), [Undo:9](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/PatternUndo.h#L9).

**Ursache:** Sowohl MouseDown als auch jeder geänderte MouseMove ruft `setStepVelocity()` auf. Dieser Pfad speichert bei jedem akzeptierten Zwischenwert den gesamten vorigen Patternstand in denselben einzigen Undo-Platz. MouseUp/Cancel schließen keine Undo-Transaktion ab.

**Fehlerszenario/Nachweis:** Ein zusammenhängender Drag bewegt einen Wert von 20 über 40 auf 80. Nach Loslassen stellt ein Undo **40**, nicht den Zustand 20 vor der Geste her; anschließend wird die Historie verbraucht. Beim Streichen über mehrere Steps bleibt der größte Teil der Geste bestehen. Diese Zwischenwertfolge wurde am tatsächlichen Capture-/Consume-Pfad nachvollzogen. Unter dem üblichen und für eine einstufige Patternaktion sinnvollen Bedienvertrag „ein Drag = eine Aktion“ ist das ein Fehler. Falls absichtlich jedes Mausereignis eine Aktion sein soll, wäre zumindest eine mehrstufige Historie nötig, um den kompletten Stroke zurücknehmen zu können.

**Empfohlene Reparatur:** Vorzustand einmal beim Beginn einer Velocity-Geste aufnehmen und erst die abgeschlossene akzeptierte Geste als einen Undo-Schritt veröffentlichen. No-op, Nachrichtenablehnung und Cancel-Vertrag ausdrücklich behandeln; bestehende per-Pattern-Isolation erhalten.

**Regression:** Echte MouseDown/mehrere MouseMove/MouseUp-Sequenz über einen und mehrere Steps; genau ein Undo liefert den vollständigen Zustand vor der Geste. Abgelehnte Edits oder unveränderte Werte dürfen keine fremde Historie verbrauchen. `PatternUndoTests.cpp` und `VelocityLaneTests.cpp` prüfen bisher nur getrennte Hilfsmodelle.

### A08 – Zoom-Recall aktualisiert den bereits geöffneten Editor nicht

**Dateien/Zeilen:** [C:69](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L69), [C:76](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L76), Editorerzeugung [C:187](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L187), Anzeige [V:365](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L365).

**Ursache:** `Controller::setState()` liest DZM1 und verändert nur `preferredZoom_`. Die Präferenz wird in `createView()` auf einen neuen Editor angewendet; bestehende registrierte Views/Editoren werden nicht benachrichtigt und durchlaufen keine Resize-Transaktion.

**Fehlerszenario/Nachweis:** Editor steht sichtbar auf 100 %. Der Host lädt einen Controllerzustand mit 150 %, während die Oberfläche offen bleibt. `getState()` speichert danach 150 %, die Oberfläche zeigt/benutzt aber weiter 100 %. Erst Neuanlage des Editors verwendet die Präferenz. Die Diskrepanz besteht unabhängig davon, ob ein konkreter Host regelmäßig in genau dieser Reihenfolge recalled. Der bestehende Controllerstate-Test [Host:128](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L128) hat keinen angehängten Editor.

**Empfohlene Reparatur:** Geladene Präferenz auch auf vorhandene Editorinstanzen über den bereits vorgesehenen resizegeprüften Zoompfad anwenden. Bei Hostablehnung tatsächlichen Zoom und gespeicherten Zustand nach dokumentiertem Vertrag konsistent halten; Mehrfach-Views berücksichtigen. Das ist eine verbleibende Teilfrage von F15, keine Rückkehr des alten fehlenden Serialisierungsformats.

**Regression:** Controllerzustand 100→150 und zurück bei geöffnetem Editor, Resize akzeptiert/abgelehnt, anschließend Speichern, Schließen und Wiederöffnen. Präferenz, Frame-Transformation, Buttonzustand und `getSize()` gemeinsam prüfen.

### A09 – Weitere Buttonlücken ändern Werte oder bestätigen eine nicht ausgeführte Aktion

**Dateien/Zeilen:** Songzeichnung [V:591](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L591), Hit-Test [V:731](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L731); Velocity-Footer [V:495](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L495), [V:841](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L841).

**Ursache:** Song-Patternbuttons sind 50 Pixel breit bei 56 Pixel Rasterabstand. Der Hit-Test akzeptiert dagegen den gesamten 56-Pixel-Schritt. Für Songlängen gilt analog 100 Pixel Zeichnung bei 106 Pixel Klickraster.

**Fehlerszenario/Nachweis:** `(167,290)` liegt in der sichtbaren Lücke zwischen A (rechte Kante 165) und B (linke Kante 171). Der Dispatcher berechnet Index 0 und schreibt A in den gewählten Songslot. Stand dieser auf B, ändert der scheinbare Leerklick den Song. Die Lücke 778–784 zwischen den ersten Längentasten wirkt entsprechend. Die zuletzt reparierten vier Headerhitboxen sind davon nicht betroffen und werden nicht fälschlich erneut beanstandet.

**Zusätzlicher bestätigter Leerklick:** Bei `x=1019`, `y=actionsY+10` nimmt [V:841](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L841) den Abstand zwischen „Velocity variieren“ und „Reset100“ an. Es wird keine Controllerfunktion aufgerufen, aber der Status bestätigt dennoch „AKTIVE STEPS: VELOCITY 100“. Ein vorhandener Stepwert ungleich100 bleibt unverändert. Auch diese falsche Erfolgsmeldung entsteht im zentralen Dispatcher, nicht durch ein Hostproblem.

**Empfohlene Reparatur:** Nach Indexermittlung innerhalb der tatsächlich gezeichneten Buttonbreite prüfen; gemeinsame Rechtecke für Zeichnung und Ereignisse verwenden. Dieselbe Entscheidung für die 64 Songzellen und deren Zwischenräume dokumentieren.

**Regression:** Linke/rechte Kante und jeder Gap aller A–H-/Längenbuttons sowie der Velocity-Footerabstand1014…1024, normal/150 % und DPI-transformiert. Bei Leerklick keine falsche Erfolgsmeldung. Lückenklick darf weder Dirty noch Nachricht noch Stateänderung erzeugen. Der vorhandene Headerhitbox-Test deckt den Song-Overlay-Dispatcher nicht ab.

## 4. Selbst-Audit und jüngste Korrekturen: unabhängige Neubewertung

Der Selbst-Audit enthält einen historischen Befundteil und eine vorangestellte Aktualisierung. Historische Aussagen werden nicht unbesehen als aktueller Defekt weitergeführt.

| Selbst-Audit | Ergebnis am aktuellen Quellcode | Urteil / verbleibender Nachweis |
|---|---|---|
| P1-01 SongChain-Assertions in Release entfernt | `tests/SongChainTests.cpp` verwendet jetzt aktive `CHECK`s; zusätzliche Übersetzung mit `DRUMATOR_TEST_EXPECTED_FAILURE`, CTest `WILL_FAIL`. | **Quellcode-Korrektur bestätigt; TEILWEISE** für tatsächlichen frischen MSVC-Lauf hier. Die alte Behauptung „Test ist leer“ gilt am Prüfstand nicht mehr. |
| P1-02 echte Bild-/Skinbindung fehlt | UIDESC enthält keine Runtime-Bitmapbindung; Slot-JSON ist Dokumentation für den anderen Chat. | **PASS nur für die ehrliche Scopebeschreibung**: `skinIntegrationReady:false`. Keine vorgetäuschte fertige Skinintegration; kein Reparaturauftrag für Artwork in diesem Audit. |
| P2-03 UIDESC max650 zu klein | Obergrenze ist nun 850; erforderliche 677/792 liegen darunter. | **Geometrischer Teil korrigiert**, native Resize-Abnahme OFFEN; A06 betrifft einen anderen Restpfad. |
| P2-04 keine angehängte GUI getestet | `EditorGeometrySmoke` prüft Factory/getSize/Preattach-DPI, kein HWND/Interaktionslauf. | **OFFEN**, durch den grünen Build nicht geschlossen. |
| P2-05 gleichwertige Automation gegen Restore | Aktive Klasse ist `AtomicPatternSelection`; historischer Latch wird nicht mehr benutzt. | **OFFEN/R01**, kein ungeprüftes Wiederholen des alten N07-FAIL. |
| P2-06 konkrete Headerlücken | Master± und Songtoggle/-editor verwenden im Dispatcher die echten `UiHeaderHitZones`; separate positive/negative Headerpunkte sind getestet. | **Quellcode-Fix bestätigt; TEILWEISE** für gesamten UI-Vertrag. A09 ist ein anderer Bereich. |
| P2-07 tatsächlicher Host-/Realtimebetrieb | Keine zusätzliche unabhängig ausgeführte native Matrix in diesem Audit. | **OFFEN**, siehe Abschnitte 7, 8, 11. |
| P3-08 Undo selected statt disabled | [V:492](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L492) übergibt nun `selected=false` und separaten `enabled`-Wert. | **Quellcode-Fix bestätigt**; A07 ist Gestenverhalten, nicht dieselbe Darstellungsfrage. |
| P3-09 veraltete Auditnavigation | Neue Dokumentkopfzeilen nennen aktuelle Basis und kennzeichnen alte Meilensteine als Historie. | **Verbessert**, Altberichte bleiben historische Dokumente. Metadaten `0.1.0` sind keine zugesagte V1-Freigabe. |
| P3-10 eigene Warnungen/CI-Hygiene | Unbenutzte Eingabe explizit behandelt; überschattende Rowvariable umbenannt. | **Konkrete Source-Fixes bestätigt**; vollständiges aktuelles MSVC-Nullwarnungsprotokoll nicht selbst eingesehen. Produktziel weiterhin nicht generell `/WX`. |
| P3-11 vorgezogene Gestaltung | Zurückgenommene grafische Arbeit wird nicht als aktuelle Runtime-Skinbindung verkauft. | Auftragstrennung respektieren; keine neue Grafikarbeit vorgenommen. |

**Zusätzliche Korrektur gegenüber früheren Auditnotizen:** Der KitState-Test prüft bei abgelehnter malformed/budgetüberschreitender Dekodierung inzwischen den tatsächlichen Ausgabezustand `again` ([KitT:349](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/KitStateTests.cpp#L349), [KitT:360](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/KitStateTests.cpp#L360)). Eine frühere versehentliche Prüfung einer anderen lokalen Variablen wird deshalb hier nicht als fortbestehender Fehler gemeldet.

## 5. Rückverfolgung ursprünglicher F01–F17, N01–N10 und weiterer Reparaturen

Diese Tabelle bewertet den gesamten ursprünglichen Vertrag konservativ. **„TEILWEISE“ bedeutet häufig: alte Ursache im Code beseitigt, Laufzeitabnahme noch unvollständig.** Das ist weder ein pauschales FAIL sämtlicher Reparaturen noch ein natives PASS ohne Test.

### 5.1 F01–F17

| ID | Status | Unabhängige Feststellung / Rest |
|---|---|---|
| F01 – Editorbreite/Clipping | TEILWEISE | 1280 logische Pixel konsistent; Grid endet bei `622+32×19=1230`, alle 32 Spalten passen rechnerisch. Angehangener Editor, letzte Steps und Mausbereiche nicht nativ geprüft. |
| F02 – falsche/lückenhafte Klickbereiche | TEILWEISE | VOL/PAN/PITCH sind getrennt; Pattern-/Zoom- und zuletzt vier Headerbereiche verbessert. A09 bestätigt weitere falsche Lücken; siehe vollständige Bedienmatrix. |
| F03 – Host→View-Refresh | TEILWEISE | Viewregister und gezieltes `notifyViews` für Controller-Parameter-/Komponentenzustand vorhanden. Native Paint-/Lifecycleprüfung fehlt; MIDI-Rückmeldung A01 ist ein zusätzlicher fehlender Pfad. |
| F04 – Nullframe-/Ausgangslos-Flush | TEILWEISE | [P:485](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L485) verarbeitet Auswahl ohne Audio; Busy-Restore verarbeitet Hostpunkte ebenfalls. Alte vollständige Ignorierung korrigiert. A02 betrifft den normalen Audiozweig mit vielen Punkten. |
| F05 – Host-Dirty | TEILWEISE | Erfolgreiche mutierende Controller-Nachrichten benutzen [C:56](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L56) und `IComponentHandler2::setDirty`; fehlgeschlagene nicht. Hilfs-/Direktnachrichtentests beweisen nicht jeden tatsächlichen GUI→Host-Dirtypfad. Zoompräferenz-Dirtyverhalten separat im Host prüfen. |
| F06 – Legacy-Controllerdefaults | TEILWEISE | Legacy-Komponentenstate setzt Mix-/Namen-/Mute-/Solo-/Ausgangsdefaults zurück; kein unverändert belegter alter Fehler. Frische und bereits benutzte echte Hostinstanz mit Legacyfixtures weiter prüfen. |
| F07 – verstecktes Solo | TEILWEISE | Controller und Prozessor maskieren versteckte Soloflags, Samples bleiben erhalten; [P:854](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L854). Stateprüfung vorhanden, komplette hörbare Hostinteraktion noch nicht unabhängig ausgeführt. |
| F08 – MAIN inaktiv blockiert AUX | TEILWEISE | [P:509](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L509) erlaubt fehlende MAIN-Puffer; Coretest `testStemWithoutMain` hat sinnvolle Audioerwartung. Mehrbus-Pakettests vorhanden, deren 16-Bus-Orakel muss verbessert werden (T02). |
| F09 – Host-DPI fehlt im Resize | TEILWEISE | `effectiveScale()` verbindet Userzoom und ContentScale, [V:309](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L309)/[E:11](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorEditor.h#L11); spezifischer neuer Editoroverride vorhanden. Angehangene DPI-Wechsel/Ablehnung OFFEN. |
| F10 – Resize-Ablehnung ignoriert | TEILWEISE | Add/Hide-Rollback und Compact-Paging vorhanden. A06 zeigt verbleibende Höhe-/Erreichbarkeitslücke beim Recall. |
| F11 – Ctrl-Reset fehlt | TEILWEISE | Zeilenpegel=100, Pan/Pitch=0 im tatsächlichen RowMixpfad implementiert. Master besitzt eigenen falschen Reset A03. |
| F12 – UTF-8-Zerlegung beim Kürzen | TEILWEISE | [V:406](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L406) schiebt Grenzen auf Codepointgrenzen. Keine weitere belegte Bytezerlegung an dieser Stelle; Glyphenbreite/ungültige Eingaben/native Textdarstellung nicht umfassend gemessen. |
| F13 – FLAC-Grenzen zu spät | TEILWEISE | Metadaten-/Kanal-/Größenprüfung vor Vollimport, begrenztes Chunklesen, Ausgabe erst bei Erfolg. Altes Voll-Dekodieren vor Prüfung korrigiert; unbekannte Länge/Spätkorruption/RAM bleiben R04/R05. |
| F14 – SRC-Aliasing bei Ratio16 | TEILWEISE | Kernel jetzt ratioabhängig bis 256 Taps; relevante Core-Aliasingprüfungen bei Ratio2/16 vorhanden. Alter starre-32-Tap-Gegenfall nicht unverändert gültig. Flächige Qualitätsmessung und paced Deadlineprüfung fehlen. |
| F15 – Zoomanzeige/-persistenz | TEILWEISE | Auswahlmarkierung und DZM1-Serialisierung vorhanden; neu erstellte View erhält Präferenz. Offener-Editor-Recall A08 bleibt. |
| F16 – Host zeigt keine A–H | TEILWEISE | `LegacyPatternParameter` und gemeinsame `patternFromNormalized()` liefern konsistente alte Rundungsgrenzen und A–H-Konvertierung. Tatsächliche Hostdarstellung/Automationsspur einschließlich MIDI-Feedback A01 noch offen. |
| F17 – Windows-Pfadlänge | TEILWEISE | Byteanzahl ohne zwingendes NUL wird berücksichtigt; zusätzlicher, nun vorhandener Win32-Formatpatch bevorzugt CF_HDROP. Frühere Ursache korrigiert. Aktueller nativer Drop nicht von diesem Auditor ausgeführt; A05 ist ein anderer Zielzuordnungsfehler. |

### 5.2 N01–N10

| ID | Status | Aktueller Nachweis |
|---|---|---|
| N01 – Add/Hide-Rollback | TEILWEISE | Ablehnung ruft bedingungslosen Modell-Rollback auf; Hilfsmodell testet Rücksetzen. Keine echte Host-/View-Transaktion im bestehenden Test. |
| N02 – Zoom/Frame-Rollback | TEILWEISE | [E:41](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorEditor.h#L41) prüft tatsächliche Frame-Transformation gegen effektiven Zoom und setzt Cache bei Ablehnung zurück. Alter Cache-only-Fehler korrigiert; native Akzeptanz/Ablehnung fehlt. |
| N03 – FLAC-Kurzdekodierung | TEILWEISE | Dekodierte Länge wird bei bekannter deklarierter Gesamtlänge verglichen; Paketfixture erklärt 256 und liefert 128 Frames, muss abgelehnt werden. Nicht dasselbe wie unbekannte Länge/CRC-Spätfehler R05. |
| N04 – Header-y-Versatz | TEILWEISE | A–H y38…68, Zoom y39…69 passen getrennt zu Zeichnung; keine erneute alte Einpixel-Behauptung. Native Dispatchtests fehlen. |
| N05 – abweichende Quantisierung | TEILWEISE | Gemeinsame Legacy-Rundung statt konkurrierender Buckets. Die vorhandenen Zwischenwerttests sind sachlich relevant; Rückkanal A01 und Overflow A02 bleiben separat. |
| N06 – alte Automationsgrenzen/Hosttext | TEILWEISE | `round(clamp(v)×7)` und dazu passender Parameter-Plain-/Textpfad erhalten alte Bedeutung, z.B. 0,1→B, 0,9→G. Abnahme in echtem Host noch ausstehend. |
| N07 – Restore überschreibt Hostevent | OFFEN für neuen Konkurrenzfall | Der alte `PatternRestoreLatch` ist **nicht mehr im Produktpfad**. `AtomicPatternSelection` ersetzt ihn. Historischer N07-FAIL darf nicht auf unbenutzte Datei gestützt werden; verbleibende Gleichwert-/ABA-Frage R01 braucht Produktbarrieretest. |
| N08 – Stop direkt nach Kitwechsel | TEILWEISE | Revision verwirft `wasPlaying_` nicht mehr; [Host:1581](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L1581) prüft Restore → ein laufender Block mit Sound → Stop. Dieser Regressionstest trifft tatsächlich die frühere Lücke. Hier nicht erneut nativ ausgeführt. |
| N09 – Choke bei vollem Voicepool | TEILWEISE | [Engine:267](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SampleVoiceEngine.h#L267) prüft freie Voice vor Choke; [VoiceT:252](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L252) trifft volle 64 Voices plus neue Chokegruppe mit hörbarem Erwartungswert. Kein unverändert belegter alter Chokefehler. |
| N10 – HiDPI vor Attach/Resize-Ablehnung | TEILWEISE | [E:13](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorEditor.h#L13) behandelt Scale selbst, inklusive getSize vor Attach und Frame-Fehlerpfad; [EditorT:60](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/EditorGeometrySmoke.cpp#L60) prüft Preattach-Größen. Echter Attach-/HWND-/Hostablehnungstest bleibt ausstehend. |

### 5.3 Spätere B-/D&D-Befunde

| Alt-ID/Thema | Aktueller Befund |
|---|---|
| B01 zerrissene Pattern-Replace-Generation | Per-Pattern-Sequenz und kohärenter Audiocache eingeführt; keine erneute Behauptung, der Scheduler lese weiterhin blind einzelne neue/alte Zellen. Stressprüfung hat die Aussagegrenze T03. |
| B02 alte Ausgangszuordnung nach Legacy-Recall | Defaults werden im Controller jetzt zurückgesetzt. |
| B03 Lifecycle-Timingreste | `setActive`/`setProcessing` setzen Stimmen/Phasenerwartung zurück; aktueller echter Hostlauf fehlt hier. |
| B04 keine Compact-Erreichbarkeit | Paging implementiert; A06 bleibt bei zu geringer tatsächlich abgelehnter Höhe. |
| B05 Fill-Menü unten abgeschnitten | Position wird mit 230 Pixel Reserve begrenzt; neun Einträge à25 passen grundsätzlich. Overlay-Drop A05 ist davon unabhängig. |
| B06 Alt+Output löscht Sample | Alt-Löschen endet vor x203, Output beginnt x230. Alte Überlappung beseitigt. |
| B07 Fill-Außenklick wirkt darunter weiter | Außenklick schließt und wird konsumiert, [V:678](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L678). |
| B08 Ordnernavigation bleibt an kaputter Datei hängen | Begrenzte Schleife sucht weitere Kandidaten; alphabetische/Umlauf-Helfer vorhanden. Tatsächliche Dateisystem-/Fehlerkette ist unzureichend getestet. |
| B09 Duplicate meldet Fehlschlag trotz vorhandener Kopie | Rückgabewert trennt erzeugte Kopie von eventuell abgelehnter Auswahl, [C:335](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp#L335). Aktuelle Footeroberfläche bietet COPY/PASTE/CLEAR/UNDO, keinen separaten Duplicatebutton; API und zugesagten GUI-Scope bei Abnahme unterscheiden. |
| DND01 fehlender Phraseator-Windows-Patch | CMake inkludiert den Patch jetzt bei [Build:148](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/CMakeLists.txt#L148); Patchdatei bytegleich mit funktionierender Referenz. **Kein erneuter Vorschlag einer eigenen Adaptervariante nötig.** |
| DND02 Drop meldet Erfolg trotz Importfehler | `loadPath` liefert Erfolg nur bei Accepted; Drop gibt dieses Ergebnis zurück, [V:633](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L633). |

## 6. Gesamte technische Funktionsprüfung

### 6.1 Audioengine, Routing und Stimmen

Die Verarbeitung benutzt vorbereitete, unveränderliche Sampleassets. Audio übernimmt veröffentlichte Zeiger; Freigabe alter Besitzer erfolgt im Kontrollpfad, nicht als `delete` im Renderer. Bestehende Stimmen halten ihr altes Asset bis zum Ende. Samplewechsel bedeutet somit nicht automatisch Abschneiden alter Tails; das ist eine gewählte Architektur und nicht als Use-after-free zu beanstanden. Ownership-/Rollbacktests sind vorhanden, aber nicht identisch mit einem gemessenen Race-/Deadlinebeweis unter Hostlast.

Der Voicepool ist auf 64 begrenzt. N09 ist fachlich korrigiert: ein abgewiesener neuer Hit darf nicht zuvor gültige Stimmen wegchoken. Retrigger-/Chokefade, Stereo-Amplituden, leere Instrumente, Sampleratenwechsel und Auswahl mehrerer Ausgänge besitzen tatsächliche DSP-Orakel. Mute/Solo wirkt auf neue Trigger; schon klingende Stimmen bleiben gemäß dem ausdrücklichen Test [VoiceT:111](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L111) erhalten. Ebenso werden Gain, Panorama und Ausgangszuordnung beim Start einer Stimme übernommen. Das kann musikalisch anders erwartet werden, ist aber ohne gegenteiligen Produktvertrag **kein neu nachgewiesener Defekt**.

MAIN plus 15 zusätzliche Stereoausgänge sind implementiert. Freie Zuordnung und mehrere Rows auf demselben Bus sind vorhanden. Coretests [VoiceT:334](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L334) und [VoiceT:353](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L353) prüfen keine zusätzliche MAIN-Kopie und das Summieren gemeinsamer Ziele. Ein deaktivierter Ziel-AUX fällt auf MAIN zurück; das ist gegenwärtig der explizite Vertrag, keine unbemerkte doppelte Ausgabe. MAIN kann selbst inaktiv sein; AUX-Verarbeitung läuft weiter. Die aktuelle Busaktivierungs-/Recallmatrix ist trotzdem nicht vollständig nativ belegt, insbesondere Umschalten bei klingenden alten Stimmen und starkes Stereo-Asymmetrie-Orakel fehlen.

Der Master multipliziert alle bereitgestellten Ausgangsbusse. A03 ist eine UI-Resetursache, kein zusätzlicher Bus-Multiplikationsfehler. Abrupte Masteränderungen besitzen keine nachgewiesene geglättete Pegelrampe; mögliche Klicks sind als Audio-QA-Risiko zu messen, nicht als bereits gehörte Störung zu behaupten. Hardclip/Sättigung ist nicht implementiert; Summen oberhalb 0 dBFS im Floatbus sind zunächst Headroom-/Hostfrage.

**Gesamtstatus: TEILWEISE.** Keine unabhängige Echtzeit-, Hör- oder vollständige Routingfreigabe.

### 6.2 Sampler, WAV/FLAC und Ordnernavigation

WAV unterstützt mono/stereo PCM8/16/24/32, Float32 und die behandelten Extensible-Varianten. Der Decoder prüft Container-/Chunkgrenzen, Format, Framegrenzen und endliche Floatwerte; Ergebnisübernahme erfolgt nach erfolgreicher Dekodierung. Die Row-Nachricht ist auf 16 MiB begrenzt, das eingebettete Kit-WAV-Budget auf 64 MiB. FLAC wird im Kontrollpfad über gepinnten dr_flac in ein geprüftes WAV-Repräsentat für Asset-/Stateverwendung umgewandelt. Originalname/Label und gespeicherte Audiodaten sind bewusst verschiedene Dinge.

Der Importpfad bereitet einen Kandidaten vor und ersetzt das bestehende Sample nach erfolgreicher Annahme. Vorzeitige Metadaten- und Bytegrenzen sind gegenüber F13 verbessert. Fehlendes/kaputtes WAV und bekannte FLAC-Längenfehler besitzen Tests. Bei fehlgeschlagener Navigation wird begrenzt weitergesucht, erfolgreiche Auswahl darf den Cache erst nach Annahme ersetzen. Alphabetische Sortierung und Wrap-around sind im Helfer vorhanden; echte Ordner mit Unicode, gelöschten Dateien, mehreren defekten Kandidaten und fehlenden Berechtigungen fehlen im Integrationstest.

Die Bytegrenzen sind **keine RAM-Obergrenze**: PCM8 expandiert zu Float32, temporäre Kandidaten/Preflight und alte noch klingende Assets leben zusätzlich, ebenso SRC-Kernels. R04/R05 bleiben offen. Ein fehlender oder nicht unterstützter Dateipfad darf kein vorhandenes Sample löschen; A05 betrifft dagegen eine fälschlich als gültiges Ziel erkannte verdeckte Zeile.

**Gesamtstatus: TEILWEISE; unbekannte FLAC-Länge, Spätkorruption und Speicherstress OFFEN.**

### 6.3 Transport, Pattern, Timing, MIDI und Automation

A–H enthalten jeweils 16×32 Velocitybytes. DM15 speichert zusätzlich die pro Pattern gewählte 16-/32-Step-Länge; Schritte außerhalb einer kürzeren Abspiellänge sollen erhalten bleiben. SongChain besitzt bis zu 64 Einträge mit 8/16/32 Steps. Copy/Paste/Clear/Fill und Duplicate sind nicht nur Commitbeschreibungen: Controllertransaktionen, Prozessor-Nachrichten und Patternbankpfade wurden verfolgt. Bulk-Replacement veröffentlicht eine ganze Pattern-Generation; der Audiocache liest eine kohärente Momentaufnahme statt Zellen während eines Writes zu mischen.

Der Scheduler arbeitet mit PPQ und Sampleoffsets. Patternwechsel werden zeitlich segmentiert, gleichzeitige Hostwerte haben Vorrang vor MIDI. Alte Automationsgrenzen verwenden die gemeinsame Legacy-Rundung. Nullframeflush ist vorhanden. A01/A02 verhindern trotzdem eine vollständige MIDI-/Automationfreigabe.

Bei neuem Lauf/Locate/Loop definiert der aktuelle Prozessor den Start relativ zum erkannten Ursprung. Die Coretests können diesen Ursprung als Eingabe setzen; das beweist nicht automatisch, dass der Prozessor ihn bei jedem Hostablauf richtig bestimmt. Konstantes Tempo, deterministisches Swing/Humanize und ausgewählte Blockpartitionierungen sind getestet; Tempo-Rampen, Loopgrenzen innerhalb eines Blocks, Start auf beliebigem PPQ, negative Vorlaufpositionen, Taktartwechsel und verschiedene Blockgrößen in der tatsächlichen DAW sind unvollständig. Die 4-Sample-Diskontinuitätstoleranz und blockweise Tempovorhersage erfordern die Prüfung R06. Ein pauschaler Fehler „Start ab Takt2 ist kaputt“ ist am aktuellen Quellcode nicht unabhängig nachgewiesen.

MIDI-Instrumentaudition bei gestopptem Transport ist vorhanden, solange der Host weiter `process()` aufruft. `kInfiniteTail` ist für einen intern Ereignisse erzeugenden Sequencer nachvollziehbar; es erzwingt nicht in jedem Host Audioverarbeitung beim Stopp. Preview während DAW-Stopp ist daher zusätzlich praktisch abzunehmen.

**Gesamtstatus: FAIL für A01/A02; übriger Timing-/Songvertrag TEILWEISE, echte DAW-Matrix OFFEN.**

### 6.4 Projektzustand und VST3-Lebenszyklus

Aktuelles Format ist **DM15**, nicht mehr DM11. Kitdaten umfassen eingebettete Audiodaten, Auswahl, sichtbare Rows, Mix/Output, Mute/Solo/Choke, Timing, Namen, Master, Patternlängen und Song. Decoder und Migration erkennen mehrere historische DMR-/DM-Varianten, prüfen Größen/Hashes/erlaubte Werte und übernehmen einen Kandidaten statt blind den Ausgabezustand zu überschreiben. Ältere Formate erhalten Defaults für später eingeführte Felder. Getrennte Controllerdaten DZM1 speichern die Zoompräferenz.

Ein echter paketierter Test erzeugt eine frische Instanz, lädt State **vor** `setupProcessing` und prüft wiedergegebenes Audio; das ist stärker als ein reiner Codec-Roundtrip. Ein 16-Row-Recall und zahlreiche Kontrollnachrichten sind vorhanden. Das bedeutet nicht, dass jede Migration oder aktive gleichzeitige Automation atomar über Controller, Prozessor und bereits geöffnete View konsistent ist. Für den Audiothread sperrt `KitRestoreGate` eine laufende Restoretransaktion aus; dort ausgegebene Nullen sind ein ausdrücklicher Mechanismus, kein Lock im Callback. Die Eventpriorität R01 und die Dauer/Notenverluste R03 bleiben ungeklärt.

`setActive`/`setProcessing` resetten relevante Stimm-/Zeitinformationen. 32-Bit-Float ist der unterstützte Verarbeitungstyp, Ablehnung von 64-Bit ist hier kein Verstoß gegen eine selbst beworbene 64-Bit-Unterstützung. Stereoausgänge werden angeboten, kein zugesagter Mono-Produktpfad. Host-Dirty wird für angenommene Änderungen gemeldet; tatsächlich angeschlossener Handler, fehlgeschlagene Nachrichten, Controllerstate-Zoom und Undo als Hostprojektänderung gehören noch in denselben Integrationstest.

**Gesamtstatus: TEILWEISE; A08 FAIL, Konkurrenz-/Fehlerlastabnahme OFFEN.**

## 7. Multithreading, Atomizität, Speicher und Realtime: bestätigte Eigenschaften und getrennte Risiken

### R01 – Gleichwert-/ABA-Event zwischen Snapshot und Restore-Gate

**Priorität HOCH; Status OFFEN, Quellcode-abgeleitetes Konkurrenzrisiko, kein hier reproduzierter Produkt-FAIL.**

[P:860](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L860) liest `selectionAtStart` **vor** `ControlRestoreEntry`. In [Selection:13](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternSelection.h#L13) verändert normales `store()` nur die unteren drei Auswahlbits; die Generation erhöht nur `hostSubmit()` im Busy-Zweig [Selection:22](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternSelection.h#L22). Ein normaler Audioblock kann daher zwischen Snapshot und Gate A→A beziehungsweise A→G→A verarbeiten, ohne dass sich das am Snapshot sichtbare gepackte Wort endgültig ändert. `restoreIfUnchanged()` kann danach den Kitwert B übernehmen. Das einfache Bitmodell zeigt diese Möglichkeit, aber **der deterministische Interleaving-Test am echten Prozessor wurde nicht ausgeführt**. Ob dies den gewünschten Prioritätsvertrag verletzt, muss durch ausdrücklich festgelegte Linearisation beurteilt werden; „jede später eingegangene Automation gewinnt“ ist stärker als „Auswahlwert war am CAS unverändert“.

**Nacharbeit/Test:** Barrieretest im tatsächlichen `setState/process`-Pfad für Snapshot → gleichwertiges Hostevent → Restore; zusätzlich ABA, Busy-Event, Nullframe und normaler Block. Auswahl, Audio, gespeicherter Wert und Controllerbeobachtung vergleichen. Erst danach gezielte Generationstransaktion korrigieren; unbenutzten Legacy-Latch nicht erneut zur Diagnose heranziehen.

### R02 – Lockfrei ist kein garantierter maximaler Callbackaufwand

**Priorität HOCH für Realtimefreigabe; Status TEILWEISE/OFFEN.**

Keine expliziten Dateizugriffe, üblichen Heap-Allokationen, `delete`s, Logging oder blockierenden Mutexe wurden im regulären Audiopfad gefunden. Vorbereitete Assets und feste Trigger-/Voicepools sind gute Voraussetzungen. `KitRestoreGate` wartet auf der Kontrollseite; Audio wartet nicht auf die Kitmutex.

Allerdings haben [Selection:13](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternSelection.h#L13) und [Selection:22](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternSelection.h#L22) `compare_exchange_weak`-Wiederholungen ohne feste Versuchszahl. `is_always_lock_free` beweist Lockfreiheit, **keine Wait-Freiheit oder Maximaldauer**. Das ist eine offene Abweichung vom strikten Engineering-Ziel „kein unbeschränkter Callbackaufwand“, kein Nachweis, dass in einer konkreten DAW bereits ein Deadlinefehler auftrat. Auch die Anzahl eingehender Hostereignisse muss bei Worst-Case-Bewertung berücksichtigt werden.

**Nacharbeit/Test:** Prioritäts-/Fortschrittsmodell dokumentieren, begrenzte Audiotransaktion erwägen, Allocation-/Deallocation-Hooks und Deadlineinstrumentierung im vollständigen Callback. Konkurrenzlast gezielt erzeugen; nicht nur Durchschnitts-CPU messen.

### R03 – Restore unter Busy schaltet Audio stumm und verliert MIDI

**Priorität HOCH für Praxisabnahme; Status dokumentiertes Verhalten + OFFEN für zulässige Stördauer.**

[P:861](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L861) umfasst auch `publishRestoredKit()` und damit Vorbereitung mehrerer Samples/Kernels. Währenddessen liefert [P:454](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L454) Nullen auf den Ausgängen und übernimmt Hostpatternpunkte, verarbeitet aber keine MIDI-Noten. Die Synchronisation verhindert ungeschützte gleichzeitige Zugriffe auf das aktive Kit, garantiert jedoch keinen nahtlosen hörbaren Kitwechsel. Nach Ende der Transaktion wird die Revision veröffentlicht. Dieser Mechanismus ist nicht mit „Kitwechsel unterbrechungsfrei realtime-sicher bewiesen“ gleichzusetzen.

**Nacharbeit/Test:** Große 16-Row-Kits mit aktiven Stimmen und MIDI/Automation laden; längste Stummphase, verworfene Noten und p99/max messen. Soll der Wechsel hörbar atomar am Blockrand erfolgen, außerhalb des Busy-Fensters vollständig vorbereiten und erst dann eine kohärente Generation übernehmen. Erwarteten Tail-/Notenvertrag festlegen.

### R04 – Importbudget begrenzt Dateien, nicht maximalen Prozess-RAM

**Priorität MITTEL; Status OFFEN.**

16-MiB-Row-/64-MiB-Kitgrenzen, PCM→Float-Expansion, Kandidatenkopien, Preflight, neue Kits und bis zu 64 alte gehaltene Voiceassets können wesentlich mehr RAM benötigen als die kodierten Bytes. Die Testlimits prüfen Ablehnung von Eingabebudgets; sie messen keinen Peak und keine belastbare Erholung bei Allokationsfehlern. Ein konkreter OOM-Absturz wurde hier nicht provoziert.

**Nacharbeit/Test:** Peak-/Committed-Memory und UI-Blockierzeit bei kleinen Bitsamples, Stereo, hohem Pitchratio, wiederholtem Wechsel und mehreren Instanzen messen. Allokationsfehler gezielt injizieren; vorhandenes Kit/Audio/State muss erhalten bleiben. Wirkliches Prozessbudget von Dateiformatbudget trennen.

### R05 – Unbekannte FLAC-Gesamtlänge und spätes Decoderende nicht ausreichend abgesichert

**Priorität MITTEL; Status OFFEN.**

[Flac:48](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/FlacImport.cpp#L48) beendet das Chunklesen, wenn dr_flac keine weiteren Frames liefert; der explizite Vollständigkeitsvergleich greift bei deklarierter Gesamtlänge. Der vorhandene bekannte-Länge-Kurzfixture prüft nicht dieselbe Situation wie STREAMINFO ohne Gesamtlänge, beschädigte spätere Frames oder CRC-Fehler. Ohne entsprechende echte FLAC-Bytes und Ausführung wird hier **nicht** behauptet, dass genau diese Fälle bereits erfolgreich als vollständiges Sample akzeptiert wurden.

**Nacharbeit/Test:** Mehrframe-FLAC ohne deklarierte Gesamtlänge, Truncation an Header/Frame/CRC, Bitflip spät im Stream, ungültige Kanäle/Raten, Budgetüberschreitung während des Lesens. Decoderfehler von normalem EOF unterscheiden; vorheriges Sample unverändert lassen.

### R06 – Tempo-/Locate-/Loopgrenzen und Timingmodifier

**Priorität MITTEL; Status OFFEN.**

[P:577](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L577) bis [P:590](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp#L590) erkennt Diskontinuitäten mittels erwarteter Folgeposition und blockweisem Tempo; Drift über etwa vier Frames beginnt eine neue Phase. Der Quellcode beweist keine beliebige Tempo-Rampen-/Loop-Behandlung innerhalb eines Blocks. Swing-Phase gegen absolutes PPQ oder relativen Patternursprung muss bewusst festgelegt werden; Humanize darf keine verlorenen/doppelten Grenztrigger erzeugen.

**Nacharbeit/Test:** Frischer Start bei Takt1/2/3, beliebigem Sechzehntel und Vorlauf; Seek/Loop in und zwischen Blöcken, 44,1/48/96 kHz, 32/64/127/256/1024 Frames, Tempoänderung und automatisierte Patternlängen. Unabhängiges erwartetes Onsetband statt nur `isfinite` verwenden.

### R07 – Eingabe-/Build-/Dokumentationsrestpunkte

**Priorität NIEDRIG bis MITTEL; Status Risiken, keine weiteren bestätigten Produkt-FAILs.**

- Konvertierung ungewöhnlicher ungültiger UTF-8-Textpayloads über `pathFromUtf8` und Plattform-Dateiauswahl benötigt Negativtests; aus einer potenziell werfenden Konvertierung wird ohne native Eingabe kein bewiesener Absturz konstruiert.
- Positiver `drumator_song_chain_tests`-Target trägt nur Includeverzeichnisse, während sein negativer Zwilling `drumator_core` mit explizitem C++23-Feature linkt ([Build:9](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/CMakeLists.txt#L9)). Damit ist der Sprachstandard nicht auf beiden Targets gleich explizit festgelegt. Der vorhandene erfolgreiche Windowslauf widerspricht einer pauschalen aktuellen Buildfehlerbehauptung; trotzdem Target-Konfiguration vereinheitlichen, bevor ein anderer Compilerdefault überrascht.
- Hersteller-/Artifactnamen und historische Metadaten sind nicht mit einer versionierten freigegebenen FULL-ZIP gleichzusetzen. Kein Gumroad-/Owner-PASS in diesem Bericht.
- Kohärente Pattern-/Song-Snapshots setzen das dokumentierte Schreibmodell voraus. Kontrollschreiber sind zu serialisieren; ein lockfreies Lesemuster allein ersetzt weder eine Speichermodellbegründung noch zielarchitekturspezifische Konkurrenzregressionen. Hier wurde kein zusätzlicher portabler Memory-Ordering-Defekt nachgewiesen.

## 8. Performancebewertung: vorhandene Messung, fehlende Garantie

[VoiceT:276](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L276) bereitet einen Coretest mit 64 Voices und Ratio16 vor und misst **180 ungebremst berechnete Blöcke**. [VoiceT:323](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L323) druckt p95/p99/max und die rechnerische Deadline bei 48 kHz/256 Frames:

`256 / 48000 × 1000 = 5,333… ms pro Block`.

Die tatsächlich durchgesetzte Bedingung ist jedoch **Gesamtdauer <20 Sekunden** ([VoiceT:332](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp#L332)), ausdrücklich eine Hänge-/Regressionserkennung. Schon 180×5,333 ms ergeben nur 0,96 Sekunden Audiolaufzeit; die 20-Sekunden-Grenze kann selbst deutliche Echtzeitüberschreitungen nicht ausschließen. Das ist kein Beweis, dass der Code tatsächlich 20 Sekunden benötigt. Der Test ist unpaced, betrachtet den Core statt aller Host-/GUI-/Restorepfade und kennt keine Schedulingreserve oder Audiointerrupts.

**Keine konkreten p95-/p99-/Max-Zahlen werden hier erfunden:** Sie waren im verfügbaren unabhängigen Nachweis nicht vollständig einsehbar. `ctest --output-on-failure` kann die Ausgabe eines bestandenen Tests verbergen. Der Selbst-Audit/Benutzerauftrag mit grüner Anzahl liefert keine Zeitreihe und kein Deadlinehistogramm. Auch die längere Finite-Audio-Schleife im Host-Smoke ist ein Stabilitätsindikator, kein vollständiger Audio-Deadlinebeweis.

**Erforderlich:** Metriken als prüfbares Artefakt speichern, Maschinen-/Compiler-/Powerprofile nennen, Instrumentierungsaufwand messen; vollständigen Plugin-Callback gepaced bei mehreren Sampleraten/Blockgrößen, Instanzen, 16 Outputs, Sample-/Kitwechsel und GUI-Bedienung testen. p50/p95/p99/max, Zahl und Höhe aller Deadlineüberschreitungen, Dropouts, Peak-RAM und Callbackallokationen erfassen. Eine kurze Messreihe ist empirische Evidenz, keine mathematische Garantie für jede Hostlast.

**Performanceurteil: TEILWEISE für vorhandene Durchsatzinstrumentierung, OFFEN für reale Echtzeitfreigabe.**

## 9. Vollständiger GUI-/Bedienabgleich am tatsächlichen Renderer

Dies ist eine **Quellcode- und Geometrieprüfung**, keine vorgetäuschte pixelgenaue Sichtprüfung einer geöffneten DAW. Geprüft wurden alle 42 im Manifest benannten Header-/Row-/Footergruppen einschließlich ihrer Wiederholungen, zusätzliche Texte/Anzeigen, 32 Velocitybalken, neun Fill-Einträge und 64 Songpositionen. Die Tabelle nennt die tatsächliche Funktionszuordnung und verbleibende Grenzen. Für jeden Bereich gilt zusätzlich: reale Schriftmetriken, Fokus, Betriebssystemereignisse und transformierte Mauspunkte benötigen native Prüfung.

### 9.1 Header: alle 13 Manifestgruppen und Texte

Quellen: Zeichnung [V:335](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L335), Eingabe [V:693](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L693), [V:699](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L699), [V:751](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L751), [V:764](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L764). Koordinaten sind logische VSTGUI-Pixel; Rechtecke im Eventpfad überwiegend halb offen.

| Element | Geometrie/Zuordnung | Ergebnis |
|---|---|---|
| A–H, jeweils acht Tasten | `x=340+62i … +54`, `y=38…68`; Auswahl, Shift schaltet 1T/2T | Lücken werden explizit abgewiesen; Text enthält Länge, Auswahl folgt Controller. A01 bleibt für MIDI. |
| Compact `<` und `>` | links x1005…1032, rechts x1032…1060; y39…69 | Paging vorhanden; nur im Compactmodus. Zeichnung des rechten Symbols beginnt x1033; kein daraus abgeleiteter Routingfehler. Höhe A06. |
| 100 % und 150 % | x1070…1127 und1136…1217; y39…69 | Markierung nach tatsächlichem Editorzoom, `setZoomChecked`; A08 bei offenem State-Recall. |
| Swing −/+ | x185…214/214…244, y74…98 | ±5, Strg→0; Anzeige Controllerwert. Native Grenz-/Dirtyprüfung fehlt. |
| Humanize −/+ | x448…477/477…505, y74…98 | ±10, Strg→0; keine neue Fehlzuordnung gefunden. |
| Song ON/OFF | x540…638, y74…98 | Toggle und sichtbarer Zustand stimmen im normalen Klickpfad; neue Headerlücke inert. |
| Song-Kette editieren/zurück | x652…850, y74…98 | Schaltet Overlay; Klickvorrang vorhanden, D&D-Vorrang fehlt A05. |
| Master −/+ | x1140…1180/1184…1228, y74…98 | Normale ±5 korrekt zugeordnet; Strg falsch A03. Abstand1180…1184 jetzt inert. |

Zusätzliche passive Texte: Titel, Modifierhilfe, Swing-/Human-/Masterwerte, Chain-Zusammenfassung, Compact-Seitenanzeige. Diese sind keine weiteren Regler. **Offene Beschriftungsrisiken:** Die Hilfe benutzt `CRect(16,47,990,67)`, die danach gezeichneten Patternbuttons liegen teilweise im selben y-Band; die sehr lange Chain-Zusammenfassung hat nur x866…1030. Das sind geometrisch kollidierende beziehungsweise knappe Textflächen, keine hier per Windows-Fontpixel bestätigte vollständige Lesbarkeitsprüfung. Screenshot-/Textmetriken bei beiden Zoomstufen nachreichen; nicht einfach das Slot-JSON als exakten Sichtnachweis verwenden.

### 9.2 Instrumentenzeile: alle 14 Gruppen, jede Wiederholung

Quellen: Zeichnung [V:396](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L396), Eingabe [V:892](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L892). Zeile `y=108+(row-firstRow)×29`. Default vier Rows; das tatsächliche Modell erlaubt **1–16**. Ein Audit nur für 4/16 würde den zulässigen Einzeilenfall A06 übersehen.

| Feld | Bounds x / Funktion | Ergebnis |
|---|---|---|
| FILL | 16…48; neun Fill-/Clear-/Rhythmusoptionen | Öffnet das Menü für die ermittelte sichtbare Row; Popupklemmung vorhanden. |
| P | 52…78; Sample-One-shot | Nachricht an Prozessor; echter Stopp-Hostcallback OFFEN. |
| Name/Samplefeld | gezeichnet81…202; normaler Ladefangbereich16…203 nach Vorrang von Fill/Preview | Shift→Nameeditor, Alt→Sampleclear ohne Stepclear. A04; größere horizontale Fangfläche ist explizite Bedienentscheidung. |
| Vorheriges Sample | 203…216 | Ordner −1; keine überlagerte Outputfunktion. Echte FS-Integration ungetestet. |
| Nächstes Sample | 216…230 | Ordner +1; Darstellung endet229, 1px Fangreserve nicht als wesentlicher Defekt behandelt. |
| Ausgang | 230…275 | `MAIN`, danach `O02…O16`; zyklische freie Zuordnung. Alt fällt nicht mehr ins Samplelöschen. |
| VOL | 280…345 | Linke/rechte Hälfte senkt/erhöht; Strgdefault100. Die Lücke345…350 wird nicht mehr als VOL behandelt. |
| PAN | 350…415 | Linke/rechte Hälfte, Strgdefault0; Lücke415…420 inert. |
| PITCH | 420…475 | Halbtonsteuerung, Strgdefault0; vorbereiteter SRC-Kernel/Assetpfad. |
| M | 478…500 | Mutezustand an Controller gebunden; betrifft neue Trigger, bestehende Tails gemäß Vertrag. |
| S | 507…529 | Solozustand; Hidden-Solo-Migration berücksichtigt. |
| Choke | 535…558 | 0–4, Shift rückwärts; aktueller Wert/Zustand gezeichnet. Voicecap-Fix berücksichtigt. |
| RND | 560…619 | ±10 je Hälfte, Strg→0; sichtbarer Prozentwert, deterministische Triggerstreuung. |
| Steps 1–32 | `622+19i … +19`, ganze29px-Zellenhöhe | Toggle/Shift-Velocityzyklus; Fillfläche innen15×18, ganze Zelle absichtlich größer klickbar. Letzte Zelle endet1230. Keine abgeschnittenen letzten sechs Steps aus F01 mehr abgeleitet. |

Die numerischen Anzeigen VOL/PAN/PITCH haben zusätzlich sichtbare Minus-/Pluszeichen; es existieren hier keine klassischen Drehknob- oder Texteingaberegler für diese Werte. Pro Row wurden getrennte Werte, Grenzen, Modifier, Anzeigequelle und Nachrichtenpfad geprüft. Ein dynamischer Namenseditor ist eine zusätzliche tatsächliche VSTGUI-Control, nicht lediglich gemalter Text.

### 9.3 Footer: alle 15 Manifestgruppen plus Statuswerte

Quellen: [V:474](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L474), [V:788](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L788), [V:826](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L826), [V:841](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp#L841). `F=108+displayedRows×29+5`.

| Element | Geometrie/Aktion | Ergebnis |
|---|---|---|
| Instrument hinzufügen | x16…213, yF…F+29 | Max16, Nachricht/Resize/Rollback vorhanden; native Ablehnung noch testen. |
| Instrument entfernen | x226…266, yF…F+29 | Nur letzte vollständig leere Row, Minimum1; Prüfung über alle A–H. |
| COPY | x480…565 | Kopiert aktuelles Pattern in Sessionclipboard. |
| PASTE | x570…655 | Übernimmt Clipboard in gewähltes Pattern, transaktionale Nachricht; bestehende überschreibbare Zielaktion ist explizites Paste. |
| CLEAR | x660…745 | Leert aktuelles Pattern, nicht sämtliche Kits/Patterns; Undo verfügbar. |
| UNDO | x750…835 | Verfügbarkeit jetzt korrekt als disabled gezeichnet; Gestenproblem A07 bleibt. |
| Velocity variieren | x844…1014 | Ändert aktive Steps im aktuellen Pattern; keine Sampleersetzung. |
| Velocity Reset100 | x1024…1220 | Setzt aktive Steps auf100. Die Lücke1014…1024 erzeugt im Dispatcher eine falsche Erfolgsmeldung ohne Reset, Zusatz zu A09. |
| Velocitylane öffnen/schließen | x16…266, yF+34…F+64 | Zustands-/Resize-Rollback implementiert. Recall-/Compacthöhe A06. |
| Velocity-Row `<` | x280…324, yF+34…F+64 | Gewählte Bearbeitungsrow begrenzt decrementieren. |
| Velocity-Row `>` | x408…455, gleiche y | Inkrementieren; mittlere gezeichnete Rowanzeige keine zusätzliche mutierende Aktion. |
| Velocity −10 | x916…986, gleiche y | Gewählter Step, auf1…127 begrenzt. Step-AUS läuft separat über Lane/Toggle. |
| Velocity −1 | x994…1064 | Gleicher Wertpfad, echte negative Zwischenräume. |
| Velocity +1 | x1072…1142 | Gleicher Wertpfad. |
| Velocity +10 | x1150…1220 | Gleicher Wertpfad. |

Passive Footertexte: Rowanzahl, gewählte Row/Step/Velocity, Lane-Titel, 0/127-Skala und Status-/Fehlermeldung. Ihre Sichtbarkeit bei Ablehnung ist durch A06 nicht garantiert. Der Status wird außerdem vor dem Song-Overlay gezeichnet; Overlayfehler sollten bei nativer Abnahme sichtbar und verständlich angezeigt werden, statt nur eine Nachricht im verdeckten Hintergrund zu ändern.

### 9.4 Velocitylane, Fill und Song vollständig

- **32 Velocitybalken:** x622+19i, vertikal Gridende+95…+165; alle32 inklusive Step32 im Mapper geprüft. Normaldrag1…127, Strgdrag0. Inaktive zweite Patternhälfte wird abgedunkelt, Daten sollen erhalten bleiben. A07 betrifft den tatsächlichen Event-/Undozusammenhang.
- **Neun Fill-Einträge:** Fill2/4/8, Snare2+4, All, Clear, Offbeat, Galopp, Ghost; x650…819, je25px Raster, 24px Zeichnung. Menü schließt/konsumiert Außenklick, keine ungeprüfte Weiterleitung mehr wie B07. Ein Pixel größere Trefffläche ist hier nicht automatisch ein falscher Befehlsindex. D&D auf das Popup bleibt A05.
- **64 Songpositionen:** 16 Spalten×4 Reihen, x16+76c, y134+32r, Zeichnung73×29. Gewählte/verfügbare/nächste freie Position unterschiedlich gezeichnet; nur nächste freie Position verlängert die Kette. Das breite Zellraster akzeptiert auch kleine Zwischenräume; bewusste Zell- statt Buttonflächenentscheidung festlegen und testen.
- **Song A–H, drei Längen, Plus und Minus:** Direkte Editornachrichten und angezeigte Eintragswerte vorhanden. A09 bei den diskreten Pattern-/Längenlücken. Count1…64, Auswahl nach Entfernen begrenzen. Songdarstellung und Instrument-Drops A05 sind nicht konsistent.

### 9.5 Zoom, DPI, Fenster und Editor-Lifecycle

Bei angenommenem Resize ergeben sich folgende **rechnerische** Höhen. Es handelt sich nicht um gemessene HWND-Clientgrößen:

| Rows | Lane zu | Lane offen | Bei150 % zu/offen, ohne zusätzlichen Host-DPI |
|---|---:|---:|---:|
| 1 | 326 | 357 | 489 / 535,5 |
| 4 | 329 | 444 | 493,5 / 666 |
| 8 | 445 | 560 | 667,5 / 840 |
| 16 | 677 | 792 | 1015,5 / 1188 |

Zusätzlicher Host-ContentScale multipliziert den Userzoom; physische Integergrößen unterliegen SDK-Rundung. UIDESC erlaubt nun max850 logisch. Neue Preattach-Editoren melden zunächst1280×300; das ist nicht die spätere vollständig angehängte Vierzeilenoberfläche. N02/N10 source-seitig verbessert, A06/A08 offen fehlerhaft. Kein aktueller Test prüft alle vier tatsächlichen Größen zugleich: Hostrect, Editorrect, Frame-Transformation, Childbounds. Viewregister und `removed()`-Cleanup sind vorhanden; Mehrfach-View, Fokus, Reload bei offenem Editor und native Ablehnung müssen noch ausgeführt werden.

## 10. Automatisierte Regressionen: vorhandene Aussage und konkrete Lücken

### 10.1 Alle vorhandenen Testdateien

| Testdatei | Reale Abdeckung im Quelltext | Aussagegrenze |
|---|---|---|
| `CoreTests.cpp` | Scheduler, Timingpartitionierung, Swing/Humanize, Fills, Patternoperationen, Songmodell, Ordner-/Compacthelper | Kein voller VST3-Transport oder native Klicks; Ursprung oft als bereits korrekt berechnete Eingabe gesetzt. |
| `SampleVoiceTests.cpp` | Stereo-/Pegel, Mute-/Tailvertrag, SRC, Voicecap/Choke, Assetwechsel, freie/shared Ausgänge, Finite-/Durchsatzlast | Kein vollständiger Hostcallback-Deadlinebeweis; Qualitätsmessung nur ausgewählte Filterpunkte. |
| `WavDecoderTests.cpp` | Mehrere PCM/Float/Extensible-Formate und Korruption/Ergebnis-Erhalt | Kein Datei-/GUI-/D&D- oder Ressourcenerschöpfungslauf. |
| `SampleExchangeTests.cpp` | Ownership, Abhol-/Freigabegrenzen, Fehlererhalt | Kein vollständiger mehrthreadiger Host-Swap mit erzwungenen Interleavings. |
| `TriggerBatchTests.cpp` | MIDI-Werte, Velocity-RND und ausgewählte Partitionierung | Keine Patternoverflowgrenze A02, kein Controllerfeedback A01; kleine musikalische Grenzmenge. |
| `AtomicPatternTests.cpp` | Einzelzellen, Bulkgen, kohärenter Snapshot, parallel schreibender Stress | T03 kann keine erfolgreiche Überlappungsbeobachtung erzwingen. |
| `VisibleRowsTests.cpp` | Hidden-Solo/Resize/Dirtyhelper, Gatebarriere, Auswahlhelper | Integer-/Callbackmodelle ersetzen keine echte Controller-/Viewtransaktion; R01-Normalpfad fehlt. |
| `KitStateTests.cpp` | Migration, CRC/Struktur/Budgets, Ausgabeerhalt; aktuelle Ausgabevariable jetzt korrekt geprüft | Kein Peak-RAM-/bad_alloc-/Hostlebenszyklus-Beweis, nicht jeder mögliche beschädigte Stream. |
| `PatternUndoTests.cpp` | Sessionlokaler einstufiger per-Pattern-Snapshot | Kein Velocity-Drag und keine View-/Controller-Verknüpfung A07. |
| `VelocityLaneTests.cpp` | Mapping aller32 Steps, Werte/Erase und Grenzen | Keine tatsächlichen Mausgesten, Fokus oder Undo. |
| `SongChainTests.cpp` | Jetzt aktive Releasechecks für Count, Entries, Wrap, Origins; Negativtarget prüft Fehlerrückgabe | Keine eigene audiovisuelle Host-/GUIabnahme; zwei CTests aus derselben Quelldatei. |
| `UiHeaderHitZonesTests.cpp` | Vier gemeinsame Headerhitboxen, Kanten/Lücken | Nicht gesamte42-Gruppen-/Song-/Rowdispatchmatrix. |
| `EditorGeometrySmoke.cpp` | Paketierte Factory, UIDESC-Größe1280×300, Preattach-DPI100/150 | Kein `attached`, HWND, Paint, Maus, Textinput oder `IPlugFrame`-Ablehnung. |
| `PackagedAudioHostSmoke.cpp` | Tatsächliches Paket, frische Instanz, Recall vor Setup, Audioformate/Mix/Pattern/Song/MIDI/Flush,16Rows/AUX, Restore/Stop | Viele `notify()`-Direktnachrichten umgehen tatsächlichen Controller-/GUIpfad; keine native Bedienung und unzureichende Orakel T01–T04. |

Die Suite ist substanziell und keineswegs nur aus Attrappen zusammengesetzt. Ihre Lücken lassen sich trotzdem nicht durch eine grüne Gesamtsumme schließen. Insbesondere die neu vorhandenen Pakettests zu `PatternReplace` und `RowOutput` werden ausdrücklich anerkannt; deren frühere vollständige Abwesenheit wird nicht erneut behauptet.

### 10.2 Bestätigte Schwächen bestehender Testorakel

**T01 – MITTEL: DM15-Datenerhalt prüft lokale Fixture statt Pluginzustand.** [Host:911](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L911) setzt Step20 auf95. [Host:933](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L933) behauptet Erhalt der versteckten zweiten Hälfte, liest dafür aber erneut `patternKit.grid[...]` aus der lokalen Vorlage. Selbst wenn der Prozessor die Daten gelöscht hätte, könnte diese Assertion bestehen. **Empfehlung:** Nach Länge2T→1T→2T echten Komponentenstate auslesen und decodieren oder Step20 wieder hörbar messen; alle A–H, frische Instanz und aktive Automation einbeziehen.

**T02 – MITTEL: 15-AUX-Test kann benachbarte Busvertauschung übersehen.** [Host:1438](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L1438) trennt erwartete Pegel um0,015. [Host:1488](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L1488) erlaubt je Kanal eine Abweichung von0,03. Das Vertauschen benachbarter Busse liegt innerhalb der Toleranz; gleiche Monopegel auf L/R unterscheiden auch keinen Kanalswap. **Empfehlung:** deutlich unterscheidbare, unabhängige Stereo-/Zeit-Signaturen oder eine streng begründete kleinere Toleranz; vollständige Matrix, leere Busse und keine MAIN-Duplikation bei aktiver MAIN mitmessen. Das ist eine Testlücke, kein hier bewiesener tatsächlicher Bus-Swap.

**T03 – MITTEL: Pattern-Konkurrenztest kann ohne überprüften überlappenden Snapshot bestehen.** [AtomicT:160](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/AtomicPatternTests.cpp#L160) erhöht `verified`, aber [AtomicT:190](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/AtomicPatternTests.cpp#L190) verwirft den Zähler ohne Mindestbedingung. Ist der Schreiber fertig, bevor der Leser erfolgreich prüft, bestehen nur Endzustandsassertionen. **Empfehlung:** deterministischer Start-/Interleaving-Barrierepunkt, tatsächlich beobachtete alte/neue kohärente Generationen verlangen; unzulässige Mischsignatur absichtlich einschleusen und erwartetes Testversagen belegen.

**T04 – HOCH für behauptete Atomizitätsfreigabe: Concurrent-Restore-Smoke hat kein Generations-/Musikorakel.** [Host:1497](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp#L1497) lädt sechsmal denselben Kitstate, während ein anderer Thread Audio verarbeitet. Er verlangt positive Blockzahl, Erfolg und endliche Samples. Durchgehend null oder gemischte, aber endliche Audio-/Parameterstände können bestehen; automatische Patternpunkte werden in diesem Test nicht eingespeist. **Empfehlung:** zwei klar verschiedene Kits mit unterscheidbaren Samples, Patterns, Mix/Outputs und selektierten Programmen; erzwungene Überlappung, Automation/MIDI zwischen konkreten Barrieren und eine pro Block zulässige Gesamtsignatur. Gate-/Stummintervalle gesondert zählen. Die vorhandene Prüfung bleibt nützlich als grober Crash-/Finite-Smoke, nicht als vollständiger Transaktionsbeweis.

### 10.3 Zusätzliche Regressionen vor funktionaler Freigabe

1. **A01–A09 produktnah nachstellen**, einschließlich ihrer negativen Fälle und Ergebnis-Erhalt bei abgelehnter Nachricht. Tests am realen Dispatcher/Controller/Prozessor statt ausschließlich an nochmals nachprogrammierten Lambdas.
2. **Native Windows-GUIfixture:** HWND + echter Attach + `IPlugFrame`, Paint und Eingaben; 100/150 %, DPI-Wechsel, Annahme/Ablehnung, 1/4/8/16 Rows, alle32 Steps, Overlayfokus, Textinput, Lifecycle. Alle42 Gruppen mit Rand-/Gapkoordinaten iterieren.
3. **Win32-Dateiformatfixture:** `IDataObject` mit gleichzeitig CF_UNICODETEXT-Anzeigename und CF_HDROP-Pfad durch den tatsächlichen gepatchten `Win32DataPackage` schicken; erwartetes `kFilePath`, UTF-8-Pfad und richtige Row. Nicht nur `extractSamplePath()` mit vorbereitetem Text aufrufen.
4. **Import-/Statefehler:** beschädigte Mehrframe-FLACs unbekannter Länge, ungültige UTF-8-Payloads, echte Ordnernavigation mit fehlerhaften Kandidaten, Zieldatei während Auswahl gelöscht, OOM-/Budgeterhalt, abgeschnittene Streams/CRC/unterstützte Altformate in echter frischer und bereits benutzter Instanz.
5. **Automation/Timing:** >256 Punkte, Nullframes, A→B→A, gleichwertiges Event während Recall, Host-/MIDI-Gleichoffset, Tempo-/PPQ-Sprünge und variable Blöcke gegen unabhängigen Sample-Onset-Oracle.
6. **Echtzeit:** Gesamtcallback und Freigaben instrumentieren, reale Deadlineüberläufe statt nur20s-Gesamtlauf prüfen; Kit-/Samplewechsel und mehrere Instanzen während GUI-/Disklast.
7. **Releasechecks tatsächlich aktiv halten:** absichtliche negative Fälle für neue Orakel; `NDEBUG` darf keine Akzeptanzbedingungen entfernen. Keine bestehenden Tests umschreiben oder Referenzen lockern, nur um Grün zu erzielen.

## 11. Verbleibende Studio-One-/Studio-Pro-Praxistests

Automatisierbare Produktfehler sollen vor diesen Besitzerprüfungen bereits reproduziert und gezielt korrigiert sein. Die konkrete Hostumgebung, Windowsbuild, installierte Bundle-SHA und Pluginversion aufzeichnen. Der frühere vom Besitzer bestätigte normale D&D-Erfolg am Referenzstand `145c4fceca7eb2c4693fcbc0376f2e2c030cbe28` wird anerkannt; er ist kein aktueller Test aller Overlay-/Zoom-/Recallkombinationen.

| Praktischer Ablauf | Klare Akzeptanz |
|---|---|
| Instanz frisch einfügen; Editor öffnen/schließen, mehrfach und während Playback | Kein Absturz/Leak/Stale-View, korrekte sichtbare Instrumente und letzte Steps; Fokus geht nach Dialogen zurück. |
| 100/150 %, verschiedene Windows-DPI/Monitore, kleine Fenster, Resize-Ablehnung | Hostrect/Frame/Child konsistent, keine verdeckten wesentlichen Controls/Statuszeile; Rollback ohne Modellabweichung. |
| WAV/FLAC aus Explorer und Hostbrowser, Unicode/Leerzeichen, falsche/fehlende Datei | Genau sichtbare Row 1/4/8/16 beziehungsweise Compactpage; Fehler erhält Sample. Kein Laden unter Song-/Fill-Overlay. |
| Name: Shift, Escape, Enter, Klick außerhalb, Projekt-Recall bei offenem Feld | Abschluss ohne hängenbleibendes Textoverlay, keine falsche Rowumbenennung/Dirtyänderung beim Abbruch. |
| Copy/Paste/Clear/Undo, alle Fills, Velocity-Drag und Ctrl-Erase | Ausschließlich vorgesehenes Pattern/Row, ein Undo stellt ganze Geste wieder her, Projekt speichert Ergebnis. |
| A–H per GUI, MIDI und Automationsspur, auch Zwischenwerte | Hörbares Pattern, Hosttext, GUIauswahl und Editierziel stimmen; keine Rückkopplung oder verlorene Endwerte. |
| Start Takt1/2/beliebige Position, Seek/Loop/Stop-Start, Tempoänderung | Nach dokumentiertem Ursprung richtige32-/16-Step-Phase, keine Doppeltrigger oder verschwundenen Downbeats. |
| Restore unter Playback, MIDI/Automation gleichzeitig; direkt danach Stop | Nur ausdrücklich zulässiger Kitübergang, kein Mischkit; neuere Auswahl nach festgelegter Priorität erhalten; Stop beendet vorgesehene Stimmen. Stummdauer messen. |
| 16 Routingziele, mehrere Rows gemeinsam, MAIN deaktiviert, AUX-Lücken und dynamisches Umschalten | Eindeutige L/R-Signatur an genau vorgesehenen Bussen, keine Doppelung, dokumentierter MAIN-Fallback und Tailvertrag; State nach Neustart identisch. |
| Mute/Solo/Choke, volle64 Voices, Pitch-/Sampleratenwechsel | Neue Trigger korrekt gesperrt, Tails gemäß Vertrag, abgewiesener Voicehit choked nichts; keine Klick-/CPUausreißer ohne Dokumentation. |
| Projekt speichern, DAW vollständig beenden, Originalsamples verschieben/löschen, neu starten | Eingebettete Samples, alle Patterns, Song, Mix, Output, Name und Zoom korrekt; Legacyprojekt erhält definierte Defaults. |
| Gestoppter Transport mit Preview, mehrere Instanzen, Offlinebounce | Tatsächlicher Hostcallback ermöglicht Preview; Offline-/Realtime-Ausgabe nach neutralisierten Zufallseinstellungen vergleichbar. |
| Längerer Betrieb mit kleinen Puffern und gleichzeitigem Kit-/GUIbetrieb | Deadline-/Dropout-/p99-/Max-Protokoll, Peak-RAM, keine Callbackallokationen; Ergebnisse an exaktes Bundle binden. |

Steinberg Validator, ein 125A-Plugin-Tester-Report und Studio-One-Abnahme sind getrennte Nachweise. Ein Validator-PASS sagt nichts über falsch zugeordnete Mausflächen, fehlenden MIDI-Controllerabgleich oder einen mit zu großer Toleranz geprüften Bus aus.

## 12. Reproduzierbare Gegenbeispiele und Freigabereihenfolge

Die folgenden unabhängigen Ablaufmodelle verwenden die aus dem aktuellen C++ gelesenen Verzweigungen/Formeln. Sie liefern kleine, prüfbare Gegenbeispiele; die später erforderlichen Tests müssen denselben Fall **am Produkt** prüfen.

```python
# A02: wörtliche Overflowstrategie aus addPattern, kein C++-Plugintest
edits, overflow = [], False
for offset in range(257):              # gültig in einem 512-Frame-Block
    if overflow:
        continue
    if len(edits) == 256:
        edits, overflow = [], True
        continue
    edits.append((offset, 1))           # alle Punkte wählen B
assert overflow and edits == []        # Prozessor bleibt bei ursprünglichem A

# A03: aktueller Resetpfad addiert -current, Default wäre100
for current in (0, 50, 100, 200):
    assert max(0, min(200, current - current)) == 0

# A05: der sichtbare Songpunkt wird als Hintergrundinstrument2 erkannt
assert 16 <= 700 <= 1230 and 108 <= 150 < 108 + 4*29
assert (150-108)//29 == 1

# A06: alte Einzeilenhöhe vs fester Vierzeilenfallback mit Lane
old_height = max(326, 108+29+220)        # 357
fallback_bottom = 108+4*29+165          # 389
assert fallback_bottom-old_height == 32

# A07: derselbe einstufige Undo-Platz wird zweimal überschrieben
value, undo = 20, None
for next_value in (40, 80):
    undo, value = value, next_value
assert undo == 40                      # vor der ganzen Geste war20

# A09: sichtbarer Gap wird trotzdem PatternindexA
assert 165 < 167 < 171
assert (167-115)//56 == 0
```

Für A01 zeigt die Quellprüfung die fehlende Rückmeldung trotz `selected_.store(...)`; für A04 ist der nicht ausgelöste SDK-`valueChanged`-Zweig entscheidend; für A08 fehlt im Controllerstate-Pfad jede Aktualisierung bestehender Editoren. Diese Befunde benötigen keine erfundene Hörprobe, um den fehlenden Pfad aufzuzeigen. Die genaue native Wirkung und Reparaturregression bleiben gesondert nachzuweisen.

**Empfohlene Reihenfolge nach Bewertung durch den Besitzer/ChatGPT:**

1. A01/A02 mit echten VST3-Regressionen korrigieren; Prioritätsvertrag R01 festlegen und deterministisch prüfen.
2. A03–A08 am tatsächlichen GUI-/Controllerpfad korrigieren; A09 bei der gemeinsamen Geometrie mitbereinigen. T01–T04 verstärken und jeweils einen Negativnachweis führen.
3. Restore-/RAM-/FLAC-/Timingrisiken mit den gezielten Fixtures und paced Callbackmessungen schließen. Erst danach eine funktionale technische Freigabe erwägen.
4. Korrigierten Stand mit exakt zugeordnetem Windows-Build, Validator und paketierten nativen Tests prüfen. Keine spekulativen Actionsläufe für bloße Auditdokumentation.
5. Besitzerabnahme in Studio One, konsolidierte Versions-/Artifactprovenienz und getrennte Freigabe der endgültigen GUI. Ein weiteres grünes Gesamtergebnis allein schließt diesen Bericht nicht.

**Abschlussurteil:** Die bisherige Arbeit enthält echte und fachlich sinnvolle Reparaturen, ist aber **funktional noch nicht abgeschlossen**. Die Freigabeantwort bleibt **NEIN**. Dieser Audit hat ausschließlich den Bericht erstellt; er enthält Empfehlungen und keinerlei angewandte Quellcode-, Test- oder Designreparatur.

## 13. Inventar der vollständig einbezogenen Implementierungs- und Testdateien

Nachfolgend sind alle 56 Dateien des geprüften Implementierungs-/Test-/Build-/Ressourcenbestands aufgeführt. Dokumentationsberichte und Engineering-/SDK-Referenzen kommen ergänzend hinzu. Die Zeilenzahl dient als prüfbarer Größenhinweis, nicht als Beweis einer Laufzeitverifikation. Die ersten zwölf Hexzeichen sind der SHA-256-Präfix des Inhalts; die exakte Git-Basis oben identifiziert den vollständigen Inhalt.

| Datei | Zeilen | SHA-256-Präfix |
|---|---:|---|
| [.github/workflows/core-ci.yml](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/.github/workflows/core-ci.yml) | 116 | `edcdec0a19ae` |
| [CMakeLists.txt](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/CMakeLists.txt) | 227 | `b682603fda34` |
| [cmake/PatchVstguiWin32Drop.cmake](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/cmake/PatchVstguiWin32Drop.cmake) | 177 | `c8d9802b64ea` |
| [resource/Drumator.uidesc](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/resource/Drumator.uidesc) | 36 | `171d9c645a14` |
| [resource/VSTGUI_ASSET_SLOTS.json](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/resource/VSTGUI_ASSET_SLOTS.json) | 596 | `c3c2ac051df1` |
| [src/core/AtomicPatternBank.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternBank.h) | 102 | `c46c57e81e0b` |
| [src/core/AtomicPatternSelection.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/AtomicPatternSelection.h) | 37 | `b1c7501f230a` |
| [src/core/CompactRowViewport.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/CompactRowViewport.h) | 26 | `d619e7286550` |
| [src/core/FlacImport.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/FlacImport.h) | 10 | `e638d8b33574` |
| [src/core/HostEditDirty.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/HostEditDirty.h) | 9 | `9b756a678289` |
| [src/core/KitRestoreGate.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/KitRestoreGate.h) | 45 | `a775b7f2da34` |
| [src/core/KitState.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/KitState.h) | 276 | `aa7d2b8dbd2a` |
| [src/core/PatternOperations.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/PatternOperations.h) | 34 | `02941fe4077a` |
| [src/core/PatternQuantization.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/PatternQuantization.h) | 12 | `8a1f47073fcd` |
| [src/core/PatternRestoreLatch.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/PatternRestoreLatch.h) | 25 | `18c5b00041ea` |
| [src/core/PatternUndo.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/PatternUndo.h) | 23 | `b48d1aee1647` |
| [src/core/RowFill.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/RowFill.h) | 26 | `65d4607802b3` |
| [src/core/RowMixControl.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/RowMixControl.h) | 13 | `6251ea26fac0` |
| [src/core/RowMuteSolo.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/RowMuteSolo.h) | 18 | `fa0a8e93aab4` |
| [src/core/RowResizeTransaction.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/RowResizeTransaction.h) | 12 | `07563d35e864` |
| [src/core/SampleExchange.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SampleExchange.h) | 233 | `68e40c2d5dd3` |
| [src/core/SampleFolderNavigation.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SampleFolderNavigation.h) | 23 | `5edc101f456e` |
| [src/core/SampleVoiceEngine.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SampleVoiceEngine.h) | 309 | `c0e7a60d94c6` |
| [src/core/SequencerCore.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SequencerCore.h) | 142 | `4d713f3ff50e` |
| [src/core/SongChain.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/SongChain.h) | 52 | `c1791a4a0246` |
| [src/core/StepVelocity.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/StepVelocity.h) | 9 | `d65d5cc5fc5f` |
| [src/core/TriggerBatch.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/TriggerBatch.h) | 85 | `6129e678d156` |
| [src/core/UiHeaderHitZones.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/UiHeaderHitZones.h) | 18 | `aa53de96076e` |
| [src/core/Utf8Path.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/Utf8Path.h) | 22 | `d025ef3f2fb9` |
| [src/core/VelocityLane.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/VelocityLane.h) | 43 | `f7ef379f655d` |
| [src/core/VisibleRows.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/VisibleRows.h) | 30 | `a612c2623a2a` |
| [src/core/WavPcm16Decoder.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/core/WavPcm16Decoder.h) | 137 | `ad2b62cd6c19` |
| [src/vst3/DrumatorController.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.cpp) | 721 | `f42e994cab41` |
| [src/vst3/DrumatorController.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorController.h) | 135 | `9b676937fa10` |
| [src/vst3/DrumatorEditor.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorEditor.h) | 53 | `92bf1dbc0222` |
| [src/vst3/DrumatorProcessor.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.cpp) | 895 | `19c5214d5d56` |
| [src/vst3/DrumatorProcessor.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorProcessor.h) | 124 | `cf39fc7f3884` |
| [src/vst3/DrumatorView.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.cpp) | 1004 | `155cf9ba4363` |
| [src/vst3/DrumatorView.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/DrumatorView.h) | 58 | `b30d530b31d0` |
| [src/vst3/Factory.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/Factory.cpp) | 28 | `59635ac66010` |
| [src/vst3/FlacImport.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/FlacImport.cpp) | 81 | `d25b3f24633a` |
| [src/vst3/PluginIDs.h](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/src/vst3/PluginIDs.h) | 9 | `e19feedc5d57` |
| [tests/AtomicPatternTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/AtomicPatternTests.cpp) | 197 | `5e61dcccc3a2` |
| [tests/CoreTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/CoreTests.cpp) | 330 | `bd4d42ff3bf1` |
| [tests/EditorGeometrySmoke.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/EditorGeometrySmoke.cpp) | 82 | `05d4810aaa1a` |
| [tests/KitStateTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/KitStateTests.cpp) | 371 | `396953987f62` |
| [tests/PackagedAudioHostSmoke.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PackagedAudioHostSmoke.cpp) | 1637 | `60fd9727e2ca` |
| [tests/PatternUndoTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/PatternUndoTests.cpp) | 18 | `c92ef4af4c95` |
| [tests/SampleExchangeTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleExchangeTests.cpp) | 173 | `c977d083e5a1` |
| [tests/SampleVoiceTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SampleVoiceTests.cpp) | 442 | `a1434223cdf1` |
| [tests/SongChainTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/SongChainTests.cpp) | 31 | `674a3ffa06b5` |
| [tests/TriggerBatchTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/TriggerBatchTests.cpp) | 69 | `58a14ef4090b` |
| [tests/UiHeaderHitZonesTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/UiHeaderHitZonesTests.cpp) | 28 | `0eed9ef0fae6` |
| [tests/VelocityLaneTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/VelocityLaneTests.cpp) | 27 | `f6484d501cba` |
| [tests/VisibleRowsTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/VisibleRowsTests.cpp) | 122 | `8b0e099fae93` |
| [tests/WavDecoderTests.cpp](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/tests/WavDecoderTests.cpp) | 129 | `bb5af242cdc6` |

## 14. Veröffentlichungsumfang und Referenzen

Dieser Bericht gehört auf `audit/codex-drumator-2026-10-19`. Sein Commit soll gegenüber dem Branch-Elternstand **ausschließlich** `docs/audits/CODEX-INDEPENDENT-AUDIT-2026-10-19.md` hinzufügen. `develop/v1.0.0`, Pluginquellcode, vorhandene Tests, Workflows und Engineering-Regeln werden nicht geändert. Bericht-Commit und überprüfter Produktcommit sind verschiedene Identitäten; der Bericht erklärt nicht seinen eigenen Dokumentationscommit zum erneut getesteten Plugin.

- [Verbindlicher Engineering-Einstieg](https://github.com/challanger2000/125A-Engineering/blob/2eb41eadea88c32d774e918ee2d71756f81a4459/START-HERE.md)
- [Geprüfte projektspezifische Regeln](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/AGENTS.md)
- [Vollständiger Selbst-Audit einschließlich Aktualisierung](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/docs/SELF-AUDIT-FOR-CODEX-2026-10-19.md)
- [Ursprünglicher technischer Audit](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/TECHNISCHER-AUDIT.md)
- [Historischer Abschluss-Audit, ausdrücklich kein aktueller Produktstatus](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/CODEX-FINAL-AUDIT.md)
- [D&D-Referenz und historische Besitzerbestätigung](https://github.com/challanger2000/125A-Drumator/blob/ee712dd094e28fdc9bd6da9917f249aa7ef7b185/docs/DRAG-DROP-VERIFIED-REFERENCE-2026-10-19.md)
- [Gepinnter VSTGUI-Texteditorvertrag, Fokusabschluss und Änderungsereignis](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/controls/ctextedit.cpp#L378)
- [Gemeldeter erfolgreicher Windowslauf am identischen Produktionscode](https://github.com/challanger2000/125A-Drumator/actions/runs/39324490951)

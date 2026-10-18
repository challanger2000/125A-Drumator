# Codex – Drag-and-drop-Analyse: Drumator und Phraseator

**Datum:** 19.10.2026, Europe/Berlin  
**Drumator:** `challanger2000/125A-Drumator`, Branch `develop/v1.0.0`, geprüfter Commit `1d00be07446721f7d401ff8387d8f41466281dc2`  
**Funktionierende Referenz laut Auftraggeber:** `challanger2000/125A-Phraseator-Final`, Branch `main`, geprüfter Commit `b843d5e1756bb4fd89053633fa546cdcf05e5a65`  
**VST3 SDK:** `v3.8.1_build_84`, Commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`  
**VSTGUI:** Commit `5db272256172557818b6158cf0bb2c4410bddb25`  
**Auftrag und dokumentierter Fehlerverlauf:** [Drumator Issue #3](https://github.com/challanger2000/125A-Drumator/issues/3)

## Ergebnis und Nachweisgrenze

**Der entscheidende Unterschied ist gefunden: Im Drumator fehlt der Windows-Kompatibilitätspatch des Phraseators für die Auswahl des Drag-and-drop-Datenformats.** Die bisherigen Reparaturen haben überwiegend den nachgelagerten View-Code verändert.

Die frühere Aussage im Chat, die Einbindung sei korrekt, war unvollständig: Sie berücksichtigte die vorgelagerte Windows-Datenkonvertierung nicht ausreichend.

`125A-Engineering/START-HERE.md`, die einschlägigen Engineering-/QA-Regeln sowie die projektspezifischen Vorgaben wurden berücksichtigt. Die Analyse erfolgte ausschließlich lesend. Quellcode, Tests und SDK wurden nicht verändert; es wurden keine Builds oder nativen Hosttests ausgeführt. Dieser Bericht und seine Veröffentlichung sind Dokumentation, keine Umsetzung der vorgeschlagenen Reparatur.

Die fehlende Übernahme und der daraus folgende Fehler für gleichzeitig angebotene Text-/Dateiformate sind im tatsächlichen Quellcode nachweisbar. Das konkrete native Datenpaket des letzten Studio-Pro-Fehlversuchs wurde in Issue #3 nicht aufgezeichnet. Deshalb wird nicht behauptet, damit bereits jeden beobachteten Fehlversuch nativ reproduziert oder die gesamte Reparatur praktisch verifiziert zu haben.

Statusbegriffe:

- **FAIL:** konkreter Fehlerpfad beziehungsweise fehlender erforderlicher Referenzbestandteil nachgewiesen; Umfang jeweils unten beschrieben.
- **TEILWEISE:** Implementierung vorhanden, vollständige Integration nicht verifiziert.
- **OFFEN:** zusätzlicher nativer Integrationsnachweis erforderlich.
- **PASS:** nur für tatsächlich ausgeführte und geeignete Tests; ein solcher D&D-Host-PASS wird hier nicht vergeben.

## DND-01 – HOCH – FAIL: Dateipfad geht bei konkurrierenden nativen Formaten vor dem Drop-Handler verloren

### Ursache

Der unveränderte VSTGUI-Windows-Adapter fragt zuerst `CF_UNICODETEXT` ab. Nur wenn dieses Format nicht angeboten wird, fragt er `CF_HDROP` ab. Eine erfolgreiche Formatabfrage für Text verhindert damit die Prüfung einer ebenfalls verfügbaren echten Dateiliste.

Primärbeleg der exakt gepinnten Abhängigkeit: [win32datapackage.cpp:32](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32datapackage.cpp#L32), alternativer Dateipfadzweig ab [Zeile 53](https://github.com/steinbergmedia/vstgui/blob/5db272256172557818b6158cf0bb2c4410bddb25/vstgui/lib/platform/win32/win32datapackage.cpp#L53).

### Konkretes Fehlerszenario

```text
Der Browser bietet gleichzeitig an:
CF_UNICODETEXT → "Kick.wav"                 (Anzeigename)
CF_HDROP       → "C:\Samples\Kick.wav"      (existierende Datei)

Drumator mit unverändertem VSTGUI:
Text wird ausgewählt → Dateiliste wird nicht mehr abgefragt.
Der View erhält nur "Kick.wav".
Liegt diese relative Datei nicht im Arbeitsverzeichnis, schlägt die
Existenzprüfung fehl → Drop wird abgelehnt.
```

Der echte Dateipfad wäre verfügbar, wird aber vor dem Aufruf des Plugin-Handlers verborgen. Eine weitere Erweiterung des Plugin-Textparsers kann einen nicht weitergegebenen Dateipfad nicht wiederherstellen.

### Funktionierende Referenz und Nachweis

Der Phraseator besitzt genau hierfür einen CMake-Patch. Er bevorzugt `CF_HDROP`; Unicode-Text bleibt als Ausweichformat erhalten.

- Referenzimplementierung: [cmake/PatchVstguiWin32Drop.cmake:87](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/cmake/PatchVstguiWin32Drop.cmake#L87).
- Tatsächlicher Aufruf im Build: [Phraseator CMakeLists.txt:131](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/CMakeLists.txt#L131).
- Drumator-SDK-Einbindung ohne entsprechenden Patchaufruf: [Drumator CMakeLists.txt:103](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/CMakeLists.txt#L103).

Zusätzliche lesende Prüfung: Der vom Phraseator-Patch erwartete Originalblock kommt im exakt gepinnten Drumator-SDK **genau einmal und unverändert** vor. Eine ausschließlich im Arbeitsspeicher vorgenommene Zeichenkettenersetzung ergab die vorgesehene Priorität Dateiliste vor Text. SDK-Dateien wurden dabei nicht geschrieben oder kompiliert. Dies belegt die Anwendbarkeit auf den geprüften Quellstand, keinen nativen Windows-Test.

**Empfehlung:** Den bestehenden Phraseator-Patch unverändert übernehmen und in der Drumator-Konfiguration auf dieselbe SDK-Quelldatei anwenden. Kein SDK-Upgrade und kein neuer selbst entwickelter Windows-Drop-Handler.

**Erforderlicher Regressionstest:** Tatsächlichen `Win32DataPackage` mit synthetischem Windows-`IDataObject` testen, das beide Formate anbietet. Der unvollständige Text darf den gültigen Dateipfad nicht verdrängen. Details unter Testplan.

## Vollständiger Vergleich des Importwegs

| Komponente | Funktionierender Phraseator | Aktueller Drumator | Bewertung |
|---|---|---|---|
| Windows-Datenadapter | Gepatcht: Dateiliste vor Text | Unverändert: Text vor Dateiliste | Entscheidender fehlender Bestandteil, DND-01 |
| Editor-/View-Erzeugung | `createView` → `createCustomView` → konkrete Drop-View | `createView` → `createCustomView` → `DrumatorView` | Erzeugung vorhanden |
| Drop-Fläche | Separate mausaktive `SourceDropView` über schreibgeschützter Statusanzeige | Mausaktive, ungebundene `DrumatorView` | Phraseators Read-only-Problem liegt hier nicht vor |
| Drop-Interface | `CView` und `IDropTarget`, `getDropTarget()` liefert `this` | Gleiche aktive Anbindung | Bereits übernommen |
| Ereignisse | Enter/Move prüfen Pfad; Drop startet Laden | Entsprechende aktive Callbacks vorhanden | Kein weiterer Interface-Umbau begründet |
| Pfaderkennung | Text/Dateipfad, NUL-Ende, Anführungszeichen, Datei-URL, Existenzprüfung | Praktisch identischer Code, ergänzt um FLAC | Erneute Parser-Neuentwicklung behebt vorgelagerten Fehler nicht |
| Zielzuordnung | Fester Source-Index aus View-Tag | Instrumentenzeile aus Position und `firstRow_` | Notwendige Drumator-Anpassung |
| Sampleübergabe | Controller sendet Pfad; Prozessor beauftragt Lade-Worker | Controller liest WAV/FLAC und sendet WAV-Daten an Prozessor | Unterschiedliche vorhandene Samplerarchitektur |
| Drop-Rückgabe | Ergebnis der Controller-Anfrage | Nach Ladeversuch immer `true` | Zusätzliche Abweichung, DND-02 |

### Phraseator: View-Erzeugung und tatsächliche Hierarchie

- [PhraseatorController.cpp:276](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorController.cpp#L276): Erzeugung des Editors und Weiterleitung der Custom-View-Erzeugung.
- [PhraseatorViews.cpp:1125](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorViews.cpp#L1125): Übernahme von Ursprung und Größe aus den UI-Attributen.
- [PhraseatorViews.cpp:1164](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorViews.cpp#L1164): separate ungebundene Drop-View, weil die an schreibgeschützte Parameter gebundenen Status-Controls von VSTGUI für Mausbedienung deaktiviert werden können.
- [Phraseator.uidesc:27](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/resource/Phraseator.uidesc#L27): Statusansicht und darüber angeordnete `PhraseSourceDrop` mit derselben Geometrie und Source-Zuordnung.
- [PhraseatorViews.h:100](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorViews.h#L100): `SourceDropView` implementiert `IDropTarget`, `getDropTarget()` liefert die View selbst.
- [PhraseatorViews.cpp:681](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorViews.cpp#L681): mausaktive transparente View, Enter/Move/Leave/Drop, Source-Zuordnung und Übergabe an Controller.

### Phraseator: Pfad und Sample-Ladevorgang

[PhraseatorViews.cpp:446](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorViews.cpp#L446) normalisiert Text- und Dateipfadpakete, entfernt End-NUL/Zeilenenden, berücksichtigt Anführungszeichen und Datei-URLs einschließlich Prozentkodierung und prüft, ob eine reguläre WAV-Datei existiert.

[PhraseatorController.cpp:582](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorController.cpp#L582) nimmt den UTF-8-Pfad entgegen, wandelt ihn in Steinbergs Stringformat um und sendet über `sendLoadSampleMode` die Nachricht `Phraseator.LoadSample` mit Source-Zuordnung.

[PhraseatorProcessor.cpp:289](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/vst3/PhraseatorProcessor.cpp#L289) validiert die Nachricht und übergibt einen `SampleLoadRequest` an den vorhandenen Lade-Worker. Dieser liest und decodiert außerhalb des Audiothreads und veröffentlicht die vorbereitete Samplebank; siehe [sample_file_loader.cpp:9](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/source/sample_file_loader.cpp#L9) und [sample_load_worker.cpp:349](https://github.com/challanger2000/125A-Phraseator-Final/blob/b843d5e1756bb4fd89053633fa546cdcf05e5a65/source/sample_load_worker.cpp#L349).

Der Rückgabewert des Phraseator-Drops bestätigt die Annahme der Controller-/Ladeanfrage. Er ist keine Behauptung, dass die asynchrone Verarbeitung bereits hörbar abgeschlossen ist.

### Drumator: bereits übernommene Teile und erforderliche Anpassungen

- [DrumatorController.cpp:172](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorController.cpp#L172): Erzeugung von Editor und `DrumatorStepGrid`.
- [Drumator.uidesc:21](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/resource/Drumator.uidesc#L21): mausaktive ungebundene `CView`, Ursprung `(0,0)`.
- [DrumatorView.h:18](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.h#L18): aktives View-eigenes Drop-Interface.
- [DrumatorView.cpp:69](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.cpp#L69): übernommene Phraseator-Pfaderkennung mit WAV-/FLAC-Endungen.
- [DrumatorView.cpp:217](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.cpp#L217): aktive View-Callbacks für Enter/Move/Drop.
- [DrumatorView.cpp:472](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.cpp#L472): Begrenzung auf sichtbare Instrumentenzeilen und Zuordnung mit `firstRow_`.
- [DrumatorController.cpp:478](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorController.cpp#L478): WAV-/FLAC-Import, FLAC-Konvertierung, Decoderprüfung und Übergabe über `Drumator.LoadPcm16Wav`.
- [DrumatorProcessor.cpp:265](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorProcessor.cpp#L265): Prüfung von Zeile, WAV-Daten und Namen sowie Vorbereitung und Veröffentlichung des Samples.

Die logische Zeilenberechnung lautet nach Prüfung der Drop-Grenzen:

```cpp
row = firstRow_ + static_cast<std::size_t>((position.y - 108.0) / 29.0);
```

Im aktuellen Aufbau liegen Grid und Elterncontainer bei Ursprung `(0,0)`. Die zusätzliche `frameToLocal`-Umrechnung erneut einzuführen ist nicht begründet. Die gleichnamigen Callbacks der alten Hilfsklasse `DrumatorDropTarget` sind nicht installiert; aktiv sind die Methoden von `DrumatorView`. Die Hilfsklasse wird weiterhin für die statische Pfaderkennung verwendet.

Für die Reparatur der Drag-and-drop-Anbindung ist kein Austausch der gesamten Drumator-Samplerarchitektur durch Phraseators Worker-/Source-Pool-Architektur erforderlich. Die notwendigen Drumator-Unterschiede bleiben Instrumentenzeile, WAV/FLAC und der vorhandene Controller-/Prozessor-Ladepfad.

## DND-02 – MITTEL – FAIL: Drop meldet Annahme auch bei abgelehntem Sampleimport

**Dateien:** [DrumatorView.cpp:450](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.cpp#L450), [DrumatorView.cpp:491](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.cpp#L491), [DrumatorView.h:35](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/src/vst3/DrumatorView.h#L35).

`loadPath` verarbeitet den tatsächlichen `LoadStatus`, liefert ihn aber nicht zurück. `acceptFileDrop` liefert nach diesem Ladeversuch unabhängig vom Ergebnis `true`.

**Fehlerszenario:** Eine existierende Datei mit WAV-Endung passiert die Pfadprüfung, wird aber als beschädigtes WAV oder wegen fehlender Prozessorverbindung abgelehnt. Die Statuszeile zeigt den Fehler; das Drop-Protokoll erhält dennoch Erfolg. Der Phraseator gibt dagegen das Ergebnis der Controller-Anfrage zurück.

Dies erklärt keine fehlenden Drag-Ereignisse und ist von DND-01 getrennt. Es ist eine unzureichende Weitergabe des tatsächlichen Anfrageergebnisses.

**Empfehlung:** `loadPath` soll nach unveränderter Statusbehandlung einen booleschen Annahmewert liefern. `acceptFileDrop` gibt diesen Wert zurück.

## Präziser Reparaturvorschlag – nicht umgesetzt

### A. Vorhandenen Phraseator-SDK-Patch übernehmen

`cmake/PatchVstguiWin32Drop.cmake` aus dem geprüften Phraseator-Commit unverändert übernehmen, einschließlich Prüfung auf den erwarteten SDK-Block und Behandlung bereits gepatchter Dateien.

Im Drumator direkt nach der SDK-Pfadnormalisierung in `CMakeLists.txt:107` einbinden:

```cmake
if(WIN32)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchVstguiWin32Drop.cmake")
  phraseator_patch_vstgui_windows_drop(
    "${_DrumatorSdkRoot}/vstgui4/vstgui/lib/platform/win32/win32datapackage.cpp"
  )
endif()
```

Der Funktionsname kann für die unveränderte Übernahme bestehen bleiben. Kein SDK-Upgrade, kein zusätzlicher nativer Windows-Drop-Handler und keine neue Parservariante. Der gezielte SDK-Konfigurationspatch wird bewusst genauso wie in der funktionierenden Referenz angewandt.

### B. Ladeergebnis zurückgeben

Änderungsausschnitt, keine bereits angewandte Reparatur:

```diff
// DrumatorView.h
- void loadPath(std::size_t row,const std::string& path);
+ bool loadPath(std::size_t row,const std::string& path);

// DrumatorView.cpp
-void DrumatorView::loadPath(std::size_t row,const std::string& path) {
+bool DrumatorView::loadPath(std::size_t row,const std::string& path) {
   // Bestehenden Ladeaufruf und vollständige Statusbehandlung beibehalten.
   // ...
   invalid();
+  return outcome == Controller::LoadStatus::Accepted;
 }

// Am Ende von acceptFileDrop:
- loadPath(row,path);
- return true;
+ return loadPath(row,path);
```

Der Ausschnitt zeigt die präzisen Änderungen; der vorhandene Switch mit sämtlichen Fehlermeldungen bleibt vollständig erhalten. Bestehende Dateidialog-Aufrufer können den Rückgabewert ignorieren.

### C. Vorhandene Drumator-Anpassungen erhalten

- WAV und FLAC weiterhin über dieselbe bestehende Importfunktion laden.
- Zeile weiterhin aus gültigem Dropbereich, logischer Y-Position und `firstRow_` bestimmen.
- Keine zusätzliche manuelle Zoomumrechnung einführen.
- Kein neues schreibgeschütztes Parameter-Control als Drop-Empfänger einsetzen.
- Keine zusätzlichen Overlay-Views oder Samplerumbauten ohne einen entsprechenden Integrationsnachweis einführen.

## Automatische Regressionstests

Die folgenden Tests sind erforderlich beziehungsweise empfohlen; sie wurden während dieser lesenden Analyse nicht implementiert oder ausgeführt.

### T01: Native Windows-Formatwahl – entscheidender Nachweis

Ein synthetisches COM-`IDataObject` bietet gleichzeitig einen bloßen Anzeigenamen als `CF_UNICODETEXT` und eine existierende Datei als `CF_HDROP` an. Den tatsächlich kompilierten `Win32DataPackage` der gepinnten SDK-Abhängigkeit verwenden.

**Erwartung:** Der vollständige Dateipfad wird als `kFilePath` geliefert; die Dateiliste verdrängt den unvollständigen Text. Der ungepatchte Adapter muss diesen Test verfehlen, der Phraseator-Patch ihn bestehen. Auch nur Dateiliste sowie nur Text als getrennte Fälle prüfen.

Ein erst hinter dem Adapter konstruiertes `IDataPackage` reicht hierfür nicht aus: Es würde den Hauptfehler vollständig übergehen. Dieser Test ist der minimale automatisierte Nachweis für die konkrete kausale Lücke zwischen den beiden Implementierungen.

### T02: Patch-Anwendung

Erwarteten SDK-Block prüfen. Zweimalige Anwendung muss stabil bleiben. Bei unbekanntem SDK-Block ohne gültige Bereits-gepatcht-Markierung muss die Konfiguration ausdrücklich abbrechen. Auf derselben SDK-Version konfigurieren, die das ausgelieferte Plugin tatsächlich verwendet.

### T03: Tatsächliche View-Hierarchie und Koordinaten

Den echten Drumator-Editor an ein Windows-Fenster anhängen. Enter/Move/Drop über den Frame zustellen, nicht ausschließlich direkt `acceptFileDrop` aufrufen.

- Richtige Instrumentenzeile bei 100 und 150 Prozent.
- Mehrere Instrumentenzahlen einschließlich letzter sichtbarer Zeile.
- Host-Resize verweigert, kompakter Zeilenmodus und weitergeschalteter `firstRow_`.
- Grenzen oberhalb/unterhalb der Zeilen sowie außerhalb der zulässigen X-Fläche.
- Bei kombiniertem Host-DPI und Benutzerzoom weiterhin korrekte Zuordnung.
- Mausaktivität und tatsächliches Drop-Ziel der erzeugten Custom-View prüfen.

### T04: Pfadformate und Dateien

`kFilePath`, Textpfad, NUL-Ende, Anführungszeichen und Datei-URL mit Leerzeichen/Umlauten prüfen; jeweils WAV und FLAC. Fehlende Dateien, ungültige Endungen und reine Anzeigenamen sicher ablehnen. Groß-/Kleinschreibung der unterstützten Dateiendungen berücksichtigen.

### T05: Vollständiger Importpfad

Drop → echter Controller → verbundener Prozessor → tatsächliche Audioausgabe. Instrumentenzeilen mit unterscheidbaren Samples belegen; Zielzeile, Dateiname, Wiedergabe und unveränderte Nachbarzeilen kontrollieren. Annahme der Anfrage und hörbare Übernahme am Audioblock getrennt prüfen.

### T06: Fehlererhalt und Rückgabe

Beschädigte oder zu große Datei sowie fehlende Prozessorverbindung prüfen. Erwartet: `false`, passende Fehlermeldung und bisheriges Sample unverändert. Prozessorablehnung ebenfalls gezielt simulieren.

## Warum die bestehenden Tests den Fehler nicht erfasst haben

Der vorhandene paketierte Audiotest umgeht den Drag-and-drop-Eingang. Er erzeugt `Drumator.LoadPcm16Wav` mit WAV-Daten und sendet direkt an `processorPoint->notify`.

Beleg: [PackagedAudioHostSmoke.cpp:179](https://github.com/challanger2000/125A-Drumator/blob/1d00be07446721f7d401ff8387d8f41466281dc2/tests/PackagedAudioHostSmoke.cpp#L179).

Damit werden insbesondere nicht geprüft:

- Auswahl nativer Windows-Formate;
- tatsächlicher VSTGUI-Drop-Empfänger;
- Ereigniszustellung über die View-Hierarchie;
- Dateipfaderkennung und Dateizugriff des Controllers;
- Zuordnung der Drop-Koordinaten zur Instrumentenzeile.

Der Editor-Smoke-Test prüft vor allem Editorabmessungen und Content-Scale vor nativer Anhängung. Ein erfolgreicher Build oder Validator kann diese Lücken nicht schließen.

## Einordnung der bisherigen Reparaturversuche

Die untersuchte Historie enthält unter anderem:

- `950f8c3`: Text- und Datei-URL-Erkennung;
- `1bbe31b`: NUL-terminierte Dateipfade;
- `9c1ca32` und `207b43f`: View-eigenes Drop-Interface und Callback-Anbindung;
- `da50951`: Entfernung doppelter Koordinatenumrechnung;
- `455c92d`: diagnostische Statusausgaben;
- `1d00be0`: Übernahme der Phraseator-Pfaderkennung mit Existenzprüfung.

Diese Änderungen liegen überwiegend hinter der nativen Formatwahl. Keine übernimmt den in der funktionierenden Referenz vorhandenen CMake-Patch für `Win32DataPackage`.

Issue #3 dokumentiert ausdrücklich den weiterhin fehlgeschlagenen Praxistest bis `da50951`. Der aktuelle Quellstand `1d00be0` ist hier vergleichend analysiert; ihm wird kein neuer nativer D&D-PASS zugeschrieben.

## Empfehlung und Freigabestatus

**Empfehlung:** Den fehlenden Phraseator-SDK-Kompatibilitätspatch gezielt übernehmen und zusätzlich das Ergebnis der Controller-Ladeanfrage korrekt an den Drop-Aufrufer zurückgeben. Die bereits vorhandenen Drumator-Anpassungen für WAV/FLAC, Instrumentenzeilen und Sampleveröffentlichung erhalten.

Weitere Umbauten an Drop-Interface, Koordinaten oder Sampler sind durch diesen Vergleich derzeit nicht begründet.

**Status der Umsetzung:** nicht begonnen.  
**Status der nativen automatischen Regressionen:** nicht ausgeführt.  
**Studio-Pro-Praxistest:** weiterhin separat erforderlich; Explorer-Drop und interner Browser-Drop getrennt prüfen.  
**Freigabe „D&D repariert“:** keine.

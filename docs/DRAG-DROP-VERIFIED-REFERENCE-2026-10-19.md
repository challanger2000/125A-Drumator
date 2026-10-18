# 125A Drumator – bestätigte Drag-&-Drop-Referenz

**Status:** Studio-One-Praxistest durch Projektinhaber: **D&D PASS** (18./19.10.2026, Europe/Berlin). Dies gilt für das getestete Ziehen von Samples auf Instrumentenzeilen; keine pauschale Gesamtfreigabe des Plugins.

- Repository: `challanger2000/125A-Drumator`
- Branch: `develop/v1.0.0`
- Bestätigter Funktionsstand: `145c4fceca7eb2c4693fcbc0376f2e2c030cbe28`
- GitHub Actions: [39174065285](https://github.com/challanger2000/125A-Drumator/actions/runs/39174065285) – Windows Build/Validator/übliche Suite SUCCESS
- Benutzerbestätigung im Studio-One-Test: „nach nun zig Versuchen geht endlich Drag&Drop“ (18.10.2026, 23:05 UTC)
- Codex-Ursachenanalyse: [CODEX-DND-ANALYSE.md](../CODEX-DND-ANALYSE.md)

## Verbindlich wiederzuverwendender Lösungsweg

Das funktionierende Projekt [125A-Phraseator-Final](https://github.com/challanger2000/125A-Phraseator-Final), Referenz-Commit `b843d5e1756bb4fd89053633fa546cdcf05e5a65`, besitzt den benötigten **CMake-Patch für die native VSTGUI-Windows-Formatwahl**:

- `cmake/PatchVstguiWin32Drop.cmake` – **Dateiliste CF_HDROP vor CF_UNICODETEXT**. Nicht nur die View-C++-Methoden übernehmen.
- `CMakeLists.txt` – Patch auf `vstgui4/vstgui/lib/platform/win32/win32datapackage.cpp` des gepinnten SDK anwenden, **bevor** die passende SDK/VSTGUI-Bibliothek gebaut wird. Erwarteten Originalcode zwingend prüfen; bei ungekanntem SDK-Quellstand abbrechen.
- `vst3/PhraseatorViews.cpp/.h` – echte VSTGUI-`IDropTarget`-View, `getDropTarget()`, Ereignisse, korrekte Prüfung und Normalisierung von Dateipfaden; Drumator unterstützt zusätzlich FLAC.
- Im Drumator übernimmt `src/vst3/DrumatorView.cpp/.h` die Eingabe und ordnet anhand der sichtbaren Instrumentenzeilen zu. `loadPath`/Drop liefern nur bei angenommener Controller-Ladeanfrage Erfolg.

Die **Ursache der früheren Fehlversuche** war die Priorisierung von Text gegenüber tatsächlichen Dateilisten durch den ungepatchten nativen Windows-Adapter. Ein Text/Dateiname kann den verfügbaren vollständigen Pfad überdecken. Fünf lokale Änderungen an Parser/Drop-Callbacks/Koordinaten beseitigten diese vorgelagerte Ursache nicht.

## Nicht wiederholen

- Nie aus `GitHub Actions SUCCESS` allein `Drag & Drop PASS` ableiten.
- Vor Neuimplementierung die **gesamte funktionierende Referenzkette einschließlich CMake, SDK-Patches und View-Hierarchie** prüfen, nicht nur gleichnamige GUI-Funktionen.
- Beim Übertrag auf neue Plugins einen native-Windows-Test mit gleichzeitigem `CF_UNICODETEXT` und `CF_HDROP` vorsehen; nur der Adaptertest deckt den ursprünglichen Fehler sicher ab.
- Explorer-Drop und DAW-interner Browser-Drop getrennt testen; ungetestete Kombinationen offen kennzeichnen.
- Bestehende geprüfte Lösung nicht ohne kausalen Befund umstellen. Benutzerabnahme ist für Studio One weiterhin erforderlich.

**Release-Hinweis:** Dokumentationssicherung der bestätigten D&D-Funktion. Keine neue Version, kein erneuter Build und keine Behauptung, dass sämtliche Funktionen des Drumators abgenommen wurden.

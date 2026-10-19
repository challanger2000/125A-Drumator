# 125A Drumator – technische VSTGUI-Übergabe

**Abgrenzung:** Hier wird ausschließlich die vollständig funktionale VSTGUI-Grundlage für den **anderen Chat** vorbereitet. Alle endgültigen GUI-Grafiken, Taster, Knöpfe, LED-Grafiken, Hintergründe und die finale ästhetische Umsetzung erstellt bzw. integriert **der andere Chat**. Nichts davon ist hier freigegeben.

## Technische Quelle

Der Entwicklungsbranch enthält wieder die funktionale VSTGUI aus Commit `6a8384b241b865adc3d4e6266ca5c1c3f581fb34`; die von mir irrtümlich vorgenommene optische Überarbeitung `deae6a69` wurde rückgängig gemacht. Dies ist **kein** neuer GUI-Entwurf, sondern der vorherige funktionierende Platzhalter-Renderer.

- `src/vst3/DrumatorView.cpp/.h`: komplette bedienbare Custom View, bestehende Klickbereiche und dynamische Zustandsanzeige.
- `src/vst3/DrumatorController.cpp/.h`: echte Funktionen, keine Attrappen.
- `resource/Drumator.uidesc`: bestehendes Custom-View-Template, mit absichtlich noch leeren Bildressourcen.
- **`resource/VSTGUI_ASSET_SLOTS.json`**: kontrollgenaue Koordinaten, Zustände, Austauschpunkte und später benötigte Grafikfamilien.

## Übergabe zum Austausch gegen fertig generierte Bilder

1. Der zentrale Renderer `drawControl(...)` in `DrumatorView.cpp` besitzt bereits Rectangle, Text, `selected` und `enabled`. Ein anderer Chat ersetzt **nur seine Grafikdarstellung**, nicht die tatsächlichen Controller-Aufrufe.
2. Die Step-Schleife im Instrumenten-Renderer hat **32 getrennte Positionen** pro Zeile. Raster x=622+19*Step, y=108+29*(Instrument-firstRow). Die später generierten Step-Bilder werden exakt auf diese Fläche gelegt, nicht umgekehrt.
3. 4–16 Instrumentenzeilen, Footer, Song-Overlay und die ausklappbare 32-Step-Velocity-Leiste haben **variable Fensterhöhe**; keine einzige starre Hintergrundgrafik verwenden.
4. Der Song-Editor hat 64 Plätze (16×4), darunter Pattern A–H und Längen. Der Velocity-Editor zeigt dynamisch 32 Balken (0–127); Strg+Klick und Mausziehen sind bereits angebunden.
5. Alle Beschriftungen – Instrumentennamen, Sample-Dateinamen, Panorama, Pitch, Master, MIDI-Pattern, Ausgangsbus, Velocity – bleiben **echter dynamischer VSTGUI-Text**.
6. Zoom 100 % und 150 %, Host-Resize und Kompaktansicht müssen erhalten bleiben. Grafiken in passender nativer Auflösung vorbereiten.
7. Offizielles 125A-Logo: ausschließlich `challanger2000/125A-Branding/125A_Logo_Master_FINAL.svg` und geometrisch unverändert. **Hier wird noch kein Design oder Logo-Rendering implementiert.**

Die genaue Liste aller visualisierten und klickbaren Bereiche in logischen Pixeln steht im **JSON-Manifest**. Beim späteren Austausch gelten die tatsächlichen C++-Hitbereiche als technische Quelle. Keine Änderung an Audio/DSP, MIDI, Automation, Samples oder Projektzustand.

**QA des Ausgangscodes:** Commit `3944ed2`, GitHub Actions [#39314206625](https://github.com/challanger2000/125A-Drumator/actions/runs/39314206625): 11/11 CTests PASS, Steinberg Validator 47/47 PASS, paketierte GUI-/Audio-Hosttests PASS. Danach bis `6a8384b` nur Dokumentation. Keine praktische Studio-One-Freigabe der endgültigen Optik.

**Fazit:** Technische VSTGUI und definierte Grafik-Austauschplätze sind übergeben. Der andere Chat muss die echte visuelle Oberfläche bauen. Keine FINAL-V1-Freigabe.

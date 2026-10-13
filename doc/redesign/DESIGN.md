# VLC Redesign – Design-Spezifikation

Quelle: Figma-Community-Datei „VLC Redesign“, Werte direkt aus den lokalen Styles der Datei ausgelesen (13.10.2026).
Die Screens liegen als PNG im Ordner `screens/`. Alle exakten Werte stehen maschinenlesbar in `design-tokens.json`.

**Regel für die Umsetzung:** Keine Farben, Größen oder Abstände frei erfinden. Immer die Tokens unten verwenden. Wenn ein Wert in den Screens nicht zu einem Token passt, nachfragen statt raten.

## Theme-Struktur

Es gibt zwei Themes, **Dark** (Standard) und **Light**, mit identischen Token-Namen. Im Code ein zentrales Theme-Objekt anlegen, das je nach Modus die Werte umschaltet.

Benennung:
- `FG` = Vordergrund (Text, Icons). `1`/`2` = hellste Textstufen, `A1`–`A4` = abgestufte Sekundär-/Icon-Farben, `B1` = Text auf Akzentflächen
- `BG` = Hintergründe. `A1`–`A5` = Flächen von hell nach dunkel (Panels, Karten, Listen), `B1`/`B2` = tiefste Ebene (App-/Player-Hintergrund)
- `Accent` = VLC-Orange
- `Divider` = Trennlinien

## Farben

| Token | Dark | Light |
|---|---|---|
| FG/1 | `#EAEAEA` | `#EAEAEA` ⚠️ |
| FG/2 | `#C6C6C6` | `#C6C6C6` ⚠️ |
| FG/A1 | `#969696` | `#3C3C3C` |
| FG/A2 | `#8F8F8F` | `#2E2E2E` |
| FG/A3 | `#6B6B6B` | `#6B6B6B` |
| FG/A4 | `#404040` | `#3B3B3B` |
| FG/B1 | `#161616` | `#161616` |
| Accent/Main | `#D37918` | `#E8851A` |
| Accent/Bright | `#FC9729` | `#FFA13B` |
| Accent/Logo | `#D37918` | `#C2711A` |
| Accent/Dim | `#AB6314` | `#965916` |
| BG/Accent Bright | `#371F06` | `#D8BA9A` |
| BG/Accent Dark | `#1A1611` | `#C6AB89` |
| BG/A1 | `#222222` | `#EDEDED` |
| BG/A2 | `#1F1F1F` | `#DDDDDD` |
| BG/A3 | `#1B1B1B` | `#D5D5D5` |
| BG/A4 | `#191919` | `#B3B3B3` |
| BG/A5 | `#151515` | `#888888` |
| BG/B1 | `#0E0E0E` | `#E7E7E7` |
| BG/B2 | `#090909` | `#DEDEDE` |
| BG/A3 Transparent | `#191919` @ 95 % | `#D5D5D5` @ 75 % |
| BG/B1 Transparent | `#0E0E0E` @ 95 % | `#E7E7E7` @ 80 % |
| BG/B2 Transparent | `#000000` @ 80 % | – |
| Divider/Bright | `#373737` | `#9B9B9B` |
| Divider/Dim | `#222222` | `#ACACAC` |

⚠️ Im Light-Theme sind FG/1 und FG/2 in der Figma-Datei identisch mit Dark (sehr hell). Auf hellem Hintergrund wäre das unlesbar, vermutlich ein Fehler in der Vorlage. Im Light-Screen prüfen, wofür sie genutzt werden (evtl. nur Text auf dunklen Overlays).

Am häufigsten im Design genutzt: `#8F8F8F` (Icons/Sekundärtext), `#404040`, `#090909` (Player-BG), `#6B6B6B`, Akzent `#D37918`, `#1A1611` / `#371F06` (Akzent-Hintergründe, z. B. aktive Einträge).

Label-/Tag-Farben (z. B. für Playlist-Markierungen): `#E15032` `#C49E23` `#19A234` `#18AAD3` `#185DD3` `#8E42FF` `#F052B3`

## Effekte

- **bg blur**: Background-Blur 50
- **bg blur + shadow**: Background-Blur 50 + Drop-Shadow x 0, y 8, Blur 15, Spread 0, Schwarz @ 46 %

Wird für schwebende Elemente über dem Video genutzt (Steuerleiste, Menüs, Panels), zusammen mit den „Transparent“-Hintergründen.

## Typografie

- UI-Schrift: **Inter** (Regular für fast alles, dazu Light, Medium, Semi Bold, Bold)
- Monospace: **DM Mono** (Zeitangaben/Timecodes)
- Es gibt keine Text-Styles in der Datei, daher diese Skala aus den tatsächlich genutzten Größen:

| Größe | Verwendung (Häufigkeit) |
|---|---|
| 14 px | Standard-UI-Text (mit Abstand am häufigsten) |
| 16 px | Hervorgehobener Text, Titel in Listen |
| 12 px | Kleiner Text, Metadaten |
| 15 px | vereinzelt |
| 10 px | sehr kleine Labels |
| 20 / 24 / 32 px | Überschriften (Dialoge, Onboarding) |

## Eckenradien

`2` · `3` · `4` (Standard) · `6` · `10` · `12` · `14` · `100` (Pill/rund)

## Abstände

- Standard-Gap in Auto-Layouts: **10 px**, außerdem 4, 5, 8, 12, 14, 16, 20
- Häufige Paddings: `4`, `2`, `10`, `8`, `20` rundum; Buttons/Chips `2px 14px`; `4px 8px`; `8px 10px`; Listeneinträge `10px 18px 10px 12px`

## Screens & Größen

| Bereich | Frames | Größe |
|---|---|---|
| Player (Standard, Music, Light, hide UI, customize) | 20 | 1920 × 1080 |
| Einstellungen (global, interface, privacy, playback, video, audio, subtitles, controls, library, network, performance, recording, extensions) | 19 | 1048 × 865 |
| Untertitel zu Playlist/Ordner hinzufügen, Tastenkürzel | 5 | 1048 × 800 |
| Netzwerk-Stream öffnen | 1 | 1048 × 631 |
| Neue Playlist | 1 | 713 × 589 |
| Onboarding | 11 | 1090 × 759 |
| Kontextmenüs (Sortierung, Ordner, Datei, Playlist, Ansicht) | 6 | 265 px breit |
| Rechtsklick-Menü | 1 | 292 × 683 |
| Profile | 1 | 651 × 633 |

Komponenten in der Datei: Icon, Icon Wrapper, Button, Toggle, Dropdown, Controls, volume, play, sidebar, Miniplayer, Current Time, Duration, subtitle (one/multiple), Values, EQ, EQ presets, Setting Preset, setting category, media, playlist, thumbnail sowie Seitenpanels (400 px breit) für Audio (Compressor, Equalizer, Spatializer, Stereo Widener) und Video (Essential, Crop, Colors, HDR Options).

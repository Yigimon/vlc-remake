# VLC Redesign – Notiz zum Weitermachen

Stand: 14.10.2026, 08:10 – Schritte 1–7 und 9 fertig, Schritt 8 vorbereitet

## Wo alles liegt

- Repo: `/home/claude/vlc`, Branch `redesign`, Basis `1176b56` (upstream videolan/vlc, flacher Klon)
- Ziel zum Pushen: `https://github.com/Yigimon/vlc-remake` (Remote `remake`), Branch `redesign`
  (`main` mit den Design-PNGs bleibt unverändert). Der alte Fork `vlc-redesign-ui` (Remote `fork`) wird
  nicht mehr genutzt. Niemals nach `origin` (videolan/vlc) pushen.
  - Push schlägt mit 403 fehl: Die Claude GitHub App ist für das Konto Yigimon nicht installiert
    (https://github.com/apps/claude/installations/select_target, Zugriff auf `vlc-remake` geben).
  - Der Klon ist flach (ab `23211ea`). GitHub lehnt einen Push mit fehlender Vorgeschichte ab
    („shallow update not allowed“). Wege: (a) vom PC des Nutzers aus dem Bundle pushen (siehe unten),
    (b) einen Branch mit einem Import-Commit des Upstream-Stands `1176b56` als Wurzel und den
    Redesign-Commits darauf bauen und den pushen, (c) die ganze Historie holen (`git fetch --unshallow`,
    sehr groß, der Proxy kann große Pushes mit 413 ablehnen).
  - Bundle: `scratchpad/vlc-redesign.bundle` (braucht `1176b56`, also einen Klon von videolan/vlc):
    `git clone https://github.com/videolan/vlc.git vlc && cd vlc && git fetch <bundle> redesign:redesign &&
    git push https://github.com/Yigimon/vlc-remake.git redesign`
- Design-Spec: `doc/redesign/DESIGN.md`, `doc/redesign/design-tokens.json`, Design-PNGs in `/home/claude/vlc-remake`
- Figma ist nur über den eingebauten Browser erreichbar (in späteren Sitzungen meist nicht verbunden)
- Build: `ninja -C /home/claude/vlc/build` (meson, Qt 6.4.2); autotools-Prüfung: `make -C /home/claude/vlc-at/modules/gui/qt`
- Test: `scratchpad/shot.sh` (`THEME=1|2 ./shot.sh out.png WAIT [vlc-Args]`, `-v` zeigt QML-Warnungen im Log
  `/home/tester/vlc.log`), `scratchpad/act.sh out.png click X Y | drag … | key … | type … | scroll X Y N 4|5`
  (Koordinaten im Fenster), `scratchpad/crashtest.sh [Medium]` (baut und startet, 139 = Absturz).
  Xvfb :99, Benutzer `tester`, PulseAudio vorher starten
  (`su tester -c "XDG_RUNTIME_DIR=/tmp/runtime-tester pulseaudio --start --exit-idle-time=-1"`).
  VLC vor dem Bearbeiten von `/home/tester/.config/vlc/vlc-qt-interface.conf` beenden und `geometry=` löschen.

## Erledigt (alles committet und getestet)

| Schritt | Commits |
|---|---|
| 1 Baseline-Build | – |
| 2 Farbpalette | `00583be` |
| 3 Schrift, Radien | `95bc98e`, `ea4fdf7` |
| 4 Steuerleiste, Zeitleiste, Titelleiste | `e7dfd3a`, `70812f3`, `0871b84`, Absturz-Fix `67f5871` |
| 5 Sidebar, Seitenkopf | `40fcc04` |
| 6 Menüs, Play Queue, Qt-Widget-Dialoge, Dropdown | `abf5947`, `82650eb`, `5d94de6`, `41b90f1`, `63a92a3`, `59d7b7f`, `7325dd0` |
| 7a Einstellungen | `5924abf`, `634d39b` |
| 7b Onboarding (Erststart-Assistent), Akzentfarbe zur Laufzeit | `4efd9d2`, `8fc7136`, `6886765` |
| 9 Effekt-Seitenpanel im Player | `57c5279`, dazu `2b6649a` (Equalizer-Presets), `da7a388` |

Wichtig: `67f5871` behebt einen Absturz beim Start mit einem Medium (z. B. Doppelklick auf eine Datei),
eingeführt mit `0871b84`.

## Schritt 9 – Effekt-Seitenpanel

- Button „Extended settings“ der Steuerleiste öffnet im Player das Panel (statt Play Queue, immer nur
  eins), außerhalb des Players weiter den Dialog. Der Dialog bleibt über die Menüs erreichbar.
- `player/effects_controller.*` (QML-Singleton `PlayerEffects` in VLC.Player; der Name `Effects` ist schon
  von `EffectsImageProvider` in VLC.Util belegt), `player/qml/EffectsPanel.qml`, `EffectsSection.qml`,
  `EffectsSlider.qml`, `EffectsSwitch.qml`
- Getestet: Equalizer (an/aus, Preset Rock, Band ziehen, Wert tippen), Kompressor, Bildeinstellungen
  (Sättigung 0 = grau), Zuschneiden (oben/unten verknüpft), Negativ, Farbschwelle (Farbe tippen), Sepia
  (aus „Off“ ziehen, Doppelklick setzt zurück), Hell/Dunkel, Wechsel mit der Play Queue, Escape schließt
- Nicht umgesetzt: „HDR Options“ (VLC hat dafür keine allgemeine Einstellung), Symbole in der
  Preset-Liste, eigene Presets („Setting Preset“)
- Beim Zuschneiden passt VLC die Fenstergröße an das Video an (Upstream-Verhalten, „Fenstergröße an
  Video anpassen“ in den Einstellungen)

## Schritt 8 – Windows-Build (vorbereitet, noch nicht gelaufen)

- Workflow `.github/workflows/windows.yml` (`290d28a`, `2d0b9ce`): Cross-Build wie das upstream-CI („win64“: Docker-Image
  `vlc-debian-win64-posix`, vorkompilierte contribs, autotools, Release-Modus), Ergebnis als portable ZIP
  (Artefakt `vlc-redesign-win64`, mit Ordner `portable`)
- Anleitung: `doc/redesign/WINDOWS.md` (GitHub Actions oder lokal mit Docker/WSL 2)
- Offen: Workflow in `vlc-remake` laufen lassen (braucht den Push, siehe oben), Ergebnis unter Windows
  ansehen (Menüs mit runden Ecken, Fensterleiste, Schriften, HiDPI)

## Offene Kleinigkeiten

- Medienbibliothek-Ansichten (Videos, Musik, Playlists) ungetestet: VLC braucht medialibrary >= 0.13,
  Debian hat 0.12.3, und die VideoLAN-Server sind aus der Testumgebung nicht erreichbar

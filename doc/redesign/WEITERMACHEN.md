# VLC Redesign – Notiz zum Weitermachen

Stand: 13.10.2026, 23:00 – Schritte 1–7 fertig, Schritt 8 vorbereitet

## Wo alles liegt

- Repo: `/home/claude/vlc`, Branch `redesign`, Basis `1176b56` (upstream videolan/vlc)
- Fork zum Pushen: `https://github.com/Yigimon/vlc-redesign-ui.git` (Remote `fork`)
  - Push schlägt mit 403 fehl: Die Claude GitHub App ist für den Fork nicht installiert
    (https://github.com/apps/claude/installations/select_target). Danach `git push fork redesign`.
- Design-Spec: `doc/redesign/DESIGN.md`, `doc/redesign/design-tokens.json`
- Figma (Kopie des Nutzers): figma.com/design/hbaQfRGY3Qp598ItY5QCwb, im eingebauten Browser-Tab `seed`,
  `figma`-Plugin-API per Seiten-JS. Hilfsfunktion `window.__show(id, scale)` zeigt einen Knoten als PNG im
  Overlay `#__ov` (danach Screenshot). Lange `await`s vermeiden, sonst Timeout (45 s).
- Build: `ninja -C /home/claude/vlc/build` (meson, Qt 6.4.2)
- Test: `scratchpad/shot.sh` (`THEME=1|2 ./shot.sh out.png WAIT [vlc-Args]`, `PRIVACY=--qt-privacy-ask` zeigt den
  Erststart-Assistenten), `scratchpad/queue.sh` (startet mit 6 Queue-Einträgen). Xvfb :99, Benutzer `tester`.
  VLC vor dem Bearbeiten von `/home/tester/.config/vlc/vlc-qt-interface.conf` beenden (VLC schreibt beim Beenden).

## Erledigt (alles committet und getestet)

| Schritt | Commits |
|---|---|
| 1 Baseline-Build | – |
| 2 Farbpalette | `00583be` |
| 3 Schrift, Radien | `95bc98e`, `ea4fdf7` |
| 4 Steuerleiste, Zeitleiste, Titelleiste | `e7dfd3a`, `70812f3`, `0871b84` |
| 5 Sidebar, Seitenkopf | `40fcc04` |
| 6 Menüs, Play Queue, Qt-Widget-Dialoge, Dropdown | `abf5947`, `82650eb`, `5d94de6`, `41b90f1`, `63a92a3` |
| 7a Einstellungen | `5924abf` |
| 7b Onboarding (Erststart-Assistent), Akzentfarbe zur Laufzeit | `4efd9d2`, `8fc7136` |

## Schritt 8 – Windows-Build (vorbereitet, noch nicht gelaufen)

- Workflow `.github/workflows/windows.yml`: Cross-Build wie das upstream-CI („win64“: Docker-Image
  `vlc-debian-win64-posix`, vorkompilierte contribs, autotools), Ergebnis als portable ZIP (Artefakt
  `vlc-redesign-win64`, mit Ordner `portable`)
- Anleitung: `doc/redesign/WINDOWS.md` (GitHub Actions oder lokal mit Docker/WSL 2)
- Der autotools-Build des Qt-Moduls (den der Windows-Build nutzt) ist unter Linux geprüft
  (`/home/claude/vlc-at`, `make -C modules/gui/qt`)
- Farbschema „System“ unter Windows: die Qt-Widgets nutzen jetzt auch dort die Redesign-Palette
  (`SystemPalette::hasExternalColors()`)
- Offen: Workflow im Fork laufen lassen (braucht den Push, siehe oben), Ergebnis unter Windows
  ansehen (Menüs mit runden Ecken, Fensterleiste, Schriften, HiDPI)

## Offene Kleinigkeiten

- Medienbibliothek-Ansichten (Videos, Musik, Playlists) ungetestet: VLC braucht medialibrary >= 0.13,
  Debian hat 0.12.3, und die VideoLAN-Server sind aus der Testumgebung nicht erreichbar
- Figma ist nur über den eingebauten Browser erreichbar (Tab `seed`), der in späteren Sitzungen fehlen kann;
  Design-Werte stehen in `DESIGN.md` / `design-tokens.json`

# VLC Redesign – Notiz zum Weitermachen

Stand: 13.10.2026, 22:30 – Schritte 1–7 fertig

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

## Als Nächstes: Schritt 8 – Windows-Build

- GitHub-Actions-Workflow im Fork (MSYS2/MinGW oder die VLC-Docker-Images für win64) oder eine
  MSYS2-Anleitung zum lokalen Bauen
- Windows-Theme-Provider prüfen (Farbschema „System“ nutzt dort die Redesign-Paletten)
- Fusion-Stil und Menüs mit abgerundeten Ecken unter Windows prüfen (`useTranslucentMenus()`)

## Offene Kleinigkeiten

- Icons im „All“-Baum der Einstellungen noch alt/bunt, ausgewähltes Grid-Item gräulich,
  Medienbibliothek-Ansichten ungetestet (keine Medienbibliothek in der Testumgebung)

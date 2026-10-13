# VLC Redesign unter Windows

Der Branch `redesign` wird so gebaut wie das offizielle VLC für Windows: unter Linux für Windows
cross-kompiliert, mit dem Docker-Image und den vorkompilierten Abhängigkeiten („contribs“, inkl. Qt)
des VideoLAN-CI.

## Variante 1: GitHub Actions (empfohlen)

Der Workflow `.github/workflows/windows.yml` baut bei jedem Push auf `redesign` (Änderungen an der
Qt-Oberfläche) oder auf Knopfdruck eine portable Windows-Version.

1. Den Branch `redesign` in den Fork pushen (`git push fork redesign`).
2. Im Fork unter **Actions** die Workflows einmalig aktivieren (bei Forks standardmäßig aus).
3. **Windows build** → **Run workflow** (Branch `redesign`).
4. Nach dem Lauf (mit vorkompilierten contribs grob geschätzt 1–2 Stunden) unten auf der Seite des Laufs das
   Artefakt **vlc-redesign-win64** herunterladen.
5. Die ZIP-Datei im Artefakt entpacken und `vlc.exe` starten.

Die Version läuft im **portablen Modus** (Ordner `portable` neben `vlc.exe`): Einstellungen und
Medienbibliothek liegen in diesem Ordner, ein installiertes VLC behält seine eigenen Einstellungen.
Beim ersten Start erscheint deshalb der Erststart-Assistent.

Hinweise:

- Der Workflow nutzt die vorkompilierten contribs der VLC-Revision, auf der das Redesign aufbaut.
  Gibt es diese nicht mehr, werden die neuesten genommen; fehlen auch die, baut `build.sh` alle
  Abhängigkeiten selbst (inkl. Qt, mehrere Stunden, Limit des Workflows: 6 Stunden).
- Die Übersetzungen (Deutsch usw.) werden mitgebaut (`-l`).
- Das Redesign ändert nichts an `contrib/`, `extras/ci/` und `extras/package/`, damit die
  vorkompilierten contribs passen.

## Variante 2: lokal mit Docker (Linux oder Windows mit WSL 2)

Voraussetzung: Docker (unter Windows: Docker Desktop mit WSL 2) und ca. 20 GB freier Platz.

```sh
git clone https://github.com/Yigimon/vlc-redesign-ui.git vlc
cd vlc
git checkout redesign

docker run --rm -it -v "$PWD:/build" -w /build \
  registry.videolan.org/vlc-debian-win64-posix:20260611225331 \
  bash -c 'git config --global --add safe.directory /build &&
           extras/package/win32/build.sh -a x86_64 -p -l -o /build/vlc-redesign-win64'
```

Danach liegt die portable Version im Ordner `vlc-redesign-win64` (für den portablen Modus dort
einen leeren Ordner `portable` anlegen). Unter WSL am besten im Linux-Dateisystem bauen (z. B.
`~/vlc`), nicht unter `/mnt/c`, das ist deutlich langsamer.

## Was unter Windows anders ist als im Linux-Test

- Farbschema „System“: VLC liest den hellen/dunklen Modus von Windows und nutzt dann die
  Redesign-Paletten (unter Linux die Qt-Systempalette).
- Menüs haben abgerundete Ecken (unter X11 ohne Compositor bleiben sie eckig).
- Die Fensterleiste ist im modernen Layout die eigene (client-side decoration) von VLC.

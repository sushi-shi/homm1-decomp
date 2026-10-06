# Checks of the Windows build (windows.nix) under Wine.
#
# tests: the path and data-folder unit tests (file_test, data_root_test), a
#   flake check; neither needs a display or the game data.
# smoke: plays the part of a Windows player: the package's bin/ copied into a
#   game folder, started from that folder, from another one, and with a quoted
#   HOMM1_DATA (as cmd's `set HOMM1_DATA="C:\..."` leaves it) from a separate
#   program folder. Each start has to reach the main menu. Game data cannot be
#   in a flake check, so this is an app:
#     HOMM1_DATA=/path/to/game nix run .#windows-smoke
{ pkgs, windows }:

let
  wine = pkgs.wineWow64Packages.stable;
in
{
  tests = pkgs.runCommand "homm1-windows-tests" {
    nativeBuildInputs = [ wine ];
  } ''
    export HOME="$TMPDIR" WINEPREFIX="$TMPDIR/wine" WINEDEBUG=-all
    export WINEDLLOVERRIDES="mscoree,mshtml=" DISPLAY=
    # Wine names host files in the locale's charset; the tests use non-ASCII
    # folder names, as a Windows user's often are.
    export LC_ALL=C.UTF-8
    wineboot -i >/dev/null 2>&1
    # data_root_test creates a game folder beside itself.
    mkdir "$TMPDIR/tests"
    cp ${windows.tests}/bin/* "$TMPDIR/tests/"
    chmod -R u+w "$TMPDIR/tests"
    status=0
    for test in file_test data_root_test; do
      echo "== $test"
      wine "$TMPDIR/tests/$test.exe" || status=1
    done
    wineserver -k || true
    [ "$status" = 0 ]
    touch "$out"
  '';

  smoke = pkgs.writeShellApplication {
    name = "homm1-windows-smoke";
    runtimeInputs = [ pkgs.coreutils pkgs.findutils pkgs.python3 wine pkgs.xorg-server ];
    text = ''
      data="''${HOMM1_DATA:-}"
      if [ -z "$data" ] || [ ! -d "$data" ]; then
        echo "homm1-windows-smoke: set HOMM1_DATA to the game folder (DATA, MAPS, GAMES, SOUND, ANIM)" >&2
        exit 1
      fi
      unset HOMM1_DATA HOMM1_CD HOMM1_CONFIG

      work=$(mktemp -d)
      xvfb=
      cleanup() {
        wineserver -k 2>/dev/null || true
        if [ -n "$xvfb" ]; then kill "$xvfb" 2>/dev/null || true; fi
        rm -rf "$work"
      }
      trap cleanup EXIT

      # A player's folder: the game's folders and the program side by side.
      game="$work/Heroes of Might and Magic"
      mkdir -p "$game" "$work/elsewhere" "$work/program"
      for entry in "$data"/*; do
        case "$(basename "$entry" | tr '[:lower:]' '[:upper:]')" in
          DATA|MAPS|GAMES|SOUND|ANIM|HELP) cp -R "$entry" "$game/" ;;
        esac
      done
      cp -R ${windows}/bin/. "$game/"
      cp -R ${windows}/bin/. "$work/program/"
      chmod -R u+w "$game" "$work/program"

      export WINEPREFIX="$work/prefix" WINEDEBUG=-all
      export WINEDLLOVERRIDES="mscoree,mshtml=" LC_ALL=C.UTF-8
      export HOMM1_NO_DIALOGS=1 SDL_AUDIO_DRIVER=dummy
      unset WAYLAND_DISPLAY
      Xvfb -displayfd 4 -screen 0 1024x768x24 -nolisten tcp 4>"$work/display" 2>/dev/null &
      xvfb=$!
      for _ in $(seq 100); do [ -s "$work/display" ] && break; sleep 0.1; done
      DISPLAY=":$(cat "$work/display")"
      export DISPLAY
      wineboot -i >/dev/null 2>&1

      failures=0
      # start NAME DIRECTORY PROGRAM [VARIABLE=VALUE...]: runs PROGRAM /I0
      # from DIRECTORY, takes a screenshot after eight seconds and quits.
      start() {
        local name=$1 directory=$2 program=$3
        shift 3
        local shot replay
        shot="$work/$name.bmp"
        replay="$work/$name.replay"
        printf '8000 shot %s\n+500 exit\n' "$(winepath -w "$shot")" > "$replay"
        local status=0
        (cd "$directory" && env HOMM1_INPUT_REPLAY="$(winepath -w "$replay")" \
          HOMM1_CONFIG="$(winepath -w "$work/config-$name")" "$@" \
          timeout -k 5 90 wine "$program" /I0 >"$work/$name.log" 2>&1) || status=$?
        # The main menu: a full picture, not an empty screen or a message.
        if [ "$status" = 0 ] && [ -s "$shot" ] && python3 - "$shot" <<'PYTHON'
      import sys
      data = open(sys.argv[1], "rb").read()
      offset = int.from_bytes(data[10:14], "little")
      raise SystemExit(0 if len(set(data[offset:])) > 32 else 1)
      PYTHON
        then
          echo "ok   $name"
        else
          echo "FAIL $name (exit $status)"
          grep -a '^\[homm1\]' "$work/$name.log" | sed 's/^/  /' || true
          failures=$((failures + 1))
        fi
      }

      start game-folder "$game" "$game/heroes.exe"
      start other-folder "$work/elsewhere" "$game/heroes.exe"
      start quoted-HOMM1_DATA "$work/elsewhere" "$work/program/heroes.exe" \
        HOMM1_DATA="\"$(winepath -w "$game")\""
      [ "$failures" = 0 ]
    '';
  };
}

# The installable game (README, "Install with a NixOS flake"): the launchers
# `heroes` and `heroes-editor` (`heroes-te` and `heroes-te-editor` for the
# Tournament Edition) on the native programs, with desktop entries.
#
#   game     the player's copy of the Buka 2003 edition: its CD image, the CD,
#            an installed game folder or a .zip/.7z/.rar of one (nix/game-data.py).
#            Checked and laid out in the store at install time, locally, never
#            substituted from a cache; the icons come from its programs. Without
#            it the launchers import $HOMM1_GAME on their first run.
#   locale   the language compiled into the programs (locales/<LANG>.json).
#   editor   whether to install the scenario editor.
#   edition  "buka", or "te" for the Tournament Edition (port-te): names the
#            game in its desktop entry and gives its launchers, desktop entries
#            and icons their own names, so both editions install side by side.
#
# Another edition (Tournament Edition) is another set of programs and its own
# per-user folder: `stateName` keeps the editions' saved games apart.
{ pkgs, native, runner }:
{ game ? null, locale ? "ru", editor ? true, stateName ? "homm1", edition ? "buka" }:
let
  inherit (pkgs) lib;
  suffix = lib.optionalString (edition == "te") "-te";
  gameName = "heroes${suffix}";
  editorName = "heroes${suffix}-editor";
  programs = if locale == "ru" then native else native.overrideAttrs (old: {
    pname = "homm1-native-${locale}";
    cmakeFlags = old.cmakeFlags ++ [ "-DHOMM1_LOCALE=${locale}" ];
  });
  importer = pkgs.writeShellApplication {
    name = "homm1-import";
    runtimeInputs = [ pkgs.python3 pkgs.p7zip pkgs.unar pkgs.unshield ];
    text = ''exec python3 ${./game-data.py} --runner ${runner} "$@"'';
  };
  data = pkgs.runCommand "homm1-game-data" {
    preferLocalBuild = true;
    allowSubstitutes = false;
    nativeBuildInputs = [ importer pkgs.icoutils pkgs.imagemagick ];
  } ''
    homm1-import --game ${lib.escapeShellArg "${game}"} --out "$out" --retail retail
    # Each program's 32x32 icon, and larger copies scaled by whole pixels.
    for icon in heroes:HEROES editor:EDITOR; do
      executable=retail/''${icon#*:}.EXE
      [ -f "$executable" ] || continue
      wrestool -x -t 14 -o icon.ico "$executable"
      icotool -x -w 32 -h 32 -o icon.png icon.ico
      for size in 32 64 128 256; do
        mkdir -p "$out/share/icons/hicolor/''${size}x$size/apps"
        magick icon.png -filter point -resize "''${size}x$size" \
          "$out/share/icons/hicolor/''${size}x$size/apps/homm1${suffix}-''${icon%%:*}.png"
      done
      rm icon.ico icon.png
    done
  '';
  title = {
    ${gameName} = if edition == "te" then "Heroes of Might and Magic TE" else "Heroes of Might and Magic";
    ${editorName} = if edition == "te"
      then "Heroes of Might and Magic TE Scenario Editor"
      else "Heroes of Might and Magic Scenario Editor";
  };
  launcher = name: executable: pkgs.writeShellApplication {
    inherit name;
    runtimeInputs = [ pkgs.coreutils pkgs.findutils pkgs.gnugrep pkgs.util-linux importer ];
    text = ''
      program=${programs}/bin/${executable}
      title=${lib.escapeShellArg title.${name}}
      store_data=${if game == null then "" else data}
      importer=homm1-import
      state_name=${lib.escapeShellArg stateName}
      ${builtins.readFile ./launch.sh}
    '';
  };
  desktop = name: icon: comment: pkgs.makeDesktopItem ({
    inherit name comment;
    desktopName = title.${name};
    exec = name;
    categories = [ "Game" "StrategyGame" ];
  } // lib.optionalAttrs (game != null) { icon = "homm1${suffix}-${icon}"; });
in
pkgs.symlinkJoin {
  name = "homm1${suffix}-${locale}";
  paths = [
    (launcher gameName "homm1")
    (desktop gameName "heroes" (if edition == "te"
      then "Turn-based strategy (the Tournament Edition on the native port of the Buka 2003 edition)"
      else "Turn-based strategy (native port of the Buka 2003 edition)"))
  ] ++ lib.optionals editor [
    (launcher editorName "homm1-editor")
    (desktop editorName "editor" (if edition == "te"
      then "Scenario editor for the Tournament Edition of Heroes of Might and Magic"
      else "Scenario editor for Heroes of Might and Magic"))
  ];
  postBuild = lib.optionalString (game != null) ''
    mkdir -p "$out/share"
    cp -r ${data}/share/icons "$out/share/icons"
  '';
  passthru = { inherit data programs; };
  meta = {
    description = if game == null
      then "Heroes of Might and Magic (native port); set HOMM1_GAME to your Buka 2003 game on first run"
      else "Heroes of Might and Magic (native port) with your Buka 2003 game data";
    mainProgram = gameName;
    platforms = [ "x86_64-linux" ];
  };
}

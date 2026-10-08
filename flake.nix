{
  description = "Heroes of Might and Magic (Buka 2003) reconstructed source";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      release = pkgs.fetchurl {
        url = "https://github.com/sushi-shi/homm1-decomp/releases/download/toolchain-buka-2003-v2/homm1-toolchain-buka-2003-v2.tar.xz";
        sha256 = "ea8efe04f52f36e0371dd5dde77e25e552a2f221819a663ca39530b005b3713c";
      };
      toolchain = pkgs.runCommand "homm1-vc6-toolchain" { } ''
        mkdir -p $out
        tar xf ${release} -C $out
      '';
      wine = pkgs.wineWow64Packages.staging;
      # Each language runs under its descriptor's system locale (ru_RU.UTF-8
      # for Russian), so Wine shows its text in the language's code page.
      locales = pkgs.glibcLocales.override {
        allLocales = false;
        locales = map (name: "${(builtins.fromJSON (builtins.readFile (./locales + "/${name}"))).system_locale}/UTF-8")
          (builtins.filter (pkgs.lib.hasSuffix ".json") (builtins.attrNames (builtins.readDir ./locales)));
      };
      tools = [ pkgs.python3 wine pkgs.llvm pkgs.p7zip pkgs.unshield ];
      environment = {
        HOMM1_TOOLCHAIN = "${toolchain}/toolchains";
        HOMM1_LOCALE_ARCHIVE = "${locales}/lib/locale/locale-archive";
        WINEDLLOVERRIDES = "mscoree,mshtml=";
      };
      # Runs play.py from the checkout it is started in (builds go to its
      # build/), else from this flake's own copy (builds go to the state folder).
      runner = name: target: pkgs.writeShellApplication {
        inherit name;
        runtimeInputs = tools;
        text = ''
          export HOMM1_TOOLCHAIN="${environment.HOMM1_TOOLCHAIN}"
          export HOMM1_LOCALE_ARCHIVE="${environment.HOMM1_LOCALE_ARCHIVE}"
          export WINEDLLOVERRIDES="${environment.WINEDLLOVERRIDES}"
          root="$PWD"
          if [ ! -f "$root/play.py" ] || [ ! -f "$root/build.json" ]; then
            root="${self}"
          fi
          exec python3 "$root/play.py" --target ${target} "$@"
        '';
      };
      app = name: target: description: {
        type = "app";
        program = "${runner name target}/bin/${name}";
        meta.description = description;
      };
      play = app "homm1-play" "game" "Build HEROES.EXE and run it in a prepared Wine prefix";
      editor = app "homm1-editor" "editor"
        "Build the scenario editor EDITOR.EXE and run it in the game's Wine prefix";
    in {
      apps.${system} = {
        inherit play editor;
        default = play;
      };
      devShells.${system}.default = pkgs.mkShell ({
        packages = tools;
      } // environment);
    };
}

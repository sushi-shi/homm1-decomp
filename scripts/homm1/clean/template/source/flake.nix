{
  description = "Heroes of Might and Magic (Buka 2003) reconstructed source";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/@NIXPKGS_REV@";

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      release = pkgs.fetchurl {
        url = "@TOOLCHAIN_URL@";
        sha256 = "@TOOLCHAIN_SHA256@";
      };
      toolchain = pkgs.runCommand "homm1-vc6-toolchain" { } ''
        mkdir -p $out
        tar xf ${release} -C $out
      '';
      wine = pkgs.wineWow64Packages.staging;
      # The Russian build runs under ru_RU.UTF-8 (its text is Windows-1251).
      locales = pkgs.glibcLocales.override {
        allLocales = false;
        locales = [ "en_US.UTF-8/UTF-8" "ru_RU.UTF-8/UTF-8" ];
      };
      tools = [ pkgs.python3 wine pkgs.llvm pkgs.p7zip pkgs.unshield ];
      environment = {
        HOMM1_TOOLCHAIN = "${toolchain}/toolchains";
        HOMM1_LOCALE_ARCHIVE = "${locales}/lib/locale/locale-archive";
        WINEDLLOVERRIDES = "mscoree,mshtml=";
      };
      # Runs play.py from the checkout it is started in (builds go to its
      # build/), else from this flake's own copy (builds go to the state folder).
      play = pkgs.writeShellApplication {
        name = "homm1-play";
        runtimeInputs = tools;
        text = ''
          export HOMM1_TOOLCHAIN="${environment.HOMM1_TOOLCHAIN}"
          export HOMM1_LOCALE_ARCHIVE="${environment.HOMM1_LOCALE_ARCHIVE}"
          export WINEDLLOVERRIDES="${environment.WINEDLLOVERRIDES}"
          root="$PWD"
          if [ ! -f "$root/play.py" ] || [ ! -f "$root/build.json" ]; then
            root="${self}"
          fi
          exec python3 "$root/play.py" "$@"
        '';
      };
      app = {
        type = "app";
        program = "${play}/bin/homm1-play";
        meta.description = "Build HEROES.EXE and run it in a prepared Wine prefix";
      };
    in {
      apps.${system} = {
        play = app;
        default = app;
      };
      devShells.${system}.default = pkgs.mkShell ({
        packages = tools;
      } // environment);
    };
}

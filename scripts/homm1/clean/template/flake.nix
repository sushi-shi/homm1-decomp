{
  description = "Heroes of Might and Magic (Windows 95, 1997) reconstructed source";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/@NIXPKGS_REV@";

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      release = pkgs.fetchurl {
        url = "@TOOLCHAIN_URL@";
        sha256 = "@TOOLCHAIN_SHA256@";
      };
      toolchain = pkgs.runCommand "homm1-vc41-toolchain" { } ''
        mkdir -p $out
        tar xf ${release} -C $out
      '';
      wine = pkgs.wineWow64Packages.staging;
      launcher = pkgs.writeShellApplication {
        name = "homm1-play";
        runtimeInputs = [ pkgs.python3 wine pkgs.llvm pkgs.gamescope pkgs.bash pkgs.coreutils ];
        text = ''
          export HOMM1_TOOLCHAIN="${toolchain}/toolchains"
          export WINEDLLOVERRIDES="mscoree,mshtml="
          exec python3 "$PWD/play.py" "$@"
        '';
      };
    in {
      apps.${system}.default = {
        type = "app";
        program = "${launcher}/bin/homm1-play";
      };
      devShells.${system} = {
        default = pkgs.mkShell {
          packages = [ pkgs.python3 wine pkgs.llvm ];
          HOMM1_TOOLCHAIN = "${toolchain}/toolchains";
        };
        play = pkgs.mkShell {
          packages = [ pkgs.gamescope wine ];
        };
      };
    };
}

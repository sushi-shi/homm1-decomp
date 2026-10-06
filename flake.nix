{
  description = "Heroes of Might and Magic (Windows 95, 1996) reconstructed source";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      release = pkgs.fetchurl {
        url = "https://github.com/sushi-shi/homm1-decomp/releases/download/toolchain-vc40-masm611-sdk1/homm1-toolchain-vc40-masm611-sdk1.tar.xz";
        sha256 = "eb582d9a293cd0d666ea56eb937b6b8c0891da231bce1c5566e6450f15c4e9a5";
      };
      toolchain = pkgs.runCommand "homm1-vc40-toolchain" { } ''
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

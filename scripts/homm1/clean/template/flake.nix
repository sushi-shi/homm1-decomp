{
  description = "Heroes of Might and Magic (Windows 95, 1996) reconstructed source";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/@NIXPKGS_REV@";

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      release = pkgs.fetchurl {
        url = "@TOOLCHAIN_URL@";
        sha256 = "@TOOLCHAIN_SHA256@";
      };
      toolchain = pkgs.runCommand "homm1-vc40-toolchain" { } ''
        mkdir -p $out
        tar xf ${release} -C $out
      '';
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pkgs.python3 pkgs.wineWow64Packages.staging pkgs.llvm ];
        HOMM1_TOOLCHAIN = "${toolchain}/toolchains";
      };
    };
}

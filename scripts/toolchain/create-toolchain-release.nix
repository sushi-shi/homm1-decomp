# Reproduce the HoMM1 VC4 + MASM 6.11 release bundle.
# Usage: nix-shell scripts/toolchain/create-toolchain-release.nix

{ pkgs ? import <nixpkgs> {} }:

let
  vc40 = pkgs.fetchurl {
    name = "MSVC40.iso";
    url = "https://archive.org/download/msvc4x/MSVC40.iso";
    hash = "sha256-lhMm77+9KZeU4suxAun/O/546/kaEbiZeAlfWeCuqT4=";
  };
  masm611-disk1 = pkgs.fetchurl {
    name = "MASM611-DISK1.json";
    url = "https://raw.githubusercontent.com/jeffpar/pcjs-miscdisks/bd4cc85928f2291c8d08e610038a1dd70b94ac6c/pcx86/lang/microsoft/masm/6.11/MASM611-DISK1.json";
    hash = "sha256-ZtLuXQb8nCSbGIevSu1EnxQyJ+7+qTvHMLfQobufa/k=";
  };
in
pkgs.mkShell {
  packages = [ pkgs.python3 pkgs.p7zip pkgs.libmspack pkgs.gnutar pkgs.xz ];
  shellHook = ''
    export MSVC40_MEDIA="${vc40}"
    export MASM611_DISK1="${masm611-disk1}"
    export LIBMSPACK="${pkgs.libmspack}/lib/libmspack.so"
    export HOMM1_DIR="$PWD"
    exec python3 ${./create-toolchain-release.py}
  '';
}

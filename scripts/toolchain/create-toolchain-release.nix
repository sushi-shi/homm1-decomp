# Reproduce the HoMM1 VC4 + MASM 6.11 release bundle.
# Usage: nix-shell scripts/toolchain/create-toolchain-release.nix

{ pkgs ? import <nixpkgs> {} }:

let
  vc41 = pkgs.fetchurl {
    url = "https://archive.org/download/MSDN_July_1996_Disc_2_Visual_C_4.1/MSDN_July_1996_Disc_2_Visual_C%2B%2B_4.1.iso";
    hash = "sha256-urvNfc7+wJLGFoMwqbe3FXsFuuog/lxZOZLQ5Q3cHnI=";
  };
  wing10 = pkgs.fetchurl {
    url = "https://archive.org/download/Windows_Game_SDK_Developers_Guide_The_Coriolis_Group_1996/Windows%20Game%20SDK%20Developer%27s%20Guide%20-%20The%20Coriolis%20Group%201996.ISO";
    hash = "sha256-oMFm/4M+81Yn6s4NTdiijOdB6nwHbiE/nE0MZS1Lc6U=";
  };
  dx1 = pkgs.fetchurl {
    url = "https://archive.org/download/windows-95-game-sdk-featuring-direct-x/Windows%2095%20Game%20SDK%20Featuring%20DirectX.iso";
    hash = "sha256-PcEVRFVyy6RIjusl9w8MOk1TEJ9JUupidPBTij05KVs=";
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
    export MSVC41_MEDIA="${vc41}"
    export WING10_MEDIA="${wing10}"
    export DX1_MEDIA="${dx1}"
    export MASM611_DISK1="${masm611-disk1}"
    export LIBMSPACK="${pkgs.libmspack}/lib/libmspack.so"
    export HOMM1_DIR="$PWD"
    exec python3 ${./create-toolchain-release.py}
  '';
}

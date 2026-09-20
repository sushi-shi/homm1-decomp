{
  description = "Heroes I matching-decompilation environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";
    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    vostok-delinker-src = {
      # Same reviewed-data-topology revision used by the Gruntz donor.
      url = "github:srp-survarium/vostok-delinker/81d34b204a0384a92cf3b4c641a8430256b2922e";
      flake = false;
    };
    objdiff-src = {
      url = "github:encounter/objdiff/v3.7.3";
      flake = false;
    };
    objconv-omf-src = {
      # 32-bit OMF fixes used only to make Watcom 10 comparison COFF objects.
      url = "github:tomsons26/objconv/50d635877d22ec1b483a3a752f5159762839f791";
      flake = false;
    };
  };

  outputs = { nixpkgs, rust-overlay, vostok-delinker-src, objdiff-src,
              objconv-omf-src, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };

      rust = pkgs.rust-bin.nightly.latest.default.override {
        extensions = [ "rust-src" "rustfmt" "clippy" ];
      };
      nightly-rustPlatform = pkgs.makeRustPlatform {
        cargo = rust;
        rustc = rust;
      };

      vostok-delinker = nightly-rustPlatform.buildRustPackage {
        pname = "vostok-delinker";
        version = "0.1.0";
        src = vostok-delinker-src;
        cargoHash = "sha256-ry3TH1fz7Aj/JdbmlgQFFn29m8E7EQHyGaVXnZTEcXo=";
        patches = [
          ./nix/patches/vostok-data-manifest-folded-comdat.patch
          ./nix/patches/vostok-ilt-thunk-resolution.patch
          ./nix/patches/vostok-comdat-leader-nonzero-offset.patch
          ./nix/patches/vostok-grouped-section-names.patch
          ./nix/patches/vostok-legacy-data-not-into-comdat.patch
          ./nix/patches/vostok-data-hypothesis-must-contain.patch
          ./nix/patches/vostok-canonical-alias-owner.patch
          ./nix/patches/vostok-unprovisioned-identity-refusal.patch
        ];
      };

      objdiffVersion = "3.7.3";
      objdiffUrl = name:
        "https://github.com/encounter/objdiff/releases/download/v${objdiffVersion}/${name}";
      objdiffGuiLibs = with pkgs; [
        libGL
        libxkbcommon
        wayland
        fontconfig
        freetype
        libx11
        libxcursor
        libxi
        libxrandr
        libxcb
      ];

      objdiff-cli = nightly-rustPlatform.buildRustPackage {
        pname = "objdiff-cli";
        version = objdiffVersion;
        src = objdiff-src;
        patches = [
          ./nix/patches/objdiff-bss-inferred-extent.patch
          ./nix/patches/objdiff-score-reloc-addend.patch
        ];
        cargoHash = "sha256-Z9vyUj35nrHuUoOYM54RLCn7CzcQ6k3A6FsDYKCVqVM=";
        cargoBuildFlags = [ "-p" "objdiff-cli" ];
        cargoTestFlags = [ "-p" "objdiff-core" "-p" "objdiff-cli" ];
        cargoInstallFlags = [ "-p" "objdiff-cli" ];
        nativeBuildInputs = [ pkgs.protobuf ];
        OBJDIFF_REGENERATE_PROTO = "1";
      };

      objdiff = pkgs.stdenv.mkDerivation {
        pname = "objdiff";
        version = objdiffVersion;
        src = pkgs.fetchurl {
          url = objdiffUrl "objdiff-linux-x86_64";
          hash = "sha256-1pzhzJUl/BJQP2XS333KIfkx1YYi8ZyRdPMv5MnJGyA=";
        };
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.autoPatchelfHook pkgs.makeWrapper ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ] ++ objdiffGuiLibs;
        installPhase = ''
          install -Dm755 $src $out/bin/objdiff
          wrapProgram $out/bin/objdiff \
            --prefix LD_LIBRARY_PATH : "${pkgs.lib.makeLibraryPath objdiffGuiLibs}"
        '';
      };

      objconv-omf = pkgs.stdenv.mkDerivation {
        pname = "objconv-omf";
        version = "2.54.1-50d6358";
        src = objconv-omf-src;
        # Upstream's fixed-size legacy buffers trip modern fortify before the
        # validated OMF conversion completes. The compiler's stack protector
        # and the remaining Nix hardening stay enabled.
        hardeningDisable = [ "fortify" ];
        nativeBuildInputs = [ pkgs.gnumake pkgs.gcc ];
        postPatch = ''
          substituteInPlace src/omf2cof.cpp \
            --replace-fail 'switch (Records[RecNum].Type) {' \
                           'switch (Records[RecNum].Type2) {'
        '';
        installPhase = ''
          install -Dm755 objconv $out/bin/objconv-omf
        '';
      };

      python = pkgs.python3.withPackages (ps: [ ps.capstone ps.libclang ]);
      homm1-cli = pkgs.writeShellScriptBin "homm1" ''
        project_dir="''${HOMM1_DIR:-$PWD}"
        exec ${python}/bin/python3 "$project_dir/homm1" "$@"
      '';
      commonTools = with pkgs; [
        homm1-cli python git ninja binutils llvm llvmPackages.clang-unwrapped clang-tools
        ripgrep file jq p7zip objconv-omf vostok-delinker objdiff objdiff-cli
      ];
      commonHook = ''
        export HOMM1_DIR="$PWD"
        export PYTHONPATH="$HOMM1_DIR/scripts''${PYTHONPATH:+:$PYTHONPATH}"
        export MSVC_DIR="$HOMM1_DIR/build/toolchains/vc40"
        export WATCOM_DIR="$HOMM1_DIR/build/toolchains/watcom10"
        export PYTHONDONTWRITEBYTECODE=1
      '';
    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli objconv-omf;
        default = vostok-delinker;
      };
      checks.${system}.tooling = pkgs.runCommand "homm1-tooling-tests" {
        nativeBuildInputs = [ python pkgs.llvmPackages.clang-unwrapped ];
      } ''
        cp -r ${./.} source
        chmod -R u+w source
        cd source
        export PYTHONPATH="$PWD/scripts"
        python3 -m unittest discover -s tests -v
        touch $out
      '';
      devShells.${system} = {
        default = pkgs.mkShell {
          packages = commonTools;
          shellHook = commonHook;
        };
        build = pkgs.mkShell {
          packages = commonTools ++ [ pkgs.wineWow64Packages.staging ];
          shellHook = commonHook + ''
            export WINEPREFIX="$HOMM1_DIR/build/wineprefix"
            export WINEDLLOVERRIDES="mscoree,mshtml="
            export WINEDEBUG="fixme-all"
          '';
        };
      };
    };
}

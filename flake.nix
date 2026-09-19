{
  description = "Heroes I matching-decompilation environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";
    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    vostok-delinker-src = {
      url = "github:srp-survarium/vostok-delinker/1393e24b4804cb357fdac147c68013f0aa5a9d95";
      flake = false;
    };
    objdiff-src = {
      url = "github:encounter/objdiff/v3.7.3";
      flake = false;
    };
  };

  outputs = { nixpkgs, rust-overlay, vostok-delinker-src, objdiff-src, ... }:
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
        cargoHash = "sha256-ZwFdbqUyh4b0S+fUYKGMN1fWaxRu1zU2ozKpe7CbcYs=";
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

      homm1-cli = pkgs.writeShellScriptBin "homm1" ''
        project_dir="''${HOMM1_DIR:-$PWD}"
        exec ${pkgs.python3}/bin/python3 "$project_dir/homm1" "$@"
      '';
      commonTools = with pkgs; [
        homm1-cli python3 git ninja binutils llvm clang-tools
        ripgrep file jq p7zip vostok-delinker objdiff objdiff-cli
      ];
      commonHook = ''
        export HOMM1_DIR="$PWD"
        export PYTHONDONTWRITEBYTECODE=1
      '';
    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli;
        default = vostok-delinker;
      };
      checks.${system}.tooling = pkgs.runCommand "homm1-tooling-tests" {
        nativeBuildInputs = [ pkgs.python3 ];
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

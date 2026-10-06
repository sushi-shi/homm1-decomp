# The native port cross-compiled for 64-bit Windows with MinGW-w64: the game
# (heroes.exe) and the scenario editor (heroes-editor.exe) in bin/, with the
# DLLs they need beside them. SDL3 is nixpkgs' cross build; FFmpeg is cut down
# to the decoders the game uses (ffmpeg-minimal.nix).
{ pkgs, src }:

let
  cross = pkgs.pkgsCross.mingwW64;
  sdl3 = cross.sdl3;
  ffmpeg = cross.callPackage ./ffmpeg-minimal.nix { };
  # GCC's thread model on this target: libstdc++ calls into mcfgthread.
  threads = cross.windows.mcfgthreads;
in
cross.stdenv.mkDerivation {
  pname = "homm1-windows";
  version = "0";
  inherit src;

  nativeBuildInputs = with cross.buildPackages; [ cmake ninja pkg-config python3 ];
  buildInputs = [ sdl3 ffmpeg ];
  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=RelWithDebInfo" "-DBUILD_TESTING=ON" ];
  # The programs and the path tests that run under Wine (windows-checks.nix);
  # the other tests are not cross-built here.
  ninjaFlags = [ "homm1" "homm1_editor" "file_test" "data_root_test" ];
  outputs = [ "out" "tests" ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -m755 heroes.exe heroes-editor.exe $out/bin/
    install -m644 ${sdl3.out}/bin/SDL3.dll ${ffmpeg}/bin/*.dll $out/bin/
    install -m644 ${threads}/bin/libmcfgthread-2.dll $out/bin/
    mkdir -p $tests/bin
    install -m755 tests/port/file_test.exe tests/port/data_root_test.exe $tests/bin/
    cp $out/bin/*.dll $tests/bin/
    runHook postInstall
  '';

  # Everything the programs import must be shipped here or be part of Windows.
  doInstallCheck = true;
  installCheckPhase = ''
    shipped=$(cd $out/bin && ls *.dll | tr 'A-Z' 'a-z')
    for program in $out/bin/*.exe $out/bin/*.dll $tests/bin/*.exe; do
      for dll in $(${cross.stdenv.cc.targetPrefix}objdump -p "$program" | sed -n 's/^\s*DLL Name: //p' | tr 'A-Z' 'a-z'); do
        case "$dll" in
          kernel32.dll|user32.dll|gdi32.dll|advapi32.dll|shell32.dll|ole32.dll|oleaut32.dll| \
          imm32.dll|winmm.dll|version.dll|setupapi.dll|cfgmgr32.dll|bcrypt.dll|msvcrt.dll| \
          ntdll.dll|uxtheme.dll|dwmapi.dll|shcore.dll|hid.dll|ws2_32.dll|api-ms-win-*) ;;
          *) echo "$shipped" | grep -qx "$dll" || { echo "$program imports $dll, which is not shipped"; exit 1; } ;;
        esac
      done
    done
  '';

  passthru = { inherit sdl3 ffmpeg; };
  meta.platforms = [ "x86_64-windows" ];
}

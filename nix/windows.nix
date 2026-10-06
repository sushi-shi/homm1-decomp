# The native port cross-compiled for 64-bit Windows with MinGW-w64: the game
# (heroes.exe) and the scenario editor (heroes-editor.exe) in bin/. Each is a
# single file: SDL3, the C++ runtime and the compiler's thread library are
# linked in (CMakeLists.txt links MinGW programs with -static), so a player
# can copy them anywhere without DLLs.
{ pkgs, src }:

let
  cross = pkgs.pkgsCross.mingwW64;
  # nixpkgs builds SDL3 shared only, and -static cannot link a DLL in.
  sdl3 = cross.sdl3.overrideAttrs (previous: {
    cmakeFlags = previous.cmakeFlags ++ [
      "-DSDL_SHARED:BOOL=OFF"
      "-DSDL_STATIC:BOOL=ON"
    ];
  });
  # Windows' own DLLs. Anything else a program imported would be a "missing
  # DLL" error on the player's machine, since the package ships no DLL.
  windowsSystemDlls = [
    "advapi32.dll" "bcrypt.dll" "cfgmgr32.dll" "gdi32.dll" "hid.dll"
    "imm32.dll" "kernel32.dll" "msvcrt.dll" "ntdll.dll" "ole32.dll"
    "oleaut32.dll" "setupapi.dll" "shell32.dll" "user32.dll"
    "version.dll" "winmm.dll" "ws2_32.dll"
  ];
in
cross.stdenv.mkDerivation {
  pname = "homm1-windows";
  version = "0";
  inherit src;

  nativeBuildInputs = with cross.buildPackages; [ cmake ninja pkg-config python3 ];
  # GCC's thread model on this target: libstdc++ calls into mcfgthread,
  # whose static library -static links in.
  buildInputs = [ sdl3 cross.windows.mcfgthreads ];
  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=RelWithDebInfo" "-DBUILD_TESTING=ON" ];
  # The programs and the path tests that run under Wine (windows-checks.nix);
  # the other tests are not cross-built here.
  ninjaFlags = [ "homm1" "homm1_editor" "file_test" "data_root_test" ];
  outputs = [ "out" "tests" ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -m755 heroes.exe heroes-editor.exe $out/bin/
    mkdir -p $tests/bin
    install -m755 tests/port/file_test.exe tests/port/data_root_test.exe $tests/bin/
    runHook postInstall
  '';

  # Every program may import only Windows' own DLLs, and none is shipped.
  # (An install check would be skipped: the build machine cannot run them.)
  postFixup = ''
    for program in $out/bin/*.exe $tests/bin/*.exe; do
      for dll in $(${cross.stdenv.cc.targetPrefix}objdump -p "$program" | sed -n 's/^[[:space:]]*DLL Name: //p' | tr 'A-Z' 'a-z'); do
        case " ${pkgs.lib.concatStringsSep " " windowsSystemDlls} " in
          *" $dll "*) ;;
          *) echo "$program imports $dll, which is not part of Windows" >&2; exit 1 ;;
        esac
      done
    done
    # (stdenv globs with nullglob: no match is an empty list.)
    for dll in $out/bin/*.dll $tests/bin/*.dll; do
      echo "the Windows package is meant to be static, but ships $dll" >&2
      exit 1
    done
  '';

  passthru = { inherit sdl3; };
  meta.platforms = [ "x86_64-windows" ];
}

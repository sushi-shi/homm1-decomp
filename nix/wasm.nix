# The browser build (docs/port/README.md, "In a browser"): the game and the
# scenario editor compiled with Emscripten, with SDL3 and a minimal FFmpeg
# (Smacker movies and Ogg Vorbis music only) built for WebAssembly, and the
# static page that takes the player's own game files. No game data is part
# of any output here.
{ pkgs, src }:
let
  emscripten = pkgs.emscripten;

  # Emscripten builds its system libraries into a cache on first use; the
  # store copy is read-only, so each build works on a private copy.
  emscriptenSetup = ''
    export HOME=$TMPDIR
    export EM_CACHE=$TMPDIR/emscripten-cache
    cp -r --no-preserve=mode ${emscripten}/share/emscripten/cache $EM_CACHE
  '';

  sdl3 = pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl3-wasm";
    inherit (pkgs.sdl3) version src;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.ninja pkgs.python3 ];
    dontFixup = true;
    configurePhase = emscriptenSetup + ''
      emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$out -DSDL_SHARED=OFF -DSDL_STATIC=ON \
        -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF
    '';
    buildPhase = "cmake --build build";
    installPhase = "cmake --install build";
  };

  # Only what the game plays: Smacker video and audio in .smk files and Ogg
  # Vorbis music, read from the virtual file system, resampled by
  # libswresample. No assembly, threads, network or programs.
  # configure also builds small host tools, hence a host compiler (stdenv).
  ffmpeg = pkgs.stdenv.mkDerivation {
    pname = "ffmpeg-minimal-wasm";
    inherit (pkgs.ffmpeg-headless) version src;
    nativeBuildInputs = [ emscripten pkgs.python3 pkgs.llvm pkgs.pkg-config ];
    dontFixup = true;
    configurePhase = emscriptenSetup + ''
      emconfigure ./configure --prefix=$out \
        --target-os=none --arch=x86_32 --enable-cross-compile \
        --cc=emcc --cxx=em++ --host-cc=cc --ar=emar --ranlib=emranlib --nm=llvm-nm \
        --disable-asm --disable-x86asm --disable-inline-asm \
        --disable-runtime-cpudetect --disable-autodetect --disable-pthreads \
        --disable-network --disable-programs --disable-doc --disable-debug \
        --disable-stripping --disable-everything \
        --disable-avdevice --disable-avfilter --disable-swscale \
        --enable-avformat --enable-avcodec --enable-swresample \
        --enable-decoder=smacker,smackaud,vorbis \
        --enable-demuxer=smacker,ogg --enable-protocol=file \
        --enable-static --disable-shared
    '';
    # configure only warns about unknown component names; insist on each.
    postConfigure = ''
      for component in SMACKER_DECODER SMACKAUD_DECODER VORBIS_DECODER \
          SMACKER_DEMUXER OGG_DEMUXER FILE_PROTOCOL; do
        grep -q "define CONFIG_$component 1" config_components.h \
          || { echo "FFmpeg configured without $component"; exit 1; }
      done
    '';
    buildPhase = "make -j$NIX_BUILD_CORES";
    installPhase = "make install";
  };

  # The game and the editor, and the page that runs them: the site to serve
  # is $out/share/homm1-web.
  site = pkgs.stdenvNoCC.mkDerivation {
    pname = "homm1-wasm";
    version = "0";
    inherit src;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.ninja pkgs.python3 pkgs.pkg-config ];
    dontFixup = true;
    configurePhase = emscriptenSetup + ''
      export PKG_CONFIG_PATH=${ffmpeg}/lib/pkgconfig
      emcmake cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
        -DSDL3_DIR=${sdl3}/lib/cmake/SDL3 "-DCMAKE_FIND_ROOT_PATH=${sdl3};${ffmpeg}"
    '';
    buildPhase = "cmake --build build";
    installPhase = ''
      site=$out/share/homm1-web
      mkdir -p $site
      cp build/heroes.js build/heroes.wasm build/heroes-editor.js build/heroes-editor.wasm $site/
      cp src/PLATFORM/Web/index.html src/PLATFORM/Web/homm1.js src/PLATFORM/Web/homm1.css $site/
    '';
  };

  python = pkgs.python3.withPackages (p: [ p.playwright ]);

  # Serves the page on http://127.0.0.1:8000/ (or the port given).
  serve = pkgs.writeShellApplication {
    name = "homm1-web";
    runtimeInputs = [ pkgs.python3 ];
    text = ''
      port=''${1:-8000}
      echo "Heroes of Might and Magic: http://127.0.0.1:$port/"
      exec python3 -c '
      import functools, http.server, sys
      handler = http.server.SimpleHTTPRequestHandler
      handler.extensions_map[".wasm"] = "application/wasm"
      http.server.ThreadingHTTPServer(("127.0.0.1", int(sys.argv[1])),
          functools.partial(handler, directory=sys.argv[2])).serve_forever()
      ' "$port" ${site}/share/homm1-web
    '';
  };

  # The page in headless Chromium or Firefox on the player's own data
  # (tests/port/web_smoke.py; pass --data DIR, optionally --cd, --help-file,
  # --browser firefox and --out DIR for the screenshots).
  smoke = pkgs.writeShellApplication {
    name = "homm1-web-smoke";
    runtimeInputs = [ python ];
    text = ''
      export PLAYWRIGHT_BROWSERS_PATH=${pkgs.playwright-driver.browsers}
      export PLAYWRIGHT_SKIP_VALIDATE_HOST_REQUIREMENTS=true
      exec python3 ${src}/tests/port/web_smoke.py ${site}/share/homm1-web "$@"
    '';
  };
in
{
  inherit sdl3 ffmpeg site serve smoke;
}

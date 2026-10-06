# FFmpeg cut down to what the port decodes: Smacker movies (video and audio)
# and Ogg Vorbis music, through libavformat, libavcodec, libavutil and
# libswresample. Cross builds (Windows, the browser) use it instead of the
# full FFmpeg, which pulls in dozens of codec libraries.
{ stdenv, lib, ffmpeg-headless, buildPackages, extraConfigureFlags ? [ ], shared ? true }:

stdenv.mkDerivation {
  pname = "ffmpeg-homm1";
  inherit (ffmpeg-headless) version src;

  depsBuildBuild = [ buildPackages.stdenv.cc ];
  nativeBuildInputs = [ buildPackages.pkg-config ];

  # FFmpeg's configure is hand-written: it takes the cross prefix rather than
  # autoconf's --host.
  configurePlatforms = [ ];
  dontAddStaticConfigureFlags = true;
  configureFlags = [
    "--disable-everything"
    "--disable-autodetect"
    "--disable-programs"
    "--disable-doc"
    "--disable-network"
    "--disable-debug"
    "--disable-avdevice"
    "--disable-avfilter"
    "--disable-swscale"
    "--disable-x86asm"
    "--enable-avformat"
    "--enable-avcodec"
    "--enable-swresample"
    "--enable-protocol=file"
    "--enable-demuxer=smacker"
    "--enable-demuxer=ogg"
    "--enable-decoder=smacker"
    "--enable-decoder=smackaud"
    "--enable-decoder=vorbis"
    "--enable-parser=vorbis"
    "--host-cc=${buildPackages.stdenv.cc.targetPrefix}cc"
  ] ++ (if shared then [ "--enable-shared" "--disable-static" ] else [ "--enable-static" "--disable-shared" ])
    ++ lib.optionals (stdenv.hostPlatform != stdenv.buildPlatform) [
      "--enable-cross-compile"
      "--cross-prefix=${stdenv.cc.targetPrefix}"
      "--arch=${stdenv.hostPlatform.parsed.cpu.name}"
    ] ++ lib.optionals stdenv.hostPlatform.isWindows [ "--target-os=mingw32" ]
    ++ extraConfigureFlags;

  # configure only warns about a component name it does not know; make sure
  # each one the port needs is really built.
  postConfigure = ''
    for component in SMACKER_DEMUXER OGG_DEMUXER SMACKER_DECODER SMACKAUD_DECODER \
        VORBIS_DECODER FILE_PROTOCOL; do
      grep -q "^#define CONFIG_$component 1" config_components.h \
        || { echo "FFmpeg component $component is not enabled"; exit 1; }
    done
  '';

  enableParallelBuilding = true;
  doCheck = false;
}

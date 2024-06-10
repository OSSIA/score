{ stdenv
, lib
, fetchFromGitHub
, cmake
, ninja
, pkg-config
, alsa-lib
, avahi
, bluez
, boost185
# , faust
, fmt
, git
, ffmpeg
, fftw
, flac
, gnutls
, lame
, libcoap
, libjack2
, libopus
, libsndfile
, libvorbis
, lilv
, lv2
, mpg123
, pipewire
, portaudio
, portmidi
, libsamplerate
, qt6
, rapidfuzz-cpp
, re2
, SDL2
, spdlog
, suil
, udev
}:

# TODO: figure out LLVM jit
# assert lib.versionAtLeast llvm.version "15";

stdenv.mkDerivation (finalAttrs: {
  pname = "ossia-score";
  version = "devel";
  src = ../.;
  nativeBuildInputs = [ cmake ninja pkg-config qt6.qttools qt6.wrapQtAppsHook ];

  buildInputs = [
    alsa-lib
    boost185
    # faust
    avahi
    bluez
    ffmpeg
    fftw
    flac
    fmt
    git
    gnutls
    lame
    libcoap
    libjack2
    libopus
    libsamplerate
    libsndfile
    libvorbis
    lilv
    lv2
    mpg123
    pipewire
    portaudio
    portmidi
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qtserialport
    qt6.qtscxml
    qt6.qtshadertools
    qt6.qtsvg
    qt6.qtwayland
    qt6.qtwebsockets
    rapidfuzz-cpp
    re2
    SDL2
    spdlog
    suil
    udev
  ];

  cmakeFlags = [
    "-Wno-dev"

    "-DSCORE_DEPLOYMENT_BUILD=1"
    "-DSCORE_STATIC_PLUGINS=1"
    "-DSCORE_FHS_BUILD=1"
    "-DCMAKE_UNITY_BUILD=1"
    "-DCMAKE_SKIP_RPATH=ON"
    "-DOSSIA_USE_SYSTEM_LIBRARIES=1"

    "-DLilv_INCLUDE_DIR=${lilv.dev}/include/lilv-0"
    "-DLilv_LIBRARY=${lilv}/lib/liblilv-0.so"

    "-DSuil_INCLUDE_DIR=${suil}/include/suil-0"
    "-DSuil_LIBRARY=${suil}/lib/libsuil-0.so"
  ];

  # Ossia dlopen's these at runtime, refuses to start without them
  env.NIX_LDFLAGS = toString [
    "-llilv-0"
    "-lsuil-0"
  ];

  installPhase = ''
    runHook preInstall

    cmake -DCMAKE_INSTALL_DO_STRIP=1 -DCOMPONENT=OssiaScore -P cmake_install.cmake

    runHook postInstall
  '';

  meta = with lib; {
    homepage = "https://ossia.io/score/about.html";
    description = "A sequencer for audio-visual artists, designed to enable the creation of interactive shows, museum installations, intermedia digital artworks, interactive music and more";
    # TODO: this should work for darwin too
    platforms = platforms.linux;
    license = licenses.gpl3Only;
    maintainers = with maintainers; [ minijackson jcelerier ];
  };
})

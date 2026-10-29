# syntax=docker/dockerfile:1
FROM ubuntu:26.04@sha256:f144425ff09be612d6d9ad965196e9cdc23dae1f42110a8a11a3e9a8198759f7 AS build
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C.UTF-8 TZ=UTC
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake git ninja-build pkg-config python3 \
    libbluetooth-dev libegl-dev libevdev-dev libgl-dev libsystemd-dev libudev-dev \
    libx11-dev libxi-dev libxrandr-dev && rm -rf /var/lib/apt/lists/*
RUN git init --quiet /src && cd /src \
 && git remote add origin https://github.com/dolphin-emu/dolphin.git \
 && git fetch --quiet --depth 1 origin de44626d23a85aa3cc07260f6f97e64f36600652 \
 && git checkout --quiet FETCH_HEAD \
 && git submodule update --quiet --init --recursive --depth 1 --jobs 4
COPY dolphin/*.patch /src/patches/
RUN cd /src && git apply patches/*.patch \
 && cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/dolphin \
    -DENABLE_QT=OFF -DENABLE_NOGUI=ON -DENABLE_TESTS=OFF -DENABLE_AUTOUPDATE=OFF \
    -DENABLE_ANALYTICS=OFF -DUSE_DISCORD_PRESENCE=OFF -DUSE_RETRO_ACHIEVEMENTS=OFF \
    -DUSE_MGBA=OFF -DUSE_UPNP=OFF -DENABLE_LLVM=OFF -DENCODE_FRAMEDUMPS=OFF -DENABLE_SDL=OFF \
    -DENABLE_ALSA=OFF -DENABLE_PULSEAUDIO=OFF -DENABLE_CUBEB=OFF \
 && cmake --build /build -j2 && cmake --install /build \
 && strip /opt/dolphin/bin/dolphin-emu-nogui

FROM ubuntu:26.04@sha256:f144425ff09be612d6d9ad965196e9cdc23dae1f42110a8a11a3e9a8198759f7
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C.UTF-8 TZ=UTC
RUN apt-get update && apt-get install -y --no-install-recommends \
    dosfstools ffmpeg libbluetooth3 libegl-mesa0 libegl1 libevdev2 libgl1 libgl1-mesa-dri \
    libudev1 libx11-6 libxi6 libxrandr2 mtools python3 python3-numpy python3-pil xauth xvfb \
 && rm -rf /var/lib/apt/lists/*
COPY --from=build /opt/dolphin /opt/dolphin
RUN ln -s /opt/dolphin/bin/dolphin-emu-nogui /usr/local/bin/dolphin-emu-nogui \
 && mkdir -m 1777 /tmp/.X11-unix \
 && ! ldd /opt/dolphin/bin/dolphin-emu-nogui | grep 'not found'
WORKDIR /work

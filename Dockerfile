FROM ubuntu:24.04

ARG DEBUG=0
ENV DEBUG=${DEBUG:-0}

RUN apt-get update && apt-get install -y clang-20 cmake git lldb valgrind

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-20 100
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-20 100

WORKDIR /srv/

COPY ./WinQuake /srv/WinQuake
COPY ./build-native.sh /srv/build-native.sh

RUN --mount=type=cache,target=/srv/WinQuake/build-native ./build-native.sh

RUN --mount=type=cache,target=/srv/WinQuake/build-native cp -frv /srv/WinQuake/build-native /srv/WinQuake/build-linux

ENV LD_LIBRARY_PATH=/srv/WinQuake/build-linux

WORKDIR /srv/WinQuake

RUN test -e /srv/WinQuake/build-linux/libSDL2-2.0d.so.0.3200.4 && cp -frv /srv/WinQuake/build-linux/libSDL2-2.0d.so.0.3200.4 /usr/local/lib/libSDL2-2.0d.so.0.3200.4 || true

RUN test -e /srv/WinQuake/build-linux/libSDL2-2.0.so.0.3200.4 && cp -frv /srv/WinQuake/build-linux/libSDL2-2.0.so.0.3200.4 /usr/local/lib/libSDL2-2.0.so.0.3200.4 || true

RUN ln -s /usr/bin/llvm-symbolizer-19 /usr/bin/llvm-symbolizer

ENTRYPOINT ["/srv/WinQuake/build-linux/Quake", "-dedicated", "16", "+map", "dm6"]

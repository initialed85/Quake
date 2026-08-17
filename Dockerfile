FROM ubuntu:24.04

RUN apt-get update && apt-get install -y clang-20 cmake git lldb valgrind libsdl2-dev

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-20 100
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-20 100

WORKDIR /srv/

COPY ./WinQuake /srv/WinQuake
COPY ./build-native.sh /srv/build-native.sh

ARG DEBUG=0
ENV DEBUG=${DEBUG:-0}

RUN ./build-native.sh

RUN cp -frv /srv/WinQuake/build-native /srv/WinQuake/build-linux

ENV LD_LIBRARY_PATH=/srv/WinQuake/build-linux

WORKDIR /srv/WinQuake

RUN ln -s /usr/bin/llvm-symbolizer-20 /usr/bin/llvm-symbolizer

ENTRYPOINT ["/srv/WinQuake/build-linux/Quake", "-dedicated", "16", "+map", "dm6"]

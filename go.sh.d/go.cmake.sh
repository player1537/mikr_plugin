#!/usr/bin/env bash

[ -n "${__GO_CMAKE_SH__+isset}" ] || {
__GO_CMAKE_SH__=

cmake=
config=()
source=
build=
stage=

go--cmake-configure() {
    cmake -H"${source:?}" -B"${build:?}" \
        -DCMAKE_INSTALL_PREFIX:PATH="${stage:?}" \
        "${config[@]}"
}

go--cmake-build() {
    make -C"${build:?}" \
        VERBOSE=1 \
        "$@"
}

go--cmake-install() {
    make -C"${build:?}" \
        VERBOSE=1 \
        install
}

}  # __GO_CMAKE_SH__
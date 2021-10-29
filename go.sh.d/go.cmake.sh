#!/usr/bin/env bash

[ -z "${cmake+isset}" ] || return

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
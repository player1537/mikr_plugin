#!/usr/bin/env bash

die() { printf $'Error: %s\n' "$*" >&2; exit 1; }

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)
PATH=${root:?}/go.sh.d${PATH:+:${PATH:?}}
project=${root##*/}

source go.clean.sh
go--clean-pollute \
    go-clean \
    go-distclean

source go.exec.sh
go--exec-pollute \
    go-exec

source go.package.sh
go--package-pollute \
    go-package

source go.ospray.sh
go--ospray-pollute \
    go-ospray \
    go-ospray-clone \
    go-ospray-clean \
    go-ospray-configure \
    go-ospray-build \
    go-ospray-exec \
    \$clean \
    \$distclean \
    \$exec

source go.studio.sh
go--studio-pollute \
    go-studio \
    go-studio-clone \
    go-studio-clean \
    go-studio-configure \
    go-studio-build \
    go-studio-install \
    go-studio-exec \
    \$clean \
    \$distclean \
    \$exec

source go.spack.sh
go--spack-pollute \
    go-spack \
    go-spack-clone \
    go-spack-install \
    go-spack-exec \
    \$clean \
    \$distclean \
    \$package \
    \$exec

source go.mikr_plugin.sh
go--mikr_plugin-pollute \
    go-mikr_plugin \
    go-mikr_plugin-clone

go-buildall() (
    source "${root:?}/go.sh.d/go.makefile.sh"
    go--makefile-pollute \
        init=-init \
        run=-run \
        target=-target \
        depend=-depend \
        rule=-rule

    -init

    -target .PHONY
    -depend all ospray-{clone,configure,build} studio-{clone,configure,build,install}

    -target all
    -depend studio-install

    -target ospray-clone
    -rule "${root:?}/go.sh" ospray clone

    -target mikr_plugin-clone
    -depend ospray-clone
    -rule "${root:?}/go.sh" mikr_plugin clone

    -target ospray-configure
    -depend mikr_plugin-clone
    -rule "${root:?}/go.sh" ospray configure

    -target ospray-build
    -depend ospray-configure
    -rule ${MAKEFLAGS:+MAKEFLAGS="${MAKEFLAGS:?}"} "${root:?}/go.sh" ospray build

    -target studio-clone
    -rule "${root:?}/go.sh" studio clone

    -target studio-configure
    -depend studio-clone ospray-build
    -rule "${root:?}/go.sh" studio configure

    -target studio-build
    -depend studio-configure
    -rule ${MAKEFLAGS:+MAKEFLAGS="${MAKEFLAGS:?}"} "${root:?}/go.sh" studio build

    -target studio-install
    -depend studio-build
    -rule "${root:?}/go.sh" studio install

    -run MAKEFLAGS= "$@"
)

go-parbuildall() {
    MAKEFLAGS=-j$(nproc) go buildall
}

go() {
    case "${1:-}" in
    (go-*) "$@";;
    (*) go-"$@";;
    esac
}

for f in \
    env.sh \
    ${HOSTNAME:+@${HOSTNAME:?}.env.sh} \
    ${USER:+${USER:?}@.env.sh} \
    ${USER:+${HOSTNAME:+${USER:?}@${HOSTNAME:?}.env.sh}}
do
    [ -f "${root:?}/${f:?}" ] && source "${root:?}/${f:?}" "$@"
done

go "$@"
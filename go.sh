#!/usr/bin/env bash

die() { printf $'Error: %s\n' "$*" >&2; exit 1; }

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)
project=${root##*/}

source "${root:?}/go.sh.d/go.ospray.sh" "$@"
go--ospray-pollute \
    go-ospray \
    go-ospray-clone \
    go-ospray-clean \
    go-ospray-configure \
    go-ospray-build \
    go-ospray-exec

source "${root:?}/go.sh.d/go.studio.sh" "$@"
go--studio-pollute \
    go-studio \
    go-studio-clone \
    go-studio-clean \
    go-studio-configure \
    go-studio-build \
    go-studio-install \
    go-studio-exec

source "${root:?}/go.sh.d/go.spack.sh" "$@"
go--spack-pollute \
    go-spack \
    go-spack-clone \
    go-spack-install \
    go-spack-exec

source "${root:?}/go.sh.d/go.mikr_plugin.sh" "$@"
go--mikr_plugin-pollute \
    go-mikr_plugin \
    go-mikr_plugin-clone

go-exec() {
    go ospray exec \
    go studio exec \
    go spack exec \
    "$@"
}

go-clean() {
    go studio clean &&
    go ospray clean
}

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

go-package() (
    shopt -s nullglob
    tar zcvf \
        "${root:?}/${project:?}.tar.gz" \
        --dereference \
        --xform "s!^!${project:?}/!" \
        go.sh \
        env.sh \
        *.env.sh \
        go.sh.d/go.*.sh \
        spack.yaml
)

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
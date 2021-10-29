#!/usr/bin/env bash

[ -z "${spack+isset}" ] || return

spack=${root:?}/spack
spackenv=${root:?}
spack_source=${spack:?}
spack_git=https://github.com/spack/spack.git
spack_ref=

go--spack-pollute() {
    for arg; do
        case "$arg" in
        (spack=*|go-spack) eval "${arg#spack=}"'() { go-spack-"$@"; }';;
        (clone=*|go-spack-clone) eval "${arg#clone=}"'() { go--spack-clone; }';;
        (install=*|go-spack-install) eval "${arg#install=}"'() { go--spack-install; }';;
        (exec=*|go-spack-exec) eval "${arg#exec=}"'() { go--spack-exec "$@"; }';;
        esac
    done
}


go--spack-clone() {
    source=${spack_source:?} \
    git=${spack_git:?} \
    ref=${spack_ref:-} \
    go--git-clone
}

go--spack-install() {
    SPACK_ENV=${spackenv:?} \
    ${spack:?}/bin/spack install \
        --yes-to-all
}

go--spack-exec() (  # subshell
    eval $(${spack:?}/bin/spack env activate --sh --dir "${spackenv:?}")
    "$@"
)
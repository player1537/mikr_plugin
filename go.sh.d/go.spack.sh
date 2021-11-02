#!/usr/bin/env bash

[ -n "${__GO_SPACK_SH__+isset}" ] || {
__GO_SPACK_SH__=

spack=${root:?}/spack
spackenv=${root:?}
spack_source=${spack:?}
spack_git=https://github.com/spack/spack.git
spack_ref=
spack_clean=(
)
spack_distclean=(
    "${spack_source:?}"
)
spack_package=(
    "${spackenv:?}/spack.yaml"
)
spack_exec=(
    go--spack-exec
)

go--spack-pollute() {
    for arg; do
        case "$arg" in
        (spack=*|go-spack) eval "${arg#spack=}"'() { go-spack-"$@"; }';;
        (clone=*|go-spack-clone) eval "${arg#clone=}"'() { go--spack-clone; }';;
        (install=*|go-spack-install) eval "${arg#install=}"'() { go--spack-install; }';;
        (exec=*|go-spack-exec) eval "${arg#exec=}"'() { go--spack-exec "$@"; }';;
        (\$clean=*|\$clean) eval ":; ${arg##*$}"'+=( "${spack_clean[@]}" )';;
        (\$distclean=*|\$distclean) eval ":; ${arg##*$}"'+=( "${spack_distclean[@]}" )';;
        (\$package=*|\$package) eval ":; ${arg##*$}"'+=( "${spack_package[@]}" )';;
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

}  # __GO_SPACK_SH__
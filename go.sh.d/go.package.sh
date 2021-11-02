#!/usr/bin/env bash

[ -n "${__GO_PACKAGE_SH__+isset}" ] || {
__GO_PACKAGE_SH__=

_shopt=$(shopt -p nullglob)
shopt -s nullglob
package=(
    "${root:?}"/go.sh
    "${root:?}"/env.sh
    "${root:?}"/*@*.env.sh
    "${root:?}"/go.sh.d/go.*.sh
)
eval "${_shopt:?}"
unset _shopt

go--package-pollute() {
    for arg; do case "$arg" in
        (package=*|go-package) eval "${arg#package=}"'() { go--package; }';;
    esac; done
}

go--package() {
    tar zcvf \
        "${root:?}/${project:?}.tar.gz" \
        --dereference \
        --transform "s!^!${project:?}/!" \
        --show-transformed-names \
        "${package[@]#${root:?}/}"
}

}  # __GO_PACKAGE_SH__
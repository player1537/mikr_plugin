#!/usr/bin/env bash

[ -n "${__GO_CLEAN_SH__+isset}" ] || {
__GO_CLEAN_SH__=

clean=()
distclean=()

go--clean-pollute() {
    for arg; do case "$arg" in
        (clean=*|go-clean) eval "${arg#clean=}"'() { go--clean; }';;
        (distclean=*|go-distclean) eval "${arg#distclean=}"'() { go--distclean; }';;
    esac; done
}

go--clean() {
    go--clean-ref clean
}

go--distclean() {
    go--clean-ref distclean
}

go--clean-ref() {
    local _clean
    : "${1:?need name of array}[@]"
    _clean=( "${!_}" )
    [ "${#_clean[@]}" -eq 0 ] && return
    printf $'rm -rfv' >&2
    printf $' \\\n\t%q' "${_clean[@]}" >&2
    printf $'\n' >&2
    if read -p 'Continue? [y/N] '; then
        case "$REPLY" in
        ([Yy]) rm -rfv "${_clean[@]}";;
        (*) printf $'Cancelled\n' >&2;;
        esac
    fi
}

}  # __GO_CLEAN_SH__
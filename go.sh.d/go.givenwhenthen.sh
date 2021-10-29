#!/usr/bin/env bash

[ -z "${givenwhenthen+isset}" ] || return

givenwhenthen=
givenwhenthen_namespace=_givenwhenthen

go--givenwhenthen-pollute() {
    for arg; do
        case "$arg" in
        (givenwhenthen=*|go-givenwhenthen) eval "${arg#givenwhenthen=}"'() { go-givenwhenthen-"$@"; }';;
        (init=*|go-givenwhenthen-init) eval "${arg#init=}"'() { go--givenwhenthen-init; }';;
        (label=*|go-givenwhenthen-label) eval "${arg#label=}"'() { go--givenwhenthen-label "$@"; }';;
        (given=*|go-givenwhenthen-given) eval "${arg#given=}"'() { go--givenwhenthen-given "$@"; }';;
        (and=*|go-givenwhenthen-and) eval "${arg#and=}"'() { go--givenwhenthen-and "$@"; }';;
        (when=*|go-givenwhenthen-when) eval "${arg#when=}"'() { go--givenwhenthen-when "$@"; }';;
        (then=*|go-givenwhenthen-then) eval "${arg#then=}"'() { go--givenwhenthen-then "$@"; }';;
        esac
    done
}

go--givenwhenthen-init() {
    printf -v "${givenwhenthen_namespace}_state" $'%s' INIT
}

go--givenwhenthen-label() {
    go--givenwhenthen--repl LABEL "$@"
}

go--givenwhenthen-given() {
    go--givenwhenthen--repl GIVEN "$@"
}

go--givenwhenthen-and() {
    go--givenwhenthen--repl AND "$@"
}

go--givenwhenthen-when() {
    go--givenwhenthen--repl WHEN "$@"
}

go--givenwhenthen-then() {
    go--givenwhenthen--repl THEN "$@"
}

go--givenwhenthen--repl() {
    local type
    type=${1:?}
    shift

    : "${givenwhenthen_namespace}_state"
    case "${!_:-unset}:${type:?}" in
    (SKIP:LABEL) printf -v "${givenwhenthen_namespace}_state" $'%s' INIT;;
    (SKIP:*) return;;
    esac

    printf $'%s:' "${type:?}"
    printf $' %q' "$@"
    printf $'\n'
    read -p $'Command (e[x]ecute, [s]kip, [y]es, [n]o): '
    case "$REPLY" in
    (x) "$@"; go--givenwhenthen--repl "${type:?}" "$@";;
    (s) printf -v "${givenwhenthen_namespace}_state" $'%s' SKIP;;
    (y|"") :;;
    (n) exit 1;;
    esac
}
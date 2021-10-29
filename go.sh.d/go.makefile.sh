#!/usr/bin/env bash

[ -z "${makefile+isset}" ] || return

makefile=${root:?}/Makefile
makefile_namespace=_makefile

go--makefile-pollute() {
    for arg; do
        case "$arg" in
        (makefile=|go-makefile) eval ":; ${arg#makefile=}"'() { go-makefile-"$@"; }';;
        (init=*|go-makefile-init) eval ":; ${arg#init=}"'() { go--makefile-init; }';;
        (run=*|go-makefile-run) eval ":; ${arg#run=}"'() { go--makefile-run "$@"; }';;
        (target=*|go-makefile-target) eval ":; ${arg#target=}"'() { go--makefile-target "$@"; }';;
        (depend=*|go-makefile-depend) eval ":; ${arg#depend=}"'() { go--makefile-depend "$@"; }';;
        (rule=*|go-makefile-rule) eval ":; ${arg#rule=}"'() { go--makefile-rule "$@"; }';;
        esac
    done
}

go--makefile-init() {
    exec 3>"${makefile:?}"
    printf -v "${makefile_namespace:?}_state" $'%s' "INIT"
}

go--makefile-run() {
    exec 3>&-
    make "$@"
    printf -v "${makefile_namespace:?}_state" $'%s' "RUN"
}

go--makefile-target() {
    for arg; do
        : "${makefile_namespace:?}_state"
        case "${!_}" in
        (INIT) printf $'%q' "${arg:?}" >&3;;
        (RULE) printf $'\n%q' "${arg:?}" >&3;;
        (TARGET) printf $' %q' "${arg:?}" >&3;;
        (DEPEND) printf $'\n%q' "${arg:?}" >&3;;
        (RUN) die "Unexpected state for go--makefile-target: RUN";;
        esac
        printf -v "${makefile_namespace:?}_state" $'%s' "TARGET"
    done
}

go--makefile-depend() {
    for arg; do
        : "${makefile_namespace:?}_state"
        case "${!_}" in
        (INIT|RULE|RUN) die "Unexpected state for go--makefile-depend: INIT|RULE|RUN";;
        (TARGET) printf $': %q' "${arg:?}" >&3;;
        (DEPEND) printf $' %q' "${arg:?}" >&3;;
        esac
        printf -v "${makefile_namespace:?}_state" $'%s' "DEPEND"
    done
}

go--makefile-rule() {
    for arg; do
        : "${makefile_namespace:?}_state"
        case "${!_}" in
        (INIT|RUN) die "Unexpected state for go--makefile-depend: INIT|RULE|RUN";;
        (TARGET) printf $':\n\t%q' "${arg:?}" >&3;;
        (DEPEND) printf $'\n\t%q' "${arg:?}" >&3;;
        (RULE) printf $'\n\t%q' "${arg:?}" >&3;;
        (RULEINT) printf $' %q' "${arg:?}" >&3;;
        esac
        printf -v "${makefile_namespace:?}_state" $'%s' "RULEINT"
    done
    : "${makefile_namespace:?}_state"
    case "${!_}" in
    (RULEINT) printf -v "${makefile_namespace:?}_state" $'%s' "RULE";;
    esac
}
#!/usr/bin/env bash

[ -n "${__GO_EXEC_SH__+isset}" ] || {
__GO_EXEC_SH__=

exec=()

go--exec-pollute() {
    for arg; do case "$arg" in
        (exec=*|go-exec) eval "${arg#exec=}"'() { go--exec; }';;
        (exec-ref=*|go-exec-ref) eval "${arg#exec-ref=}"'() { go--exec-ref; }';;
    esac; done
}

go--exec() {
    go--exec-ref \$exec "$@"
}

go--exec-ref() (
    local _exec
    : "${1#$}[@]}"
    _exec=( "${!_}" )
    "${_exec[@]}" "$@"
)

} # __GO_EXEC_SH__
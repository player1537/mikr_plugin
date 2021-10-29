#!/usr/bin/env bash

[ -z "${git+isset}" ] || return

git=
source=
ref=

go--git-clone() {
    if ! [ -d "${source:?}/.git" ]; then
        if git clone \
            "${git:?}" \
            "${source:?}"
        then
            if [ -n "${ref:-}" ]; then
                git -C "${source:?}" checkout "${ref:?}"
            fi
        fi
    fi
}
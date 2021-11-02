#!/usr/bin/env bash

[ -n "${__GO_GIT_SH__+isset}" ] || {
__GO_GIT_SH__=

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

}  # __GO_GIT_SH__
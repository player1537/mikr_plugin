#!/usr/bin/env bash

[ -n "${__GO_STUDIO_SH__+isset}" ] || {
__GO_STUDIO_SH__=

source go.git.sh
source go.cmake.sh
source go.exec.sh
source go.ospray.sh

studio=${root:?}/studio
studio_source=${studio:?}
studio_build=${studio:?}/build
studio_stage=${studio:?}/stage
studio_git=https://github.com/ospray/ospray_studio.git
studio_ref=
studio_config=(
    -Drkcommon_ROOT:PATH="${ospray_stage:?}"
    -Dospray_ROOT:PATH="${ospray_stage:?}"
)
studio_clean=(
    "${studio_build:?}"
    "${studio_stage:?}"
)
studio_distclean=(
    "${studio:?}"
)
studio_exec=(
    go--studio-exec
)

go--studio-pollute() {
    for arg; do case "$arg" in
        (studio=*|go-studio) eval ":; ${arg#studio=}"'() { go-studio-"$@"; }';;
        (clone=*|go-studio-clone) eval ":; ${arg#clone=}"'() { go--studio-clone; }';;
        (clean=*|go-studio-clean) eval ":; ${arg#clean=}"'() { go--studio-clean; }';;
        (configure=*|go-studio-configure) eval ":; ${arg#configure=}"'() { go--studio-configure; }';;
        (build=*|go-studio-build) eval ":; ${arg#build=}"'() { go--studio-build; }';;
        (install=*|go-studio-install) eval ":; ${arg#install=}"'() { go--studio-install; }';;
        (exec=*|go-studio-exec) eval ":; ${arg#exec=}"'() { go--studio-exec "$@"; }';;
        (\$clean=*|\$clean) eval ":; ${arg##*$}"'+=( "${studio_clean[@]}" )';;
        (\$distclean=*|\$distclean) eval ":; ${arg##*$}"'+=( "${studio_distclean[@]}" )';;
        (\$exec=*|\$exec) eval ":; ${arg##*$}"'+=( "${studio_exec[@]}" )';;
    esac; done
}

go--studio-clone() {
    source=${studio_source:?} \
    git=${studio_git:?} \
    ref=${studio_ref:-} \
    go--git-clone
}

go--studio-clean() {
    rm -rfv \
        "${studio_build:?}" \
        "${studio_stage:?}"
}

go--studio-configure() {
    config=( "${studio_config[@]}" )
    source=${studio_source:?} \
    build=${studio_build:?} \
    stage=${studio_stage:?} \
    go--cmake-configure
}

go--studio-build() {
    build=${studio_build:?} \
    go--cmake-build
}

go--studio-install() {
    build=${studio_build:?} \
    go--cmake-install
}

go--studio-exec() (
    LD_LIBRARY_PATH=${studio_stage:?}/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    PATH=${studio_stage:?}/bin${PATH:+:${PATH:?}} \
    "$@"
)

}  # __GO_STUDIO_SH__
#!/usr/bin/env bash

[ -n "${__GO_OSPRAY_SH__+isset}" ] || {
__GO_OSPRAY_SH__=

ospray=${root:?}/ospray
ospray_source=${ospray:?}
ospray_build=${ospray:?}/build
ospray_stage=${ospray:?}/stage
ospray_git=ssh://git@vis-gitlab.an.intel.com:renderkit/ospray.git
ospray_ref=
ospray_config=(
    -DINSTALL_IN_SEPARATE_DIRECTORIES:BOOL=OFF
)
ospray_clean=(
    "${ospray_build:?}"
    "${ospray_stage:?}"
)
ospray_distclean=(
    "${ospray:?}"
)
ospray_exec=(
    go--ospray-exec
)

go--ospray-pollute() {
    for arg; do case "$arg" in
        (ospray=*|go-ospray) eval ":; ${arg#ospray=}"'() { go-ospray-"$@"; }';;
        (clone=*|go-ospray-clone) eval ":; ${arg#clone=}"'() { go--ospray-clone; }';;
        (clean=*|go-ospray-clean) eval ":; ${arg#clean=}"'() { go--ospray-clean; }';;
        (configure=*|go-ospray-configure) eval ":; ${arg#configure=}"'() { go--ospray-configure; }';;
        (build=*|go-ospray-build) eval ":; ${arg#build=}"'() { go--ospray-build; }';;
        (fix=*|go-ospray-fix) eval ":; ${arg#fix=}"'() { go--ospray-fix; }';;
        (exec=*|go-ospray-exec) eval ":; ${arg#exec=}"'() { go--ospray-exec "$@"; }';;
        (\$clean=*|\$clean) eval ":; ${arg##*$}"'+=( "${ospray_clean[@]}" )';;
        (\$distclean=*|\$distclean) eval ":; ${arg##*$}"'+=( "${ospray_distclean[@]}" )';;
        (\$exec=*|\$exec) eval ":; ${arg##*$}"'+=( "${ospray_exec[@]}" )';;
    esac; done
}

go--ospray-clone() {
    source=${ospray_source:?} \
    git=${ospray_git:?} \
    ref=${ospray_ref:-} \
    go--git-clone
}

go--ospray-clean() {
    rm -rfv \
        "${ospray_build:?}" \
        "${ospray_stage:?}"
}

go--ospray-configure() {
    config=( "${ospray_config[@]}" )
    source=${ospray_source:?}/scripts/superbuild \
    build=${ospray_build:?} \
    stage=${ospray_stage:?} \
    go--cmake-configure
}

go--ospray-build() {
    build=${ospray_build:?} \
    go--cmake-build
}

go--ospray-fix() ( # subshell
    shopt -s failglob
    cd "${ospray_stage:?}"/lib/cmake
    for f in embree openvkl ospray rkcommon; do
        for g in "${f:?}"-*; do
            ln -sf "${g:?}" "${f:?}"
        done
    done
)

go--ospray-exec() (
    LD_LIBRARY_PATH=${ospray_stage:?}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    LD_LIBRARY_PATH=${ospray_stage:?}/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    PATH=${ospray_stage:?}/bin${PATH:+:${PATH:?}} \
    "$@"
)

}  # __GO_OSPRAY_SH__
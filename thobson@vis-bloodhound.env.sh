ospray_ref=devel
ospray_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/renderkit/ospray
ospray_config+=(
    -DCMAKE_BUILD_TYPE:STRING=Debug
    -DDEPENDENCIES_BUILD_TYPE:STRING=Debug
)

studio_ref=devel
studio_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/renderkit/ospray_studio
studio_config+=(
    -DCMAKE_BUILD_TYPE:STRING=Debug
)

mikr_plugin_ref=devel
mikr_plugin_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/thobson/mikr_plugin


[ -n "${__GO_REPL_SH__+isset}" ] || {
__GO_REPL_SH__=

go-repl() {
    go-repl-"$@"
}

go-repl-git/local() {
    if ! cd "${mikr_plugin_source:?}"; then
        printf $'Error: %s: Could not change directories\n' "${FUNCNAME[0]}" >&2
        return 1
    fi

    repl \
        git -C "${mikr_plugin_source:?}" \
        --repl-setup status \
        --repl-teardown log -n 1 --oneline
}

go-repl-git/origin() {
    : "${mikr_plugin_git:?}"
    if ! cd "${mikr_plugin_git#file://}"; then
        printf $'Error: %s: Could not change directories\n' "${FUNCNAME[0]}" >&2
        return 1
    fi

    repl \
        git -C "${mikr_plugin_git#file://}" \
        --repl-setup status \
        --repl-setup log -n 1 --oneline
}


go-repl-git/go.sh() {
    local clean
    
    clean=( "${root:?}/${project:?}" )
    if ! go--clean-ref clean; then
        printf $'Error: %s: Clean cancelled or failed; bailing early\n' "${FUNCNAME[0]}" >&2
        return 1
    fi

    if ! go package; then
        printf $'Error: %s: Package cancelled or failed; bailing early\n' "${FUNCNAME[0]}" >&2
        return 1
    fi

    if ! tar xvf "${root:?}/${project:?}.tar.gz" -C "${root:?}"; then
        printf $'Error: %s: Unpackage cancelled or failed; bailing early\n' "${FUNCNAME[0]}" >&2
        return 1
    fi

    : "${mikr_plugin_git:?}"
    repl \
        --repl-setup git -C "${mikr_plugin_git#file://}" checkout -f go.sh \
        --repl-teardown git -C "${mikr_plugin_git#file://}" checkout -f orphan \
        --repl-prefix git --git-dir "${mikr_plugin_git#file://}/.git" --work-tree "${root:?}/${project:?}"
}

}  # __GO_REPL_SH__
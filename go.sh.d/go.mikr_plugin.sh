#!/usr/bin/env bash

[ -z "${mikr_plugin+isset}" ] || return

source "${root:?}/go.sh.d/go.studio.sh" "$@"
source "${root:?}/go.sh.d/go.git.sh" "$@"

mikr_plugin=${studio:?}/plugins/mikr_plugin
mikr_plugin_source=${mikr_plugin:?}
mikr_plugin_git=git@vis-gitlab.an.intel.com:thobson/mikr_plugin.git
mikr_plugin_ref=

go--mikr_plugin-pollute() {
    for arg; do
        case "$arg" in
        (mikr_plugin=*|go-mikr_plugin) eval "${arg#mikr_plugin=}"'() { go-mikr_plugin-"$@"; }';;
        (clone=*|go-mikr_plugin-clone) eval "${arg#clone=}"'() { go--mikr_plugin-clone; }';;
        esac
    done
}

go--mikr_plugin-clone() {
    source=${mikr_plugin_source:?} \
    git=${mikr_plugin_git:?} \
    ref=${mikr_plugin_ref:-} \
    go--git-clone
}
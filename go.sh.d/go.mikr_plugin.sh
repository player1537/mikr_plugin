#!/usr/bin/env bash

[ -n "${__GO_MIKR_PLUGIN_SH__+isset}" ] || {
__GO_MIKR_PLUGIN_SH__=

source go.studio.sh
source go.git.sh

mikr_plugin=${studio:?}/plugins/mikr_plugin
mikr_plugin_source=${mikr_plugin:?}
mikr_plugin_git=https://github.com/player1537/mikr_plugin.git
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

}  # __GO_MIKR_PLUGIN_SH__
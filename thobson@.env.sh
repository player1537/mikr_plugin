ospray_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/renderkit/ospray
studio_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/renderkit/ospray_studio
mikr_plugin_git=file://${HOME:?}/git/vis-gitlab.an.intel.com/thobson/mikr_plugin

studio_config+=(
    -DCMAKE_BUILD_TYPE:STRING=Debug
)
ospray_config+=(
    -DCMAKE_BUILD_TYPE:STRING=Debug
    -DDEPENDENCIES_BUILD_TYPE:STRING=Debug
)

go-nofree-build() {
    printf $'void free(void *v) { (void)v; }\n' | \
    gcc -Wall -fPIC -shared -x c -nostdlib - -o "${root:?}/nofree.so"
}

go-code() {
    code \
        "${root:?}/go.sh" \
        "${root:?}/env.sh" \
        "${root:?}/thobson@.env.sh" \
        "${mikr_plugin_source:?}/CMakeLists.txt" \
        "${mikr_plugin_source:?}/plugin_mikr.cpp" \
        "${mikr_plugin_source:?}/PanelMikr.cpp" \
        "${mikr_plugin_source:?}/PanelMikr.h" \
        "${mikr_plugin_source:?}/plugin_mikr.py" \
        "${studio_source:?}/app/MainWindow.cpp"
}
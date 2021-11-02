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
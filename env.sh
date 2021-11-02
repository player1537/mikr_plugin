ospray_ref=v2.7.1
studio_ref=v0.8.1
mikr_plugin_ref=master

studio_config+=(
    -DBUILD_PLUGINS:BOOL=ON
    -DBUILD_PLUGIN_EXAMPLE:BOOL=ON
    -DBUILD_PLUGIN_MIKR:BOOL=ON
)

go-ospStudio() {
    go exec ospStudio "$@"
}
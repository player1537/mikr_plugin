ospray_ref=devel
studio_ref=devel
mikr_plugin_ref=devel
studio_config+=(
    -DBUILD_PLUGINS:BOOL=ON
    -DBUILD_PLUGIN_EXAMPLE:BOOL=ON
    -DBUILD_PLUGIN_MIKR:BOOL=ON
)

go-ospStudio() {
    go exec ospStudio "$@"
}
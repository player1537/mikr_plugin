## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

option(BUILD_PLUGIN_EXAMPLE "Mikr plugin" OFF)

if (BUILD_PLUGIN_EXAMPLE)
  set( pluginName "ospray_studio_plugin_mikr")

  add_library(${pluginName} SHARED
    plugin_mikr.cpp
    PanelMikr.cpp
  )

  target_link_libraries(${pluginName} ospray_sg)

  # Only link against imgui if needed (ie, pure file importers don't)
  target_link_libraries(${pluginName} imgui)

  target_include_directories(${pluginName}
    PRIVATE ${CMAKE_SOURCE_DIR}
  )

  install(TARGETS ${pluginName}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT lib
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT lib
  )

endif()
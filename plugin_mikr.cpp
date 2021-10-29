// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelMikr.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

#include <memory>
#include <optional>


namespace ospray {
namespace mikr_plugin {

struct PluginMikr : public Plugin
{
  PluginMikr() : Plugin("Mikr") {}

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.argc;
      const char **av = studioCommon.argv;

      std::string optDefaultFilename;

      for (int i=1; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:mikr:defaultFilename") {
          optDefaultFilename = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelMikr(ctx, optDefaultFilename));
    }
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_mikr()
{
  std::cout << "loaded plugin 'mikr'!" << std::endl;
  return new PluginMikr();
}

} // namespace mikr_plugin
} // namespace ospray
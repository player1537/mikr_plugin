// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

namespace ospray {
namespace mikr_plugin {

struct PanelMikr : public Panel
{
  PanelMikr(std::shared_ptr<StudioContext> _context);

  void buildUI(void *ImGuiCtx) override;

  void startCoProcess();
  void loadInCoProcess();
  void transferFromCoProcess();
  void createGeometry();
  void stopCoProcess();

protected:
  struct {
    int pid; // coprocess.pid
    FILE *stdin; // coprocess.stdin
    FILE *stdout; // coprocess.stdout
  } coprocess;

  struct {
    size_t count; // timesteps.count
    size_t selected; // timesteps.selected
    std::vector<std::string> names; // timesteps.names
  } timesteps;

  struct {
    struct {
      size_t count; // vertex.position.count
      size_t width; // vertex.position.width
      vec3f *data; // vertex.position.data
      vec3f minimum; // vertex.position.minimum
      vec3f maximum; // vertex.position.maximum
    } position; // vertex.position
  } vertex;

  struct {
    size_t count; // index.count
    size_t width; // index.width
    uint32_t *data; // index.data
  } index;

  struct {
    struct {
      size_t count; // cell.index.count
      size_t width; // cell.index.width
      uint32_t *data; // cell.index.data
    } index;
    struct {
      size_t count; // cell.type.count
      size_t width; // cell.type.width
      uint8_t *data; // cell.type.data
    } type;
    struct {
      size_t count; // cell.data.count
      size_t width; // cell.data.width
      float *data; // cell.data.data
      float minimum; // cell.data.minimum
      float maximum; // cell.data.maximum
    } data; // cell.data
  } cell;
};

}  // namespace mikr_plugin
}  // namespace ospray

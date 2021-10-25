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
  using clock = std::chrono::system_clock;
  using time_point = std::chrono::time_point<clock>;
  using duration = std::chrono::duration<float>;

  struct {
    struct {
      enum {
        INITED, // ui.coprocess.INITED
        STARTED, // ui.coprocess.STARTED
        LOADED, // ui.coprocess.LOADED
        TRANSFERRED, // ui.coprocess.TRANSFERRED
        CREATED, // ui.coprocess.CREATED
        STOPPED, // ui.coprocess.STOPPED
      } state; // ui.coprocess.state
    } coprocess; // ui.coprocess

    struct {
      enum {
        SEPARATE_WORLDS, // ui.geometry.SEPARATE_WORLDS
        SAME_WORLD, // ui.geometry.SAME_WORLD
      } mode; // ui.geometry.mode
    } geometry; // ui.geometry

    struct {
      struct {
        size_t current; // ui.timestep.index.current
        size_t previous; // ui.timestep.index.previous
      } index; // ui.timestep.index
    } timestep; // ui.timestep

    struct {
      enum {
        STOPPED, // ui.animation.STOPPED
        PLAYING, // ui.animation.PLAYING
      } mode; // ui.animation.mode

      int fps; // ui.animation.fps

      struct {
        time_point current; // ui.animation.time.current
        time_point previous; // ui.animation.time.previous
      } time; // ui.animation.time
    } animation; // ui.animation
  } ui;

  struct {
    int pid; // coprocess.pid
    FILE *stdin; // coprocess.stdin
    FILE *stdout; // coprocess.stdout
  } coprocess;

  struct {
    size_t count; // timesteps.count
    struct _T;
    std::vector<_T> t; // timesteps.t[i]

    struct _T {
      std::string name; // timesteps.t[i].name

      struct {
        sg::NodePtr node; // timesteps.t[i].world.node
        struct {
          sg::NodePtr node; // timesteps.t[i].world.xfm.node
          struct {
            sg::NodePtr node; // timesteps.t[i].world.xfm.tfn.node
            struct {
              sg::NodePtr node; // timesteps.t[i].world.xfm.tfn.vol.node
            } vol; // timesteps.t[i].world.xfm.tfn.vol
          } tfn; // timesteps.t[i].world.xfm.tfn
        } xfm; // timesteps.t[i].world.xfm
      } world; // timesteps.t[i].world

      struct {
        struct {
          size_t count; // timesteps.t[i].vertex.position.count
          vec3f *data; // timesteps.t[i].vertex.position.data
          vec3f minimum; // timesteps.t[i].vertex.position.minimum
          vec3f maximum; // timesteps.t[i].vertex.position.maximum
        } position; // timesteps.t[i].vertex.position
      } vertex; // timesteps.t[i].vertex

      struct {
        size_t count; // timesteps.t[i].index.count
        uint32_t *data; // timesteps.t[i].index.data
      } index; // timesteps.t[i].index

      struct {
        struct {
          size_t count; // timesteps.t[i].cell.index.count
          uint32_t *data; // timesteps.t[i].cell.index.data
        } index; // timesteps.t[i].cell.index
        struct {
          size_t count; // timesteps.t[i].cell.type.count
          uint8_t *data; // timesteps.t[i].cell.type.data
        } type; // timesteps.t[i].cell.type
        struct {
          size_t count; // timesteps.t[i].cell.data.count
          float *data; // timesteps.t[i].cell.data.data
          float minimum; // timesteps.t[i].cell.data.minimum
          float maximum; // timesteps.t[i].cell.data.maximum
        } data; // timesteps.t[i].cell.data
      } cell; // timesteps.t[i].cell
    };
  } timesteps;
};

}  // namespace mikr_plugin
}  // namespace ospray

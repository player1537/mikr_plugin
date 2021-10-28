// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"
#include "rkcommon/tasking/AsyncTask.h"

#include <condition_variable>

namespace ospray {
namespace mikr_plugin {

// Thanks https://stackoverflow.com/a/36851059
class NotImplemented : public std::logic_error
{
public:
    NotImplemented()
      : std::logic_error("Not Implemented")
    {};
};

struct PanelMikr : public Panel
{
  PanelMikr(std::shared_ptr<StudioContext> context,
            std::string optDefaultFilename);

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
  using Task = rkcommon::tasking::AsyncTask<void>;

  struct {
    struct {
      std::string filename; // ui.coprocess.filename

      struct {
        enum _S {
          INITED, // ui.coprocess.state.INITED
          STARTED, // ui.coprocess.state.STARTED
          STARTED_ACTIVE, // ui.coprocess.state.STARTED_ACTIVE
          LOADED, // ui.coprocess.state.LOADED
          LOADED_ACTIVE, // ui.coprocess.state.LOADED_ACTIVE
          TRANSFERRED, // ui.coprocess.state.TRANSFERRED
          TRANSFERRED_ACTIVE, // ui.coprocess.state.TRANSFERRED_ACTIVE
          CREATED, // ui.coprocess.state.CREATED
          CREATED_ACTIVE, // ui.coprocess.state.CREATED_ACTIVE
          STOPPED, // ui.coprocess.state.STOPPED
          STOPPED_ACTIVE, // ui.coprocess.state.STOPPED_ACTIVE
        };

        enum _S;
        _S current; // ui.coprocess.state.current
        std::atomic<_S> next; // ui.coprocess.state.next
      } state; // ui.coprocess.state

      struct {
        std::unique_ptr<Task> task; // ui.coprocess.started.task
      } started; // ui.coprocess.started

      struct {
        std::unique_ptr<Task> task; // ui.coprocess.loaded.task
        std::atomic<float> percent; // ui.coprocess.loaded.percent
      } loaded; // ui.coprocess.loaded

      struct {
        std::unique_ptr<Task> task; // ui.coprocess.transferred.task
        std::atomic<float> percent; // ui.coprocess.transferred.percent
      } transferred; // ui.coprocess.transferred

      struct {
        std::unique_ptr<Task> task; // ui.coprocess.created.task
        std::atomic<float> percent; // ui.coprocess.created.percent
      } created; // ui.coprocess.created

      struct {
        std::unique_ptr<Task> task; // ui.coprocess.stopped.task
      } stopped; // ui.coprocess.stopped
    } coprocess; // ui.coprocess

    struct {
      enum {
        SEPARATE_WORLDS, // ui.geometry.SEPARATE_WORLDS
        SAME_WORLD, // ui.geometry.SAME_WORLD
      } mode; // ui.geometry.mode
    } geometry; // ui.geometry

    struct {
      enum {
        SEPARATE_TRANSFER_FUNCTIONS, // ui.tfn.SEPARATE_TRANSFER_FUNCTION
        SAME_TRANSFER_FUNCTION, // ui.tfn.SAME_TRANSFER_FUNCTION
      } mode; // ui.tfn.mode
    } tfn; // ui.tfn

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
        bool waitingForFinishedFrame; // ui.animation.time.waitingForFinishedFrame
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

    struct {
      struct {
        float minimum; // timesteps.cell.data.minumum
        float maximum; // timesteps.cell.data.maximum
      } data; // timesteps.cell.data
    } cell; // timesteps.cell

    struct _T;
    std::vector<_T> t; // timesteps.t[i]
    struct _T {
      std::string name; // timesteps.t[i].name

      struct {
        sg::NodePtr node; // timesteps.t[i].world.node
        struct {
          sg::NodePtr node; // timesteps.t[i].world.tfn.node
          struct {
            sg::NodePtr node; // timesteps.t[i].world.tfn.xfm.node
            struct {
              sg::NodePtr node; // timesteps.t[i].world.tfn.xfm.vol.node
            } vol; // timesteps.t[i].world.tfn.xfm.vol
          } xfm; // timesteps.t[i].world.tfn.xfm
        } tfn; // timesteps.t[i].world.tfn
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

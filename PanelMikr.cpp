// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelMikr.h"

#include "sg/visitors/PrintNodes.h"

#include "imgui.h"
#include "imgui_internal.h" // ImGui::PushDisabled, ImGui::PopDisabled

namespace ImGui {
  IMGUI_API void PushEnabled(bool enabled = true) {
    PushDisabled(!enabled);
  }

  IMGUI_API void PopEnabled() {
    PopDisabled();
  }
}

namespace ospray {
namespace mikr_plugin {

#define D(x) std::cerr << #x": " << (x) << std::endl

PanelMikr::PanelMikr(std::shared_ptr<StudioContext> _context)
    : Panel("Mikr Panel", _context)
{
  ui.coprocess.state = ui.coprocess.INITED;
  ui.geometry.mode = ui.geometry.SEPARATE_WORLDS;
  ui.timestep.index.current = 0;
  ui.timestep.index.previous = 0;
  ui.animation.mode = ui.animation.STOPPED;
  ui.animation.fps = 10;
  ui.animation.time.current = clock::now();
  ui.animation.time.previous = ui.animation.time.current;
  coprocess.pid = 0;
  coprocess.stdin = nullptr;
  coprocess.stdout = nullptr;
  timesteps.count = 0;
  timesteps.t.clear();
}

void PanelMikr::buildUI(void *ImGuiCtx)
{
  // Need to set ImGuiContext in *this* address space
  ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
  ImGuiContext &imGuiCtx = *ImGui::GetCurrentContext();
  if (!ImGui::Begin("Mikr Panel", &show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::End();
    return;
  }

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.INITED); {
    {
      bool temp = ui.geometry.mode == ui.geometry.SEPARATE_WORLDS;
      if (ImGui::Checkbox("Use Separate Worlds", &temp)) {
        ui.geometry.mode = temp ? ui.geometry.SEPARATE_WORLDS : ui.geometry.SAME_WORLD;
      }
    }
    if (ImGui::Button("Start Python Co-Process")) {
      startCoProcess();
      ui.coprocess.state = ui.coprocess.STARTED;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.INITED */);

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.STARTED); {
    if (ImGui::Button("Load Data in Co-Process")) {
      loadInCoProcess();
      ui.coprocess.state = ui.coprocess.LOADED;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.STARTED */);

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.LOADED); {
    if (ImGui::Button("Transfer Data from Co-Process")) {
      transferFromCoProcess();
      ui.coprocess.state = ui.coprocess.TRANSFERRED;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.LOADED */);

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.TRANSFERRED); {
    if (ImGui::Button("Create Geometry")) {
      createGeometry();
      ui.coprocess.state = ui.coprocess.CREATED;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.TRANSFERRED */);

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.CREATED); {
    {
      bool temp = ui.animation.mode == ui.animation.PLAYING;
      if (ImGui::Checkbox("Animate", &temp)) {
        ui.animation.mode = temp ? ui.animation.PLAYING : ui.animation.STOPPED;
      }
    }

    ImGui::PushEnabled(ui.animation.mode == ui.animation.STOPPED); {
      { // timestep
        int temp = (int)ui.timestep.index.current;
        int old = temp;
        if (ImGui::Button("<<")) {
          std::fprintf(stderr, "<<\n");
          temp = 0;
        }
        if (ImGui::SameLine(), ImGui::Button("<")) {
          std::fprintf(stderr, "<\n");
          --temp;
        }
        if (ImGui::SameLine(), ImGui::SliderInt("Timestep Index", &temp, 0, timesteps.count - 1, "%d", ImGuiSliderFlags_AlwaysClamp)) {
          std::fprintf(stderr, "=\n");
          // no op
        }
        if (ImGui::SameLine(), ImGui::Button(">")) {
          std::fprintf(stderr, ">\n");
          ++temp;
        }
        if (ImGui::SameLine(), ImGui::Button(">>")) {
          std::fprintf(stderr, ">>\n");
          temp = timesteps.count - 1;
        }

        if (ui.coprocess.state == ui.coprocess.CREATED && ui.animation.mode == ui.animation.PLAYING) {
          ui.animation.time.current = clock::now();
          using namespace std::literals::chrono_literals;

          duration threshold = 1s / (float)ui.animation.fps;
          duration difference = ui.animation.time.current - ui.animation.time.previous;

          if (difference >= threshold) {
            ++temp;
            ui.animation.time.previous = ui.animation.time.current;
          }
        }

        if (timesteps.count != 0 && temp < 0) {
          std::fprintf(stderr, "temp (%d) < 0\n", temp);
          temp += timesteps.count;
          assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        }
        if (timesteps.count != 0 && temp >= timesteps.count) {
          std::fprintf(stderr, "temp (%d) >= timesteps.count (%zu)\n", temp, timesteps.count);
          temp -= timesteps.count;
          assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        }
        assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        if (temp != old) std::fprintf(stderr, "temp = %d, current = %zu\n", temp, ui.timestep.index.current);
        ui.timestep.index.current = (size_t)temp;
      } // timestep index
    } ImGui::PopEnabled(/* ui.animation.mode == ui.animation.STOPPED */);

    { // frames per second
      int temp = (int)ui.animation.fps;
      if (ImGui::Button("<<")) {
        std::fprintf(stderr, "<<\n");
        temp = 1;
      }
      if (ImGui::SameLine(), ImGui::Button("<")) {
        std::fprintf(stderr, "<\n");
        --temp;
      }
      if (ImGui::SameLine(), ImGui::SliderInt("Frames per Second", &temp, 1, 60, "%d FPS", ImGuiSliderFlags_AlwaysClamp)) {
        std::fprintf(stderr, "=\n");
        // no op
      }
      if (ImGui::SameLine(), ImGui::Button(">")) {
        std::fprintf(stderr, ">\n");
        ++temp;
      }
      if (ImGui::SameLine(), ImGui::Button(">>")) {
        std::fprintf(stderr, ">>\n");
        temp = 60;
      }

      if (temp < 1) {
        std::fprintf(stderr, "temp (%d) < 1\n", temp);
        temp = 1;
        assert(1 <= temp && temp <= 60);
      }
      if (temp >= 60) {
        std::fprintf(stderr, "temp (%d) >= 60\n", temp);
        temp = 60;
        assert(1 <= temp && temp <= 60);
      }
      assert(1 <= temp && temp <= 60);
      ui.animation.fps = (int)temp;
    } // frames per second

    if (ui.timestep.index.current != ui.timestep.index.previous) {
      std::fprintf(stderr, "current = %zu, previous = %zu\n", ui.timestep.index.current, ui.timestep.index.previous);

      if (ui.geometry.mode == ui.geometry.SEPARATE_WORLDS) {
        context->frame->add(timesteps.t[ui.timestep.index.current].world.node);
      } else if (ui.geometry.mode == ui.geometry.SAME_WORLD) {
        timesteps.t[ui.timestep.index.previous].world.xfm.tfn.vol.node->child("visible").setValue(false);
        timesteps.t[ui.timestep.index.current].world.xfm.tfn.vol.node->child("visible").setValue(true);
      } else {
        assert(0);
      }

      context->refreshScene(false);
      ui.timestep.index.previous = ui.timestep.index.current;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.CREATED */);

  ImGui::PushEnabled(ui.coprocess.state == ui.coprocess.INITED); {
    if (ImGui::Button("Stop Python Co-Process")) {
      stopCoProcess();
      ui.coprocess.state = ui.coprocess.STOPPED;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state == ui.coprocess.INITED */);

  if (ImGui::Button("Close")) {
    setShown(false);
    ImGui::CloseCurrentPopup();
  }

  ImGui::End();
}

void PanelMikr::startCoProcess() {
  int pid;
  int fds_stdin[2];
  int fds_stdout[2];

  std::fprintf(stderr, "Start\n");

  pipe(fds_stdin);
  pipe(fds_stdout);
  if ((pid = fork()) == 0) {
    // child
    close(0);
    dup2(fds_stdin[0], 0);
    close(fds_stdin[0]);
    close(fds_stdin[1]);

    close(1);
    dup2(fds_stdout[1], 1);
    close(fds_stdout[0]);
    close(fds_stdout[1]);
    execlp("python3",
           "python3",
           "/home/thobson/src/ospray_studio_mikr/studio/plugins/mikr_plugin/plugin_mikr.py",
           NULL);
    perror("execlp");
    exit(0);

  } else {
    // parent
    coprocess.pid = pid;
    close(fds_stdin[0]);
    coprocess.stdin = fdopen(fds_stdin[1], "w");
    close(fds_stdout[1]);
    coprocess.stdout = fdopen(fds_stdout[0], "r");
    //wait(coprocess_pid);
  }
}

void PanelMikr::loadInCoProcess() {
  ssize_t temp;
  size_t i;

  std::fprintf(stderr, "Load\n");

  temp = 0;
  fwrite(&temp, sizeof(temp), 1, coprocess.stdin);
  fflush(coprocess.stdin);

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  assert(temp >= 0);
  timesteps.count = temp;
  timesteps.t.resize(temp);
  for (i=0; i<timesteps.count; ++i) {
    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].name.resize(temp, '\0');
    fread(const_cast<char *>(timesteps.t[i].name.data()), 1, temp, coprocess.stdout);
  }
}

void PanelMikr::transferFromCoProcess() {
  ssize_t temp;
  size_t nread;

  std::fprintf(stderr, "Transfer\n");

  for (size_t i=0; i<timesteps.count; ++i) {
    temp = i;
    fwrite(&temp, sizeof(temp), 1, coprocess.stdin);
    fflush(coprocess.stdin);

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].vertex.position.count = temp;
    
    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].index.count = 8 * temp;
    timesteps.t[i].cell.index.count = temp;
    timesteps.t[i].cell.type.count = temp;
    timesteps.t[i].cell.data.count = temp;

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    D(temp);
    assert(temp >= 0);
    timesteps.t[i].vertex.position.data = (vec3f *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].vertex.position.data, 1, temp, coprocess.stdout);
    D(nread);
    std::fprintf(stderr, "Read vertex.position\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    D(temp);
    assert(temp >= 0);
    timesteps.t[i].index.data = (uint32_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].index.data, 1, temp, coprocess.stdout);
    D(nread);
    std::fprintf(stderr, "Read index\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    D(temp);
    assert(temp >= 0);
    timesteps.t[i].cell.index.data = (uint32_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.index.data, 1, temp, coprocess.stdout);
    D(nread);
    std::fprintf(stderr, "Read cell.index\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    D(temp);
    assert(temp >= 0);
    timesteps.t[i].cell.type.data = (uint8_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.type.data, 1, temp, coprocess.stdout);
    D(nread);
    std::fprintf(stderr, "Read cell.type\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    D(temp);
    assert(temp >= 0);
    timesteps.t[i].cell.data.data = (float *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.data.data, 1, temp, coprocess.stdout);
    D(nread);
    std::fprintf(stderr, "Read cell.data\n");

    timesteps.t[i].vertex.position.minimum = timesteps.t[i].vertex.position.data[0];
    timesteps.t[i].vertex.position.maximum = timesteps.t[i].vertex.position.data[0];
    for (size_t j=0; j<timesteps.t[i].vertex.position.count; ++j) {
      if (timesteps.t[i].vertex.position.data[j].x < timesteps.t[i].vertex.position.minimum.x) {
        timesteps.t[i].vertex.position.minimum.x = timesteps.t[i].vertex.position.data[j].x;
      }
      if (timesteps.t[i].vertex.position.data[j].y < timesteps.t[i].vertex.position.minimum.y) {
        timesteps.t[i].vertex.position.minimum.y = timesteps.t[i].vertex.position.data[j].y;
      }
      if (timesteps.t[i].vertex.position.data[j].z < timesteps.t[i].vertex.position.minimum.z) {
        timesteps.t[i].vertex.position.minimum.z = timesteps.t[i].vertex.position.data[j].z;
      }
      if (timesteps.t[i].vertex.position.data[j].x > timesteps.t[i].vertex.position.maximum.x) {
        timesteps.t[i].vertex.position.maximum.x = timesteps.t[i].vertex.position.data[j].x;
      }
      if (timesteps.t[i].vertex.position.data[j].y > timesteps.t[i].vertex.position.maximum.y) {
        timesteps.t[i].vertex.position.maximum.y = timesteps.t[i].vertex.position.data[j].y;
      }
      if (timesteps.t[i].vertex.position.data[j].z > timesteps.t[i].vertex.position.maximum.z) {
        timesteps.t[i].vertex.position.maximum.z = timesteps.t[i].vertex.position.data[j].z;
      }
    }
    timesteps.t[i].vertex.position.minimum -= vec3f(1.0f);
    timesteps.t[i].vertex.position.maximum += vec3f(1.0f);

    timesteps.t[i].cell.data.minimum = timesteps.t[i].cell.data.data[0];
    timesteps.t[i].cell.data.maximum = timesteps.t[i].cell.data.data[0];
    for (size_t j=0; j<timesteps.t[i].cell.data.count; ++j) {
      if (timesteps.t[i].cell.data.data[j] < timesteps.t[i].cell.data.minimum) {
        timesteps.t[i].cell.data.minimum = timesteps.t[i].cell.data.data[j];
      }
      if (timesteps.t[i].cell.data.data[j] > timesteps.t[i].cell.data.maximum) {
        timesteps.t[i].cell.data.maximum = timesteps.t[i].cell.data.data[j];
      }
    }
    timesteps.t[i].cell.data.minimum -= 1.0f;
    timesteps.t[i].cell.data.maximum += 1.0f;
  }
}

void PanelMikr::createGeometry() {
  std::fprintf(stderr, "Create\n");

  for (size_t i=0; i<timesteps.count; ++i) {
    sg::NodePtr world = nullptr;
    if (ui.geometry.mode == ui.geometry.SAME_WORLD) {
      world = context->frame->childNodeAs<sg::Node>("world");
    } else {
      world = sg::createNode("world", "world");
      if (i == 0) {
        context->frame->add(world);
      }
    }
    timesteps.t[i].world.node = world;

    std::string xfmName(128, '\0');
    std::snprintf(const_cast<char *>(xfmName.data()), 128, "xfm_%s", timesteps.t[i].name.c_str());
    auto &xfm = world->createChild(xfmName, "transform");
    timesteps.t[i].world.xfm.node = xfm.nodeAs<sg::Node>();

    std::string tfnName(128, '\0');
    std::snprintf(const_cast<char *>(tfnName.data()), 128, "tfn_%s", timesteps.t[i].name.c_str());
    auto &tfn = xfm.createChild(tfnName, "transfer_function_viridis");
    timesteps.t[i].world.xfm.tfn.node = tfn.nodeAs<sg::Node>();
    tfn["valueRange"] = vec2f(timesteps.t[i].cell.data.minimum, timesteps.t[i].cell.data.maximum);

    std::string volName(128, '\0');
    std::snprintf(const_cast<char *>(volName.data()), 128, "vol_%s", timesteps.t[i].name.c_str());
    auto &vol = tfn.createChild(volName, "volume_unstructured");
    timesteps.t[i].world.xfm.tfn.vol.node = vol.nodeAs<sg::Node>();
    vol["valueRange"] = range1f(timesteps.t[i].cell.data.minimum, timesteps.t[i].cell.data.maximum);
    vol.child("valueRange").setSGOnly();
    vol.remove("vertex.data");
    vol.createChildData("vertex.position",
                        timesteps.t[i].vertex.position.count,
                        timesteps.t[i].vertex.position.data);
    vol.createChildData("index",
                        timesteps.t[i].index.count,
                        timesteps.t[i].index.data);
    vol.createChildData("cell.index",
                        timesteps.t[i].cell.index.count,
                        timesteps.t[i].cell.index.data);
    vol.createChildData("cell.type",
                        timesteps.t[i].cell.type.count,
                        timesteps.t[i].cell.type.data);
    vol.createChildData("cell.data",
                        timesteps.t[i].cell.data.count,
                        timesteps.t[i].cell.data.data);

    vol.commit();
    tfn.commit();
    xfm.commit();
    world->commit();
  }
  context->frame->traverse<sg::PrintNodes>();

  context->refreshScene(true);
}

void PanelMikr::stopCoProcess() {
  std::fprintf(stderr, "Stop\n");
}

#undef D

}  // namespace mikr_plugin
}  // namespace ospray

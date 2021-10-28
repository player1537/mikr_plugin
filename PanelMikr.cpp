// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelMikr.h"

#include "sg/visitors/PrintNodes.h"

#include "imgui.h"
#include "imgui_internal.h" // ImGui::PushDisabled, ImGui::PopDisabled

#include <thread>

template <>
struct rkcommon::tasking::AsyncTask<void>
  : rkcommon::tasking::AsyncTask<int>
{
  AsyncTask(std::function<void()> fcn)
    : rkcommon::tasking::AsyncTask<int>::AsyncTask([fcn]() {
      fcn();
      return 0;
    })
  {}
};

namespace ImGui {

IMGUI_API void PushEnabled(bool enabled = true) {
  PushDisabled(!enabled);
}

IMGUI_API void PopEnabled() {
  PopDisabled();
}

} /* } namespace ImGui */

namespace ospray {
namespace mikr_plugin {

#define D(x) do { std::fprintf(stderr, "D:%s:%d: %s\n", __FILE__, __LINE__, #x); x; } while (0)

PanelMikr::PanelMikr(
  std::shared_ptr<StudioContext> context,
  std::string optDefaultFilename
)
  : Panel("Mikr Panel", context)
{
  ui.coprocess.filename = optDefaultFilename;
  ui.coprocess.state.next = ui.coprocess.state.INITED;
  ui.coprocess.state.current = ui.coprocess.state.next;
  ui.coprocess.loaded.task = nullptr;
  ui.coprocess.loaded.percent = 0.0f;
  ui.coprocess.transferred.task = nullptr;
  ui.coprocess.transferred.percent = 0.0f;
  ui.coprocess.created.task = nullptr;
  ui.coprocess.created.done = false;
  ui.coprocess.created.percent = 0.0f;
  ui.geometry.mode = ui.geometry.SAME_WORLD;
  ui.tfn.mode = ui.tfn.SAME_TRANSFER_FUNCTION;
  ui.timestep.index.current = 0;
  ui.timestep.index.previous = 0;
  ui.animation.mode = ui.animation.STOPPED;
  ui.animation.fps = 10;
  ui.animation.time.current = clock::now();
  ui.animation.time.previous = ui.animation.time.current;
  ui.animation.time.waitingForFinishedFrame = false;
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
  if (!ImGui::Begin("Mikr Panel###PanelMikr", &show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::End();
    return;
  }

  ui.coprocess.state.current = ui.coprocess.state.next;

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.INITED); {
    {
      std::string temp{ui.coprocess.filename};
      temp.resize(1024, '\0');
      ImGuiInputTextFlags flags{0};
      if (ImGui::InputText("###ui.coprocess.filename", const_cast<char *>(temp.data()), 1024, flags)) {
        ui.coprocess.filename = temp;
      }
    }
    {
      bool temp = ui.geometry.mode == ui.geometry.SEPARATE_WORLDS;
      if (ImGui::Checkbox("Use Separate Worlds###ui.geometry.mode", &temp)) {
        ui.geometry.mode = temp ? ui.geometry.SEPARATE_WORLDS : ui.geometry.SAME_WORLD;
      }
    }
    if (ui.geometry.mode == ui.geometry.SEPARATE_WORLDS) {
      ui.tfn.mode = ui.tfn.SEPARATE_TRANSFER_FUNCTIONS;
    }
    ImGui::PushEnabled(ui.geometry.mode == ui.geometry.SAME_WORLD); {
      {
        bool temp = ui.tfn.mode == ui.tfn.SEPARATE_TRANSFER_FUNCTIONS;
        if (ImGui::Checkbox("Use Separate Transfer Functions###ui.tfn.mode", &temp)) {
          ui.tfn.mode = temp ? ui.tfn.SEPARATE_TRANSFER_FUNCTIONS : ui.tfn.SAME_TRANSFER_FUNCTION;
        }
      }
    } ImGui::PopEnabled(/* ui.geometry.mode == ui.geometry.SAME_WORLD */);
    if (ImGui::Button("Start Python Co-Process###ui.coprocess.started.task")) {
      ui.coprocess.state.next = ui.coprocess.state.STARTED_ACTIVE;
      std::fprintf(stderr, "Before startCoProcess task\n");
      ui.coprocess.started.task = std::make_unique<Task>([this]() {
        std::ostringstream ss;
        ss << std::this_thread::get_id();
        std::fprintf(stderr, "Inside startCoProcess task: thread id = %s\n", ss.str().c_str());
        startCoProcess();
        ui.coprocess.state.next = ui.coprocess.state.STARTED;
        std::fprintf(stderr, "After startCoProcess task\n");
      });
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.INITED */);

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.STARTED); {
    if (ImGui::Button("Load Data in Co-Process###ui.coprocess.loaded.task")) {
      ui.coprocess.state.next = ui.coprocess.state.LOADED_ACTIVE;
      ui.coprocess.loaded.task = std::make_unique<Task>([this]() {
        loadInCoProcess();
        ui.coprocess.state.next = ui.coprocess.state.LOADED;
      });
    }
    {
      float temp{ui.coprocess.loaded.percent};
      ImGui::ProgressBar(temp);
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.STARTED */);

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.LOADED); {
    if (ImGui::Button("Transfer Data from Co-Process###ui.coprocess.transferred.task")) {
      ui.coprocess.state.next = ui.coprocess.state.TRANSFERRED_ACTIVE;
      ui.coprocess.transferred.task = std::make_unique<Task>([this]() {
        transferFromCoProcess();
        ui.coprocess.state.next = ui.coprocess.state.TRANSFERRED;
      });
    }
    {
      float temp{ui.coprocess.transferred.percent};
      ImGui::ProgressBar(temp);
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.LOADED */);

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.TRANSFERRED); {
    if (ImGui::Button("Create Geometry###ui.coprocess.created.task")) {
      ui.coprocess.state.next = ui.coprocess.state.CREATED_ACTIVE;

      {
        D(ui.coprocess.created.render.mutex.lock());
      }
      ui.coprocess.created.task = std::make_unique<Task>([this]() {
        createGeometry();
      });

      {
        D(ui.coprocess.created.render.condition.wait(ui.coprocess.created.render.mutex));
      }
    }
    if (ui.coprocess.state.current == ui.coprocess.state.CREATED_ACTIVE) {
      {
        D(ui.coprocess.created.create.mutex.lock());
        D(ui.coprocess.created.create.condition.notify_one());
        D(ui.coprocess.created.create.mutex.unlock());
      }

      {
        D(ui.coprocess.created.render.condition.wait(ui.coprocess.created.render.mutex));
      }

      if (ui.coprocess.created.done) {
        {
          D(ui.coprocess.created.render.mutex.unlock());
        }

        {
          D(ui.coprocess.created.create.mutex.lock());
          D(ui.coprocess.created.create.condition.notify_one());
          D(ui.coprocess.created.create.mutex.unlock());
        }

        D(ui.coprocess.created.task->wait());
        ui.coprocess.created.task = nullptr;

        ui.coprocess.state.next = ui.coprocess.state.CREATED;
      }
    }
    {
      float temp{ui.coprocess.created.percent};
      ImGui::ProgressBar(temp);
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.TRANSFERRED */);

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.CREATED); {
    {
      bool temp = ui.animation.mode == ui.animation.PLAYING;
      if (ImGui::Checkbox("Animate###ui.animation.mode", &temp)) {
        ui.animation.mode = temp ? ui.animation.PLAYING : ui.animation.STOPPED;
      }
    }

    ImGui::PushEnabled(ui.animation.mode == ui.animation.STOPPED); {
      ImGui::PushID("ui.timestep.index.current"); {
        int temp = (int)ui.timestep.index.current;
        int old = temp;
        if (ImGui::Button("<<")) {
          temp = 0;
        }
        if (ImGui::SameLine(), ImGui::Button("<")) {
          --temp;
        }
        if (ImGui::SameLine(), ImGui::SliderInt("", &temp, 0, timesteps.count - 1, "Timestep %d", ImGuiSliderFlags_AlwaysClamp)) {
          // no op
        }
        if (ImGui::SameLine(), ImGui::Button(">")) {
          ++temp;
        }
        if (ImGui::SameLine(), ImGui::Button(">>")) {
          temp = timesteps.count - 1;
        }

        bool shouldAnimate = 
          ui.coprocess.state.current == ui.coprocess.state.CREATED &&
          ui.animation.mode == ui.animation.PLAYING;

        if (shouldAnimate) {
          ui.animation.time.current = clock::now();
          using namespace std::literals::chrono_literals;

          duration threshold = 1s / (float)ui.animation.fps;
          duration difference = ui.animation.time.current - ui.animation.time.previous;

          if (difference >= threshold) {
            if (!ui.animation.time.waitingForFinishedFrame) {
              ++temp;
              ui.animation.time.waitingForFinishedFrame = true;
            } else if (context->frame->frameIsReady()) {
              ui.animation.time.previous = ui.animation.time.current;
              ui.animation.time.waitingForFinishedFrame = false;
            }
          }
        }

        if (timesteps.count != 0 && temp < 0) {
          temp += timesteps.count;
          assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        }
        if (timesteps.count != 0 && temp >= timesteps.count) {
          temp -= timesteps.count;
          assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        }
        assert(timesteps.count == 0 || (0 <= temp && temp < timesteps.count));
        ui.timestep.index.current = (size_t)temp;
      } ImGui::PopID(/* "ui.timestep.index.current" */);
    } ImGui::PopEnabled(/* ui.animation.mode == ui.animation.STOPPED */);

    ImGui::PushID("ui.animation.fps"); { // frames per second
      int temp = (int)ui.animation.fps;
      if (ImGui::Button("<<")) {
        temp = 1;
      }
      if (ImGui::SameLine(), ImGui::Button("<")) {
        --temp;
      }
      if (ImGui::SameLine(), ImGui::SliderInt("", &temp, 1, 60, "%d FPS", ImGuiSliderFlags_AlwaysClamp)) {
        // no op
      }
      if (ImGui::SameLine(), ImGui::Button(">")) {
        ++temp;
      }
      if (ImGui::SameLine(), ImGui::Button(">>")) {
        temp = 60;
      }

      if (temp < 1) {
        temp = 1;
        assert(1 <= temp && temp <= 60);
      }
      if (temp > 60) {
        temp = 60;
        assert(1 <= temp && temp <= 60);
      }
      assert(1 <= temp && temp <= 60);
      ui.animation.fps = (int)temp;
    } ImGui::PopID(/* "ui.animation.fps" */);

    if (ui.timestep.index.current != ui.timestep.index.previous) {
      if (ui.geometry.mode == ui.geometry.SEPARATE_WORLDS) {
        context->frame->add(timesteps.t[ui.timestep.index.current].world.node, "world");
      } else if (ui.geometry.mode == ui.geometry.SAME_WORLD) {
        timesteps.t[ui.timestep.index.previous].world.tfn.xfm.vol.node->child("visible").setValue(false);
        timesteps.t[ui.timestep.index.current].world.tfn.xfm.vol.node->child("visible").setValue(true);
      } else {
        assert(0);
      }

      context->refreshScene(false);
      ui.timestep.index.previous = ui.timestep.index.current;
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.CREATED */);

  ImGui::PushEnabled(ui.coprocess.state.current == ui.coprocess.state.INITED); {
    if (ImGui::Button("Stop Python Co-Process###ui.coprocess.state.STOPPED")) {
      ui.coprocess.state.next = ui.coprocess.state.STOPPED_ACTIVE;
      ui.coprocess.stopped.task = std::make_unique<Task>([this]() {
        stopCoProcess();
        ui.coprocess.state.next = ui.coprocess.state.STOPPED;
      });
    }
  } ImGui::PopEnabled(/* ui.coprocess.state.current == ui.coprocess.state.INITED */);

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
  if ((pid = fork()) == 0) { // child
    close(0);
    dup2(fds_stdin[0], 0);
    close(fds_stdin[0]);
    close(fds_stdin[1]);

    close(1);
    dup2(fds_stdout[1], 1);
    close(fds_stdout[0]);
    close(fds_stdout[1]);

    const char *argv[16];
    size_t i = 0;
    argv[i++] = "python3";
    argv[i++] = "/home/thobson/src/ospray_studio_mikr/studio/plugins/mikr_plugin/plugin_mikr.py";
    if (!ui.coprocess.filename.empty()) {
      argv[i++] = "--data";
      argv[i++] = ui.coprocess.filename.c_str();
    }
    argv[i++] = NULL;


    execvp(argv[0], const_cast<char *const *>(argv));
    perror("execlp");
    exit(0);

  } else { // parent
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
    ui.coprocess.loaded.percent = (float)i / (float)timesteps.count;
    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].name.resize(temp, '\0');
    fread(const_cast<char *>(timesteps.t[i].name.data()), 1, temp, coprocess.stdout);
  }
  ui.coprocess.loaded.percent = 1.0f;
}

void PanelMikr::transferFromCoProcess() {
  ssize_t temp;
  size_t nread;

  std::fprintf(stderr, "Transfer\n");

  for (size_t i=0; i<timesteps.count; ++i) {
    ui.coprocess.transferred.percent = (float)i / (float)timesteps.count;
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
    assert(temp >= 0);
    timesteps.t[i].vertex.position.data = (vec3f *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].vertex.position.data, 1, temp, coprocess.stdout);
    std::fprintf(stderr, "Read vertex.position\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].index.data = (uint32_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].index.data, 1, temp, coprocess.stdout);
    std::fprintf(stderr, "Read index\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].cell.index.data = (uint32_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.index.data, 1, temp, coprocess.stdout);
    std::fprintf(stderr, "Read cell.index\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].cell.type.data = (uint8_t *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.type.data, 1, temp, coprocess.stdout);
    std::fprintf(stderr, "Read cell.type\n");

    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    timesteps.t[i].cell.data.data = (float *)std::malloc(2 * temp); // XXX(th): Don't malloc 2x buffer size
    nread = fread(timesteps.t[i].cell.data.data, 1, temp, coprocess.stdout);
    std::fprintf(stderr, "Read cell.data\n");

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

  timesteps.cell.data.minimum = timesteps.t[0].cell.data.minimum;
  timesteps.cell.data.maximum = timesteps.t[0].cell.data.maximum;
  for (size_t i=1; i<timesteps.count; ++i) {
    if (timesteps.t[i].cell.data.minimum < timesteps.cell.data.minimum) {
      timesteps.cell.data.minimum = timesteps.t[i].cell.data.minimum;
    }
    if (timesteps.t[i].cell.data.maximum > timesteps.cell.data.maximum) {
      timesteps.cell.data.maximum = timesteps.t[i].cell.data.maximum;
    }
  }

  ui.coprocess.transferred.percent = 1.0f;
}

#define SG_PREFIX(x) ("mikr_" x)
void PanelMikr::createGeometry() {
  std::fprintf(stderr, "Create\n");

  {
    D(ui.coprocess.created.create.mutex.lock());
  }

  context->frame->pauseRendering = true;
  context->frame->cancelFrame();
  context->frame->waitOnFrame();

  {
    D(ui.coprocess.created.render.mutex.lock());
    D(ui.coprocess.created.render.condition.notify_one());
    D(ui.coprocess.created.render.mutex.unlock());
  }

  for (size_t i=0; i<timesteps.count; ++i) {
    {
      D(ui.coprocess.created.create.condition.wait(ui.coprocess.created.create.mutex));
    }

    ui.coprocess.created.percent = (float)i / (float)timesteps.count;

    if (ui.geometry.mode == ui.geometry.SEPARATE_WORLDS) {
      std::string name(128, '\0');
      std::snprintf(const_cast<char *>(name.data()), 128, SG_PREFIX("world_%s"), timesteps.t[i].name.c_str());
      timesteps.t[i].world.node = sg::createNode(name, "world");
    } else if (ui.geometry.mode == ui.geometry.SAME_WORLD) {
      timesteps.t[i].world.node = context->frame->childNodeAs<sg::Node>("world");
    } else {
      throw NotImplemented();
    }

    auto &world = *timesteps.t[i].world.node; {
      if (ui.tfn.mode == ui.tfn.SEPARATE_TRANSFER_FUNCTIONS) {
        std::string name(128, '\0');
        std::snprintf(const_cast<char *>(name.data()), 128, SG_PREFIX("tfn_%s"), timesteps.t[i].name.c_str());
        timesteps.t[i].world.tfn.node = sg::createNode(name, "transfer_function_viridis");
      } else if (ui.tfn.mode == ui.tfn.SAME_TRANSFER_FUNCTION) {
        if (i == 0) {
          timesteps.t[i].world.tfn.node = sg::createNode(SG_PREFIX("tfn"), "transfer_function_viridis");
        } else {
          timesteps.t[i].world.tfn.node = timesteps.t[0].world.tfn.node;
        }
      } else {
        throw NotImplemented();
      }

      auto &tfn = *timesteps.t[i].world.tfn.node; {
        tfn["valueRange"] = vec2f(timesteps.cell.data.minimum, timesteps.cell.data.maximum);

        {
          std::string name(128, '\0');
          std::snprintf(const_cast<char *>(name.data()), 128, SG_PREFIX("xfm_%s"), timesteps.t[i].name.c_str());
          timesteps.t[i].world.tfn.xfm.node = sg::createNode(name, "transform");
        }

        auto &xfm = *timesteps.t[i].world.tfn.xfm.node; {
          {
            std::string name(128, '\0');
            std::snprintf(const_cast<char *>(name.data()), 128, SG_PREFIX("vol_%s"), timesteps.t[i].name.c_str());
            timesteps.t[i].world.tfn.xfm.vol.node = sg::createNode(name, "volume_unstructured");
          }
          auto &vol = *timesteps.t[i].world.tfn.xfm.vol.node; {
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
          } vol.commit();
          xfm.add(vol);
        } xfm.commit();
        tfn.add(xfm);
      } tfn.commit();
      world.add(tfn);
    } world.commit();

    if (i + 1 < timesteps.count) {
      D(ui.coprocess.created.render.mutex.lock());
      D(ui.coprocess.created.render.condition.notify_one());
      D(ui.coprocess.created.render.mutex.unlock());
    }
  }

  if (ui.geometry.mode == ui.geometry.SEPARATE_WORLDS) {
    context->frame->add(timesteps.t[0].world.node, "world");
  } else if (ui.geometry.mode == ui.geometry.SAME_WORLD) {
    timesteps.t[0].world.tfn.xfm.vol.node->child("visible") = true;
    for (size_t i=1; i<timesteps.count; ++i) {
      timesteps.t[i].world.tfn.xfm.vol.node->child("visible") = false;
    }
  } else {
    throw NotImplemented();
  }

  D(context->frame->traverse<sg::PrintNodes>());

  D(context->refreshScene(true));

  std::cerr << std::this_thread::get_id() << std::endl;

  {
    D(ui.coprocess.created.percent = 1.0f);
    D(ui.coprocess.created.done = true);
  }

  {
    D(ui.coprocess.created.render.mutex.lock());
    D(ui.coprocess.created.render.condition.notify_one());
    D(ui.coprocess.created.render.mutex.unlock());
  }

  context->frame->pauseRendering = false;

  {
    D(ui.coprocess.created.create.condition.wait(ui.coprocess.created.create.mutex));
  }

  {
    D(ui.coprocess.created.create.mutex.unlock());
  }


  D(return);
}
#undef SG_PREFIX

void PanelMikr::stopCoProcess() {
  std::fprintf(stderr, "Stop\n");
}

#undef D

}  // namespace mikr_plugin
}  // namespace ospray

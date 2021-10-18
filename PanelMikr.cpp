// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelMikr.h"

#include "sg/visitors/GenerateImGuiWidgets.h"

#include "imgui.h"

namespace ospray {
namespace mikr_plugin {

#define D(x) std::cerr << #x": " << x << std::endl

PanelMikr::PanelMikr(std::shared_ptr<StudioContext> _context)
    : Panel("Mikr Panel", _context)
{}

void PanelMikr::buildUI(void *ImGuiCtx)
{
  // Need to set ImGuiContext in *this* address space
  ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
  ImGui::OpenPopup("Mikr Panel");

  if (ImGui::BeginPopupModal(
          "Mikr Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (ImGui::Button("Start Python Co-Process")) {
      startCoProcess();
    }

    if (ImGui::Button("Load Data in Co-Process")) {
      loadInCoProcess();
    }

    if (ImGui::Button("Transfer Data from Co-Process")) {
      transferFromCoProcess();
    }

    if (ImGui::Button("Create Geometry")) {
      createGeometry();
    }

    if (ImGui::Button("Stop Python Co-Process")) {
      stopCoProcess();
    }

    if (ImGui::Button("Close")) {
      setShown(false);
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
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
  for (i=0; i<timesteps.count; ++i) {
    fread(&temp, sizeof(temp), 1, coprocess.stdout);
    assert(temp >= 0);
    std::string &name = (timesteps.names.emplace_back(temp, '\0'), timesteps.names.back());
    fread(const_cast<char *>(name.data()), 1, temp, coprocess.stdout);
  }
}

void PanelMikr::transferFromCoProcess() {
  ssize_t temp;
  size_t nread;

  std::fprintf(stderr, "Transfer\n");

  timesteps.selected = 0;
  temp = timesteps.selected;
  fwrite(&temp, sizeof(temp), 1, coprocess.stdin);
  fflush(coprocess.stdin);

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  assert(temp >= 0);
  vertex.position.count = temp;
  vertex.position.width = 1;
  
  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  assert(temp >= 0);
  index.count = 8 * temp;
  index.width = 1;
  cell.index.count = temp;
  cell.index.width = 1;
  cell.type.count = temp;
  cell.type.width = 1;
  cell.data.count = temp;
  cell.data.width = 1;

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  D(temp);
  assert(temp >= 0);
  vertex.position.data = (vec3f *)std::malloc(temp);
  nread = fread(vertex.position.data, 1, temp, coprocess.stdout);
  D(nread);
  std::fprintf(stderr, "Read vertex.position\n");

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  D(temp);
  assert(temp >= 0);
  index.data = (uint32_t *)std::malloc(temp);
  nread = fread(index.data, 1, temp, coprocess.stdout);
  D(nread);
  std::fprintf(stderr, "Read index\n");

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  D(temp);
  assert(temp >= 0);
  cell.index.data = (uint32_t *)std::malloc(temp);
  nread = fread(cell.index.data, 1, temp, coprocess.stdout);
  D(nread);
  std::fprintf(stderr, "Read cell.index\n");

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  D(temp);
  assert(temp >= 0);
  cell.type.data = (uint8_t *)std::malloc(temp);
  nread = fread(cell.type.data, 1, temp, coprocess.stdout);
  D(nread);
  std::fprintf(stderr, "Read cell.type\n");

  fread(&temp, sizeof(temp), 1, coprocess.stdout);
  D(temp);
  assert(temp >= 0);
  cell.data.data = (float *)std::malloc(temp);
  nread = fread(cell.data.data, 1, temp, coprocess.stdout);
  D(nread);
  std::fprintf(stderr, "Read cell.data\n");
}

void PanelMikr::createGeometry() {
  std::fprintf(stderr, "Create\n");

  auto &xfm = context->frame->createChild("xfm", "transform");
  auto &vol = xfm.createChild("mikr", "volume_unstructured");
  D(vertex.position.count);
  D(vertex.position.width);
  D(index.count);
  D(index.width);
  D(cell.index.count);
  D(cell.index.width);
  D(cell.type.count);
  D(cell.type.width);
  D(cell.data.count);
  D(cell.data.width);
  vol.remove("vertex.data");
  vol.createChildData("vertex.position",
                      vec2ul(vertex.position.count, vertex.position.width),
                      vertex.position.data);
  vol.createChildData("index",
                      vec2ul(index.count, index.width),
                      index.data);
  vol.createChildData("cell.index",
                      vec2ul(cell.index.count, cell.index.width),
                      cell.index.data);
  vol.createChildData("cell.type",
                      vec2ul(cell.type.count, cell.type.width),
                      cell.type.data);
  vol.createChildData("cell.data",
                      vec2ul(cell.data.count, cell.data.width),
                      cell.data.data);
}

void PanelMikr::stopCoProcess() {
  std::fprintf(stderr, "Stop\n");
}

#undef D

}  // namespace mikr_plugin
}  // namespace ospray
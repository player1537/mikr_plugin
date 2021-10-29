// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "imgui.h"
#include "imgui_internal.h" // ImGui::PushDisabled, ImGui::PopDisabled

namespace ImGui {

IMGUI_API void PushEnabled(bool enabled = true) {
  PushDisabled(!enabled);
}

IMGUI_API void PopEnabled() {
  PopDisabled();
}

} /* } namespace ImGui */
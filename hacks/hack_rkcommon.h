// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "rkcommon/tasking/AsyncTask.h"

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
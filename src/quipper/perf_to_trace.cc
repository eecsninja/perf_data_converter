// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>

#include <string>

#include "base/logging.h"
#include "chrome_trace_builder.h"
#include "perf_protobuf_io.h"

namespace {

DEFINE_string(render_mode, "flame", "Rendering mode: flame|cascade|command");

}  // anonymous namespace

namespace quipper {

// Converts the "--render_mode" flag arg to an enum.
ChromeTraceBuilder::RenderMode GetRenderModeFromString(const std::string& arg) {
  if (arg == "flame") {
    return ChromeTraceBuilder::RenderMode::FLAME;
  }
  if (arg == "cascade") {
    return ChromeTraceBuilder::RenderMode::CASCADE;
  }
  if (arg == "command") {
    return ChromeTraceBuilder::RenderMode::COMMAND;
  }
  return ChromeTraceBuilder::RenderMode::FLAME;
}

}  // namespace quipper

int main(int argc, char** argv) {
  if (argc < 2) {
    LOG(ERROR) << "Must specify args: " << argv[0] << " [input perf.data]";
    return 0;
  }

  quipper::PerfParserOptions options;
  options.sort_events_by_time = true;

  quipper::PerfDataProto proto;
  if (!quipper::SerializeFromFileWithOptions(argv[1], options, &proto)) {
    LOG(ERROR) << "Could not convert perf data file " << argv[1] << " to "
               << "PerfDataProto.";
    return 1;
  }

  quipper::ChromeTraceBuilder builder;
  builder.IngestPerfData(proto);

  auto render_mode = GetRenderModeFromString(FLAGS_render_mode)
  std::string json_string = builder.RenderJson(render_mode).toStyledString();

  printf("%s", json_string.c_str());

  return 0;
}

// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>

#include <string>

#include "base/logging.h"
#include "chrome_trace_builder.h"
#include "perf_protobuf_io.h"

namespace {

DEFINE_string(input, "", "Name of perf data file to read");
DEFINE_string(render_mode, "flat",
              "Rendering mode: flat|flame|cascade|command");

}  // anonymous namespace

namespace quipper {

// Converts the "--render_mode" flag arg to an enum.
ChromeTraceBuilder::RenderMode GetRenderModeFromString(const std::string& arg) {
  if (arg == "flat") {
    return ChromeTraceBuilder::RenderMode::FLAT;
  }
  if (arg == "flame") {
    return ChromeTraceBuilder::RenderMode::FLAME;
  }
  if (arg == "cascade") {
    return ChromeTraceBuilder::RenderMode::CASCADE;
  }
  if (arg == "command") {
    return ChromeTraceBuilder::RenderMode::COMMAND;
  }
  return ChromeTraceBuilder::RenderMode::FLAT;
}

}  // namespace quipper

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_input == "") {
    LOG(ERROR) << "Must specify input file.";
    return 1;
  }

  quipper::PerfParserOptions options;
  options.sort_events_by_time = true;

  quipper::PerfDataProto proto;
  if (!quipper::SerializeFromFileWithOptions(FLAGS_input, options, &proto)) {
    LOG(ERROR) << "Could not convert perf data file " << FLAGS_input << " to "
               << "PerfDataProto.";
    return 1;
  }

  quipper::ChromeTraceBuilder builder;
  builder.IngestPerfData(proto);

  auto render_mode = quipper::GetRenderModeFromString(FLAGS_render_mode);
  std::string json_string = builder.RenderJson(render_mode).toStyledString();

  printf("%s", json_string.c_str());

  return 0;
}

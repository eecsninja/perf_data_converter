// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_DATA_CONVERTER_SRC_QUIPPER_CHROME_TRACE_BUILDER_H_
#define PERF_DATA_CONVERTER_SRC_QUIPPER_CHROME_TRACE_BUILDER_H_

#include <jsoncpp/json/value.h>
#include <stdint.h>

#include <map>
#include <set>
#include <stack>
#include <string>
#include <utility>

#include "compat/proto.h"

namespace quipper {

class ChromeTraceBuilder {
 public:
  enum class RenderMode {
    // All processes are rendered on the same row, as a flamegraph where they
    // overlap.
    FLAT,
    // Flamegraph with parallel processes on multiple row.
    FLAME,
    // Each process is rendered on its own row.
    CASCADE,
    // All processes with the same command are rendered on the same row.
    COMMAND,
  };

  void IngestPerfData(const PerfDataProto& proto);

  Json::Value RenderJson(RenderMode mode) const;

 private:
  using PidTid = std::pair<uint32_t, uint32_t>;

  // Contains info about a process provided by corresponding COMM/FORK/EXIT
  // events.
  struct ProcessInfo {
    std::string name;
    PidTid pidtid;
    ProcessInfo* parent = nullptr;
    uint64_t start_time_ns = 0;
    uint64_t end_time_ns = 0;
    std::set<const ProcessInfo*> children;

    uint64_t render_row = 0;
  };

  // For handling different events within the perf data.
  void ProcessCommEvent(const PerfDataProto_CommEvent& comm);

  void ProcessForkEvent(const PerfDataProto_ForkEvent& fork) {
    ProcessForkOrExitEvent(fork, /* is_exit */ false);
  }
  void ProcessExitEvent(const PerfDataProto_ForkEvent& exit) {
    ProcessForkOrExitEvent(exit, /* is_exit */ true);
  }
  void ProcessForkOrExitEvent(const PerfDataProto_ForkEvent& fork,
                              bool is_exit);

  // Gets existing ProcessInfo for |pidtid| or creates a new one if none exists.
  ProcessInfo* GetOrCreateProcessInfo(const PidTid& pidtid);

  // Gets the ID for a command, or -1 if not available.
  int64_t GetCommandID(const std::string& command_name) const;

  // Gets the ID for displaying a process in Chrome trace JSON.
  Json::Value::Int GetRenderID(const ProcessInfo& info, RenderMode mode) const;

  // Stores a ProcessInfo object for each pid/tid.
  std::map<PidTid, ProcessInfo> pid_to_info_;

  // Stores an ID for each command name.
  std::map<std::string, uint32_t> command_to_id_;

  std::map<uint64_t, std::stack<const ProcessInfo*>> row_to_call_stacks_;
};

}  // namespace quipper

#endif  // PERF_DATA_CONVERTER_SRC_QUIPPER_CHROME_TRACE_BUILDER_H_

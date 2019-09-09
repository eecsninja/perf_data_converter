// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_trace_builder.h"

#include "base/logging.h"
#include "kernel/perf_event.h"

namespace quipper {

void ChromeTraceBuilder::IngestPerfData(const PerfDataProto& proto) {
  for (const auto& event : proto.events()) {
    auto event_type = event.header().type();
    switch (event_type) {
      case PERF_RECORD_COMM:
        ProcessCommEvent(event.comm_event());
        break;
      case PERF_RECORD_FORK:
        ProcessForkEvent(event.fork_event());
        break;
      case PERF_RECORD_EXIT:
        ProcessExitEvent(event.exit_event());
        break;
      default:
        VLOG(1) << "Skipping event type " << event_type;
        break;
    }
  }
}

Json::Value ChromeTraceBuilder::ToJsonValue() const {
  Json::Value result(Json::arrayValue);

  for (const auto& pid_to_info : pid_to_info_) {
    const auto& pidtid = pid_to_info.first;
    const auto& process_info = pid_to_info.second;

    if (process_info.start_time_ns == 0) {
      LOG(ERROR) << "Missing start timestamp for " << process_info.name;
      continue;
    }
    if (process_info.end_time_ns == 0) {
      LOG(ERROR) << "Missing end timestamp for " << process_info.name;
      continue;
    }

    Json::Value object(Json::objectValue);
    object["ph"] = "X";  // For complete events.
    object["name"] = process_info.name;
    object["pid"] = pidtid.first;
    object["tid"] = pidtid.second;
    // Times are in microseconds.
    object["ts"] =
        static_cast<Json::Value::UInt64>(process_info.start_time_ns / 1000);
    object["dur"] = static_cast<Json::Value::UInt64>(
        (process_info.end_time_ns - process_info.start_time_ns) / 1000);

    result.append(object);
  }

  return result;
}

void ChromeTraceBuilder::ProcessCommEvent(const PerfDataProto_CommEvent& comm) {
  auto pidtid = std::make_pair(comm.pid(), comm.tid());
  const auto& command_name = comm.comm();

  // TODO(sque): What if pid/tid wraps around and overwrites entries from
  // previous process with same pid/tid?
  auto process_info = GetOrCreateProcessInfo(pidtid);
  process_info->name = command_name;

  // For the first process, there might not be a FORK event. In that case, use
  // the COMM event to provide the start time. But if there is a FORK event that
  // appears later, it should overwrite this value.
  if (process_info->start_time_ns == 0) {
    process_info->start_time_ns = comm.sample_info().sample_time_ns();
  }
}

void ChromeTraceBuilder::ProcessForkOrExitEvent(
    const PerfDataProto_ForkEvent& fork, bool is_exit) {
  auto pidtid = std::make_pair(fork.pid(), fork.tid());
  uint64_t time_ns = fork.fork_time_ns();
  auto process_info = GetOrCreateProcessInfo(pidtid);

  if (!is_exit) {
    process_info->start_time_ns = time_ns;
  } else {  // is_exit
    process_info->end_time_ns = time_ns;
  }
}

ChromeTraceBuilder::ProcessInfo* ChromeTraceBuilder::GetOrCreateProcessInfo(
    const PidTid& pidtid) {
  auto iter = pid_to_info_.find(pidtid);
  if (iter != pid_to_info_.end()) {
    return &iter->second;
  }
  auto insert_result =
      pid_to_info_.insert(std::make_pair(pidtid, ProcessInfo()));
  return &insert_result.first->second;
}

}  // namespace quipper

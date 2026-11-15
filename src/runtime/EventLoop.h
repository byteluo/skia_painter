#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "v8.h"

namespace canvas_engine {

class EventLoop {
 public:
  EventLoop(v8::Isolate* isolate, bool realtime_clock,
            std::chrono::steady_clock::time_point start_time);

  bool DrainPendingTasks(
      v8::Local<v8::Context> context, std::string* last_error,
      const std::function<std::string(v8::TryCatch*)>& format_exception);
  std::uint32_t EnqueueTask(
      v8::Local<v8::Function> callback, bool animation_frame,
      double ready_at_ms,
      const std::vector<v8::Local<v8::Value>>& arguments = {});
  std::uint32_t EnqueueNativeTask(std::function<void()> callback,
                                  double ready_at_ms);
  double NowMs() const;
  double NextFrameMs() const { return next_frame_ms_; }
  void RecordDrawOp() { ++draw_op_count_; }
  void CancelTask(std::uint32_t task_id);

 private:
  struct PendingTask {
    std::uint32_t id = 0;
    bool animation_frame = false;
    bool cancelled = false;
    double ready_at_ms = 0.0;
    v8::Global<v8::Function> callback;
    std::vector<v8::Global<v8::Value>> arguments;
    std::function<void()> native;
  };

  v8::Isolate* isolate_;
  bool realtime_clock_;
  std::chrono::steady_clock::time_point engine_start_time_;
  std::uint32_t next_task_id_ = 1;
  double clock_offset_ms_ = 0.0;
  std::uint64_t draw_op_count_ = 0;
  std::size_t consecutive_idle_frame_batches_ = 0;
  double next_frame_ms_ = 0.0;
  bool draining_tasks_ = false;
  std::vector<PendingTask> pending_tasks_;
};

}  // namespace canvas_engine

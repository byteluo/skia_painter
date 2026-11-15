#include "EventLoop.h"

#include <algorithm>
#include <iostream>
#include <utility>

namespace canvas_engine {

EventLoop::EventLoop(v8::Isolate* isolate, bool realtime_clock,
                     std::chrono::steady_clock::time_point start_time)
    : isolate_(isolate),
      realtime_clock_(realtime_clock),
      engine_start_time_(start_time) {}

bool EventLoop::DrainPendingTasks(
    v8::Local<v8::Context> context, std::string* last_error,
    const std::function<std::string(v8::TryCatch*)>& format_exception) {
  if (draining_tasks_) {
    return true;
  }

  v8::Context::Scope context_scope(context);
  draining_tasks_ = true;

  constexpr std::size_t kMaxExecutedTasks = 100000;
  constexpr double kMaxVirtualAdvanceMs = 10000.0;
  constexpr double kFrameDurationMs = 1000.0 / 60.0;
  double virtual_advance_ms = 0.0;
  std::size_t executed_tasks = 0;
  consecutive_idle_frame_batches_ = 0;

  while (true) {
    pending_tasks_.erase(
        std::remove_if(pending_tasks_.begin(), pending_tasks_.end(),
                       [](const PendingTask& task) { return task.cancelled; }),
        pending_tasks_.end());
    if (pending_tasks_.empty()) {
      draining_tasks_ = false;
      return true;
    }

    const auto earliest_task =
        std::min_element(pending_tasks_.begin(), pending_tasks_.end(),
                         [](const PendingTask& lhs, const PendingTask& rhs) {
                           if (lhs.ready_at_ms != rhs.ready_at_ms) {
                             return lhs.ready_at_ms < rhs.ready_at_ms;
                           }
                           return lhs.id < rhs.id;
                         });
    const double now = NowMs();
    if (earliest_task->ready_at_ms > now) {
      const double advance_ms = earliest_task->ready_at_ms - now;
      const double remaining_advance_ms =
          kMaxVirtualAdvanceMs - virtual_advance_ms;
      if (advance_ms > remaining_advance_ms) {
        const std::size_t dropped_tasks = pending_tasks_.size();
        pending_tasks_.clear();
        std::cerr << "[canvas_engine] DrainPendingTasks hit the "
                  << kMaxVirtualAdvanceMs << "ms virtual advance cap, "
                  << dropped_tasks << " task(s) dropped" << std::endl;
        draining_tasks_ = false;
        return true;
      }
      clock_offset_ms_ += advance_ms;
      virtual_advance_ms += advance_ms;
    }

    std::vector<PendingTask> ready_tasks;
    std::vector<PendingTask> deferred_tasks;
    const double batch_time_ms = NowMs();
    for (auto& task : pending_tasks_) {
      if (task.ready_at_ms <= batch_time_ms) {
        ready_tasks.push_back(std::move(task));
      } else {
        deferred_tasks.push_back(std::move(task));
      }
    }
    pending_tasks_ = std::move(deferred_tasks);
    std::sort(ready_tasks.begin(), ready_tasks.end(),
              [](const PendingTask& lhs, const PendingTask& rhs) {
                if (lhs.ready_at_ms != rhs.ready_at_ms) {
                  return lhs.ready_at_ms < rhs.ready_at_ms;
                }
                return lhs.id < rhs.id;
              });

    const bool advances_frame =
        std::any_of(ready_tasks.begin(), ready_tasks.end(),
                    [](const PendingTask& task) {
                      return task.animation_frame && !task.cancelled;
                    });
    const std::uint64_t draw_ops_before_batch = draw_op_count_;
    if (advances_frame) {
      next_frame_ms_ = batch_time_ms + kFrameDurationMs;
    }

    for (std::size_t index = 0; index < ready_tasks.size(); ++index) {
      auto& task = ready_tasks[index];
      if (task.cancelled ||
          (task.callback.IsEmpty() && !task.native)) {
        continue;
      }
      if (executed_tasks >= kMaxExecutedTasks) {
        const std::size_t dropped_tasks =
            ready_tasks.size() - index + pending_tasks_.size();
        pending_tasks_.clear();
        std::cerr << "[canvas_engine] DrainPendingTasks hit the "
                  << kMaxExecutedTasks << " executed-task cap, "
                  << dropped_tasks << " task(s) dropped" << std::endl;
        draining_tasks_ = false;
        return true;
      }

      ++executed_tasks;
      if (task.native) {
        task.native();
        if (!last_error->empty()) {
          draining_tasks_ = false;
          pending_tasks_.clear();
          return false;
        }
        continue;
      }

      v8::TryCatch try_catch(isolate_);
      v8::Local<v8::Function> callback = task.callback.Get(isolate_);
      std::vector<v8::Local<v8::Value>> argv;
      if (task.animation_frame) {
        argv.push_back(v8::Number::New(isolate_, batch_time_ms));
      } else {
        argv.reserve(task.arguments.size());
        for (const auto& argument : task.arguments) {
          argv.push_back(argument.Get(isolate_));
        }
      }
      v8::Local<v8::Value> result;
      if (!callback
               ->Call(context, context->Global(),
                      static_cast<int>(argv.size()),
                      argv.empty() ? nullptr : argv.data())
               .ToLocal(&result)) {
        *last_error = format_exception(&try_catch);
        draining_tasks_ = false;
        pending_tasks_.clear();
        return false;
      }
    }

    const auto is_frame_task = [](const PendingTask& task) {
      return task.cancelled || task.animation_frame;
    };
    const bool only_frame_tasks_pending =
        std::all_of(ready_tasks.begin(), ready_tasks.end(), is_frame_task) &&
        std::all_of(pending_tasks_.begin(), pending_tasks_.end(),
                    is_frame_task);
    if (advances_frame && only_frame_tasks_pending &&
        draw_op_count_ == draw_ops_before_batch) {
      if (++consecutive_idle_frame_batches_ >= 60) {
        pending_tasks_.clear();
        draining_tasks_ = false;
        return true;
      }
    } else {
      consecutive_idle_frame_batches_ = 0;
    }
  }
}

std::uint32_t EventLoop::EnqueueTask(
    v8::Local<v8::Function> callback, bool animation_frame,
    double ready_at_ms,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  PendingTask task;
  task.id = next_task_id_++;
  task.animation_frame = animation_frame;
  task.ready_at_ms = ready_at_ms;
  task.callback.Reset(isolate_, callback);
  task.arguments.reserve(arguments.size());
  for (auto argument : arguments) {
    task.arguments.emplace_back(isolate_, argument);
  }
  pending_tasks_.push_back(std::move(task));
  return pending_tasks_.back().id;
}

std::uint32_t EventLoop::EnqueueNativeTask(std::function<void()> callback,
                                          double ready_at_ms) {
  PendingTask task;
  task.id = next_task_id_++;
  task.ready_at_ms = ready_at_ms;
  task.native = std::move(callback);
  pending_tasks_.push_back(std::move(task));
  return pending_tasks_.back().id;
}

double EventLoop::NowMs() const {
  if (!realtime_clock_) {
    return clock_offset_ms_;
  }
  const auto elapsed = std::chrono::steady_clock::now() - engine_start_time_;
  return std::chrono::duration<double, std::milli>(elapsed).count() +
         clock_offset_ms_;
}

void EventLoop::CancelTask(std::uint32_t task_id) {
  for (auto& task : pending_tasks_) {
    if (task.id == task_id) {
      task.cancelled = true;
      task.callback.Reset();
      task.arguments.clear();
      task.native = {};
      return;
    }
  }
}

}  // namespace canvas_engine

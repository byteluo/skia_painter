#pragma once

#include <type_traits>

#include <v8.h>

namespace canvas_engine::V8Compat {

template <typename Callback>
struct SetterCallbackTraits;

template <typename ReturnType>
struct SetterCallbackTraits<void (*)(
    v8::Local<v8::Name>, v8::Local<v8::Value>,
    const v8::PropertyCallbackInfo<ReturnType>&)> {
  using ReturnValue = ReturnType;
};

using SetterReturnValue =
    typename SetterCallbackTraits<v8::AccessorNameSetterCallbackV2>::ReturnValue;
using SetterInfo = v8::PropertyCallbackInfo<SetterReturnValue>;

template <typename Info>
auto Holder(const Info& info, int) -> decltype(info.HolderV2()) {
  return info.HolderV2();
}

template <typename Info>
auto Holder(const Info& info, long) -> decltype(info.Holder()) {
  return info.Holder();
}

template <typename Info>
auto Holder(const Info& info) -> decltype(Holder(info, 0)) {
  return Holder(info, 0);
}

inline void SetSetterSuccess(const SetterInfo& info) {
  if constexpr (std::is_same_v<SetterReturnValue, v8::Boolean>) {
    info.GetReturnValue().Set(true);
  }
}

}  // namespace canvas_engine::V8Compat

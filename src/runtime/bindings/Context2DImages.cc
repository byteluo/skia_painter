#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "BindingSupport.h"
#include "../EventLoop.h"
#include "../HandleRegistry.h"
#include "canvas_engine/runtime/V8Platform.h"
#include "include/effects/SkGradient.h"

namespace canvas_engine {

using namespace binding_internal;

void ScriptEngineImpl::CtxDrawImage(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle || info.Length() < 3 || !info[0]->IsObject()) {
    ThrowTypeError(info.GetIsolate(),
                   "drawImage(image, ...) requires an Image or Canvas source");
    return;
  }

  auto source = info[0].As<v8::Object>();
  sk_sp<SkImage> image;
  float source_width = 0.0f;
  float source_height = 0.0f;
  float source_pixel_ratio = 1.0f;
  if (auto* image_handle = UnwrapHandle<ImageHandle>(
          info.GetIsolate(), source, kImageHandleTag)) {
    if (image_handle->asset) {
      image = image_handle->asset->image();
      source_width = static_cast<float>(image_handle->asset->width());
      source_height = static_cast<float>(image_handle->asset->height());
    }
  } else if (auto* canvas_handle = UnwrapHandle<CanvasHandle>(
                 info.GetIsolate(), source, kCanvasHandleTag)) {
    image = canvas_handle->surface->MakeImageSnapshot();
    source_width = static_cast<float>(canvas_handle->surface->width());
    source_height = static_cast<float>(canvas_handle->surface->height());
    source_pixel_ratio = canvas_handle->surface->pixel_ratio();
  }

  if (!image || !(source_width > 0.0f) || !(source_height > 0.0f)) {
    return;
  }

  double dx = 0.0;
  double dy = 0.0;
  double dw = 0.0;
  double dh = 0.0;
  double sx = 0.0;
  double sy = 0.0;
  double sw = source_width;
  double sh = source_height;

  if (info.Length() == 3) {
    if (!ExtractDouble(info, 1, &dx) || !ExtractDouble(info, 2, &dy)) {
      ThrowTypeError(info.GetIsolate(),
                     "drawImage(image, dx, dy) requires numeric coordinates");
      return;
    }
    dw = source_width;
    dh = source_height;
  } else if (info.Length() == 5) {
    if (!ExtractDouble(info, 1, &dx) || !ExtractDouble(info, 2, &dy) ||
        !ExtractDouble(info, 3, &dw) || !ExtractDouble(info, 4, &dh)) {
      ThrowTypeError(info.GetIsolate(),
                     "drawImage(image, dx, dy, dw, dh) requires numbers");
      return;
    }
  } else if (info.Length() >= 9) {
    if (!ExtractDouble(info, 1, &sx) || !ExtractDouble(info, 2, &sy) ||
        !ExtractDouble(info, 3, &sw) || !ExtractDouble(info, 4, &sh) ||
        !ExtractDouble(info, 5, &dx) || !ExtractDouble(info, 6, &dy) ||
        !ExtractDouble(info, 7, &dw) || !ExtractDouble(info, 8, &dh)) {
      ThrowTypeError(
          info.GetIsolate(),
          "drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh) requires numbers");
      return;
    }
  } else {
    ThrowTypeError(
        info.GetIsolate(),
        "drawImage supports 3, 5, or 9 arguments after the image source");
    return;
  }

  handle->context->DrawImage(image,
                             static_cast<float>(sx * source_pixel_ratio),
                             static_cast<float>(sy * source_pixel_ratio),
                             static_cast<float>(sw * source_pixel_ratio),
                             static_cast<float>(sh * source_pixel_ratio),
                             static_cast<float>(dx), static_cast<float>(dy),
                             static_cast<float>(dw), static_cast<float>(dh));
}

void ScriptEngineImpl::CtxSetLineDash(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle || info.Length() < 1 || !info[0]->IsArray()) {
    ThrowTypeError(info.GetIsolate(), "setLineDash(segments) requires an array");
    return;
  }

  auto context = info.GetIsolate()->GetCurrentContext();
  auto array = info[0].As<v8::Array>();
  std::vector<float> segments;
  segments.reserve(array->Length());
  for (uint32_t i = 0; i < array->Length(); ++i) {
    v8::Local<v8::Value> value;
    if (!array->Get(context, i).ToLocal(&value)) {
      ThrowTypeError(info.GetIsolate(), "setLineDash segments must be numeric");
      return;
    }
    float segment = 0.0f;
    if (!ExtractFloat(info.GetIsolate(), value, &segment)) {
      ThrowTypeError(info.GetIsolate(), "setLineDash segments must be numeric");
      return;
    }
    segments.push_back(segment);
  }

  handle->context->SetLineDash(segments);
}

void ScriptEngineImpl::CtxGetLineDash(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle) {
    return;
  }

  const auto& line_dash = handle->context->line_dash();
  auto array =
      v8::Array::New(info.GetIsolate(), static_cast<int>(line_dash.size()));
  auto context = info.GetIsolate()->GetCurrentContext();
  for (uint32_t i = 0; i < line_dash.size(); ++i) {
    array->Set(context, i, v8::Number::New(info.GetIsolate(), line_dash[i]))
        .Check();
  }
  info.GetReturnValue().Set(array);
}

void ScriptEngineImpl::CtxGetImageData(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &width) || !ExtractDouble(info, 3, &height)) {
    ThrowTypeError(info.GetIsolate(),
                   "getImageData(x, y, width, height) requires four numbers");
    return;
  }

  const auto image_data = handle->context->GetImageData(
      static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
      static_cast<int>(std::lround(width)), static_cast<int>(std::lround(height)));
  if (!image_data.has_value()) {
    ThrowError(info.GetIsolate(), "failed to read image data");
    return;
  }

  auto backing_store =
      v8::ArrayBuffer::NewBackingStore(info.GetIsolate(),
                                       image_data->data.size());
  std::memcpy(backing_store->Data(), image_data->data.data(), image_data->data.size());
  auto array_buffer = v8::ArrayBuffer::New(info.GetIsolate(), std::move(backing_store));
  auto data = v8::Uint8ClampedArray::New(array_buffer, 0,
                                         image_data->data.size());

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = v8::Object::New(info.GetIsolate());
  object
      ->Set(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "width"),
            v8::Integer::New(info.GetIsolate(), image_data->width))
      .Check();
  object
      ->Set(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "height"),
            v8::Integer::New(info.GetIsolate(), image_data->height))
      .Check();
  object
      ->Set(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "data"), data)
      .Check();
  info.GetReturnValue().Set(object);
}

void ScriptEngineImpl::CtxPutImageData(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle || info.Length() < 3 || !info[0]->IsObject()) {
    ThrowTypeError(info.GetIsolate(),
                   "putImageData(imageData, dx, dy) requires imageData and coordinates");
    return;
  }

  auto context = info.GetIsolate()->GetCurrentContext();
  auto image_data_object = info[0].As<v8::Object>();
  v8::Local<v8::Value> width_value;
  v8::Local<v8::Value> height_value;
  v8::Local<v8::Value> data_value;
  if (!image_data_object
           ->Get(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "width"))
           .ToLocal(&width_value) ||
      !image_data_object
           ->Get(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "height"))
           .ToLocal(&height_value) ||
      !image_data_object
           ->Get(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "data"))
           .ToLocal(&data_value)) {
    ThrowTypeError(info.GetIsolate(), "putImageData requires a valid imageData object");
    return;
  }

  double width = 0.0;
  double height = 0.0;
  double dx = 0.0;
  double dy = 0.0;
  if (!ExtractDouble(info.GetIsolate(), width_value, &width) ||
      !ExtractDouble(info.GetIsolate(), height_value, &height) ||
      !ExtractDouble(info, 1, &dx) || !ExtractDouble(info, 2, &dy) ||
      !data_value->IsArrayBufferView()) {
    ThrowTypeError(info.GetIsolate(),
                   "putImageData requires width, height, data, dx, and dy");
    return;
  }

  auto view = data_value.As<v8::ArrayBufferView>();
  std::shared_ptr<v8::BackingStore> backing_store = view->Buffer()->GetBackingStore();
  const size_t byte_offset = view->ByteOffset();
  const size_t byte_length = view->ByteLength();
  const int image_width = static_cast<int>(std::lround(width));
  const int image_height = static_cast<int>(std::lround(height));
  const size_t expected_size =
      static_cast<size_t>(image_width) * static_cast<size_t>(image_height) * 4u;
  if (byte_length != expected_size) {
    ThrowTypeError(info.GetIsolate(), "putImageData data length does not match dimensions");
    return;
  }

  canvas_engine::Canvas2DContext::ImageData image_data;
  image_data.width = image_width;
  image_data.height = image_height;
  image_data.data.resize(byte_length);
  std::memcpy(image_data.data.data(),
              static_cast<const std::uint8_t*>(backing_store->Data()) + byte_offset,
              byte_length);

  if (!handle->context->PutImageData(image_data, static_cast<int>(std::lround(dx)),
                                     static_cast<int>(std::lround(dy)))) {
    ThrowError(info.GetIsolate(), "failed to write image data");
  }
}


}  // namespace canvas_engine

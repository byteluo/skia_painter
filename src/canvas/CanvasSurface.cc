#include "canvas_engine/canvas/CanvasSurface.h"

#include <algorithm>
#include <cmath>
#include <vector>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

#include "include/core/SkImageInfo.h"
#include "include/core/SkImage.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkStream.h"
#if !defined(__APPLE__)
#include "include/encode/SkPngEncoder.h"
#endif

namespace canvas_engine {

namespace {

int ComputePixelSize(int logical_size, float pixel_ratio) {
  return std::max(1, static_cast<int>(std::lround(logical_size * pixel_ratio)));
}

sk_sp<SkSurface> CreateRasterSurface(int width, int height, float pixel_ratio) {
  if (width <= 0 || height <= 0) {
    return nullptr;
  }

  if (!(pixel_ratio > 0.0f)) {
    return nullptr;
  }

  const int pixel_width = ComputePixelSize(width, pixel_ratio);
  const int pixel_height = ComputePixelSize(height, pixel_ratio);
  SkImageInfo image_info = SkImageInfo::MakeN32Premul(pixel_width, pixel_height);
  return SkSurfaces::Raster(image_info);
}

}  // namespace

CanvasSurface::CanvasSurface(int width, int height, float pixel_ratio,
                             sk_sp<SkSurface> surface)
    : width_(width),
      height_(height),
      pixel_width_(ComputePixelSize(width, pixel_ratio)),
      pixel_height_(ComputePixelSize(height, pixel_ratio)),
      pixel_ratio_(pixel_ratio),
      surface_(std::move(surface)) {}

std::shared_ptr<CanvasSurface> CanvasSurface::CreateRaster(int width, int height,
                                                           float pixel_ratio) {
  auto surface = CreateRasterSurface(width, height, pixel_ratio);
  if (!surface) {
    return nullptr;
  }

  return std::shared_ptr<CanvasSurface>(
      new CanvasSurface(width, height, pixel_ratio, std::move(surface)));
}

bool CanvasSurface::Resize(int width, int height) {
  auto surface = CreateRasterSurface(width, height, pixel_ratio_);
  if (!surface) {
    return false;
  }

  width_ = width;
  height_ = height;
  pixel_width_ = ComputePixelSize(width, pixel_ratio_);
  pixel_height_ = ComputePixelSize(height, pixel_ratio_);
  surface_ = std::move(surface);
  return true;
}

sk_sp<SkImage> CanvasSurface::MakeImageSnapshot() const {
  return surface_ ? surface_->makeImageSnapshot() : nullptr;
}

bool CanvasSurface::SavePng(const std::string& output_path) const {
  SkPixmap pixmap;
  if (!surface_) {
    return false;
  }

  if (!surface_->peekPixels(&pixmap)) {
    return false;
  }

#if defined(__APPLE__)
  std::vector<std::uint8_t> rgba_pixels(static_cast<size_t>(pixmap.width()) *
                                        static_cast<size_t>(pixmap.height()) * 4);
  const auto* src = static_cast<const std::uint8_t*>(pixmap.addr());
  if (src == nullptr) {
    return false;
  }

  const size_t src_row_bytes = pixmap.rowBytes();
  const size_t dst_row_bytes = static_cast<size_t>(pixmap.width()) * 4;
  for (int y = 0; y < pixmap.height(); ++y) {
    const auto* src_row = src + static_cast<size_t>(y) * src_row_bytes;
    auto* dst_row = rgba_pixels.data() + static_cast<size_t>(y) * dst_row_bytes;
    for (int x = 0; x < pixmap.width(); ++x) {
      const auto* pixel = src_row + static_cast<size_t>(x) * 4;
      auto* dst_pixel = dst_row + static_cast<size_t>(x) * 4;
      if (pixmap.colorType() == kBGRA_8888_SkColorType) {
        dst_pixel[0] = pixel[2];
        dst_pixel[1] = pixel[1];
        dst_pixel[2] = pixel[0];
        dst_pixel[3] = pixel[3];
      } else {
        dst_pixel[0] = pixel[0];
        dst_pixel[1] = pixel[1];
        dst_pixel[2] = pixel[2];
        dst_pixel[3] = pixel[3];
      }
    }
  }

  const size_t data_size = rgba_pixels.size();
  auto release_callback = [](void*, const void*, size_t) {};

  CGDataProviderRef provider = CGDataProviderCreateWithData(
      nullptr, rgba_pixels.data(), data_size, release_callback);
  if (!provider) {
    return false;
  }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (!color_space) {
    CGDataProviderRelease(provider);
    return false;
  }

  const CGBitmapInfo bitmap_info =
      static_cast<CGBitmapInfo>(static_cast<unsigned>(kCGBitmapByteOrder32Big) |
                                static_cast<unsigned>(kCGImageAlphaPremultipliedLast));

  CGImageRef image = CGImageCreate(
      pixmap.width(), pixmap.height(), 8, 32, dst_row_bytes, color_space,
      bitmap_info, provider, nullptr, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(color_space);
  CGDataProviderRelease(provider);
  if (!image) {
    return false;
  }

  CFStringRef path = CFStringCreateWithCString(
      nullptr, output_path.c_str(), kCFStringEncodingUTF8);
  if (!path) {
    CGImageRelease(image);
    return false;
  }

  CFURLRef url = CFURLCreateWithFileSystemPath(
      nullptr, path, kCFURLPOSIXPathStyle, false);
  CFRelease(path);
  if (!url) {
    CGImageRelease(image);
    return false;
  }

  CGImageDestinationRef destination =
      CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
  CFRelease(url);
  if (!destination) {
    CGImageRelease(image);
    return false;
  }

  CGImageDestinationAddImage(destination, image, nullptr);
  const bool success = CGImageDestinationFinalize(destination);
  CFRelease(destination);
  CGImageRelease(image);
  return success;
#else
  SkFILEWStream stream(output_path.c_str());
  if (!stream.isValid()) {
    return false;
  }

  SkPngEncoder::Options options;
  options.fZLibLevel = 6;
  return SkPngEncoder::Encode(&stream, pixmap, options);
#endif
}

}  // namespace canvas_engine

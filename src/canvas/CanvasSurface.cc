#include "canvas_engine/canvas/CanvasSurface.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"
#include "include/core/SkStream.h"
#if !defined(__APPLE__)
#include "include/encode/SkPngEncoder.h"
#endif

namespace canvas_engine {

CanvasSurface::CanvasSurface(int width, int height, sk_sp<SkSurface> surface)
    : width_(width), height_(height), surface_(std::move(surface)) {}

std::shared_ptr<CanvasSurface> CanvasSurface::CreateRaster(int width, int height) {
  if (width <= 0 || height <= 0) {
    return nullptr;
  }

  SkImageInfo image_info = SkImageInfo::MakeN32Premul(width, height);
  auto surface = SkSurfaces::Raster(image_info);
  if (!surface) {
    return nullptr;
  }

  return std::shared_ptr<CanvasSurface>(
      new CanvasSurface(width, height, std::move(surface)));
}

bool CanvasSurface::SavePng(const std::string& output_path) const {
  SkPixmap pixmap;
  if (!surface_ || !surface_->peekPixels(&pixmap)) {
    return false;
  }

#if defined(__APPLE__)
  const size_t data_size = static_cast<size_t>(pixmap.rowBytes()) * pixmap.height();
  auto release_callback = [](void*, const void*, size_t) {};

  CGDataProviderRef provider = CGDataProviderCreateWithData(
      nullptr, pixmap.addr(), data_size, release_callback);
  if (!provider) {
    return false;
  }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (!color_space) {
    CGDataProviderRelease(provider);
    return false;
  }

  const CGBitmapInfo bitmap_info =
      static_cast<CGBitmapInfo>(static_cast<unsigned>(kCGBitmapByteOrder32Little) |
                                static_cast<unsigned>(kCGImageAlphaPremultipliedFirst));

  CGImageRef image = CGImageCreate(
      pixmap.width(), pixmap.height(), 8, 32, pixmap.rowBytes(), color_space,
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

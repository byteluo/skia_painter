#include "canvas_engine/canvas/ImageAsset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"

namespace canvas_engine {

namespace {

#if defined(__APPLE__)
struct ScopedCFTypeRef {
  ScopedCFTypeRef() = default;
  explicit ScopedCFTypeRef(CFTypeRef value) : value(value) {}
  ~ScopedCFTypeRef() {
    if (value != nullptr) {
      CFRelease(value);
    }
  }

  CFTypeRef value = nullptr;
};
#endif

}  // namespace

ImageAsset::ImageAsset(std::string path, int width, int height, sk_sp<SkImage> image)
    : path_(std::move(path)),
      width_(width),
      height_(height),
      image_(std::move(image)) {}

std::shared_ptr<ImageAsset> ImageAsset::LoadFromFile(const std::string& path) {
#if defined(__APPLE__)
  ScopedCFTypeRef path_ref(
      CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8));
  if (path_ref.value == nullptr) {
    return nullptr;
  }

  ScopedCFTypeRef url_ref(
      CFURLCreateWithFileSystemPath(nullptr, static_cast<CFStringRef>(path_ref.value),
                                    kCFURLPOSIXPathStyle, false));
  if (url_ref.value == nullptr) {
    return nullptr;
  }

  ScopedCFTypeRef source_ref(
      CGImageSourceCreateWithURL(static_cast<CFURLRef>(url_ref.value), nullptr));
  if (source_ref.value == nullptr) {
    return nullptr;
  }

  CGImageRef cg_image = CGImageSourceCreateImageAtIndex(
      reinterpret_cast<CGImageSourceRef>(const_cast<void*>(source_ref.value)), 0,
      nullptr);
  if (cg_image == nullptr) {
    return nullptr;
  }

  const int width = static_cast<int>(CGImageGetWidth(cg_image));
  const int height = static_cast<int>(CGImageGetHeight(cg_image));
  if (width <= 0 || height <= 0) {
    CGImageRelease(cg_image);
    return nullptr;
  }

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (color_space == nullptr) {
    CGImageRelease(cg_image);
    return nullptr;
  }

  CGContextRef context = CGBitmapContextCreate(
      pixels.data(), width, height, 8, static_cast<size_t>(width) * 4, color_space,
      static_cast<CGBitmapInfo>(static_cast<unsigned>(kCGBitmapByteOrder32Big) |
                                static_cast<unsigned>(kCGImageAlphaPremultipliedLast)));
  CGColorSpaceRelease(color_space);
  if (context == nullptr) {
    CGImageRelease(cg_image);
    return nullptr;
  }

  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cg_image);
  CGContextRelease(context);
  CGImageRelease(cg_image);

  const SkImageInfo info = SkImageInfo::Make(width, height,
                                             kRGBA_8888_SkColorType,
                                             kPremul_SkAlphaType);
  const SkPixmap pixmap(info, pixels.data(), static_cast<size_t>(width) * 4);
  auto image = SkImages::RasterFromPixmapCopy(pixmap);
  if (!image) {
    return nullptr;
  }

  return std::shared_ptr<ImageAsset>(
      new ImageAsset(path, width, height, std::move(image)));
#else
  (void)path;
  return nullptr;
#endif
}

}  // namespace canvas_engine

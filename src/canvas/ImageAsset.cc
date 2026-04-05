#include "canvas_engine/canvas/ImageAsset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#elif defined(__linux__)
#include <png.h>
#include <cstdio>
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
#elif defined(__linux__)
  FILE* fp = fopen(path.c_str(), "rb");
  if (!fp) {
    return nullptr;
  }

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png) {
    fclose(fp);
    return nullptr;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, nullptr, nullptr);
    fclose(fp);
    return nullptr;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return nullptr;
  }

  png_init_io(png, fp);
  png_read_info(png, info);

  const int width = static_cast<int>(png_get_image_width(png, info));
  const int height = static_cast<int>(png_get_image_height(png, info));
  const png_byte color_type = png_get_color_type(png, info);
  const png_byte bit_depth = png_get_bit_depth(png, info);

  if (bit_depth == 16) {
    png_set_strip_16(png);
  }
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_to_rgb(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
  }
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) {
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  }

  png_read_update_info(png, info);

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
  std::vector<png_bytep> row_pointers(static_cast<std::size_t>(height));
  for (int y = 0; y < height; ++y) {
    row_pointers[y] = pixels.data() + static_cast<std::size_t>(y) * width * 4;
  }
  png_read_image(png, row_pointers.data());
  png_read_end(png, nullptr);
  png_destroy_read_struct(&png, &info, nullptr);
  fclose(fp);

  // Pre-multiply alpha
  for (std::size_t i = 0; i < pixels.size(); i += 4) {
    const unsigned a = pixels[i + 3];
    pixels[i + 0] = static_cast<std::uint8_t>(pixels[i + 0] * a / 255);
    pixels[i + 1] = static_cast<std::uint8_t>(pixels[i + 1] * a / 255);
    pixels[i + 2] = static_cast<std::uint8_t>(pixels[i + 2] * a / 255);
  }

  const SkImageInfo info_sk = SkImageInfo::Make(width, height,
                                                kRGBA_8888_SkColorType,
                                                kPremul_SkAlphaType);
  const SkPixmap pixmap(info_sk, pixels.data(), static_cast<size_t>(width) * 4);
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

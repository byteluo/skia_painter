#pragma once

#include <memory>
#include <string>

#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

namespace canvas_engine {

class CanvasSurface {
 public:
  static std::shared_ptr<CanvasSurface> CreateRaster(int width, int height,
                                                     float pixel_ratio = 1.0f);

  int width() const { return width_; }
  int height() const { return height_; }
  float pixel_ratio() const { return pixel_ratio_; }
  int pixel_width() const { return pixel_width_; }
  int pixel_height() const { return pixel_height_; }
  SkCanvas* canvas() const { return surface_->getCanvas(); }
  bool PeekPixels(SkPixmap* pixmap) const { return surface_ && surface_->peekPixels(pixmap); }
  sk_sp<SkImage> MakeImageSnapshot() const;

  bool Resize(int width, int height);
  bool SavePng(const std::string& output_path) const;

 private:
  CanvasSurface(int width, int height, float pixel_ratio, sk_sp<SkSurface> surface);

  int width_;
  int height_;
  int pixel_width_;
  int pixel_height_;
  float pixel_ratio_;
  sk_sp<SkSurface> surface_;
};

}  // namespace canvas_engine

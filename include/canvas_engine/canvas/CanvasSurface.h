#pragma once

#include <memory>
#include <string>

#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

namespace canvas_engine {

class CanvasSurface {
 public:
  static std::shared_ptr<CanvasSurface> CreateRaster(int width, int height);

  int width() const { return width_; }
  int height() const { return height_; }
  SkCanvas* canvas() const { return surface_->getCanvas(); }
  bool PeekPixels(SkPixmap* pixmap) const { return surface_ && surface_->peekPixels(pixmap); }

  bool Resize(int width, int height);
  bool SavePng(const std::string& output_path) const;

 private:
  CanvasSurface(int width, int height, sk_sp<SkSurface> surface);

  int width_;
  int height_;
  sk_sp<SkSurface> surface_;
};

}  // namespace canvas_engine

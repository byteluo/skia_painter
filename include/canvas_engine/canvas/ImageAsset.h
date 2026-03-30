#pragma once

#include <memory>
#include <string>

#include "include/core/SkImage.h"

namespace canvas_engine {

class ImageAsset {
 public:
  static std::shared_ptr<ImageAsset> LoadFromFile(const std::string& path);

  int width() const { return width_; }
  int height() const { return height_; }
  const std::string& path() const { return path_; }
  const sk_sp<SkImage>& image() const { return image_; }

 private:
  ImageAsset(std::string path, int width, int height, sk_sp<SkImage> image);

  std::string path_;
  int width_;
  int height_;
  sk_sp<SkImage> image_;
};

}  // namespace canvas_engine

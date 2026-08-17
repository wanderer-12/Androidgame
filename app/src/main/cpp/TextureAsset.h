#pragma once

#include <GLES3/gl3.h>
#include <android/asset_manager.h>

#include <memory>
#include <string>

class TextureAsset {
public:
    static std::shared_ptr<TextureAsset> loadAsset(
            AAssetManager *assetManager,
            const std::string &assetPath,
            bool nearestFiltering);

    ~TextureAsset();

    GLuint textureId() const { return textureId_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    TextureAsset(GLuint textureId, int width, int height)
            : textureId_(textureId), width_(width), height_(height) {}

    GLuint textureId_ = 0;
    int width_ = 0;
    int height_ = 0;
};

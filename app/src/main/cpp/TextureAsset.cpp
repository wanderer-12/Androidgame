#include "TextureAsset.h"

#include <android/asset_manager.h>

#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "third_party/stb_image.h"

#include "AndroidOut.h"

std::shared_ptr<TextureAsset> TextureAsset::loadAsset(
        AAssetManager *assetManager,
        const std::string &assetPath,
        bool nearestFiltering) {
    AAsset *asset = AAssetManager_open(assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        aout << "Unable to open texture asset: " << assetPath << std::endl;
        return nullptr;
    }

    const auto length = static_cast<int>(AAsset_getLength(asset));
    std::vector<unsigned char> encoded(length);
    int totalRead = 0;
    while (totalRead < length) {
        const int bytesRead = AAsset_read(asset, encoded.data() + totalRead, length - totalRead);
        if (bytesRead <= 0) break;
        totalRead += bytesRead;
    }
    AAsset_close(asset);
    if (totalRead != length) {
        aout << "Unable to read texture asset: " << assetPath << std::endl;
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load_from_memory(encoded.data(), length, &width, &height,
                                                  &channels, STBI_rgb_alpha);
    if (!pixels) {
        aout << "Unable to decode texture asset: " << assetPath << " "
             << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const GLint filter = nearestFiltering ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    return std::shared_ptr<TextureAsset>(new TextureAsset(texture, width, height));
}

TextureAsset::~TextureAsset() {
    if (textureId_) {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }
}

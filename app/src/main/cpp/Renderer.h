#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <memory>
#include <string>
#include <utility>

#include "TextureAsset.h"

struct android_app;

constexpr float kCanvasWidth = 360.0f;
constexpr float kCanvasHeight = 640.0f;

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

enum class TextAlign { Left, Center, Right };

class Renderer {
public:
    explicit Renderer(android_app *app);
    ~Renderer();

    bool ready() const { return ready_; }
    std::shared_ptr<TextureAsset> loadTexture(const std::string &path, bool nearestFiltering);

    void beginFrame();
    void drawSprite(
            const TextureAsset &texture,
            float x,
            float y,
            float width,
            float height,
            float rotation = 0.0f,
            Color tint = {},
            float u0 = 0.0f,
            float v0 = 0.0f,
            float u1 = 1.0f,
            float v1 = 1.0f);
    void drawRect(float x, float y, float width, float height, Color color);
    void drawText(
            const TextureAsset &font,
            const std::string &text,
            float x,
            float y,
            float scale,
            Color color = {},
            TextAlign align = TextAlign::Left);
    void endFrame();

    bool screenToCanvas(float screenX, float screenY, float &canvasX, float &canvasY) const;
    std::pair<float, float> screenDeltaToCanvas(float dx, float dy) const;

private:
    bool initEgl();
    bool initGl();
    void updateViewport();
    static GLuint compileShader(GLenum type, const char *source);

    android_app *app_ = nullptr;
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;

    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLuint whiteTexture_ = 0;
    GLint positionUniform_ = -1;
    GLint sizeUniform_ = -1;
    GLint rotationUniform_ = -1;
    GLint canvasUniform_ = -1;
    GLint uvRectUniform_ = -1;
    GLint tintUniform_ = -1;

    int surfaceWidth_ = 0;
    int surfaceHeight_ = 0;
    int viewportX_ = 0;
    int viewportY_ = 0;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    float canvasScale_ = 1.0f;
    bool ready_ = false;
};

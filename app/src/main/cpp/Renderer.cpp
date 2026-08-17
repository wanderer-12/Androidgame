#include "Renderer.h"

#include <android/native_window.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "AndroidOut.h"

namespace {
constexpr char kVertexShader[] = R"shader(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUv;

uniform vec2 uCanvas;
uniform vec2 uPosition;
uniform vec2 uSize;
uniform float uRotation;
uniform vec4 uUvRect;

out vec2 vUv;

void main() {
    vec2 local = aPosition * uSize;
    float c = cos(uRotation);
    float s = sin(uRotation);
    vec2 world = vec2(local.x * c - local.y * s, local.x * s + local.y * c) + uPosition;
    vec2 clip = vec2(world.x / uCanvas.x * 2.0 - 1.0, 1.0 - world.y / uCanvas.y * 2.0);
    gl_Position = vec4(clip, 0.0, 1.0);
    vUv = uUvRect.xy + aUv * uUvRect.zw;
}
)shader";

constexpr char kFragmentShader[] = R"shader(#version 300 es
precision mediump float;

in vec2 vUv;
uniform sampler2D uTexture;
uniform vec4 uTint;
out vec4 outColor;

void main() {
    outColor = texture(uTexture, vUv) * uTint;
}
)shader";

int glyphIndex(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'A' && character <= 'Z') return 10 + character - 'A';
    if (character == ':') return 36;
    if (character == '-') return 37;
    if (character == '/') return 38;
    if (character == '.') return 39;
    return -1;
}
}

Renderer::Renderer(android_app *app) : app_(app) {
    ready_ = initEgl() && initGl();
}

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY && context_ != EGL_NO_CONTEXT) {
        eglMakeCurrent(display_, surface_, surface_, context_);
        if (whiteTexture_) glDeleteTextures(1, &whiteTexture_);
        if (indexBuffer_) glDeleteBuffers(1, &indexBuffer_);
        if (vertexBuffer_) glDeleteBuffers(1, &vertexBuffer_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
        if (program_) glDeleteProgram(program_);
    }
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
        eglTerminate(display_);
    }
}

bool Renderer::initEgl() {
    constexpr EGLint attributes[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE};
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY || !eglInitialize(display_, nullptr, nullptr)) return false;

    EGLConfig config = nullptr;
    EGLint configCount = 0;
    if (!eglChooseConfig(display_, attributes, &config, 1, &configCount) || configCount == 0) {
        aout << "No suitable EGL config" << std::endl;
        return false;
    }

    EGLint format = 0;
    eglGetConfigAttrib(display_, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app_->window, 0, 0, format);
    surface_ = eglCreateWindowSurface(display_, config, app_->window, nullptr);
    constexpr EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, contextAttributes);
    if (surface_ == EGL_NO_SURFACE || context_ == EGL_NO_CONTEXT ||
        !eglMakeCurrent(display_, surface_, surface_, context_)) {
        aout << "Unable to create OpenGL ES 3 context" << std::endl;
        return false;
    }
    eglSwapInterval(display_, 1);
    return true;
}

GLuint Renderer::compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
        aout << "Shader compile failure: " << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Renderer::initGl() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!vertexShader || !fragmentShader) return false;
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) return false;

    positionUniform_ = glGetUniformLocation(program_, "uPosition");
    sizeUniform_ = glGetUniformLocation(program_, "uSize");
    rotationUniform_ = glGetUniformLocation(program_, "uRotation");
    canvasUniform_ = glGetUniformLocation(program_, "uCanvas");
    uvRectUniform_ = glGetUniformLocation(program_, "uUvRect");
    tintUniform_ = glGetUniformLocation(program_, "uTint");

    constexpr float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f,
             0.5f, -0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f};
    constexpr uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const uint32_t whitePixel = 0xffffffff;
    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, &whitePixel);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return glGetError() == GL_NO_ERROR;
}

std::shared_ptr<TextureAsset> Renderer::loadTexture(const std::string &path, bool nearestFiltering) {
    return TextureAsset::loadAsset(app_->activity->assetManager, path, nearestFiltering);
}

void Renderer::updateViewport() {
    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
    if (width == surfaceWidth_ && height == surfaceHeight_) return;
    surfaceWidth_ = width;
    surfaceHeight_ = height;
    const float canvasAspect = kCanvasWidth / kCanvasHeight;
    if (static_cast<float>(width) / static_cast<float>(height) > canvasAspect) {
        viewportHeight_ = height;
        viewportWidth_ = static_cast<int>(std::round(height * canvasAspect));
    } else {
        viewportWidth_ = width;
        viewportHeight_ = static_cast<int>(std::round(width / canvasAspect));
    }
    viewportX_ = (width - viewportWidth_) / 2;
    viewportY_ = (height - viewportHeight_) / 2;
    canvasScale_ = static_cast<float>(viewportWidth_) / kCanvasWidth;
}

void Renderer::beginFrame() {
    updateViewport();
    glViewport(0, 0, surfaceWidth_, surfaceHeight_);
    glClearColor(0.02f, 0.025f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(viewportX_, viewportY_, viewportWidth_, viewportHeight_);
    glUseProgram(program_);
    glBindVertexArray(vao_);
    glUniform2f(canvasUniform_, kCanvasWidth, kCanvasHeight);
}

void Renderer::drawSprite(
        const TextureAsset &texture,
        float x,
        float y,
        float width,
        float height,
        float rotation,
        Color tint,
        float u0,
        float v0,
        float u1,
        float v1) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.textureId());
    glUniform2f(positionUniform_, x, y);
    glUniform2f(sizeUniform_, width, height);
    glUniform1f(rotationUniform_, rotation);
    glUniform4f(uvRectUniform_, u0, v0, u1 - u0, v1 - v0);
    glUniform4f(tintUniform_, tint.r, tint.g, tint.b, tint.a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void Renderer::drawRect(float x, float y, float width, float height, Color color) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    glUniform2f(positionUniform_, x, y);
    glUniform2f(sizeUniform_, width, height);
    glUniform1f(rotationUniform_, 0.0f);
    glUniform4f(uvRectUniform_, 0.0f, 0.0f, 1.0f, 1.0f);
    glUniform4f(tintUniform_, color.r, color.g, color.b, color.a);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void Renderer::drawText(
        const TextureAsset &font,
        const std::string &text,
        float x,
        float y,
        float scale,
        Color color,
        TextAlign align) {
    const float advance = 6.0f * scale;
    float cursor = x;
    if (align == TextAlign::Center) cursor -= text.size() * advance * 0.5f;
    if (align == TextAlign::Right) cursor -= text.size() * advance;
    for (char character : text) {
        const int index = glyphIndex(character);
        if (index >= 0) {
            const int column = index % 16;
            const int row = index / 16;
            const float u0 = column / 16.0f;
            const float v0 = row / 3.0f;
            const float u1 = (column + 1) / 16.0f;
            const float v1 = (row + 1) / 3.0f;
            drawSprite(font, cursor + 3.0f * scale, y + 4.0f * scale,
                       6.0f * scale, 8.0f * scale, 0.0f, color, u0, v0, u1, v1);
        }
        cursor += advance;
    }
}

void Renderer::endFrame() {
    eglSwapBuffers(display_, surface_);
}

bool Renderer::screenToCanvas(float screenX, float screenY, float &canvasX, float &canvasY) const {
    if (canvasScale_ <= 0.0f) return false;
    canvasX = (screenX - viewportX_) / canvasScale_;
    canvasY = (screenY - viewportY_) / canvasScale_;
    const bool inside = canvasX >= 0.0f && canvasX <= kCanvasWidth &&
                        canvasY >= 0.0f && canvasY <= kCanvasHeight;
    canvasX = std::clamp(canvasX, 0.0f, kCanvasWidth);
    canvasY = std::clamp(canvasY, 0.0f, kCanvasHeight);
    return inside;
}

std::pair<float, float> Renderer::screenDeltaToCanvas(float dx, float dy) const {
    if (canvasScale_ <= 0.0f) return {0.0f, 0.0f};
    return {dx / canvasScale_, dy / canvasScale_};
}

#include "Renderer.h"
#include <cmath>
#include <cstdlib>

namespace HeavyWeapon {

Color4f Renderer::sCurrentColor = Color4f::White();
bool Renderer::sIsAdditive = false;
float Renderer::sShakeIntensity = 0.0f;
float Renderer::sShakeTimer = 0.0f;
float Renderer::sShakeOffsetX = 0.0f;
float Renderer::sShakeOffsetY = 0.0f;

void Renderer::Init() {
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void Renderer::BeginFrame() {
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (sShakeTimer > 0.0f) {
        glTranslatef(sShakeOffsetX, sShakeOffsetY, 0.0f);
    }

    SetAdditiveBlend(false);
    SetColor(Color4f::White());
}

void Renderer::EndFrame() {
    vglSwapBuffers(GL_FALSE);
}

void Renderer::SetAdditiveBlend(bool additive) {
    if (sIsAdditive != additive) {
        sIsAdditive = additive;
        if (additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
}

void Renderer::SetColor(const Color4f& color) {
    sCurrentColor = color;
    glColor4f(color.r, color.g, color.b, color.a);
}

void Renderer::DrawTexture(const Texture* tex, float dx, float dy) {
    if (!tex || tex->id == 0) return;
    DrawTexture(tex, dx, dy, (float)tex->width, (float)tex->height);
}

void Renderer::DrawTexture(const Texture* tex, float dx, float dy, float dw, float dh) {
    if (!tex || tex->id == 0) return;
    Rect dst = { dx, dy, dw, dh };
    Rect src = { 0.0f, 0.0f, (float)tex->width, (float)tex->height };
    DrawTexture(tex, dst, src);
}

void Renderer::DrawTexture(const Texture* tex, const Rect& dst, const Rect& src) {
    if (!tex || tex->id == 0) return;

    glBindTexture(GL_TEXTURE_2D, tex->id);

    float u0 = src.x / (float)tex->width;
    float v0 = src.y / (float)tex->height;
    float u1 = (src.x + src.w) / (float)tex->width;
    float v1 = (src.y + src.h) / (float)tex->height;

    GLfloat vertices[] = {
        dst.x,         dst.y,
        dst.x + dst.w, dst.y,
        dst.x,         dst.y + dst.h,
        dst.x + dst.w, dst.y + dst.h
    };

    GLfloat texCoords[] = {
        u0, v0,
        u1, v0,
        u0, v1,
        u1, v1
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Renderer::DrawTextureRotated(const Texture* tex, const Rect& dst, const Rect& src, float angleDegrees, float pivotX, float pivotY) {
    if (!tex || tex->id == 0) return;

    float px = (pivotX < 0.0f) ? dst.x + dst.w * 0.5f : dst.x + pivotX;
    float py = (pivotY < 0.0f) ? dst.y + dst.h * 0.5f : dst.y + pivotY;

    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glRotatef(angleDegrees, 0.0f, 0.0f, 1.0f);
    glTranslatef(-px, -py, 0.0f);

    DrawTexture(tex, dst, src);

    glPopMatrix();
}

void Renderer::DrawCel(const Texture* tex, int col, int row, float x, float y, bool centered, float scaleX, float scaleY, float angleDegrees) {
    if (!tex || tex->id == 0) return;

    float celW = (float)tex->GetCelWidth();
    float celH = (float)tex->GetCelHeight();
    Rect src = { (float)col * celW, (float)row * celH, celW, celH };

    float dw = celW * scaleX;
    float dh = celH * scaleY;
    Rect dst = centered ? Rect{ x - dw * 0.5f, y - dh * 0.5f, dw, dh } : Rect{ x, y, dw, dh };

    if (angleDegrees != 0.0f) {
        DrawTextureRotated(tex, dst, src, angleDegrees);
    } else {
        DrawTexture(tex, dst, src);
    }
}

void Renderer::DrawRect(float x, float y, float w, float h, const Color4f& color) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(color.r, color.g, color.b, color.a);

    GLfloat vertices[] = {
        x,     y,
        x + w, y,
        x + w, y + h,
        x,     y + h
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_TEXTURE_2D);
    SetColor(sCurrentColor);
}

void Renderer::DrawFillRect(float x, float y, float w, float h, const Color4f& color) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(color.r, color.g, color.b, color.a);

    GLfloat vertices[] = {
        x,     y,
        x + w, y,
        x,     y + h,
        x + w, y + h
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_TEXTURE_2D);
    SetColor(sCurrentColor);
}

void Renderer::DrawLine(float x1, float y1, float x2, float y2, const Color4f& color, float width) {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(width);
    glColor4f(color.r, color.g, color.b, color.a);

    GLfloat vertices[] = { x1, y1, x2, y2 };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_LINES, 0, 2);
    glDisableClientState(GL_VERTEX_ARRAY);

    glLineWidth(1.0f);
    glEnable(GL_TEXTURE_2D);
    SetColor(sCurrentColor);
}

void Renderer::AddScreenShake(float intensity, float durationSeconds) {
    sShakeIntensity = std::max(sShakeIntensity, intensity);
    sShakeTimer = std::max(sShakeTimer, durationSeconds);
}

void Renderer::UpdateScreenShake(float dt) {
    if (sShakeTimer > 0.0f) {
        sShakeTimer -= dt;
        if (sShakeTimer <= 0.0f) {
            sShakeTimer = 0.0f;
            sShakeIntensity = 0.0f;
            sShakeOffsetX = 0.0f;
            sShakeOffsetY = 0.0f;
        } else {
            float progress = sShakeTimer;
            float maxOffset = sShakeIntensity * progress;
            sShakeOffsetX = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * maxOffset;
            sShakeOffsetY = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * maxOffset;
        }
    }
}

} // namespace HeavyWeapon

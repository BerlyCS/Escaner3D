#include "camera.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera()
    : angH(0.0f), angV(30.0f), dist(5.0f)
    , cx(0.0f), cy(0.0f), cz(0.0f)
    , mx(0), my(0), drag(false), btn(0)
{}

void Camera::apply() {
    float p = angV * (float)M_PI / 180.0f;
    float t = angH * (float)M_PI / 180.0f;
    float x = cx + dist * sinf(t) * sinf(p);
    float y = cy + dist * cosf(p);
    float z = cz + dist * cosf(t) * sinf(p);
    glLoadIdentity();
    gluLookAt(x, y, z, cx, cy, cz, 0.0f, 1.0f, 0.0f);
}

void Camera::lookAt(float x, float y, float z) {
    cx = x; cy = y; cz = z;
}

void Camera::setDist(float d) {
    dist = d;
}

void Camera::mouse(int b, int st, int x, int y) {
    if (st == GLUT_DOWN) {
        drag = true;
        btn = b;
        mx = x;
        my = y;
    } else {
        drag = false;
    }
}

void Camera::motion(int x, int y) {
    if (!drag) return;
    int dx = x - mx;
    int dy = y - my;
    if (btn == GLUT_LEFT_BUTTON) {
        angH -= dx * 0.5f;
        angV -= dy * 0.5f;
        if (angV < 1.0f) angV = 1.0f;
        if (angV > 179.0f) angV = 179.0f;
    } else if (btn == GLUT_RIGHT_BUTTON) {
        dist *= (1.0f - dy * 0.01f);
        if (dist < 0.1f) dist = 0.1f;
        if (dist > 100.0f) dist = 100.0f;
    }
    mx = x;
    my = y;
}

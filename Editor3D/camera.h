#pragma once

class Camera {
public:
    Camera();
    void apply();
    void lookAt(float x, float y, float z);
    void setDist(float d);
    void mouse(int btn, int st, int x, int y);
    void motion(int x, int y);
private:
    float angH, angV, dist;
    float cx, cy, cz;
    int mx, my;
    bool drag;
    int btn;
};

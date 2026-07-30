#pragma once
#include <GL/gl.h>
#include <vector>
#include <string>

struct Mat {
    std::string name;
    float Ka[3], Kd[3], Ks[3];
    float Ns;
    GLuint tex;
    Mat() : Ns(0), tex(0) {
        Ka[0]=0.2f; Ka[1]=0.2f; Ka[2]=0.2f;
        Kd[0]=0.8f; Kd[1]=0.8f; Kd[2]=0.8f;
        Ks[0]=0.0f; Ks[1]=0.0f; Ks[2]=0.0f;
    }
};

struct Grp {
    std::vector<float> pos;
    std::vector<float> uv;
    std::vector<float> nrm;
    int mat;
    Grp() : mat(-1) {}
};

class Model {
public:
    Model();
    ~Model();
    bool load(const char* path, const char* mtlPath = nullptr);
    void render();
    void getCenter(float& x, float& y, float& z) const;
    float getRad() const;
private:
    std::vector<Grp> grps;
    std::vector<Mat> mats;
    float cx, cy, cz, rad;
    bool readMtl(const char* path, const char* dir);
    int findMat(const char* name) const;
    void genNrm(Grp& g);
    void fit();
};

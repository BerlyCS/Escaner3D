#include "model.h"
#include "texture.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>

struct V3 { float x, y, z; };
struct V2 { float u, v; };

Model::Model() : cx(0), cy(0), cz(0), rad(1) {}

Model::~Model() {
    for (auto& m : mats)
        if (m.tex) freeTex(m.tex);
}

int Model::findMat(const char* name) const {
    for (size_t i = 0; i < mats.size(); i++)
        if (mats[i].name == name) return (int)i;
    return -1;
}

bool Model::readMtl(const char* path, const char* dir) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Can't open MTL %s\n", path); return false; }

    char line[1024];
    Mat* cur = nullptr;

    while (fgets(line, sizeof(line), f)) {
        char cmd[64];
        if (sscanf(line, "%63s", cmd) != 1) continue;

        if (strcmp(cmd, "newmtl") == 0) {
            mats.push_back(Mat());
            cur = &mats.back();
            char* p = line + 7;
            while (*p == ' ' || *p == '\t') p++;
            char* e = p + strlen(p) - 1;
            while (e > p && (*e == '\n' || *e == '\r' || *e == ' ')) e--;
            *(e + 1) = '\0';
            cur->name = p;
        } else if (!cur) {
            continue;
        } else if (strcmp(cmd, "Ka") == 0) {
            sscanf(line, "%*s %f %f %f", &cur->Ka[0], &cur->Ka[1], &cur->Ka[2]);
        } else if (strcmp(cmd, "Kd") == 0) {
            sscanf(line, "%*s %f %f %f", &cur->Kd[0], &cur->Kd[1], &cur->Kd[2]);
        } else if (strcmp(cmd, "Ks") == 0) {
            sscanf(line, "%*s %f %f %f", &cur->Ks[0], &cur->Ks[1], &cur->Ks[2]);
        } else if (strcmp(cmd, "Ns") == 0) {
            sscanf(line, "%*s %f", &cur->Ns);
        } else if (strcmp(cmd, "map_Kd") == 0) {
            char* last = nullptr;
            char* save = nullptr;
            char* tok = strtok_r(line + 7, " \t\r\n", &save);
            while (tok) {
                if (tok[0] != '-') last = tok;
                tok = strtok_r(nullptr, " \t\r\n", &save);
            }
            if (last) {
                char texPath[1024];
                snprintf(texPath, sizeof(texPath), "%s%s", dir, last);
                cur->tex = loadTex(texPath);
            }
        }
    }
    fclose(f);
    return !mats.empty();
}

bool Model::load(const char* path, const char* mtlPath) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Can't open %s\n", path); return false; }

    std::vector<V3> verts, norms;
    std::vector<V2> texcs;
    std::map<int, int> matToGrp;
    char objDir[512] = {0};

    const char* s = strrchr(path, '/');
    if (s) {
        int len = (int)(s - path) + 1;
        strncpy(objDir, path, len);
        objDir[len] = '\0';
    }

    // Load explicit MTL before parsing OBJ
    if (mtlPath && mtlPath[0])
        readMtl(mtlPath, objDir);

    char line[1024];
    int curMat = -1;

    while (fgets(line, sizeof(line), f)) {
        char cmd[64];
        if (sscanf(line, "%63s", cmd) != 1) continue;

        if (strcmp(cmd, "v") == 0) {
            V3 v;
            if (sscanf(line, "%*s %f %f %f", &v.x, &v.y, &v.z) >= 3)
                verts.push_back(v);
        } else if (strcmp(cmd, "vt") == 0) {
            V2 t;
            if (sscanf(line, "%*s %f %f", &t.u, &t.v) >= 2)
                texcs.push_back(t);
        } else if (strcmp(cmd, "vn") == 0) {
            V3 n;
            if (sscanf(line, "%*s %f %f %f", &n.x, &n.y, &n.z) >= 3)
                norms.push_back(n);
        } else if (strcmp(cmd, "f") == 0) {
            struct { int v, t, n; } fv[32];
            int cnt = 0;
            char* save = nullptr;
            char* tok = strtok_r(line + 2, " \t\r\n", &save);
            while (tok && cnt < 32) {
                fv[cnt].v = 0; fv[cnt].t = -1; fv[cnt].n = -1;
                if (strstr(tok, "//")) {
                    sscanf(tok, "%d//%d", &fv[cnt].v, &fv[cnt].n);
                } else if (strchr(tok, '/')) {
                    int r = sscanf(tok, "%d/%d/%d", &fv[cnt].v, &fv[cnt].t, &fv[cnt].n);
                    if (r == 2) fv[cnt].n = -1;
                } else {
                    sscanf(tok, "%d", &fv[cnt].v);
                }
                if (fv[cnt].v > 0) fv[cnt].v--;
                else if (fv[cnt].v < 0) fv[cnt].v = (int)verts.size() + fv[cnt].v;
                if (fv[cnt].t > 0) fv[cnt].t--;
                else if (fv[cnt].t < 0) fv[cnt].t = (int)texcs.size() + fv[cnt].t;
                if (fv[cnt].n > 0) fv[cnt].n--;
                else if (fv[cnt].n < 0) fv[cnt].n = (int)norms.size() + fv[cnt].n;
                cnt++;
                tok = strtok_r(nullptr, " \t\r\n", &save);
            }
            if (cnt < 3) continue;

            auto it = matToGrp.find(curMat);
            int gi;
            if (it == matToGrp.end()) {
                Grp g;
                g.mat = curMat;
                grps.push_back(g);
                gi = (int)grps.size() - 1;
                matToGrp[curMat] = gi;
            } else {
                gi = it->second;
            }
            Grp& g = grps[gi];
            bool hasVt = !texcs.empty();
            bool hasVn = !norms.empty();

            for (int i = 1; i < cnt - 1; i++) {
                int idx[3] = {0, i, i + 1};
                for (int j = 0; j < 3; j++) {
                    int k = idx[j];
                    int vi = fv[k].v, ti = fv[k].t, ni = fv[k].n;
                    if (vi >= 0 && vi < (int)verts.size()) {
                        g.pos.push_back(verts[vi].x);
                        g.pos.push_back(verts[vi].y);
                        g.pos.push_back(verts[vi].z);
                    } else {
                        g.pos.push_back(0);
                        g.pos.push_back(0);
                        g.pos.push_back(0);
                    }
                    if (hasVt) {
                        if (ti >= 0 && ti < (int)texcs.size()) {
                            g.uv.push_back(texcs[ti].u);
                            g.uv.push_back(texcs[ti].v);
                        } else {
                            g.uv.push_back(0);
                            g.uv.push_back(0);
                        }
                    }
                    if (hasVn) {
                        if (ni >= 0 && ni < (int)norms.size()) {
                            g.nrm.push_back(norms[ni].x);
                            g.nrm.push_back(norms[ni].y);
                            g.nrm.push_back(norms[ni].z);
                        }
                    }
                }
            }
        } else if (strcmp(cmd, "usemtl") == 0) {
            char name[256];
            if (sscanf(line, "%*s %255s", name) == 1)
                curMat = findMat(name);
        } else if (strcmp(cmd, "mtllib") == 0 && !mtlPath) {
            char name[256];
            if (sscanf(line, "%*s %255s", name) == 1) {
                char mPath[512];
                snprintf(mPath, sizeof(mPath), "%s%s", objDir, name);
                readMtl(mPath, objDir);
            }
        }
    }
    fclose(f);

    if (verts.empty()) return false;

    for (auto& g : grps) {
        if (g.nrm.empty() || g.nrm.size() != g.pos.size())
            genNrm(g);
    }

    fit();
    return !grps.empty();
}

void Model::fit() {
    if (grps.empty() || grps[0].pos.empty()) return;
    float minX, minY, minZ, maxX, maxY, maxZ;
    minX = maxX = grps[0].pos[0];
    minY = maxY = grps[0].pos[1];
    minZ = maxZ = grps[0].pos[2];
    for (auto& g : grps) {
        for (size_t i = 0; i < g.pos.size(); i += 3) {
            float x = g.pos[i], y = g.pos[i+1], z = g.pos[i+2];
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        }
    }
    cx = (minX + maxX) * 0.5f;
    cy = (minY + maxY) * 0.5f;
    cz = (minZ + maxZ) * 0.5f;
    float dx = maxX - minX, dy = maxY - minY, dz = maxZ - minZ;
    rad = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
    if (rad < 0.001f) rad = 1.0f;
}

void Model::genNrm(Grp& g) {
    size_t n = g.pos.size();
    g.nrm.resize(n);
    for (size_t i = 0; i < n; i += 9) {
        float ax = g.pos[i+3] - g.pos[i+0];
        float ay = g.pos[i+4] - g.pos[i+1];
        float az = g.pos[i+5] - g.pos[i+2];
        float bx = g.pos[i+6] - g.pos[i+0];
        float by = g.pos[i+7] - g.pos[i+1];
        float bz = g.pos[i+8] - g.pos[i+2];
        float nx = ay*bz - az*by;
        float ny = az*bx - ax*bz;
        float nz = ax*by - ay*bx;
        float len = sqrtf(nx*nx + ny*ny + nz*nz);
        if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; }
        for (int j = 0; j < 3; j++) {
            g.nrm[i + j*3 + 0] = nx;
            g.nrm[i + j*3 + 1] = ny;
            g.nrm[i + j*3 + 2] = nz;
        }
    }
}

void Model::getCenter(float& x, float& y, float& z) const {
    x = cx; y = cy; z = cz;
}

float Model::getRad() const {
    return rad;
}

void Model::render() {
    for (auto& g : grps) {
        Mat* mat = nullptr;
        if (g.mat >= 0 && g.mat < (int)mats.size())
            mat = &mats[g.mat];

        if (mat) {
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat->Ka);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat->Kd);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat->Ks);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->Ns);
            if (mat->tex) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mat->tex);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            } else {
                glDisable(GL_TEXTURE_2D);
            }
        } else {
            glDisable(GL_TEXTURE_2D);
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, &g.pos[0]);

        if (g.nrm.empty()) {
            glDisableClientState(GL_NORMAL_ARRAY);
        } else {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, &g.nrm[0]);
        }

        if (g.uv.empty()) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        } else {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, &g.uv[0]);
        }

        glDrawArrays(GL_TRIANGLES, 0, (int)g.pos.size() / 3);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
}

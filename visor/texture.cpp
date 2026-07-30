#include "texture.h"
#include <cstdio>
#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint loadTex(const char* path) {
    int w, h, n;
    unsigned char* dat = stbi_load(path, &w, &h, &n, 0);
    if (!dat) {
        fprintf(stderr, "Failed to load texture: %s\n", path);
        return 0;
    }

    GLint intern = (n == 4) ? GL_RGBA : GL_RGB;
    GLenum fmt   = (n == 4) ? GL_RGBA : GL_RGB;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    gluBuild2DMipmaps(GL_TEXTURE_2D, intern, w, h, fmt, GL_UNSIGNED_BYTE, dat);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(dat);
    return id;
}

void freeTex(GLuint id) {
    if (id) glDeleteTextures(1, &id);
}

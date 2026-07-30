#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cstdio>
#include <cstring>
#include "camera.h"
#include "model.h"

static Model mdl;
static Camera cam;

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float pos[] = {0.0f, 5.0f, 10.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    cam.apply();
    mdl.render();
    glutSwapBuffers();
}

static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float r = mdl.getRad() * 3.0f;
    if (r < 1.0f) r = 5.0f;
    gluPerspective(45.0, (double)w / (double)h, 0.01f, r * 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

static void mouse(int btn, int st, int x, int y) {
    cam.mouse(btn, st, x, y);
}

static void motion(int x, int y) {
    cam.motion(x, y);
    glutPostRedisplay();
}

static void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    float amb[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float dif[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float spe[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spe);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Use: %s <modelo.obj> [--mtl modelo.mtl]\n", argv[0]);
        return 1;
    }

    const char* objPath = nullptr;
    const char* mtlPath = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mtl") == 0 && i + 1 < argc)
            mtlPath = argv[++i];
        else
            objPath = argv[i];
    }

    if (!objPath) {
        fprintf(stderr, "Use: %s <modelo.obj> [--mtl modelo.mtl]\n", argv[0]);
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("OBJ Viewer");

    if (!mdl.load(objPath, mtlPath)) {
        fprintf(stderr, "Error al cargar %s\n", objPath);
        return 1;
    }

    float cx, cy, cz;
    mdl.getCenter(cx, cy, cz);
    cam.lookAt(cx, cy, cz);
    cam.setDist(mdl.getRad() * 2.5f);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}

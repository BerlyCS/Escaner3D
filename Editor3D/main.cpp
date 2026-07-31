#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "camera.h"
#include "model.h"

using namespace std;

void save_scene();
void load_scene(string filename);

struct vec3 {
  float x = 0, y = 0, z = 0;
  vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};
struct col3 {
  float r = 1, g = 1, b = 1;
  col3(float r = 1, float g = 1, float b = 1) : r(r), g(g), b(b) {}
};

struct SceneObj {
  vec3 pos, rot;
  float scl = 1.f;
  col3 col;
  bool sel = false;
  Model *mdl = nullptr;

  SceneObj(vec3 p, col3 c) : pos(p), col(c) {}
  SceneObj(vec3 p, col3 c, Model *m) : pos(p), col(c), mdl(m) {}
};

vector<SceneObj *> objs;
int sel_idx = -1;

Camera cam;
float c_fov = 60;
int W = 1000, H = 1000;
int save_c = 0;

Model *baseMdl = nullptr;

vector<col3> pal = {{1, .3f, .3f}, {.3f, 1, .3f}, {.3f, .5f, 1},  {1, 1, .3f},
                    {1, .5f, 0},   {.9f, .3f, 1}, {0, .8f, .8f},  {1, .4f, .7f},
                    {.5f, 1, .5f}, {1, .7f, .3f}, {.4f, .4f, 1},  {.8f, 1, 0},
                    {1, .2f, .6f}, {0, .9f, .5f}, {.6f, .3f, .1f}};
int pi = 0;

void set_proj() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(c_fov, (float)W / H, 0.01f, 200.f);
  glMatrixMode(GL_MODELVIEW);
}

void draw_grid() {
  glDisable(GL_LIGHTING);
  glColor3f(.2f, .2f, .2f);
  glBegin(GL_LINES);
  for (int i = -8; i <= 8; i++) {
    glVertex3f(i, 0, -8);
    glVertex3f(i, 0, 8);
    glVertex3f(-8, 0, i);
    glVertex3f(8, 0, i);
  }
  glEnd();
}

void draw_gizmo() {
  glDisable(GL_LIGHTING);
  glLineWidth(2.5f);
  glBegin(GL_LINES);
  glColor3f(1, .2f, .2f);
  glVertex3f(0, 0, 0);
  glVertex3f(1.5f, 0, 0);
  glColor3f(.2f, 1, .2f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 1.5f, 0);
  glColor3f(.3f, .5f, 1);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 1.5f);
  glEnd();
  glLineWidth(1.f);
}

void draw_obj(SceneObj *o) {
  if (!o->mdl) return;
  glPushMatrix();
  glTranslatef(o->pos.x, o->pos.y, o->pos.z);
  glRotatef(o->rot.x, 1, 0, 0);
  glRotatef(o->rot.y, 0, 1, 0);
  glRotatef(o->rot.z, 0, 0, 1);
  glScalef(o->scl, o->scl, o->scl);
  glColor3f(o->col.r, o->col.g, o->col.b);
  glEnable(GL_LIGHTING);
  o->mdl->render();
  if (o->sel) {
    glDisable(GL_LIGHTING);
    glColor3f(.8f, .8f, 0.1f);
    float cx, cy, cz, r;
    o->mdl->getCenter(cx, cy, cz);
    r = o->mdl->getRad() * o->scl * 1.05f;
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glutWireSphere(r, 20, 20);
    glPopMatrix();
  }
  glPopMatrix();
}

void draw_str(float x, float y, const string &s, col3 c = {1, 1, 1}) {
  glColor3f(c.r, c.g, c.b);
  glRasterPos2f(x, y);
  for (char ch : s)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ch);
}

void draw_hud() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, W, 0, H, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glColor3f(.06f, .06f, .08f);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(W, 0);
  glVertex2f(W, 50);
  glVertex2f(0, 50);
  glEnd();

  draw_str(8, 34,
           "1:add OBJ  Tab:select  Del:delete  Q:clone  E:save  0:load custom OBJ",
           {.75f, .75f, .75f});
  draw_str(8, 18,
           "IJKLNM:move  UO:rotY  PR:rotZ  GH:rotX  +/-:scale  "
           "VB:FOV  Scroll:zoom  C:color",
           {.5f, .5f, .5f});

  if (sel_idx >= 0 && sel_idx < (int)objs.size()) {
    SceneObj *o = objs[sel_idx];
    char buf[300];
    sprintf(buf,
            "obj[%d]  |  pos(%.2f, %.2f, %.2f)  |  rot(%.0f, %.0f, %.0f)  | "
            " scl %.2f",
            sel_idx, o->pos.x, o->pos.y, o->pos.z, o->rot.x,
            o->rot.y, o->rot.z, o->scl);
    glColor3f(.06f, .06f, .1f);
    glBegin(GL_QUADS);
    glVertex2f(0, H - 22);
    glVertex2f(W, H - 22);
    glVertex2f(W, H);
    glVertex2f(0, H);
    glEnd();
    draw_str(8, H - 16, buf, {1, 1, .5f});
  }

  string cnt = to_string(objs.size()) + " objetos";
  draw_str(W - 100, 34, cnt.data(), {.5f, .5f, .5f});

  glEnable(GL_DEPTH_TEST);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  GLfloat lp[] = {0, 5, 10, 1};
  glLightfv(GL_LIGHT0, GL_POSITION, lp);

  cam.apply();
  draw_grid();
  draw_gizmo();

  for (auto *o : objs)
    draw_obj(o);

  draw_hud();
  glutSwapBuffers();
}

void reshape(int w, int h) {
  W = w;
  H = h;
  glViewport(0, 0, w, h);
  set_proj();
}

void keyboard(unsigned char key, int, int) {
  SceneObj *s =
      (sel_idx >= 0 && sel_idx < (int)objs.size()) ? objs[sel_idx] : nullptr;
  const float step = 0.2f;

  switch (key) {
  case 27:
    exit(0);

  case '1': {
    if (!baseMdl) {
      printf("No hay modelo base cargado\n");
      break;
    }
    SceneObj *so = new SceneObj({0, 0, 0}, pal[pi], baseMdl);
    float cx, cy, cz;
    baseMdl->getCenter(cx, cy, cz);
    so->pos.x = -cx;
    so->pos.y = -cy;
    so->pos.z = -cz;
    so->scl = 0.01f;
    objs.push_back(so);
    break;
  }

  case '0': {
    printf("Ruta del modelo OBJ: ");
    fflush(stdout);
    char path[512];
    if (fgets(path, sizeof(path), stdin)) {
      char *nl = strchr(path, '\n');
      if (nl) *nl = '\0';
      if (path[0] == '\0') break;

      Model *m = new Model();
      if (m->load(path)) {
        SceneObj *so = new SceneObj({0, 0, 0}, pal[pi], m);
        float cx, cy, cz;
        m->getCenter(cx, cy, cz);
        so->pos.x = -cx;
        so->pos.y = -cy;
        so->pos.z = -cz;
        objs.push_back(so);
        printf("Modelo cargado: %s\n", path);
      } else {
        printf("Error al cargar: %s\n", path);
        delete m;
      }
    }
    break;
  }

  case 9:
    if (!objs.empty()) {
      if (sel_idx >= 0)
        objs[sel_idx]->sel = false;
      sel_idx = (sel_idx + 1) % (int)objs.size();
      objs[sel_idx]->sel = true;
    }
    break;

  case 'c':
  case 'C':
    pi = (pi + 1) % (int)pal.size();
    if (s)
      s->col = pal[pi];
    break;

  case 127:
  case 8:
    if (sel_idx >= 0) {
      delete objs[sel_idx];
      objs.erase(objs.begin() + sel_idx);
      sel_idx = -1;
    }
    break;

  case 'i': if (s) s->pos.y += step; break;
  case 'k': if (s) s->pos.y -= step; break;
  case 'j': if (s) s->pos.x -= step; break;
  case 'l': if (s) s->pos.x += step; break;
  case 'n': if (s) s->pos.z += step; break;
  case 'm': if (s) s->pos.z -= step; break;

  case 'u': if (s) s->rot.y = fmodf(s->rot.y + 3, 360); break;
  case 'o': if (s) s->rot.y = fmodf(s->rot.y - 3 + 360, 360); break;
  case 'p': if (s) s->rot.z = fmodf(s->rot.z + 3, 360); break;
  case 'r': if (s) s->rot.z = fmodf(s->rot.z - 3 + 360, 360); break;
  case 'g': if (s) s->rot.x = fmodf(s->rot.x + 3, 360); break;
  case 'h': if (s) s->rot.x = fmodf(s->rot.x - 3 + 360, 360); break;

  case '+':
  case '=':
    if (s) s->scl *= 1.1f;
    break;
  case '-':
    if (s) s->scl /= 1.1f;
    break;

  case 'v':
    c_fov = fminf(c_fov + 5, 120);
    set_proj();
    break;
  case 'b':
    c_fov = fmaxf(c_fov - 5, 10);
    set_proj();
    break;

  case 'e':
  case 'E':
    save_scene();
    printf("Escena guardada en 'scene%d'\n", save_c - 1);
    fflush(stdout);
    break;

  case 'q':
  case 'Q':
    if (s && s->mdl) {
      SceneObj *clone = new SceneObj(s->pos, s->col, s->mdl);
      clone->rot = s->rot;
      clone->scl = s->scl;
      objs.push_back(clone);
    }
    break;

  default:
    printf("tecla %d no activa\n", (int)key);
  }
  glutPostRedisplay();
}

void mouse_btn(int btn, int state, int x, int y) {
  cam.mouse(btn, state, x, y);
  if (btn == 3) {
    c_fov = fmaxf(10.f, c_fov - 5);
    set_proj();
    glutPostRedisplay();
  }
  if (btn == 4) {
    c_fov = fminf(120.f, c_fov + 5);
    set_proj();
    glutPostRedisplay();
  }
}

void mouse_move(int x, int y) {
  cam.motion(x, y);
  glutPostRedisplay();
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(W, H);
  glutCreateWindow("Editor 3D – OBJ Viewer");

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  float amb[] = {.3f, .3f, .3f, 1};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
  GLfloat dif[] = {0.8f, 0.8f, 0.8f, 1.0f};
  GLfloat spe[] = {0.2f, 0.2f, 0.2f, 1.0f};
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
  glLightfv(GL_LIGHT0, GL_SPECULAR, spe);

  glClearColor(0, 0, 0, 1);
  glPointSize(2);
  set_proj();

  cam.lookAt(0, 0, 0);
  cam.setDist(10);

  baseMdl = new Model();
  if (!baseMdl->load("../shining/Scan.obj", "../shining/Scan.mtl")) {
    fprintf(stderr, "No se pudo cargar ../shining/Scan.obj\n");
    delete baseMdl;
    baseMdl = nullptr;
  } else {
    printf("Modelo base cargado: ../shining/Scan.obj\n");
  }

  if (argc > 1)
    load_scene(argv[1]);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse_btn);
  glutMotionFunc(mouse_move);

  glutMainLoop();
  return 0;
}

void save_scene() {
  ofstream file("scene" + to_string(save_c++));
  for (auto *o : objs) {
    file << "OBJ " << o->pos.x << " " << o->pos.y << " " << o->pos.z << " "
         << o->rot.x << " " << o->rot.y << " " << o->rot.z << " " << o->scl
         << " " << o->col.r << " " << o->col.g << " " << o->col.b << "\n";
  }
}

void load_scene(string filename) {
  ifstream file(filename);
  if (!file.is_open()) {
    printf("No se pudo abrir: %s\n", filename.c_str());
    return;
  }

  for (auto *o : objs)
    delete o;
  objs.clear();
  sel_idx = -1;

  string type_str;
  float px, py, pz, rx, ry, rz, scl, r, g, b;

  while (file >> type_str >> px >> py >> pz >> rx >> ry >> rz >> scl >> r >>
         g >> b) {
    if (type_str != "OBJ") continue;
    if (!baseMdl) continue;

    SceneObj *o = new SceneObj({px, py, pz}, {r, g, b}, baseMdl);
    o->rot = {rx, ry, rz};
    o->scl = scl;
    objs.push_back(o);
  }
}

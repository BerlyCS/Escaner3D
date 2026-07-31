#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "visor.h"

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

enum obj_t { CUBE, SPHERE, TORUS, TEAPOT, OBJMESH };

struct obj {
  obj_t type;
  vec3 pos, rot;
  float scl = 1.f;
  col3 col;
  bool sel = false;
  bool wire = false;
  string objPath, mtlPath;
  Model* model = nullptr;
  obj(obj_t t, vec3 p, col3 c) : type(t), pos(p), col(c) {}
  ~obj() { if (model) { delete model; model = nullptr; } }
  bool loadMesh() {
    if (type != OBJMESH) return false;
    if (model) { delete model; model = nullptr; }
    model = new Model();
    return model->load(objPath.c_str(),
                       mtlPath.empty() ? nullptr : mtlPath.c_str());
  }
};

vector<obj *> objs;
int sel_idx = -1;

float c_az = 45, c_el = 25, c_dist = 10, c_fov = 60;
int W = 1000, H = 1000;
int mx0 = -1, my0 = -1;
bool dragging = false;
int save_c = 0;

vector<col3> pal = {{1, .3f, .3f}, {.3f, 1, .3f}, {.3f, .5f, 1},  {1, 1, .3f},
                    {1, .5f, 0},   {.9f, .3f, 1}, {0, .8f, .8f},  {1, .4f, .7f},
                    {.5f, 1, .5f}, {1, .7f, .3f}, {.4f, .4f, 1},  {.8f, 1, 0},
                    {1, .2f, .6f}, {0, .9f, .5f}, {.6f, .3f, .1f}};
int pi = 0;

vec3 eye() {
  float az = c_az * M_PI / 180, el = c_el * M_PI / 180;
  return {c_dist * cosf(el) * sinf(az), c_dist * sinf(el),
          c_dist * cosf(el) * cosf(az)};
}

vec3 c_target = {0, 0, 0};

void set_proj() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(c_fov, (float)W / H, 0.1f, 200.f);
  glMatrixMode(GL_MODELVIEW);
}

void set_cam() {
  glLoadIdentity();
  vec3 e = eye();
  gluLookAt(e.x + c_target.x, e.y + c_target.y, e.z + c_target.z, c_target.x,
            c_target.y, c_target.z, 0, 1, 0);
  GLfloat lp[] = {e.x, e.y, e.z, 1};
  glLightfv(GL_LIGHT0, GL_POSITION, lp);
}

void draw_grid() {
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
  glVertex3f(1.5f, 0, 0); // X rojo
  glColor3f(.2f, 1, .2f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 1.5f, 0); // Y verde
  glColor3f(.3f, .5f, 1);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 1.5f); // Z azul
  glEnd();
  glLineWidth(1.f);
}

void draw_obj(obj *o) {
  glPushMatrix();
  glTranslatef(o->pos.x, o->pos.y, o->pos.z);
  glRotatef(o->rot.x, 1, 0, 0);
  glRotatef(o->rot.y, 0, 1, 0);
  glRotatef(o->rot.z, 0, 0, 1);
  glScalef(o->scl, o->scl, o->scl);

  glColor3f(o->col.r, o->col.g, o->col.b);

  if (o->wire) {
    glDisable(GL_LIGHTING);
    if (o->sel)
      glColor3f(1, 1, .2f);
    switch (o->type) {
    case CUBE:
      glutWireCube(1);
      break;
    case SPHERE:
      glutWireSphere(.65f, 20, 20);
      break;
    case TORUS:
      glutWireTorus(.22f, .55f, 14, 28);
      break;
    case TEAPOT:
      glutWireTeapot(.65f);
      break;
    case OBJMESH:
      if (o->model) {
        float cx, cy, cz;
        o->model->getCenter(cx, cy, cz);
        glTranslatef(-cx, -cy, -cz);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        o->model->render();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
      break;
    }
  } else {
    glEnable(GL_LIGHTING);
    switch (o->type) {
    case CUBE:
      glutSolidCube(1);
      break;
    case SPHERE:
      glutSolidSphere(.63f, 20, 20);
      break;
    case TORUS:
      glutSolidTorus(.22f, .55f, 14, 28);
      break;
    case TEAPOT:
      glutSolidTeapot(.63f);
      break;
    case OBJMESH:
      if (o->model) {
        float cx, cy, cz;
        o->model->getCenter(cx, cy, cz);
        glTranslatef(-cx, -cy, -cz);
        glDisable(GL_COLOR_MATERIAL);
        o->model->render();
        glEnable(GL_COLOR_MATERIAL);
      }
      break;
    }
    // overline , selected object
    if (o->sel) {
      glDisable(GL_LIGHTING);
      glColor3f(.8f, .8f, 0.1f);
      switch (o->type) {
      case CUBE:
        glutWireCube(1.01f);
        break;
      case SPHERE:
        glutWireSphere(.64f, 20, 20);
        break;
      case TORUS:
        glutWireTorus(.23f, .57f, 14, 28);
        break;
      case TEAPOT:
        glutWireTeapot(.64f);
        break;
      case OBJMESH:
        if (o->model) {
          float cx, cy, cz;
          o->model->getCenter(cx, cy, cz);
          glTranslatef(-cx, -cy, -cz);
          glEnable(GL_POLYGON_OFFSET_LINE);
          glPolygonOffset(-1, -1);
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          o->model->render();
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glDisable(GL_POLYGON_OFFSET_LINE);
        }
        break;
      }
    }
  }
  glPopMatrix();
}

// texto
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

  // barra inferior
  glColor3f(.06f, .06f, .08f);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(W, 0);
  glVertex2f(W, 50);
  glVertex2f(0, 50);
  glEnd();
  draw_str(8, 34,
           "1:Cube  2:Sphere  3:Torus  4:Teapot  5:LoadOBJ  Tab:select  "
           "W:wire  Del:delete WASDTY:moveCam",
           {.75f, .75f, .75f});
  draw_str(8, 14,
           ("IJKLNM:move  UO:rotY  PR:rotZ  +/-:scale  VB:FOV=" +
            to_string((int)c_fov) +
            "  Scroll:zoom  E:save  Q:clone  F:wire  C:color  Del:del")
               .data(),
           {.5f, .5f, .5f});

  if (sel_idx >= 0 && sel_idx < (int)objs.size()) {
    obj *o = objs[sel_idx];
    const char *tnames[] = {"Cubo", "Esfera", "Toro", "Tetera", "Malla"};
    char buf[200];
    sprintf(buf,
            "obj[%d] %s  |  pos(%.2f, %.2f, %.2f)  |  rot(%.0f, %.0f, %.0f)  | "
            " scl %.2f  |  %.0f",
            sel_idx, tnames[o->type], o->pos.x, o->pos.y, o->pos.z, o->rot.x,
            o->rot.y, o->rot.z, o->scl, c_fov);
    glColor3f(.06f, .06f, .1f);
    glBegin(GL_QUADS);
    glVertex2f(0, H - 22);
    glVertex2f(W, H - 22);
    glVertex2f(W, H);
    glVertex2f(0, H);
    glEnd();
    draw_str(8, H - 16, buf, {1, 1, .5f});
  }

  // contador de objetos (esquina inferior derecha)
  string cnt = to_string(objs.size()) + " objetos";
  draw_str(W - 100, 34, cnt.data(), {.5f, .5f, .5f});

  string wf_str = "";
  if (sel_idx >= 0 && sel_idx < (int)objs.size())
    wf_str = objs[sel_idx]->wire ? "WIREFRAME" : "SOLID";
  draw_str(W - 100, 14, wf_str,
           (wf_str == "WIREFRAME") ? col3{1, .8f, .3f} : col3{.3f, 1, .5f});

  glEnable(GL_DEPTH_TEST);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  set_cam();
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

void keyboard(unsigned char key, int x, int y) {
  obj *s =
      (sel_idx >= 0 && sel_idx < (int)objs.size()) ? objs[sel_idx] : nullptr;
  const float step = 0.2f;

  switch (key) {
  case 27:
    exit(0);

  case '1':
    objs.push_back(new obj(CUBE, {0, .5f, 0}, pal[pi]));
    break;

  case '2':
    objs.push_back(new obj(SPHERE, {0, .5f, 0}, pal[pi]));
    break;

  case '3':
    objs.push_back(new obj(TORUS, {0, .5f, 0}, pal[pi]));
    break;

  case '4':
    objs.push_back(new obj(TEAPOT, {0, .5f, 0}, pal[pi]));
    break;

  case '5': {
    string op;
    printf("Ruta del archivo .obj: ");
    fflush(stdout);
    getline(cin, op);
    if (op.empty()) break;

    string mp;
    size_t dot = op.rfind('.');
    if (dot != string::npos)
      mp = op.substr(0, dot) + ".mtl";
    else
      mp = op + ".mtl";

    ifstream test(mp);
    bool mtlExiste = test.good();
    test.close();

    if (mtlExiste) {
      printf("Se encontro %s. Usar MTL? (s/n): ", mp.c_str());
      fflush(stdout);
      string resp;
      getline(cin, resp);
      if (resp != "s" && resp != "S" && resp != "si" && resp != "SI" && resp != "y" && resp != "Y")
        mp.clear();
    } else {
      printf("No se encontro %s, cargando sin texturas.\n", mp.c_str());
      fflush(stdout);
      mp.clear();
    }

    obj* o = new obj(OBJMESH, {0, 0, 0}, pal[pi]);
    o->objPath = op;
    o->mtlPath = mp;
    if (!o->loadMesh()) {
      printf("Error al cargar el modelo\n");
      fflush(stdout);
      delete o;
    } else {
      float rad = o->model->getRad();
      if (rad > 0.001f) o->scl = 1.0f / rad;
      objs.push_back(o);
      printf("Modelo cargado: %s  (radio=%.2f, escala=%.2f)\n", op.c_str(), rad, o->scl);
      fflush(stdout);
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

  // wireframe toggle
  case 'f':
  case 'F':
    if (sel_idx >= 0) {
      objs[sel_idx]->wire = !objs[sel_idx]->wire;
    }
    break;

  // color
  case 'c':
  case 'C':
    pi = (pi + 1) % pal.size();
    if (s)
      s->col = pal[pi];
    break;

  // delete
  case 127:
  case 8:
    if (sel_idx >= 0) {
      delete objs[sel_idx];
      objs.erase(objs.begin() + sel_idx);
      sel_idx = -1;
    }
    break;

  // translate
  case 'i':
    if (s)
      s->pos.y += step;
    break;
  case 'k':
    if (s)
      s->pos.y -= step;
    break;
  case 'j':
    if (s)
      s->pos.x -= step;
    break;
  case 'l':
    if (s)
      s->pos.x += step;
    break;
  case 'n':
    if (s)
      s->pos.z += step;
    break;
  case 'm':
    if (s)
      s->pos.z -= step;
    break;

  // rotation
  case 'u':
    if (s) {
      s->rot.y = fmodf(s->rot.y + 3, 360);
    }
    break;
  case 'o':
    if (s) {
      s->rot.y = fmodf(s->rot.y - 3 + 360, 360);
    }
    break;
  case 'p':
    if (s) {
      s->rot.z = fmodf(s->rot.z + 3, 360);
    }
    break;
  case 'r':
    if (s) {
      s->rot.z = fmodf(s->rot.z - 3 + 360, 360);
    }
    break;
  case 'g':
    if (s) {
      s->rot.x = fmodf(s->rot.x + 3, 360);
    }
    break;

  case 'h':
    if (s) {
      s->rot.x = fmodf(s->rot.x - 3 + 360, 360);
    }
    break;

  // scale
  case '+':
  case '=':
    if (s)
      s->scl *= 1.1f;
    break;
  case '-':
    if (s)
      s->scl /= 1.1f;
    break;

  // fov
  case 'v':
    c_fov = fminf(c_fov + 5, 120);
    set_proj();
    break;
  case 'b':
    c_fov = fmaxf(c_fov - 5, 10);
    set_proj();
    break;

  // zoom
  case 'z':
    c_dist = fmaxf(1.f, c_dist - .5f);
    break;
  case 'x':
    c_dist += .5f;
    break;

  case 'e':
  case 'E':
    save_scene();
    printf("Escena guardada en 'scene%d'\n", save_c - 1);
    fflush(stdout);
    break;

  // clone
  case 'q':
  case 'Q':
    if (s) {
      obj *clone = new obj(s->type, s->pos, s->col);
      clone->rot = s->rot;
      clone->scl = s->scl;
      clone->wire = s->wire;
      if (s->type == OBJMESH) {
        clone->objPath = s->objPath;
        clone->mtlPath = s->mtlPath;
        clone->loadMesh();
      }
      objs.push_back(clone);
      s = clone;
    }
    break;

    // camera movement
  case 'w':
  case 'W':
    c_target.y += step;
    break;
  case 's':
  case 'S':
    c_target.y -= step;
    break;
  case 'a':
  case 'A':
    c_target.x -= step;
    break;
  case 'd':
  case 'D':
    c_target.x += step;
    break;

  case 't':
  case 'T':
    c_target.z += step;
    break;
  case 'y':
  case 'Y':
    c_target.z -= step;
    break;

  default:
    printf("tecla %d no activa\n", (int)key);
  }
  glutPostRedisplay();
}

// mouse
void mouse_btn(int btn, int state, int x, int y) {
  if (btn == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      dragging = true;
      mx0 = x;
      my0 = y;
    } else {
      dragging = false;
    }
  }
  if (btn == 3) {
    c_dist = fmaxf(1.f, c_dist - .5f);
    glutPostRedisplay();
  } // scroll up
  if (btn == 4) {
    c_dist += .5f;
    glutPostRedisplay();
  } // scroll down
}

void mouse_move(int x, int y) {
  if (dragging) {
    c_az -= (x - mx0);
    c_el += (y - my0);
    if (c_el > 89.9)
      c_el = 89.9;
    if (c_el < -89.9)
      c_el = -89.9;
    mx0 = x;
    my0 = y;
    glutPostRedisplay();
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(W, H);
  glutCreateWindow("Editor 3D – OpenGL");

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  GLfloat amb[] = {.3f, .3f, .3f, 1};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
  GLfloat la[] = {0.4f, 0.4f, 0.4f, 1.0f};
  GLfloat dif[] = {0.8f, 0.8f, 0.8f, 1.0f};
  GLfloat spe[] = {0.2f, 0.2f, 0.2f, 1.0f};
  glLightfv(GL_LIGHT0, GL_AMBIENT, la);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
  glLightfv(GL_LIGHT0, GL_SPECULAR, spe);
  glClearColor(0, 0, 0, 1);
  glPointSize(2);
  set_proj();

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
    switch (o->type) {
    case CUBE:
      file << "CUBE ";
      break;
    case SPHERE:
      file << "SPHERE ";
      break;
    case TORUS:
      file << "TORUS ";
      break;
    case TEAPOT:
      file << "TEAPOT ";
      break;
    case OBJMESH:
      file << "OBJMESH ";
      break;
    }

    file << o->pos.x << " " << o->pos.y << " " << o->pos.z << " " << o->rot.x
         << " " << o->rot.y << " " << o->rot.z << " " << o->scl << " "
         << o->col.r << " " << o->col.g << " " << o->col.b << " " << o->wire;
    file << "\n";
    if (o->type == OBJMESH) {
      file << o->objPath << "\n";
      file << o->mtlPath << "\n";
    }
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
  int ws;

  while (file >> type_str >> px >> py >> pz >> rx >> ry >> rz >> scl >> r >>
         g >> b >> ws) {
    obj_t type;
    if (type_str == "CUBE")
      type = CUBE;
    else if (type_str == "SPHERE")
      type = SPHERE;
    else if (type_str == "TORUS")
      type = TORUS;
    else if (type_str == "TEAPOT")
      type = TEAPOT;
    else if (type_str == "OBJMESH")
      type = OBJMESH;
    else {
      printf("Tipo desconocido: %s\n", type_str.c_str());
      continue;
    }

    obj *o = new obj(type, {px, py, pz}, {r, g, b});
    o->rot = {rx, ry, rz};
    o->scl = scl;
    o->wire = ws;

    if (type == OBJMESH) {
      file.ignore(10000, '\n');
      getline(file, o->objPath);
      getline(file, o->mtlPath);
      o->loadMesh();
    }

    objs.push_back(o);
  }
}

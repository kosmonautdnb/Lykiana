/*
  MIT License
  
  Copyright (c) 2025 Stefan Mader
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include "O_GLTF.HPP"
#include "GL.H"
#define CGLTF_IMPLEMENTATION
#include "CGLTFA.HPP"
#include "IMAGE.HPP"
#include <math.h>
#include "SET.HPP"

Set<unsigned int> clothingTextures;

/**
* This function is used to make the skin of a vroid studio character more natural looking.
* It is a texture callback for GLTFA_File::load();
*
* @param img the input texture provided by the gltf loader.
* @return an OpenGL texture handle.
* @example  GLTFA_File *r = new GLTFA_File(); r->textureCallback = animeCharacterTexture;
*/
static unsigned int animeCharacterTexture(RGBAImage *img) {
  bool withClothing = false;
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width; x++) {
      unsigned int rgba = img->data[x+y*img->width];
      int r = rgba & 255;
      int g = (rgba>>8) & 255;
      int b = (rgba>>16) & 255;
      int a = (rgba>>24) & 255;
      int grey = (r + g + b)/3;
      int rs = r-grey;
      int gs = g-grey;
      int bs = b-grey;
      bool isGrey = rs*rs+gs*gs+bs*bs<64*64;
      if (isGrey && grey>192) {
        r = 255; // skin tone
        g = 200;
        b = 175;
      } else if (isGrey && grey < 5) {
        a = 128;
        withClothing = true;
      } else if (r < 100 && g > 100 && b > 100) {
        a = 100;
        withClothing = true;
      }
      img->data[x+y*img->width] = r | (g<<8) | (b<<16) | (a<<24);
    }
  }
  unsigned int r;
  glGenTextures(1, &r);
  glBindTexture(GL_TEXTURE_2D, r);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img->width,img->height,0,GL_RGBA,GL_UNSIGNED_BYTE,img->data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (withClothing) clothingTextures.insert(r);
  return r;
}

/**
* This function gets a node with the given name from the gltf. It's case insensitive.
*
* @param name The case insensitive name of the node/bone of the gltf.
* @param a The gltf file to look through for this node.
* @return An unsigned int, which is the ID of the searched node/bone or 0 if not found.
*/
static unsigned int getGLTFNode(const String &name, const GLTFA_File *a) {
  Array<unsigned int> keys = a->gltfNodes.keys();
  for (int i = 0; i < keys.size(); i++) {
    GLTFA_Node *n = a->gltfNodes[keys[i]];
    if (toLower(n->name) == toLower(name)) {
      return keys[i];
    }
  }
  return 0;
}

/**
* This function loads a gltf from a *.glb file to be used as a game character.
*
* @param name The file name of the *.glb to load.
* @param objectId The objectId for the gltfs later identification.
* @example GLTFA_File *gltf = loadGLTF_Character("girl1.glb");
*/
class GLTFA_File *loadGLTF_Character(const char *name, unsigned int objectId) {
  GLTFA_File *r = new GLTFA_File();
  r->textureCallback = animeCharacterTexture;
  r->gltfId = objectId;
  r->load(name, 0.5); // texture scale
  return r;
}

/**
* This function animates a gltf which is used as a game character. Including bone setups.
*
* @param gltf the gltf file which should be animated.
* @param frame The frame number of the animation.
* @example animateGLTF_Character(gltf, 8.0);
*/
void animateGLTF_Character(class GLTFA_File *gltf, double frame) {
  gltf->applyAnimation(frame/24.f, gltf->gltfAnimations[0]);
}

/**
* This function renders a gltf which is used as a game character. Including bone setups.
*
* @param gltf the gltf file which should be rendered.
* @param color A color for the current player state.
* @example renderGLTF_Character(gltf);
*/
void renderGLTF_Character(class GLTFA_File *gltf, const Vector &color) {
  unsigned int boneId = 0;
  switch(gltf->gltfId) {
  case GLTF_OBJECTID_MAINCHARACTER_NORMAL: {
    const static unsigned int _boneId = getGLTFNode("J_BIP_C_SPINE",gltf);
    boneId = _boneId;
  } break;
  }
  const Vector center = gltf->getMatrix(boneId,0).getColumn(3);

  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
  float specular[4] = {1-(float)color.x,1-(float)color.y,1-(float)color.z,1}; glLightfv(GL_LIGHT0, GL_SPECULAR,specular);
  float diffuse[4] = {1-(float)color.x,1-(float)color.y,1-(float)color.z,1}; glLightfv(GL_LIGHT0, GL_DIFFUSE,diffuse);
  float am[4]={(float)color.x*0.5f+0.5f,(float)color.y*0.5f+0.5f,(float)color.z*0.5f+0.5f,0}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT,am);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,specular);
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,20.0);
  glColor4f(1,1,1,1);

  glPushMatrix();
  glTranslatef(-center.x,-center.y,-center.z);
  gltf->drawScene(gltf->defaultScene);
  glPopMatrix();

  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHTING);
}

/**
* This function logs all nodes and some more data via printf (it closes OpenGL first) and exits the program.
*
* @param gltf A GLTF object.
* @example logGLTFNodes(girl);
*/
void logGLTFNodes(class GLTFA_File *gltf) {
  glDone(); 
  GLTFA_Scene *s = gltf->gltfScenes[gltf->defaultScene];
  {for (int i = 0; i < s->nodes.size(); i++) {
    GLTFA_Node *n = s->nodes[i];
    printf("%s\n",n->name.c_str());
  }}
  printf("-----\n");
  {Array<unsigned int> keys = gltf->gltfNodes.keys();
    for (int i = 0; i < keys.size(); i++) {
    GLTFA_Node *n = gltf->gltfNodes[keys[i]];
    printf("%s\n",n->name.c_str());
  }}
  printf("-----\n");
  {for (int i = 0; i < gltf->gltfAnimations[0]->channels.size(); i++) {
    GLTFA_AnimationChannel *c = &gltf->gltfAnimations[0]->channels[i];
    GLTFA_Node *n = gltf->gltfNodes.has(c->node_id) ? gltf->gltfNodes[c->node_id] : NULL;
    if (n != NULL) printf("%s\n",n->name.c_str());
  }}
  exit(0);
}

/**
* This function shows the triangle count of the entire gltf via printf. It closes OpenGL before and exits the program.
*
* @param gltf A GLTF object.
* @example logGLTFFaceCount(girl);
*/
void logGLTFFaceCount(class GLTFA_File *gltf) {
  glDone(); 
  int polyCount = 0;
  Array<unsigned int> keys = gltf->gltfMeshes.keys();
  for (int i = 0; i < keys.size(); i++) {
    GLTFA_Mesh *m = gltf->gltfMeshes[keys[i]];
    for (int j = 0; j < m->primitives.size(); j++) {
      polyCount += m->primitives[j]->indices.size();
    }
  }
  printf("%d\n",polyCount); 
  exit(0);
}

/**
* This function exchanges the clothing of the character if it's not already that type.
*
* @param clothingType The type of clothing to apply.
* @example updateClothings(0);
*/
void updateClothings(unsigned int clothingType) {
  static int lastClothingType = -1;
  if (lastClothingType == clothingType) return;
  lastClothingType = clothingType;

  //Vector portalColors[4];
  //portalColors[0] = Vector(1,0,0);
  //portalColors[1] = Vector(0,1,1);
  //portalColors[2] = Vector(1,0.5,0);
  //portalColors[3] = Vector(1,0,0.5);

  Array<unsigned int> textureIds = clothingTextures.array();
  for (int i = 0; i < textureIds.size(); i++) {
    unsigned int textureId = textureIds[i];
    unsigned int *p = glGetTexturePointer(textureId);
    int w = glGetTextureWidth(textureId);
    int h = glGetTextureHeight(textureId);
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        unsigned int rgba = p[x+y*w];
        int r = rgba & 255;
        int g = (rgba>>8) & 255;
        int b = (rgba>>16) & 255;
        int a = (rgba>>24) & 255;
        if (a == 128) {
          bool skinColored = (y >= h * 58 / 100) && (y <= h * 65 / 100);
          if (skinColored) {
            r = 255; // skin tone
            g = 200;
            b = 175;
          } else {
            switch(clothingType & 3) {
            case 0: {
              r = 255;
              g = 20;
              b = 20;
              int kx = x*40/w;
              int ky = y*40/h;
              if ((kx+ky)&1) {
                r = 20;
                g = 255;
                b = 255;
              }
            } break;
            case 1: {
              r = 20;
              g = 255;
              b = 255;
            } break;
            case 2: {
              r = 255;
              g = 20;
              b = 20;
              int kx = (x+y)*30/w;
              if (kx&1) {
                r = 255;
                g = 128;
                b = 20;
              }
            } break;
            case 3: {
              r = 255;
              g = 20;
              b = 128;
            } break;
            }
          }
        }
        if (a == 100) {
          switch(clothingType & 3) {
          case 0: {
            r = 255;
            g = 20;
            b = 20;
          } break;
          case 1: {
            r = 20;
            g = 255;
            b = 255;
          } break;
          case 2: {
            r = 255;
            g = 255;
            b = 255;
          } break;
          case 3: {
            r = 255;
            g = 20;
            b = 128;
          } break;
          }
        }
        p[x+y*w] = r | (g<<8) | (b<<16) | (a<<24);
      }
    }
  }
}

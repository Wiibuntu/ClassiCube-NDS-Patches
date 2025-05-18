#include "Graphics.h"
#include "_GraphicsBase.h"

#include <nds.h>
#include <nds/arm9/videoGL.h>

#define FP(x)        ((int)((x)*4096))
#define MAX_VERTS    4096

// A DS‐friendly textured‐vertex struct matching _GraphicsBase.h’s VertexTextured
typedef struct {
    int       x, y, z;    // 12.4 fixed‐point
    int       u, v;       // 12.4 fixed‐point
    cc_color  color;      // 0xRRGGBB
} VertexTextured;

static VertexTextured dsVertices[MAX_VERTS];
static int           dsVertCount = 0;

// DS’s matrix modes
static int matrix_modes[2]   = { DS_MAT_PROJECTION, DS_MAT_MODELVIEW };
static int matrix_position   = DS_MAT_PROJECTION;

// Called once at startup on DS/DSi
void Graphics_InitDS(void) {
    videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);

    glInit();
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);

    // Set up an orthographic projection matching the screen
    glMatrixMode(matrix_modes[0]);
    glLoadIdentity();
    glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);

    glMatrixMode(matrix_modes[1]);
    glLoadIdentity();
}

// Called at the start of every frame
void Graphics_BeginFrameDS(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    dsVertCount = 0;
}

// Change projection/modelview
void Graphics_SetMatrixDS(int mode) {
    if (mode < 0 || mode > 1) return;
    matrix_position = mode;
    glMatrixMode(matrix_modes[mode]);
}
void Graphics_LoadMatrixDS(const float *m) { glLoadMatrixf(m); }
void Graphics_MultMatrixDS(const float *m) { glMultMatrixf(m); }
void Graphics_PushMatrixDS(void)       { glPushMatrix(); }
void Graphics_PopMatrixDS(void)        { glPopMatrix(); }

// We ignore colour‐only verts on DS (always textured quads)
void Graphics_SetVertexFormatDS(VertexFormat fmt) {
    (void)fmt;
}

// Enqueue a single textured vertex into our batch
void Graphics_QueueTexturedVertexDS(float x, float y, float z,
                                    float u, float v, cc_color c) {
    if (dsVertCount >= MAX_VERTS) return;
    dsVertices[dsVertCount++] = (VertexTextured){
        FP(x), FP(y), FP(z), FP(u), FP(v), c
    };
}

// Draw all queued quads in one go
void Graphics_DrawBufferedDS(void) {
    if (!dsVertCount) return;
    glBegin(GL_QUADS);
    for (int i = 0; i < dsVertCount; i++) {
        VertexTextured v = dsVertices[i];
        // unpack 0xRRGGBB into 3x8‐bit
        glColor3b((v.color >> 16) & 0xFF,
                  (v.color >>  8) & 0xFF,
                   v.color        & 0xFF);
        glTexCoord2t16(v.u, v.v);
        glVertex3t16  (v.x, v.y, v.z);
    }
    glEnd();
    glFlush(0);
    swiWaitForVBlank();
    dsVertCount = 0;
}

// Upload or update a texture’s bitmap on DS
void Graphics_UpdateTextureDS(TextureID id, const BitmapCol *src) {
    int w = Graphics_GetTextureWidth(id);
    int h = Graphics_GetTextureHeight(id);
    glBindTexture(0, id);
    glTexImage2D(0, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, src);
}

// Called at the end of every frame (nothing extra needed on DS)
void Graphics_EndFrameDS(void) {
    // no-op
}

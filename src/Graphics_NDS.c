#include "Graphics.h"
#include "_GraphicsBase.h"

#include <nds.h>
#include <nds/arm9/videoGL.h>
#include <stdint.h> // for glFixed

#define FP(x)        ((int)((x)*4096))
#define MAX_VERTS    4096

// DS‐friendly textured vertex
typedef struct {
    int            x, y, z;    // 12.4 fixed‐point
    int            u, v;       // 12.4 fixed‐point
    unsigned int   color;      // 0xRRGGBB
} VertexTextured;

static VertexTextured dsVertices[MAX_VERTS];
static int           dsVertCount;

// Use DS GL’s projection/modelview enums directly
static int matrix_modes[2] = { GL_PROJECTION, GL_MODELVIEW };
static int matrix_position  = 0; // 0 = proj, 1 = modelview

// Initialize the DS 3D engine
void Graphics_InitDS(void) {
    videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);

    glInit();
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);

    // Orthographic projection
    glMatrixMode(matrix_modes[0]);
    glLoadIdentity();
    glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);

    glMatrixMode(matrix_modes[1]);
    glLoadIdentity();
}

// Start of frame – clear our batch (DS GL-lite doesn’t have glClear)
void Graphics_BeginFrameDS(void) {
    dsVertCount = 0;
}

// Matrix stack ops (load/mult aren’t exposed in DS-GL-lite)
void Graphics_SetMatrixDS(int mode) {
    if (mode < 0 || mode > 1) return;
    matrix_position = mode;
    glMatrixMode(matrix_modes[mode]);
}
void Graphics_LoadMatrixDS(const float *m) { /* no-op on DS */ }
void Graphics_MultMatrixDS(const float *m) { /* no-op on DS */ }
void Graphics_PushMatrixDS(void)       { glPushMatrix(); }
void Graphics_PopMatrixDS(void)        { glPopMatrix(1); }

// We don’t support colour-only paths on DS
void Graphics_SetVertexFormatDS(VertexFormat fmt) {
    (void)fmt;
}

// Enqueue one textured vertex
void Graphics_QueueTexturedVertexDS(float x, float y, float z,
                                    float u, float v, unsigned int c) {
    if (dsVertCount >= MAX_VERTS) return;
    dsVertices[dsVertCount++] = (VertexTextured){
        FP(x), FP(y), FP(z), FP(u), FP(v), c
    };
}

// Flush all queued quads
void Graphics_DrawBufferedDS(void) {
    if (!dsVertCount) return;

    glBegin(GL_QUADS);
    for (int i = 0; i < dsVertCount; i++) {
        VertexTextured v = dsVertices[i];
        // unpack 0xRRGGBB
        glColor3b((v.color >> 16) & 0xFF,
                  (v.color >>  8) & 0xFF,
                   v.color        & 0xFF);
        glTexCoord2t16(v.u, v.v);
        // pass fixed‐point xyz as a 3-vector
        {
            glFixed vec[3] = { v.x, v.y, v.z };
            glVertex3v16(vec);
        }
    }
    glEnd();
    glFlush(0);
    swiWaitForVBlank();

    dsVertCount = 0;
}

// Upload/update a texture on the DS
void Graphics_UpdateTextureDS(int id, const BitmapCol *src) {
    int w = Graphics_GetTextureWidth(id);
    int h = Graphics_GetTextureHeight(id);

    glBindTexture(0, id);
    glTexImage2D(0, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, src);
}

// End of frame (no extra teardown)
void Graphics_EndFrameDS(void) {
    // no-op
}

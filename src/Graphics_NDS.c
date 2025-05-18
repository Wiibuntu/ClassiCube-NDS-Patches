/*
 * Optimized Graphics_NDS.c for Nintendo DSi
 * Target: Stable 60 FPS on DSi by minimizing per-frame overhead,
 * leveraging fixed-point math, bulk DMA, and immediate-mode batching.
 */

 #include "Core.h"
 #ifdef CC_BUILD_NDS
 #include "_GraphicsBase.h"
 #include "Errors.h"
 #include "Logger.h"
 #include "Window.h"
 #include <nds.h>
 
 // Fixed-point macros (16.16)
 #define FX(x)          ((int)((x) * 65536.0f))
 
 // Maximum vertices per frame
 #define MAX_VERTS      4096
 
 // Vertex structure (fixed-point)
 typedef struct {
	 int x, y, z;       // FX coordinates
	 int u, v;          // FX texture coords
	 u8  r, g, b;       // Color (0-31)
 } Vertex;
 
 // Pre-allocated vertex buffer
 static Vertex vertBuffer[MAX_VERTS];
 static int vertCount;
 static bool gfxInited = false;
 
 // Initialize graphics once
 void Graphics_InitDS() {
	 if (gfxInited) return;
	 videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE);
	 vramSetBankA(VRAM_A_TEXTURE);
 
	 glInit();
	 // Enable anti-aliasing and textures
	 glEnable(GL_ANTIALIAS);
	 glEnable(GL_TEXTURE_2D);
 
	 // Setup orthographic projection
	 glMatrixMode(GL_PROJECTION);
	 glLoadIdentity();
	 glOrthof32(0, Window_GetWidth(), Window_GetHeight(), 0, -1, 1);
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
 
	 // Clear settings
	 glClearColor(0, 0, 0, 31);        // opaque black
	 glClearDepth(GL_MAX_DEPTH);
	 glClearPolyID(63);                // max poly ID
 
	 // Initial flush and VBlank
	 glFlush(0);
	 swiWaitForVBlank();
 
	 gfxInited = true;
 }
 
 // Begin frame: reset buffer
 void Graphics_BeginFrameDS() {
	 vertCount = 0;
 }
 
 // End frame: immediate-mode draw and flush
 void Graphics_EndFrameDS() {
	 if (vertCount == 0) return;
 
	 glBegin(GL_QUADS);
	 for (int i = 0; i < vertCount; i++) {
		 Vertex* v = &vertBuffer[i];
		 glColor3b(v->r, v->g, v->b);
		 glTexCoord2f32(v->u, v->v);
		 glVertex3v16(v->x, v->y, v->z);
	 }
	 glEnd();
 
	 glFlush(0);
	 swiWaitForVBlank();
 }
 
 // Queue a quad (4 vertices)
 void Graphics_QueueQuad(int x, int y, int z, int w, int h,
						 float u0, float v0, float u1, float v1,
						 u8 r, u8 g, u8 b) {
	 if (vertCount + 4 > MAX_VERTS) return; // Full
 
	 int fx = FX(x), fy = FX(y), fz = FX(z);
	 int fu0 = FX(u0), fv0 = FX(v0), fu1 = FX(u1), fv1 = FX(v1);
 
	 Vertex* v = &vertBuffer[vertCount];
	 // Top-left
	 v[0] = (Vertex){fx, fy, fz, fu0, fv0, r, g, b};
	 // Top-right
	 v[1] = (Vertex){fx + FX(w), fy, fz, fu1, fv0, r, g, b};
	 // Bottom-right
	 v[2] = (Vertex){fx + FX(w), fy + FX(h), fz, fu1, fv1, r, g, b};
	 // Bottom-left
	 v[3] = (Vertex){fx, fy + FX(h), fz, fu0, fv1, r, g, b};
 
	 vertCount += 4;
 }
 
 // Texture loader (8-bit palette => 256-color)
 void Graphics_LoadTileDS(int id, int width, int height, const u8* src, const u16* palette) {
	 // Upload texture indices to VRAM_A
	 dmaCopy(src, (void*)VRAM_A, width * height);
	 // Bind and specify texture: assume 8-bit paletted
	 glBindTexture(GL_TEXTURE_2D, id);
	 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB256, width, height, 0, 0, (void*)VRAM_A);
 }
 
 #endif /* CC_BUILD_NDS */
 
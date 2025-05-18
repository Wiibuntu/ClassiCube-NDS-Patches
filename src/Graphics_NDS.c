/*
 * Optimized Graphics_NDS.c for Nintendo DSi
 * Target: Stable 60 FPS on DSi by minimizing per-frame overhead,
 * leveraging fixed-point math, bulk DMA, and pre-allocated buffers.
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
 #define FX_TO_INT(x)   ((x) >> 16)
 
 // Maximum vertices per frame
 #define MAX_VERTS      4096
 
 // Vertex structure (fixed-point)
 typedef struct {
	 int x, y, z;       // FX coordinates
	 int u, v;          // FX texture coords
	 u8  r, g, b, a;    // Color
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
	 glEnable(GL_TEXTURE_2D);
	 glEnable(GL_ANTIALIAS);
	 glClearColor(0, 0, 0, 0);
 
	 glSetClientState(GL_VERTEX_ARRAY);
	 glSetClientState(GL_TEXTURE_COORD_ARRAY);
	 glSetClientState(GL_COLOR_ARRAY);
	 
	 glMatrixMode(GL_PROJECTION);
	 glLoadIdentity();
	 glOrthof(FX(0), FX(Window_GetWidth()), FX(Window_GetHeight()), FX(0), FX(-1), FX(1));
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
 
	 glClear(GL_COLOR_BUFFER_BIT);
	 glFlush(0);
	 swiWaitForVBlank();
 
	 gfxInited = true;
 }
 
 // Begin frame: reset buffer and clear
 void Graphics_BeginFrameDS() {
	 vertCount = 0;
	 glClear(GL_COLOR_BUFFER_BIT);
 }
 
 // End frame: push vertices and flush
 void Graphics_EndFrameDS() {
	 if (vertCount == 0) return;
 
	 // Setup pointers
	 glVertexPointer(3, GL_FIXED, sizeof(Vertex), &vertBuffer[0].x);
	 glTexCoordPointer(2, GL_FIXED, sizeof(Vertex), &vertBuffer[0].u);
	 glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vertBuffer[0].r);
 
	 // Issue draw in one call
	 glDrawArrays(GL_QUADS, 0, vertCount);
 
	 glFlush(0);            // Flush FIFO (batch DMA)
	 swiWaitForVBlank();     // Wait for vertical blank
 }
 
 // Queue a quad (4 vertices)
 void Graphics_QueueQuad(int x, int y, int z, int w, int h,
						 float u0, float v0, float u1, float v1,
						 u8 r, u8 g, u8 b, u8 a) {
	 if (vertCount + 4 > MAX_VERTS) return; // Full
 
	 int fx = FX(x), fy = FX(y), fz = FX(z);
	 int fu0 = FX(u0), fv0 = FX(v0), fu1 = FX(u1), fv1 = FX(v1);
 
	 Vertex* v = &vertBuffer[vertCount];
	 // Top-left
	 v[0].x = fx;        v[0].y = fy;        v[0].z = fz;
	 v[0].u = fu0;       v[0].v = fv0;
	 v[0].r = r;         v[0].g = g;         v[0].b = b;     v[0].a = a;
	 
	 // Top-right
	 v[1].x = fx + FX(w); v[1].y = fy;        v[1].z = fz;
	 v[1].u = fu1;        v[1].v = fv0;
	 v[1].r = r;          v[1].g = g;          v[1].b = b;    v[1].a = a;
 
	 // Bottom-right
	 v[2].x = fx + FX(w); v[2].y = fy + FX(h); v[2].z = fz;
	 v[2].u = fu1;        v[2].v = fv1;
	 v[2].r = r;          v[2].g = g;          v[2].b = b;    v[2].a = a;
 
	 // Bottom-left
	 v[3].x = fx;         v[3].y = fy + FX(h); v[3].z = fz;
	 v[3].u = fu0;        v[3].v = fv1;
	 v[3].r = r;          v[3].g = g;          v[3].b = b;    v[3].a = a;
 
	 vertCount += 4;
 }
 
 // Simple texture loader (8-bit palette -> 16-bit)
 void Graphics_LoadTileDS(int id, int width, int height, const u8* src, const u16* palette) {
	 int texSize = width * height * 2; // bytes
	 void* vram = (void*)(VRAM_A);      
	 dmaCopy(src, vram, width * height); // copy raw pixel indices
	 dmaCopy(palette, vram + 0x10000, 512); // palette
	 
	 glBindTexture(GL_TEXTURE_2D, id);
	 glTexImage2D(GL_TEXTURE_2D, 0, GL_TEXTURE_8, width, height,
				  0, GL_PALETTE, GL_UNSIGNED_BYTE, vram);
 }
 
 #endif /* CC_BUILD_NDS */
 
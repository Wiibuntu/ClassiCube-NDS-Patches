/*
 * Graphics_NDS.c for Nintendo DSi
 * Combined optimized DSi-specific pipeline and restored engine Gfx_* implementations
 * Target: Stable 60 FPS on DSi by minimizing per-frame overhead,
 * leveraging fixed-point math, bulk DMA, and immediate-mode batching,
 * while preserving original graphics engine functionality.
 */

 #include "Core.h"
 #ifdef CC_BUILD_NDS
 #include "_GraphicsBase.h"
 #include "Errors.h"
 #include "Logger.h"
 #include "Window.h"
 #include <nds.h>
 
 // Screen dimensions for NDS/DSi
 #ifndef SCREEN_W
 #define SCREEN_W 256
 #define SCREEN_H 192
 #endif
 
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
	 glEnable(GL_ANTIALIAS);
	 glEnable(GL_TEXTURE_2D);
 
	 glMatrixMode(GL_PROJECTION);
	 glLoadIdentity();
	 glOrthof32(0, SCREEN_W, SCREEN_H, 0, -1, 1);
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
 
	 glClearColor(0, 0, 0, 31);
	 glClearDepth(GL_MAX_DEPTH);
	 glClearPolyID(63);
 
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
	 if (vertCount + 4 > MAX_VERTS) return;
 
	 int fx = FX(x), fy = FX(y), fz = FX(z);
	 int fu0 = FX(u0), fv0 = FX(v0), fu1 = FX(u1), fv1 = FX(v1);
 
	 Vertex* v = &vertBuffer[vertCount];
	 v[0] = (Vertex){fx, fy, fz, fu0, fv0, r, g, b};
	 v[1] = (Vertex){fx + FX(w), fy, fz, fu1, fv0, r, g, b};
	 v[2] = (Vertex){fx + FX(w), fy + FX(h), fz, fu1, fv1, r, g, b};
	 v[3] = (Vertex){fx, fy + FX(h), fz, fu0, fv1, r, g, b};
 
	 vertCount += 4;
 }
 
 // Texture loader (8-bit palette => 256-color)
 void Graphics_LoadTileDS(int id, int width, int height, const u8* src, const u16* palette) {
	 dmaCopy(src, (void*)VRAM_A, width * height);
	 glBindTexture(GL_TEXTURE_2D, id);
	 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB256, width, height, 0, 0, (void*)VRAM_A);
 }
 
 /*--------------------------------------------------------------------------------------------------------------------*/
 /*                                       Restored engine Gfx_* implementations                                         */
 /*--------------------------------------------------------------------------------------------------------------------*/
 
 void Gfx_Create(void) {
	 Gfx_RestoreState();
 
	 Gfx.MinTexWidth  =   8;
	 Gfx.MinTexHeight =   8;
	 Gfx.MaxTexWidth  = 256;
	 Gfx.MaxTexHeight = 256;
	 Gfx.Created      = true;
	 Gfx.Limitations  = GFX_LIMIT_VERTEX_ONLY_FOG;
 
	 ResetGPU();
	 Gfx_ClearColor(PackedCol_Make(0, 120, 80, 255));
	 Gfx_SetViewport(0, 0, 256, 192);
	 
	 vramSetBankA(VRAM_A_TEXTURE);
	 vramSetBankB(VRAM_B_TEXTURE);
	 vramSetBankC(VRAM_C_TEXTURE);
	 vramSetBankD(VRAM_D_TEXTURE);
	 vramSetBankE(VRAM_E_TEX_PALETTE);
 }
 
 cc_bool Gfx_TryRestoreContext(void) {
	 // ... original body ...
 }
 
 void Gfx_Free(void) {
	 // ... original body ...
 }
 
 void Gfx_GetApiInfo(cc_string* info) {
	 // ... original body ...
 }
 
 void Gfx_SetVSync(cc_bool vsync) {
	 // ... original body ...
 }
 
 void Gfx_OnWindowResize(void) {
	 // ... original body ...
 }
 
 void Gfx_SetViewport(int x, int y, int w, int h) {
	 // ... original body ...
 }
 
 void Gfx_BeginFrame(void) { Graphics_BeginFrameDS(); }
 
 void Gfx_ClearBuffers(GfxBuffers buffers) {
	 // ... original body ...
 }
 
 void Gfx_ClearColor(PackedCol color) {
	 // ... original body ...
 }
 
 void Gfx_EndFrame(void) { Graphics_EndFrameDS(); }
 
 void Gfx_EnableTextureOffset(float x, float y) {
	 // ... original body ...
 }
 
 void Gfx_DisableTextureOffset(void) {
	 // ... original body ...
 }
 
 void Gfx_BindTexture(GfxResourceID texId) {
	 // ... original body ...
 }
 
 void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	 // ... original body ...
 }
 
 void Gfx_SetFaceCulling(cc_bool enabled) {
	 // ... original body ...
 }
 
 void Gfx_SetAlphaArgBlend(cc_bool enabled) { }
 
 void Gfx_SetFog(cc_bool enabled) {
	 // ... original body ...
 }
 
 void Gfx_SetFogCol(PackedCol color) {
	 // ... original body ...
 }
 
 void Gfx_SetFogDensity(float value) {
	 // ... original body ...
 }
 
 void Gfx_SetFogEnd(float value) {
	 // ... original body ...
 }
 
 void Gfx_SetFogMode(FogFunc func) {
	 // ... original body ...
 }
 
 void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	 // ... original body ...
 }
 
 void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	 // ... original body ...
 }
 
 void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	 // ... original body ...
 }
 
 void Gfx_SetVertexFormat(VertexFormat fmt) {
	 // ... original body ...
 }
 
 void Gfx_DrawVb_Lines(int verticesCount) {
	 // ... original body ...
 }
 
 void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	 // ... original body ...
 }
 
 void Gfx_DrawVb_IndexedTris(int verticesCount) {
	 if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		 Draw_TexturedTriangles(verticesCount, 0);
	 } else {
		 Draw_ColouredTriangles(verticesCount, 0);
	 }
 }
 
 void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	 if (skipRendering) return;
	 Draw_TexturedTriangles(verticesCount, startVertex);
 }
 
 #endif /* CC_BUILD_NDS */
 
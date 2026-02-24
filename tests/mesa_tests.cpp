/**
 * \file mesa_tests.cpp
 * Behavioural regression tests for SpecusGL.
 *
 * These tests use the public OSMesa off-screen API to drive the renderer
 * and verify that the output pixels and GL state match expected values.
 * They serve as a safety-net for the C++17 modernisation work.
 *
 * Build: linked against libosmesa (see tests/CMakeLists.txt).
 * Run  : ./mesa_tests        – prints PASS/FAIL lines, exits 0 on success.
 */

#define GL_GLEXT_PROTOTYPES
#include "OSMesa/gl.h"
#include "OSMesa/osmesa.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

/* ------------------------------------------------------------------ */
/* Minimal test-runner                                                 */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

static void check(bool condition, const char *name)
{
    if (condition) {
        printf("  PASS: %s\n", name);
        ++g_pass;
    } else {
        printf("  FAIL: %s\n", name);
        ++g_fail;
    }
}

static void check_gl(const char *where)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("  FAIL: GL error 0x%x after %s\n", (unsigned)err, where);
        ++g_fail;
    }
}

/* Run a test block with its own OSMesa context + pixel buffer. */
struct TestCtx {
    OSMesaContext ctx = nullptr;
    std::vector<GLubyte> buf;
    int w, h;

    TestCtx(int width = 64, int height = 64,
            GLenum format = OSMESA_RGBA,
            GLint depth = 16, GLint stencil = 8)
        : w(width), h(height)
    {
        ctx = OSMesaCreateContextExt(format, depth, stencil, 0, nullptr);
        if (!ctx) return;
        buf.resize(w * h * 4, 0);
        if (!OSMesaMakeCurrent(ctx, buf.data(), GL_UNSIGNED_BYTE, w, h)) {
            OSMesaDestroyContext(ctx);
            ctx = nullptr;
        }
        OSMesaPixelStore(OSMESA_Y_UP, 0);
    }

    ~TestCtx()
    {
        if (ctx) {
            OSMesaMakeCurrent(nullptr, nullptr, 0, 0, 0);
            OSMesaDestroyContext(ctx);
        }
    }

    bool valid() const { return ctx != nullptr; }

    /* Read RGBA of pixel at (x,y) from the rendered buffer. */
    void pixel(int x, int y, GLubyte out[4]) const
    {
        const int idx = (y * w + x) * 4;
        out[0] = buf[idx + 0];
        out[1] = buf[idx + 1];
        out[2] = buf[idx + 2];
        out[3] = buf[idx + 3];
    }

    /* True when rendered color at (x,y) is within 'tol' of (r,g,b,a). */
    bool near(int x, int y, GLubyte r, GLubyte g, GLubyte b, GLubyte a,
              int tol = 4) const
    {
        GLubyte px[4];
        pixel(x, y, px);
        return (std::abs((int)px[0] - r) <= tol &&
                std::abs((int)px[1] - g) <= tol &&
                std::abs((int)px[2] - b) <= tol &&
                std::abs((int)px[3] - a) <= tol);
    }
};

/* ------------------------------------------------------------------ */
/* Individual tests                                                    */
/* ------------------------------------------------------------------ */

/* --- context creation -------------------------------------------- */
static void test_context_creation()
{
    printf("\n=== Context creation ===\n");

    TestCtx tc;
    check(tc.valid(), "OSMesa context created");

    /* Version string must be present. */
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    check(vendor != nullptr, "GL_VENDOR string non-null");

    const char *renderer = (const char *)glGetString(GL_RENDERER);
    check(renderer != nullptr, "GL_RENDERER string non-null");

    /* No errors so far. */
    check(glGetError() == GL_NO_ERROR, "no GL errors on fresh context");
}

/* --- clear color -------------------------------------------------- */
static void test_clear_color()
{
    printf("\n=== Clear color ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    check(tc.near(0, 0, 255, 0, 0, 255), "corner (0,0) is red");
    check(tc.near(63, 63, 255, 0, 0, 255), "corner (63,63) is red");

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    check(tc.near(32, 32, 0, 0, 255, 255), "center is blue after second clear");

    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- basic triangle rendering ------------------------------------ */
static void test_triangle()
{
    printf("\n=== Triangle rendering ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Draw a white triangle that covers the center of the viewport. */
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f( 0.0f,  0.75f);
    glVertex2f(-0.75f, -0.75f);
    glVertex2f( 0.75f, -0.75f);
    glEnd();
    glFinish();

    /* The centroid should be white; corners should remain black. */
    check(tc.near(32, 32, 255, 255, 255, 255, 8), "triangle centroid is white");
    check(tc.near(0, 0, 0, 0, 0, 255, 4), "corner outside triangle is black");

    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- matrix push/pop -------------------------------------------- */
static void test_matrix_push_pop()
{
    printf("\n=== Matrix push/pop ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Push an identity matrix; verify stack depth increases. */
    GLint depth0, depth1, depth2;
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth0);

    glPushMatrix();
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth1);
    check(depth1 == depth0 + 1, "stack depth increments on push");

    /* Apply a translation so we can detect the restore. */
    glTranslatef(100.0f, 0.0f, 0.0f);

    glPopMatrix();
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth2);
    check(depth2 == depth0, "stack depth back to original after pop");

    /* The matrix should have been restored to identity. */
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    check(std::abs(m[12]) < 1e-4f, "translation X restored to zero after pop");

    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- nested matrix push/pop ------------------------------------- */
static void test_matrix_nested_push_pop()
{
    printf("\n=== Nested matrix push/pop ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Push three levels and apply distinct translations. */
    glTranslatef(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 3.0f);

    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    check(std::abs(m[12] - 1.0f) < 1e-4f, "X = 1 at depth 3");
    check(std::abs(m[13] - 2.0f) < 1e-4f, "Y = 2 at depth 3");
    check(std::abs(m[14] - 3.0f) < 1e-4f, "Z = 3 at depth 3");

    glPopMatrix();
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    check(std::abs(m[12] - 1.0f) < 1e-4f, "X = 1 after first pop");
    check(std::abs(m[13] - 2.0f) < 1e-4f, "Y = 2 after first pop");
    check(std::abs(m[14] - 0.0f) < 1e-4f, "Z = 0 after first pop");

    glPopMatrix();
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    check(std::abs(m[12] - 1.0f) < 1e-4f, "X = 1 after second pop");
    check(std::abs(m[13] - 0.0f) < 1e-4f, "Y = 0 after second pop");

    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- matrix stack overflow error --------------------------------- */
static void test_matrix_stack_overflow()
{
    printf("\n=== Matrix stack overflow ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Push until overflow. */
    for (int i = 0; i < 64; ++i)
        glPushMatrix();

    /* Flush the error state — we expect GL_STACK_OVERFLOW. */
    bool got_overflow = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        if (err == GL_STACK_OVERFLOW)
            got_overflow = true;
    }
    check(got_overflow, "GL_STACK_OVERFLOW generated at stack limit");

    /* Pop back to a clean state. */
    for (int i = 0; i < 64; ++i)
        glPopMatrix();
    while (glGetError() != GL_NO_ERROR) {}  /* drain underflow errors */
}

/* --- projection matrix ------------------------------------------ */
static void test_projection_matrix()
{
    printf("\n=== Projection matrix ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -2.0, 2.0, -1.0, 1.0);

    GLfloat m[16];
    glGetFloatv(GL_PROJECTION_MATRIX, m);

    /* For glOrtho(-2,2,-2,2,-1,1): m[0]=0.5, m[5]=0.5, m[10]=-1, m[15]=1 */
    check(std::abs(m[0]  - 0.5f) < 1e-4f, "ortho m[0] = 0.5");
    check(std::abs(m[5]  - 0.5f) < 1e-4f, "ortho m[5] = 0.5");
    check(std::abs(m[10] + 1.0f) < 1e-4f, "ortho m[10] = -1");
    check(std::abs(m[15] - 1.0f) < 1e-4f, "ortho m[15] = 1");

    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- depth test -------------------------------------------------- */
static void test_depth_test()
{
    printf("\n=== Depth test ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* Standard ortho: near=0 maps z_eye=0 → window_z=0 (nearest),
     * far=1 maps z_eye=-1 → window_z=1 (farthest).
     * Red at z=-0.25 → window_z=0.25 (nearer); GL_LESS wins over
     * green at z=-0.75 → window_z=0.75 (farther). */
    glOrtho(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Draw red first at z=-0.25 (nearer, window_z=0.25). */
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.25f);
    glVertex3f( 0.5f, -0.5f, -0.25f);
    glVertex3f( 0.5f,  0.5f, -0.25f);
    glVertex3f(-0.5f,  0.5f, -0.25f);
    glEnd();

    /* Draw green second at z=-0.75 (farther, window_z=0.75).
     * GL_LESS: 0.75 >= 0.25 so green fails → red remains. */
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.75f);
    glVertex3f( 0.5f, -0.5f, -0.75f);
    glVertex3f( 0.5f,  0.5f, -0.75f);
    glVertex3f(-0.5f,  0.5f, -0.75f);
    glEnd();
    glFinish();

    /* Center should be red (nearer quad wins with GL_LESS). */
    check(tc.near(32, 32, 255, 0, 0, 255, 8),
          "center is red (nearer quad occludes green)");

    glDisable(GL_DEPTH_TEST);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- vertex arrays / VBO ---------------------------------------- */
static void test_vertex_arrays()
{
    printf("\n=== Vertex arrays ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    /* Check GL_ARB_vertex_buffer_object is available. */
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    if (!exts || !strstr(exts, "GL_ARB_vertex_buffer_object")) {
        printf("  SKIP: GL_ARB_vertex_buffer_object not advertised\n");
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const float verts[] = {
         0.0f,  0.75f, 0.0f,
        -0.75f, -0.75f, 0.0f,
         0.75f, -0.75f, 0.0f,
    };
    const float colors[] = {
        1.0f, 1.0f, 0.0f,  /* yellow */
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };

    GLuint vbo[2];
    glGenBuffersARB(2, vbo);
    check(glGetError() == GL_NO_ERROR, "glGenBuffersARB ok");

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo[0]);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts, GL_STATIC_DRAW_ARB);
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    glEnableClientState(GL_VERTEX_ARRAY);

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo[1]);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(colors), colors, GL_STATIC_DRAW_ARB);
    glColorPointer(3, GL_FLOAT, 0, nullptr);
    glEnableClientState(GL_COLOR_ARRAY);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFinish();

    check(tc.near(32, 32, 255, 255, 0, 255, 12), "VBO triangle centroid is yellow");

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDeleteBuffersARB(2, vbo);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- texture 2D basic ------------------------------------------- */
static void test_texture2d()
{
    printf("\n=== Texture 2D ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Create a 2×2 cyan texture. */
    GLubyte texdata[4 * 4] = {
        0, 255, 255, 255,   0, 255, 255, 255,
        0, 255, 255, 255,   0, 255, 255, 255,
    };

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texdata);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);  /* modulate with white → tex colour */

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.5f,  0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f,  0.5f);
    glEnd();
    glFinish();

    check(tc.near(32, 32, 0, 255, 255, 255, 12), "texured quad is cyan");

    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &tex);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- stencil test ------------------------------------------------ */
static void test_stencil()
{
    printf("\n=== Stencil test ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Write 1 into stencil for a centred quad. */
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.5f); glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.5f,  0.5f); glVertex2f(-0.5f,  0.5f);
    glEnd();

    /* Draw blue only where stencil == 1. */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f); glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
    glFinish();

    check(tc.near(32, 32, 0, 0, 255, 255, 8),  "center (stencil=1) is blue");
    check(tc.near(1, 1, 0, 0, 0, 255, 4),       "corner (stencil=0) is black");

    glDisable(GL_STENCIL_TEST);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- blending ---------------------------------------------------- */
static void test_blending()
{
    printf("\n=== Blending ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Background: opaque red */
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Blend 50% blue over it → should yield ~127,0,127. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
    glVertex2f(-1.0f, -1.0f); glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
    glFinish();

    GLubyte px[4];
    tc.pixel(32, 32, px);
    /* Result = 0.5*red + 0.5*blue = (127,0,127).  Allow ±10. */
    check(std::abs((int)px[0] - 127) <= 10, "blend: R channel ~127");
    check(px[1] <= 10,                       "blend: G channel ~0");
    check(std::abs((int)px[2] - 127) <= 10, "blend: B channel ~127");

    glDisable(GL_BLEND);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- scissor test ------------------------------------------------ */
static void test_scissor()
{
    printf("\n=== Scissor test ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Only allow writes to the top-right 32×32 quadrant. */
    glEnable(GL_SCISSOR_TEST);
    glScissor(32, 32, 32, 32);

    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);  /* yellow */
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    /* OSMESA_Y_UP=0 stores rows top-to-bottom in the buffer, so
     * glScissor(32, 32, 32, 32) (GL y=32-63) maps to buffer rows 0-31.
     * Check buffer (48, 16): GL coords (48, 63-16=47) → inside scissor. */
    check(tc.near(48, 16, 255, 255, 0, 255, 4), "inside scissor is yellow");
    check(tc.near(16, 16, 0, 0, 0, 255, 4),      "outside scissor is black");
    check(tc.near(0, 0, 0, 0, 0, 255, 4),         "corner outside scissor is black");

    glDisable(GL_SCISSOR_TEST);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- colour masking ---------------------------------------------- */
static void test_color_mask()
{
    printf("\n=== Colour mask ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Clear to white with only red channel enabled. */
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    check(tc.near(32, 32, 255, 0, 0, 255, 4), "only red channel written");

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- multiple contexts / state isolation ------------------------- */
static void test_multi_context()
{
    printf("\n=== Multi-context isolation ===\n");

    TestCtx tc1, tc2;
    if (!tc1.valid() || !tc2.valid()) {
        printf("  SKIP: could not create two contexts\n");
        return;
    }

    /* Set state in context 1. */
    OSMesaMakeCurrent(tc1.ctx, tc1.buf.data(), GL_UNSIGNED_BYTE, tc1.w, tc1.h);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    /* Switch to context 2 and verify it's independent. */
    OSMesaMakeCurrent(tc2.ctx, tc2.buf.data(), GL_UNSIGNED_BYTE, tc2.w, tc2.h);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    check(tc1.near(32, 32, 255, 0, 0, 255), "ctx1 pixel is red");
    check(tc2.near(32, 32, 0, 0, 255, 255), "ctx2 pixel is blue");
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* --- display lists ----------------------------------------------- */
static void test_display_lists()
{
    printf("\n=== Display lists ===\n");
    TestCtx tc;
    if (!tc.valid()) { printf("  SKIP: no context\n"); return; }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Compile a display list that draws a green quad. */
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(-0.5f, -0.5f); glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.5f,  0.5f); glVertex2f(-0.5f,  0.5f);
    glEnd();
    glEndList();

    glCallList(list);
    glFinish();

    check(tc.near(32, 32, 0, 255, 0, 255, 12), "display list quad is green");

    glDeleteLists(list, 1);
    check(glGetError() == GL_NO_ERROR, "no GL errors");
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int main()
{
    printf("SpecusGL behavioural test suite\n");
    printf("================================\n");

    test_context_creation();
    test_clear_color();
    test_triangle();
    test_matrix_push_pop();
    test_matrix_nested_push_pop();
    test_matrix_stack_overflow();
    test_projection_matrix();
    test_depth_test();
    test_vertex_arrays();
    test_texture2d();
    test_stencil();
    test_blending();
    test_scissor();
    test_color_mask();
    test_multi_context();
    test_display_lists();

    printf("\n================================\n");
    printf("Results: %d passed, %d failed\n", g_pass, g_fail);

    return g_fail ? 1 : 0;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: c++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

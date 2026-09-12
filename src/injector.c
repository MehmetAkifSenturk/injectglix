#define _GNU_SOURCE
#include "renderer.h"
#include "logger.h"
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef void (*PFNGLXSWAPBUFFERS)(Display *dpy, GLXDrawable drawable);
static PFNGLXSWAPBUFFERS real_glXSwapBuffers = NULL;

typedef int (*PFNEGLSWAPBUFFERS)(void *dpy, void *surface);
static PFNEGLSWAPBUFFERS real_eglSwapBuffers = NULL;

typedef void (*PFNSDL_GL_SWAPWINDOW)(void *window);
static PFNSDL_GL_SWAPWINDOW real_SDL_GL_SwapWindow = NULL;

typedef void *(*PFNSDL_GL_GETPROCADDRESS)(const char *proc);
PFNSDL_GL_GETPROCADDRESS real_SDL_GL_GetProcAddress = NULL;

typedef void (*__GLXextFuncPtr)(void);
typedef __GLXextFuncPtr (*PFNGLXGETPROCADDRESSPROC)(const GLubyte *procName);
static PFNGLXGETPROCADDRESSPROC real_glXGetProcAddress = NULL;
static PFNGLXGETPROCADDRESSPROC real_glXGetProcAddressARB = NULL;

typedef void *(*dlsym_func_t)(void *, const char *);
static dlsym_func_t real_dlsym_ptr = NULL;

static void init_real_dlsym(void) {
    if (real_dlsym_ptr) return;

    real_dlsym_ptr = (dlsym_func_t)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
    if (!real_dlsym_ptr) real_dlsym_ptr = (dlsym_func_t)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.0");
    if (!real_dlsym_ptr) real_dlsym_ptr = (dlsym_func_t)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.17");
}

static void *real_dlsym(void *handle, const char *symbol) {
    if (!real_dlsym_ptr) {
        init_real_dlsym();
    }
    return real_dlsym_ptr ? real_dlsym_ptr(handle, symbol) : NULL;
}

static GLuint g_capture_texture = 0;
static Pipeline g_pipeline = {0};
static int g_width = 0;
static int g_height = 0;
static int g_initialized = 0;
static struct timespec g_start_time = {0};
static float g_last_hotreload_check = 0.0f;

static float get_elapsed_seconds(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (float)(now.tv_sec - g_start_time.tv_sec) + 
           (float)(now.tv_nsec - g_start_time.tv_nsec) / 1000000000.0f;
}

static void ensure_default_config(const char *config_path) {
    struct stat st = {0};
    
    if (strcmp(config_path, "shaders/pipeline.cfg") == 0 && stat("shaders", &st) == -1) {
#ifdef _WIN32
        mkdir("shaders");
#else
        mkdir("shaders", 0755);
#endif
    }

    FILE *f = fopen(config_path, "r");
    if (!f) {
        f = fopen(config_path, "w");
        if (f) {
            fprintf(f, "# Pipeline Configuration\n");
            fprintf(f, "# Format: pass_index, shader_path, format, scale_x, scale_y\n");
            fprintf(f, "# Example:\n");
            fprintf(f, "# 0, shaders/test-passthrough.glsl, rgba8, 1.0, 1.0\n");
            fclose(f);
            LOG_INFO("Created missing default configuration at: %s", config_path);
        } else {
            LOG_ERROR("Failed to create configuration file at: %s", config_path);
        }
    } else {
        fclose(f);
    }
}

static const char *get_config_path(void) {
    const char *path = getenv("SHADER_CONFIG");
    return path ? path : "shaders/pipeline.cfg";
}

static void init_injector(int width, int height) {
    clock_gettime(CLOCK_MONOTONIC, &g_start_time);
    load_gl_extensions();

    const char *config_path = get_config_path();
    ensure_default_config(config_path);
    
    if (g_capture_texture == 0) {
        glGenTextures(1, &g_capture_texture);
    }
    glBindTexture(GL_TEXTURE_2D, g_capture_texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    init_pipeline(&g_pipeline, config_path, width, height);

    g_width = width;
    g_height = height;
    g_initialized = 1;
    LOG_INFO("Multi-pass pipeline initialized at %dx%d using config: %s", width, height, config_path);
}

static void run_injector_pipeline(void) {
    static __thread int in_pipeline = 0;
    if (in_pipeline) return;

    // Integer milliseconds prevent floating point precision loss
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t current_ms = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
    static uint64_t last_frame_ms = 0;
    
        // Skip execution if called again within 3ms (prevents double-swapping)
    if (last_frame_ms != 0 && (current_ms - last_frame_ms < 3)) {
        return;
    }
    last_frame_ms = current_ms;
    
    in_pipeline = 1;

    // --- SAVE OPENGL STATE ---
    GLint prev_program = 0;
    GLint prev_active_texture = GL_TEXTURE0;
    GLint prev_array_buffer = 0;
    GLint prev_element_buffer = 0;
    GLint prev_vao = 0;
    GLint prev_texture_2d = 0;
    GLint prev_read_buffer = 0;
    GLint prev_draw_fbo = 0;
    GLint prev_read_fbo = 0;
    GLint viewport[4];

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    GLboolean prev_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_cull_face = glIsEnabled(GL_CULL_FACE);

    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_element_buffer);
    
#ifdef GL_VERTEX_ARRAY_BINDING
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
#endif

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_2d);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_VIEWPORT, viewport);

    int current_width = viewport[2];
    int current_height = viewport[3];

    if (current_width <= 0 || current_height <= 0) {
        in_pipeline = 0;
        return;
    }

    if (!g_initialized) {
        init_injector(current_width, current_height);
    } else if (current_width != g_width || current_height != g_height) {
        resize_pipeline(&g_pipeline, current_width, current_height);

        glBindTexture(GL_TEXTURE_2D, g_capture_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, current_width, current_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        g_width = current_width;
        g_height = current_height;
    }

    if (glBindFramebuffer_ptr) {
        glBindFramebuffer_ptr(GL_READ_FRAMEBUFFER, prev_draw_fbo);
    }
    
    if (prev_draw_fbo == 0) {
        glReadBuffer(GL_BACK);
    } else {
        glReadBuffer(GL_COLOR_ATTACHMENT0); 
    }
    
    glBindTexture(GL_TEXTURE_2D, g_capture_texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], current_width, current_height);
    glBindTexture(GL_TEXTURE_2D, 0);

    float elapsed_time = get_elapsed_seconds();

    if (elapsed_time - g_last_hotreload_check >= 1.0f) {
        check_pipeline_hotreload(&g_pipeline, get_config_path());
        g_last_hotreload_check = elapsed_time;
    }

    render_pipeline(&g_pipeline, g_capture_texture, current_width, current_height, elapsed_time);

    // --- RESTORE OPENGL STATE ---
    if (glUseProgram_ptr) glUseProgram_ptr(prev_program);
    if (glUseProgram_ptr) glUseProgram_ptr(prev_program);
        
    glActiveTexture(prev_active_texture);
    glBindTexture(GL_TEXTURE_2D, prev_texture_2d);
    
    if (glBindBuffer_ptr) {
        glBindBuffer_ptr(GL_ARRAY_BUFFER, prev_array_buffer);
        glBindBuffer_ptr(GL_ELEMENT_ARRAY_BUFFER, prev_element_buffer);
    }
    
    if (glBindVertexArray_ptr && prev_vao != 0) {
            glBindVertexArray_ptr(prev_vao);
    }
    
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glReadBuffer(prev_read_buffer);
    
    if (glBindFramebuffer_ptr) {
        glBindFramebuffer_ptr(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
        glBindFramebuffer_ptr(GL_READ_FRAMEBUFFER, prev_read_fbo);
    }
    
    if (prev_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (prev_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prev_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prev_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    in_pipeline = 0;
}

__attribute__((visibility("default")))
void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
    if (!real_glXSwapBuffers) {
        real_glXSwapBuffers = (PFNGLXSWAPBUFFERS)real_dlsym(RTLD_NEXT, "glXSwapBuffers");
        LOG_INFO("Hooked glXSwapBuffers");
    }

    run_injector_pipeline();

    if (real_glXSwapBuffers) {
        real_glXSwapBuffers(dpy, drawable);
    }
}

__attribute__((visibility("default")))
int eglSwapBuffers(void *dpy, void *surface) {
    if (!real_eglSwapBuffers) {
        real_eglSwapBuffers = (PFNEGLSWAPBUFFERS)real_dlsym(RTLD_NEXT, "eglSwapBuffers");
        LOG_INFO("Hooked eglSwapBuffers");
    }

    run_injector_pipeline();
    return real_eglSwapBuffers ? real_eglSwapBuffers(dpy, surface) : 0;
}

__attribute__((visibility("default")))
void SDL_GL_SwapWindow(void *window) {
    if (!real_SDL_GL_SwapWindow) {
        real_SDL_GL_SwapWindow = (PFNSDL_GL_SWAPWINDOW)real_dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
        LOG_INFO("Hooked SDL_GL_SwapWindow");
    }

    run_injector_pipeline();

    if (real_SDL_GL_SwapWindow) {
        real_SDL_GL_SwapWindow(window);
    }
}

__attribute__((visibility("default")))
void *SDL_GL_GetProcAddress(const char *proc) {
    if (!proc) return NULL;

    if (strcmp(proc, "SDL_GL_SwapWindow") == 0) return (void *)SDL_GL_SwapWindow;
    if (strcmp(proc, "glXSwapBuffers") == 0)     return (void *)glXSwapBuffers;
    if (strcmp(proc, "eglSwapBuffers") == 0)     return (void *)eglSwapBuffers;

    if (!real_SDL_GL_GetProcAddress) {
        real_SDL_GL_GetProcAddress = (PFNSDL_GL_GETPROCADDRESS)real_dlsym(RTLD_NEXT, "SDL_GL_GetProcAddress");
    }

    return real_SDL_GL_GetProcAddress ? real_SDL_GL_GetProcAddress(proc) : NULL;
}

__attribute__((visibility("default")))
__GLXextFuncPtr glXGetProcAddress(const GLubyte *procName) {
    if (!procName) return NULL;

    const char *name = (const char *)procName;
    if (strcmp(name, "glXSwapBuffers") == 0) return (__GLXextFuncPtr)glXSwapBuffers;
    if (strcmp(name, "SDL_GL_SwapWindow") == 0) return (__GLXextFuncPtr)SDL_GL_SwapWindow;

    if (!real_glXGetProcAddress) {
        real_glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)real_dlsym(RTLD_NEXT, "glXGetProcAddress");
    }

    return real_glXGetProcAddress ? real_glXGetProcAddress(procName) : NULL;
}

__attribute__((visibility("default")))
__GLXextFuncPtr glXGetProcAddressARB(const GLubyte *procName) {
    if (!procName) return NULL;

    const char *name = (const char *)procName;
    if (strcmp(name, "glXSwapBuffers") == 0) return (__GLXextFuncPtr)glXSwapBuffers;
    if (strcmp(name, "SDL_GL_SwapWindow") == 0) return (__GLXextFuncPtr)SDL_GL_SwapWindow;

    if (!real_glXGetProcAddressARB) {
        real_glXGetProcAddressARB = (PFNGLXGETPROCADDRESSPROC)real_dlsym(RTLD_NEXT, "glXGetProcAddressARB");
    }

    return real_glXGetProcAddressARB ? real_glXGetProcAddressARB(procName) : NULL;
}

__attribute__((constructor))
static void injector_loaded(void) {
    LOG_INFO("libinjector.so was loaded into this process.");
    init_real_dlsym();
}

__attribute__((visibility("default")))
void *dlsym(void *handle, const char *symbol) {
    static __thread int in_dlsym = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
    if (!symbol) return NULL;
#pragma GCC diagnostic pop

    if (in_dlsym) {
        return real_dlsym(handle, symbol);
    }

    in_dlsym = 1;
    void *result = NULL;

    if (strcmp(symbol, "glXSwapBuffers") == 0) {
        if (!real_glXSwapBuffers) {
            real_glXSwapBuffers = (PFNGLXSWAPBUFFERS)real_dlsym(handle, symbol);
        }
        result = (void *)glXSwapBuffers;
    } else if (strcmp(symbol, "eglSwapBuffers") == 0) {
        if (!real_eglSwapBuffers) {
            real_eglSwapBuffers = (PFNEGLSWAPBUFFERS)real_dlsym(handle, symbol);
        }
        result = (void *)eglSwapBuffers;
    } else if (strcmp(symbol, "SDL_GL_SwapWindow") == 0) {
        if (!real_SDL_GL_SwapWindow) {
            real_SDL_GL_SwapWindow = (PFNSDL_GL_SWAPWINDOW)real_dlsym(handle, symbol);
        }
        result = (void *)SDL_GL_SwapWindow;
    } else if (strcmp(symbol, "SDL_GL_GetProcAddress") == 0) {
        if (!real_SDL_GL_GetProcAddress) {
            real_SDL_GL_GetProcAddress = (PFNSDL_GL_GETPROCADDRESS)real_dlsym(handle, symbol);
        }
        result = (void *)SDL_GL_GetProcAddress;
    } else if (strcmp(symbol, "glXGetProcAddress") == 0) {
        if (!real_glXGetProcAddress) {
            real_glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)real_dlsym(handle, symbol);
        }
        result = (void *)glXGetProcAddress;
    } else if (strcmp(symbol, "glXGetProcAddressARB") == 0) {
        if (!real_glXGetProcAddressARB) {
            real_glXGetProcAddressARB = (PFNGLXGETPROCADDRESSPROC)real_dlsym(handle, symbol);
        }
        result = (void *)glXGetProcAddressARB;
    } else {
        result = real_dlsym(handle, symbol);
    }

    in_dlsym = 0;
    return result;
}

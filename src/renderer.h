#ifndef RENDERER_H
#define RENDERER_H

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <time.h>

#define MAX_PASSES 16
#define MAX_PIPELINE_TEXTURES 16

extern PFNGLCREATEPROGRAMPROC glCreateProgram_ptr;
extern PFNGLCREATESHADERPROC glCreateShader_ptr;
extern PFNGLSHADERSOURCEPROC glShaderSource_ptr;
extern PFNGLCOMPILESHADERPROC glCompileShader_ptr;
extern PFNGLATTACHSHADERPROC glAttachShader_ptr;
extern PFNGLLINKPROGRAMPROC glLinkProgram_ptr;
extern PFNGLUSEPROGRAMPROC glUseProgram_ptr;
extern PFNGLGETSHADERIVPROC glGetShaderiv_ptr;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ptr;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv_ptr;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ptr;
extern PFNGLDELETESHADERPROC glDeleteShader_ptr;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram_ptr;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ptr;
extern PFNGLUNIFORM1IPROC glUniform1i_ptr;
extern PFNGLUNIFORM1FPROC glUniform1f_ptr;
extern PFNGLUNIFORM2FPROC glUniform2f_ptr;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv_ptr;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers_ptr;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer_ptr;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D_ptr;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_ptr;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers_ptr;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ptr;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ptr;
extern PFNGLGENBUFFERSPROC glGenBuffers_ptr;
extern PFNGLBINDBUFFERPROC glBindBuffer_ptr;
extern PFNGLBUFFERDATAPROC glBufferData_ptr;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ptr;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_ptr;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation_ptr;
extern PFNGLTEXIMAGE3DPROC glTexImage3D_ptr;

extern void *(*real_SDL_GL_GetProcAddress)(const char *proc);

typedef struct {
    GLuint texture;
    GLuint fbo;
    int width;
    int height;
    GLenum filter_mode;
} FBO;

typedef struct {
    char uniform_name[64];
    GLuint id;
    GLenum target;
} PipelineTexture;

typedef struct {
    GLint u_Texture;
    GLint u_OrigTexture;
    GLint u_MVPMatrix;
    GLint u_time;
    GLint u_frame_count;
    GLint u_FrameCount;
    GLint u_FrameDirection;
    GLint u_resolution;
    GLint u_TextureSize;
    GLint u_InputSize;
    GLint u_OutputSize;

    GLint u_OrigTextureSize;
    GLint u_OrigInputSize;

    GLint pass_prev_tex[MAX_PASSES];
    GLint pass_prev_tex_size[MAX_PASSES];
    GLint pass_prev_input_size[MAX_PASSES];

    GLint alias_tex[MAX_PASSES];
    GLint alias_tex_size[MAX_PASSES];
    GLint alias_input_size[MAX_PASSES];

    GLint user_tex[MAX_PIPELINE_TEXTURES];
} PassUniforms;

typedef struct {
    char shader_path[256];
    char alias[64];
    GLuint program;
    FBO fbo;
    GLenum format;
    GLenum filter_mode;
    float scale_x;
    float scale_y;
    time_t shader_mtime;
    PassUniforms uniforms;
} Pass;

typedef struct {
    Pass passes[MAX_PASSES];
    int pass_count;
    PipelineTexture textures[MAX_PIPELINE_TEXTURES];
    int texture_count;
    time_t cfg_mtime;
} Pipeline;

void load_gl_extensions(void);
GLuint load_generic_texture(const char *filepath, GLenum target, GLenum wrap_mode);
GLuint compile_shader_program(const char *shader_path);
void init_fbo(FBO *fbo, int w, int h, GLenum format, GLenum filter_mode);
void free_fbo(FBO *fbo);
void init_pipeline(Pipeline *pipe, const char *cfg_path, int w, int h);
void resize_pipeline(Pipeline *pipe, int w, int h);
void check_pipeline_hotreload(Pipeline *pipe, const char *cfg_path);
void render_pipeline(Pipeline *pipe, GLuint capture_texture, int screen_w, int screen_h, float time);
void free_pipeline(Pipeline *pipe);

#endif

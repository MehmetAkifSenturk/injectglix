#define _GNU_SOURCE
#include <dlfcn.h>
#include <GL/glx.h>

#include "renderer.h"
#include "logger.h"
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma GCC diagnostic pop

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

PFNGLCREATEPROGRAMPROC glCreateProgram_ptr = NULL;
PFNGLCREATESHADERPROC glCreateShader_ptr = NULL;
PFNGLSHADERSOURCEPROC glShaderSource_ptr = NULL;
PFNGLCOMPILESHADERPROC glCompileShader_ptr = NULL;
PFNGLATTACHSHADERPROC glAttachShader_ptr = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram_ptr = NULL;
PFNGLUSEPROGRAMPROC glUseProgram_ptr = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv_ptr = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ptr = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv_ptr = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ptr = NULL;
PFNGLDELETESHADERPROC glDeleteShader_ptr = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram_ptr = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ptr = NULL;
PFNGLUNIFORM1IPROC glUniform1i_ptr = NULL;
PFNGLUNIFORM1FPROC glUniform1f_ptr = NULL;
PFNGLUNIFORM2FPROC glUniform2f_ptr = NULL;
PFNGLUNIFORM4FPROC glUniform4f_ptr = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv_ptr = NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers_ptr = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer_ptr = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D_ptr = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_ptr = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers_ptr = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays_ptr = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray_ptr = NULL;
PFNGLGENBUFFERSPROC glGenBuffers_ptr = NULL;
PFNGLBINDBUFFERPROC glBindBuffer_ptr = NULL;
PFNGLBUFFERDATAPROC glBufferData_ptr = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ptr = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_ptr = NULL;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation_ptr = NULL;
PFNGLTEXIMAGE3DPROC glTexImage3D_ptr = NULL;

static GLuint g_quad_vao = 0;
static GLuint g_quad_vbo = 0;

static void* get_gl_proc(const char *name) {
    void *p = NULL;
    if (real_SDL_GL_GetProcAddress) {
        p = real_SDL_GL_GetProcAddress(name);
    }
    if (!p) {
        p = dlsym(RTLD_DEFAULT, name);
    }
    if (!p) {
        p = (void*)glXGetProcAddress((const GLubyte*)name);
    }
    return p;
}

void load_gl_extensions(void) {
    glCreateProgram_ptr = (PFNGLCREATEPROGRAMPROC)get_gl_proc("glCreateProgram");
    glCreateShader_ptr = (PFNGLCREATESHADERPROC)get_gl_proc("glCreateShader");
    glShaderSource_ptr = (PFNGLSHADERSOURCEPROC)get_gl_proc("glShaderSource");
    glCompileShader_ptr = (PFNGLCOMPILESHADERPROC)get_gl_proc("glCompileShader");
    glAttachShader_ptr = (PFNGLATTACHSHADERPROC)get_gl_proc("glAttachShader");
    glLinkProgram_ptr = (PFNGLLINKPROGRAMPROC)get_gl_proc("glLinkProgram");
    glUseProgram_ptr = (PFNGLUSEPROGRAMPROC)get_gl_proc("glUseProgram");
    glGetShaderiv_ptr = (PFNGLGETSHADERIVPROC)get_gl_proc("glGetShaderiv");
    glGetShaderInfoLog_ptr = (PFNGLGETSHADERINFOLOGPROC)get_gl_proc("glGetShaderInfoLog");
    glGetProgramiv_ptr = (PFNGLGETPROGRAMIVPROC)get_gl_proc("glGetProgramiv");
    glGetProgramInfoLog_ptr = (PFNGLGETPROGRAMINFOLOGPROC)get_gl_proc("glGetProgramInfoLog");
    glDeleteShader_ptr = (PFNGLDELETESHADERPROC)get_gl_proc("glDeleteShader");
    glDeleteProgram_ptr = (PFNGLDELETEPROGRAMPROC)get_gl_proc("glDeleteProgram");
    glGetUniformLocation_ptr = (PFNGLGETUNIFORMLOCATIONPROC)get_gl_proc("glGetUniformLocation");
    glUniform1i_ptr = (PFNGLUNIFORM1IPROC)get_gl_proc("glUniform1i");
    glUniform1f_ptr = (PFNGLUNIFORM1FPROC)get_gl_proc("glUniform1f");
    glUniform2f_ptr = (PFNGLUNIFORM2FPROC)get_gl_proc("glUniform2f");
    glUniform4f_ptr = (PFNGLUNIFORM4FPROC)get_gl_proc("glUniform4f");
    glUniformMatrix4fv_ptr = (PFNGLUNIFORMMATRIX4FVPROC)get_gl_proc("glUniformMatrix4fv");
    glGenFramebuffers_ptr = (PFNGLGENFRAMEBUFFERSPROC)get_gl_proc("glGenFramebuffers");
    glBindFramebuffer_ptr = (PFNGLBINDFRAMEBUFFERPROC)get_gl_proc("glBindFramebuffer");
    glFramebufferTexture2D_ptr = (PFNGLFRAMEBUFFERTEXTURE2DPROC)get_gl_proc("glFramebufferTexture2D");
    glCheckFramebufferStatus_ptr = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)get_gl_proc("glCheckFramebufferStatus");
    glDeleteFramebuffers_ptr = (PFNGLDELETEFRAMEBUFFERSPROC)get_gl_proc("glDeleteFramebuffers");
    glGenVertexArrays_ptr = (PFNGLGENVERTEXARRAYSPROC)get_gl_proc("glGenVertexArrays");
    glBindVertexArray_ptr = (PFNGLBINDVERTEXARRAYPROC)get_gl_proc("glBindVertexArray");
    glGenBuffers_ptr = (PFNGLGENBUFFERSPROC)get_gl_proc("glGenBuffers");
    glBindBuffer_ptr = (PFNGLBINDBUFFERPROC)get_gl_proc("glBindBuffer");
    glBufferData_ptr = (PFNGLBUFFERDATAPROC)get_gl_proc("glBufferData");
    glEnableVertexAttribArray_ptr = (PFNGLENABLEVERTEXATTRIBARRAYPROC)get_gl_proc("glEnableVertexAttribArray");
    glVertexAttribPointer_ptr = (PFNGLVERTEXATTRIBPOINTERPROC)get_gl_proc("glVertexAttribPointer");
    glBindAttribLocation_ptr = (PFNGLBINDATTRIBLOCATIONPROC)get_gl_proc("glBindAttribLocation");
    glTexImage3D_ptr = (PFNGLTEXIMAGE3DPROC)get_gl_proc("glTexImage3D");
}

GLuint load_generic_texture(const char *filepath, GLenum target, GLenum wrap_mode) {
    int w, h, ch;
    unsigned char *data = stbi_load(filepath, &w, &h, &ch, 4);
    if (!data) {
        LOG_ERROR("Failed to load texture file: %s", filepath);
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(target, tex);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_mode);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (target == GL_TEXTURE_2D) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        LOG_INFO("Loaded 2D Texture: %s (%dx%d)", filepath, w, h);
    } else if (target == GL_TEXTURE_3D) {
        glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap_mode);
        int lut_size = h;

        unsigned char *lut3D = (unsigned char*)malloc(lut_size * lut_size * lut_size * 4);
        for (int z = 0; z < lut_size; z++) {
            for (int y = 0; y < lut_size; y++) {
                for (int x = 0; x < lut_size; x++) {
                    int srcX = z * lut_size + x;
                    int srcY = y;
                    int srcIdx = (srcY * w + srcX) * 4;
                    int dstIdx = (z * lut_size * lut_size + y * lut_size + x) * 4;
                    memcpy(&lut3D[dstIdx], &data[srcIdx], 4);
                }
            }
        }

        if (glTexImage3D_ptr) {
            glTexImage3D_ptr(GL_TEXTURE_3D, 0, GL_RGBA8, lut_size, lut_size, lut_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, lut3D);
        }
        free(lut3D);
        LOG_INFO("Loaded 3D LUT Texture: %s (%dx%dx%d)", filepath, lut_size, lut_size, lut_size);
    }

    stbi_image_free(data);
    glBindTexture(target, 0);
    return tex;
}

static void init_quad_geometry(void) {
    if (g_quad_vao != 0) return;

    float quad_vertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    if (glGenVertexArrays_ptr) {
        glGenVertexArrays_ptr(1, &g_quad_vao);
        glBindVertexArray_ptr(g_quad_vao);
    }

    if (glGenBuffers_ptr) {
        glGenBuffers_ptr(1, &g_quad_vbo);
        glBindBuffer_ptr(GL_ARRAY_BUFFER, g_quad_vbo);
        glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

        if (glEnableVertexAttribArray_ptr && glVertexAttribPointer_ptr) {
            glEnableVertexAttribArray_ptr(0);
            glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

            glEnableVertexAttribArray_ptr(1);
            glVertexAttribPointer_ptr(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        }
    }

    if (glBindVertexArray_ptr) glBindVertexArray_ptr(0);
    if (glBindBuffer_ptr) glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (buf) {
        fread(buf, 1, sz, f);
        buf[sz] = '\0';
    }
    fclose(f);
    return buf;
}

static GLuint compile_shader_stage(GLenum type, const char *src, const char *path) {
    GLuint s = glCreateShader_ptr(type);
    const char *stage_def = (type == GL_VERTEX_SHADER) ? "#define VERTEX\n" : "#define FRAGMENT\n";

    const char *version_pos = strstr(src, "#version");
    if (version_pos != NULL) {
        const char *version_end = strchr(version_pos, '\n');
        if (version_end) {
            size_t ver_len = version_end - version_pos + 1;
            char *ver_line = malloc(ver_len + 1);
            strncpy(ver_line, version_pos, ver_len);
            ver_line[ver_len] = '\0';

            const char *body = version_end + 1;
            const char *sources[3] = { ver_line, stage_def, body };

            glShaderSource_ptr(s, 3, sources, NULL);
            glCompileShader_ptr(s);
            free(ver_line);
        } else {
            const char *sources[2] = { stage_def, src };
            glShaderSource_ptr(s, 2, sources, NULL);
            glCompileShader_ptr(s);
        }
    } else {
        const char *default_ver = "#version 330 core\n";
        const char *sources[3] = { default_ver, stage_def, src };
        glShaderSource_ptr(s, 3, sources, NULL);
        glCompileShader_ptr(s);
    }

    GLint ok = 0;
    glGetShaderiv_ptr(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog_ptr(s, sizeof(log), NULL, log);
        LOG_ERROR("Shader compilation error [%s] (%s): %s", path, (type == GL_VERTEX_SHADER ? "VERT" : "FRAG"), log);
        glDeleteShader_ptr(s);
        return 0;
    }
    return s;
}

GLuint compile_shader_program(const char *shader_path) {
    char *src = read_file(shader_path);
    if (!src) {
        LOG_ERROR("Failed to read shader file: %s", shader_path);
        return 0;
    }

    GLuint vert = compile_shader_stage(GL_VERTEX_SHADER, src, shader_path);
    GLuint frag = compile_shader_stage(GL_FRAGMENT_SHADER, src, shader_path);
    free(src);

    if (!vert || !frag) {
        if (vert) glDeleteShader_ptr(vert);
        if (frag) glDeleteShader_ptr(frag);
        return 0;
    }

    GLuint prog = glCreateProgram_ptr();
    glAttachShader_ptr(prog, vert);
    glAttachShader_ptr(prog, frag);

    if (glBindAttribLocation_ptr) {
        glBindAttribLocation_ptr(prog, 0, "VertexCoord");
        glBindAttribLocation_ptr(prog, 1, "TexCoord");
        glBindAttribLocation_ptr(prog, 2, "COLOR");
    }
    
    glLinkProgram_ptr(prog);

    GLint ok = 0;
    glGetProgramiv_ptr(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetProgramInfoLog_ptr(prog, sizeof(log), NULL, log);
        LOG_ERROR("Shader link error [%s]: %s", shader_path, log);
        glDeleteProgram_ptr(prog);
        prog = 0;
    }

    glDeleteShader_ptr(vert);
    glDeleteShader_ptr(frag);
    return prog;
}

void init_fbo(FBO *fbo, int w, int h, GLenum format, GLenum filter_mode) {
    fbo->width = w;
    fbo->height = h;
    fbo->filter_mode = filter_mode;

    glGenTextures(1, &fbo->texture);
    glBindTexture(GL_TEXTURE_2D, fbo->texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, fbo->filter_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, fbo->filter_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    GLenum data_type = (format == GL_RGBA16F) ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, data_type, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (glGenFramebuffers_ptr) {
        glGenFramebuffers_ptr(1, &fbo->fbo);
        glBindFramebuffer_ptr(GL_FRAMEBUFFER, fbo->fbo);
        glFramebufferTexture2D_ptr(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->texture, 0);
        glBindFramebuffer_ptr(GL_FRAMEBUFFER, 0);
    }
}

void free_fbo(FBO *fbo) {
    if (fbo->texture) glDeleteTextures(1, &fbo->texture);
    if (fbo->fbo && glDeleteFramebuffers_ptr) glDeleteFramebuffers_ptr(1, &fbo->fbo);
    memset(fbo, 0, sizeof(FBO));
}

static time_t get_file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return st.st_mtime;
    return 0;
}

static void trim_string(char *str) {
    char *p = str + strlen(str) - 1;
    while (p > str && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) *p-- = '\0';
    p = str;
    while (*p == ' ' || *p == '\t') p++;
    if (p != str) memmove(str, p, strlen(p) + 1);
}

static void cache_pass_uniforms(Pipeline *pipe, int pass_idx) {
    Pass *pass = &pipe->passes[pass_idx];
    GLuint prog = pass->program;
    if (!prog) return;

    PassUniforms *u = &pass->uniforms;
    memset(u, -1, sizeof(PassUniforms));

    u->u_Texture = glGetUniformLocation_ptr(prog, "Texture");
    if (u->u_Texture == -1) u->u_Texture = glGetUniformLocation_ptr(prog, "u_Texture");

    u->u_OrigTexture = glGetUniformLocation_ptr(prog, "OrigTexture");
    if (u->u_OrigTexture == -1) u->u_OrigTexture = glGetUniformLocation_ptr(prog, "u_OrigTexture");

    u->u_MVPMatrix = glGetUniformLocation_ptr(prog, "MVPMatrix");
    if (u->u_MVPMatrix == -1) u->u_MVPMatrix = glGetUniformLocation_ptr(prog, "u_MVPMatrix");

    u->u_time = glGetUniformLocation_ptr(prog, "u_time");
    if (u->u_time == -1) u->u_time = glGetUniformLocation_ptr(prog, "time");

    u->u_frame_count = glGetUniformLocation_ptr(prog, "u_frame_count");
    u->u_FrameCount = glGetUniformLocation_ptr(prog, "FrameCount");
    u->u_FrameDirection = glGetUniformLocation_ptr(prog, "FrameDirection");
    u->u_resolution = glGetUniformLocation_ptr(prog, "u_resolution");

    u->u_TextureSize = glGetUniformLocation_ptr(prog, "TextureSize");
    u->u_InputSize = glGetUniformLocation_ptr(prog, "InputSize");
    u->u_OutputSize = glGetUniformLocation_ptr(prog, "OutputSize");

    u->u_OrigTextureSize = glGetUniformLocation_ptr(prog, "OrigTextureSize");
    if (u->u_OrigTextureSize == -1) u->u_OrigTextureSize = glGetUniformLocation_ptr(prog, "u_OrigTextureSize");

    u->u_OrigInputSize = glGetUniformLocation_ptr(prog, "OrigInputSize");
    if (u->u_OrigInputSize == -1) u->u_OrigInputSize = glGetUniformLocation_ptr(prog, "u_OrigInputSize");

    for (int n = 1; n <= pass_idx; n++) {
        char name[64];
        snprintf(name, sizeof(name), "PassPrev%dTexture", n);
        u->pass_prev_tex[n] = glGetUniformLocation_ptr(prog, name);

        snprintf(name, sizeof(name), "PassPrev%dTextureSize", n);
        u->pass_prev_tex_size[n] = glGetUniformLocation_ptr(prog, name);

        snprintf(name, sizeof(name), "PassPrev%dInputSize", n);
        u->pass_prev_input_size[n] = glGetUniformLocation_ptr(prog, name);
    }

    for (int p = 0; p < pipe->pass_count; p++) {
        if (pipe->passes[p].alias[0] != '\0') {
            char name[128];
            snprintf(name, sizeof(name), "%sTexture", pipe->passes[p].alias);
            u->alias_tex[p] = glGetUniformLocation_ptr(prog, name);

            snprintf(name, sizeof(name), "%sTextureSize", pipe->passes[p].alias);
            u->alias_tex_size[p] = glGetUniformLocation_ptr(prog, name);

            snprintf(name, sizeof(name), "%sInputSize", pipe->passes[p].alias);
            u->alias_input_size[p] = glGetUniformLocation_ptr(prog, name);
        }
    }

    for (int t = 0; t < pipe->texture_count; t++) {
        u->user_tex[t] = glGetUniformLocation_ptr(prog, pipe->textures[t].uniform_name);
    }
}

static void cache_pipeline_uniforms(Pipeline *pipe) {
    for (int i = 0; i < pipe->pass_count; i++) {
        cache_pass_uniforms(pipe, i);
    }
}

void parse_pipeline_cfg(Pipeline *pipe, const char *cfg_path, int base_w, int base_h) {
    pipe->pass_count = 0;
    pipe->texture_count = 0;

    FILE *f = fopen(cfg_path, "r");
    if (!f) {
        LOG_ERROR("Could not open configuration file: %s", cfg_path);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        if (strncmp(line, "texture", 7) == 0) {
            char name[64] = {0}, path[256] = {0}, type[16] = {0}, wrap[16] = {0};
            int parsed = sscanf(line, "texture, %63[^,], %255[^,], %15[^,], %15s", name, path, type, wrap);
            if (parsed >= 4) {
                trim_string(name); trim_string(path); trim_string(type); trim_string(wrap);

                GLenum target = (strcmp(type, "3d") == 0) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
                GLenum wrap_mode = (strcmp(wrap, "clamp") == 0) ? GL_CLAMP_TO_EDGE : GL_REPEAT;

                GLuint tex_id = load_generic_texture(path, target, wrap_mode);
                if (tex_id != 0 && pipe->texture_count < MAX_PIPELINE_TEXTURES) {
                    strncpy(pipe->textures[pipe->texture_count].uniform_name, name, 64);
                    pipe->textures[pipe->texture_count].id = tex_id;
                    pipe->textures[pipe->texture_count].target = target;
                    pipe->texture_count++;
                }
            }
            continue;
        }

        Pass pass = {0};
        char format_str[32] = {0};
        char alias_str[64] = {0};
        char filter_str[16] = {0};
        int pass_idx = 0;

        int scanned = sscanf(line, "%d,%255[^,],%31[^,],%f,%f,%15[^,],%63[^,\n\r]", 
                            &pass_idx, pass.shader_path, format_str, 
                            &pass.scale_x, &pass.scale_y, filter_str, alias_str);

        if (scanned >= 5) {
            trim_string(filter_str);
            if (strcmp(filter_str, "nearest") == 0) {
                pass.filter_mode = GL_NEAREST;
            } else {
                pass.filter_mode = GL_LINEAR; // Default to linear
            }
            
            trim_string(pass.shader_path);
            if (scanned >= 6) {
                trim_string(alias_str);
                snprintf(pass.alias, sizeof(pass.alias), "%s", alias_str);
            }

            pass.program = compile_shader_program(pass.shader_path);
            if (!pass.program) {
                LOG_ERROR("Skipping Pass %d due to shader compilation failure.", pipe->pass_count);
                continue;
            }

            pass.format = GL_RGBA8;
            if (strstr(format_str, "rgba16f")) pass.format = GL_RGBA16F;

            pass.shader_mtime = get_file_mtime(pass.shader_path);

            int fbo_w = (pass.scale_x >= 2.0f) ? (int)pass.scale_x : (int)(base_w * pass.scale_x);
            int fbo_h = (pass.scale_y >= 2.0f) ? (int)pass.scale_y : (int)(base_h * pass.scale_y);
            init_fbo(&pass.fbo, fbo_w, fbo_h, pass.format, pass.filter_mode);

            pipe->passes[pipe->pass_count] = pass;
            LOG_INFO("Loaded Pass %d: Shader = '%s', Format = %s, Res = %dx%d, Alias = '%s'", 
                     pipe->pass_count, pass.shader_path, format_str, fbo_w, fbo_h, pass.alias);

            pipe->pass_count++;
            if (pipe->pass_count >= MAX_PASSES) break;
        }
    }
    fclose(f);
    pipe->cfg_mtime = get_file_mtime(cfg_path);

    cache_pipeline_uniforms(pipe);

    LOG_INFO("Pipeline initialized: %d pass(es), %d texture(s).", pipe->pass_count, pipe->texture_count);
}

void init_pipeline(Pipeline *pipe, const char *cfg_path, int w, int h) {
    init_quad_geometry();
    parse_pipeline_cfg(pipe, cfg_path, w, h);
}

void resize_pipeline(Pipeline *pipe, int w, int h) {
    for (int i = 0; i < pipe->pass_count; i++) {
        free_fbo(&pipe->passes[i].fbo);
        int fbo_w = (pipe->passes[i].scale_x >= 2.0f) ? (int)pipe->passes[i].scale_x : (int)(w * pipe->passes[i].scale_x);
        int fbo_h = (pipe->passes[i].scale_y >= 2.0f) ? (int)pipe->passes[i].scale_y : (int)(h * pipe->passes[i].scale_y);
        init_fbo(&pipe->passes[i].fbo, fbo_w, fbo_h, pipe->passes[i].format, pipe->passes[i].filter_mode);
    }
}

void check_pipeline_hotreload(Pipeline *pipe, const char *cfg_path) {
    time_t cfg_time = get_file_mtime(cfg_path);
    if (cfg_time > pipe->cfg_mtime) {
        LOG_INFO("Config updated! Reloading pipeline...");
        int w = pipe->passes[0].fbo.width;
        int h = pipe->passes[0].fbo.height;
        free_pipeline(pipe);
        parse_pipeline_cfg(pipe, cfg_path, w, h);
        return;
    }

    for (int i = 0; i < pipe->pass_count; i++) {
        time_t shader_time = get_file_mtime(pipe->passes[i].shader_path);
        if (shader_time > pipe->passes[i].shader_mtime) {
            LOG_INFO("Shader updated [%s]! Recompiling...", pipe->passes[i].shader_path);
            GLuint new_prog = compile_shader_program(pipe->passes[i].shader_path);
            if (new_prog) {
                if (pipe->passes[i].program) glDeleteProgram_ptr(pipe->passes[i].program);
                pipe->passes[i].program = new_prog;
                pipe->passes[i].shader_mtime = shader_time;
                cache_pass_uniforms(pipe, i);
            }
        }
    }
}

void free_pipeline(Pipeline *pipe) {
    for (int i = 0; i < pipe->pass_count; i++) {
        if (pipe->passes[i].program) glDeleteProgram_ptr(pipe->passes[i].program);
        free_fbo(&pipe->passes[i].fbo);
    }
    for (int t = 0; t < pipe->texture_count; t++) {
        if (pipe->textures[t].id) glDeleteTextures(1, &pipe->textures[t].id);
    }
    pipe->pass_count = 0;
    pipe->texture_count = 0;
}

void render_pipeline(Pipeline *pipe, GLuint capture_texture, int screen_w, int screen_h, float time) {
    if (pipe->pass_count == 0) return;

    GLint orig_vao = 0, orig_vbo = 0, orig_fbo = 0, orig_program = 0, orig_active_tex = GL_TEXTURE0;
    GLint orig_viewport[4];

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &orig_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &orig_vbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &orig_fbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &orig_program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &orig_active_tex);
    glGetIntegerv(GL_VIEWPORT, orig_viewport);
    GLboolean orig_depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean orig_blend = glIsEnabled(GL_BLEND);
    GLboolean orig_cull = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    static float frame_count = 0.0f;
    frame_count += 1.0f;

    float mvp[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    for (int i = 0; i < pipe->pass_count; i++) {
        Pass *pass = &pipe->passes[i];
        if (!pass->program) continue;

        if (i == pipe->pass_count - 1) {
            glBindFramebuffer_ptr(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, screen_w, screen_h);
        } else {
            glBindFramebuffer_ptr(GL_FRAMEBUFFER, pass->fbo.fbo);
            glViewport(0, 0, pass->fbo.width, pass->fbo.height);
        }

        glUseProgram_ptr(pass->program);

        GLuint prev_tex = (i == 0) ? capture_texture : pipe->passes[i - 1].fbo.texture;

        int src_w = (i == 0) ? screen_w : pipe->passes[i - 1].fbo.width;
        int src_h = (i == 0) ? screen_h : pipe->passes[i - 1].fbo.height;

        int pass_out_w = (i == pipe->pass_count - 1) ? orig_viewport[2] : pass->fbo.width;
        int pass_out_h = (i == pipe->pass_count - 1) ? orig_viewport[3] : pass->fbo.height;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, prev_tex);
        if (pass->uniforms.u_Texture != -1) {
            glUniform1i_ptr(pass->uniforms.u_Texture, 0);
        }

        if (pass->uniforms.u_OrigTexture != -1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, capture_texture);
            glUniform1i_ptr(pass->uniforms.u_OrigTexture, 1);
            glActiveTexture(GL_TEXTURE0);
        }

        int current_unit = 2;

        // PassPrev Uniforms
        for (int n = 1; n <= i; n++) {
            int src_idx = i - n;
            if (pass->uniforms.pass_prev_tex[n] != -1) {
                GLuint src_tex = (src_idx < 0) ? capture_texture : pipe->passes[src_idx].fbo.texture;
                glActiveTexture(GL_TEXTURE0 + current_unit);
                glBindTexture(GL_TEXTURE_2D, src_tex);
                glUniform1i_ptr(pass->uniforms.pass_prev_tex[n], current_unit);
                current_unit++;
            }

            int src_pass_w = (src_idx < 0) ? screen_w : pipe->passes[src_idx].fbo.width;
            int src_pass_h = (src_idx < 0) ? screen_h : pipe->passes[src_idx].fbo.height;

            if (pass->uniforms.pass_prev_tex_size[n] != -1) {
                glUniform2f_ptr(pass->uniforms.pass_prev_tex_size[n], (float)src_pass_w, (float)src_pass_h);
            }
            if (pass->uniforms.pass_prev_input_size[n] != -1) {
                glUniform2f_ptr(pass->uniforms.pass_prev_input_size[n], (float)src_pass_w, (float)src_pass_h);
            }
        }

        // Alias Uniforms
        for (int p = 0; p < pipe->pass_count; p++) {
            if (pipe->passes[p].alias[0] != '\0') {
                if (pass->uniforms.alias_tex[p] != -1) {
                    glActiveTexture(GL_TEXTURE0 + current_unit);
                    glBindTexture(GL_TEXTURE_2D, pipe->passes[p].fbo.texture);
                    glUniform1i_ptr(pass->uniforms.alias_tex[p], current_unit);
                    current_unit++;
                }

                if (pass->uniforms.alias_tex_size[p] != -1) {
                    glUniform2f_ptr(pass->uniforms.alias_tex_size[p], (float)pipe->passes[p].fbo.width, (float)pipe->passes[p].fbo.height);
                }

                if (pass->uniforms.alias_input_size[p] != -1) {
                    glUniform2f_ptr(pass->uniforms.alias_input_size[p], (float)pipe->passes[p].fbo.width, (float)pipe->passes[p].fbo.height);
                }
            }
        }

        // Pipeline Textures
        for (int t = 0; t < pipe->texture_count; t++) {
            if (pass->uniforms.user_tex[t] != -1) {
                glActiveTexture(GL_TEXTURE0 + current_unit);
                glBindTexture(pipe->textures[t].target, pipe->textures[t].id);
                glUniform1i_ptr(pass->uniforms.user_tex[t], current_unit);
                current_unit++;
            }
        }
        glActiveTexture(GL_TEXTURE0);

        if (pass->uniforms.u_MVPMatrix != -1) glUniformMatrix4fv_ptr(pass->uniforms.u_MVPMatrix, 1, GL_FALSE, mvp);
        if (pass->uniforms.u_time != -1) glUniform1f_ptr(pass->uniforms.u_time, time);
        if (pass->uniforms.u_frame_count != -1) glUniform1f_ptr(pass->uniforms.u_frame_count, frame_count);
        if (pass->uniforms.u_FrameCount != -1) glUniform1i_ptr(pass->uniforms.u_FrameCount, (int)frame_count);
        if (pass->uniforms.u_FrameDirection != -1) glUniform1i_ptr(pass->uniforms.u_FrameDirection, 1);
        if (pass->uniforms.u_resolution != -1) glUniform2f_ptr(pass->uniforms.u_resolution, (float)screen_w, (float)screen_h);

        if (pass->uniforms.u_TextureSize != -1) glUniform2f_ptr(pass->uniforms.u_TextureSize, (float)src_w, (float)src_h);
        if (pass->uniforms.u_InputSize != -1) glUniform2f_ptr(pass->uniforms.u_InputSize, (float)src_w, (float)src_h);
        if (pass->uniforms.u_OutputSize != -1) glUniform2f_ptr(pass->uniforms.u_OutputSize, (float)pass_out_w, (float)pass_out_h);

        if (pass->uniforms.u_OrigTextureSize != -1) glUniform2f_ptr(pass->uniforms.u_OrigTextureSize, (float)screen_w, (float)screen_h);
        if (pass->uniforms.u_OrigInputSize != -1) glUniform2f_ptr(pass->uniforms.u_OrigInputSize, (float)screen_w, (float)screen_h);

        if (glBindVertexArray_ptr && g_quad_vao) {
            glBindVertexArray_ptr(g_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray_ptr(0);
        }
    }

    if (orig_depth) glEnable(GL_DEPTH_TEST);
    if (orig_blend) glEnable(GL_BLEND);
    if (orig_cull) glEnable(GL_CULL_FACE);

    glActiveTexture(orig_active_tex);
    glUseProgram_ptr(orig_program);
    glBindFramebuffer_ptr(GL_FRAMEBUFFER, orig_fbo);
    if (glBindVertexArray_ptr) glBindVertexArray_ptr(orig_vao);
    if (glBindBuffer_ptr) glBindBuffer_ptr(GL_ARRAY_BUFFER, orig_vbo);
}

# Injectglix

A lightweight post-processing shader injector for native Linux OpenGL applications using `LD_PRELOAD`. Hooks into `glXSwapBuffers`, `eglSwapBuffers`, and `SDL_GL_SwapWindow` to intercept frame buffers, apply multi-pass shaders, and support hot-reloading.

## Features

- Hooks `glXSwapBuffers`, `eglSwapBuffers`, and `SDL_GL_SwapWindow` dynamically.
- Intercepts drawing surfaces and applies multi-pass fragment/vertex shaders.
- Live hot-reloading for shaders and configuration without restarting games or apps.
- Custom texture loading and multi-pass pipeline support via configuration file.

## Prerequisites

- GCC or Clang compiler
- OpenGL development headers (`libgl1-mesa-dev`, `libegl1-mesa-dev`, `libx11-dev`)

## Building

Compile the shared library using GCC:

```bash
gcc -shared -fPIC -o libinjector.so injector.c renderer.c -lGL -ldl -lm
```
## Usage
```
LD_PRELOAD=./libinjector.so ./your_opengl_game
```
## Configuration

The injector automatically looks for `shaders/pipeline.cfg` or the environment variable `SHADER_CONFIG`.

## Configuration Format:

```plaintext
# Format: pass_index, shader_path, format, scale_x, scale_y, filter, alias
0, shaders/passthrough.glsl, rgba8, 1.0, 1.0, linear, Pass1

# External texture definition
texture, u_NoiseTex, shaders/noise.png, 2d, repeat
```
## License

This project is licensed under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text)

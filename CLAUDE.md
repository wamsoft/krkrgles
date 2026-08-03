# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A kirikiri Z (吉里吉里Z) engine plugin (`krkrgles.dll`) that exposes OpenGL ES rendering to TJS2 scripts. GL ES is reached through **ANGLE/EGL** — the plugin loads `libEGL`/`libGLESv2` (ANGLE) at runtime, so on Windows the actual backend is Direct3D (D3D11 by default, D3D9 when `forceD3D9` is passed). The plugin's job is to render layer bitmaps into an offscreen FBO via GL ES and copy the result back into kirikiri `Layer` image buffers.

## Build

The build is driven by CMake presets + vcpkg, wrapped by a `Makefile`. From the repo root:

```sh
make            # configure (prebuild) is implicit via BUILD_PATH; runs build
make prebuild   # cmake --preset <PRESET>  (configure only)
make build      # cmake --build ...
make clean
```

`PRESET` auto-selects from the OS (`x64-windows` on Windows). Override per invocation, e.g. `make build PRESET=x86-windows BUILD_TYPE=Debug`. Output lands in `build/<preset>/`. `VCPKG_ROOT` must be set (the toolchain file is referenced from it). There is **no test suite and no linter** — `manual.tjs` is API documentation only, not a runnable test.

### Build dependencies (sibling directories, not vendored here)

`CMakeLists.txt` includes `../tp_stub/krkrz.cmake`, which provides the `krkrz_plugin(...)` macro and the `NCBIND` option. The build therefore requires these to exist as siblings of this repo:

- `../tp_stub/` — kirikiri Z plugin stub (`tp_stub.h`, `krkrz.cmake`). Override location with `-DTPSTUB_DIR=...`.
- `../ncbind/` — the ncbind binding framework (pulled in by the `NCBIND` flag).

`glad/` (EGL + GLES2 loader) **is** vendored here and built as a static lib subproject.

## Architecture

All C++ → TJS2 binding is done through **ncbind** macros at the bottom of `src/GLES.cpp` (`NCB_REGISTER_CLASS`, `NCB_METHOD`, `NCB_PROPERTY`, `Factory`). Two classes are exported to TJS2:

- **`GLESAdaptor`** (`src/GLES.cpp`) — the main object. Constructed with a kirikiri `Window` (`new GLESAdaptor(window [, forceD3D9])`). Owns the EGL display/context/surface and the offscreen `GLFrameBufferObject`. Key TJS methods: `capture(layer, callback, param, color)` renders via a TJS callback into the FBO then reads pixels back into the layer; `drawLayer`/`copyLayer` draw a layer or `GLESTexture` with an affine transform; `makeCurrent`, `setScreenSize`, `blendMode` property.
- **`GLESTexture`** (`src/GLES.cpp`) — a pre-uploaded texture built from a layer's bitmap (`load(layer)`), so repeated draws skip re-upload.

Internal helpers (not TJS-exposed):

- `GLFrameBufferObject` (`GLFrameBufferObject.{h,cpp}`) — offscreen render target (texture + FBO + PBO). Comment in the header notes the format basically must be `GL_RGBA`/`GL_BGRA_EXT` or some GPUs misbehave.
- `GLTexture` + `GLTextureDrawer` (`GLTexture.{h,cpp}`) — texture wrapper (uploads via PBO + `glMapBufferRange`) and a minimal textured-quad shader pass used by `drawLayer`. The shader source is inline string literals in `GLTexture.cpp`.
- `GLShaderUtil.{h,cpp}` — shader/program compile helpers, copied/trimmed from ANGLE's `shader_utils`.
- `OpenGLError.cpp` + `CheckGLErrorAndLog(...)` macro (`OpenGLHeader.h`) — error logging, **active only in debug** (`#ifndef NDEBUG`).

### Things that bite

- **Source files are UTF-8 encoded**, comments are Japanese (`.vscode/settings.json` → `files.encoding: utf8`). The MSVC build forces `/utf-8` only when this is the top-level project (see `CMakeLists.txt`) — when built as part of the kirikiri tree the parent must supply the encoding flag, otherwise the Japanese comments warn (C4828). String literals in code are ASCII (shaders, `TJS_W(...)` messages), so encoding only affects comments.
- **EGL/GLES are loaded once, globally.** `egl_inited`/`gles_inited` are static; `contextList` is a static shared list and new contexts share with `contextList[0]`. Multiple `GLESAdaptor` instances share GL objects.
- **Coordinate/format conventions:** layer bitmaps are BGRA and **vertically flipped** relative to GL — every upload/readback path (`capture`, `drawLayer`, `GLESTexture::load`) reverses rows by hand. `drawLayer`'s `_position[]` math maps pixel coords into NDC using `mWidth/2`,`mHeight/2`.
- Layer data is accessed via `ncbPropAccessor` reading kirikiri properties by name (`mainImageBuffer`, `mainImageBufferForWrite`, `mainImageBufferPitch`, `width`, `height`) — these are kirikiri `Layer` API contracts, not local fields.
- `blendMode` values map to kirikiri's `tTVPBlendMode` enum (mirrored at the top of `GLES.cpp`); `ApplyBlendMode()` translates them to `glBlendFuncSeparate`/`glBlendEquationSeparate` calls.

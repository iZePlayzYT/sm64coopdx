# sm64rt

Fork of [sm64pc/sm64ex](https://github.com/sm64pc/sm64ex) that adds support for [RT64](https://github.com/DarioSamo/sm64rt-legacy-renderer), a hardware-accelerated real-time path tracer.

## Alpha Phase

**At the moment, this mod is currently in its alpha phase and should only be used by those willing to tolerate issues such as visual glitches and performance problems**. This project is subject to have major changes in the future in anything from its architecture, design, aesthetic and performance. Any help towards solving these issues is welcome.

Please do not report issues that don't provide new information. Remember to check if your problem has already been reported on the [issue tracker](https://github.com/DarioSamo/sm64rt/issues).

Performance is highly dependent on the target resolution and it still has plenty of room for improvement. Using upscalers like **NVIDIA DLSS** and **AMD FSR** will help significantly in achieving higher framerates.

## Requirements
* Windows 10 (2004 or newer)
* [Microsoft Visual C++ Redistributable for Visual Studio 2019](https://aka.ms/vs/16/release/vc_redist.x64.exe)
* GPU with DXR support (NVIDIA Drivers version 470 or newer **required** for DLSS support)
  - Unsure if you can run the mod? [Download and run the RT64 sample instead!](https://github.com/DarioSamo/RT64/releases/download/test-sample-v1/rt64sample-v1.zip)

## Features
* Fully path-traced renderer.
* Custom level lighting for all stages.
* Dynamic sphere lights for objects and particles.
* Classic Phong shading.
* Custom material properties based on texture names or geometry layouts.
* Normal map support.
* Real-time raytraced shadows, reflections, refractions and global illumination.
* Real-time denoiser.
* NVIDIA DLSS (Deep Learning Super Sampling).
* AMD FSR 2 (FidelityFX Super Resolution).
* Intel XeSS (Intel Xe Super Sampling).

## Building
For building instructions, please refer to the [sm64ex wiki](https://github.com/sm64pc/sm64ex/wiki) and follow the process as normal with these additional build flags:

* RENDER_API=RT64 (Required to use RT64 as the renderer)

* EXTERNAL_DATA=1 (Required for associating textures to the renderer's material properties)

The repository already comes with a prebuilt binary and the compatible header file for RT64 to make the build process easier. If you wish to build this module yourself, you can do it from the [RT64 repository](https://github.com/DarioSamo/RT64) instead.

## Render96 Support
A native version of sm64rt is currently integrated into Render96ex. You don't need to build this repository to access it.

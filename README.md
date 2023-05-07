
# kram, kram.exe
C++11 main to libkram to create CLI tool.  Encode/decode/info on PNG/KTX/KTX2/DDS files with LDR/HDR and BC/ASTC/ETC2.  Runs on macOS/win.

# libkram.a, libkram-ios.a, kram.lib
C++11 library from 200 to 800KB in size depending on encoder options.  Compiles for iOS (ARM), macOS (ARM/Intel), win (Intel).

# hslparser
Parses HLSL syntax and generates readable HLSL/MSL code without transpiling.  DXC is then used to compile to spirv.
https://github.com/alecazam/kram/tree/main/hlslparser

# kram-thumb-win.dll
Windows thumbnailer for DDS/KTX/KTX2.  Go to build or bin folder.  Install with "regsvr32.exe kram-thumb-win.dll".  Uninstall with "regsvr32.exe /u kram-thumb-win.dll"

https://github.com/alecazam/kram/tree/main/kram-thumb-win

https://github.com/iOrange/QOIThumbnailProvider

# kramv.app
ObjC++ viewer for PNG/KTX/KTX2/DDS supported files from kram.  Uses Metal compute and shaders, eyedropper, grids, debugging, preview.  Supports HDR and all texture types.  Mip, face, volume, and array access.  No dmg yet, just drop onto /Applications folder.  Runs on macOS (arm64/x64).  Generates Finder thumbnails and QuickLook previews via modern macOS app extension mechanisms.

Diagrams and screenshots can be located here:
https://www.figma.com/file/bPmPSpBGTi2xTVnBDqVEq0/kram

#### Releases includes builds for macOS (Xcode 14.3 - arm64/x64/clang) and Windows x64 (VS 2022 - x64/clang).  kramv for macOS, kram for macOS/Win, libkram for macOS/iOS/Win, win-thumb-kram for Win.  Android library via NDK is possible, but f16 support is spotty on devices.

### About kram
kram is a wrapper to several popular encoders.  Most encoders have sources, and have been optimized to use very little memory and generate high quality encodings at all settings.  All kram encoders are currently CPU-based.  Some of these encoders use SSE, and a SSE to Neon layer translates those.  kram was built to be small and used as a library or app.  It's also designed for mobile and desktop use.  The final size with all encoders is under 1MB, and disabling each encoder chops off around 200KB down to a final 200KB app size via dead-code stripping.  The code should compile with C++11 or higher.

kram focuses on sending data efficiently and precisely to the encoders.  kram handles srgb and premul at key points in mip generation.  Source files use mmap to reduce memory, but fallback to file ops if that fails.  Temp files are generated for output, and then renamed in case the app fails or is terminated.  Mips are done in-place, and mip data is written out to a file to reduce memory usage. kram leaves out BC2 and etcrgb8a1 and PVRTC.  Also BC6 still needs an encoder, and ASTC HDR encoding needs a bit more work to pull from half4/float4 source pixels.  

Many of the encoder sources can multithread a single image, but that is unused.  kram is designed to batch process one texture per core/thread via a python script or a C++11 task system inside kram.  This can use more ram depending on the core count.  Texture-per-process and scripted modes currently both take the same amount of CPU time, but scripted mode is best if kram ever adds GPU-accelerated encoding.

Similar to a makefile system, the script sample kramtexture.py uses modstamps to skip textures that have already been processed.  If the source png/ktx/ktx2 is older than the output, then the file is skipped.  Command line options are not yet compared, so if those change then use --force on the python script to rebuild all textures.  Also a crc/hash could be used instead when modstamp isn't sufficient or the same data could come from different folders.

### About kramv
kramv is a viewer for the BC/ASTC/ETC2 LDR/HDR KTX/KTX2/DDS textures generated by kram from LDR PNG and LDR/HDR KTX/KTX2/DDS sources.  kramv decodes ASTC/ETC2 textures on macOS Intel, where the GPU doesn't support them.  macOS with Apple Silicon supports all three formats, and doesn't need to decode.   

kramv uses ObjC++ with the intent to port to Windows C++ as time permits.  Uses menus, buttons, and keyboard handling useful for texture triage and analysis.  Drag and drop folders, bundles, and click-to-launch are supported.  Recently used textures/folders/bundles are listed in the menu.  The app currently shows a single document at a time.  Subsequent opens reuse the same document Window.  With bundles and folders, kramv will attempt to pair albedo and normal maps together by filename for the preview. 

Preview mode provides lighting, sdf cutoff, and mip visuals for a given texture.  Multiple shapes can help identify inconsistent normal maps.  The u-axis advances counterclockwise, and v-axis advances down on the shapes.  +Y OpenGL normals are assumed, not -Y DirectX convention.  Lighting appears up and to the right when normal maps are correctly specified.   

In non-preview mode, point sampling in a pixel shader is used to show exact pixel values of a single mip, array, and face.  Debug modes provide pixel analysis.  KramLoader shows synchronous cpu upload to a private Metal texture, but does not yet supply the underlying KTXImage.  Pinch-zoom and pan tries to keep the image from onscreen, and zoom is to the cursor so navigating feels intuitive.

Compute shaders are used to sample a single pixel sample from the gpu texture for the eyedropper.  This simplifies adding more viewable formats in the future, but there is not a cpu fallback.  Normal.z is reconstructed and displayed in the hud, and linear and srgb channels are shown.

```
Formats - R/RG/RGBA 8/16F/32F, BC/ETC2/ASTC,  RGB has limited import support
Container Types - KTX, KTX2, PNG
Content Types - Albedo, Normal, SDF, Height
Debug modes - transparent, color, non-zero, gray, +x, +y, xy >= 1
Texture Types - 1darray (no mips), 2d, 2darray, 3d (no mips), cube, cube array

⇧ decrement any advance/toggle listed below

? - show keyboard shortcuts
P - toggle preview, disables debug mode, shows lit normals, and mips and filtering are enabled
G - advance grid, none, pixel grid, block grid, atlas grid (32, 64, 128, 256),
D - advance debug mode
H - toggle hud
U - toggle ui
V - toggle vertical vs. horizontal buttons
I - show texture info in overlay
W - toggle wrap/address filter, scales uv from [0,1] to [0,2] and changes sampler to wrap/repeat
A - show all - arrays, faces, slices and mips all on-screen

1/2/3/4 - show rgba channels in isolation, alpha as grayscale
7 - toggle signed/unsigned
8 - toggle shader premul, shader does this post-sample so only correct for point-sampling not preview

R - reload from disk if changed, zoom to fit (at 1x with ⇧)
0 - fit the current mip image to 1x, or fit view.  (at 1x with ⇧).

Y - advance array 
F - advance face/slide
M - advance mip

S - advance shape mesh (plane, unit box, sphere, capsule), displays list, esc to get out of list
C - advance shape channel (depth, uv, face normal, vtx normal, tangent, bitangent, mip)
L - advance lighting mode (none, diffuse, diffuse + specular)
T - toggle tangent generation

↓ - advance bundle/folder image (can traverse zip of ktx/ktx2 files), displays list, esc to get out of list
→ - advance counterpart (can see png, then encodes if viewing folders).  Not yet finished.

```

### Limitations

Texture processing is complex and there be dragons.  Just be aware of some of the limitations of kram and encoding.  Lossy compression can only solve so much.  ASTC and BC4-7 are newer formats, but all formats have time and quality tradeoffs.  And encoder quality and issues remain.  WebGL is still often stuck with older formats due to lack of implemented extensions.  And all formats need endpoints/selectors reordering and zstd compression that KTX2 offers.  I added a platform called "any" to with KTX2 holding UASTC+zstd and also ETC2/ASTC/BC+zstd.  The scripts bundle up textures in an archive, but these should go to resource packs and asset catalogs which get signed and can have ODR applied.

```
GPU - none of the encoders use the GPU, so cpu threading and multi-process is used

Rescale Filtering - 1x1 point filter
Mip filtering - 2x2 box filter that's reasonable for pow2, and a non-linear filters for non-pow2 so there is no pixel shift 
  done in linear space using half4 storage, in-place to save mem

1D array - no mip support due to hardware, no encoding
3D textures - no mip support, uses ASTC 2d slice encode used by Metal/Android, not exotic ASTC 3d format
    
BC/ETC2/ASTC - supposedly WebGL requires pow2, and some implementation need top multiple of 4 for BC/ETC2

These formats are disabled:
BC1 w/alpha - may re-enable 3 color for black + rgb
BC2 - not useful
ETC2_RGB8A1 - broken in ETC2 optimizations

BC1 - artifacts from limits of format, artifacts from encoder, use BC7 w/2x memory

ASTC LDR - rrr1, rrrg/gggr, rgb1, rgba must be followed to avoid endpoint storage, requires swizzles
ASTC HDR - encoder uses 8-bit source image, need 16f/32f passed to encoder, no hw L+A mode

R/RG/RGBA 8/16F/32F - use kram or ktx2ktx2+ktx2sc to generate supercompressed ktx2
R8/RG8/R16F - input/output rowBytes not aligned to 4 bytes to match KTX spec, code changes needed

PVRTC - unsupported, no open-source encoders, requires pow2 size

Containers
PVR/Basis/Crunch - unsupoorted 

KTX - only uncompressed, mip levels are unaligned to block size from 4 byte length at chunk 0 
  metadata/props aren't standardized or prevalent
  libkram supports only text props for display in kramv

KTX2 - works in kram and viewer, has aligned levels of mips when uncompressed, 
  libkram supports None/Zlib/Zstd supercompression for read/write
  libkram does not support UASTC/BasisLZ yet
  
DDS - works in kram and viewer, no mip compression, only BC and explicit formats, extended for ASTC/ETC
  kram/kramv only support newer DX10 style DDS format.  Can view in Preview on macOS too.
  DDSHelper provides load/save.  Pixel data ordered by chunk instead of by mips.  No metadata.
  
```

### An example pipeline

```
At build:

* Lossless 8u/16F/32F KTX2 sources 2D, 2D, cube, and 2D atlas textures with zstd mips.  Editing these in Photoshop/Gimp is still an issue.
* Need to stop basing pipelines around PNG.  This is a limited source format, but supported by editors.
* Textures should be higher resolution, and checked into source control (git-lfs or p4).  
* Some sort of scripting to supply encoder preset index for textures. 

* Drop mips and encode to KTX using kram using the encoder preset
* Build 2D array or 2D atalas assets and name/uv locations for those assets.
* Convert KTX to KTX2 via ktx2ktx2 + ktx2sc as lossy encoded ETC2/BC/ASTC+zstd or UASTC+rdo+zstd
* Bundle into an Asset Catalog (macOS/iOS), or resource pack (Android) for slicing and on-demand resource loading

At runtime:

* Mmap load all KTX2 textures read-only into memory using KramLoader.  This is the backing-store.
* Decode smaller faces/slices/array and their mips and upload to staging buffer and then gpu transfer/twiddle to private texture. 
* For example, can upload all lower mips in 1/3rd the space and skip all the top mips.  Textures without mips cannot do this.

* Use sparse texturing hardware or readback to indicate what mips are accessed by the hardware.
* Purge top mips of large unused textures, but keep the bottom mips.
* Upload top mips as needed and memory allows.
```

### Building
kram uses an explicit Xcode workspace and projects on Apple platforms.  CMake can't clean, build workspaces, or handle app extensions needed for thumbnails/previews.  I spent a lot of time trying to keep CMake working since it keeps kram from being tied to Xcode releases, but I also wanted to add better Finder integration.  These all live in 'build2' to distinguish from the 'build' directory created for CMake.  Like CMake, the cibuild.h script runs xcodebuild from the command line to generate all the libraries and apps into the bin directory.  Note that Xcode has never been able to simultaneously open the same project included in different workspaces, so organize derivative workspaces carefully.

```
./scripts/cibuild.h

open build2/kram.xcworkspace

```


kram was using CMake to setup the projects and build, but it doesn't support workspaces, clean, or the thumbnail/preview extension linking.  As a result, kramv.app, kram, and libkram are generated.  So I'm building kramv.app and everything with a custom Xcode project now.  The library can be useful in apps that want to include the decoder, or runtime compression of gpu-generated data.

For Mac, the CMake build is out-of-source, and can be built from the command line, or debugged from the xcodeproj that is built.  Ninja and Makefiles can also be generated from cmake, but remember to trash the CMakeCache.txt file.

```
mkdir build
cmake .. -G Xcode

cmake --build . --config Release
or
open kramWorkspace.xcodeproj
or
cmake --install ../bin --config Release
```

For Windows, CMake is still used. CMake build libkram, kramc, and kram-thumb-win.dll. This uses the clang compiler and x64 only.

```
mkdir build
cmake .. -G "Visual Studio 16 2019" -T ClangCL -A x64
or
cmake .. -G "Visual Studio 17 2022" -T ClangCL -A x64

cmake --build . --config Release
or
open kramWorkspace.sln
or
# not sure if install works on Win
cmake --install ../bin --config Release
```

There are various CMake settings that control the various encoders.  Each of these adds around 200KB.  I tested with each of these turned off, so code should be isolated.  The project will still show all sources.

* -DATE=ON
* -DASTCENC=ON
* -DBCENC=ON
* -DSQUISH=ON
* -DETCTOOL=ON

### Commands
* encode - encode/decode block formats, mipmaps, fast sdf, premul, srgb, swizzles, LDR and HDR support, 16f/32f
* decode - can convert any of the encode formats to s/rgba8 ktx files for display 
* info   - dump dimensions and formats and metadata/props from png and ktx files
* script - send a series of kram commands that are processed in a task system.  Ammenable to gpu acceleration.

### Sample Scripts
* kramTextures.py  - python3 example that recursively walks directories and calls kram, or accumulates command and runs as a script
* formatSources.sh - zsh script to run clang_format on the kram source directory (excludes open source)
* fixfinder.sh - after updating /Applications, this flushes any cached copy of kram.app from LaunchServices

To demonstrate how kram works, scripts/kramtextures.py applies platform-specific presets based on source filenames endings.  The first form executes multiple kram processes with each file using a Python ThreadPoolExecutor.  The second generates a script file, and then runs that in a C++ task system inside kram.  The scripting system would allow gpu compute of commands, and more balanced memory and thread usage.


```
cd build

# this will install "click" and other python package dependencies
macOS
pip3 install -r ../scripts/requirements.txt

Win
python3.exe -m pip install -U pip
python3.exe -m pip install -r ../scripts/requirements.txt

# Gen ktx using 8 processes, and bundles the results to a zip file
../scripts/kramTextures.py --jobs 8 -p android --bundle

# this writes out a script of all commands and runs on threads in a single process
../scripts/kramTextures.py --jobs 8 -p ios --script
../scripts/kramTextures.py --jobs 8 -p mac --script --force 
../scripts/kramTextures.py --jobs 8 -p win --script --force 

# Generate ktx2 output, and then bundle them into a zip
../scripts/kramTextures.py -p any -c ktx2 --bundle
../scripts/kramTextures.py -p android -c ktx2 --bundle --check

# Generate dds output, and then bundle them into a zip
../scripts/kramTextures.py -p any -c dds --bundle
../scripts/kramTextures.py -p android -c dds --bundle --check

# Generate textures for all platforms
../scripts/kramTests.sh 

```

### Sample Commands
To test individual encoders, there are tests cases embedded into kram.  Also individual textures can be processed, or the script records these commands and executes the encodes on multiple cores.

```
cd build
./Release/kram -testall
./Release/kram -test 1002

# for ktx
./Release/kram encode -f astc4x4 -srgb -premul -quality 49 -mipmax 1024 -type 2d -i ../tests/src/ColorMap-a.png -o ../tests/out/ios/ColorMap-a.ktx
./Release/kram encode -f etc2rg -signed -normal -quality 49 -mipmax 1024 -type 2d -i ../tests/src/collectorbarrel-n.png -o ../tests/out/ios/collectorbarrel-n.ktx
./Release/kram encode -f etc2r -signed -sdf -quality 49 -mipmax 1024 -type 2d -i ../kram/tests/src/flipper-sdf.png -o ../tests/out/ios/flipper-sdf.ktx

# for ktx (without and with zstd compression)
./Release/kram encode -f astc4x4 -srgb -premul -quality 49 -mipmax 1024 -type 2d -i ../tests/src/ColorMap-a.png -o ../tests/out/ios/ColorMap-a.ktx2
./Release/kram encode -f astc4x4 -srgb -premul -quality 49 -mipmax 1024 -type 2d -zstd 0 -i ../tests/src/ColorMap-a.png -o ../tests/out/ios/ColorMap-a.ktx2

```

### Open Source Encoder Usage
This app would not be possible without the open-source contributions from the following people and organizations.  These people also inspired me to make this app open-source, and maybe this will encourage more great tools or game tech.

kram includes the following encoders/decoders:

| Encoder  | Author           | License     | Encodes                     | Decodes | 
|----------|------------------|-------------|-----------------------------|---------|
| BCEnc    | Rich Geldreich   | MIT         | BC1,3,4,5,7                 | same    |
| Squish   | Simon Brown      | MIT         | BC1,3,4,5                   | same    |
| ATE      | Apple            | no sources  | BC1,4,5,7 ASTC4x4,8x8 LDR   | LDR     |
| Astcenc  | Arm              | Apache 2.0  | ASTC4x4,5x5,6x6,8x8 LDR/HDR | same    |
| Etc2comp | Google           | MIT         | ETC2r11,rg11,rgb,rgba       | same    |
| Explicit | Me               | MIT         | r/rg/rgba 8u/16f/32f        | none    |
| Compress | AMD              | MIT         | BC6                         | same    |
| GTLFKit  | Warren Moore     | MIT         | none                        | gltf    |

```
ATE
Simple wrapper for encode/decode around this.
Identifies encode/decode formats off version.

BCEnc
Commented out some unused code to suppress warnings
Hooked in some SIMD code.

Squish
Simplified to single folder.
Replaced sse vector with float4/a for ARM/Neon support.

Astcenc v3.4
Provide rgba8u source pixels.  Converted to 32f at tile level.
Improved 1 and 2 channel format encoding (disable now).
Avoid reading off end of arrays with padding.
Support 2d array of src pixels instead of 3d.
Force AVX and SSE path, and implement using sse2neon emlation on Neon.

Etc2comp 
Simplified to single folder.
Keep r11 and rg11 in integer space. 6x faster.
Memory reduced signficantly.  One block allocated.
Single pass encoder works on one block at a time, and does not skip blocks.
Multipass and multithread algorithm sorts vector, and split out blockPercentage from iteration count.
RGB8A1 is broken.
Optimized encodes by inlining CalcPixelError. 2x faster.
Reduced memory by 4x and passing down rgba8u instead of rgba32f.  Converted to 32f at tile level.

```

### Open source usage

kram includes additional open-source:

| Library        | Author             | License   | Purpose                   |
|----------------|--------------------|-----------|---------------------------|
| lodepng        | Lode Vandevenne    | MIT       | png encode/decode         |
| SSE2Neon       | John W. Ratcliff   | MIT       | sse to neon               |
| heman          | Philip Rideout     | MIT       | parabola EDT for SDF      |
| TaskSystem     | Sean Parent        | MIT       | C++11 work queue          |
| tmpfileplus    | David Ireland      | Moz 2.0   | fixes C tmpfile api       |
| mmap universal | Mike Frysinger     | Pub       | mmap on Windows           |
| zstd           | Yann Collett (FB)  | BSD-2     | KTX2 mip decode           |
| miniz	         | Rich Gelreich      | Unlicense | bundle support via zip    |
| gltfKit        | Warren Moore       | MIT       | gltf decoder/renderer     |

kram-thumb-win.dll addtional open-source

| Library        | Author             | License   | Purpose                   |
|----------------|--------------------|-----------|---------------------------|
| QOI thumbnails | iOrange            | MIT       | win thumbnailer           |

#### Open source changes

* lodepng - altered header paths.
* SSE2Neon - updated to newer arm64 permute instructions.
* heman - altered sdf calcs for mipgen off largest sdf mip
* TaskSystem - altered to control thread count, 
* tmpfileplus - small changes to work on Mac/Win better allow extension suffix
* mmap universal - may leak a file mapping handle on Win.
* zstd - using single file version of zstd for decode, disabled encode paths
* miniz - expose raw data and offset for mmap-ed zip files, disabled writer, disable read crc checks, in .cpp file
* gltfkit - several warning fixes, changes to support kram texture loader

## kram unstarted features:

* Tile command for SVT tiling
* Block twiddling support for consoles
* Merge command to combine images (similar to ImageMagick)
* Atlas command to atlas to 2D and 2D array textures.  Display names, show bounds of atlases.  Have -chunks arg now.
* 3D chart flattening.
* Motion vector direction analysis.
* Split view comparison rendering.  Move horizontal slider like ShaderToy.
* Add GPU encoder (use compute in Metal/Vulkan)
* Save prop with args and compare args and modstamp before rebuilding to avoid --force
* Multichannel SDF
* Plumb half4/float4 through to BC6 encoding.  Sending 8u.
* Run srgb conversion on endpoint data after fitting linear color point cloud
* PSNR stats off encode + decode
* Dump stats on BC6/7 block types, and ASTC void extent, dual-plane, etc
* Iterate through block encoded types on source images to help artists see tradeoffs

### Test Images

Some of these images like collectorbarrel-a and Toof.a look grayscale but are not. 
Some of the encoders turn these non-opaque, and generate alpha of 254/255.

* color_grid-a from ktx/ktx2 samples
* ColorMap-a from Apple's sample apps to test premultiplied alpha and srgb.
* flipper-sdf image taken from EDT paper that inspired heman SDF.
* collectorbarrel-n/a from Id's old GPU BC1/3 compression article.
* Toof-a is my own artwork drawn in Figma

### Timings for test suite

These are basic timings running kram encoding for all the specific platform test cases using kramTexture.py 
Impressive that M1 wins on the Android test case by over 2x, and is close in the others.

* Date: 1/16/21
* 2020 M1 13" Macbook Air, 3.4Ghz, 4+4 core M1, 8GB
* 2019 16" Macbook Pro, 2.3Ghz, 8/16 core i9, 16GB

Timings
* Any - Basis supercompress via ktxsc is long with UASTC + RDO

| Sys  | macOS  | iOS    | Android | Any    |
|------|--------|--------|---------|--------|
| M1   | 0.643s | 4.603s |  6.441s |        |
| i9   | 0.970s | 3.339s | 14.617s | 75.0s  |

Bundle Sizes
* KTX file is compressed before archiving, but decompress entire file to disk to mmap
* KTX2/Basis is simply stored in archive, mips already zstd compressed, can mmap as compressed backing store

| Sys    | macOS   | iOS      | Android | Any         |
|--------|---------|----------|---------|-------------|
| PNG    | 460K    |          |         |             |
| KTX    | 507K    | 517K     | 281K    |             |
| KTX2   | 481K    | 495K     | 288K    | 471K        | 
| Format | BC4/5/7 | ASTC/ETC | ETC2    | Basis-UASTC |
| Compr. | zstd    | zstd     | zstd    | RDO+zstd    |


### Syntax
```
kram[encode | decode | info | script | ...]
Usage: kram encode
	 -f/ormat (bc1 | astc4x4 | etc2rgba | rgba16f)
	 [-srgb] [-signed] [-normal]
	 -i/nput <source.png | .ktx>
	 -o/utput <target.ktx | .ktxa>

	 [-type 2d|3d|..]
	 [-e/ncoder (squish | ate | etcenc | bcenc | astcenc | explicit | ..)]
	 [-resize (16x32 | pow2)]

	 [-mipnone]
	 [-mipmin size] [-mipmax size]

	 [-chunks 4x4]
	 [-swizzle rg01]
	 [-avg rxbx]
	 [-sdf]
	 [-premul]
	 [-prezero]
	 [-quality 0-100]
	 [-optopaque]
	 [-v]
   
         [-test 1002]
         [-testall]

OPTIONS
	-type 2d|3d|cube|1darray|2darray|cubearray

	-format [r|rg|rgba| 8|16f|32f]	Explicit format to build mips and for hdr.
	-format bc[1,3,4,5,7]	BC compression
	-format etc2[r|rg|rgb|rgba]	ETC2 compression - r11sn, rg11sn, rgba, rgba
	-format astc[4x4|5x5|6x6|8x8]	ASTC and block size. ETC/BC are 4x4 only.

	-encoder squish	bc[1,3,4,5]
	-encoder bcenc	bc[1,3,4,5,7]
	-encoder ate	bc[1,4,5,7]
	-encoder ate	astc[4x4,8x8]
	-encoder astcenc	astc[4x4,5x5,6x6,8x8] ldr/hdr support
	-encoder etcenc	etc2[r,rg,rgb,rgba]
	-encoder explicit	r|rg|rgba[8|16f|32f]

	-mipnone	Don't build mips even if pow2 dimensions
	-mipmin size	Only output mips >= size px
	-mipmax size	Only output mips <= size px

	-srgb	sRGB for rgb/rgba formats
	-signed	Signed r or rg for etc/bc formats, astc doesn't have signed format.
	-normal	Normal map rg storage signed for etc/bc (rg01), only unsigned astc L+A (gggr).
	-sdf	Generate single-channel SDF from a bitmap, can mip and drop large mips. Encode to r8, bc4, etc2r, astc4x4 (Unorm LLL1) to encode
	-premul	Premultiplied alpha to src pixels before output.  Disable multiply of alpha post-sampling.  In kramv, view with "Premul off".
	-prezero Premultiplied alpha only where 0, where shaders multiply alpha post-sampling.  Not true premul and black halos if alpha ramp is fast.  In kramv, view with "Premul on".
	-optopaque	Change format from bc7/3 to bc1, or etc2rgba to rgba if opaque
	
	-chunks 4x4	Specifies how many chunks to split up texture into 2darray
	-swizzle [rgba01 x4]	Specifies pre-encode swizzle pattern
	-avg [rgba]	Post-swizzle, average channels per block (f.e. normals) lrgb astc/bc3/etc2rgba
	-v	Verbose encoding output

Usage: kram info
	 -i/nput <.png | .ktx> [-o/utput info.txt] [-v]

Usage: kram decode
	 -i/nput .ktx -o/utput .ktx [-swizzle rgba01] [-v]

Usage: kram script
	 -i/nput kramscript.txt [-v] [-j/obs numJobs]

```

### Other wrappers
These encoders have their own wrappers with different functionality.
* Astcenc (astcenc) WML, ASTC
* ETC2comp (etctool) WML, ETC2
* Squish (squishpng) WML, BC
* BCEnc - WML, BC

Other great encoder wrappers to try.  Many of these require building the app from CMake, and many only supply Windows executables. 
* Cuttlefish (cuttlefish) - WML, ASTC/BC/ETC/PVRTC
* PVRTexTool (PVRTexToolCLI) - WML, ASTC/BC/ETC/PVRTC, no BC on ML
* Nvidia Texture Tools (nvtt) - WML, ASTC/BC/ETC/PVRTC
* Basis Universal (basisu) - WML, ASTC/ETC1, transodes to 4x4 formats
* KTX Software (toktx, ktx2ktx2, ktxsc) - basis as encode
* Intel ISPC - WML, BC/ASTC
* ICBC - Ignacio Costano's BC encoder - WML, BC
* DirectX Texture Tools
* AMD Compressonator

### On Encoding Formats
```
*ASTC* 
Android and iOS, Apple M1
Requires swizzles to reduce endpoint storage (rrr1, rrrg, rgb1)
Full 8-bit channel endpoints
No HDR L+A dualplane, only RGB1
No signed format
ASTC4x4 is same size as R8Unorm explicit format.
Can change block size across all mips 4x4, 5x5, 6x6, 8x8...
Hard to store/fit endpoints to larger point clouds
Square format pixel counts ramp up quickly. 16, 25, 36, 64.
Adapts per block to L, LA, RGB, RGBA.
High encoder complexity, but getting faster
No GPU encode/decode.
Fast ISPC encoder but that doesn't pick best block types
Fixed 16 byte block size (even for HDR).  
Optimal storage depends on how much wasted on endpoints
Can fit more than 2 colors to a 4x4 block

rrr1  - 2 1-byte endpoints
rrrg  - 2 2-byte endpoints, dual plane possible in LDR
rg01  - 2 3-byte endpoints, dual plane possible, only 2 channel format in HDR
rgba1 - 2 3-byte endpoints

*ETC2*
Android and iOS, Apple M1
No GPU encode/decode
Etcpak and ETC2Comp are two encoders.   Etcpak is fast with AVX2, less for ARM.
ETC2Comp encoder slow on rgb/a due to large iteration space of each block.  
ETC2Comp multipass skips encoding blocks by treating quality as percentage which is dubious, fixed in kram.
kram breaks out quality from block percentage for multipass.

r - 4bpp, 2 11-bit endpoints, unpacks to r16f in texture cache, signed/unsigned
rg - 8bpp, 2 22-bit endpoints, unpacks to rg16f in texture cache, signed/unsigned
rgb - 4bpp, similar to ETC1, several permuations that slow encode times
rgba - 8bpp, has several permuations that slow encode times

*BC*
Desktop and consoles, Apple M1
GPU accelerated encoders
Several encoders to choose from
Can fit more than 2 unique colors to a 4x4 block with BC7, but no BC1/3
GPU accelerators 100 to 1000x faster.

BC1 - 4bpp, 565, 2-bit selector, kram doesn't support 1-bit alpha form
BC2 - not exposed
BC3 - 8bpp 565, 2-bit selector, 8-bit alpha, 3-bit selector
BC4 - 4bpp 2 8-bit endpoints, 3 bit selector, unpacks to r16f in texture cache, signed/unsigned
BC5 - 8bpp, 2 16-bit endpoints, 3 bit selector, unpacks to rg16f in texture cache, signed/unsigned
BC6 - 8bpp, not supported yet, rgb16 signed/unsigned
BC7 - 8bpp, rgba, adaptive, can pack 4 unique colors into 4x4 block via partitioning

*Basis* (not in kram)
Lots of great concepts here
Written by Rich Geldreich who also did BCenc
Transcoder format from ETC1s and UASTC storage
Can reduce storage of redundant blocks across mips
One format to rule them all
Encode once
Only 4x4 block sizes
ETC1s quality issues from 5-bit channel endpoints, and skipped block orient.
ASTC doesn't compress and RDO as tightly.

```

### Quick chart on formats

| Fmt   | GPU  | Block Size | Sizes        |
|-------|------|------------|--------------|
| BC    | Yes  | Fixed      | 4/8bpp       |
| ASTC  | No   | Variable   | 2-8bpp @ 8x8 |
| ETC   | No   | Fixed      | 4/8bpp       |
| Basis | No   | Fixed      | 2-8bpp       |


### Normal map formats for 2 channels

| Fmt    | pre  | post | pl | size  | comment                      | endpoints |
|--------|------|------|----|-------|------------------------------|-----------|
| RG8    | rg01 | rg   | 2  | 2     |                              |   8:e     |
| RG16f  | rg01 | rg   | 2  | 4     |                              | 16f:e     |
| RG32f  | rg01 | rg   | 2  | 8     |                              | 32f:e     |
| BC1nm  | rg01 | rg   | 1  | 0.5   | used by Capcom w/BC3nm       | 56:2      |
| BC3nm  | xgxr | ag   | 2  | 1     | can store block constant rb  |  6:2, 8:3 |
| BC5nm  | rg01 | rg   | 2  | 1     | signed, rg16f in cache       |  8:3      |
| ETCrg  | rg01 | rg   | 2  | 1     | rg11n, signed, rg16f in cache|  11:3     |
| ASTCnm | gggr | ag   | 2  | 1@4x4 | swizzle more like BC3nm      |  8:3      |
| ASTCnm | rrrg | ga   | 2  | 1     |                              |  8:3      |
| ASTCrg | rg01 | rg   | 2  | 1     | 2 bytes for b=0, rba + g     |  8:3      |

### On mip calculations and non-power-of-two textures

With the exception of PVRTC, the block encoded formats support non-power-of-two mipmaps.  But very little literature talks about how mips are calculated.  OpenGL/D3D first used round-down mips, and Metal/Vulkan had to follow suit.  Round down cuts out a mip level, and does a floor of the mip levels.   Round-up mips generally have a better mapping to the upper with a simple box filter.  kram now has reasonable cases for pow2 and non-pow2 mip generation.  Odd source pixel counts have to shift weights as leftmost/rightmost pixels contribute more on the left/right sides, and avoid a shift in image pixels.

```
Round Down
3px  a      b      c
1px       a+b+c

Round Up
3px  a      b      c
2px    a+b     b+c 
1px      a+b+c
```

### On memory handling in kram:

Batch processing on multiple threads can use a lot of memory.  Paging can reduce performance.  kram memory maps the source png/ktx, decodes the png into memory or reads ktx pixels from the mmap, pulls chunks for the slice/cube/texture, then generates mips in place, and uses a half4 type for srgb/premul to maintain higher precision, and finally writes the mip to disk.  Then kram process another mip.

The rgba8u image and half4 image are stored for mips.  1gb can be used to process separate textures on 16 cores.  This may be an argument for multiple cores processing a single image.  Running fewer threads can reduce peak memory use.  Also hyperthreaded cores share SIMD units, so using physical processor counts may be similar performance to HT counts.  Alignment is maintained and memory is reduced with in-place mips, but that also means mipgen in kram is intermingled with encoding.

| Dims   | MPix | 8u MB | 16f MB | 32f MB |
|--------|------|-------|--------|--------|
| 1024^2 |  1   |  4    |  8     |  16    |
| 2048^2 |  4   |  16   |  32    |  64    |
| 4096^2 | 16   |  64   |  128   |  256   |
| 8192^2 | 64   |  256  |  512   |  1024  |

### On lossless PNG, KTX, KTX2, DDS output formats:

PNG is limited to srgb8u/8u/16u data.  Most editors can work with this format.  Gimp and Photoshop can maintain 16u data.  There's no provision for mips, or cube, 3d, hdr, or premultiplied alpha.  And most tools always set png as sRGB data.  Content tools, image editors, browsers really need to replace PNG and DDS with compressed KTX2 for source and output content. 

kram encourages the use of lossless and hdr source data.  There are not many choices for lossless data - PNG, EXR, and Tiff to name a few.  Instead, kram can input PNG files with 8u pixels, and KTX/2 files for 8u/16f/32f pixels.  Let kram convert source pixels to premultiplied alpha and from srgb to linear, since ordering matters here and kram does this using half4/float4.  LDR and HDR data can come in as horizontal or vertical strips, and these strips can then have mips generated for them.  So cube maps, cube map arrays, 2D arrays, and 1d arrays are all handled the same.

KTX is a well-designed format, and KTX2 continues that tradition.  It was also faily easy to convert between these formats.  Once mips are decoded, KTX2 looks very much like KTX.

Visually validating and previewing the results is complicated.  KTX/2 have few viewers, hence the need for kramv.  Apple's Preview can open BC and ASTC files on macOS, but not ETC/PVRTC.  And then you can't look at channels or mips, or turn on/off premultiplied alpha, or view signed/unsigned data.  Preview premultiplies PNG images, but KTX files aren't.  Apple's thumbnails don't work for ETC2 or PVRTC data in KTX files.  Windows thumbnails don't work for KTX at all.  PVRTexToolGUI 2020R2 applies sRGB incorrectly to images, and can't open BC4/5/7 files on Mac.  

kram adds props to KTX/2 file to store data.  Currently props store Metal and Vulkan formats.  This is important since GL's ASTC LDR and HDR formats are the same constant.  Also props are saved for channel content and post-swizzle.  Loaders, viewers, and shaders can utilize this metadata.

Kram now supports KTX2 export.  But KTX can also be converted to KTX2 and each mip supercompressed via ktx2ktx2 and ktxsc.  KTX2 reverses mip ordering smallest to largest, so that streamed textures can display smaller mips before they finish fully streaming.  KTX2 can also supercompress each mip with zstd and Basis for transcode.  I suppose this could then be unpacked to tiles for sparse texturing.  KTX2 does not store a length field inside the mip data which keeps consistent alignment. 

Metal cannot load mmap mip data that isn't aligned to a multiple of the block size (8 or 16 bytes for BC/ASTC/ETC).  KTX adds a 4 byte length into the mip data that breaks alignment, but KTX2 fortunately skips that.  But KTX2 typically compresses the levels and needs decode/transcode to send to the GPU.

Note that textures and render textures don't necessarily use pixels or encoded blocks in the order that you specify in the KTX file.  Twiddling creates serpentine patterns of pixels/blocks that are platform and hardware dependent.  Hardware often writes to linear for the display system, and reads/writes twiddled layouts.  It's hard for a generic tool like kram to address this.  I recommend that the texture loader always upload ktx blocks to private texture surfaces, and let the API twiddle the data during the copy.  This can sometimes be a source of upload timing differences.

* KTX mmap -> Copy Level to Shared Buffer -> Blit Level to Private Tex
* KTX2 mmap -> Decompress Level to Shared Buffer -> Blit Level To Private Tex

With sparse texture, the above becomes more involved since only parts of the decompressed level are uploaded.  KTX2 is still the ideal choice, since textures are considerably smaller 2-10x and can be mmap-ed directly from the bundle.

### Encoding and hardware lookup of srgb and premultiplied data.

```
Texture units convert srgb to linear data on the way to the texture cache.  
The texture cache typically stores 4x4 block to match encoded formats.  
Sampling is then done from that block of linear data with higher precision.

Texturing hardware does not yet support premultiplied alpha.  
kram does premul prior to mip generation.
Srgb is then re-applied to linear premul data before encoding.

kram uses half4/float4 to preserve precision when srgb or premul are specified.  
The encoders all have to encode non-linear srgb point clouds, which isn't correct.

```

### On texture atlases and charts (TODO:)

2D atlas packing works for source textures like particle flipbooks, but suffers from many issues.  Often packed by hand or algorithm, the results look great as PNG, but break down once mipped and block encoded. These are some of the complex problems:

* Mip bleed - Solved with mip lod clamping or disabling mips.
* Alignment bleed - Solved with padding to smallest visible mip blocks.
* Block bleed - Solved with pow2 blocks - 4x4 scales down to 2x2 and 1x1.  6x6 scales to non-integral 3x3 and 1.5x1.5.
* Clamp only - Solved by disabling wrap/mirror modes and uv scaling.
* Complex pack - stb_rect_pack tightly pack images to a 2d area without accounting for bleed issues
 
kram will soon offer an atlas mode that uses ES3-level 2d array textures.  These waste some space, but are much simpler to pack, provide a full encoded mip chain with any block type, and also avoid the 5 problems mentioned above.  Named atlas entries reference a given array element, but could be repacked and remapped as used to a smaller atlas.  Dropping mip levels can be done across all entries, but is a little harder for a single array element.  Sparse textures work for 2d array textures, but often the min sparse size is 256x256 (64K) or 128x128 (16K) and the rest is the packed mip tail.  Can draw many types of objects and particles with only a single texture array.

The idea is to copy all atlased images to a 2d vertical strip.  This makes row-byte handling simpler.  Then kram can already convert a vertical strip to a 2D array, and the output rectangle, array index, mip range, and altas names are tracked as well.  But there is some subtlety to copy smaller textures to the smaller mips and use sampler mip clamping.  Non-pow2 textures will have transparent fill around the sides.

Apps like Substance Painter use charts of unwrapped UV.  These need to be gapped and aligned to block sizes to avoid the problems above.  Often times the gap is too small (1px) for the mipchain, and instead the algorithms cover up the issue by dilating colors into the gutter regions, so that black outlines are not visible.  thelka_atlas, xatlas, and other utilities can build these charts.


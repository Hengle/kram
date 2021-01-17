# kram, kram.exe
C++11 main to libkram to create CLI tool.  Encode/decode/info PNG/KTX files with LDR/HDR and BC/ASTC/ETC2.  Runs on iOS/macOS/winOS.

# libkram.a, kram.lib
C++11 library from 200 to 800KB in size depending on encoder options.  Compiles for iOS (ARM), macOS (ARM/Intel), winOS (Intel).

# kramv.app
ObjC++ Viewer for PNG/KTX supported files from kram.  530KB in size.  Uses Metal compute and shaders, eyedropper, grids, debugging, preview.  Supports HDR and all texture types.  Mip, face, and array access.  No dmg yet, just drop onto /Applications folder, and then run scripts/fixfinder.sh to flush LaunchServices (see below).  Runs on macOS (ARM/Intel).

### About kram
kram is a wrapper to several popular encoders.  Most encoders have sources, and have been optimized to use very little memory and generate high quality encodings at all settings.  All kram encoders are currently cpu-based.  Some of these encoders use SSE but not Neon, and I'd like to fix that.  kram was built to be small and used as a library or app.  It's also designed for mobile and desktop use.  The final size with all encoders is under 1MB, and disabling each encoder chops off around 200KB down to a final 200KB app size via dead-code stripping.  The code should compile with C++11 or higher.

kram focuses on sending data efficiently and precisely to the encoders.  kram handles srgb and premul at key points in mip generation.  Source files use mmap to reduce memory, but fallback to file ops if that fails.  Temp files are generated for output, and then renamed in case the app fails or is terminated.  Mips are done in-place, and mip data is written out to a file to reduce memory usage. kram leaves out BC2 and etcrgb8a1 and PVRTC.  Also BC6 still needs an encoder, and ASTC HDR encoding needs a bit more work to pull from half4/float4 source pixels.  

Many of the encoder sources can multithread a single image, but that is unused.  kram is designed to batch process one texture per thread via a python script or a C++11 task system inside kram.  Texture-per-process and scripted to threads currently both take the same amount of cpu time, but the latter is best if kram ever adds gpu accelerated encoding.

Similar to a makefile system, the script sample kramtexture.py uses modstamps to skip textures that have already been processed.  If the source png is older than the ktx output, then the file is skipped.  Command line options are not yet compared, so if those change then use --force on the python script to rebuild all textures.  Also a crc/hash could be used instead when modstamp isn't sufficient or the same data could come from different folders.

### About kramv
kramv is a viewer for the BC/ASTC/ETC2 and HDR KTX textures generated by kram from LDR PNG and LDR/HDR KTX sources.  kramv decodes ASTC/ETC2 textures on macOS Intel, where the GPU doesn't support them.  macOS with Apple Silicon supports all three formats, and doesn't need to decode.   

This is all in ObjC++ with the intent to port to Windows as time permits.  It's adapted from Apple's Metal sample app.  There's very little GUI and it's all controlled via keyboard to make the code easy to port and change, but the key features are useful for texture triage and analysis.  Drag and drop, and click-launch are supported.  Recently used textures are listed in the menu.  The app is currently single-document only, but I'd like to fix that.  Subsequent opens reuse the same document Window.

Compute shaders are used to display a single pixel sample from the gpu texture.  This simplifies adding more viewable formats in the future, but there is not a cpu fallback.  The preview is rendered to a cube with a single face visible using shaders.  Preview mode provides lighting, sdf cutoff, and mip visuals for a given texture.

In non-preview mode, point sampling in a pixel shader is used to show exact pixel values of a single mip, array, and face.  Debug modes provide pixel analysis.  KramLoader shows synchronous cpu upload to a private Metal texture, but does not yet supply the underlying KTXImage.  Pinch zoom and panning tries to keep the image from onscreen, and zoom is to the cursor so navigating feels intuitive.

```
Formats - R/RG/RGBA 8/16F/32F, BC/ETC2/ASTC
Container Types - KTX, PNG
Content Types - Albedo, Normal, SDF
Debug modes - transparent, color, gray, +x, +y
Texture Types - 1darray (no mips), 2d, 2darray, 3d (no mips), cube, cube array

/ - show keyboard shortcuts
O - toggle preview, disables debug mode, shows lit normals, and mips and filtering are enabled
Shift-/D - toggle block/pixel grid, must be zoomed-in to see it
Shift-/E - advance debug mode, this is texture content specific
H - toggle hud
I - show texture info in overlay
W - toggle repeat filter, scales uv from [0,1] to [0,2]

R/G/B/A - show channel in isolation
P - toggle shader premul, the shader performs this after sampling but for point sampling it is correct
S - toggle signed/unsigned

Shift-/0 - refit the current mip image to 1x, or fit view.  Smaller mips will appear at size with Shift-0.
L - reload from disk if changed, zoom to fit
Shift-L - reload, but at 1x

Shift-/Y advance array
Shift-/F advance face
Shift-/M advance mip
```

### Limitations

Texture processing is complex and there be dragons.  Just be aware of some of the limitations of kram as currently implemented.  There is a lot that works, and BC1-BC3 should be retired along with all of ETC2.  There are better formats, and hardware has moved to ASTC, and then recently back to BC.  WebGL is still often stuck with older formats for lack of extensions.  These formats are still too big without further re-ordering endpoints/selectors and compressing the enire texture compressing mips.  Basis is one way to compress to storage and transcode at runtime to available GPU capabilities, but I suggest basisu or toktx to generate that until KTX2 support is added to kram.

```
GPU - none of the encoders use the GPU, so cpu threading and multi-process is used

Rescale Filtering - 1x1 point filter
Mip filtering - 2x2 box filter that's reasonable for pow2, but not ideal for non-pow2 mips, done in linear space using half4 storage, in-place to save mem

1D array - no mip support due to hardware
3D textures - no mip support, uses ASTC 2d slice encode used by Metal/Android, not exotic ASTC 3d format
    
BC/ETC2/ASTC - supposedly WebGL requires pow2, and some implementation need top multiple of 4 for BC/ETC2

BC1 - artifacts from limits of format, artifacts from encoder, use BC7 w/2x memory
BC1 w/alpha - blocked
BC2 - blocked
BC6H - unsupported, no decode either, need to pull BCH encode/decode and pass 8u/16f/32f data

ETC2_RGB8A1 - disabled, broken in ETC2 optimizations

ASTC LDR - rrr1, rrrg/gggr, rgb1, rgba must be followed to avoid endpoint storage, requires swizzles
ASTC HDR - encoder uses 8-bit source image, need 16f/32f passed to encoder, no hw L+A mode

R/RG/RGBA 8/16F/32F - use ktx2ktx2 and ktx2sc KTX2 to supercompress, use as source formats
R8/RG8/R16F - input/output rowBytes not aligned to 4 bytes to match KTX spec, code changes needed

Basis - unsupported, will come with KTX2 support, can transcode from UASTC/ETC1S at runtime, RDO
Crunch - unsupported, has RDO, predecessor to Basis, Unity provides a release of this

KTX - breaks loads of mips with 4 byte length offset at the start of each level of mips, metadata/props aren't standardized and only ascii prop support so easy to dump out, convert to ktx2 with ktx2ktx2 for some texture types, can only supercompress to zstd not Basis using ktx2ktx2 + ktxsc
KTX2 - unsupported, no viewers, mips flipped from KTX, need to get working in kram and viewer, has compressed mips

```

### Building
Kram uses CMake to setup the projects and build.  kramv.app, kram, and libkram are generated, but kramv.app and kram are stand-alone.  The library can be useful in apps that want to include the decoder, or runtime compression of gpu-generated data.

For Mac, the build is out-of-source, and can be built from the command line, or debugged from the xcodeproj that is built.  Ninja and Makefiles can also be generated from cmake, but remember to trash the CMakeCache.txt file.

```
mkdir build
cmake .. -G Xcode

cmake --build . --config Release
or
open kramWorkspace.xcodeproj
or
cmake --install ../bin --config Release
```

For Windows, the steps are similar. I tried to fix CMake to build the library into the app directory so the app is updated.  "Rebuild Solution" if your changes don't take effect, or if breakpoints stop being hit.

```
mkdir build
cmake .. -G "Visual Studio 15 2017 Win64" 
or
cmake .. -G "Visual Studio 16 2019" -A x64

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

# these scripts process all the platforms on 8 threads
../scripts/kramTextures.py --jobs 8 -p android 
../scripts/kramTextures.py --jobs 8 -p ios --script
../scripts/kramTextures.py --jobs 8 -p mac --script --force 
../scripts/kramTextures.py --jobs 8 -p win --script --force

```

### Sample Commands
To test individual encoders, there are tests cases embedded into kram.  Also individual textures can be processed, or the script records these commands and executes the encodes on multiple cores.

```
cd build
./Release/kram -testall
./Release/kram -test 1002

./Release/kram encode -f astc4x4 -srgb -premul -quality 49 -mipmax 1024 -type 2d -i ../tests/src/ColorMap-a.png -o ../tests/out/ios/ColorMap-a.ktx
./Release/kram encode -f etc2rg -signed -normal -quality 49 -mipmax 1024 -type 2d -i ../tests/src/collectorbarrel-n.png -o ../tests/out/ios/collectorbarrel-n.ktx
./Release/kram encode -f etc2r -signed -sdf -quality 49 -mipmax 1024 -type 2d -i ../kram/tests/src/flipper-sdf.png -o ../tests/out/ios/flipper-sdf.ktx

```

### Open Source Encoder Usage
This app would not be possible without the open-source contributions from the following people and organizations.  These people also inspired me to make this app open-source, and maybe this will encourage more great tools or game tech.

Kram includes the following encoders/decoders:

| Encoder  | Author           | License     | Encodes                     | Decodes | 
|----------|------------------|-------------|-----------------------------|---------|
| BCEnc    | Rich Geldreich   | MIT         | BC1,3,4,5,7                 | same    |
| Squish   | Simon Brown      | MIT         | BC1,3,4,5                   | same    |
| ATE      | Apple            | no sources  | BC1,4,5,7 ASTC4x4,8x8 LDR   | all LDR |
| Astcenc  | Arm              | Apache 2.0  | ASTC4x4,5x5,6x6,8x8 LDR/HDR | same    |
| Etc2comp | Google           | MIT         | ETC2r11,rg11,rgb,rgba       | same    |
| Explicit | Me               | MIT         | r/rg/rgba 8u/16f/32f        | none    |


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

Astcenc v2.1
Provide rgba8u source pixels.  Converted to 32f at tile level.
Improved 1 and 2 channel format encoding (not transfered to v2.1).
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

### Open Source  Usage
Kram includes additional open-source:

| Library        | Author           | License | Purpose                   |
|----------------|------------------|---------|---------------------------|
| lodepng        | Lode Vandevenne  | MIT     | png encode/decode         |
| SSE2Neon       | John W. Ratcliff | MIT     | sse to neon               |
| heman          | Philip Rideout   | MIT     | parabola EDT for SDF      |
| TaskSystem     | Sean Parent      | MIT     | C++11 work queue          |
| tmpfileplus    | David Ireland    | Moz 2.0 | fixes C tmpfile api       |
| mmap universal | Mike Frysinger   | Pub     | mmap on Windows           |

```
lodepng
Altered header paths.

SSE2Neon
Updated to newer arm64 permute instructions.

heman 
SDF altered to support mip generation from bigger distance fields.
   This requires srcWidth x dstHeight floats.
   
TaskSystem
Altered to allow control over number of threads.
Commented out future system until needed.

mmap
This matches mmap api, but leaks a file mapping handle.
  
```

### Features to complete:
* Tile command for SVT tiling
* Merge command to combine images (similar to ImageMagick)
* Atlas command to atlas to 2D and 2D array textures
* Add GPU encoder (use compute in Metal/Vulkan)
* Add BC6H encoder
* Save prop with args and compare args and modstamp before rebuilding to avoid --force
* Multichannel SDF
* Plumb half4 and float4 through to ASTC HDR encoding.  Sending 8u.
* Test Neon support and SSE2Neon
* Run srgb conversion on endpoint data after fitting linear color point cloud
* PSNR stats off encode + decode
* Dump stats on BC6/7 block types, and ASTC void extent, dual-plane, etc
* Update to new BC7 enc with more mode support

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

Date: 1/16/21
2020 M1 13" Macbook Air, 3.4Ghz, 4+4 core M1
2018 16" Macbook Pro, 2.3Ghz, 8/16 core i9

| Sys | macOS  | iOS    | Android | 
|-----|-----------------|---------|
| M1  | 0.643s | 4.603s |  6.441s |  
| i9  | 0.970s | 3.339s | 14.617s |

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

	 [-mipalign] [-mipnone]
	 [-mipmin size] [-mipmax size]

	 [-swizzle rg01]
	 [-avg rxbx]
	 [-sdf]
	 [-premul]
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

	-mipalign	Align mip levels with .ktxa output 
	-mipnone	Don't build mips even if pow2 dimensions
	-mipmin size	Only output mips >= size px
	-mipmax size	Only output mips <= size px

	-srgb	sRGB for rgb/rgba formats
	-signed	Signed r or rg for etc/bc formats, astc doesn't have signed format.
	-normal	Normal map rg storage signed for etc/bc (rg01), only unsigned astc L+A (gggr).
	-sdf	Generate single-channel SDF from a bitmap, can mip and drop large mips. Encode to r8, bc4, etc2r, astc4x4 (Unorm LLL1) to encode
	-premul	Premultiplied alpha to src pixels before output

	-optopaque	Change format from bc7/3 to bc1, or etc2rgba to rgba if opaque

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
Android and iOS
Swizzles unique to format, but can match up with hw swizzles if supported.
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
Android and iOS
No GPU encode/decode
Slowest encoder due to large iteration space of each block
High-precision, signed r and rg format.  Can use for normals.
ETCPack and ETC2Comp are the two choices
ETC2Comp multipass skips encoding blocks by treating quality as percentage which is dubious
Kram breaks out quality from block percentage for multipass.

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
Transcoder format from ETC1s and ASTC like data
Can reduce storage of redundent blocks across mips
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

With the exception of PVRTC, the block encoded formats support non-power-of-two mipmaps.  But very little literature talks about how mips are calculated.  D3D first used round-down mips, GL followed suit, and Metal/Vulkan followed suit.  Round down cuts out a mip level, and does a floor of the mip levels.   Round-up mips generally have a better mapping to the upper with a simple box filter.  Kram hasn't adjusted it's box filter to adjust for this yet, but there are links into the code to articles about how to better weight pixels.  The kram box filter is correct for power-of-two mipgen, but should be improved for these cases.

```
Round Down
3px  a      b      c
1px       a+b+c

Round Up
3px  a      b      c
2px    a+b     b+c 
1px      a+b+c
```

### On memory handling in Kram:

Batch processing on multiple threads can use a lot of memory.  Paging can reduce performance.  Kram memory maps the source png/ktx, decodes the png into memory or reads ktx pixels from the mmap, pulls chunks for the slice/cube/texture, then generates mips in place, and uses a half4 type for srgb/premul to maintain higher precision, and finally writes the mip to disk.  Then kram process another mip.

The rgba8u image and half4 image are stored for mips.  1gb can be used to process separate textures on 16 cores.  This may be an argument for multiple cores processing a single image.  Running fewer threads can reduce peak memory use.  Also hyperthreaded cores share SIMD units, so using physical processor counts may be similar performance to HT counts.  Alignment is maintained and memory is reduced with in-place mips, but that also means mipgen in kram is intermingled with encoding.

| Dims   | MPix | 8u MB | 16f MB | 32f MB |
|--------|------|-------|--------|--------|
| 1024^2 |  1   |  4    |  8     |  16    |
| 2048^2 |  4   |  16   |  32    |  64    |
| 4096^2 | 16   |  64   |  128   |  256   |
| 8192^2 | 64   |  256  |  512   |  1024  |

### On PNG, KTX, KTX2, KTXA output formats:

PNG is limited to srgb8 and 8u and 16u data.  Most editors can work with this format.  Gimp and Photoshop can maintain 16u data.  There's no provision for mips, or cube, 3d, hdr, or premultiplied alpha.  Content tools, image editors, browsers really need to replace PNG and DDS with compressed KTX2 for source and output content.  This uses very little space in source control then, and likely more could be done with delta encoding pixels like PNG before compression.   

kram encourages the use of lossless and hdr source data.  There are not many choices for lossless data - PNG, EXR, and Tiff to name a few.  Instead, kram can input PNG files with 8u pixels, and KTX files for 8u/16f/32f pixels.  Let kram convert source pixels to premultiplied alpha and from srgb to linear, since ordering matters here and kram does this using float4.  LDR and HDR data can come in as horizontal or vertical strips, and these strips can then have mips generated for them.  So cube maps, cube map arrays, 2D arrays, and 1d arrays are all handled the same.

KTX stores everything with 4 byte alignment.  It's got a simple 64-byte header, props, and then mip data with lengths and padding.  

Validating and previewing the results is complicated.  KTX has few viewers.  Apple's Preview can open BC and ASTC files on macOS, but not ETC/PVRTC.  And then you can't look at channels or mips, or turn on/off premultiplied alpha, or view signed/unsigned data.  Preview premultiplies PNG images, but KTX files aren't.  Apple's thumbnails don't work for ETC2 or PVRTC data.  Windows thumbnails don't work for KTX at all.  PVRTexToolGUI applies sRGB and premultiplied alpha incorrectly to images, and can't open BC files on Mac, or BC7 files on Windows, or ETC srgb files.  PVRTexToolGUI should fix some of these issues in their next release, but it's almost the end of 2020.

Kram adds props to the KTX file to store data.  Currently props store Metal and Vulkan formats.  This is important since GL's ASTC LDR and HDR formats are the same constant.  Also props are saved for channel content and post-swizzle.  Loaders, viewers, and shaders can utilize this metadata.

KTX can be converted to KTX2 and each mip supercompressed via ktx2ktx2 and ktxsc.  But there are no viewers for that format.  KTX2 reverses mip ordering smallest to largest, so that streamed textures can display smaller mips progressively.   KTX2 can also supercompress each mip with zstd.  I suppose this could then be unpacked to tiles for sparse texturing.  KTX2 does not store a length field inside the mip data which keeps consistent alignment. 

I also have a custom KTXA format.  KTXA likely broke with the prop additions.  Metal cannot load mmap data that isn't aligned to a multiple of the block size (8 or 16 bytes for BC/ASTC/ETC).  But KTX stuffs a 4 byte length into the mip data.  So by leaving the size out (TODO: pad props to a 16 byte multiple), then the mips could be directly loaded.  My loader had to copy the mips to a staging MTLBuffer anyways, so it's probably best not to create a new format.  Also sparse textures imply splitting up large mips into tiles.  Also mmap'ed data on iOS/Android don't count towards jetsam limits, so that imposes some constraints on loaders.

Note that textures and render textures don't necessarily use pixels or encoded blocks in the order that you specify in the KTX file.  Twiddling creates serpentine patterns of pixels/blocks that are platform and hardware dependent.  Hardware often writes to linear for the display system, and reads/writes twiddled layouts.  It's hard for a generic tool like kram to address this.  I recommend that the texture loader always upload ktx blocks to private texture surfaces, and let the API twiddle the data during the copy.  This can sometimes be a source of upload timing differences.

### Encoding and hardware lookup of srgb and premultiplied data.

```
Texture units convert srgb to linear data on the way to the texture cache.  
The texture cache typically stores 4x4 blocks to match encoded formats.  
Sampling is then done from that block of linear data.
Ideally the cache stores >8-bit linear channels.

Texturing hardware does not yet support premultiplied alpha.  
Kram does premul prior to mip generation.
Srgb is then re-applied to linear premul data before encoding.
LDR premul typically implies rgb*a <= a, but this rule can be broken in film.
Once in premul stay there, dividing out alpha isn't performant or precise.
Premul breaks many of the common hardware blend modes, so emulate in shader.

Kram uses float4 to preserve precision when srgb or premul are specified.  
The encoders encode non-linear srgb point clouds.

encode = (srgbFromPremulLinear(rgb * a), a)
decode = bilerp(premulLinearFromSrgb(rgb), a)

```

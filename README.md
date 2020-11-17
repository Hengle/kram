# kram
Encode/decode/info to and from PNG/KTX files with LDR/HDR and BC/ASTC/ETC2.

### About
kram is a wrapper to several popular encoders.  Most encoders have sources, and have been optimized to use very little memory and generate high quality encodings at all settings.  All kram encoders are currently cpu-based.  Some of these encoders use SSE but not Neon, and I'd like to fix that.  kram was built to be small and used as a library or app.  It's also designed for mobile and desktop use.  The final size with all encoders is under 1MB, and disabling each encoder chops off around 200KB down to a final 200KB app size via dead-code stripping.  The code should compile with C++11 or higher.

kram focuses on sending data efficiently and precisely to the encoders.  kram handles srgb and premul at key points in mip generation.  Source files use mmap to reduce memory, but fallback to file ops if that fails.  Temp files are generated for output, and then renamed in case the app fails or is terminated.  Mips are done in-place, and mip data is written out to a file to reduce memory usage. kram leaves out BC2 and etcrgb8a1 and PVRTC.  Also BC6 still needs an encoder, and ASTC HDR encoding needs a bit more work to pull from the float4 pixels.  

Many of the encoder sources can multithread a single image, but that is unused.  kram is designed to batch process one texture per thread via a python script or a C++11 task system inside kram.  These currently both take the same amount of cpu time, but the latter is best if kram ever adds gpu accelerated encoding.

Similar to a makefile system, the script sample kramtexture.py uses modstamps to skip textures that have already been processed.  If the source png is older than the ktx output, then the file is skipped.  Command line options are not yet compared, so if those change then use --force on the python script to rebuild all textures.  Also a crc/hash could be used instead when modstamp isn't sufficient or the same data could come from different folders.



### Building
Kram uses CMake to setup the projects and build.  An executable kram and libkram are generated, but only kram is needed to run.  The library can be useful in apps that want to include the decoder, or runtime compression of gpu-generated data.

For Mac, the build is out-of-source, and can be built from the command line, or debugged from the xcodeproj that is built.  Ninja and Makefiles can also be generated from cmake, but remember to trash the CMakeCache.txt file.

```
mkdir build
cmake .. -G Xcode

cmake --build . --config Release
or
open kram.xcodeproj
```

For Windows, the steps are similar. I tried to fix CMake to build the library into the app directory so the app is updated.  "Rebuild Solution" if your changes don't take effect, or if breakpoints stop being hit.

```
mkdir build
cmake .. -G "Visual Studio 15 2017 Win64" 
or
cmake .. -G "Visual Studio 16 2019" -A x64

cmake --build . --config Release
or
open kram.sln
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

To demonstrate how kram works, scripts/kramtextures.py applies platform-specific presets based on source filenames endings.  The first form executes multiple kram processes with each file using a Python ThreadPoolExecutor.  The second generates a script file, and then runs that in a C++ task system inside kram.  The scripting system would allow gpu compute of commands, and more balanced memory and thread usage.

```
cd build
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

| Library     | Author           | License | Purpose                   |
|-------------|------------------|---------|---------------------------|
| lodepng     | Lode Vandevenne  | MIT     | png encode/decode         |
| SSE2Neon    | John W. Ratcliff | MIT     | sse to neon               |
| heman       | Philip Rideout   | MIT     | parabola EDT for SDF      |
| TaskSystem  | Sean Parent      | MIT     | C++11 work queue          |
| tmpfileplus | David Ireland    | Moz 2.0 | fixes C tmpfile api       |

```
lodepng
Altered header paths.

SSE2Neon
Needs updated to new arm64 permute instructions.

heman 
SDF altered to support mip generation from bigger distance fields
   This requires srcWidth x dstHeight floats.
  
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
* Add mmap for Windows
* Test Neon support and SSE2Neon
* Run srgb conversion on endpoint data after fitting linear color point cloud
* PSNR stats off encode + decode
* Dump stats on BC6/7 block types, and ASTC void extent, dual-plane, etc
* Update to new BC7 enc with more mode support

### Test Images
* color_grid from Astcenc samples
* ColorMap from Apple's sample apps to test premultiplied alpha and srgb.
* flipper-sdf image taken from EDT paper that inspired heman SDF.
* collectorbarrel-n/a from Id's old GPU BC1/3 compression article.
* Toof-a is my own artwork drawn in Figma


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

| Fmt    | pre  | post | pl | size  |
|--------|------|------|----|-------|
| RG8    | rg01 | rg   | 2  | 2     |
| RG16f  | rg01 | rg   | 2  | 4     |
| RG32f  | rg01 | rg   | 2  | 8     |
| BC1nm  | rg01 | rg   | 1  | 0.5   |
| BC3nm  | xgxr | ag   | 2  | 1     |
| BC5nm  |rg01  | rg   | 2  | 1     |
| ETCrg  | rg01 | rg   | 2  | 1     |
| ASTCnm | gggr | ag   | 2  | 1@4x4 |
| ASTCnm | rrrg | ga   | 2  | 1     |
| ASTCrg | rg01 | rg   | 2  | 1     |

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

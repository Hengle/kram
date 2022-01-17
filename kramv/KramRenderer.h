// kram - Copyright 2020 by Alec Miller. - MIT License
// The license and copyright notice shall be included
// in all copies or substantial portions of the Software.

#import <Foundation/NSURL.h>
#import <MetalKit/MetalKit.h>

#include "KramLib.h"
#import "KramShaders.h"  // for TextureChannels

// Turn on GLTF loading support for 3d models.  This relies on Warren Moore's first GLTFKit
// which only handles import and synchronous loading.
#define USE_GLTF 0

// Only use a perspective transform for models/images, otherwise perspective only used for models
#define USE_PERSPECTIVE 0

namespace kram {
class ShowSettings;
class KTXImage;
}

// Our platform independent renderer class.   Implements the MTKViewDelegate
// protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view
                                    settings:
                                        (nonnull kram::ShowSettings *)settings;

- (BOOL)loadTextureFromImage:(nonnull const char *)fullFilenameString
                   timestamp:(double)timestamp
                       image:(kram::KTXImage &)image
                 imageNormal:(nullable kram::KTXImage *)imageNormal
                   isArchive:(BOOL)isArchive;

- (BOOL)loadTexture:(nonnull NSURL *)url;

- (simd::float4x4)computeImageTransform:(float)panX
                                   panY:(float)panY
                                   zoom:(float)zoom;

- (BOOL)hotloadShaders:(nonnull const char *)filename;


// unload textures and gltf model textures
- (void)releaseAllPendingTextures;

// for gltf models
- (void)unloadModel;

@property (nonatomic) BOOL playAnimations;

@end


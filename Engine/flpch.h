#pragma once

#include <Core.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "Log.h"

#ifdef FLEUR_PLATFORM_MACOS
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <ModelIO/ModelIO.h>
#import <simd/simd.h>
#endif
#elif FLEUR_PLATFORM_WIN
#define NOMINMAX
#include <windowsx.h>
#endif

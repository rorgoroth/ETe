#ifndef __SDL_LOCAL_HEADERS__
#define __SDL_LOCAL_HEADERS__

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#if defined(NEED_VULKAN) && defined(USE_VULKAN_API)
#ifdef USE_LOCAL_HEADERS
#	include "SDL_vulkan.h"
#else
#	include <SDL_vulkan.h>
#endif
#endif

#endif
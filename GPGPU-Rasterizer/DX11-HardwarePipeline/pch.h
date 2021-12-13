// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#define WIN32_LEAN_AND_MEAN

#include <cassert>
#include <cstdint>
#include <type_traits>

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

template<typename RESOURCE_TYPE, typename = std::enable_if_t<std::is_convertible_v<RESOURCE_TYPE, IUnknown*>>>
void SafeRelease(RESOURCE_TYPE& pResource)
{
	if (pResource)
	{
		pResource->Release();
		pResource = nullptr;
	}
}

template<typename OBJECT_TYPE, typename = std::enable_if_t<!std::is_convertible_v<OBJECT_TYPE, IUnknown*> && std::is_pointer_v<OBJECT_TYPE>>>
void SafeDelete(OBJECT_TYPE& pObject)
{
	if (pObject)
	{
		delete pObject;
		pObject = nullptr;
	}
}

#endif //PCH_H

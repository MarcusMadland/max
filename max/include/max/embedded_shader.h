/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#ifndef MAX_EMBEDDED_SHADER_H_HEADER_GUARD
#define MAX_EMBEDDED_SHADER_H_HEADER_GUARD

#include "max.h"
#include <bx/platform.h>

#define MAX_EMBEDDED_SHADER_DXBC(...)
#define MAX_EMBEDDED_SHADER_PSSL(...)
#define MAX_EMBEDDED_SHADER_ESSL(...)
#define MAX_EMBEDDED_SHADER_GLSL(...)
#define MAX_EMBEDDED_SHADER_METAL(...)
#define MAX_EMBEDDED_SHADER_NVN(...)
#define MAX_EMBEDDED_SHADER_SPIRV(...)

#define MAX_PLATFORM_SUPPORTS_DXBC (0  \
	|| BX_PLATFORM_LINUX                \
	|| BX_PLATFORM_WINDOWS              \
	|| BX_PLATFORM_WINRT                \
	|| BX_PLATFORM_XBOXONE              \
	)
#define MAX_PLATFORM_SUPPORTS_PSSL (0  \
	|| BX_PLATFORM_PS4                  \
	|| BX_PLATFORM_PS5                  \
	)
#define MAX_PLATFORM_SUPPORTS_ESSL (0  \
	|| BX_PLATFORM_ANDROID              \
	|| BX_PLATFORM_EMSCRIPTEN           \
	|| BX_PLATFORM_IOS                  \
	|| BX_PLATFORM_LINUX                \
	|| BX_PLATFORM_OSX                  \
	|| BX_PLATFORM_RPI                  \
	|| BX_PLATFORM_VISIONOS             \
	|| BX_PLATFORM_WINDOWS              \
	)
#define MAX_PLATFORM_SUPPORTS_GLSL (0  \
	|| BX_PLATFORM_LINUX                \
	|| BX_PLATFORM_OSX                  \
	|| BX_PLATFORM_WINDOWS              \
	)
#define MAX_PLATFORM_SUPPORTS_METAL (0 \
	|| BX_PLATFORM_IOS                  \
	|| BX_PLATFORM_OSX                  \
	|| BX_PLATFORM_VISIONOS             \
	)
#define MAX_PLATFORM_SUPPORTS_NVN (0   \
	|| BX_PLATFORM_NX                   \
	)
#define MAX_PLATFORM_SUPPORTS_SPIRV (0 \
	|| BX_PLATFORM_ANDROID              \
	|| BX_PLATFORM_EMSCRIPTEN           \
	|| BX_PLATFORM_LINUX                \
	|| BX_PLATFORM_WINDOWS              \
	|| BX_PLATFORM_OSX                  \
	|| BX_PLATFORM_NX                   \
	)

///
#define MAX_EMBEDDED_SHADER_CONCATENATE(_x, _y) MAX_EMBEDDED_SHADER_CONCATENATE_(_x, _y)
#define MAX_EMBEDDED_SHADER_CONCATENATE_(_x, _y) _x ## _y

///
#define MAX_EMBEDDED_SHADER_COUNTOF(_x) (sizeof(_x)/sizeof(_x[0]) )

#if MAX_PLATFORM_SUPPORTS_DXBC
#	undef  MAX_EMBEDDED_SHADER_DXBC
#	define MAX_EMBEDDED_SHADER_DXBC(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _dx11), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _dx11) ) },
#endif // MAX_PLATFORM_SUPPORTS_DXBC

#if MAX_PLATFORM_SUPPORTS_PSSL
#	undef  MAX_EMBEDDED_SHADER_PSSL
#	define MAX_EMBEDDED_SHADER_PSSL(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _pssl), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _pssl_size) ) },
#endif // MAX_PLATFORM_SUPPORTS_PSSL

#if MAX_PLATFORM_SUPPORTS_ESSL
#	undef  MAX_EMBEDDED_SHADER_ESSL
#	define MAX_EMBEDDED_SHADER_ESSL(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _essl), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _essl) ) },
#endif // MAX_PLATFORM_SUPPORTS_ESSL

#if MAX_PLATFORM_SUPPORTS_GLSL
#	undef  MAX_EMBEDDED_SHADER_GLSL
#	define MAX_EMBEDDED_SHADER_GLSL(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _glsl), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _glsl) ) },
#endif // MAX_PLATFORM_SUPPORTS_GLSL

#if MAX_PLATFORM_SUPPORTS_SPIRV
#	undef  MAX_EMBEDDED_SHADER_SPIRV
#	define MAX_EMBEDDED_SHADER_SPIRV(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _spv), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _spv) ) },
#endif // MAX_PLATFORM_SUPPORTS_SPIRV

#if MAX_PLATFORM_SUPPORTS_METAL
#	undef  MAX_EMBEDDED_SHADER_METAL
#	define MAX_EMBEDDED_SHADER_METAL(_renderer, _name) \
	{ _renderer, MAX_EMBEDDED_SHADER_CONCATENATE(_name, _mtl), MAX_EMBEDDED_SHADER_COUNTOF(MAX_EMBEDDED_SHADER_CONCATENATE(_name, _mtl) ) },
#endif // MAX_PLATFORM_SUPPORTS_METAL

#define MAX_EMBEDDED_SHADER(_name)                                                        \
	{                                                                                      \
		#_name,                                                                            \
		{                                                                                  \
			MAX_EMBEDDED_SHADER_PSSL (max::RendererType::Agc,        _name)              \
			MAX_EMBEDDED_SHADER_DXBC (max::RendererType::Direct3D11, _name)              \
			MAX_EMBEDDED_SHADER_DXBC (max::RendererType::Direct3D12, _name)              \
			MAX_EMBEDDED_SHADER_PSSL (max::RendererType::Gnm,        _name)              \
			MAX_EMBEDDED_SHADER_METAL(max::RendererType::Metal,      _name)              \
			MAX_EMBEDDED_SHADER_NVN  (max::RendererType::Nvn,        _name)              \
			MAX_EMBEDDED_SHADER_ESSL (max::RendererType::OpenGLES,   _name)              \
			MAX_EMBEDDED_SHADER_GLSL (max::RendererType::OpenGL,     _name)              \
			MAX_EMBEDDED_SHADER_SPIRV(max::RendererType::Vulkan,     _name)              \
			{ max::RendererType::Noop,  (const uint8_t*)"VSH\x5\x0\x0\x0\x0\x0\x0", 10 }, \
			{ max::RendererType::Count, NULL, 0 }                                         \
		}                                                                                  \
	}

#define MAX_EMBEDDED_SHADER_END()                 \
	{                                              \
		NULL,                                      \
		{                                          \
			{ max::RendererType::Count, NULL, 0 } \
		}                                          \
	}

namespace max
{
	struct EmbeddedShader
	{
		struct Data
		{
			RendererType::Enum type;
			const uint8_t* data;
			uint32_t size;
		};

		const char* name;
		Data data[RendererType::Count];
	};

	/// Create shader from embedded shader data.
	///
	/// @param[in] _es Pointer to `MAX_EMBEDDED_SHADER` data.
	/// @param[in] _type Renderer backend type. See: `max::RendererType`
	/// @param[in] _name Shader name.
	/// @returns Shader handle.
	///
	ShaderHandle createEmbeddedShader(
		  const max::EmbeddedShader* _es
		, RendererType::Enum _type
		, const char* _name
		);

} // namespace max

#endif // MAX_EMBEDDED_SHADER_H_HEADER_GUARD

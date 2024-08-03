/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#ifndef MAX_PLATFORM_H_HEADER_GUARD
#define MAX_PLATFORM_H_HEADER_GUARD

// NOTICE:
// This header file contains platform specific interfaces. It is only
// necessary to use this header in conjunction with creating windows.

#include "max.h"

namespace max
{
	/// Render frame enum.
	///
	/// @attention C99's equivalent binding is `max_render_frame_t`.
	///
	struct RenderFrame
	{
		enum Enum
		{
			NoContext,
			Render,
			Timeout,
			Exiting,

			Count
		};
	};

	/// Render frame.
	///
	/// @param _msecs Timeout in milliseconds.
	///
	/// @returns Current renderer state. See: `max::RenderFrame`.
	///
	/// @attention `max::renderFrame` is blocking call. It waits for
	///   `max::frame` to be called from API thread to process frame.
	///   If timeout value is passed call will timeout and return even
	///   if `max::frame` is not called.
	///
	/// @warning This call should be only used on platforms that don't
	///   allow creating separate rendering thread. If it is called before
	///   to max::init, render thread won't be created by max::init call.
	///
	/// @attention C99's equivalent binding is `max_render_frame`.
	///
	RenderFrame::Enum renderFrame(int32_t _msecs = -1);

	/// Set platform data.
	///
	/// @warning Must be called before `max::init`.
	///
	/// @attention C99's equivalent binding is `max_set_platform_data`.
	///
	void setPlatformData(const PlatformData& _data);

	/// Internal data.
	///
	/// @attention C99's equivalent binding is `max_internal_data_t`.
	///
	struct InternalData
	{
		const struct Caps* caps; //!< Renderer capabilities.
		void* context;           //!< GL context, or D3D device.
	};

	/// Get internal data for interop.
	///
	/// @attention It's expected you understand some max internals before you
	///   use this call.
	///
	/// @warning Must be called only on render thread.
	///
	/// @attention C99's equivalent binding is `max_get_internal_data`.
	///
	const InternalData* getInternalData();

	/// Override internal texture with externally created texture. Previously
	/// created internal texture will released.
	///
	/// @attention It's expected you understand some max internals before you
	///   use this call.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _ptr Native API pointer to texture.
	///
	/// @returns Native API pointer to texture. If result is 0, texture is not created yet from the
	///   main thread.
	///
	/// @warning Must be called only on render thread.
	///
	/// @attention C99's equivalent binding is `max_override_internal_texture_ptr`.
	///
	uintptr_t overrideInternal(TextureHandle _handle, uintptr_t _ptr);

	/// Override internal texture by creating new texture. Previously created
	/// internal texture will released.
	///
	/// @attention It's expected you understand some max internals before you
	///   use this call.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _width Width.
	/// @param[in] _height Height.
	/// @param[in] _numMips Number of mip-maps.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Default texture sampling mode is linear, and wrap mode
	///   is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @returns Native API pointer to texture. If result is 0, texture is not created yet from the
	///   main thread.
	///
	/// @warning Must be called only on render thread.
	///
	/// @attention C99's equivalent binding is `max_override_internal_texture`.
	///
	uintptr_t overrideInternal(
		  TextureHandle _handle
		, uint16_t _width
		, uint16_t _height
		, uint8_t _numMips
		, TextureFormat::Enum _format
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		);

} // namespace max

#endif // MAX_PLATFORM_H_HEADER_GUARD

/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#include "max_p.h"

#if MAX_CONFIG_USE_NOOP

namespace max
{
	const Event* poll()
	{
		return NULL;
	}

	const Event* poll(WindowHandle _handle)
	{
		BX_UNUSED(_handle);
		return NULL;
	}

	void release(const Event* _event)
	{
		BX_UNUSED(_event);
	}

	WindowHandle createWindow(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height, uint32_t _flags, const char* _title)
	{
		BX_UNUSED(_x, _y, _width, _height, _flags, _title);
		WindowHandle handle = { UINT16_MAX };
		return handle;
	}

	void destroyWindow(WindowHandle _handle)
	{
		BX_UNUSED(_handle);
	}

	void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
	{
		BX_UNUSED(_handle, _x, _y);
	}

	void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height)
	{
		BX_UNUSED(_handle, _width, _height);
	}

	void setWindowTitle(WindowHandle _handle, const char* _title)
	{
		BX_UNUSED(_handle, _title);
	}

	void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
	{
		BX_UNUSED(_handle, _flags, _enabled);
	}

	void toggleFullscreen(WindowHandle _handle)
	{
		BX_UNUSED(_handle);
	}

	void setMouseLock(WindowHandle _handle, bool _lock)
	{
		BX_UNUSED(_handle, _lock);
	}

	void* getNativeWindowHandle(WindowHandle _handle)
	{
		BX_UNUSED(_handle);
		return NULL;
	}

	void* getNativeDisplayHandle()
	{
		return NULL;
	}

	max::NativeWindowHandleType::Enum getNativeWindowHandleType()
	{
		BX_UNUSED(_handle);
		return max::NativeWindowHandleType::Default;
	}

} // namespace max

int main(int _argc, const char* const* _argv)
{
	return entry::main(_argc, _argv);
}

#endif // MAX_CONFIG_USE_NOOP

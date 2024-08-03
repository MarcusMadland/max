/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/max/blob/master/LICENSE
 */

#include "max_p.h"

namespace max
{

#define MAX_DECLARE_EMBEDDED_SHADER(_name)                                             \
	extern const uint8_t* BX_CONCATENATE(_name, _pssl);                                 \
	extern const uint32_t BX_CONCATENATE(_name, _pssl_size);                            \
	static const uint8_t  BX_CONCATENATE(_name, _int_pssl)[] = { 0 };                   \
	const uint8_t* BX_CONCATENATE(_name, _pssl) = &BX_CONCATENATE(_name, _int_pssl)[0]; \
	const uint32_t BX_CONCATENATE(_name, _pssl_size) = 1

MAX_DECLARE_EMBEDDED_SHADER(vs_debugfont);
MAX_DECLARE_EMBEDDED_SHADER(fs_debugfont);
MAX_DECLARE_EMBEDDED_SHADER(vs_clear);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear0);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear1);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear2);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear3);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear4);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear5);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear6);
MAX_DECLARE_EMBEDDED_SHADER(fs_clear7);

#undef MAX_DECLARE_EMBEDDED_SHADER

} // namespace max

namespace max { namespace gnm
{
	RendererContextI* rendererCreate(const Init& _init)
	{
		BX_UNUSED(_init);
		return NULL;
	}

	void rendererDestroy()
	{
	}

} /* namespace gnm */ } // namespace max

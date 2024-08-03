/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/max/blob/master/LICENSE
 */

#include "max_p.h"

namespace max { namespace agc
{
	RendererContextI* rendererCreate(const Init& _init)
	{
		BX_UNUSED(_init);
		return NULL;
	}

	void rendererDestroy()
	{
	}

} /* namespace agc */ } // namespace max

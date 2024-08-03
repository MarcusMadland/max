/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/max/blob/master/LICENSE
 */

#ifndef MAX_RENDERDOC_H_HEADER_GUARD
#define MAX_RENDERDOC_H_HEADER_GUARD

namespace max
{
	void* loadRenderDoc();
	void unloadRenderDoc(void*);
	void renderDocTriggerCapture();

} // namespace max

#endif // MAX_RENDERDOC_H_HEADER_GUARD

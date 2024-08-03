/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
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

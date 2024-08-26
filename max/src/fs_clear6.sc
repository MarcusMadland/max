/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "max_shader.sh"

uniform vec4 max_clear_color[8];

void main()
{
	gl_FragData[0] = max_clear_color[0];
	gl_FragData[1] = max_clear_color[1];
	gl_FragData[2] = max_clear_color[2];
	gl_FragData[3] = max_clear_color[3];
	gl_FragData[4] = max_clear_color[4];
	gl_FragData[5] = max_clear_color[5];
	gl_FragData[6] = max_clear_color[6];
}

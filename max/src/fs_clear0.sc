/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "max_shader.sh"

uniform vec4 max_clear_color[8];

void main()
{
	gl_FragColor = max_clear_color[0];
}

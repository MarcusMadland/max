$input a_position

/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "max_shader.sh"

uniform vec4 max_clear_depth;

void main()
{
	gl_Position = vec4(a_position.xy, max_clear_depth.x, 1.0);
}

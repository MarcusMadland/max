$input v_color0, v_color1, v_texcoord0

/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "max_shader.sh"

SAMPLER2D(s_texColor, 0);

void main()
{
	vec4 color = mix(v_color1, v_color0, texture2D(s_texColor, v_texcoord0).xxxx);
	if (color.w < 1.0/255.0)
	{
		discard;
	}
	gl_FragColor = color;
}

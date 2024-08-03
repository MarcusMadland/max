$input v_wpos, v_view, v_normal

#include "../../common/common.sh"

uniform vec4 u_color;

void main()
{
    vec3 color = u_color.xyz;

	gl_FragColor.xyz = color;
	gl_FragColor.w = 1.0;
}


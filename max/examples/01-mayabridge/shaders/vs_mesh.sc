$input a_position, a_normal, a_indices, a_weight
$output v_pos, v_view, v_normal, v_color0

#include "../../common/common.sh"

uniform mat4 u_joints[88];

void main()
{
	// @todo I am normalizing weights here. This should be done in motionlink right?
	float weightSum = a_weight.x + a_weight.y + a_weight.z + a_weight.w;
	vec4 normalizedWeight = a_weight / weightSum;

	mat4 model = u_model[0];

	model = mul(u_model[0], normalizedWeight.x * u_joints[int(a_indices.x)] + 
							normalizedWeight.y * u_joints[int(a_indices.y)] +
							normalizedWeight.z * u_joints[int(a_indices.z)] +
							normalizedWeight.w * u_joints[int(a_indices.w)]);
	vec3 wpos = mul(model, vec4(a_position, 1.0) ).xyz;

	gl_Position = mul(u_viewProj, vec4(wpos, 1.0));
	v_pos = gl_Position.xyz;

	vec3 normal = a_normal.xyz*2.0 - 1.0;
	v_normal = mul(u_modelView, vec4(normal, 0.0) ).xyz;

	v_view = mul(u_modelView, vec4(wpos.xyz, 1.0) ).xyz;

	v_color0 = vec4(normalizedWeight.xyz, 1.0);
}

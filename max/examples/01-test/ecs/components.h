#pragma once

#include <max/max.h>

// Components
struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

struct RenderComponent
{
	max::MeshHandle m_mesh;
	max::MaterialHandle m_material;
};

struct BodyComponent
{
	max::BodyHandle m_body;
	bx::Vec3 m_offset;
};

struct PlayerComponent
{
	uint32_t m_index;
};

#ifdef MAX_BUILD_MINK
struct AnimationComponent
{
	mink::MotionHandle m_motion;
	mink::SamplerHandle m_sampler;
};
#endif // MAX_BUILD_MINK

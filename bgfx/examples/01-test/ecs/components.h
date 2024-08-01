#pragma once

#include <bgfx/bgfx.h>

// Components
struct TransformComponent
{
	bx::Vec3 m_position;
	bx::Quaternion m_rotation;
	bx::Vec3 m_scale;
};

struct RenderComponent
{
	bgfx::MeshHandle m_mesh;
	bgfx::MaterialHandle m_material;
};

struct BodyComponent
{
	bgfx::BodyHandle m_body;
	bx::Vec3 m_offset;
};

struct PlayerComponent
{
	uint32_t m_index;
};

struct AnimationComponent
{
	mink::MotionHandle m_motion;
	mink::SamplerHandle m_sampler;
};

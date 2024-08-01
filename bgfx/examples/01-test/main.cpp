#include <bgfx/bgfx.h>

#include <mink/platform.h>

#include <bx/math.h>
#include <bx/timer.h>

#include <vector>
#include <unordered_map>

#include "sharedbuffer/sync_maya.h"
#include "ecs/components.h"

// Scene
struct Scene
{
	Scene()
	{
		m_timeOffset = (float)bx::getHPCounter();
	}

	enum Entity
	{
		Player,
		Floor,

		Box0,
		Box1,
		Box2,

		Count
	};

	void load()
	{
		// Create resources.
		bgfx::MeshHandle mannequin = bgfx::loadMesh("meshes/mannequin.bin");
		bgfx::MeshHandle cube = bgfx::loadMesh("meshes/cube.bin");

		bgfx::MaterialHandle whiteMaterial = bgfx::createMaterial(bgfx::loadProgram("vs_cube", "fs_cube"));
		float white[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		bgfx::addParameter(whiteMaterial, "u_color", white);

		m_material = bgfx::createMaterial(bgfx::loadProgram("vs_mesh", "fs_mesh"));
		bgfx::addParameter(m_material, "u_color", white);

		mink::MotionHandle motion = mink::loadMotion("motions/mixamo.bin");
		mink::SamplerHandle sampler = mink::createSampler(motion);

		// Create player.
		{
			m_entities.push_back(bgfx::createEntity());
			bgfx::addComponent<RenderComponent>(m_entities[Entity::Player],
				bgfx::createComponent<RenderComponent>({ mannequin, { m_material } })
			);
			bgfx::addComponent<TransformComponent>(m_entities[Entity::Player],
				bgfx::createComponent<TransformComponent>({ {0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
			);
			bgfx::BodyHandle capsule = bgfx::createBodyCapsule(
				bgfx::getComponent<TransformComponent>(m_entities[Entity::Player])->m_position,
				{ 0.0f, 0.0f, 0.0f, 1.0f },
				0.5f,
				0.5f,
				bgfx::LayerType::Moving,
				bgfx::MotionType::Dynamic,
				bgfx::Activation::Activate,
				500.0f,
				BGFX_BODY_CHARACTER);
			bgfx::setFriction(capsule, 10.0f);
			bgfx::addComponent<BodyComponent>(m_entities[Entity::Player],
				bgfx::createComponent<BodyComponent>({ capsule, {0.0f, -1.0f, 0.0f} })
			);
			bgfx::addComponent<PlayerComponent>(m_entities[Entity::Player],
				bgfx::createComponent<PlayerComponent>({ {0} })
			);
			bgfx::addComponent<AnimationComponent>(m_entities[Entity::Player],
				bgfx::createComponent<AnimationComponent>({ motion, sampler }));
		}

		// Create floor.
		{
			m_entities.push_back(bgfx::createEntity());
			bgfx::addComponent<RenderComponent>(m_entities[Entity::Floor],
				bgfx::createComponent<RenderComponent>({ cube, { whiteMaterial } })
			);
			bgfx::addComponent<TransformComponent>(m_entities[Entity::Floor],
				bgfx::createComponent<TransformComponent>({ {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 1.0f, 5.0f} })
			);
			bgfx::BodyHandle floor = bgfx::createBody(
				bgfx::CollisionShape::Box,
				bgfx::getComponent<TransformComponent>(m_entities[Entity::Floor])->m_position,
				{ 0.0f, 0.0f, 0.0f, 1.0f },
				{ 5.0f, 1.0f, 5.0f },
				bgfx::LayerType::NonMoving,
				bgfx::MotionType::Static,
				bgfx::Activation::Activate);
			bgfx::addComponent<BodyComponent>(m_entities[Entity::Floor],
				bgfx::createComponent<BodyComponent>({ floor, { 0.0f, 0.0f, 0.0f } })
			);
		}

		// Create boxes
		{
			m_entities.push_back(bgfx::createEntity());
			bgfx::addComponent<RenderComponent>(m_entities[Entity::Box0],
				bgfx::createComponent<RenderComponent>({ cube, { whiteMaterial } })
			);
			bgfx::addComponent<TransformComponent>(m_entities[Entity::Box0],
				bgfx::createComponent<TransformComponent>({ {0.0f, -1.0f, 3.0f}, bx::fromEuler({0.0f, bx::toRad(90.0f), bx::toRad(30.0f)}), {2.0f, 2.0f, 2.0f}})
			);
			bgfx::BodyHandle body = bgfx::createBody(
				bgfx::CollisionShape::Box,
				bgfx::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_position,
				bgfx::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_rotation,
				bgfx::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_scale,
				bgfx::LayerType::NonMoving,
				bgfx::MotionType::Static,
				bgfx::Activation::Activate);
			bgfx::addComponent<BodyComponent>(m_entities[Entity::Box0],
				bgfx::createComponent<BodyComponent>({ body, { 0.0f, 0.0f, 0.0f } })
			);
		}
		
	}

	void unload()
	{
		for (uint32_t ii = 0; ii < m_entities.size(); ++ii)
		{
			// Destroy component handles.
			if (RenderComponent* comp = bgfx::getComponent<RenderComponent>(m_entities[ii]))
			{
				bgfx::destroy(comp->m_mesh);
				bgfx::destroy(comp->m_material);
			}
			if (BodyComponent* comp = bgfx::getComponent<BodyComponent>(m_entities[ii]))
			{
				bgfx::destroy(comp->m_body);
			}
			if (AnimationComponent* comp = bgfx::getComponent<AnimationComponent>(m_entities[ii]))
			{
				mink::destroy(comp->m_sampler);
				mink::destroy(comp->m_motion);
			}

			// Destroy entities.
			bgfx::destroy(m_entities[ii]);
		}
	}

	void update()
	{
		// Update skinned joints.
		AnimationComponent* ac = bgfx::getComponent<AnimationComponent>(m_entities[Entity::Player]);
		if (ac != NULL)
		{
			mink::Transform pose = mink::getSkinnedTransform(ac->m_sampler);
			bgfx::addParameter(m_material, "u_joints", pose.data, pose.num);
		}
		
	}

	float m_timeOffset;
	bgfx::MaterialHandle m_material;

	std::vector<bgfx::EntityHandle> m_entities;
};

struct MayaScene
{
	void load()
	{
		linkMaya();
	}

	void unload()
	{
		unlinkMaya();
	}

	void update()
	{
		updateMaya();
	}
};

// Input
enum Action
{
	MoveForward,
	MoveRight,

	Count
};

static bgfx::InputMapping s_mapping[] =
{
	// @todo Add gamepad idx as userData.
	{ Action::MoveForward, [](const void* _userData)
		{
			const float axis = (float)bgfx::inputGetGamepadAxis({0}, bgfx::GamepadAxis::LeftY) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return -axis;
			}

			if (bgfx::inputGetKeyState(bgfx::Key::KeyW))
			{
				return 1.0f;
			}

			if (bgfx::inputGetKeyState(bgfx::Key::KeyS))
			{
				return -1.0f;
			}

			return 0.0f;
		}
	},
	{ Action::MoveRight, [](const void* _userData)
		{
			const float axis = (float)bgfx::inputGetGamepadAxis({0}, bgfx::GamepadAxis::LeftX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			if (bgfx::inputGetKeyState(bgfx::Key::KeyD))
			{
				return 1.0f;
			}

			if (bgfx::inputGetKeyState(bgfx::Key::KeyA))
			{
				return -1.0f;
			}

			return 0.0f;
		}
	},

	BGFX_INPUT_MAPPING_END
};

static bgfx::InputBinding s_bindings[] =
{
	{ bgfx::Key::Esc, bgfx::Modifier::None, 1, [](const void* _userData)
		{
			bgfx::destroyWindow({0});
		}
	},
	{ bgfx::Key::KeyF, bgfx::Modifier::LeftCtrl, 1, [](const void* _userData)
		{
			bgfx::toggleFullscreen({0});
		}
	},

	BGFX_INPUT_BINDING_END
};

class ExampleHelloWorld : public bgfx::AppI
{
public:
	ExampleHelloWorld(const char* _name)
		: bgfx::AppI(_name)
	{}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		// Initialize engine.
		bgfx::Init init;
		init.rendererType = bgfx::RendererType::Count;
		init.physicsType = bgfx::PhysicsType::Count;
		init.vendorId = BGFX_PCI_ID_NONE;
		init.platformData.nwh  = bgfx::getNativeWindowHandle({0});
		init.platformData.type = bgfx::getNativeWindowHandleType();
		init.platformData.ndt  = bgfx::getNativeDisplayHandle();
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Call mink::animationFrame before mink::init to signal to mink not to create a animation thread.
		// Running in single threaded mode.
		//mink::animationFrame();
		//mink::init();

		// Setup input.
		bgfx::inputAddMappings({0}, s_mapping);
		bgfx::inputAddBindings("default", s_bindings);

		// Load scene.
		m_scene.load();
	}

	virtual int shutdown() override
	{
		// Unload scene.
		m_scene.unload();

		// Shutdown engine.
		//mink::shutdown();
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		bgfx::MouseState mouseState;

		// Process events.
		if (!bgfx::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Set debug mode.
			bgfx::setDebug(m_debug);

			// Debug drawing.
			bgfx::dbgTextClear();

			bgfx::dbgDrawBegin(0);
			bgfx::dbgDrawGrid(bgfx::Axis::Y, { 0.0f, 0.0f, 0.0f });
			bgfx::dbgDrawEnd();

			// Update scene.
			m_scene.update();

			// Camera
			float view[16];
			bx::mtxLookAt(view, { 0.0f, 5.0f, -8.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, bgfx::getCaps()->homogeneousDepth);

			/*
			// Basic physics system.
			bgfx::System<BodyComponent, TransformComponent> physics;
			physics.each(10, [](bgfx::EntityHandle _entity, void* _userData)
				{
					BodyComponent* bc = bgfx::getComponent<BodyComponent>(_entity);
					TransformComponent* tc = bgfx::getComponent<TransformComponent>(_entity);

					tc->m_position = bx::add(bgfx::getPosition(bc->m_body), bc->m_offset);
					tc->m_rotation = bgfx::getRotation(bc->m_body);
				});

			// Basic movement system.
			bgfx::System<BodyComponent, PlayerComponent, TransformComponent> movement;
			movement.each(10, [](bgfx::EntityHandle _entity, void* _userData)
				{
					constexpr float speed = 5.0f;

					BodyComponent* bc = bgfx::getComponent<BodyComponent>(_entity);
					PlayerComponent* pc = bgfx::getComponent<PlayerComponent>(_entity);
					TransformComponent* tc = bgfx::getComponent<TransformComponent>(_entity);

					// Construct direction based o nmovement input.
					bx::Vec3 direction = { 0.0f, 0.0f, 0.0f };
					direction.x = bgfx::inputGetValue(pc->m_index, Action::MoveRight);
					direction.z = bgfx::inputGetValue(pc->m_index, Action::MoveForward);

					// Normalize direction and set rotation if not zero.
					if (direction.x + direction.z != 0.0f)
					{
						direction = bx::normalize(direction);
						bgfx::setRotation(bc->m_body, { 0.0f, 0.0f, 0.0f, 1.0f }, bgfx::Activation::Activate); // @todo
					}

					// Ground information.
					bgfx::GroundInfo info;
					bgfx::getGroundInfo(bc->m_body, info);

					// Determine new velocity
					const bx::Vec3 horizontalVelocity = bx::mul(direction, speed);
					const bx::Vec3 verticalVelocity = { 0.0f, bgfx::getLinearVelocity(bc->m_body).y, 0.0f };
					const bx::Vec3 groundVelocity = info.m_velocity;

					bx::Vec3 velocity = { 0.0f, 0.0f, 0.0f };

					if ((info.m_state != bgfx::GroundState::InAir) // If on ground.
						&& (verticalVelocity.y - groundVelocity.y < 0.1f)) // And not moving away from ground.
					{
						// Assume velocity of ground when not falling.
						velocity = groundVelocity;

						bgfx::dbgTextPrintf(0, 0, 0x0f, "Vel: %f, %f, %f", velocity.x, velocity.y, velocity.z);

						// Jump @todo
						//velocity = bx::add(velocity, { 0.0f, jumpSpeed, 0.0f });
						//jumpSpeed = 0.0f;
					}
					else
					{

						// Falling
						velocity = verticalVelocity;

						// Gravity
						velocity = bx::add(velocity, bx::mul(bgfx::getGravity(), bgfx::getDeltaTime()));

						bgfx::dbgTextPrintf(0, 0, 0x0f, "Vel: %f, %f, %f", velocity.x, velocity.y, velocity.z);
					}

					// Movement
					velocity = bx::add(velocity, horizontalVelocity);

					// Set velocity
					bgfx::setLinearVelocity(bc->m_body, velocity);
				});

			// Basic animation system.
			bgfx::System<AnimationComponent> animation;
			animation.each(10, [](bgfx::EntityHandle _entity, void* _userData)
				{
					constexpr float animDuration = 5.03f;
					constexpr uint32_t fps = 30;

					AnimationComponent* ac = bgfx::getComponent<AnimationComponent>(_entity);

					// Calculate anim time.
					static float animTime = 0.0f;
					if (animTime > animDuration)
					{
						animTime = 0.0f; 
					}
					else
					{
						animTime += bgfx::getDeltaTime();
					}
					uint32_t sampleIndex = (uint32_t)(animTime * fps);
					uint32_t numSamples = (uint32_t)(animDuration * fps);
					if (sampleIndex > numSamples) sampleIndex = 0; 

					// Sample anim.
					mink::sample(ac->m_sampler, sampleIndex);
				});
			mink::frame();*/

			// Basic render system.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);
			//bgfx::setViewTransform(0, view, proj);
			//bgfx::touch(0);

			bgfx::System<RenderComponent, TransformComponent> renderer;
			renderer.each(10, [](bgfx::EntityHandle _entity, void* _userData)
				{
					RenderComponent* rc = bgfx::getComponent<RenderComponent>(_entity);
					TransformComponent* tc = bgfx::getComponent<TransformComponent>(_entity);
					
					bgfx::MeshQuery* query = bgfx::queryMesh(rc->m_mesh);

					for (uint32_t ii = 0; ii < query->m_num; ++ii)
					{
						float mtx[16];
						bx::mtxSRT(mtx, 
							tc->m_scale.x, tc->m_scale.y, tc->m_scale.z,
							tc->m_rotation.x, tc->m_rotation.y, tc->m_rotation.z, tc->m_rotation.w,
							tc->m_position.x, tc->m_position.y, tc->m_position.z);

						bgfx::setTransform(mtx);

						bgfx::MeshQuery::HandleData& handleData = query->m_handleData[ii];
						if (handleData.m_dynamic)
						{
							bgfx::DynamicVertexBufferHandle dvbh = { handleData.m_vertexHandleIdx };
							bgfx::setVertexBuffer(0, dvbh);

							bgfx::DynamicIndexBufferHandle dibh = { handleData.m_indexHandleIdx };
							bgfx::setIndexBuffer(dibh);
						}
						else
						{
							bgfx::VertexBufferHandle vbh = { handleData.m_vertexHandleIdx };
							bgfx::setVertexBuffer(0, vbh);

							bgfx::IndexBufferHandle ibh = { handleData.m_indexHandleIdx };
							bgfx::setIndexBuffer(ibh);
						}
						
						bgfx::setMaterial(rc->m_material);
						bgfx::setState(0
							| BGFX_STATE_WRITE_RGB
							| BGFX_STATE_WRITE_A
							| BGFX_STATE_WRITE_Z
							| BGFX_STATE_DEPTH_TEST_LESS
							| BGFX_STATE_MSAA
						);

						bgfx::submit(0, rc->m_material);
					}
				});
			
			bgfx::frame();

			return true;
		}

		return false;
	}

	//Scene m_scene;
	MayaScene m_scene;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
};

BGFX_IMPLEMENT_MAIN(ExampleHelloWorld, "00-helloworld");

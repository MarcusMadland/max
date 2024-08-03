#include <max/max.h>

//#include <mink/platform.h>

#include <bx/math.h>
#include <bx/timer.h>

#include <vector>
#include <unordered_map>

#include "sharedbuffer/sync_maya.h"
#include "ecs/components.h"

// Scene
/*
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
		max::MeshHandle mannequin = max::loadMesh("meshes/mannequin.bin");
		max::MeshHandle cube = max::loadMesh("meshes/cube.bin");

		max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
		float white[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		max::addParameter(whiteMaterial, "u_color", white);

		m_material = max::createMaterial(max::loadProgram("vs_mesh", "fs_mesh"));
		max::addParameter(m_material, "u_color", white);

		mink::MotionHandle motion = mink::loadMotion("motions/mixamo.bin");
		mink::SamplerHandle sampler = mink::createSampler(motion);

		// Create player.
		{
			m_entities.push_back(max::createEntity());
			max::addComponent<RenderComponent>(m_entities[Entity::Player],
				max::createComponent<RenderComponent>({ mannequin, { m_material } })
			);
			max::addComponent<TransformComponent>(m_entities[Entity::Player],
				max::createComponent<TransformComponent>({ {0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
			);
			max::BodyHandle capsule = max::createBodyCapsule(
				max::getComponent<TransformComponent>(m_entities[Entity::Player])->m_position,
				{ 0.0f, 0.0f, 0.0f, 1.0f },
				0.5f,
				0.5f,
				max::LayerType::Moving,
				max::MotionType::Dynamic,
				max::Activation::Activate,
				500.0f,
				MAX_BODY_CHARACTER);
			max::setFriction(capsule, 10.0f);
			max::addComponent<BodyComponent>(m_entities[Entity::Player],
				max::createComponent<BodyComponent>({ capsule, {0.0f, -1.0f, 0.0f} })
			);
			max::addComponent<PlayerComponent>(m_entities[Entity::Player],
				max::createComponent<PlayerComponent>({ {0} })
			);
			max::addComponent<AnimationComponent>(m_entities[Entity::Player],
				max::createComponent<AnimationComponent>({ motion, sampler }));
		}

		// Create floor.
		{
			m_entities.push_back(max::createEntity());
			max::addComponent<RenderComponent>(m_entities[Entity::Floor],
				max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
			);
			max::addComponent<TransformComponent>(m_entities[Entity::Floor],
				max::createComponent<TransformComponent>({ {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 1.0f, 5.0f} })
			);
			max::BodyHandle floor = max::createBody(
				max::CollisionShape::Box,
				max::getComponent<TransformComponent>(m_entities[Entity::Floor])->m_position,
				{ 0.0f, 0.0f, 0.0f, 1.0f },
				{ 5.0f, 1.0f, 5.0f },
				max::LayerType::NonMoving,
				max::MotionType::Static,
				max::Activation::Activate);
			max::addComponent<BodyComponent>(m_entities[Entity::Floor],
				max::createComponent<BodyComponent>({ floor, { 0.0f, 0.0f, 0.0f } })
			);
		}

		// Create boxes
		{
			m_entities.push_back(max::createEntity());
			max::addComponent<RenderComponent>(m_entities[Entity::Box0],
				max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
			);
			max::addComponent<TransformComponent>(m_entities[Entity::Box0],
				max::createComponent<TransformComponent>({ {0.0f, -1.0f, 3.0f}, bx::fromEuler({0.0f, bx::toRad(90.0f), bx::toRad(30.0f)}), {2.0f, 2.0f, 2.0f}})
			);
			max::BodyHandle body = max::createBody(
				max::CollisionShape::Box,
				max::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_position,
				max::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_rotation,
				max::getComponent<TransformComponent>(m_entities[Entity::Box0])->m_scale,
				max::LayerType::NonMoving,
				max::MotionType::Static,
				max::Activation::Activate);
			max::addComponent<BodyComponent>(m_entities[Entity::Box0],
				max::createComponent<BodyComponent>({ body, { 0.0f, 0.0f, 0.0f } })
			);
		}
		
	}

	void unload()
	{
		for (uint32_t ii = 0; ii < m_entities.size(); ++ii)
		{
			// Destroy component handles.
			if (RenderComponent* comp = max::getComponent<RenderComponent>(m_entities[ii]))
			{
				max::destroy(comp->m_mesh);
				max::destroy(comp->m_material);
			}
			if (BodyComponent* comp = max::getComponent<BodyComponent>(m_entities[ii]))
			{
				max::destroy(comp->m_body);
			}
			if (AnimationComponent* comp = max::getComponent<AnimationComponent>(m_entities[ii]))
			{
				mink::destroy(comp->m_sampler);
				mink::destroy(comp->m_motion);
			}

			// Destroy entities.
			max::destroy(m_entities[ii]);
		}
	}

	void update()
	{
		// Update skinned joints.
		AnimationComponent* ac = max::getComponent<AnimationComponent>(m_entities[Entity::Player]);
		if (ac != NULL)
		{
			mink::Transform pose = mink::getSkinnedTransform(ac->m_sampler);
			max::addParameter(m_material, "u_joints", pose.data, pose.num);
		}
		
	}

	float m_timeOffset;
	max::MaterialHandle m_material;

	std::vector<max::EntityHandle> m_entities;
};
*/
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

static max::InputMapping s_mapping[] =
{
	// @todo Add gamepad idx as userData.
	{ Action::MoveForward, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::LeftY) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return -axis;
			}

			if (max::inputGetKeyState(max::Key::KeyW))
			{
				return 1.0f;
			}

			if (max::inputGetKeyState(max::Key::KeyS))
			{
				return -1.0f;
			}

			return 0.0f;
		}
	},
	{ Action::MoveRight, [](const void* _userData)
		{
			const float axis = (float)max::inputGetGamepadAxis({0}, max::GamepadAxis::LeftX) / 32767.0f;
			if (axis < -0.1f || axis > 0.1f)
			{
				return axis;
			}

			if (max::inputGetKeyState(max::Key::KeyD))
			{
				return 1.0f;
			}

			if (max::inputGetKeyState(max::Key::KeyA))
			{
				return -1.0f;
			}

			return 0.0f;
		}
	},

	MAX_INPUT_MAPPING_END
};

static max::InputBinding s_bindings[] =
{
	{ max::Key::Esc, max::Modifier::None, 1, [](const void* _userData)
		{
			max::destroyWindow({0});
		}
	},
	{ max::Key::KeyF, max::Modifier::LeftCtrl, 1, [](const void* _userData)
		{
			max::toggleFullscreen({0});
		}
	},

	MAX_INPUT_BINDING_END
};

class ExampleHelloWorld : public max::AppI
{
public:
	ExampleHelloWorld(const char* _name)
		: max::AppI(_name)
	{}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		m_width  = _width;
		m_height = _height;
		m_debug  = MAX_DEBUG_TEXT;
		m_reset  = MAX_RESET_VSYNC;

		// Initialize engine.
		max::Init init;
		init.rendererType = max::RendererType::Count;
		init.physicsType = max::PhysicsType::Count;
		init.vendorId = MAX_PCI_ID_NONE;
		init.platformData.nwh  = max::getNativeWindowHandle({0});
		init.platformData.type = max::getNativeWindowHandleType();
		init.platformData.ndt  = max::getNativeDisplayHandle();
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		max::init(init);

		// Call mink::animationFrame before mink::init to signal to mink not to create a animation thread.
		// Running in single threaded mode.
		//mink::animationFrame();
		//mink::init();

		// Setup input.
		max::inputAddMappings({0}, s_mapping);
		max::inputAddBindings("default", s_bindings);

		// Load scene.
		m_scene.load();
	}

	virtual int shutdown() override
	{
		// Unload scene.
		m_scene.unload();

		// Shutdown engine.
		//mink::shutdown();
		max::shutdown();

		return 0;
	}

	bool update() override
	{
		max::MouseState mouseState;

		// Process events.
		if (!max::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Set debug mode.
			max::setDebug(m_debug);

			// Debug drawing.
			max::dbgTextClear();

			max::dbgDrawBegin(0);
			max::dbgDrawGrid(max::Axis::Y, { 0.0f, 0.0f, 0.0f });
			max::dbgDrawEnd();

			// Update scene.
			m_scene.update();

			// Camera
			float view[16];
			bx::mtxLookAt(view, { 0.0f, 5.0f, -8.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, max::getCaps()->homogeneousDepth);

			/*
			// Basic physics system.
			max::System<BodyComponent, TransformComponent> physics;
			physics.each(10, [](max::EntityHandle _entity, void* _userData)
				{
					BodyComponent* bc = max::getComponent<BodyComponent>(_entity);
					TransformComponent* tc = max::getComponent<TransformComponent>(_entity);

					tc->m_position = bx::add(max::getPosition(bc->m_body), bc->m_offset);
					tc->m_rotation = max::getRotation(bc->m_body);
				});

			// Basic movement system.
			max::System<BodyComponent, PlayerComponent, TransformComponent> movement;
			movement.each(10, [](max::EntityHandle _entity, void* _userData)
				{
					constexpr float speed = 5.0f;

					BodyComponent* bc = max::getComponent<BodyComponent>(_entity);
					PlayerComponent* pc = max::getComponent<PlayerComponent>(_entity);
					TransformComponent* tc = max::getComponent<TransformComponent>(_entity);

					// Construct direction based o nmovement input.
					bx::Vec3 direction = { 0.0f, 0.0f, 0.0f };
					direction.x = max::inputGetValue(pc->m_index, Action::MoveRight);
					direction.z = max::inputGetValue(pc->m_index, Action::MoveForward);

					// Normalize direction and set rotation if not zero.
					if (direction.x + direction.z != 0.0f)
					{
						direction = bx::normalize(direction);
						max::setRotation(bc->m_body, { 0.0f, 0.0f, 0.0f, 1.0f }, max::Activation::Activate); // @todo
					}

					// Ground information.
					max::GroundInfo info;
					max::getGroundInfo(bc->m_body, info);

					// Determine new velocity
					const bx::Vec3 horizontalVelocity = bx::mul(direction, speed);
					const bx::Vec3 verticalVelocity = { 0.0f, max::getLinearVelocity(bc->m_body).y, 0.0f };
					const bx::Vec3 groundVelocity = info.m_velocity;

					bx::Vec3 velocity = { 0.0f, 0.0f, 0.0f };

					if ((info.m_state != max::GroundState::InAir) // If on ground.
						&& (verticalVelocity.y - groundVelocity.y < 0.1f)) // And not moving away from ground.
					{
						// Assume velocity of ground when not falling.
						velocity = groundVelocity;

						max::dbgTextPrintf(0, 0, 0x0f, "Vel: %f, %f, %f", velocity.x, velocity.y, velocity.z);

						// Jump @todo
						//velocity = bx::add(velocity, { 0.0f, jumpSpeed, 0.0f });
						//jumpSpeed = 0.0f;
					}
					else
					{

						// Falling
						velocity = verticalVelocity;

						// Gravity
						velocity = bx::add(velocity, bx::mul(max::getGravity(), max::getDeltaTime()));

						max::dbgTextPrintf(0, 0, 0x0f, "Vel: %f, %f, %f", velocity.x, velocity.y, velocity.z);
					}

					// Movement
					velocity = bx::add(velocity, horizontalVelocity);

					// Set velocity
					max::setLinearVelocity(bc->m_body, velocity);
				});

			// Basic animation system.
			max::System<AnimationComponent> animation;
			animation.each(10, [](max::EntityHandle _entity, void* _userData)
				{
					constexpr float animDuration = 5.03f;
					constexpr uint32_t fps = 30;

					AnimationComponent* ac = max::getComponent<AnimationComponent>(_entity);

					// Calculate anim time.
					static float animTime = 0.0f;
					if (animTime > animDuration)
					{
						animTime = 0.0f; 
					}
					else
					{
						animTime += max::getDeltaTime();
					}
					uint32_t sampleIndex = (uint32_t)(animTime * fps);
					uint32_t numSamples = (uint32_t)(animDuration * fps);
					if (sampleIndex > numSamples) sampleIndex = 0; 

					// Sample anim.
					mink::sample(ac->m_sampler, sampleIndex);
				});
			mink::frame();*/

			// Basic render system.
			max::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			max::setViewClear(0
				, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);
			//max::setViewTransform(0, view, proj);
			//max::touch(0);

			max::System<RenderComponent, TransformComponent> renderer;
			renderer.each(10, [](max::EntityHandle _entity, void* _userData)
				{
					RenderComponent* rc = max::getComponent<RenderComponent>(_entity);
					TransformComponent* tc = max::getComponent<TransformComponent>(_entity);
					
					max::MeshQuery* query = max::queryMesh(rc->m_mesh);

					for (uint32_t ii = 0; ii < query->m_num; ++ii)
					{
						float mtx[16];
						bx::mtxSRT(mtx, 
							tc->m_scale.x, tc->m_scale.y, tc->m_scale.z,
							tc->m_rotation.x, tc->m_rotation.y, tc->m_rotation.z, tc->m_rotation.w,
							tc->m_position.x, tc->m_position.y, tc->m_position.z);

						max::setTransform(mtx);

						max::MeshQuery::HandleData& handleData = query->m_handleData[ii];
						if (handleData.m_dynamic)
						{
							max::DynamicVertexBufferHandle dvbh = { handleData.m_vertexHandleIdx };
							max::setVertexBuffer(0, dvbh);

							max::DynamicIndexBufferHandle dibh = { handleData.m_indexHandleIdx };
							max::setIndexBuffer(dibh);
						}
						else
						{
							max::VertexBufferHandle vbh = { handleData.m_vertexHandleIdx };
							max::setVertexBuffer(0, vbh);

							max::IndexBufferHandle ibh = { handleData.m_indexHandleIdx };
							max::setIndexBuffer(ibh);
						}
						
						max::setMaterial(rc->m_material);
						max::setState(0
							| MAX_STATE_WRITE_RGB
							| MAX_STATE_WRITE_A
							| MAX_STATE_WRITE_Z
							| MAX_STATE_DEPTH_TEST_LESS
							| MAX_STATE_MSAA
						);

						max::submit(0, rc->m_material);
					}
				});
			
			max::frame();

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

MAX_IMPLEMENT_MAIN(ExampleHelloWorld, "00-helloworld");

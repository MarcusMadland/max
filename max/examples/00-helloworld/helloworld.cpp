#include <max/max.h>
#include <bx/math.h>
#include <bx/timer.h>

#include <vector>
#include <unordered_map>

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
//

// Scene
struct Scene
{
	Scene()
	{
		m_timeOffset = (float)bx::getHPCounter();
	}

	void load()
	{
		// Create resources.
		max::MeshHandle mannequin = max::loadMesh("meshes/mannequin.bin");
		max::MeshHandle bunny = max::loadMesh("meshes/bunny.bin");
		max::MeshHandle orb = max::loadMesh("meshes/orb.bin");
		max::MeshHandle cube = max::loadMesh("meshes/cube.bin");

		max::TextureHandle rgba = max::loadTexture("textures/fieldstone-rgba.dds");
		max::TextureHandle normal = max::loadTexture("textures/fieldstone-n.dds");

		max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
		float white[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		max::addParameter(whiteMaterial, "u_color", white);

		m_material = max::createMaterial(max::loadProgram("vs_bump", "fs_bump"));
		max::addParameter(m_material, "s_texColor", 0, rgba);
		max::addParameter(m_material, "s_texNormal", 1, normal);

		// Create bunny.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[0],
			max::createComponent<RenderComponent>({ bunny, { m_material } })
		);
		max::addComponent<TransformComponent>(m_entities[0],
			max::createComponent<TransformComponent>({ {-2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);

		// Create orb.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[1],
			max::createComponent<RenderComponent>({ orb, { m_material } })
		);
		max::addComponent<TransformComponent>(m_entities[1],
			max::createComponent<TransformComponent>({ {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);

		// Create player.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[2],
			max::createComponent<RenderComponent>({ mannequin, { m_material } })
		);
		max::addComponent<TransformComponent>(m_entities[2],
			max::createComponent<TransformComponent>({ {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);
		max::BodyHandle capsule = max::createBodyCapsule(
			max::getComponent<TransformComponent>(m_entities[2])->m_position,
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			0.5f,
			0.5f,
			max::LayerType::Moving,
			max::MotionType::Dynamic,
			max::Activation::Activate,
			5.0f,
			MAX_BODY_CHARACTER);
		max::setFriction(capsule, 100.0f);
		max::addComponent<BodyComponent>(m_entities[2],
			max::createComponent<BodyComponent>({ capsule, {0.0f, -1.0f, 0.0f} })
		);
		max::addComponent<PlayerComponent>(m_entities[2],
			max::createComponent<PlayerComponent>({ {0} })
		);

		// Create floor.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[3],
			max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		max::addComponent<TransformComponent>(m_entities[3],
			max::createComponent<TransformComponent>({ {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 1.0f, 5.0f} })
		);
		max::BodyHandle floor = max::createBody(
			max::CollisionShape::Box,
			max::getComponent<TransformComponent>(m_entities[3])->m_position,
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 5.0f, 1.0f, 5.0f },
			max::LayerType::NonMoving,
			max::MotionType::Static,
			max::Activation::Activate);
		max::addComponent<BodyComponent>(m_entities[3],
			max::createComponent<BodyComponent>({ floor, { 0.0f, 0.0f, 0.0f } })
		);
		
		// Create box.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[4],
			max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		TransformComponent transform = { {0.0f, 1.0f, 3.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {2.0f, 1.0f, 0.1f} };
		max::addComponent<TransformComponent>(m_entities[4],
			max::createComponent<TransformComponent>(transform)
		);
		max::BodyHandle box = max::createBody(
			max::CollisionShape::Box,
			transform.m_position,
			transform.m_rotation,
			transform.m_scale,
			max::LayerType::NonMoving,
			max::MotionType::Static,
			max::Activation::Activate);
		max::addComponent<BodyComponent>(m_entities[4],
			max::createComponent<BodyComponent>({ box, { 0.0f, 0.0f, 0.0f } })
		);

		// Create box.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[5],
			max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		TransformComponent transform2 = { {2.0f, 3.0f, 2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.2f, 0.2f, 0.2f} };
		max::addComponent<TransformComponent>(m_entities[5],
			max::createComponent<TransformComponent>(transform2)
		);
		max::BodyHandle box2 = max::createBody(
			max::CollisionShape::Box,
			transform2.m_position,
			transform2.m_rotation,
			transform2.m_scale,
			max::LayerType::Moving,
			max::MotionType::Dynamic,
			max::Activation::Activate);
		max::addComponent<BodyComponent>(m_entities[5],
			max::createComponent<BodyComponent>({ box2, { 0.0f, 0.0f, 0.0f } })
		);
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

			// Destroy entities.
			max::destroy(m_entities[ii]);
		}
	}

	void update()
	{
		float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));

		// Move lights around.
		float lightPosRadius[4][4];
		for (uint32_t ii = 0; ii < 4; ++ii)
		{
			lightPosRadius[ii][0] = bx::sin((time * (0.1f + ii * 0.17f) + ii * bx::kPiHalf * 1.37f)) * 3.0f;
			lightPosRadius[ii][1] = bx::cos((time * (0.2f + ii * 0.29f) + ii * bx::kPiHalf * 1.49f)) * 3.0f;
			lightPosRadius[ii][2] = -2.5f;
			lightPosRadius[ii][3] = 3.0f;
		}
		max::addParameter(m_material, "u_lightPosRadius", &lightPosRadius[0][0], 4);

		// Update lights colors.
		float lightRgbInnerR[4][4] =
		{
			{ 1.0f, 0.7f, 0.2f, 0.8f },
			{ 0.7f, 0.2f, 1.0f, 0.8f },
			{ 0.2f, 1.0f, 0.7f, 0.8f },
			{ 1.0f, 0.4f, 0.2f, 0.8f },
		};
		max::addParameter(m_material, "u_lightRgbInnerR", &lightRgbInnerR[0][0], 4);
	}

	float m_timeOffset;
	max::MaterialHandle m_material;

	std::vector<max::EntityHandle> m_entities;
};
//

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
//

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

		// Shutdown max.
		max::shutdown();

		return 0;
	}

	bool update() override
	{
		max::MouseState mouseState;

		// Process events.
		if (!max::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Update scene.
			m_scene.update();

			// Set debug mode.
			max::setDebug(m_debug);

			// Debug drawing.
			max::dbgDrawBegin(0);
			max::dbgDrawGrid(max::Axis::Y, { 0.0f, 0.0f, 0.0f });
			max::dbgDrawEnd();

			// Camera
			float view[16];
			bx::mtxLookAt(view, { 0.0f, 10.0f, -15.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, max::getCaps()->homogeneousDepth);

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

					// Make sure we are not falling
					bx::Vec3 desiredDirection = { 0.0f, 0.0f, 0.0f };
					desiredDirection.x = max::inputGetValue(pc->m_index, Action::MoveRight);
					desiredDirection.z = max::inputGetValue(pc->m_index, Action::MoveForward);

					bx::Vec3 desiredVelocity = bx::mul(desiredDirection, speed);
					max::addLinearAndAngularVelocity(bc->m_body, desiredVelocity, { 0.0f, 0.0f, 0.0f });
				});

			// Basic render system.
			max::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			max::setViewClear(0
				, MAX_CLEAR_COLOR | MAX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);
			max::setViewTransform(0, view, proj);

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
						max::setVertexBuffer(0, query->m_vbh[ii]);
						max::setIndexBuffer(query->m_ibh[ii]);
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

	Scene m_scene;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
};

MAX_IMPLEMENT_MAIN(ExampleHelloWorld, "00-helloworld");

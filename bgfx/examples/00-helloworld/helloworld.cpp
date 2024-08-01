#include <bgfx/bgfx.h>
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
		bgfx::MeshHandle mannequin = bgfx::loadMesh("meshes/mannequin.bin");
		bgfx::MeshHandle bunny = bgfx::loadMesh("meshes/bunny.bin");
		bgfx::MeshHandle orb = bgfx::loadMesh("meshes/orb.bin");
		bgfx::MeshHandle cube = bgfx::loadMesh("meshes/cube.bin");

		bgfx::TextureHandle rgba = bgfx::loadTexture("textures/fieldstone-rgba.dds");
		bgfx::TextureHandle normal = bgfx::loadTexture("textures/fieldstone-n.dds");

		bgfx::MaterialHandle whiteMaterial = bgfx::createMaterial(bgfx::loadProgram("vs_cube", "fs_cube"));
		float white[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		bgfx::addParameter(whiteMaterial, "u_color", white);

		m_material = bgfx::createMaterial(bgfx::loadProgram("vs_bump", "fs_bump"));
		bgfx::addParameter(m_material, "s_texColor", 0, rgba);
		bgfx::addParameter(m_material, "s_texNormal", 1, normal);

		// Create bunny.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[0],
			bgfx::createComponent<RenderComponent>({ bunny, { m_material } })
		);
		bgfx::addComponent<TransformComponent>(m_entities[0],
			bgfx::createComponent<TransformComponent>({ {-2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);

		// Create orb.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[1],
			bgfx::createComponent<RenderComponent>({ orb, { m_material } })
		);
		bgfx::addComponent<TransformComponent>(m_entities[1],
			bgfx::createComponent<TransformComponent>({ {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);

		// Create player.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[2],
			bgfx::createComponent<RenderComponent>({ mannequin, { m_material } })
		);
		bgfx::addComponent<TransformComponent>(m_entities[2],
			bgfx::createComponent<TransformComponent>({ {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} })
		);
		bgfx::BodyHandle capsule = bgfx::createBodyCapsule(
			bgfx::getComponent<TransformComponent>(m_entities[2])->m_position,
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			0.5f,
			0.5f,
			bgfx::LayerType::Moving,
			bgfx::MotionType::Dynamic,
			bgfx::Activation::Activate,
			5.0f,
			BGFX_BODY_CHARACTER);
		bgfx::setFriction(capsule, 100.0f);
		bgfx::addComponent<BodyComponent>(m_entities[2],
			bgfx::createComponent<BodyComponent>({ capsule, {0.0f, -1.0f, 0.0f} })
		);
		bgfx::addComponent<PlayerComponent>(m_entities[2],
			bgfx::createComponent<PlayerComponent>({ {0} })
		);

		// Create floor.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[3],
			bgfx::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		bgfx::addComponent<TransformComponent>(m_entities[3],
			bgfx::createComponent<TransformComponent>({ {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {5.0f, 1.0f, 5.0f} })
		);
		bgfx::BodyHandle floor = bgfx::createBody(
			bgfx::CollisionShape::Box,
			bgfx::getComponent<TransformComponent>(m_entities[3])->m_position,
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 5.0f, 1.0f, 5.0f },
			bgfx::LayerType::NonMoving,
			bgfx::MotionType::Static,
			bgfx::Activation::Activate);
		bgfx::addComponent<BodyComponent>(m_entities[3],
			bgfx::createComponent<BodyComponent>({ floor, { 0.0f, 0.0f, 0.0f } })
		);
		
		// Create box.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[4],
			bgfx::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		TransformComponent transform = { {0.0f, 1.0f, 3.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {2.0f, 1.0f, 0.1f} };
		bgfx::addComponent<TransformComponent>(m_entities[4],
			bgfx::createComponent<TransformComponent>(transform)
		);
		bgfx::BodyHandle box = bgfx::createBody(
			bgfx::CollisionShape::Box,
			transform.m_position,
			transform.m_rotation,
			transform.m_scale,
			bgfx::LayerType::NonMoving,
			bgfx::MotionType::Static,
			bgfx::Activation::Activate);
		bgfx::addComponent<BodyComponent>(m_entities[4],
			bgfx::createComponent<BodyComponent>({ box, { 0.0f, 0.0f, 0.0f } })
		);

		// Create box.
		m_entities.push_back(bgfx::createEntity());
		bgfx::addComponent<RenderComponent>(m_entities[5],
			bgfx::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		TransformComponent transform2 = { {2.0f, 3.0f, 2.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.2f, 0.2f, 0.2f} };
		bgfx::addComponent<TransformComponent>(m_entities[5],
			bgfx::createComponent<TransformComponent>(transform2)
		);
		bgfx::BodyHandle box2 = bgfx::createBody(
			bgfx::CollisionShape::Box,
			transform2.m_position,
			transform2.m_rotation,
			transform2.m_scale,
			bgfx::LayerType::Moving,
			bgfx::MotionType::Dynamic,
			bgfx::Activation::Activate);
		bgfx::addComponent<BodyComponent>(m_entities[5],
			bgfx::createComponent<BodyComponent>({ box2, { 0.0f, 0.0f, 0.0f } })
		);
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

			// Destroy entities.
			bgfx::destroy(m_entities[ii]);
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
		bgfx::addParameter(m_material, "u_lightPosRadius", &lightPosRadius[0][0], 4);

		// Update lights colors.
		float lightRgbInnerR[4][4] =
		{
			{ 1.0f, 0.7f, 0.2f, 0.8f },
			{ 0.7f, 0.2f, 1.0f, 0.8f },
			{ 0.2f, 1.0f, 0.7f, 0.8f },
			{ 1.0f, 0.4f, 0.2f, 0.8f },
		};
		bgfx::addParameter(m_material, "u_lightRgbInnerR", &lightRgbInnerR[0][0], 4);
	}

	float m_timeOffset;
	bgfx::MaterialHandle m_material;

	std::vector<bgfx::EntityHandle> m_entities;
};
//

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
//

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

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		bgfx::MouseState mouseState;

		// Process events.
		if (!bgfx::processEvents(m_width, m_height, m_debug, m_reset, &mouseState) )
		{
			// Update scene.
			m_scene.update();

			// Set debug mode.
			bgfx::setDebug(m_debug);

			// Debug drawing.
			bgfx::dbgDrawBegin(0);
			bgfx::dbgDrawGrid(bgfx::Axis::Y, { 0.0f, 0.0f, 0.0f });
			bgfx::dbgDrawEnd();

			// Camera
			float view[16];
			bx::mtxLookAt(view, { 0.0f, 10.0f, -15.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, bgfx::getCaps()->homogeneousDepth);

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

					// Make sure we are not falling
					bx::Vec3 desiredDirection = { 0.0f, 0.0f, 0.0f };
					desiredDirection.x = bgfx::inputGetValue(pc->m_index, Action::MoveRight);
					desiredDirection.z = bgfx::inputGetValue(pc->m_index, Action::MoveForward);

					bx::Vec3 desiredVelocity = bx::mul(desiredDirection, speed);
					bgfx::addLinearAndAngularVelocity(bc->m_body, desiredVelocity, { 0.0f, 0.0f, 0.0f });
				});

			// Basic render system.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);
			bgfx::setViewTransform(0, view, proj);

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
						bgfx::setVertexBuffer(0, query->m_vbh[ii]);
						bgfx::setIndexBuffer(query->m_ibh[ii]);
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

	Scene m_scene;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
};

BGFX_IMPLEMENT_MAIN(ExampleHelloWorld, "00-helloworld");

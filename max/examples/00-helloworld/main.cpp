#include <max/max.h>

#include <bx/math.h>
#include <bx/timer.h>

#include <vector>

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

// Scene
struct Scene
{
	Scene()
	{
		m_timeOffset = bx::getHPCounter();
	}

	void load()
	{
		// Create resources.
		max::MeshHandle bunny = max::loadMesh("meshes/bunny.bin");
		max::MeshHandle cube = max::loadMesh("meshes/cube.bin");

		max::TextureHandle rgba = max::loadTexture("textures/fieldstone-rgba.dds");
		max::TextureHandle normal = max::loadTexture("textures/fieldstone-n.dds");

		max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
		float white[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		max::addParameter(whiteMaterial, "u_color", white);

		m_material = max::createMaterial(max::loadProgram("vs_bump", "fs_bump"));
		max::addParameter(m_material, "s_texColor", 0, rgba);
		max::addParameter(m_material, "s_texNormal", 1, normal);

		// Create Entities.
		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[0],
			max::createComponent<RenderComponent>({ bunny, { m_material } })
		);
		max::addComponent<TransformComponent>(m_entities[0],
			max::createComponent<TransformComponent>({
				{ 0.0f, 0.0f, 0.0f },
				bx::fromEuler({ 0.0f, bx::toRad(180.0f), 0.0f }),
				{ 2.0f, 2.0f, 2.0f }
			})
		);

		m_entities.push_back(max::createEntity());
		max::addComponent<RenderComponent>(m_entities[1],
			max::createComponent<RenderComponent>({ cube, { whiteMaterial } })
		);
		max::addComponent<TransformComponent>(m_entities[1],
			max::createComponent<TransformComponent>({
				{ 0.0f, -0.9f, 0.0f },
				{ 0.0f, 0.0f, 0.0f, 1.0f },
				{ 5.0f, 1.0f, 5.0f }
			})
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

	int64_t m_timeOffset;
	max::MaterialHandle m_material;

	std::vector<max::EntityHandle> m_entities;
};

// Application
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
			bx::mtxLookAt(view, { 0.0f, 5.0f, -10.0f }, { 0.0f, 0.5f, 0.0f });
			float proj[16];
			bx::mtxProj(proj, 45.0f, (float)m_width / (float)m_height, 0.01f, 100.0f, max::getCaps()->homogeneousDepth);

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

						max::VertexBufferHandle vbh = { query->m_handleData[ii].m_vertexHandleIdx };
						max::IndexBufferHandle ibh = { query->m_handleData[ii].m_indexHandleIdx };
						max::setVertexBuffer(0, vbh);
						max::setIndexBuffer(ibh);

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

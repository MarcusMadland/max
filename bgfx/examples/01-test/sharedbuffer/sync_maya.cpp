#include "sync_maya.h"

#include <bgfx/bgfx.h>
#include <bx/sharedbuffer.h>

#include <string>
#include <unordered_map>

#include "../ecs/components.h"
#include "../../../../../syncmax-plugin/include/shared_data.h" // Plugin decides data layout

struct EntityHandle
{
	EntityHandle()
		: m_handle(BGFX_INVALID_HANDLE)
	{}

	bgfx::EntityHandle m_handle;
};

struct Mayalink
{
	void init()
	{
		m_isSynced = false;

		if (m_buffer.init("MayaToMax", sizeof(SharedData)))
		{
			m_shared = BX_NEW(bgfx::getAllocator(), SharedData);
			m_shared->m_processed = false;
			m_shared->m_meshChanged.m_changed      = false;
			m_shared->m_transformChanged.m_changed = false;

			BX_TRACE("Shared memory initialized successfully.");
		}
		else
		{
			BX_TRACE("Failed to initialize shared memory.");
		}
	}

	void shutdown()
	{
		bx::deleteObject(bgfx::getAllocator(), m_shared);
		m_shared = NULL;

		m_buffer.shutdown();
	}

	void update()
	{
		if (!m_buffer.read(m_shared, sizeof(SharedData)))
		{
			m_isSynced = false;
			return;
		}

		if (!m_shared->m_processed)
		{
			m_isSynced = false;
			return;
		}

		// Update synced.
		SharedData::SyncEvent& syncEvent = m_shared->m_sync;
		m_isSynced = syncEvent.m_isSynced;

		// Update camera.
		SharedData::CameraEvent& cameraEvent = m_shared->m_camera;
		bgfx::setViewTransform(0, cameraEvent.m_view, cameraEvent.m_proj); // @todo

		// Update mesh.
		SharedData::MeshEvent& meshEvent = m_shared->m_meshChanged;
		if (meshEvent.m_changed)
		{
			bgfx::VertexLayout layout;
			layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.end();

			bgfx::EntityHandle& entity = m_entities[meshEvent.m_name].m_handle;
			if (isValid(entity))
			{
				if (meshEvent.m_numVertices == 0 && meshEvent.m_numIndices == 0)
				{
					// Destroy entity.
					if (RenderComponent* rc = bgfx::getComponent<RenderComponent>(entity))
					{
						bgfx::destroy(rc->m_mesh);
						bgfx::destroy(rc->m_material);
					}
					bgfx::destroy(entity);
					entity = BGFX_INVALID_HANDLE;
				}
				else
				{
					// Update entity.
					RenderComponent* rc = bgfx::getComponent<RenderComponent>(entity);
					if (rc != NULL)
					{
						const bgfx::Memory* vertices = bgfx::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
						const bgfx::Memory* indices = bgfx::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

						bgfx::update(rc->m_mesh, vertices, indices);
					}
				}
			}
			else if (meshEvent.m_numVertices != 0 && meshEvent.m_numIndices != 0)
			{
				// Create entity.
				entity = bgfx::createEntity();

				TransformComponent tc = {
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 1.0f },
					{ 1.0f, 1.0f, 1.0f },
					// @todo Will it always be this? Even when importing meshes?
				};
				bgfx::addComponent<TransformComponent>(entity, bgfx::createComponent<TransformComponent>(tc));

				// @todo How much extra memory should I allocate for these dynamic buffers? 256 enough?
				const bgfx::Memory* vertices = bgfx::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices) + 256);
				const bgfx::Memory* indices = bgfx::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t) + 256);

				bgfx::MaterialHandle whiteMaterial = bgfx::createMaterial(bgfx::loadProgram("vs_cube", "fs_cube"));
				float white[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
				bgfx::addParameter(whiteMaterial, "u_color", white);

				RenderComponent rc;
				rc.m_mesh = bgfx::createMesh(vertices, indices, layout, true);
				rc.m_material = whiteMaterial;
				bgfx::addComponent<RenderComponent>(entity, bgfx::createComponent<RenderComponent>(rc));
			}
		}

		// Update transform.
		SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
		if (transformEvent.m_changed)
		{
			bgfx::EntityHandle& entity = m_entities[transformEvent.m_name].m_handle;
			if (isValid(entity))
			{
				// Update entity.
				TransformComponent* tc = bgfx::getComponent<TransformComponent>(entity);
				if (tc != NULL)
				{
					tc->m_position = {
						transformEvent.m_pos[0],
						transformEvent.m_pos[1],
						transformEvent.m_pos[2]
					};
					tc->m_rotation = {
						transformEvent.m_rotation[0],
						transformEvent.m_rotation[1],
						transformEvent.m_rotation[2],
						transformEvent.m_rotation[3]
					};
					tc->m_scale = {
						transformEvent.m_scale[0],
						transformEvent.m_scale[1],
						transformEvent.m_scale[2]
					};
				}
			}
		}
	}

	bool m_isSynced;

	bx::SharedBuffer m_buffer;
	SharedData* m_shared;

	std::unordered_map<std::string, EntityHandle> m_entities;
};

static Mayalink* s_ctx;

void linkMaya()
{
	s_ctx = BX_NEW(bgfx::getAllocator(), Mayalink);
	s_ctx->init();
}

void unlinkMaya()
{
	s_ctx->shutdown();
	bx::deleteObject(bgfx::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void updateMaya()
{
	if (s_ctx->m_isSynced)
	{
		bgfx::dbgTextPrintf(0, 0, 0xf, "Connected to Maya...");
	}

	s_ctx->update();

	float deltaTime = bgfx::getDeltaTime(); 
	static float s_accumulatedTime = 0.0f;
	s_accumulatedTime += deltaTime;

	if (s_accumulatedTime >= 0.03f)
	{
		
		s_accumulatedTime = 0.0f;
	}
}

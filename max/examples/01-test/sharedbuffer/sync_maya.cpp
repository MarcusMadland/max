#include "sync_maya.h"

#include <max/max.h>
#include <bx/sharedbuffer.h>

#include <string>
#include <unordered_map>

#include "../ecs/components.h"
#include "../../../../../maya-bridge/include/shared_data.h" // Plugin decides data layout

struct EntityHandle
{
	EntityHandle()
		: m_handle(MAX_INVALID_HANDLE)
	{}

	max::EntityHandle m_handle;
};

struct Mayalink
{
	void init()
	{
		m_isSynced = false;

		if (m_buffer.init("maya-bridge", sizeof(SharedData)))
		{
			m_shared = BX_NEW(max::getAllocator(), SharedData);
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
		bx::deleteObject(max::getAllocator(), m_shared);
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
		max::setViewTransform(0, cameraEvent.m_view, cameraEvent.m_proj); // @todo

		// Update mesh.
		SharedData::MeshEvent& meshEvent = m_shared->m_meshChanged;
		if (meshEvent.m_changed)
		{
			max::VertexLayout layout;
			layout.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::Normal, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
				.end();

			max::EntityHandle& entity = m_entities[meshEvent.m_name].m_handle;
			if (isValid(entity))
			{
				if (meshEvent.m_numVertices == 0 && meshEvent.m_numIndices == 0)
				{
					// Destroy entity.
					if (RenderComponent* rc = max::getComponent<RenderComponent>(entity))
					{
						max::destroy(rc->m_mesh);
						max::destroy(rc->m_material);
					}
					max::destroy(entity);
					entity = MAX_INVALID_HANDLE;
				}
				else
				{
					// Update entity.
					RenderComponent* rc = max::getComponent<RenderComponent>(entity);
					if (rc != NULL)
					{
						const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
						const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

						max::update(rc->m_mesh, vertices, indices);
					}
				}
			}
			else if (meshEvent.m_numVertices != 0 && meshEvent.m_numIndices != 0)
			{
				// Create entity.
				entity = max::createEntity();

				TransformComponent tc = {
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f, 0.0f, 1.0f },
					{ 1.0f, 1.0f, 1.0f },
					// @todo Will it always be this? Even when importing meshes?
				};
				max::addComponent<TransformComponent>(entity, max::createComponent<TransformComponent>(tc));

				const max::Memory* vertices = max::copy(meshEvent.m_vertices, layout.getSize(meshEvent.m_numVertices));
				const max::Memory* indices = max::copy(meshEvent.m_indices, meshEvent.m_numIndices * sizeof(uint16_t));

				max::MaterialHandle whiteMaterial = max::createMaterial(max::loadProgram("vs_cube", "fs_cube"));
				float white[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
				max::addParameter(whiteMaterial, "u_color", white);

				RenderComponent rc;
				rc.m_mesh = max::createMesh(vertices, indices, layout, true);
				rc.m_material = whiteMaterial;
				max::addComponent<RenderComponent>(entity, max::createComponent<RenderComponent>(rc));
			}
		}

		// Update transform.
		SharedData::TransformEvent transformEvent = m_shared->m_transformChanged;
		if (transformEvent.m_changed)
		{
			max::EntityHandle& entity = m_entities[transformEvent.m_name].m_handle;
			if (isValid(entity))
			{
				// Update entity.
				TransformComponent* tc = max::getComponent<TransformComponent>(entity);
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
	s_ctx = BX_NEW(max::getAllocator(), Mayalink);
	s_ctx->init();
}

void unlinkMaya()
{
	s_ctx->shutdown();
	bx::deleteObject(max::getAllocator(), s_ctx);
	s_ctx = NULL;
}

void updateMaya()
{
	if (s_ctx->m_isSynced)
	{
		max::dbgTextPrintf(0, 0, 0xf, "Connected to Maya...");
	}

	s_ctx->update();

	float deltaTime = max::getDeltaTime(); 
	static float s_accumulatedTime = 0.0f;
	s_accumulatedTime += deltaTime;

	if (s_accumulatedTime >= 0.03f)
	{
		
		s_accumulatedTime = 0.0f;
	}
}

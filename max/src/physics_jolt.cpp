/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/max/blob/master/LICENSE
 */

#include "max_p.h"

#if (MAX_CONFIG_PHYSICS_JOLT)

#include <jolt/jolt.h>
#include <jolt/registertypes.h>
#include <jolt/core/factory.h>
#include <jolt/core/tempallocator.h>
#include <jolt/core/jobsystemthreadpool.h>
#include <jolt/physics/physicssettings.h>
#include <jolt/physics/physicssystem.h>
#include <jolt/physics/collision/shape/boxshape.h>
#include <jolt/physics/collision/shape/capsuleshape.h>
#include <jolt/physics/collision/shape/sphereshape.h>
#include <jolt/physics/body/bodycreationsettings.h>
#include <jolt/physics/body/bodyactivationlistener.h>
#include <jolt/physics/body/bodymanager.h>
#include <jolt/physics/body/motionproperties.h>
#include <jolt/physics/character/character.h>
#include <jolt/renderer/debugrenderer.h>
#include <jolt/renderer/debugrenderersimple.h>

// @todo Check
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/ObjectStream/TypeDeclarations.h>

JPH_SUPPRESS_WARNINGS

#include <bx/allocator.h>
#include <bx/bounds.h>
#include <bx/easing.h>
#include <bx/rng.h>

namespace JPH
{
	static void TraceImpl(const char* inFMT, ...)
	{
		va_list args;
		va_start(args, inFMT);
		bx::debugPrintf(inFMT, args);
		va_end(args);
	}

#ifdef JPH_ENABLE_ASSERTS

	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
	{
		// @todo
		return true;
	};

#endif // JPH_ENABLE_ASSERTS

	class DebugRendererImpl : public JPH::DebugRendererSimple
	{
	public:
		virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
		{
		}

		virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override
		{
			// @todo Batch together. 
			max::dbgDrawBegin(0);
			max::dbgDrawSetColor(0xff00ffff);
			//max::dbgDrawSetWireframe(true);
			bx::Triangle triangle;
			triangle.v0 = { inV1.GetX(), inV1.GetY(), inV1.GetZ() };
			triangle.v1 = { inV2.GetX(), inV2.GetY(), inV2.GetZ() };
			triangle.v2 = { inV3.GetX(), inV3.GetY(), inV3.GetZ() };
			max::dbgDraw(triangle);
			max::dbgDrawEnd();
		}

		virtual void DrawText3D(JPH::RVec3Arg inPosition, const string_view& inString, JPH::ColorArg inColor, float inHeight) override
		{
		}
	};

	namespace Layers
	{
		static constexpr ObjectLayer NON_MOVING = 0;
		static constexpr ObjectLayer MOVING = 1;
		static constexpr ObjectLayer NUM_LAYERS = 2;
	};

	class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case Layers::NON_MOVING:
				return inObject2 == Layers::MOVING; // Non moving only collides with moving
			case Layers::MOVING:
				return true; // Moving collides with everything
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	namespace BroadPhaseLayers
	{
		static constexpr BroadPhaseLayer NON_MOVING(0);
		static constexpr BroadPhaseLayer MOVING(1);
		static constexpr uint NUM_LAYERS(2);
	};

	class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}

		virtual uint GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
		{
			switch ((BroadPhaseLayer::Type)inLayer)
			{
			case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
			case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
			default:													JPH_ASSERT(false); return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case Layers::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::MOVING;
			case Layers::MOVING:
				return true;
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	class MyContactListener : public ContactListener
	{
	public:
		virtual ValidateResult	OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
		{
			return ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
		{
		}

		virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
		{
		}

		virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
		{
		}
	};

	class MyBodyActivationListener : public BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
		}

		virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
		}
	};

	class MyCollector : public CollideShapeCollector
	{
	public:
		explicit MyCollector(Vec3Arg inUp, RVec3 inBaseOffset) : mUp(inUp), mBaseOffset(inBaseOffset) { }

		virtual void AddHit(const CollideShapeResult& inResult) override
		{
			Vec3 normal = -inResult.mPenetrationAxis.Normalized();
			float dot = normal.Dot(mUp);
			if (dot > mBestDot) 
			{
				mGroundBodyID = inResult.mBodyID2;
				mGroundBodySubShapeID = inResult.mSubShapeID2;
				mGroundPosition = mBaseOffset + inResult.mContactPointOn2;
				mGroundNormal = normal;
				mBestDot = dot;
			}
		}

		BodyID				mGroundBodyID;
		SubShapeID			mGroundBodySubShapeID;
		RVec3				mGroundPosition = RVec3::sZero();
		Vec3				mGroundNormal = Vec3::sZero();

	private:
		RVec3				mBaseOffset;
		Vec3				mUp;
		float				mBestDot = -FLT_MAX;
	};
}

namespace max { namespace jolt
{
	static JPH::Vec3 toVec(const bx::Vec3& _vec)
	{
		return JPH::Vec3(_vec.x, _vec.y, _vec.z);
	}

	static JPH::Quat toQuat(const bx::Quaternion& _quat)
	{
		return JPH::Quat(_quat.x, _quat.y, _quat.z, _quat.w);
	}

	static bx::Vec3 fromVec(const JPH::Vec3& _vec)
	{
		return { _vec.GetX(), _vec.GetY(), _vec.GetZ() };
	}

	static bx::Quaternion fromQuat(const JPH::Quat& _quat)
	{
		return { -_quat.GetX(), -_quat.GetY(), -_quat.GetZ(), _quat.GetW() };
	}

	static JPH::PhysicsSystem* s_system;

	struct BodyRef
	{
		BodyRef()
			: m_id(JPH::BodyID::cInvalidBodyID)
		{}

		JPH::Ref<JPH::Shape> createShape(CollisionShape::Enum shape, const bx::Vec3& pos, const bx::Quaternion& quat, const bx::Vec3& scale)
		{
			JPH::ShapeSettings::ShapeResult result;

			switch (shape)
			{
			case max::CollisionShape::Sphere:
			{
				JPH::SphereShapeSettings settings(scale.x);
				result = settings.Create();
				break;
			}
			case max::CollisionShape::Box:
			{
				JPH::BoxShapeSettings settings(JPH::Vec3(scale.x, scale.y, scale.z));
				result = settings.Create();
				break;
			}
			case max::CollisionShape::Capsule:
			{
				JPH::CapsuleShapeSettings settings(scale.y, scale.x);
				result = settings.Create();
				break;
			}
			}

			if (result.IsValid())
			{
				return result.Get();
			}

			return NULL;
		}

		void create(CollisionShape::Enum _shape, const bx::Vec3& _pos, const bx::Quaternion& _quat, const bx::Vec3& _scale, LayerType::Enum _layer, MotionType::Enum _motion, Activation::Enum _activation, float _maxVelocity, uint8_t _flags)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();

			m_shape = createShape(_shape, _pos, _quat, _scale);

			m_layer = _layer;

			JPH::BodyCreationSettings settings(
				m_shape,
				toVec(_pos),
				toQuat(_quat),
				(JPH::EMotionType)_motion,
				(uint16_t)m_layer
			);

			settings.mAllowedDOFs = (JPH::EAllowedDOFs)_flags;
			//settings.mGravityFactor = 2.0f;
			//settings.mMaxLinearVelocity = _maxVelocity; @todo Let's not expose this?

			m_id = bodyInterface.CreateAndAddBody(settings, (JPH::EActivation)_activation);
		}

		void destroy()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.RemoveBody(m_id);
			bodyInterface.DestroyBody(m_id);
		}

		void checkCollision(JPH::RMat44Arg inCenterOfMassTransform, JPH::Vec3Arg inMovementDirection, JPH::RVec3Arg inBaseOffset, JPH::CollideShapeCollector& ioCollector) const
		{
			JPH::DefaultBroadPhaseLayerFilter broadphase_layer_filter = s_system->GetDefaultBroadPhaseLayerFilter(m_layer);
			JPH::DefaultObjectLayerFilter object_layer_filter = s_system->GetDefaultLayerFilter(m_layer);
			JPH::IgnoreSingleBodyFilter body_filter(m_id); // Ignore my own body

			JPH::CollideShapeSettings settings;
			settings.mMaxSeparationDistance = 0.05f; // Collision Toelrance
			settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
			settings.mActiveEdgeMovementDirection = inMovementDirection;
			settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;

			s_system->GetNarrowPhaseQuery().CollideShape(m_shape, JPH::Vec3::sReplicate(1.0f), inCenterOfMassTransform, settings, inBaseOffset, ioCollector, broadphase_layer_filter, object_layer_filter, body_filter);
		}

		void checkCollision(JPH::RVec3Arg inPosition, JPH::QuatArg inRotation, JPH::Vec3Arg inMovementDirection, JPH::RVec3Arg inBaseOffset, JPH::CollideShapeCollector& ioCollector) const
		{
			JPH::RMat44 center_of_mass = JPH::RMat44::sRotationTranslation(inRotation, inPosition).PreTranslated(m_shape->GetCenterOfMass());
			checkCollision(center_of_mass, inMovementDirection, inBaseOffset, ioCollector);
		}

		void simulate()
		{
			JPH::RVec3 char_pos;
			JPH::Quat char_rot;
			JPH::Vec3 char_vel;
			{
				JPH::BodyLockRead lock(s_system->GetBodyLockInterface(), m_id);
				if (!lock.Succeeded())
				{
					return;
				}

				const JPH::Body& body = lock.GetBody();
				char_pos = body.GetPosition();
				char_rot = body.GetRotation();
				char_vel = body.GetLinearVelocity();
			}

			JPH::MyCollector collector(m_up, char_pos);
			checkCollision(char_pos, char_rot, char_vel, char_pos, collector); 

			GroundInfo info;

			//mGroundBodySubShapeID = collector.mGroundBodySubShapeID;
			info.m_position = fromVec(collector.mGroundPosition);
			info.m_normal = fromVec(collector.mGroundNormal);

			JPH::BodyLockRead lock(s_system->GetBodyLockInterface(), collector.mGroundBodyID);
			if (lock.Succeeded())
			{
				const JPH::Body& body = lock.GetBody();

				JPH::RMat44 inv_transform = JPH::RMat44::sInverseRotationTranslation(char_rot, char_pos);
				if (m_supportingVolume.SignedDistance(JPH::Vec3(inv_transform * toVec(info.m_position))) > 0.0f)
				{
					info.m_state = GroundState::NotSupported;
				}
				else if (JPH::Cos(m_maxSlopeAngle) < 0.9999f && toVec(info.m_normal).Dot(m_up) < m_maxSlopeAngle)
				{
					info.m_state = GroundState::OnSteepGround;
				}
				else
				{
					info.m_state = GroundState::OnGround;
				}

				info.m_velocity = fromVec(body.GetPointVelocity(toVec(info.m_position)));
			}
			else
			{
				info.m_state = GroundState::InAir;
				info.m_velocity = { 0.0f, 0.0f, 0.0f };
			}

			m_ground = info;
		}

		void setPosition(const bx::Vec3& _pos, Activation::Enum _activation)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.SetPosition(m_id, toVec(_pos), (JPH::EActivation)_activation);
		}

		bx::Vec3 getPosition()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			return fromVec(bodyInterface.GetPosition(m_id));
		}

		void setRotation(const bx::Quaternion& _rot, Activation::Enum _activation)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.SetRotation(m_id, toQuat(_rot), (JPH::EActivation)_activation);
		}

		bx::Quaternion getRotation()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			return fromQuat(bodyInterface.GetRotation(m_id));
		}

		void setLinearVelocity(const bx::Vec3& _velocity)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.SetLinearVelocity(m_id, toVec(_velocity));
		}

		bx::Vec3 getLinearVelocity()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			return fromVec(bodyInterface.GetLinearVelocity(m_id));
		}

		void setAngularVelocity(const bx::Vec3& _angularVelocity)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.SetAngularVelocity(m_id, toVec(_angularVelocity));
		}

		bx::Vec3 getAngularVelocity()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			return fromVec(bodyInterface.GetAngularVelocity(m_id));
		}

		void addLinearAndAngularVelocity(const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.AddLinearAndAngularVelocity(m_id, toVec(_linearVelocity), toVec(_angularVelocity));
		}

		void addLinearImpulse(const bx::Vec3& _impulse)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.AddImpulse(m_id, toVec(_impulse));
		}

		void addAngularImpulse(const bx::Vec3& _impulse)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.AddAngularImpulse(m_id, toVec(_impulse));
		}

		void addBuoyancyImpulse(const bx::Vec3& _surfacePosition, const bx::Vec3& _surfaceNormal, float _buoyancy, float _linearDrag, float _angularDrag, const bx::Vec3& _fluidVelocity, const bx::Vec3& _gravity, float _deltaTime)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.ApplyBuoyancyImpulse(m_id, toVec(_surfacePosition), toVec(_surfaceNormal), _buoyancy, _linearDrag, _angularDrag, toVec(_fluidVelocity), toVec(_gravity), _deltaTime);
		}

		void addForce(const bx::Vec3& _force, Activation::Enum _activation)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.AddForce(m_id, toVec(_force), (JPH::EActivation)_activation);
		}

		void addTorque(const bx::Vec3& _torque, Activation::Enum _activation)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.AddTorque(m_id, toVec(_torque), (JPH::EActivation)_activation);
		}

		void addMovement(const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.MoveKinematic(m_id, toVec(_position), toQuat(_rotation), _deltaTime);
		}

		void setFriction(float _friction)
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			bodyInterface.SetFriction(m_id, _friction);
		}

		float getFriction()
		{
			JPH::BodyInterface& bodyInterface = s_system->GetBodyInterface();
			return bodyInterface.GetFriction(m_id);
		}

		void getGroundInfo(GroundInfo& _info)
		{
			_info = m_ground;
		}

		JPH::BodyID m_id;

		/// 
		JPH::Ref<JPH::Shape> m_shape;

		/// 
		LayerType::Enum m_layer;

		/// Plane, defined in local space relative to the body. Every contact behind this plane can support the
		/// body, every contact in front of this plane is treated as only colliding with the body.
		/// Default: Accept any contact.
		JPH::Plane m_supportingVolume { JPH::Vec3::sAxisY(), -1.0e10f }; // @todo Constructor.

		/// Up vector of body. @todo This should not be here?
		JPH::Vec3 m_up { JPH::Vec3::sAxisY() };;

		/// 
		float m_maxSlopeAngle = JPH::DegreesToRadians(50.0f);

		/// 
		GroundInfo m_ground;
	};

	struct PhysicsContextJolt : public PhysicsContextI
	{
		PhysicsContextJolt()
			: m_allocator(NULL)
			, m_jobSystem(NULL)
		{}

		~PhysicsContextJolt()
		{}

		bool init(const Init& _init)
		{
			JPH::RegisterDefaultAllocator();

			JPH::Trace = JPH::TraceImpl;
			JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JPH::AssertFailedImpl;)

			JPH::Factory::sInstance = new JPH::Factory();
			JPH::DebugRenderer::sInstance = new JPH::DebugRendererImpl();

			JPH::RegisterTypes();

			m_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
			m_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

			// @todo Make max::Init settings for physics.
			s_system = new JPH::PhysicsSystem();
			s_system->Init(
				MAX_CONFIG_MAX_BODIES,
				0, // @todo
				MAX_CONFIG_MAX_BODY_PAIRS,
				MAX_CONFIG_MAX_CONTACT_CONSTRAINTS,
				m_bpli,
				m_ovbplf,
				m_olpf
			);

			s_system->SetBodyActivationListener(&m_bodyActivationListener);
			s_system->SetContactListener(&m_contactListener);

			s_system->SetGravity(JPH::Vec3(0.0f, -10.0f, 0.0f));

			s_system->OptimizeBroadPhase();

			return true;
		}

		void shutdown()
		{
			JPH::UnregisterTypes();

			delete JPH::Factory::sInstance;
			delete JPH::DebugRenderer::sInstance;
			delete m_allocator;
			delete m_jobSystem;
			delete s_system;
		}

		PhysicsType::Enum getPhysicsType() const override
		{
			return PhysicsType::Jolt;
		}

		const char* getPhysicsName() const override
		{
			return MAX_PHYSICS_JOLT_NAME;
		}

		void simulate(const float _dt) override
		{
			// Main simulaton
			int collisionSteps = 1;
			JPH::EPhysicsUpdateError err = s_system->Update(_dt, collisionSteps, m_allocator, m_jobSystem);
			BX_ASSERT(err == JPH::EPhysicsUpdateError::None, "Error updating physics system.");

			// Post simulation on bodies.
			for (uint32_t ii = 0; ii < MAX_CONFIG_MAX_BODIES; ++ii)
			{
				BodyRef& br = m_bodies[ii];
				if (br.m_id.IsInvalid())
				{
					break;
				}

				br.simulate();
			}

			// @todo Check for MAX_DEBUG_PHYSICS
			JPH::BodyManager::DrawSettings settings;
			s_system->DrawBodies(settings, JPH::DebugRenderer::sInstance);
		}

		void createBody(BodyHandle _handle, CollisionShape::Enum _shape, const bx::Vec3& _pos, const bx::Quaternion& _quat, const bx::Vec3& _scale, LayerType::Enum _layer, MotionType::Enum _motion, Activation::Enum _activation, float _maxVelocity, uint8_t _flags) override
		{
			m_bodies[_handle.idx].create(_shape, _pos, _quat, _scale, _layer, _motion, _activation, _maxVelocity, _flags);
		}

		void destroyBody(BodyHandle _handle) override
		{
			m_bodies[_handle.idx].destroy();
		}

		void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation) override
		{
			m_bodies[_handle.idx].setPosition(_pos, _activation);
		}

		bx::Vec3 getPosition(BodyHandle _handle) override
		{
			return m_bodies[_handle.idx].getPosition();
		}

		void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation) override
		{
			m_bodies[_handle.idx].setRotation(_rot, _activation);
		}

		bx::Quaternion getRotation(BodyHandle _handle) override
		{
			return m_bodies[_handle.idx].getRotation();
		}

		void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity) override
		{
			m_bodies[_handle.idx].setLinearVelocity(_velocity);
		}

		bx::Vec3 getLinearVelocity(BodyHandle _handle) override
		{
			return m_bodies[_handle.idx].getLinearVelocity();
		}

		void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity) override
		{
			m_bodies[_handle.idx].setAngularVelocity(_angularVelocity);
		}

		bx::Vec3 getAngularVelocity(BodyHandle _handle) override
		{
			return m_bodies[_handle.idx].getAngularVelocity();
		}

		void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity) override
		{
			m_bodies[_handle.idx].addLinearAndAngularVelocity(_linearVelocity, _angularVelocity);
		}

		void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse) override
		{
			m_bodies[_handle.idx].addLinearImpulse(_impulse);
		}

		void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse) override
		{
			m_bodies[_handle.idx].addAngularImpulse(_impulse);
		}

		void addBuoyancyImpulse(BodyHandle _handle, const bx::Vec3& _surfacePosition, const bx::Vec3& _surfaceNormal, float _buoyancy, float _linearDrag, float _angularDrag, const bx::Vec3& _fluidVelocity, const bx::Vec3& _gravity, float _deltaTime) override
		{
			m_bodies[_handle.idx].addBuoyancyImpulse(_surfacePosition, _surfaceNormal, _buoyancy, _linearDrag, _angularDrag, _fluidVelocity, _gravity, _deltaTime);
		}

		void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation) override
		{
			m_bodies[_handle.idx].addForce(_force, _activation);
		}

		void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation) override
		{
			m_bodies[_handle.idx].addTorque(_torque, _activation);
		}

		void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime) override
		{
			m_bodies[_handle.idx].addMovement(_position, _rotation, _deltaTime);
		}

		void setFriction(BodyHandle _handle, float _friction) override
		{
			m_bodies[_handle.idx].setFriction(_friction);
		}

		float getFriction(BodyHandle _handle) override
		{
			return m_bodies[_handle.idx].getFriction();
		}

		void getGroundInfo(BodyHandle _handle, GroundInfo& _info) override
		{
			m_bodies[_handle.idx].getGroundInfo(_info);
		}

		const bx::Vec3 getGravity() override
		{
			return fromVec(s_system->GetGravity());
		}

		JPH::TempAllocatorImpl* m_allocator;
		JPH::JobSystemThreadPool* m_jobSystem;

		JPH::BPLayerInterfaceImpl m_bpli;
		JPH::ObjectVsBroadPhaseLayerFilterImpl m_ovbplf;
		JPH::ObjectLayerPairFilterImpl m_olpf;

		JPH::MyBodyActivationListener m_bodyActivationListener;
		JPH::MyContactListener m_contactListener;

		BodyRef m_bodies[MAX_CONFIG_MAX_BODIES];
	};

	PhysicsContextJolt* s_physicsJolt;

	PhysicsContextI* physicsCreate(const Init& _init)
	{
		s_physicsJolt = BX_NEW(g_allocator, PhysicsContextJolt);
		if (!s_physicsJolt->init(_init) )
		{
			bx::deleteObject(g_allocator, s_physicsJolt);
			s_physicsJolt = NULL;
		}
		return s_physicsJolt;
	}

	void physicsDestroy()
	{
		s_physicsJolt->shutdown();
		bx::deleteObject(g_allocator, s_physicsJolt);
		s_physicsJolt = NULL;
	}

} } // namespace max

#else

namespace max { namespace jolt
{
	PhysicsContextI* physicsCreate(const Init& _init)
	{
		BX_UNUSED(_init);
		return NULL;
	}

	void physicsDestroy()
	{
	}
} /* namespace jolt */ } // namespace max

#endif // MAX_CONFIG_PHYSICS_JOLT

/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#include "max_p.h"

namespace max { namespace noop
{
	struct PhysicsContextNOOP : public PhysicsContextI
	{
		PhysicsContextNOOP()
		{
		}

		~PhysicsContextNOOP()
		{
		}

		PhysicsType::Enum getPhysicsType() const override
		{
			return PhysicsType::Noop;
		}

		const char* getPhysicsName() const override
		{
			return MAX_PHYSICS_NOOP_NAME;
		}

		void simulate(const float _dt) override
		{
		}

		void createBody(BodyHandle _handle, CollisionShape::Enum _shape, const bx::Vec3& _pos, const bx::Quaternion& _quat, const bx::Vec3& _scale, LayerType::Enum _layer, MotionType::Enum _motion, Activation::Enum _activation, float _maxVelocity, uint8_t _flags) override
		{
		}

		void destroyBody(BodyHandle _handle) override
		{
		}

		void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation) override
		{
		}

		bx::Vec3 getPosition(BodyHandle _handle) override
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation) override
		{
		}

		bx::Quaternion getRotation(BodyHandle _handle) override
		{
			return { 0.0f, 0.0f, 0.0f, 1.0f };
		}

		void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity) override
		{
		}

		bx::Vec3 getLinearVelocity(BodyHandle _handle) override
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity) override
		{
		}

		bx::Vec3 getAngularVelocity(BodyHandle _handle) override
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity) override
		{
		}

		void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse) override
		{
		}

		void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse) override
		{
		}

		void addBuoyancyImpulse(BodyHandle _handle, const bx::Vec3& _surfacePosition, const bx::Vec3& _surfaceNormal, float _buoyancy, float _linearDrag, float _angularDrag, const bx::Vec3& _fluidVelocity, const bx::Vec3& _gravity, float _deltaTime) override
		{
		}

		void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation) override
		{
		}

		void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation) override
		{
		}

		void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime) override
		{
		}

		void setFriction(BodyHandle _handle, float _friction) override
		{
		}

		float getFriction(BodyHandle _handle) override
		{
			return 0.0f;
		}

		void getGroundInfo(BodyHandle _handle, GroundInfo& _info) override
		{
		}

		const bx::Vec3 getGravity() override
		{
			return { 0.0f, 0.0f, 0.0f };
		}
	};

	static PhysicsContextNOOP* s_physicsNOOP;

	PhysicsContextI* physicsCreate(const Init& _init)
	{
		BX_UNUSED(_init);
		s_physicsNOOP = BX_NEW(g_allocator, PhysicsContextNOOP);
		return s_physicsNOOP;
	}

	void physicsDestroy()
	{
		bx::deleteObject(g_allocator, s_physicsNOOP);
		s_physicsNOOP = NULL;
	}
} /* namespace noop */ } // namespace max

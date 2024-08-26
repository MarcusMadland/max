set(JOLT_DIR ${MAX_DIR}/3rdparty/jolt)

set(DEBUG_RENDERER_IN_DISTRIBUTION ON)

set(JOLT_PHYSICS_SRC_FILES
	${JOLT_DIR}/AABBTree/AABBTreeBuilder.cpp
	${JOLT_DIR}/AABBTree/AABBTreeBuilder.h
	${JOLT_DIR}/AABBTree/AABBTreeToBuffer.h
	${JOLT_DIR}/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h
	${JOLT_DIR}/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h
	${JOLT_DIR}/Core/ARMNeon.h
	${JOLT_DIR}/Core/Array.h
	${JOLT_DIR}/Core/Atomics.h
	${JOLT_DIR}/Core/ByteBuffer.h
	${JOLT_DIR}/Core/Color.cpp
	${JOLT_DIR}/Core/Color.h
	${JOLT_DIR}/Core/Core.h
	${JOLT_DIR}/Core/Factory.cpp
	${JOLT_DIR}/Core/Factory.h
	${JOLT_DIR}/Core/FixedSizeFreeList.h
	${JOLT_DIR}/Core/FixedSizeFreeList.inl
	${JOLT_DIR}/Core/FPControlWord.h
	${JOLT_DIR}/Core/FPException.h
	${JOLT_DIR}/Core/FPFlushDenormals.h
	${JOLT_DIR}/Core/HashCombine.h
	${JOLT_DIR}/Core/InsertionSort.h
	${JOLT_DIR}/Core/IssueReporting.cpp
	${JOLT_DIR}/Core/IssueReporting.h
	${JOLT_DIR}/Core/JobSystem.h
	${JOLT_DIR}/Core/JobSystem.inl
	${JOLT_DIR}/Core/JobSystemSingleThreaded.cpp
	${JOLT_DIR}/Core/JobSystemSingleThreaded.h
	${JOLT_DIR}/Core/JobSystemThreadPool.cpp
	${JOLT_DIR}/Core/JobSystemThreadPool.h
	${JOLT_DIR}/Core/JobSystemWithBarrier.cpp
	${JOLT_DIR}/Core/JobSystemWithBarrier.h
	${JOLT_DIR}/Core/LinearCurve.cpp
	${JOLT_DIR}/Core/LinearCurve.h
	${JOLT_DIR}/Core/LockFreeHashMap.h
	${JOLT_DIR}/Core/LockFreeHashMap.inl
	${JOLT_DIR}/Core/Memory.cpp
	${JOLT_DIR}/Core/Memory.h
	${JOLT_DIR}/Core/Mutex.h
	${JOLT_DIR}/Core/MutexArray.h
	${JOLT_DIR}/Core/NonCopyable.h
	${JOLT_DIR}/Core/Profiler.cpp
	${JOLT_DIR}/Core/Profiler.h
	${JOLT_DIR}/Core/Profiler.inl
	${JOLT_DIR}/Core/QuickSort.h
	${JOLT_DIR}/Core/Reference.h
	${JOLT_DIR}/Core/Result.h
	${JOLT_DIR}/Core/RTTI.cpp
	${JOLT_DIR}/Core/RTTI.h
	${JOLT_DIR}/Core/Semaphore.cpp
	${JOLT_DIR}/Core/Semaphore.h
	${JOLT_DIR}/Core/StaticArray.h
	${JOLT_DIR}/Core/StreamIn.h
	${JOLT_DIR}/Core/StreamOut.h
	${JOLT_DIR}/Core/StreamUtils.h
	${JOLT_DIR}/Core/StreamWrapper.h
	${JOLT_DIR}/Core/StringTools.cpp
	${JOLT_DIR}/Core/StringTools.h
	${JOLT_DIR}/Core/STLAlignedAllocator.h
	${JOLT_DIR}/Core/STLAllocator.h
	${JOLT_DIR}/Core/STLTempAllocator.h
	${JOLT_DIR}/Core/TempAllocator.h
	${JOLT_DIR}/Core/TickCounter.cpp
	${JOLT_DIR}/Core/TickCounter.h
	${JOLT_DIR}/Core/UnorderedMap.h
	${JOLT_DIR}/Core/UnorderedSet.h
	${JOLT_DIR}/Geometry/AABox.h
	${JOLT_DIR}/Geometry/AABox4.h
	${JOLT_DIR}/Geometry/ClipPoly.h
	${JOLT_DIR}/Geometry/ClosestPoint.h
	${JOLT_DIR}/Geometry/ConvexHullBuilder.cpp
	${JOLT_DIR}/Geometry/ConvexHullBuilder.h
	${JOLT_DIR}/Geometry/ConvexHullBuilder2D.cpp
	${JOLT_DIR}/Geometry/ConvexHullBuilder2D.h
	${JOLT_DIR}/Geometry/ConvexSupport.h
	${JOLT_DIR}/Geometry/Ellipse.h
	${JOLT_DIR}/Geometry/EPAConvexHullBuilder.h
	${JOLT_DIR}/Geometry/EPAPenetrationDepth.h
	${JOLT_DIR}/Geometry/GJKClosestPoint.h
	${JOLT_DIR}/Geometry/IndexedTriangle.h
	${JOLT_DIR}/Geometry/Indexify.cpp
	${JOLT_DIR}/Geometry/Indexify.h
	${JOLT_DIR}/Geometry/MortonCode.h
	${JOLT_DIR}/Geometry/OrientedBox.cpp
	${JOLT_DIR}/Geometry/OrientedBox.h
	${JOLT_DIR}/Geometry/Plane.h
	${JOLT_DIR}/Geometry/RayAABox.h
	${JOLT_DIR}/Geometry/RayAABox8.h
	${JOLT_DIR}/Geometry/RayCapsule.h
	${JOLT_DIR}/Geometry/RayCylinder.h
	${JOLT_DIR}/Geometry/RaySphere.h
	${JOLT_DIR}/Geometry/RayTriangle.h
	${JOLT_DIR}/Geometry/RayTriangle8.h
	${JOLT_DIR}/Geometry/Sphere.h
	${JOLT_DIR}/Geometry/Triangle.h
	${JOLT_DIR}/Jolt.cmake
	${JOLT_DIR}/Jolt.h
	${JOLT_DIR}/Math/DMat44.h
	${JOLT_DIR}/Math/DMat44.inl
	${JOLT_DIR}/Math/Double3.h
	${JOLT_DIR}/Math/DVec3.h
	${JOLT_DIR}/Math/DVec3.inl
	${JOLT_DIR}/Math/DynMatrix.h
	${JOLT_DIR}/Math/EigenValueSymmetric.h
	${JOLT_DIR}/Math/FindRoot.h
	${JOLT_DIR}/Math/Float2.h
	${JOLT_DIR}/Math/Float3.h
	${JOLT_DIR}/Math/Float4.h
	${JOLT_DIR}/Math/GaussianElimination.h
	${JOLT_DIR}/Math/HalfFloat.h
	${JOLT_DIR}/Math/Mat44.h
	${JOLT_DIR}/Math/Mat44.inl
	${JOLT_DIR}/Math/Math.h
	${JOLT_DIR}/Math/MathTypes.h
	${JOLT_DIR}/Math/Matrix.h
	${JOLT_DIR}/Math/Quat.h
	${JOLT_DIR}/Math/Quat.inl
	${JOLT_DIR}/Math/Real.h
	${JOLT_DIR}/Math/Swizzle.h
	${JOLT_DIR}/Math/Trigonometry.h
	${JOLT_DIR}/Math/UVec4.h
	${JOLT_DIR}/Math/UVec4.inl
	${JOLT_DIR}/Math/UVec8.h
	${JOLT_DIR}/Math/UVec8.inl
	${JOLT_DIR}/Math/Vec3.cpp
	${JOLT_DIR}/Math/Vec3.h
	${JOLT_DIR}/Math/Vec3.inl
	${JOLT_DIR}/Math/Vec4.h
	${JOLT_DIR}/Math/Vec4.inl
	${JOLT_DIR}/Math/Vec8.h
	${JOLT_DIR}/Math/Vec8.inl
	${JOLT_DIR}/Math/Vector.h
	${JOLT_DIR}/ObjectStream/SerializableObject.cpp
	${JOLT_DIR}/ObjectStream/SerializableObject.h
	${JOLT_DIR}/Physics/Body/AllowedDOFs.h
	${JOLT_DIR}/Physics/Body/Body.cpp
	${JOLT_DIR}/Physics/Body/Body.h
	${JOLT_DIR}/Physics/Body/Body.inl
	${JOLT_DIR}/Physics/Body/BodyAccess.cpp
	${JOLT_DIR}/Physics/Body/BodyAccess.h
	${JOLT_DIR}/Physics/Body/BodyActivationListener.h
	${JOLT_DIR}/Physics/Body/BodyCreationSettings.cpp
	${JOLT_DIR}/Physics/Body/BodyCreationSettings.h
	${JOLT_DIR}/Physics/Body/BodyFilter.h
	${JOLT_DIR}/Physics/Body/BodyID.h
	${JOLT_DIR}/Physics/Body/BodyInterface.cpp
	${JOLT_DIR}/Physics/Body/BodyInterface.h
	${JOLT_DIR}/Physics/Body/BodyLock.h
	${JOLT_DIR}/Physics/Body/BodyLockInterface.h
	${JOLT_DIR}/Physics/Body/BodyLockMulti.h
	${JOLT_DIR}/Physics/Body/BodyManager.cpp
	${JOLT_DIR}/Physics/Body/BodyManager.h
	${JOLT_DIR}/Physics/Body/BodyPair.h
	${JOLT_DIR}/Physics/Body/BodyType.h
	${JOLT_DIR}/Physics/Body/MassProperties.cpp
	${JOLT_DIR}/Physics/Body/MassProperties.h
	${JOLT_DIR}/Physics/Body/MotionProperties.cpp
	${JOLT_DIR}/Physics/Body/MotionProperties.h
	${JOLT_DIR}/Physics/Body/MotionProperties.inl
	${JOLT_DIR}/Physics/Body/MotionQuality.h
	${JOLT_DIR}/Physics/Body/MotionType.h
	${JOLT_DIR}/Physics/Character/Character.cpp
	${JOLT_DIR}/Physics/Character/Character.h
	${JOLT_DIR}/Physics/Character/CharacterBase.cpp
	${JOLT_DIR}/Physics/Character/CharacterBase.h
	${JOLT_DIR}/Physics/Character/CharacterVirtual.cpp
	${JOLT_DIR}/Physics/Character/CharacterVirtual.h
	${JOLT_DIR}/Physics/Collision/AABoxCast.h
	${JOLT_DIR}/Physics/Collision/ActiveEdgeMode.h
	${JOLT_DIR}/Physics/Collision/ActiveEdges.h
	${JOLT_DIR}/Physics/Collision/BackFaceMode.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhase.cpp
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhase.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseBruteForce.cpp
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseBruteForce.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseLayer.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseQuadTree.cpp
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseQuadTree.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/BroadPhaseQuery.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h
	${JOLT_DIR}/Physics/Collision/BroadPhase/QuadTree.cpp
	${JOLT_DIR}/Physics/Collision/BroadPhase/QuadTree.h
	${JOLT_DIR}/Physics/Collision/CastConvexVsTriangles.cpp
	${JOLT_DIR}/Physics/Collision/CastConvexVsTriangles.h
	${JOLT_DIR}/Physics/Collision/CastSphereVsTriangles.cpp
	${JOLT_DIR}/Physics/Collision/CastSphereVsTriangles.h
	${JOLT_DIR}/Physics/Collision/CastResult.h
	${JOLT_DIR}/Physics/Collision/CollectFacesMode.h
	${JOLT_DIR}/Physics/Collision/CollideConvexVsTriangles.cpp
	${JOLT_DIR}/Physics/Collision/CollideConvexVsTriangles.h
	${JOLT_DIR}/Physics/Collision/CollidePointResult.h
	${JOLT_DIR}/Physics/Collision/CollideShape.h
	${JOLT_DIR}/Physics/Collision/CollideSoftBodyVerticesVsTriangles.h
	${JOLT_DIR}/Physics/Collision/CollideSphereVsTriangles.cpp
	${JOLT_DIR}/Physics/Collision/CollideSphereVsTriangles.h
	${JOLT_DIR}/Physics/Collision/CollisionCollector.h
	${JOLT_DIR}/Physics/Collision/CollisionCollectorImpl.h
	${JOLT_DIR}/Physics/Collision/CollisionDispatch.cpp
	${JOLT_DIR}/Physics/Collision/CollisionDispatch.h
	${JOLT_DIR}/Physics/Collision/CollisionGroup.cpp
	${JOLT_DIR}/Physics/Collision/CollisionGroup.h
	${JOLT_DIR}/Physics/Collision/ContactListener.h
	${JOLT_DIR}/Physics/Collision/EstimateCollisionResponse.cpp
	${JOLT_DIR}/Physics/Collision/EstimateCollisionResponse.h
	${JOLT_DIR}/Physics/Collision/GroupFilter.cpp
	${JOLT_DIR}/Physics/Collision/GroupFilter.h
	${JOLT_DIR}/Physics/Collision/GroupFilterTable.cpp
	${JOLT_DIR}/Physics/Collision/GroupFilterTable.h
	${JOLT_DIR}/Physics/Collision/InternalEdgeRemovingCollector.h
	${JOLT_DIR}/Physics/Collision/ManifoldBetweenTwoFaces.cpp
	${JOLT_DIR}/Physics/Collision/ManifoldBetweenTwoFaces.h
	${JOLT_DIR}/Physics/Collision/NarrowPhaseQuery.cpp
	${JOLT_DIR}/Physics/Collision/NarrowPhaseQuery.h
	${JOLT_DIR}/Physics/Collision/NarrowPhaseStats.cpp
	${JOLT_DIR}/Physics/Collision/NarrowPhaseStats.h
	${JOLT_DIR}/Physics/Collision/ObjectLayer.h
	${JOLT_DIR}/Physics/Collision/ObjectLayerPairFilterMask.h
	${JOLT_DIR}/Physics/Collision/ObjectLayerPairFilterTable.h
	${JOLT_DIR}/Physics/Collision/PhysicsMaterial.cpp
	${JOLT_DIR}/Physics/Collision/PhysicsMaterial.h
	${JOLT_DIR}/Physics/Collision/PhysicsMaterialSimple.cpp
	${JOLT_DIR}/Physics/Collision/PhysicsMaterialSimple.h
	${JOLT_DIR}/Physics/Collision/RayCast.h
	${JOLT_DIR}/Physics/Collision/Shape/BoxShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/BoxShape.h
	${JOLT_DIR}/Physics/Collision/Shape/CapsuleShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/CapsuleShape.h
	${JOLT_DIR}/Physics/Collision/Shape/CompoundShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/CompoundShape.h
	${JOLT_DIR}/Physics/Collision/Shape/CompoundShapeVisitors.h
	${JOLT_DIR}/Physics/Collision/Shape/ConvexHullShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/ConvexHullShape.h
	${JOLT_DIR}/Physics/Collision/Shape/ConvexShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/ConvexShape.h
	${JOLT_DIR}/Physics/Collision/Shape/CylinderShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/CylinderShape.h
	${JOLT_DIR}/Physics/Collision/Shape/DecoratedShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/DecoratedShape.h
	${JOLT_DIR}/Physics/Collision/Shape/GetTrianglesContext.h
	${JOLT_DIR}/Physics/Collision/Shape/HeightFieldShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/HeightFieldShape.h
	${JOLT_DIR}/Physics/Collision/Shape/MeshShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/MeshShape.h
	${JOLT_DIR}/Physics/Collision/Shape/MutableCompoundShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/MutableCompoundShape.h
	${JOLT_DIR}/Physics/Collision/Shape/OffsetCenterOfMassShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/OffsetCenterOfMassShape.h
	${JOLT_DIR}/Physics/Collision/Shape/PolyhedronSubmergedVolumeCalculator.h
	${JOLT_DIR}/Physics/Collision/Shape/RotatedTranslatedShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/RotatedTranslatedShape.h
	${JOLT_DIR}/Physics/Collision/Shape/ScaledShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/ScaledShape.h
	${JOLT_DIR}/Physics/Collision/Shape/ScaleHelpers.h
	${JOLT_DIR}/Physics/Collision/Shape/Shape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/Shape.h
	${JOLT_DIR}/Physics/Collision/Shape/SphereShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/SphereShape.h
	${JOLT_DIR}/Physics/Collision/Shape/StaticCompoundShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/StaticCompoundShape.h
	${JOLT_DIR}/Physics/Collision/Shape/SubShapeID.h
	${JOLT_DIR}/Physics/Collision/Shape/SubShapeIDPair.h
	${JOLT_DIR}/Physics/Collision/Shape/TaperedCapsuleShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/TaperedCapsuleShape.gliffy
	${JOLT_DIR}/Physics/Collision/Shape/TaperedCapsuleShape.h
	${JOLT_DIR}/Physics/Collision/Shape/TriangleShape.cpp
	${JOLT_DIR}/Physics/Collision/Shape/TriangleShape.h
	${JOLT_DIR}/Physics/Collision/ShapeCast.h
	${JOLT_DIR}/Physics/Collision/ShapeFilter.h
	${JOLT_DIR}/Physics/Collision/SortReverseAndStore.h
	${JOLT_DIR}/Physics/Collision/TransformedShape.cpp
	${JOLT_DIR}/Physics/Collision/TransformedShape.h
	${JOLT_DIR}/Physics/Constraints/CalculateSolverSteps.h
	${JOLT_DIR}/Physics/Constraints/ConeConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/ConeConstraint.h
	${JOLT_DIR}/Physics/Constraints/Constraint.cpp
	${JOLT_DIR}/Physics/Constraints/Constraint.h
	${JOLT_DIR}/Physics/Constraints/ConstraintManager.cpp
	${JOLT_DIR}/Physics/Constraints/ConstraintManager.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/AngleConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/AxisConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/DualAxisConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/GearConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/HingeRotationConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/IndependentAxisConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/PointConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/RackAndPinionConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/RotationEulerConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/RotationQuatConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/SpringPart.h
	${JOLT_DIR}/Physics/Constraints/ConstraintPart/SwingTwistConstraintPart.h
	${JOLT_DIR}/Physics/Constraints/ContactConstraintManager.cpp
	${JOLT_DIR}/Physics/Constraints/ContactConstraintManager.h
	${JOLT_DIR}/Physics/Constraints/DistanceConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/DistanceConstraint.h
	${JOLT_DIR}/Physics/Constraints/FixedConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/FixedConstraint.h
	${JOLT_DIR}/Physics/Constraints/GearConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/GearConstraint.h
	${JOLT_DIR}/Physics/Constraints/HingeConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/HingeConstraint.h
	${JOLT_DIR}/Physics/Constraints/MotorSettings.cpp
	${JOLT_DIR}/Physics/Constraints/MotorSettings.h
	${JOLT_DIR}/Physics/Constraints/PathConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/PathConstraint.h
	${JOLT_DIR}/Physics/Constraints/PathConstraintPath.cpp
	${JOLT_DIR}/Physics/Constraints/PathConstraintPath.h
	${JOLT_DIR}/Physics/Constraints/PathConstraintPathHermite.cpp
	${JOLT_DIR}/Physics/Constraints/PathConstraintPathHermite.h
	${JOLT_DIR}/Physics/Constraints/PointConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/PointConstraint.h
	${JOLT_DIR}/Physics/Constraints/PulleyConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/PulleyConstraint.h
	${JOLT_DIR}/Physics/Constraints/RackAndPinionConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/RackAndPinionConstraint.h
	${JOLT_DIR}/Physics/Constraints/SixDOFConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/SixDOFConstraint.h
	${JOLT_DIR}/Physics/Constraints/SliderConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/SliderConstraint.h
	${JOLT_DIR}/Physics/Constraints/SpringSettings.cpp
	${JOLT_DIR}/Physics/Constraints/SpringSettings.h
	${JOLT_DIR}/Physics/Constraints/SwingTwistConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/SwingTwistConstraint.h
	${JOLT_DIR}/Physics/Constraints/TwoBodyConstraint.cpp
	${JOLT_DIR}/Physics/Constraints/TwoBodyConstraint.h
	${JOLT_DIR}/Physics/DeterminismLog.cpp
	${JOLT_DIR}/Physics/DeterminismLog.h
	${JOLT_DIR}/Physics/EActivation.h
	${JOLT_DIR}/Physics/EPhysicsUpdateError.h
	${JOLT_DIR}/Physics/IslandBuilder.cpp
	${JOLT_DIR}/Physics/IslandBuilder.h
	${JOLT_DIR}/Physics/LargeIslandSplitter.cpp
	${JOLT_DIR}/Physics/LargeIslandSplitter.h
	${JOLT_DIR}/Physics/PhysicsLock.cpp
	${JOLT_DIR}/Physics/PhysicsLock.h
	${JOLT_DIR}/Physics/PhysicsScene.cpp
	${JOLT_DIR}/Physics/PhysicsScene.h
	${JOLT_DIR}/Physics/PhysicsSettings.h
	${JOLT_DIR}/Physics/PhysicsStepListener.h
	${JOLT_DIR}/Physics/PhysicsSystem.cpp
	${JOLT_DIR}/Physics/PhysicsSystem.h
	${JOLT_DIR}/Physics/PhysicsUpdateContext.cpp
	${JOLT_DIR}/Physics/PhysicsUpdateContext.h
	${JOLT_DIR}/Physics/Ragdoll/Ragdoll.cpp
	${JOLT_DIR}/Physics/Ragdoll/Ragdoll.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyContactListener.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyCreationSettings.cpp
	${JOLT_DIR}/Physics/SoftBody/SoftBodyCreationSettings.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyManifold.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyMotionProperties.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyMotionProperties.cpp
	${JOLT_DIR}/Physics/SoftBody/SoftBodyShape.cpp
	${JOLT_DIR}/Physics/SoftBody/SoftBodyShape.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodySharedSettings.cpp
	${JOLT_DIR}/Physics/SoftBody/SoftBodySharedSettings.h
	${JOLT_DIR}/Physics/SoftBody/SoftBodyVertex.h
	${JOLT_DIR}/Physics/StateRecorder.h
	${JOLT_DIR}/Physics/StateRecorderImpl.cpp
	${JOLT_DIR}/Physics/StateRecorderImpl.h
	${JOLT_DIR}/Physics/Vehicle/MotorcycleController.cpp
	${JOLT_DIR}/Physics/Vehicle/MotorcycleController.h
	${JOLT_DIR}/Physics/Vehicle/TrackedVehicleController.cpp
	${JOLT_DIR}/Physics/Vehicle/TrackedVehicleController.h
	${JOLT_DIR}/Physics/Vehicle/VehicleAntiRollBar.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleAntiRollBar.h
	${JOLT_DIR}/Physics/Vehicle/VehicleCollisionTester.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleCollisionTester.h
	${JOLT_DIR}/Physics/Vehicle/VehicleConstraint.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleConstraint.h
	${JOLT_DIR}/Physics/Vehicle/VehicleController.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleController.h
	${JOLT_DIR}/Physics/Vehicle/VehicleDifferential.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleDifferential.h
	${JOLT_DIR}/Physics/Vehicle/VehicleEngine.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleEngine.h
	${JOLT_DIR}/Physics/Vehicle/VehicleTrack.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleTrack.h
	${JOLT_DIR}/Physics/Vehicle/VehicleTransmission.cpp
	${JOLT_DIR}/Physics/Vehicle/VehicleTransmission.h
	${JOLT_DIR}/Physics/Vehicle/Wheel.cpp
	${JOLT_DIR}/Physics/Vehicle/Wheel.h
	${JOLT_DIR}/Physics/Vehicle/WheeledVehicleController.cpp
	${JOLT_DIR}/Physics/Vehicle/WheeledVehicleController.h
	${JOLT_DIR}/RegisterTypes.cpp
	${JOLT_DIR}/RegisterTypes.h
	${JOLT_DIR}/Renderer/DebugRenderer.cpp
	${JOLT_DIR}/Renderer/DebugRenderer.h
	${JOLT_DIR}/Renderer/DebugRendererPlayback.cpp
	${JOLT_DIR}/Renderer/DebugRendererPlayback.h
	${JOLT_DIR}/Renderer/DebugRendererRecorder.cpp
	${JOLT_DIR}/Renderer/DebugRendererRecorder.h
	${JOLT_DIR}/Renderer/DebugRendererSimple.cpp
	${JOLT_DIR}/Renderer/DebugRendererSimple.h
	${JOLT_DIR}/Skeleton/SkeletalAnimation.cpp
	${JOLT_DIR}/Skeleton/SkeletalAnimation.h
	${JOLT_DIR}/Skeleton/Skeleton.cpp
	${JOLT_DIR}/Skeleton/Skeleton.h
	${JOLT_DIR}/Skeleton/SkeletonMapper.cpp
	${JOLT_DIR}/Skeleton/SkeletonMapper.h
	${JOLT_DIR}/Skeleton/SkeletonPose.cpp
	${JOLT_DIR}/Skeleton/SkeletonPose.h
	${JOLT_DIR}/TriangleGrouper/TriangleGrouper.h
	${JOLT_DIR}/TriangleGrouper/TriangleGrouperClosestCentroid.cpp
	${JOLT_DIR}/TriangleGrouper/TriangleGrouperClosestCentroid.h
	${JOLT_DIR}/TriangleGrouper/TriangleGrouperMorton.cpp
	${JOLT_DIR}/TriangleGrouper/TriangleGrouperMorton.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitter.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitter.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterBinning.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterBinning.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterFixedLeafSize.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterFixedLeafSize.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterLongestAxis.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterLongestAxis.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterMean.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterMean.h
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterMorton.cpp
	${JOLT_DIR}/TriangleSplitter/TriangleSplitterMorton.h
)

if (ENABLE_OBJECT_STREAM)
	set(JOLT_PHYSICS_SRC_FILES
		${JOLT_PHYSICS_SRC_FILES}
		${JOLT_DIR}/ObjectStream/GetPrimitiveTypeOfType.h
		${JOLT_DIR}/ObjectStream/ObjectStream.cpp
		${JOLT_DIR}/ObjectStream/ObjectStream.h
		${JOLT_DIR}/ObjectStream/ObjectStreamBinaryIn.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamBinaryIn.h
		${JOLT_DIR}/ObjectStream/ObjectStreamBinaryOut.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamBinaryOut.h
		${JOLT_DIR}/ObjectStream/ObjectStreamIn.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamIn.h
		${JOLT_DIR}/ObjectStream/ObjectStreamOut.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamOut.h
		${JOLT_DIR}/ObjectStream/ObjectStreamTextIn.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamTextIn.h
		${JOLT_DIR}/ObjectStream/ObjectStreamTextOut.cpp
		${JOLT_DIR}/ObjectStream/ObjectStreamTextOut.h
		${JOLT_DIR}/ObjectStream/ObjectStreamTypes.h
		${JOLT_DIR}/ObjectStream/SerializableAttribute.h
		${JOLT_DIR}/ObjectStream/SerializableAttributeEnum.h
		${JOLT_DIR}/ObjectStream/SerializableAttributeTyped.h
		${JOLT_DIR}/ObjectStream/TypeDeclarations.cpp
		${JOLT_DIR}/ObjectStream/TypeDeclarations.h
	)
endif()

# Create Jolt lib
add_library(jolt ${JOLT_PHYSICS_SRC_FILES})

target_precompile_headers(jolt PRIVATE ${JOLT_DIR}/Jolt.h)

# Set the debug/non-debug build flags
target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Debug>:_DEBUG>")
target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Release,Distribution,ReleaseASAN,ReleaseUBSAN,ReleaseCoverage>:NDEBUG>")

# ASAN should use the default allocators
target_compile_definitions(jolt PUBLIC "$<$<CONFIG:ReleaseASAN>:JPH_DISABLE_TEMP_ALLOCATOR;JPH_DISABLE_CUSTOM_ALLOCATOR>")

# Setting floating point exceptions
if (FLOATING_POINT_EXCEPTIONS_ENABLED AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Debug,Release>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>")
endif()

# Setting the disable custom allocator flag
if (DISABLE_CUSTOM_ALLOCATOR)
	target_compile_definitions(jolt PUBLIC JPH_DISABLE_CUSTOM_ALLOCATOR)
endif()

# Setting enable asserts flag
if (USE_ASSERTS)
	target_compile_definitions(jolt PUBLIC JPH_ENABLE_ASSERTS)
endif()

# Setting double precision flag
if (DOUBLE_PRECISION)
	target_compile_definitions(jolt PUBLIC JPH_DOUBLE_PRECISION)
endif()

# Setting to attempt cross platform determinism
if (CROSS_PLATFORM_DETERMINISTIC)
	target_compile_definitions(jolt PUBLIC JPH_CROSS_PLATFORM_DETERMINISTIC)
endif()

# Setting to determine number of bits in ObjectLayer
if (OBJECT_LAYER_BITS)
	target_compile_definitions(jolt PUBLIC JPH_OBJECT_LAYER_BITS=${OBJECT_LAYER_BITS})
endif()

if (USE_STD_VECTOR)
	target_compile_definitions(jolt PUBLIC JPH_USE_STD_VECTOR)
endif()

# Setting to periodically trace broadphase stats to help determine if the broadphase layer configuration is optimal
if (TRACK_BROADPHASE_STATS)
	target_compile_definitions(jolt PUBLIC JPH_TRACK_BROADPHASE_STATS)
endif()

# Setting to periodically trace narrowphase stats to help determine which collision queries could be optimized
if (TRACK_NARROWPHASE_STATS)
	target_compile_definitions(jolt PUBLIC JPH_TRACK_NARROWPHASE_STATS)
endif()

# Enable the debug renderer
if (DEBUG_RENDERER_IN_DISTRIBUTION)
	target_compile_definitions(jolt PUBLIC "JPH_DEBUG_RENDERER")
elseif (DEBUG_RENDERER_IN_DEBUG_AND_RELEASE)
	target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Debug,Release,ReleaseASAN,ReleaseUBSAN>:JPH_DEBUG_RENDERER>")
endif()

# Enable the profiler
if (PROFILER_IN_DISTRIBUTION)
	target_compile_definitions(jolt PUBLIC "JPH_PROFILE_ENABLED")
elseif (PROFILER_IN_DEBUG_AND_RELEASE)
	target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Debug,Release,ReleaseASAN,ReleaseUBSAN>:JPH_PROFILE_ENABLED>")
endif()

# Compile the ObjectStream class and RTTI attribute information
if (ENABLE_OBJECT_STREAM)
	target_compile_definitions(jolt PUBLIC JPH_OBJECT_STREAM)
endif()

# Emit the instruction set definitions to ensure that child projects use the same settings even if they override the used instruction sets (a mismatch causes link errors)
function(EMIT_X86_INSTRUCTION_SET_DEFINITIONS)
	if (USE_AVX512)
		target_compile_definitions(jolt PUBLIC JPH_USE_AVX512)
	endif()
	if (USE_AVX2)
		target_compile_definitions(jolt PUBLIC JPH_USE_AVX2)
	endif()
	if (USE_AVX)
		target_compile_definitions(jolt PUBLIC JPH_USE_AVX)
	endif()
	if (USE_SSE4_1)
		target_compile_definitions(jolt PUBLIC JPH_USE_SSE4_1)
	endif()
	if (USE_SSE4_2)
		target_compile_definitions(jolt PUBLIC JPH_USE_SSE4_2)
	endif()
	if (USE_LZCNT)
		target_compile_definitions(jolt PUBLIC JPH_USE_LZCNT)
	endif()
	if (USE_TZCNT)
		target_compile_definitions(jolt PUBLIC JPH_USE_TZCNT)
	endif()
	if (USE_F16C)
		target_compile_definitions(jolt PUBLIC JPH_USE_F16C)
	endif()
	if (USE_FMADD AND NOT CROSS_PLATFORM_DETERMINISTIC)
		target_compile_definitions(jolt PUBLIC JPH_USE_FMADD)
	endif()
endfunction()

# Add the compiler commandline flags to select the right instruction sets
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86" OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
		if (USE_AVX512)
			target_compile_options(jolt PUBLIC /arch:AVX512)
		elseif (USE_AVX2)
			target_compile_options(jolt PUBLIC /arch:AVX2)
		elseif (USE_AVX)
			target_compile_options(jolt PUBLIC /arch:AVX)
		endif()
		EMIT_X86_INSTRUCTION_SET_DEFINITIONS()
	endif()
else()
	if (XCODE)
		# XCode builds for multiple architectures, we can't set global flags
	elseif (CROSS_COMPILE_ARM OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
		# ARM64 uses no special commandline flags
	elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
		# x64
		if (USE_AVX512)
			target_compile_options(jolt PUBLIC -mavx512f -mavx512vl -mavx512dq -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
		elseif (USE_AVX2)
			target_compile_options(jolt PUBLIC -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
		elseif (USE_AVX)
			target_compile_options(jolt PUBLIC -mavx -mpopcnt)
		elseif (USE_SSE4_2)
			target_compile_options(jolt PUBLIC -msse4.2 -mpopcnt)
		elseif (USE_SSE4_1)
			target_compile_options(jolt PUBLIC -msse4.1)
		else()
			target_compile_options(jolt PUBLIC -msse2)
		endif()
		if (USE_LZCNT)
			target_compile_options(jolt PUBLIC -mlzcnt)
		endif()
		if (USE_TZCNT)
			target_compile_options(jolt PUBLIC -mbmi)
		endif()
		if (USE_F16C)
			target_compile_options(jolt PUBLIC -mf16c)
		endif()
		if (USE_FMADD AND NOT CROSS_PLATFORM_DETERMINISTIC)
			target_compile_options(jolt PUBLIC -mfma)
		endif()

		# On 32-bit builds we need to default to using SSE instructions, the x87 FPU instructions have higher intermediate precision
		# which will cause problems in the collision detection code (the effect is similar to leaving FMA on, search for
		# JPH_PRECISE_MATH_ON for the locations where this is a problem).
		if (NOT MSVC)
			target_compile_options(jolt PUBLIC -mfpmath=sse)
		endif()

		EMIT_X86_INSTRUCTION_SET_DEFINITIONS()
	endif()
endif()

if(MAX_INSTALL)
	install(
		TARGETS jolt
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
	install(DIRECTORY ${JOLT_DIR} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

# Put in a "max" folder in Visual Studio
set_target_properties(jolt PROPERTIES FOLDER "max/3rdparty")

target_include_directories(jolt PRIVATE ${JOLT_DIR}/../)
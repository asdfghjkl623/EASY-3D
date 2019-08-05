cmake_minimum_required(VERSION 3.1)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME})


set(opcode_HEADERS
    OPC_AABBCollider.h
    OPC_AABBTree.h
    OPC_BaseModel.h
    OPC_BoxBoxOverlap.h
    OPC_BoxPruning.h
    OPC_Collider.h
    OPC_Common.h
    OPC_HybridModel.h
    OPC_IceHook.h
    OPC_LSSAABBOverlap.h
    OPC_LSSCollider.h
    OPC_LSSTriOverlap.h
    OPC_MeshInterface.h
    OPC_Model.h
    OPC_OBBCollider.h
    OPC_OptimizedTree.h
    OPC_Picking.h
    OPC_PlanesAABBOverlap.h
    OPC_PlanesCollider.h
    OPC_PlanesTriOverlap.h
    OPC_RayAABBOverlap.h
    OPC_RayCollider.h
    OPC_RayTriOverlap.h
    OPC_Settings.h
    OPC_SphereAABBOverlap.h
    OPC_SphereCollider.h
    OPC_SphereTriOverlap.h
    OPC_SweepAndPrune.h
    OPC_TreeBuilders.h
    OPC_TreeCollider.h
    OPC_TriBoxOverlap.h
    OPC_TriTriOverlap.h
    OPC_VolumeCollider.h
    Opcode.h
    Ice/IceRandom.h
    Ice/IceIndexedTriangle.h
    Ice/IceAABB.h
    Ice/IceAxes.h
    Ice/IceBoundingSphere.h
    Ice/IceContainer.h
    Ice/IceFPU.h
    Ice/IceHPoint.h
    Ice/IceLSS.h
    Ice/IceMatrix3x3.h
    Ice/IceMatrix4x4.h
    Ice/IceMemoryMacros.h
    Ice/IceOBB.h
    Ice/IcePairs.h
    Ice/IcePlane.h
    Ice/IcePoint.h
    Ice/IcePreprocessor.h
    Ice/IceRay.h
    Ice/IceRevisitedRadix.h
    Ice/IceSegment.h
    Ice/IceTriangle.h
    Ice/IceTriList.h
    Ice/IceTypes.h
    Ice/IceUtils.h
    )

set(opcode_SOURCES
    OPC_AABBCollider.cpp
    OPC_AABBTree.cpp
    OPC_BaseModel.cpp
    OPC_BoxPruning.cpp
    OPC_Collider.cpp
    OPC_Common.cpp
    OPC_HybridModel.cpp
    OPC_LSSCollider.cpp
    OPC_MeshInterface.cpp
    OPC_Model.cpp
    OPC_OBBCollider.cpp
    OPC_OptimizedTree.cpp
    OPC_Picking.cpp
    OPC_PlanesCollider.cpp
    OPC_RayCollider.cpp
    OPC_SphereCollider.cpp
    OPC_SweepAndPrune.cpp
    OPC_TreeBuilders.cpp
    OPC_TreeCollider.cpp
    OPC_VolumeCollider.cpp
    Opcode.cpp
    Ice/IceMatrix3x3.cpp
    Ice/IceRay.cpp
    Ice/IceRevisitedRadix.cpp
    Ice/IceSegment.cpp
    Ice/IceTriangle.cpp
    Ice/IceUtils.cpp
    Ice/IceAABB.cpp
    Ice/IceContainer.cpp
    Ice/IceHPoint.cpp
    Ice/IceIndexedTriangle.cpp
    Ice/IceMatrix4x4.cpp
    Ice/IceOBB.cpp
    Ice/IcePlane.cpp
    Ice/IcePoint.cpp
    Ice/IceRandom.cpp
    )


add_library(${PROJECT_NAME} STATIC ${opcode_SOURCES} ${opcode_HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "3rd_party")

set(OPCODE_INCLUDE_DIR   ${CMAKE_CURRENT_LIST_DIR}/)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPCODE_INCLUDE_DIR})

if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_DEPRECATE
        )
endif()

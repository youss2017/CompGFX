# Install script for directory: D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Assimp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibassimp5.2.0-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/lib/Debug/assimp-vc143-mtd.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/lib/Release/assimp-vc143-mt.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/lib/MinSizeRel/assimp-vc143-mt.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/lib/RelWithDebInfo/assimp-vc143-mt.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibassimp5.2.0x" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/bin/Debug/assimp-vc143-mtd.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/bin/Release/assimp-vc143-mt.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/bin/MinSizeRel/assimp-vc143-mt.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/bin/RelWithDebInfo/assimp-vc143-mt.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/anim.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/aabb.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ai_assert.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/camera.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/color4.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/color4.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/code/../include/assimp/config.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ColladaMetaData.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/commonMetaData.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/defs.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/cfileio.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/light.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/material.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/material.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/matrix3x3.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/matrix3x3.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/matrix4x4.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/matrix4x4.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/mesh.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ObjMaterial.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/pbrmaterial.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/GltfMaterial.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/postprocess.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/quaternion.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/quaternion.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/scene.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/metadata.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/texture.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/types.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/vector2.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/vector2.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/vector3.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/vector3.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/version.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/cimport.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/importerdesc.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Importer.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/DefaultLogger.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ProgressHandler.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/IOStream.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/IOSystem.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Logger.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/LogStream.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/NullLogger.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/cexport.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Exporter.hpp"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/DefaultIOStream.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/DefaultIOSystem.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ZipArchiveIOSystem.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SceneCombiner.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/fast_atof.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/qnan.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/BaseImporter.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Hash.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/MemoryIOWrapper.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ParsingUtils.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/StreamReader.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/StreamWriter.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/StringComparison.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/StringUtils.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SGSpatialSort.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/GenericProperty.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SpatialSort.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SkeletonMeshBuilder.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SmallVector.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SmoothingGroups.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/SmoothingGroups.inl"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/StandardShapes.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/RemoveComments.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Subdivision.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Vertex.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/LineSplitter.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/TinyFormatter.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Profiler.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/LogAux.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Bitmap.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/XMLTools.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/IOStreamBuffer.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/CreateAnimMesh.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/XmlParser.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/BlobIOSystem.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/MathFunctions.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Exceptional.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/ByteSwapper.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Base64.hpp"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Compiler/pushpack1.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Compiler/poppack1.h"
    "D:/GFX Apis/ExoabEngine/ExoabEngine/external/assimp/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/code/Debug/assimp-vc143-mtd.pdb")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "D:/GFX Apis/ExoabEngine/ExoabEngine/external/cmake_files/assimp/code/RelWithDebInfo/assimp-vc143-mt.pdb")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()


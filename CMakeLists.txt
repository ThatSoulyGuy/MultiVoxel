cmake_minimum_required(VERSION 4.0)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
      CACHE STRING "" FORCE)
endif()

project(MultiVoxel VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD  20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_SOURCE_DIR}/MultiVoxel/Source/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/MultiVoxel/Header/*.hpp")

add_executable(MultiVoxel ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(MultiVoxel PUBLIC "${CMAKE_SOURCE_DIR}/Library/Header/")

target_include_directories(MultiVoxel PUBLIC "${CMAKE_SOURCE_DIR}/Library/Header/")

if (APPLE)
    target_link_libraries(MultiVoxel PRIVATE "${CMAKE_SOURCE_DIR}/Library/Binary/MacOS/libGameNetworkingSockets_s.a")
elseif (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(MultiVoxel PRIVATE "${CMAKE_SOURCE_DIR}/Library/Binary/Windows/Debug/GameNetworkingSockets_s.lib")
elseif (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(MultiVoxel PRIVATE "${CMAKE_SOURCE_DIR}/Library/Binary/Windows/Release/GameNetworkingSockets_s.lib")
else()
    message("No suitable platform found")
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)

FetchContent_MakeAvailable(glfw)

if(LINUX)
  find_package(OpenGL REQUIRED)
  target_link_libraries(MultiVoxel PRIVATE glfw OpenGL::GL)
  add_definitions(-D_GLFW_X11)
elseif(WIN32)
  target_link_libraries(MultiVoxel PRIVATE glfw opengl32)
endif()

FetchContent_Declare(
    FreeImage
    GIT_REPOSITORY https://github.com/danoli3/FreeImage.git
    GIT_TAG master
)

FetchContent_MakeAvailable(FreeImage)

target_include_directories(MultiVoxel PRIVATE "${FreeImage_SOURCE_DIR}/Source")

target_compile_definitions(MultiVoxel PRIVATE FREEIMAGE_LIB)

target_link_libraries(MultiVoxel PRIVATE FreeImage)

set(JUST_INSTALL_CEREAL ON CACHE BOOL "Skip building tests/examples and dependency checks." FORCE)

FetchContent_Declare(
    Cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Cereal)

target_link_libraries(MultiVoxel PRIVATE cereal)

set(BUILD_BULLET3 ON CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3_DEMOS OFF CACHE BOOL "" FORCE) 
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    Bullet
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Bullet)

target_link_libraries(MultiVoxel PRIVATE BulletDynamics BulletCollision LinearMath)

target_include_directories(MultiVoxel PRIVATE "${CMAKE_BINARY_DIR}/_deps/bullet-src/src")

message(STATUS "Bullet_SOURCE_DIR = ${Bullet_SOURCE_DIR}")
message(STATUS "Bullet_BINARY_DIR = ${Bullet_BINARY_DIR}")

set(SOURCE_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(BUILD_ASSETS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")

add_custom_command(
    TARGET MultiVoxel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_ASSETS_DIR} ${BUILD_ASSETS_DIR}
    COMMENT "Copying Assets directory to build folder..."
)

if(APPLE)
  message(STATUS "Building on macOS: Including necessary frameworks and SDKs")

  target_link_libraries(MultiVoxel PRIVATE
  glfw
  "-framework Cocoa"
  "-framework QuartzCore"
  "-framework OpenGL"
  "-framework IOKit"
  "-framework CoreVideo"
  "-framework CoreFoundation"
  "-framework Metal"
  "-framework MetalKit"
  "-framework AVFoundation"
  )

  target_compile_options(MultiVoxel PRIVATE
    -Wno-deprecated-declarations
    -mmacosx-version-min=13.7.4
  )

  include_directories(
    /System/Library/Frameworks
    /Library/Frameworks
  )
endif()
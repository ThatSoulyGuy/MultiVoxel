cmake_minimum_required(VERSION 3.2...4.0)

project(MultiVoxel VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC "${CMAKE_SOURCE_DIR}/MultiVoxel/Source/*.cpp")
file(GLOB_RECURSE INC "${CMAKE_SOURCE_DIR}/MultiVoxel/Header/*.hpp")

add_executable(MultiVoxel ${SRC} ${INC})
target_include_directories(MultiVoxel PUBLIC
  "${CMAKE_SOURCE_DIR}/MultiVoxel/Header"
)

if (WIN32 OR WIN64)
  target_include_directories(MultiVoxel PUBLIC "${CMAKE_SOURCE_DIR}/Library/WindowsOnly/Header")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(MultiVoxel PUBLIC "${CMAKE_SOURCE_DIR}/Library/WindowsOnly/Debug/GameNetworkingSockets.lib")
  else ()
    target_link_libraries(MultiVoxel PUBLIC "${CMAKE_SOURCE_DIR}/Library/WindowsOnly/Release/GameNetworkingSockets.lib")
  endif ()
endif ()

include(FetchContent)

set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        lts_2025_01_27
)
FetchContent_MakeAvailable(absl)

set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        29.x
)
FetchContent_MakeAvailable(protobuf)

set(CMAKE_PREFIX_PATH "${protobuf_BINARY_DIR};${CMAKE_PREFIX_PATH}")
set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)

if(APPLE)
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/openssl@3")
endif()

find_package(OpenSSL REQUIRED)

if (NOT WIN32 AND NOT WIN64)
  set(ENABLE_EXAMPLES   OFF CACHE BOOL "" FORCE)
  set(ENABLE_GAMESERVER OFF CACHE BOOL "" FORCE)
  set(ENABLE_CHRONOLOG  OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(STEAMNETWORKINGSOCKETS_STATIC_LINK  ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    gns
    GIT_REPOSITORY https://github.com/ValveSoftware/GameNetworkingSockets.git
    GIT_TAG        master
  )
  FetchContent_MakeAvailable(gns)

  target_include_directories(MultiVoxel PRIVATE "${gns_SOURCE_DIR}/include")
endif ()

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        latest
)
FetchContent_MakeAvailable(glfw)

if(LINUX)
  find_package(OpenGL REQUIRED)
  target_link_libraries(MultiVoxel PRIVATE glfw OpenGL::GL)
  add_definitions(-D_GLFW_X11)
elseif(WIN32)
  target_link_libraries(MultiVoxel PRIVATE glfw opengl32)
elseif(APPLE)
  target_link_libraries(MultiVoxel PRIVATE
    glfw
    "-framework Cocoa" "-framework QuartzCore" "-framework OpenGL"
    "-framework IOKit" "-framework CoreVideo" "-framework CoreFoundation"
    "-framework Metal" "-framework MetalKit" "-framework AVFoundation"
  )
endif()

FetchContent_Declare(FreeImage
  GIT_REPOSITORY https://github.com/danoli3/FreeImage.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(FreeImage)
target_include_directories(MultiVoxel PRIVATE "${FreeImage_SOURCE_DIR}/Source" "${CMAKE_SOURCE_DIR}/Library/Header" )
target_compile_definitions(MultiVoxel PRIVATE FREEIMAGE_LIB)
target_link_libraries(MultiVoxel PRIVATE FreeImage)

set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)
FetchContent_Declare(cereal
  GIT_REPOSITORY https://github.com/USCiLab/cereal.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(cereal)
target_link_libraries(MultiVoxel PRIVATE cereal)

set(BUILD_BULLET3 ON  CACHE BOOL "" FORCE)
set(BUILD_EXTRAS  OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Bullet
  GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(Bullet)
target_link_libraries(MultiVoxel PRIVATE BulletDynamics BulletCollision LinearMath)
target_include_directories(MultiVoxel PRIVATE "${Bullet_SOURCE_DIR}/src")

if (NOT WIN32 AND NOT WIN64)
  target_link_libraries(MultiVoxel PRIVATE
    GameNetworkingSockets::GameNetworkingSockets
    protobuf::libprotobuf
    OpenSSL::SSL OpenSSL::Crypto
    absl::strings
  )
else ()
  target_link_libraries(MultiVoxel PRIVATE
          protobuf::libprotobuf
          OpenSSL::SSL OpenSSL::Crypto
          absl::strings
  )
endif ()

add_custom_command(TARGET MultiVoxel POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/Assets" "${CMAKE_CURRENT_BINARY_DIR}/Assets"
  COMMENT "Copying Assets…"
)
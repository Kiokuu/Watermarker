# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Watermark PRIVATE -O3 -Os -flto)
    target_compile_options(Binder PRIVATE -O3 -Os -flto)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(Watermark PRIVATE /O2 /Os /GL)
    target_compile_options(Binder PRIVATE -O3 -Os -flto)
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project ("Watermarker")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")
include(cmake/FindDiaSDK.cmake)


# Include sub-projects.
add_subdirectory("Binder")

add_subdirectory("DX11TestApplication")

add_subdirectory("Watermark")

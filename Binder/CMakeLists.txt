# CMakeList.txt : CMake project for Binder, include source and define
# project specific logic here.
#



include(FetchContent)

FetchContent_Declare(zasm
	GIT_REPOSITORY https://github.com/zyantific/zasm/
	GIT_TAG master
)

FetchContent_MakeAvailable(zasm)

# Add source to this project's executable.
add_executable (Binder "Binder.cpp" "Binder.h" "main.cpp" "ExecutableFile.h" "ExecutableFile.cpp")

target_include_directories(Binder PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(Binder PRIVATE zasm)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Binder PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

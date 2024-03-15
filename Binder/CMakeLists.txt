# CMakeList.txt : CMake project for Binder, include source and define
# project specific logic here.
#
include(FetchContent)

FetchContent_Declare(zasm
	GIT_REPOSITORY https://github.com/zyantific/zasm/
	GIT_TAG master
)

FetchContent_MakeAvailable(zasm)

FetchContent_Declare(imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui/
	GIT_TAG docking
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

FetchContent_MakeAvailable(imgui)

# Set a variable to specify the location of imgui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

FetchContent_Declare(imgui_filedialog
	GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog/
	GIT_TAG master
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui_filedialog
)

FetchContent_MakeAvailable(imgui_filedialog)

# Add source to this project's executable.
add_executable (Binder "Binder.cpp" "Binder.h" "main.cpp" "ExecutableFile.h" "ExecutableFile.cpp" "ImageSection.h" "ImageSection.cpp" "ImportedModule.h" "ExportedModule.h" "Utils.h" "PDBFile.h" "PDBFile.cpp" "Disassembly/Function.h" "Disassembly/Label.h" "Disassembly/Data.h"  "Disassembly/Instruction.h" "Disassembly/Disassembler.h" "Disassembly/Disassembler.cpp" "Disassembly/AsmItem.h" "GUI/ImguiHandler.cpp" "GUI/ImguiHandler.h" "imgui/imgui.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_demo.cpp" "imgui/imgui_tables.cpp" "imgui/imgui_widgets.cpp" "imgui/backends/imgui_impl_dx11.cpp" "imgui/backends/imgui_impl_win32.cpp" "imgui_filedialog/ImGuiFileDialog.cpp" "GUI/ImguiDraw.cpp" "GUI/ImguiDraw.h")


target_include_directories(Binder PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include PUBLIC ${DIASDK_INCLUDE_DIRS} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui ${CMAKE_CURRENT_LIST_DIR}/imgui/backends ${CMAKE_CURRENT_LIST_DIR}/imgui_filedialog)
target_link_libraries(Binder PRIVATE zasm ${DIASDK_LIBRARIES} dbghelp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Binder PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

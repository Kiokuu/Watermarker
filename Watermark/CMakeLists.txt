include(FetchContent)

FetchContent_Declare(minhook
	GIT_REPOSITORY https://github.com/TsudaKageyu/minhook
	GIT_TAG master
)

FetchContent_MakeAvailable(minhook)


add_library(Watermark SHARED "Watermark.cpp" "Watermark.h" "D3D11RenderHook.h" "D3D11RenderHook.cpp" "D3D11WatermarkShader.h" "Vertex.h" "D3D11StateBlock.h" "D3D11StateBlock.cpp")

target_include_directories(Watermark PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(Watermark PRIVATE minhook)
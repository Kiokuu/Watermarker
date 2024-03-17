#pragma once
#include <DirectXMath.h>

/**
 * @brief Represents a vertex with position and texture coordinates.
 */
struct Vertex
{
	DirectX::XMFLOAT2 pos; /**< Position of the vertex. */
	DirectX::XMFLOAT2 uv; /**< Texture coordinates of the vertex. */
};

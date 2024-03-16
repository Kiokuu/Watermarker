#pragma once

/**
 * @brief Stores the vertex shader source code (HLSL)
 */
const char* vertexShaderSource = R"(
	Texture2D _texture;
	SamplerState _sampler;
	
	float4 pos_to_screenspace(float2 position_local) {
		return float4( (position_local.x * 2) - 1, (position_local.y * -2) + 1, 0, 1);
	}

	struct vs_in {
	    float2 position_local : POSITION;
		float2 uv : TEXCOORD0;
	};

	struct vs_out {
	    float4 position_clip : SV_POSITION;
		float2 uv : TEXCOORD0;
	};

	vs_out vs_main(vs_in input) {
	  vs_out output = (vs_out)0;
	  output.position_clip = pos_to_screenspace(input.position_local);
	  output.uv = input.uv;
	  return output;
	}

	float4 ps_main(vs_out input) : SV_TARGET {
	  return _texture.Sample(_sampler, input.uv);
	}
)";
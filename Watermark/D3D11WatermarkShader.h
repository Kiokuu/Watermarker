#pragma once

const char* vertexShaderSource = R"(
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
	  output.position_clip = float4(input.position_local, 0, 1);
	  output.uv = input.uv;
	  return output;
	}

	float4 ps_main(vs_out input) : SV_TARGET {
	  return float4(input.uv, 0, 0.5);
	}
)";
#include "common.hlsli"

struct RootConsts
{
    float3 color;
};

//ConstantBuffer<RootConsts> rootConsts : register(b0);

struct VsOutput
{
    float4 position : SV_Position;
};

float2 fullscreenTrianglePos(uint _vid)
{
    return float2((_vid << 1) & 2, _vid & 2) * 2.0f - 1.0f;
    //return float2((_vid << 1) & 2, _vid & 2) * 0.75f - 0.75f;
}

// trianglestrip(14)
// see https://twitter.com/turanszkij/status/1141638406956617730
float3 cubePos(uint _vid)
{
    uint b = 1U << _vid;
    return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}


VsOutput vs_main(uint _vid : SV_VertexID)
{
    VsOutput Out;
    float3 pos = float3(fullscreenTrianglePos(_vid), 0.99f);
    Out.position = float4(pos, 1.0f);
    return Out;
}

struct PsOutput
{
    float4 color : SV_Target;
    //float depth : SV_Depth;
};


PsOutput ps_main(VsOutput _input)
{
    PsOutput Out;
    //Out.depth = _input.position.z;
    //Out.color = float4(rootConsts.color, 1.0f);
	Out.color = float4(1.0f, 0.f, 0.f, 1.0f);
    return Out;
}
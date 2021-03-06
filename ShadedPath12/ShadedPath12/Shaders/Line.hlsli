// shared data for Line VS and PS

#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "CBV(b0, space = 0), " \
              "DescriptorTable( CBV(b1)), "


// root signature: only has CBV that will hold the WVP matrix
#define LinesRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "CBV(b0, space = 0), " \
              "RootConstants(num32BitConstants = 1, b3)" // avoid GRFXTool exception, use different number for each root signature

struct LinesConstantBuffer {
	float4x4 wvp;
};

ConstantBuffer<LinesConstantBuffer> cbv: register(b0);


struct VSInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

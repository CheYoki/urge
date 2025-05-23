// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/pipeline/builtin_hlsl.h"

namespace renderer {

///
// type:
//   basis shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
///

const std::string kHLSL_BaseRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.ProjMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_BaseRender_PixelShader = R"(

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  frag.a *= PSIn.Color.a;
  PSOut.Color = frag;
}

)";

///
// type:
//   color shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
///

const std::string kHLSL_ColorRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.ProjMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_ColorRender_PixelShader = R"(

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  PSOut.Color = PSIn.Color;
}

)";

///
// type:
//   flat shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float4, float4 }
///

const std::string kHLSL_FlatRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.ProjMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_FlatRender_PixelShader = R"(

struct FlatParams {
  float4 Color;
  float4 Tone;
};

cbuffer FlatUniformConstants {
  FlatParams u_Effect;
};

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float3 lumaF = float3(0.299, 0.587, 0.114);

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);

  // Tone
  float luma = dot(frag.rgb, lumaF);
  frag.rgb = lerp(frag.rgb, float3(luma, luma, luma), u_Effect.Tone.w);
  frag.rgb += u_Effect.Tone.rgb;

  // Opacity
  frag.a *= PSIn.Color.a;

  // Color
  frag.rgb = lerp(frag.rgb, u_Effect.Color.rgb, u_Effect.Color.a);

  PSOut.Color = frag;
}

)";

///
// type:
//   sprite shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//   < { float4, float2 } >
//   < { float2, float2, float2, float, float4, float4, float, float, float } >
///

const std::string kHLSL_SpriteRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct SpriteVertex {
  float4 Pos;
  float4 UV;
};

struct SpriteParams {
  float4 Color;
  float4 Tone;
  float4 Position;
  float4 Origin;
  float4 Scale;
  float4 Rotation;
  float4 Opacity;
  float4 BushDepthAndOpacity;
};

StructuredBuffer<SpriteVertex> u_Vertices;
StructuredBuffer<SpriteParams> u_Params;

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
  uint VertexIdx : SV_VertexID;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : NORMAL0;
  float4 Tone : NORMAL1;
  float Opacity : NORMAL2;
  float BushDepth : NORMAL3;
  float BushOpacity : NORMAL4;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  uint instance_index = VSIn.VertexIdx / 4;
  uint vertex_index = VSIn.VertexIdx % 4;
  SpriteVertex vert = u_Vertices[vertex_index + 4 * instance_index];
  SpriteParams effect = u_Params[instance_index];

  float sine = sin(effect.Rotation.x);
  float cosine = cos(effect.Rotation.x);

  float sxs = effect.Scale.x * sine;
  float sxc = effect.Scale.x * cosine;
  float sys = effect.Scale.y * sine;
  float syc = effect.Scale.y * cosine;
  
  float tx = -effect.Origin.x * sxc - effect.Origin.y * sys + effect.Position.x;
  float ty = effect.Origin.x * sxs - effect.Origin.y * syc + effect.Position.y;

  float4 transPos = float4(
    vert.Pos.x * sxc + vert.Pos.y * sys + tx,
    -vert.Pos.x * sxs + vert.Pos.y * syc + ty,
    vert.Pos.z,
    vert.Pos.w
  );

  PSIn.Pos = mul(u_Transform.ProjMat, transPos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = vert.UV.xy;
  PSIn.Color = effect.Color;
  PSIn.Tone = effect.Tone;
  PSIn.Opacity = effect.Opacity.x;
  PSIn.BushDepth = effect.BushDepthAndOpacity.x;
  PSIn.BushOpacity = effect.BushDepthAndOpacity.y;
}

)";

const std::string kHLSL_SpriteRender_PixelShader = R"(

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : NORMAL0;
  float4 Tone : NORMAL1;
  float Opacity : NORMAL2;
  float BushDepth : NORMAL3;
  float BushOpacity : NORMAL4;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

static const float3 lumaF = float3(0.299, 0.587, 0.114);

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  
  // Tone
  float luma = dot(frag.rgb, lumaF);
  frag.rgb = lerp(frag.rgb, float3(luma, luma, luma), PSIn.Tone.w);
  frag.rgb += PSIn.Tone.rgb;
    
  // Color
  frag.a *= PSIn.Opacity;
  frag.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
    
  // Bush
  float currentPos = PSIn.UV.y;
  float underBush = (currentPos > PSIn.BushDepth) ? 0.0 : 1.0;
  frag.a *= clamp(PSIn.BushOpacity + underBush, 0.0, 1.0);

  PSOut.Color = frag;
}

)";

///
// type:
//   alpha transition shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D }
///

const std::string kHLSL_AlphaTransitionRender_VertexShader = R"(

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_AlphaTransitionRender_PixelShader = R"(

Texture2D u_FrozenTexture;
SamplerState u_FrozenTexture_sampler;

Texture2D u_CurrentTexture;
SamplerState u_CurrentTexture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frozenFrag = u_FrozenTexture.Sample(u_FrozenTexture_sampler, PSIn.UV);
  float4 currentFrag = u_CurrentTexture.Sample(u_CurrentTexture_sampler, PSIn.UV);
  PSOut.Color = lerp(frozenFrag, currentFrag, PSIn.Color.a);
}

)";

///
// type:
//   mapping transition shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { Texture2D, Texture2D, Texture2D }
///

const std::string kHLSL_MappingTransitionRender_VertexShader = R"(

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = VSIn.Pos;
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_MappingTransitionRender_PixelShader = R"(

Texture2D u_FrozenTexture;
SamplerState u_FrozenTexture_sampler;

Texture2D u_CurrentTexture;
SamplerState u_CurrentTexture_sampler;

Texture2D u_TransTexture;
SamplerState u_TransTexture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frozenFrag = u_FrozenTexture.Sample(u_FrozenTexture_sampler, PSIn.UV);
  float4 currentFrag = u_CurrentTexture.Sample(u_CurrentTexture_sampler, PSIn.UV);
  float transSample = u_TransTexture.Sample(u_TransTexture_sampler, PSIn.UV).r;

  float vague = PSIn.Color.r;
  float progress = PSIn.Color.a;

  transSample = clamp(transSample, progress, progress + vague);
  float mixAlpha = (transSample - progress) / vague;

  PSOut.Color = lerp(currentFrag, frozenFrag, mixAlpha);
}

)";

///
// type:
//   tilemap shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float, float }
///

const std::string kHLSL_TilemapRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct TilemapParams {
  float4 OffsetAndTexSize;
  float4 AnimateIndexAndTileSize;
};

cbuffer TilemapUniformBuffer {
  TilemapParams u_Params;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

static const float2 kAutotileArea = float2(3.0, 28.0);

void main(in VSInput VSIn, out PSInput PSIn) {
  float4 transPos = VSIn.Pos;
  float2 transUV = VSIn.UV;

  // Apply offset
  transPos.x += u_Params.OffsetAndTexSize.x;
  transPos.y += u_Params.OffsetAndTexSize.y;

  // Animated area
  float tile_size = u_Params.AnimateIndexAndTileSize.y;
  float addition = (transUV.x <= kAutotileArea.x * tile_size &&
                    transUV.y <= kAutotileArea.y * tile_size)
                       ? 1.0
                       : 0.0;
  transUV.x += 3.0 * tile_size * u_Params.AnimateIndexAndTileSize.x * addition;

  // Setup pixel shader params
  PSIn.Pos = mul(u_Transform.ProjMat, transPos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = float2(transUV.x * u_Params.OffsetAndTexSize.z,
                   transUV.y * u_Params.OffsetAndTexSize.w);
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_TilemapRender_PixelShader = R"(

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  PSOut.Color.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
  PSOut.Color.a = frag.a;
}

)";

///
// type:
//   tilemap2 shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// bind:
//   { float4x4, float4x4 }
//   { Texture2D }
//   { float2, float2, float2, float }
///

const std::string kHLSL_Tilemap2Render_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct Tilemap2Params {
  float4 OffsetAndTexSize;
  float4 AnimationOffsetAndTileSize;

  float2 Offset;
  float2 TexSize;
  float2 AnimationOffset;  
  float TileSize;
};

cbuffer Tilemap2UniformBuffer {
  Tilemap2Params u_Params;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

float PosInArea(float2 pos, float4 area) {
  return (pos.x >= area.x && pos.y >= area.y && pos.x <= (area.x + area.z) &&
          pos.y <= (area.y + area.w))
             ? 1.0
             : 0.0;
}

static const float2 kRegularArea = float2(12.0, 12.0);
static const float4 kWaterfallArea = float4(12.0, 0.0, 4.0, 12.0);
static const float4 kWaterfallAutotileArea = float4(12.0, 0.0, 2.0, 6.0);

void main(in VSInput VSIn, out PSInput PSIn) {
  float4 transPos = VSIn.Pos;
  float2 transUV = VSIn.UV;

  // Apply offset
  transPos.x += u_Params.OffsetAndTexSize.x;
  transPos.y += u_Params.OffsetAndTexSize.y;

  // Regular area
  float tile_size = u_Params.AnimationOffsetAndTileSize.z;
	float addition1 = (transUV.x <= kRegularArea.x * tile_size &&
                     transUV.y <= kRegularArea.y * tile_size)
                        ? 1.0
                        : 0.0;
	transUV.x += u_Params.AnimationOffsetAndTileSize.x * addition1;

	// Waterfall area
	float addition2 = PosInArea(transUV, kWaterfallArea * tile_size) -
                    PosInArea(transUV, kWaterfallAutotileArea * tile_size);
	transUV.y += u_Params.AnimationOffsetAndTileSize.y * addition2;

  // Setup pixel shader params
  PSIn.Pos = mul(u_Transform.ProjMat, transPos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = float2(transUV.x * u_Params.OffsetAndTexSize.z,
                   transUV.y * u_Params.OffsetAndTexSize.w);
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_Tilemap2Render_PixelShader = R"(

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  PSOut.Color.rgb = lerp(frag.rgb, PSIn.Color.rgb, PSIn.Color.a);
  PSOut.Color.a = frag.a;
}

)";

///
// type:
//   present shader
///
// entry:
//   vertex: main
//   pixel: main
///
// vertex:
//   <float4, float2, float4>
///
// resource:
//   { float4x4, float4x4 }
//   { Texture2D }
//
// defination:
//   CONVERT_PS_OUTPUT_TO_GAMMA
///

const std::string kHLSL_PresentRender_VertexShader = R"(

struct WorldMatrix {
  float4x4 ProjMat;
  float4x4 TransMat;
};

cbuffer WorldMatrixBuffer {
  WorldMatrix u_Transform;
};

struct VSInput {
  float4 Pos : ATTRIB0;
  float2 UV : ATTRIB1;
  float4 Color : ATTRIB2;
};

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

void main(in VSInput VSIn, out PSInput PSIn) {
  PSIn.Pos = mul(u_Transform.ProjMat, VSIn.Pos);
  PSIn.Pos = mul(u_Transform.TransMat, PSIn.Pos);
  PSIn.UV = VSIn.UV;
  PSIn.Color = VSIn.Color;
}

)";

const std::string kHLSL_PresentRender_PixelShader = R"(

Texture2D u_Texture;
SamplerState u_Texture_sampler;

struct PSInput {
  float4 Pos : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

struct PSOutput {
  float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut) {
  float4 frag = u_Texture.Sample(u_Texture_sampler, PSIn.UV);
  frag.a *= PSIn.Color.a;
  PSOut.Color = frag;
#if CONVERT_PS_GAMMA_TO_OUTPUT
  PSOut.Color.rgb = pow(PSOut.Color.rgb, float3(2.2, 2.2, 2.2));
#endif
}

)";

}  // namespace renderer

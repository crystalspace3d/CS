<include>
#ifndef __CS_SHADER_SECTORFOG_CG__
#define __CS_SHADER_SECTORFOG_CG__

struct AppToVert_Fog
{
  void _dummy_struct_non_empty() {}
<?if vars."fog density".float &gt; 0 ?>
  uniform float4 fogPlaneS;
  uniform float4 fogPlaneT;
<?endif?>
};

struct VertToFrag_Fog
{
<?if vars."fog density".float &gt; 0 ?>
  float2 fogTC;
<?endif?>
  
  void Compute (AppToVert_Fog IN, float4 eyePos)
  {
  <?if vars."fog density".float &gt; 0 ?>
    fogTC.x = dot (eyePos, IN.fogPlaneS);
    fogTC.y = dot (eyePos, IN.fogPlaneT);
  <?endif?>
  }
};

struct AppToFrag_Fog
{
<?if vars."fog density".float &gt; 0 ?>
  uniform sampler2D fogTex;
  uniform float4 fogColor;
<?endif?>
  
  float4 ApplyFog (VertToFrag_Fog V2F, float4 color)
  {
  <?if vars."fog density".float &gt; 0 ?>
    float4 fog = tex2D (fogTex, V2F.fogTC);
    float4 result;
    result.rgb = lerp (color.rgb, fogColor.rgb, fog.a);
    result.a = color.a;
    return result;//float4(V2F.fogTC.x, V2F.fogTC.y, 0, 1);
  <?else?>
    return color;
  <?endif?>
  }
};

#endif // __CS_SHADER_SECTORFOG_CG__
</include>

#include "shaderParam.h"

#define M_PI 3.1415926535897932384626433832795

template <typename T>
T clamp(T val, T min, T max)
{
  return val < min ? min : (val > max ? max : val);
}

DDOFParams::DDOFParams (iShaderVarStringSet * svStrings, iPostEffect * effect): effect(effect)
{
  svCoCStrID = svStrings->Request ("coc scale");
  //effect->GetLayer (0)->GetSVContext ()->AddVariable(CoCScale);
}


bool DDOFParams::SetPanel (wxPanel * panel)
{
  pCoCSlider = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderCoCScale")));
  if (!pCoCSlider) return false;
  pCoCSlider->SetMin (0);
  pCoCSlider->SetMax (30);
  SetCoCScale (15, true);
  return true;
}

void DDOFParams::SetCoCScale (float f)
{
  f = clamp<float>(f, 0.f, 30.f);
  if (effect.IsValid())
  {
    if (effect->GetLayer (0))
      effect->GetLayer (0)->GetSVContext ()->GetVariableAdd (svCoCStrID)->SetValue(f);
  }
}

void DDOFParams::SetCoCScale (int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetCoCScale ((float)i);
  if (updateCtrl)
    pCoCSlider->SetValue ((int)i);
}



HBAOParams::HBAOParams(iShaderVarStringSet * svStrings, iPostEffect * effect)
{
  angleBias.AttachNew (new csShaderVariable(svStrings->Request ("angle bias")));
  radius.AttachNew (new csShaderVariable(svStrings->Request ("radius")));
  sqrRadius.AttachNew (new csShaderVariable(svStrings->Request ("sqr radius")));
  invRadius.AttachNew (new csShaderVariable(svStrings->Request ("inv radius")));
  steps.AttachNew (new csShaderVariable(svStrings->Request ("num steps")));
  directions.AttachNew (new csShaderVariable(svStrings->Request ("num directions")));
  contrast.AttachNew (new csShaderVariable(svStrings->Request ("contrast")));
  attenuation.AttachNew (new csShaderVariable(svStrings->Request ("attenuation")));

  blurRadius.AttachNew (new csShaderVariable(svStrings->Request ("blur radius")));
  blurFalloff.AttachNew (new csShaderVariable(svStrings->Request ("blur falloff")));
  blurSharpness.AttachNew (new csShaderVariable(svStrings->Request ("sharpness")));



  effect->GetLayer (0)->GetSVContext ()->AddVariable(angleBias);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(radius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(sqrRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(invRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(steps);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(directions);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(contrast);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(attenuation);


  effect->GetLayer (1)->GetSVContext ()->AddVariable(blurRadius);
  effect->GetLayer (1)->GetSVContext ()->AddVariable(blurFalloff);
  effect->GetLayer (1)->GetSVContext ()->AddVariable(blurSharpness);
  effect->GetLayer (2)->GetSVContext ()->AddVariable(blurRadius);
  effect->GetLayer (2)->GetSVContext ()->AddVariable(blurFalloff);
  effect->GetLayer (2)->GetSVContext ()->AddVariable(blurSharpness);
}

bool HBAOParams::SetPanel(wxPanel * panel)
{
  sliderAngleBias = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderAngleBias")));
  sliderRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAORadius")));
  sliderNumSteps = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOSteps")));
  sliderNumDirs = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAODirs")));
  sliderContrast = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOContrast")));
  sliderAttenuation = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOAttenuation")));
  sliderBlurRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurRadius")));
  sliderBlurSharpness = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));


  if (!sliderAngleBias || !sliderRadius || !sliderNumSteps ||
      !sliderNumDirs || !sliderContrast || !sliderAttenuation ||
      !sliderBlurRadius || !sliderBlurSharpness)
      return false;

  sliderAngleBias->SetMin (0);
  sliderAngleBias->SetMax (90);

  sliderRadius->SetMin (0);
  sliderRadius->SetMax (50);

  sliderNumSteps->SetMin (0);
  sliderNumSteps->SetMax (31);

  sliderNumDirs->SetMin (0);
  sliderNumDirs->SetMax (31);

  sliderContrast->SetMin (0);
  sliderContrast->SetMax (30);

  sliderAttenuation->SetMin (0);
  sliderAttenuation->SetMax (30);

  sliderBlurRadius->SetMin (0);
  sliderBlurRadius->SetMax (15);

  sliderBlurSharpness->SetMin (0);
  sliderBlurSharpness->SetMax (60);

  SetAngleBias(10, true);
  SetRadius(5, true);
  SetNumSteps(8, true);
  SetNumDirections(16, true);
  SetContrast(15, true);
  SetAttenuation(30, true);
  SetBlurRadius(3, true);
  SetBlurSharpness(50, true);
  return true;
}

void HBAOParams::SetAngleBias(float f)
{
  f = clamp<float>(f, 0.f, M_PI/2);
  angleBias->SetValue(f);
}

void HBAOParams::SetRadius(float f)
{
  f = clamp<float>(f, 0.5f, 5);
  radius->SetValue(f);
  sqrRadius->SetValue(f * f);
  invRadius->SetValue(1 / f);
}

void HBAOParams::SetNumSteps(float f)
{
  f = (float)clamp<int>((int)f, 1, 32);
  steps->SetValue(f);
}

void HBAOParams::SetNumDirections(float f)
{
  f = (float)clamp<int>((int)f, 1, 32);
  directions->SetValue(f);
}

void HBAOParams::SetContrast(float f)
{
  f = clamp<float>(f, 0.f, 3.f);
  contrast->SetValue(f);
}

void HBAOParams::SetAttenuation(float f)
{
  f = clamp<float>(f, 0.f, 1.f);
  attenuation->SetValue(f);
}

void HBAOParams::SetBlurRadius(float f)
{
  int i = clamp<int>((int)f, 1, 32);
  if (!(i&1)) i += 1;
  blurRadius->SetValue((float)i);
  blurFalloff->SetValue( 1.f / (float)(2*i*i));
}

void HBAOParams::SetBlurSharpness(float f)
{
  f = clamp<float>(f, 0.f, 6.f);
  f = pow(10.f, f) - 1.f;
  blurSharpness->SetValue(f);
}


void HBAOParams::SetAngleBias(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 90);
  SetAngleBias((float)(i*M_PI / 180.f));
  if (updateCtrl)
    sliderAngleBias->SetValue(i);
}

void HBAOParams::SetRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 50);
  SetRadius(i * 0.1f);
  if (updateCtrl)
    sliderRadius->SetValue(i);
}

void HBAOParams::SetNumSteps(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 31);
  SetNumSteps(1.f + i);
  if (updateCtrl)
    sliderNumSteps->SetValue(i);
}

void HBAOParams::SetNumDirections(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 31);
  SetNumDirections(1.f + i);
  if (updateCtrl)
    sliderNumDirs->SetValue(i);
}

void HBAOParams::SetContrast(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetContrast(0.1f*i);
  if (updateCtrl)
    sliderContrast->SetValue(i);
}

void HBAOParams::SetAttenuation(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetAttenuation( (1.f/30.f)*i );
  if (updateCtrl)
    sliderAttenuation->SetValue(i);
}

void HBAOParams::SetBlurRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 8);
  i = i*2 + 1;
  SetBlurRadius((float)i);
  if (updateCtrl)
    sliderBlurRadius->SetValue(i);
}

void HBAOParams::SetBlurSharpness(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 60);
  SetBlurSharpness((float)i);
  if (updateCtrl)
    sliderBlurSharpness->SetValue(i);
}

SSDOParams::SSDOParams(iShaderVarStringSet * svStrings, iPostEffect * effect)
{
  enableAO.AttachNew (new csShaderVariable(svStrings->Request ("enable ambient occlusion")));
  enableIL.AttachNew (new csShaderVariable(svStrings->Request ("enable indirect light")));
  sampleRadius.AttachNew (new csShaderVariable(svStrings->Request ("sample radius")));
  datailSampleRadius.AttachNew (new csShaderVariable(svStrings->Request ("detail sample radius")));
  passes.AttachNew (new csShaderVariable(svStrings->Request ("num passes")));
  selfOcclusion.AttachNew (new csShaderVariable(svStrings->Request ("self occlusion")));
  occlusionStrength.AttachNew (new csShaderVariable(svStrings->Request ("occlusion strength")));
  maxDist.AttachNew (new csShaderVariable(svStrings->Request ("max occluder distance")));
  bounceStrength.AttachNew (new csShaderVariable(svStrings->Request ("bounce strength")));

  effect->GetLayer (0)->GetSVContext ()->AddVariable(enableAO);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(enableIL);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(sampleRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(datailSampleRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(passes);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(selfOcclusion);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(occlusionStrength);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(maxDist);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(bounceStrength);
}

bool SSDOParams::SetPanel(wxPanel * panel)
{
  sliderRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderDetailRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderNumPasses = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderSelfOcclusin = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderOcclusionStrength = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderMaxDist = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
  sliderBounceStrength = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));
}

void SSDOParams::SetRadius(float f)
{
}
void SSDOParams::SetRadius(int i, bool updateCtrl)
{
}

void SSDOParams::SetDetailRadius(float f)
{
}
void SSDOParams::SetDetailRadius(int i, bool updateCtrl)
{
}

void SSDOParams::SetNumPasses(float f)
{
}
void SSDOParams::SetNumPasses(int i, bool updateCtrl)
{
}

void SSDOParams::SetSelfOcclusion(float f)
{
}
void SSDOParams::SetSelfOcclusion(int i, bool updateCtrl)
{
}

void SSDOParams::SetOcclusionStrength(float f)
{
}
void SSDOParams::SetOcclusionStrength(int i, bool updateCtrl)
{
}

void SSDOParams::SetMaxOcclusionDist(float f)
{
}
void SSDOParams::SetMaxOcclusionDist(int i, bool updateCtrl)
{
}

void SSDOParams::SetBounceStrength(float f)
{
}
void SSDOParams::SetBounceStrength(int i, bool updateCtrl)
{
}
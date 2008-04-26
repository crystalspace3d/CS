/*
    Copyright (C) 2005 by Jorrit Tyberghein
              (C) 2005 by Frank Richter

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"

#include "csgfx/lightsvcache.h"

#include "csutil/csstring.h"

void csLightShaderVarCache::ClearDefVars ()
{
  for (size_t n = 0; n < _varCount; n++)
    defaultVars[n] = CS::InvalidShaderVarStringID;
  for (size_t n = 0; n < _lightCount; n++)
    lightSVIdCache_unnumbered[n] = CS::InvalidShaderVarStringID;
}

void csLightShaderVarCache::SetStrings (iShaderVarStringSet* strings)
{
  lightSVIdCache.DeleteAll ();
  ClearDefVars();
  this->strings = strings;
}
  
namespace
{
  static const char* const svSuffixes[csLightShaderVarCache::_lightCount] =
  {
    "diffuse",
    "specular",
    "position object",
    "position",
    "position world",
    "transform",
    "transform world",
    "transform world inverse",
    "attenuation",
    "attenuationtex",
    "direction object",
    "direction",
    "direction world",
    "inner falloff",
    "outer falloff",
    "type",
    "attenuation mode",
    "shadow map",
    "shadow map projection",
    "shadow map id",
    "shadow map pixel size"
  };
}

CS::ShaderVarStringID csLightShaderVarCache::GetLightSVId (size_t num, LightProperty prop)
{
  if (!strings.IsValid()) return CS::InvalidShaderVarStringID;
  
  if (num >= lightSVIdCache.GetSize())
  {
    csString str;
    for (size_t n = lightSVIdCache.GetSize(); n <= num; n++)
    {
      for (int p = 0; p < _lightCount; p++)
      {
	CS_ASSERT_MSG (
	  "You added stuff to csLightShaderVarCache::LightProperty "
	  "but didn't update " __FILE__,
	  svSuffixes[p] != 0);
	str.Format ("light %zu %s", n, svSuffixes[p]);
	lightSVIdCache.GetExtend (num).ids[p] = strings->Request (str);
      }
    }
  }
  return lightSVIdCache[num].ids[prop];
}

CS::ShaderVarStringID csLightShaderVarCache::GetLightSVId (LightProperty prop)
{
  if (!strings.IsValid()) return CS::InvalidShaderVarStringID;

  if (lightSVIdCache_unnumbered[prop] == csInvalidStringID)
  {
    csString str;
    str.Format ("light %s", svSuffixes[prop]);
    lightSVIdCache_unnumbered[prop] = strings->Request (str);
  }
  
  return lightSVIdCache_unnumbered[prop];
}

CS::ShaderVarStringID csLightShaderVarCache::GetDefaultSVId (DefaultSV var)
{
  static const char* const svNames[_varCount] = {
    "light ambient",
    "light count"
  };
  
  if (!strings.IsValid()) return CS::InvalidShaderVarStringID;

  if (defaultVars[var] == csInvalidStringID)
  {
    CS_ASSERT_MSG (
      "You added stuff to csLightShaderVarCache::DefaultSV "
      "but didn't update " __FILE__,
      defaultVars[var] != 0);
    defaultVars[var] = strings->Request (svNames[var]);
  }
  return defaultVars[var];
}

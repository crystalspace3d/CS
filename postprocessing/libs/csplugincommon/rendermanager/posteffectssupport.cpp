/*
    Copyright (C) 2010 by Frank Richter

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

#include "csplugincommon/rendermanager/posteffectssupport.h"

#include "csutil/cfgacc.h"
#include "imap/loader.h"
#include "iutil/plugin.h"

namespace CS
{
  namespace RenderManager
  {
    PostEffectsSupport::PostEffectsSupport () : postEffectParser (nullptr)
    {
    }
    
    PostEffectsSupport::~PostEffectsSupport ()
    {
      delete postEffectParser;
    }
    
    void PostEffectsSupport::Initialize (iObjectRegistry* objectReg, const char* configKey)
    {
      csRef<iPluginManager> pluginManager = 
        csQueryRegistry<iPluginManager> (objectReg);

      postEffectManager = csLoadPlugin<iPostEffectManager>
        (pluginManager, "crystalspace.rendermanager.posteffect");

      postEffectParser = new PostEffectLayersParser (objectReg);

      // Check for a post-effect to be applied
      if (configKey)
      {
        csString realConfigKey (configKey);
        realConfigKey.Append (".Effects");
        csConfigAccess cfg (objectReg);
        const char* effectsFile = cfg->GetStr (realConfigKey, nullptr);

        if (effectsFile)
        {
          csRef<iPostEffect> effect = CreatePostEffect (effectsFile);
          if (postEffectParser->AddLayersFromFile (effectsFile, effect))
            AddPostEffect (effect);
        }
      }
    }
    
    void PostEffectsSupport::ClearLayers ()
    {
      if (postEffects.GetSize ())
	    postEffects[0]->ClearLayers ();
    }

    bool PostEffectsSupport::AddLayersFromDocument (iDocumentNode* node)
    {
      if (postEffects.GetSize ())
	    return postEffectParser->AddLayersFromDocument (node, postEffects[0]);

      return false;
    }
    
    bool PostEffectsSupport::AddLayersFromFile (const char* filename)
    {
      if (postEffects.GetSize ())
        return postEffectParser->AddLayersFromFile (filename, postEffects[0]);

      return false;
    }
    
    csPtr<iPostEffect> PostEffectsSupport::CreatePostEffect (const char* name) const
    {
      if (!postEffectManager)
        return csPtr<iPostEffect> (nullptr);
      return postEffectManager->CreatePostEffect (name);
    }

    void PostEffectsSupport::AddPostEffect (iPostEffect* effect)
    {
      if (!effect) return;

      size_t pfxCount = postEffects.GetSize ();
      if (pfxCount > 0)
      {
        iTextureHandle* target = postEffects.Get (pfxCount - 1)->GetOutputTarget ();
        effect->SetOutputTarget (target);
      }

      postEffects.Push (effect);      
    }
    
    bool PostEffectsSupport::InsertPostEffect (iPostEffect* effect, size_t index)
    {
      if (!effect) return false;

      size_t pfxCount = postEffects.GetSize ();
      bool result = postEffects.Insert (index, effect);
      
      if (result && pfxCount > 1)
      {
        iTextureHandle* target = postEffects.Get (pfxCount - 1)->GetOutputTarget ();
        effect->SetOutputTarget (target);
      }

      return result;
    }

    size_t PostEffectsSupport::FindPostEffect (const char* name) const
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
        if (strcmp (postEffects[i]->GetName (), name) == 0)
	  return i;

      return (size_t) ~0;
    }

    bool PostEffectsSupport::RemovePostEffect (iPostEffect* effect)
    {
      return postEffects.Delete (effect);
    }

    bool PostEffectsSupport::RemovePostEffect (size_t index)
    {      
      return postEffects.DeleteIndex (index);
    }
    
    size_t PostEffectsSupport::GetPostEffectCount () const
    {
      return postEffects.GetSize ();
    }
    
    iPostEffect* PostEffectsSupport::GetPostEffect (size_t index)
    {
      return postEffects.Get (index);
    }

    iTextureHandle* PostEffectsSupport::GetScreenTarget () const
    {
      if (postEffects.IsEmpty ())
        return nullptr;
      
      return postEffects.Get (0)->GetScreenTarget ();
    }

    void PostEffectsSupport::SetEffectsOutputTarget (iTextureHandle* tex)
    {
      if (postEffects.GetSize () == 0) return;
      
      postEffects.Get (postEffects.GetSize () - 1)->SetOutputTarget (tex);
    }
    
    iTextureHandle* PostEffectsSupport::GetEffectsOutputTarget () const
    {
      if (postEffects.GetSize () == 0) 
        return nullptr;

      return postEffects.Get (postEffects.GetSize () - 1)->GetOutputTarget ();
    }

    void PostEffectsSupport::ClearIntermediates ()
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
        postEffects.Get (i)->ClearIntermediates ();
    }
    
    void PostEffectsSupport::DrawPostEffects (RenderTreeBase& renderTree)
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
        postEffects.Get (i)->DrawPostEffect (renderTree);
    }

    bool PostEffectsSupport::SetupView (iView* view, CS::Math::Matrix4& perspectiveFixup)
    {
      uint width = view->GetContext ()->GetWidth ();
      uint height = view->GetContext ()->GetHeight ();

      return SetupView (width, height, perspectiveFixup);
    }

    bool PostEffectsSupport::SetupView (uint width, uint height, 
      CS::Math::Matrix4& perspectiveFixup)
    {
      size_t pfxCount = postEffects.GetSize ();
      if (!pfxCount)
      {
        perspectiveFixup = CS::Math::Matrix4 ();
        return false;
      }

      bool effectsDataChanged = false;
      for (int i = pfxCount - 1; i >= 0; i--)
      {
        iPostEffect* effect = postEffects.Get (i);
        effectsDataChanged = effect->SetupView (width, height);

        if (effectsDataChanged && i > 0)
        {
          iPostEffect* prevEffect = postEffects.Get (i - 1);
          prevEffect->SetOutputTarget (effect->GetScreenTarget ());
        }
      }

      // Set the perspective fixup with the last effect
      iPostEffect* lastEffect = postEffects.Get (pfxCount - 1);
      iTextureHandle* screenTarget = lastEffect->GetScreenTarget ();
      if (screenTarget)
      {
        int targetW, targetH;
        screenTarget->GetRendererDimensions (targetW, targetH);
        float scaleX = float (width)/float (targetW);
        float scaleY = float (height)/float (targetH);
        perspectiveFixup = CS::Math::Matrix4 (
	        scaleX, 0, 0, scaleX-1.0f,
	        0, scaleY, 0, scaleY-1.0f,
	        0, 0, 1, 0,
	        0, 0, 0, 1);
      }
      else
      {
        perspectiveFixup = CS::Math::Matrix4 ();
      }

      return effectsDataChanged;
    }

    bool PostEffectsSupport::ScreenSpaceYFlipped () const
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
      {
        if (postEffects.Get (i)->ScreenSpaceYFlipped ())
          return true;
      }

      return false;
    }

  } // namespace RenderManager
} // namespace CS

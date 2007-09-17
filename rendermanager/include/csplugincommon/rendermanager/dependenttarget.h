/*
    Copyright (C) 2007 by Marten Svanfeldt
	      (C) 2007 by Frank Richter

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_DEPENDENTTARGET_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_DEPENDENTTARGET_H__

#include "csutil/fifo.h"
#include "iengine/rendermanager.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/shadersetup.h"

namespace CS
{
namespace RenderManager
{

  /**
   * Traverse all the shader variables used in a certain render context.
   */
  template<typename Tree, typename Fn, typename LayerConfigType,
    typename BitArray = csBitArray>
  class TraverseAllUsedSVs
  {    
  public:
    TraverseAllUsedSVs (Fn& function, iShaderManager* shaderManager,
      BitArray& nameStorage, const LayerConfigType& layerConfig) 
      : shaderManager (shaderManager), function (function), 
      nameStorage (nameStorage), layerConfig (layerConfig)
    {}

    void operator() (typename Tree::ContextNode* contextNode, Tree& tree)
    {
      for (size_t layer = 0; layer < layerConfig.GetLayerCount (); ++layer)
      {
        FnMeshTraverser traverse (shaderManager, function, nameStorage, layer);
        tree.TraverseMeshNodes (traverse, contextNode);
      }
    }

  private:
    iShaderManager* shaderManager;
    Fn& function;
    BitArray& nameStorage;
    const LayerConfigType& layerConfig;

    /**
    * The actual workhorse: for each mesh in the context construct the SV
    * stack. Then go over that stack, using the known used shader vars, and
    * call Fn.
    */
    struct FnMeshTraverser : 
      public NumberedMeshTraverser<Tree, FnMeshTraverser>
    {
      typedef NumberedMeshTraverser<Tree, FnMeshTraverser> BaseType;

      iShaderManager* shaderManager;
      Fn& function;
      BitArray& names;
      size_t layer;

      FnMeshTraverser (iShaderManager* shaderManager, Fn& function, 
        BitArray& names, size_t layer) 
        : BaseType (*this), shaderManager (shaderManager), 
        function (function), names (names), layer (layer)
      {
      }

      void operator() (const typename Tree::TreeTraitsType::MeshNodeKeyType& key,
        typename Tree::MeshNode* node, typename Tree::ContextNode& ctxNode, Tree& tree)
      {
        lastShader = 0;
        lastTicket = (size_t)~0;

        BaseType::operator() (key, node, ctxNode, tree);
      }

      void operator() (typename Tree::MeshNode* node,
        const typename Tree::MeshNode::SingleMesh& mesh, size_t index,
        typename Tree::ContextNode& ctxNode, const Tree& tree)
      {
        size_t layerOffset = ctxNode.totalRenderMeshes * layer;

        iShader* shader = ctxNode.shaderArray[index+layerOffset];
        if (!shader) return;
        size_t ticket = ctxNode.ticketArray[index+layerOffset];

        if ((shader != lastShader) || (ticket != lastTicket))
        {
          names.Clear();
          shader->GetUsedShaderVars (ticket, names);
        }

        csShaderVariableStack varStack;        
        ctxNode.svArrays.SetupSVStack (varStack, layer, index);

        size_t name = csMin (names.GetSize(), varStack.GetSize());
        while (name-- > 0)
        {
          if (names.IsBitSet (name))
          {
            csShaderVariable* sv = varStack[name];
            if (sv != 0) 
              function ((csStringID)name, sv);
          }
        }
      }

      iShader* lastShader;
      size_t lastTicket;
    };
  };

  template<typename Tree, typename Fn, typename LayerConfigType, typename BitArray = csBitArray>
  class TraverseAllTextures
  {
  public:
    TraverseAllTextures (Fn& function, iShaderManager* shaderManager, 
      BitArray& names, const LayerConfigType& layerConfig) 
    : function (function), shaderManager (shaderManager),
      names (names), layerConfig (layerConfig)
    {}

    void operator() (typename Tree::ContextNode* contextNode, Tree& tree)
    {
      FnShaderVarTraverser svTraverser (function);
      TraverseAllUsedSVs<Tree, FnShaderVarTraverser, LayerConfigType, BitArray> 
        traverseSVs (svTraverser, shaderManager, names, layerConfig);
      traverseSVs (contextNode, tree);
    }

  private:
    Fn& function;
    iShaderManager* shaderManager;
    BitArray& names;
    const LayerConfigType& layerConfig;

    struct FnShaderVarTraverser
    {
      Fn& function;

      FnShaderVarTraverser (Fn& function) : function (function)
      {
      }

      void operator() (csStringID name, csShaderVariable* sv)
      {
        if (sv->GetType() == csShaderVariable::TEXTURE)
        {
          iTextureHandle* texh;
          sv->GetValue (texh);
          function (name, sv, texh);
        }
      }
    };
  };

  template<typename Tree, typename TargetHandler>
  class DependentTargetManager
  {
  public:
    DependentTargetManager (TargetHandler& targetHandler)
      : targetHandler (targetHandler)
    {}

    void RegisterRenderTarget (iTextureHandle* target, 
      iView* view, int subtexture = 0, uint flags = 0)
    {
      typename RenderTargetInfo::ViewInfo newView;
      newView.view = view;
      newView.flags = flags & ~iRenderManager::updateOnce;
      TargetsHash* targets;
      if (flags & iRenderManager::updateOnce)
      {
        targets = &oneTimeTargets;
      }
      else
      {
        targets = &this->targets;
      }
      RenderTargetInfo* targetInfo = targets->GetElementPointer (target);
      if (targetInfo == 0)
      {
        RenderTargetInfo newInfo;
        newInfo.views.PutUnique (subtexture, newView);
        targets->PutUnique (target, newInfo);
      }
      else
      {
        targetInfo->views.PutUnique (subtexture, newView);
      }
    }
    void UnregisterRenderTarget (iTextureHandle* target,
      int subtexture = 0)
    {

      RenderTargetInfo* targetInfo = targets.GetElementPointer (target);
      if (targetInfo != 0)
      {
        targetInfo->views.DeleteAll (subtexture);
        if (targetInfo->views.IsEmpty()) targets.DeleteAll (target);
        return;
      }
      targetInfo = oneTimeTargets.GetElementPointer (target);
      if (targetInfo != 0)
      {
        targetInfo->views.DeleteAll (subtexture);
        if (targetInfo->views.IsEmpty()) oneTimeTargets.DeleteAll (target);
      }
    }

    void PrepareQueues (iShaderManager* shaderManager)
    {
      size_t numSVs = shaderManager->GetSVNameStringset()->GetSize();
      names.SetSize (numSVs);
      handledTargets.Empty();
      handledTargets.SetCapacity (targets.GetSize() + oneTimeTargets.GetSize());

      targetQueue.DeleteAll ();
    }

    template<typename LayerConfigType>
    void EnqueueTargetsInContext (typename Tree::ContextsContainer* contexts,
      Tree& renderTree, iShaderManager* shaderManager, const LayerConfigType& layerConfig)
    {
      TargetTraverser texTraverse (*this, renderTree);
      TraverseAllTextures<Tree, TargetTraverser, LayerConfigType> traverseAllTextures (
        texTraverse,  shaderManager, names, layerConfig);
      renderTree.TraverseContexts (contexts, traverseAllTextures);
    }

    bool HaveMoreTargets() const
    {
      return targetQueue.GetSize () > 0;
    }

    void GetNextTarget (csStringID& svName, typename Tree::ContextsContainer* &contexts)
    {
      TargetQueueEntry entry = targetQueue.PopTop ();

      svName = entry.targetSVName;
      contexts = entry.contexts;
    }

    void PostCleanupQueues ()
    {
      oneTimeTargets.DeleteAll ();
      targetQueue.DeleteAll ();
    }
  private:
    typedef DependentTargetManager<Tree, TargetHandler> DependentTargetManagerType;
    /* FIXME: better handle multiple views per target
	      'flags' per subtexture seems odd */
    struct RenderTargetInfo
    {
      struct ViewInfo
      {
        csRef<iView> view;
        uint flags;
      };
      typedef csHash<ViewInfo, int> ViewsHash;
      ViewsHash views;
    };
    typedef csHash<RenderTargetInfo, csRef<iTextureHandle> > TargetsHash;
    TargetsHash targets;
    TargetsHash oneTimeTargets;
    
    class HandledTargetsSet
    {
    public:
      void SetCapacity (size_t n) 
      {
        set.SetCapacity (n); 
      }

      void Empty () 
      {
        set.Empty(); 
      }
      
      void Insert (size_t index, iTextureHandle* texh)
      {
        set.Insert (index, texh);
      }

      bool Find (iTextureHandle* texh, size_t& candidate) const
      {
        return set.FindSortedKey (
          csArrayCmp<iTextureHandle*, iTextureHandle*> (texh), &candidate)
          != csArrayItemNotFound;
      }

    private:
      csArray<iTextureHandle*> set;
    };
    
    struct TargetQueueEntry
    {
      typename Tree::ContextsContainer* contexts;
      csStringID targetSVName;
    };

    // 
    TargetHandler& targetHandler;

    // Storage for used SV names
    csBitArray names;
    // Storage for handled targets
    HandledTargetsSet handledTargets;

    // Queue of contexts to setup
    csFIFO<TargetQueueEntry> targetQueue;

    struct TargetTraverser
    {
      TargetTraverser (DependentTargetManagerType& parent, Tree& renderTree)
        : parent (parent), renderTree (renderTree)
      {}

      void operator() (csStringID name, csShaderVariable* sv, iTextureHandle *textureHandle)
      {
        iView* localView = 0;
        bool handleTarget = false;

        // Check any of the explicit targets
        RenderTargetInfo* targetInfo;
        targetInfo = parent.targets.GetElementPointer (textureHandle);
        handleTarget = (targetInfo != 0);
        targetInfo = parent.oneTimeTargets.GetElementPointer (textureHandle);
        handleTarget |= (targetInfo != 0);

        // Dispatch upwards
        if (!handleTarget)
        {
          handleTarget = parent.targetHandler.HandleTargetSetup (name, sv, 
            textureHandle, localView);          
        }

        if (handleTarget)
        {
          size_t insertPos;
          if (parent.handledTargets.Find (textureHandle, insertPos)) return;
          parent.handledTargets.Insert (insertPos, textureHandle);

          if (targetInfo)
          {
            typename RenderTargetInfo::ViewsHash::GlobalIterator viewsIt (
              targetInfo->views.GetIterator());

            while (viewsIt.HasNext ())
            {
              int subtexture;
              const typename RenderTargetInfo::ViewInfo& viewInfo (
                viewsIt.Next (subtexture));
              HandleView (viewInfo.view, textureHandle, subtexture, name);
            }
          }
          else
          {
            HandleView (localView, textureHandle, 0, name);
          }
        }
      }


      void HandleView (iView* targetView,
        iTextureHandle* texh, int subtexture, csStringID svName)
      {
        if (!targetView) return;

        // Setup a rendering view
        targetView->UpdateClipper ();
        csRef<CS::RenderManager::RenderView> rview;
        rview.AttachNew (new (renderTree.GetPersistentData().renderViewPool)
          RenderView (targetView));

	typename Tree::ContextsContainer* targetContexts = 
	  renderTree.CreateContextContainer ();
	targetContexts->renderTarget = texh;
	targetContexts->subtexture = subtexture;
	targetContexts->rview = rview;

        TargetQueueEntry e;
        e.contexts = targetContexts;
        e.targetSVName = svName;
	
        parent.targetQueue.Push (e);
      }


      DependentTargetManager& parent;
      Tree& renderTree;
    };

    friend struct TargetTraverser;
  };

} // namespace RenderManager
} // namespace CS

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_DEPENDENTTARGET_H__

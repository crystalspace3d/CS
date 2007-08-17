/*
    Copyright (C) 2007 by Marten Svanfeldt

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_RENDERTREE_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_RENDERTREE_H__

#include "csplugincommon/rendermanager/standardtreetraits.h"
#include "iengine/viscull.h"
#include "csutil/redblacktree.h"

namespace CS
{
namespace RenderManager
{
  
  template<typename T>
  struct EBOptHelper : public T
  {};

  template<>
  struct EBOptHelper<void>
  {};

  /**
   * 
   */
  template<typename TreeTraits = RenderTreeStandardTraits>
  class RenderTree
  {
  public:
    //---- Forward declarations
    struct MeshNode;
    struct ContextNode;
    struct ContextsContainer;

    //---- Type definitions
    typedef TreeTraits TreeTraitsType;
    typedef RenderTree<TreeTraits> ThisType;

    /**
     * 
     */
    struct PersistentData
    {
      void Clear ()
      {
        // Clean up the persistent data
        contextNodeAllocator.Empty ();
        meshNodeAllocator.Empty ();
        contextsContainerAllocator.Empty ();
      }

      csBlockAllocator<MeshNode> meshNodeAllocator;
      csBlockAllocator<ContextNode> contextNodeAllocator;
      csBlockAllocator<ContextsContainer> contextsContainerAllocator;

      csStringID svObjectToWorldName;

      void Initialize (iShaderManager* shmgr)
      {
        svObjectToWorldName = 
          shmgr->GetSVNameStringset()->Request ("object2world transform");
      }
    };

    /**
     * 
     */
    struct MeshNode : public EBOptHelper<typename TreeTraits::MeshNodeExtraDataType>
    {
      /**
       * 
       */
      struct SingleMesh : public EBOptHelper<typename TreeTraits::MeshExtraDataType>
      {
        csRenderMesh* renderMesh;
        csZBufMode zmode;
      };

      csArray<SingleMesh> meshes;
    };

    /**
     * 
     */
    struct ContextNode : public EBOptHelper<typename TreeTraits::ContextNodeExtraDataType>
    {
      /**
       * 
       */
      struct PortalHolder
      {
        iPortalContainer* portalContainer;
        iMeshWrapper* meshWrapper;
      };

      // A sub-tree of mesh nodes
      csRedBlackTreeMap<typename TreeTraits::MeshNodeKeyType, MeshNode*> meshNodes;

      // All portals within context
      csArray<PortalHolder> allPortals;

      // Total number of render meshes within the context
      size_t totalRenderMeshes;
      
      ContextNode() : totalRenderMeshes (0) {}
    };

    class ContextsContainer : 
      public EBOptHelper<typename TreeTraits::ContextsContainerExtraDataType>
    {
    protected:
      friend class RenderTree;
      csArray<ContextNode*>   contextNodeList;
    };

    //---- Methods
    RenderTree (PersistentData& dataStorage)
      : persistentData (dataStorage), totalMeshNodes (0), totalRenderMeshes (0)
    {
    }

    ~RenderTree ()
    {
      persistentData.Clear ();
    }

    ContextsContainer* CreateContextContainer ()
    {
      // Create an initial context
      ContextsContainer* newCtx = persistentData.contextsContainerAllocator.Alloc ();
      contexts.Push (newCtx);

      return newCtx;
    }

    /**
     * 
     */
    ContextNode* CreateContext (ContextsContainer* contexts, iRenderView* rw)
    {
      // Create an initial context
      ContextNode* newCtx = persistentData.contextNodeAllocator.Alloc ();
      contexts->contextNodeList.Push (newCtx);

      return newCtx;
    }

    /**
     * 
     */
    bool Viscull (ContextsContainer* contexts, ContextNode* context, iRenderView* rw, 
      iVisibilityCuller* culler)
    {
      csArray<CulledObject> culledObjects;
      ViscullCallback cb (culledObjects);
      
      culler->VisTest (rw, &cb);

      csDirtyAccessArray<iMeshWrapper*> meshObjs;
      meshObjs.SetSize (culledObjects.GetSize() * 2);
      for (size_t i = 0; i < culledObjects.GetSize(); i++)
        meshObjs[i] = culledObjects[i].imesh;

      size_t numFiltered;
      iMeshWrapper** filtered = meshObjs.GetArray()+culledObjects.GetSize();
      contexts->view->FilterMeshes (meshObjs.GetArray(), 
        culledObjects.GetSize(), filtered, numFiltered);
      
      size_t c = 0;
      for (size_t i = 0; i < numFiltered; i++)
      {
        while (culledObjects[c].imesh != filtered[i]) { c++; }
        ViscullObjectVisible (culledObjects[c].imesh, culledObjects[c].frustum_mask,
          rw, context);
        c++;
      }
      return true;
    }

    /**
     * 
     */
    void DestoryContext (ContextsContainer& contexts, ContextNode* context)
    {
      CS_ASSERT(contextNodeList.Find (contextNodeList) != csArrayItemNotFound);
      contextNodeList.Delete (context);

      persistentData.contextNodeAllocator.Free (context);
    }

    /**
     * 
     */
    template<typename Fn>
    void TraverseContexts (ContextsContainer* contexts, Fn& contextFunction)
    {
      CS::ForEach (contexts->contextNodeList.GetIterator (), contextFunction, *this);
    }

    template<typename Fn>
    void TraverseContexts (Fn& contextFunction)
    {
      ContextsIterator<Fn, false> it (contextFunction);
      CS::ForEach (contexts.GetIterator (), it, *this);
    }

    template<typename Fn>
    void TraverseContextContainers (Fn& contextFunction)
    {
      CS::ForEach (contexts.GetIterator (), contextFunction, *this);
    }

    /**
     * 
     */
    template<typename Fn>
    void TraverseContextsReverse (ContextsContainer* contexts, Fn& contextFunction)
    {
      CS::ForEach (contexts->contextNodeList.GetReverseIterator (), 
        contextFunction, *this);
    }

    template<typename Fn>
    void TraverseContextsReverse (Fn& contextFunction)
    {
      ContextsIterator<Fn, true> it (contextFunction);
      CS::ForEach (contexts.GetReverseIterator (), it, *this);
    }

    template<typename Fn>
    void TraverseContextContainersReverse (Fn& contextFunction)
    {
      CS::ForEach (contexts.GetReverseIterator  (), contextFunction, *this);
    }

    /**
     * 
     */
    template<typename Fn>
    void TraverseMeshNodes (ContextsContainer* contexts, Fn& meshNodeFunction)
    {
      TraverseMeshNodesWalker<Fn> walker (meshNodeFunction);
      TraverseContexts (contexts, walker);
    }

    template<typename Fn>
    void TraverseMeshNodes (Fn& meshNodeFunction, ContextNode* context = 0)
    {
      TraverseMeshNodesWalker<Fn> walker (meshNodeFunction);
      if (context)
        walker (context, *this, 0 /* currently ignored */);
      else
        TraverseContexts (walker);
    }


    // Some helpers
    ///
    inline size_t GetTotalMeshNodes () const
    {
      return totalMeshNodes;
    }

    inline size_t GetTotalRenderMeshes () const
    {
      return totalRenderMeshes;
    }

    csStringID GetObjectToWorldName() const
    {
      return persistentData.svObjectToWorldName;
    }

  protected:    
    PersistentData&         persistentData;
    csArray<ContextsContainer*> contexts;
    size_t                  totalMeshNodes;
    size_t                  totalRenderMeshes;

    struct CulledObject
    {
      iMeshWrapper *imesh;
      uint32 frustum_mask;
    };

    /**
     * 
     */
    class ViscullCallback : public scfImplementation1<ViscullCallback, iVisibilityCullerListener>
    {
    public:
      ViscullCallback (csArray<CulledObject>& culledObjects)
        : scfImplementation1<ViscullCallback, iVisibilityCullerListener> (this), 
        culledObjects (culledObjects)
      {}


      virtual void ObjectVisible (iVisibilityObject *visobject, 
        iMeshWrapper *mesh, uint32 frustum_mask)
      {
        // Call uppwards
        //ownerTree.ViscullObjectVisible (visobject, mesh, frustum_mask, currentRenderView, context);
        CulledObject culled;
        culled.imesh = mesh;
        culled.frustum_mask = frustum_mask;
        culledObjects.Push (culled);
      }

    private:
      csArray<CulledObject>& culledObjects;
    };

    void ViscullObjectVisible (iMeshWrapper *imesh, uint32 frustum_mask, 
      iRenderView* renderView, ContextNode* context)
    {
      // Todo: Handle static lod & draw distance
      csZBufMode zmode = imesh->GetZBufMode ();
      csRef<csShaderVariable> svObjectToWorld;

      // Get the meshes
      int numMeshes;
      csRenderMesh** meshList = imesh->GetRenderMeshes (numMeshes, renderView, frustum_mask);

      // Add it to the appropriate meshnode
      for (int i = 0; i < numMeshes; ++i)
      {
        csRenderMesh* rm = meshList[i];

        if (rm->portal)
        {
          ContextNode::PortalHolder h = {rm->portal, imesh};
          context->allPortals.Push (h);
          continue;
        }

        typename TreeTraits::MeshNodeKeyType meshKey = 
          TreeTraits::GetMeshNodeKey (imesh, *rm);

        // Get the mesh node
        MeshNode* meshNode = context->meshNodes.Get (meshKey, 0);
        if (!meshNode)
        {
          // Get a new one
          meshNode = persistentData.meshNodeAllocator.Alloc ();

          TreeTraits::SetupMeshNode(*meshNode, imesh, *rm);
          context->meshNodes.Put (meshKey, meshNode);

          totalMeshNodes++;
        }

        svObjectToWorld.AttachNew (new csShaderVariable (
          persistentData.svObjectToWorldName));
        svObjectToWorld->SetValue (rm->object2world);
 
        typename MeshNode::SingleMesh sm;
        sm.renderMesh = rm;
        sm.zmode = zmode;
        sm.meshObjSVs = imesh->GetSVContext();
        sm.svObjectToWorld = svObjectToWorld;

        meshNode->meshes.Push (sm);
      }

      context->totalRenderMeshes += numMeshes;
      totalRenderMeshes += numMeshes;
    }
    
    template<typename Fn>
    struct TraverseMeshNodesWalker
    {
      TraverseMeshNodesWalker (Fn& meshNodeFunction)
        : meshNodeFunction (meshNodeFunction)
      {}

      void operator() (ContextNode* node, ThisType& tree, 
        const ContextsContainer* container)
      {        
        MeshNodeCB<Fn> cb (meshNodeFunction, node, tree);
        node->meshNodes.TraverseInOrder (cb);
      }

      Fn& meshNodeFunction;
    };

    private:

      template<typename Fn>
      struct MeshNodeCB
      {
        MeshNodeCB(Fn& meshNodeFunction, ContextNode* node, ThisType& tree)
          : meshNodeFunction (meshNodeFunction), node (node), tree (tree)
        {}

        void operator() (const typename TreeTraits::MeshNodeKeyType& key, MeshNode* meshNode)
        {
          meshNodeFunction (key, meshNode, *node, tree);
        }

        Fn& meshNodeFunction;
        ContextNode* node;
        ThisType& tree;
      };

    template<typename Fn, bool reverse>
    struct ContextsIterator
    {
      Fn& function;

      ContextsIterator (Fn& function) : function (function) { }

      void operator() (const ContextsContainer* contexts, ThisType& tree)
      {
        if (reverse)
          CS::ForEach (contexts->contextNodeList.GetReverseIterator (), 
            function, tree, contexts);
        else
          CS::ForEach (contexts->contextNodeList.GetIterator (), function, 
            tree, contexts);
      }
    };
  };

  template<typename Tree, typename Fn>
  class NumberedMeshTraverser
  {
  public:
    typedef NumberedMeshTraverser<Tree, Fn> NumberedMeshTraverserType;

    NumberedMeshTraverser (Fn& fun)
      : fun(fun), meshOffset (0)
    {}

    void operator() (const typename Tree::TreeTraitsType::MeshNodeKeyType& key,
      typename Tree::MeshNode* node, typename Tree::ContextNode& ctxNode, Tree& tree)
    {
      for (size_t i = 0; i < node->meshes.GetSize(); ++i)
      {
        fun (node, node->meshes[i], meshOffset++, ctxNode, tree);
      }
    }

  private:
    Fn& fun;
    size_t meshOffset;
  };
 

}
}

#endif

/*
    Copyright (C) 2006 by Hristo Hristov

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

#ifndef __CS_SKELETON_H__
#define __CS_SKELETON_H__

#include "csgeom/quaternion.h"
#include "csgeom/transfrm.h"
#include "csgeom/vector3.h"
#include "csutil/array.h"
#include "csutil/cscolor.h"
#include "csutil/csstring.h"
#include "csutil/flags.h"
#include "csutil/hash.h"
#include "csutil/parray.h"
#include "csutil/refarr.h"
#include "csutil/strhash.h"
#include "csutil/stringarray.h"
#include "csutil/scf_implementation.h"
#include "imesh/genmesh.h"
#include "imesh/gmeshskel2.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/virtclk.h"
#include "imesh/object.h"
#include "imesh/skeleton.h"
#include <iengine/scenenode.h>
#include <iengine/movable.h>

class csSkeleton;
class csSkeletonFactory;
class csSkeletonBoneFactory;
class csSkeletonGraveyard;
class csSkeletonSocketFactory;

class csSkeletonBone : public csRefCount
{
private:
  csString name;
  csSkeleton *skeleton;
  csSkeletonBoneFactory *factory_bone;
  csSkeletonBone* parent;
  csArray<csSkeletonBone *> bones;
  csReversibleTransform transform;
  csReversibleTransform full_transform;
  csQuaternion rot_quat;
  csBox3 skin_box;
public:
  csQuaternion &GetQuaternion () { return rot_quat; };
  void AddBone (csSkeletonBone *bone) { bones.Push (bone); }
  csArray<csSkeletonBone *> & GetBones () { return bones; }

  // Compute the transforms
  void UpdateTransform();
  // Apply the transforms
  void UpdateBones ();

  //------------------------------------------------------------------------

  csSkeletonBone (csSkeleton *skeleton, csSkeletonBoneFactory *factory_bone);
  virtual ~csSkeletonBone ();

  virtual const char* GetName () const { return name.GetData(); }
  virtual void SetName (const char* name) { csSkeletonBone::name = name; }
  virtual csReversibleTransform &GetTransform () { return transform; }
  virtual void SetTransform (const csReversibleTransform &transform)
  {
    csSkeletonBone::transform = transform;
    rot_quat.SetMatrix (transform.GetO2T());
  }
  virtual csReversibleTransform &GetFullTransform () 
    { return full_transform; }
  virtual void SetParent (csSkeletonBone* par);
  virtual csSkeletonBone* GetParent () { return parent; }
  virtual size_t GetChildrenCount () { return bones.GetSize () ;}
  virtual csSkeletonBone *GetChild (size_t i) { return bones[i]; }
  virtual csSkeletonBone *FindChild (const char *name);

  virtual csSkeletonBoneFactory *GetFactory();
  virtual size_t FindChildIndex (csSkeletonBone *child);
  virtual void SetSkinBox (csBox3 & box) { skin_box = box; }
  virtual csBox3 & GetSkinBox () { return skin_box; }
};

//----------------------------------------- csSkeletonSocket ------------------------------------------

class csSkeletonSocket :
  public scfImplementation1<csSkeletonSocket, iSkeletonSocket>
{
private:
  csString name;
  csReversibleTransform transform;
  csReversibleTransform full_transform;
  csSkeletonBone *bone;
  iSceneNode *node;
  csSkeletonSocketFactory *factory;
public:
  csSkeletonSocket (csSkeleton *skeleton, csSkeletonSocketFactory *socket_factory);
  virtual ~csSkeletonSocket ();
  virtual const char* GetName () const
  { return name; }
  virtual void SetName (const char* name)
  { csSkeletonSocket::name = name; }
  virtual csReversibleTransform &GetTransform ()
  { return transform; }
  virtual void SetTransform (const csReversibleTransform &transform)
  { csSkeletonSocket::transform = transform; }
  virtual csReversibleTransform &GetFullTransform ()
  { return full_transform; }
  virtual void SetBone (csSkeletonBone *bone)
  { csSkeletonSocket::bone = bone; }
  virtual csSkeletonBone *GetBone ()
  { return bone; }
    virtual void SetSceneNode (iSceneNode *node)
  { csSkeletonSocket::node = node; }
    virtual iSceneNode *GetSceneNode ()
  { return node; }
    virtual iSkeletonSocketFactory *GetFactory ();
};


//----------------------------- csSkeletonBoneFactory -------------------------------
class csSkeletonBoneFactory : public csRefCount
{
private:
  csString name;
  csSkeletonFactory *skeleton_factory;
  csSkeletonBoneFactory* parent;
  csArray<csSkeletonBoneFactory *> bones;
  csReversibleTransform transform;
  csReversibleTransform full_transform;
  csBox3 skin_box;
public:
  void UpdateBones();

  void AddBone (csSkeletonBoneFactory* bone) { bones.Push (bone); }
  csArray<csSkeletonBoneFactory *>& GetBones () { return bones; }

  //------------------------------------------------------------------------

  csSkeletonBoneFactory (csSkeletonFactory *);
  virtual ~csSkeletonBoneFactory ();

  virtual const char* GetName () const { return name.GetData(); }
  virtual void SetName (const char* name){ csSkeletonBoneFactory::name = name; }
  virtual csReversibleTransform &GetTransform () { return transform; }
  virtual void SetTransform (const csReversibleTransform &transform) 
  { csSkeletonBoneFactory::transform = transform; }
  virtual csReversibleTransform &GetFullTransform () { return full_transform; }
  virtual  void SetParent (csSkeletonBoneFactory *par);
  virtual csSkeletonBoneFactory* GetParent () { return parent; }
  virtual size_t GetChildrenCount () { return bones.GetSize (); }
  virtual csSkeletonBoneFactory *GetChild (size_t i) { return bones[i]; }
  virtual csSkeletonBoneFactory *FindChild (const char *name);
  virtual size_t FindChildIndex (csSkeletonBoneFactory *child);
  virtual void SetSkinBox (csBox3 & box) { skin_box = box; }
  virtual csBox3 & GetSkinBox () { return skin_box; }
};

struct bone_key_info
{
  bool relative;
  csQuaternion rot;
  csVector3 pos;
  csQuaternion tangent;
  csSkeletonBoneFactory *bone;
};

class csSkeletonAnimationKeyFrame :
  public scfImplementation1<csSkeletonAnimationKeyFrame, iSkeletonAnimationKeyFrame>
{
public:
    typedef csHash<bone_key_info, csPtrKey<csSkeletonBoneFactory> > 
      BoneKeyHash;
private:
  csString name;
  csTicks duration;
  //csArray<bone_key_info> bones_frame_transforms;
  //csArray<bone_key_info> bones_frame_transforms;
  BoneKeyHash bones_frame_transforms;
public:
  bone_key_info &GetKeyInfo(csSkeletonBoneFactory *bone_fact)
  {
    bone_key_info fallback;
    return bones_frame_transforms.Get(bone_fact, fallback);
  }

  csSkeletonAnimationKeyFrame (const char* name);
  virtual ~csSkeletonAnimationKeyFrame ();

  virtual const char* GetName () const { return name; }
  virtual void SetName (const char* name)
    { csSkeletonAnimationKeyFrame::name = name; }
  virtual csTicks GetDuration () { return duration; }
  virtual void SetDuration (csTicks time) { duration = time; }
  virtual size_t GetTransformsCount() 
    { return bones_frame_transforms.GetSize(); }

  virtual void AddTransform(csSkeletonBoneFactory *bone, 
    csReversibleTransform &transform, bool relative)
  {
    bone_key_info bf;
    csQuaternion q;
    q.SetMatrix(transform.GetO2T());
    bf.rot = q;
    bf.pos = transform.GetOrigin();
    bf.bone = bone;
    bf.relative = relative;
    bones_frame_transforms.Put(bone, bf);
  }

  bool GetTransform (csSkeletonBoneFactory *bone_fact, csReversibleTransform &dst_trans) 
  {
    bone_key_info fallback;
    fallback.bone = 0;
    const bone_key_info & bki = bones_frame_transforms.Get (bone_fact, fallback);
    if (bki.bone == 0)
      return false;

    dst_trans = csReversibleTransform (csMatrix3 (bki.rot), bki.pos);

    return true;
  }

  virtual void SetTransform(csSkeletonBoneFactory *bone_fact,
    csReversibleTransform &transform)
  {
    bone_key_info fallback;
    fallback.bone = 0;
    bone_key_info &bki = bones_frame_transforms.Get (bone_fact, fallback);
    //transform.GetO2T ()
    bki.rot.SetMatrix (transform.GetO2T ());
    bki.pos = transform.GetOrigin ();
  }

  virtual bool GetKeyFrameData(csSkeletonBoneFactory *bone_fact, 
      csQuaternion & rot, csVector3 & pos, csQuaternion & tangent,
      bool & relative)
  {
    bone_key_info fallback;
    fallback.bone = 0;
    const bone_key_info & bki = bones_frame_transforms.Get(bone_fact, fallback);
    if (bki.bone == 0)
      return false;

    rot = bki.rot;
    pos = bki.pos;
    tangent = bki.tangent;
    relative = bki.relative;
    return true;
  }
};

class csSkeletonAnimation :
  public scfImplementation1<csSkeletonAnimation, iSkeletonAnimation>
{

private:

  csString name;
  float time_factor;
  bool loop;
  int loop_times;
  csRefArray<csSkeletonAnimationKeyFrame> key_frames;
  csSkeletonFactory *fact;

public:

  csSkeletonAnimation (csSkeletonFactory *factory, const char* name);
  virtual ~csSkeletonAnimation ();

  csTicks GetFramesTime ();

  //void SetLoop (bool loop) { csSkeletonAnimation::loop = loop; }
  //bool GetLoop () { return loop; }
  void SetLoopTimes (bool loop_times) 
    { csSkeletonAnimation::loop_times = loop_times; }
  int GetLoopTimes () { return loop_times; }


  virtual const char* GetName () const { return name; }
  virtual void SetName (const char* name){ csSkeletonAnimation::name = name; }
  virtual csTicks GetTime ();
  virtual void SetTime (csTicks time);
  virtual float GetSpeed () { return time_factor; }
  virtual void SetSpeed (float speed)  {time_factor = speed;}
  virtual void SetFactor (float) {}
  virtual float GetFactor () { return 1; }
  virtual void SetLoop (bool loop)
  { csSkeletonAnimation::loop = loop; }
  virtual bool GetLoop ()
  { return loop; }

  virtual iSkeletonAnimationKeyFrame *CreateFrame(const char* name);
  virtual size_t GetFramesCount()  { return key_frames.GetSize (); }
  virtual iSkeletonAnimationKeyFrame *GetFrame(size_t i)  { return key_frames[i]; }
  virtual size_t FindFrameIndex(const char * /*name*/)  { return 0; }
  virtual void RemoveFrame(size_t i) 
  { key_frames.DeleteIndexFast(i); }
  void RemoveAllFrames () 
  { key_frames.DeleteAll (); }
  virtual void RecalcSpline();
};

struct bone_transform_data
{
  csQuaternion quat;
  csQuaternion tangent;
  csVector3 pos;
  csVector3 axis;
  csReversibleTransform transform;
};

struct sac_transform_execution
{
  csSkeletonBone* bone;
  bone_transform_data* bone_transform;
  csVector3 delta_per_tick;
  csVector3 final_position;
  csVector3 position;
  csQuaternion quat;
  csQuaternion tangent;
  csQuaternion curr_quat;
  csTicks elapsed_ticks;
};

class csSkeletonAnimationInstance :
  public scfImplementation1<csSkeletonAnimationInstance, iSkeletonAnimationInstance>
{

public:
  typedef csHash<bone_transform_data*, csPtrKey<csSkeletonBoneFactory> > 
    TransformHash;

private:

  csSkeleton *skeleton;
  csSkeletonAnimation *animation;
  size_t current_instruction;
  int current_frame;
  float blend_factor;
  float time_factor;
  csQuaternion zero_quat;
  int loop_times;

  csArray<sac_transform_execution> runnable_transforms;

  struct runnable_frame
  {
    bool active;
    int repeat_times;
  };

  csArray<runnable_frame> runnable_frames;

  enum
  {
    CS_ANIM_STATE_CURRENT,
    CS_ANIM_STATE_PARSE_NEXT,
    CS_ANIM_STATE_PARSE_PREV
  }anim_state;

  csTicks current_frame_time;
  csTicks current_frame_duration;

  TransformHash transforms;

  size_t id;

  void release_tranform_data(TransformHash&);

  void ParseFrame(csSkeletonAnimationKeyFrame *frame);
  csSkeletonAnimationKeyFrame *NextFrame();
  csSkeletonAnimationKeyFrame *PrevFrame();

public:

  csSkeletonAnimation *GetScript() { return animation; }

  bool Do (long elapsed, bool& stop, long &left);

  bone_transform_data *GetBoneTransform(csSkeletonBoneFactory *bone);
  TransformHash& GetTransforms() { return transforms; };

  csSkeletonAnimationInstance (csSkeletonAnimation* script, csSkeleton *skeleton);
  ~csSkeletonAnimationInstance ();

  const char *GetName () const { return animation->GetName (); }
  float GetFactor () { return blend_factor; }
  void SetFactor (float factor) { blend_factor = factor; }
  csTicks GetDuration () { return (csTicks) (animation->GetTime () * time_factor); }
  void SetDuration (csTicks time);
  float GetSpeed () {return time_factor; }
  void SetSpeed (float speed) {time_factor = speed;}
  size_t GetID ();
};

#include "csutil/win32/msvc_deprecated_warn_off.h"

class csSkeleton :
  public scfImplementation1<csSkeleton, iSkeleton>
{
private:
  csString name;
  iObjectRegistry* object_reg;
  csSkeletonFactory *factory;

  csRefArray<csSkeletonAnimationInstance> running_animations;
  csArray<csString> pending_scripts;

  long last_update_time;
  uint32 last_version_id;
  long elapsed;

  static csArray<csReversibleTransform> bone_transforms;
  csRefArray<csSkeletonBone> bones;
  csRefArray<csSkeletonSocket> sockets;
  csArray<size_t> parent_bones;

  csRef<iSkeletonAnimationCallback> script_callback;

  csRefArray<iSkeletonUpdateCallback> update_callbacks;

  //iODEDynamicSystem *dynamic_system;

  bool bones_updated;
  bool force_bone_update;
  void UpdateBones ();
  void UpdateSockets ();

public:

  iSkeletonAnimationCallback *GetScriptCallback() 
  { return script_callback; }

  bool UpdateAnimation (csTicks current);

  csRefArray<csSkeletonBone>& GetBones () 
  { return bones; }
  csArray<size_t>& GetParentBones () 
  { return parent_bones; }
  csRefArray<csSkeletonAnimationInstance> & GetRunningScripts () 
  { return running_animations; }

  bool IsInInitialState () {return last_update_time == -1;}

  void SetForceUpdate(bool force_update)
  { force_bone_update = force_update; }

  csSkeleton (csSkeletonFactory* fact);
  virtual ~csSkeleton ();

  virtual csReversibleTransform GetBoneOffsetTransform (size_t i)
  {
    return bones[i]->GetFactory()->GetFullTransform().GetInverse() *
      bones[i]->GetFullTransform();
  }

  virtual const char* GetName () const { return name; }
  virtual void SetName (const char* name) 
    { csSkeleton::name = name; }
  virtual size_t GetBonesCount () { return bones.GetSize (); }
  virtual csSkeletonBone *GetBone (size_t i) { return bones[i]; }
  virtual csSkeletonBone *FindBone (const char *name);

  virtual iSkeletonAnimation* Execute (const char *scriptname, float blend_factor = 0.0f);
  virtual iSkeletonAnimation* Append (const char *scriptname);
  virtual void ClearPendingAnimations ()
  { pending_scripts.DeleteAll(); }
  virtual size_t GetAnimationsCount () { return running_animations.GetSize (); }
  virtual iSkeletonAnimation* GetAnimation (size_t i);
  virtual iSkeletonAnimation* FindAnimation (const char *scriptname);
  virtual void StopAll ();
  virtual void Stop (const char* scriptname);
  virtual void Stop (iSkeletonAnimation *script);
  virtual size_t FindBoneIndex (const char* bonename);
  virtual iSkeletonFactory *GetFactory();
  virtual void SetAnimationCallback (iSkeletonAnimationCallback *cb)
  { script_callback = cb; }
  virtual iSkeletonSocket* FindSocket (const char *socketname);
    //virtual void CreateRagdoll(iODEDynamicSystem *dyn_sys, csReversibleTransform & transform);
  //virtual void DestroyRagdoll();

  iSkeletonAnimationInstance *Play (const char *animation_name);
  void Stop (iSkeletonAnimationInstance *anim_instance);

  virtual size_t AddUpdateCallback(iSkeletonUpdateCallback *update_callback)
  { return update_callbacks.Push(update_callback); }
    virtual size_t GetUpdateCallbacksCount()
  { return update_callbacks.GetSize (); }
  virtual iSkeletonUpdateCallback *GetUpdateCallback(size_t callback_idx)
  { return update_callbacks[callback_idx]; }
  virtual void RemoveUpdateCallback(size_t callback_idx)
  { update_callbacks.DeleteIndexFast(callback_idx); }
};

class csSkeletonFactory :
  public scfImplementation1<csSkeletonFactory, iSkeletonFactory>
{
private:
  csString name;
  csSkeletonGraveyard* graveyard;
  iObjectRegistry* object_reg;
  csStringArray autorun_scripts;

  csRefArray<csSkeletonBoneFactory> bones;
  csRefArray<csSkeletonSocketFactory> sockets;
  csArray<size_t> parent_bones;
  csRefArray<csSkeletonAnimation> scripts;

public:

  csSkeletonFactory (csSkeletonGraveyard* graveyard,
    iObjectRegistry* object_reg);
  virtual ~csSkeletonFactory ();

  virtual csSkeletonBoneFactory *CreateBone(const char *name);
  virtual const char* GetName () const { return name; }
  virtual void SetName (const char* name) 
    { csSkeletonFactory::name = name; }
  size_t FindBoneIndex (csSkeletonBoneFactory *bone) const;

  void UpdateParentBones ();

  csRefArray<csSkeletonBoneFactory>& GetBones () { return bones; }
  csRefArray<csSkeletonSocketFactory>& GetSockets () { return sockets; }
  csArray<size_t>& GetParentBones () { return parent_bones; }

  virtual size_t GetBonesCount() const
    { return bones.GetSize (); }
  virtual csSkeletonBoneFactory * GetBone(size_t i)
    { return bones[i]; }
  virtual size_t FindBoneIndex (const char* bonename);
  virtual csSkeletonBoneFactory *FindBone (const char *name);

  virtual iSkeletonGraveyard *GetGraveyard  ();


  size_t GetAnimationsCount () {return scripts.GetSize ();}
  iSkeletonAnimation *GetAnimation (size_t idx) {return scripts[idx];}
  virtual iSkeletonAnimation *CreateAnimation (const char *name);
  virtual iSkeletonAnimation *FindAnimation (const char *name);

  virtual iSkeletonSocketFactory *CreateSocket(const char *name, csSkeletonBoneFactory *bone);
  virtual iSkeletonSocketFactory *FindSocket(const char *name);
  virtual iSkeletonSocketFactory *GetSocket (int i);
  virtual void RemoveSocket (int i);
  virtual size_t GetSocketsCount();
};

#include "csutil/win32/msvc_deprecated_warn_on.h"

class csSkeletonSocketFactory :
  public scfImplementation1<csSkeletonSocketFactory, iSkeletonSocketFactory>
{
private:
  csString name;
  csReversibleTransform transform;
  csReversibleTransform full_transform;
  csSkeletonBoneFactory *bone;
public:
  csSkeletonSocketFactory (const char *name, csSkeletonBoneFactory *bone);
  virtual ~csSkeletonSocketFactory ();
  virtual const char* GetName () const
  { return name; }
  virtual void SetName (const char* name)
  { csSkeletonSocketFactory::name = name; }
  virtual csReversibleTransform &GetTransform ()
  { return transform; }
  virtual void SetTransform (const csReversibleTransform &transform)
  { csSkeletonSocketFactory::transform = transform; }
  virtual csReversibleTransform &GetFullTransform ()
  { return full_transform; }
  virtual void SetBone (csSkeletonBoneFactory *bone)
  { csSkeletonSocketFactory::bone = bone; }
  virtual csSkeletonBoneFactory *GetBone ()
  { return bone; }
};

class csSkeletonGraveyard :
  public scfImplementation2<csSkeletonGraveyard,
    iSkeletonGraveyard, iComponent>
{
private:
  iObjectRegistry* object_reg;
  csRef<iVirtualClock> vc;
  csRefArray<csSkeletonFactory> factories;
  csRefArray<iSkeleton> skeletons;
  csEventID PreProcess;
  bool manual_updates;
public:
  csSkeletonGraveyard (iBase*);
  virtual ~csSkeletonGraveyard ();
  bool Initialize (iObjectRegistry* object_reg);
  bool HandleEvent (iEvent& ev);

  virtual size_t GetFactoriesCount() { return factories.GetSize (); }
  virtual iSkeletonFactory *CreateFactory(const char *name);
  virtual iSkeletonFactory *LoadFactory(const char * /*file_name*/) { return 0; }
  virtual iSkeletonFactory *FindFactory(const char * /*name*/) { return 0; }
  virtual iSkeleton *CreateSkeleton(iSkeletonFactory *fact, const char *name);
  virtual void SetManualUpdates (bool man_updates) {manual_updates = man_updates;}
  virtual void Update (csTicks time);
  void AddSkeleton (iSkeleton *skeleton);
  void RemoveSkeleton (iSkeleton* skeleton);

  class csSkelEventHandler : public scfImplementation1<csSkelEventHandler,
  	iEventHandler>
  {
  private:
    csSkeletonGraveyard* parent;

  public:
    csSkelEventHandler (csSkeletonGraveyard* parent)
      : scfImplementationType (this), parent (parent) { }
    virtual ~csSkelEventHandler () { }
    virtual bool HandleEvent (iEvent& ev)
    {
      return parent->HandleEvent (ev);
    }

    CS_EVENTHANDLER_NAMES("crystalspace.skeleton.graveyard")
    CS_EVENTHANDLER_NIL_CONSTRAINTS
  };
  csRef<csSkelEventHandler> evhandler;
};

#endif // __CS_SKELETON_H__

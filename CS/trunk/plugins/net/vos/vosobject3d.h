#ifndef _CSVOSOBJECT3D_H_
#define _CSVOSOBJECT3D_H_

#include <vos/metaobjects/a3dl/object3d.hh>
#include "inetwork/vosa3dl.h"
#include "iengine/mesh.h"
#include "csvosa3dl.h"
#include "vossector.h"

class csVosObject3D : public iVosObject3D
{
private:
    csRef<iMeshWrapper> meshwrapper;

public:
    SCF_DECLARE_IBASE;

    csVosObject3D();
    virtual ~csVosObject3D();

    virtual csRef<iMeshWrapper> GetMeshWrapper();

    void SetMeshWrapper(iMeshWrapper* mw);
};

class csMetaObject3D : public virtual A3DL::Object3D
{
protected:
    csVosObject3D* csvobj3d;
public:
    csMetaObject3D(VOS::VobjectBase* superobject);
    virtual ~csMetaObject3D();

    static VOS::MetaObject* new_csMetaObject3D(VOS::VobjectBase* superobject, const std::string& type);

    virtual void setup(csVosA3DL* vosa3dl, csVosSector* sect);
    csRef<csVosObject3D> getCSinterface();
};

#endif

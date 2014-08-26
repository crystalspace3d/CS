#ifndef __VORONOI_FRAC_H__
#define __VORONOI_FRAC_H__


#include<crystalspace.h>
#include<set>
#include "cstool\csapplicationframework.h"
#include "ivaria\convexdecompose.h"

//Should this class inherit from standard Crystalspace classes or should it be standalone?

class VoronoiFrac //: public csApplicationFramework, public csBaseEventHandler 
{
public:

	VoronoiFrac();
	~VoronoiFrac();

	//Functions to set up HACD plugin for use within this class
	
//	bool Initializer();
//	bool SetupHACD();
	
	void getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut);

	void voronoiBBoxFrac(const csArray<csVector3> &points,const csVector3 &bboxMin,const csVector3 &bboxMax,csRef<iMeshWrapper> &mesh);

	csArray<csVector3> potentialVertexList;

//	csRef<iConvexDecomposer> CHull;

	csArray<csRef<csTriangleMesh> > convexShards;
	
};

#endif // __VORONOI_FRAC_H__
#ifndef __VORONOI_FRAC_H__
#define __VORONOI_FRAC_H__


#include<crystalspace.h>
#include<set>
#include<vector>
#include "cstool\csapplicationframework.h"
#include "ivaria\convexdecompose.h"


class VoronoiFrac 
{
public:

	VoronoiFrac();
	~VoronoiFrac();

	csArray<csRef<csTriangleMesh> > GetShards(){ return convexShards; };
	void getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut);

	void voronoiBBoxFrac(const csArray<csVector3> &points,const csVector3 &bboxMin,const csVector3 &bboxMax,csRef<iMeshWrapper> &mesh);

private:

	csArray<csRef<csTriangleMesh> > convexShards;

};

#endif // __VORONOI_FRAC_H__
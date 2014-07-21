#ifndef __VORONOI_FRAC_H__
#define __VORONOI_FRAC_H__


#include<crystalspace.h>
#include<set>
#include<vector>
#include "G:\Programs\CS\include\cstool\csapplicationframework.h"

class VoronoiFrac : public csApplicationFramework
{
public:

	VoronoiFrac();
	~VoronoiFrac();

	void getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut);

	void voronoiBBoxFrac(const csArray<csVector3> &points,const csVector3 &bboxMin,const csVector3 &bboxMax,csRef<iMeshWrapper> &mesh);

};

#endif // __VORONOI_FRAC_H__
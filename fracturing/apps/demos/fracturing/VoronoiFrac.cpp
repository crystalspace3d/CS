#include "VoronoiFrac.h"


static csVector3 currentVoronoiPoint;

VoronoiFrac::VoronoiFrac()
{}

VoronoiFrac::~VoronoiFrac()
{}

struct pointCmp
{
	bool operator()(const csVector3 &p1, const csVector3 &p2)
	{
		float v1 = (p1 - currentVoronoiPoint).Norm();
		float v2 = (p2 - currentVoronoiPoint).Norm();
		bool result0 = v1 < v2;
		return result0;
	}
};


void VoronoiFrac::getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut)
{
	verticesOut.SetSize(0);
	planeIndicesOut.clear();
	const int numPlanes = planes.GetSize();
	int i, j, k, l;		//indices we will use for planes
	for (i = 0; i < numPlanes; i++)
	{
		const csPlane3 &N1 = planes[i];
		for (j = i + 1; j < numPlanes; j++)
		{
			const csPlane3 &N2 = planes[j];
			
			csVector3 n1(N1.A(), N1.B(), N1.C());
			csVector3 n2(N2.A(),N2.B(),N2.C());
			csVector3 n1n2 = n1%n2;

			if (n1n2.Norm() > 0.0001)
			{
				for (k = j + 1; k < numPlanes; k++)
				{
					const csPlane3 &N3 = planes[k];
					csVector3 n3(N3.A(),N3.B(),N3.C());

					csVector3 n2n3 = n2%n3;
					csVector3 n3n1 = n3%n1;

					if (n2n3.Norm()>0.0001 && n3n1.Norm()>0.0001)
					{
						float quotient = n1*n2n3;
						if (quotient > 0.0001)
						{
							csVector3 potentialVertex(n2n3*N1.DD + n3n1*N2.DD + n1n2*N3.DD);
							potentialVertex*=(float)(-1 / quotient);

							for (l = 0; l < numPlanes; l++)
							{
								const csPlane3 & NP = planes[l];
								const csVector3 np(NP.A(),NP.B(),NP.C());

								if (((np*potentialVertex) + NP.DD) >0.0001)
									break;
							}
							if(l == numPlanes)
							{
								verticesOut.Push(potentialVertex);
								planeIndicesOut.insert(i);
								planeIndicesOut.insert(j);
								planeIndicesOut.insert(k);
							}
						}
					}
				}
			}

		}
	}
}

void VoronoiFrac::voronoiBBoxFrac(const csArray<csVector3> &points, const csVector3 &bboxMin, const csVector3 &bboxMax,csRef<iMeshWrapper> &mesh)
{
	csArray<csVector3> vertices;
	float nlength, maxDistance, distance;
	csArray<csVector3> sortedVoronoiPoints;
	sortedVoronoiPoints = points;
	csPlane3 plane;
	csVector3 normal,normalised;
	csArray<csPlane3> planes;
	std::set<int> planeIndices;
	std::set<int>::iterator planeIndicesIter;
	int numPlaneIndices;
	int cellnum = 0;
	int i, j, k;

	/*

	//iStringSet crap
	csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
	csStringID baseID = strings->Request("base");
	csStringID collisionID = strings->Request("colldet");


	csTriangle * meshTriangles = mesh->GetMeshObject()->GetObjectModel()->GetTriangleData(baseID)->GetTriangles();
	csVector3 * meshVertices = mesh->GetMeshObject()->GetObjectModel()->GetTriangleData(baseID)->GetVertices();

	csTriangle *temp = &meshTriangles[0];
	csVector3 a = temp->a;
	csVector3 b = temp->b;
	csVector3 c = temp->c;

	csVector3 M = b - a;
	csVector3 N = c - b;
	csVector3 normal = M%N;		//this gives us normal to the plane

	*/


	int numPoints = points.GetSize();
	for (i = 0; i < numPoints; i++)
	{
		currentVoronoiPoint = points[i];
		planes.SetSize(6);
		planes[0].Set(csVector3(1, 0, 0), bboxMin.x);
		planes[1].Set(csVector3(0, 1, 0), bboxMin.y);
		planes[2].Set(csVector3(0, 0, 1), bboxMin.z);
		planes[3].Set(csVector3(1, 0, 0), bboxMax.x);
		planes[4].Set(csVector3(0, 1, 0), bboxMax.y);
		planes[5].Set(csVector3(0, 0, 1), bboxMax.z);

		maxDistance = INFINITE;
		sortedVoronoiPoints.Sort(pointCmp());

		for (j = 1; j < numPoints; j++)
		{
			normal = sortedVoronoiPoints[j] - currentVoronoiPoint;
			nlength = normal.Norm();
			if (nlength>maxDistance)
				break;
			normal.Normalize();

			plane.Set(normal,(float)(-nlength/2.0f));

			planes.Push(plane);
			getVerticesInsidePlanes(planes,vertices,planeIndices);

			if (vertices.GetSize() == 0)
				break;
			numPlaneIndices = planeIndices.size();

			if (numPlaneIndices != planes.GetSize())
			{
				planeIndicesIter = planeIndices.begin();
				for (k = 0; k < numPlaneIndices; k++)
				{
					if (k != *planeIndicesIter)
						planes[k] = planes[*planeIndicesIter];
					planeIndicesIter++;
				}
				planes.SetSize(numPlaneIndices);

			}

			maxDistance = vertices[0].Norm();

			for (k = 1; k < vertices.GetSize(); k++)
			{
				distance = vertices[k].Norm();
				if (maxDistance < distance)
					maxDistance = distance;
			}
			
			maxDistance *= 2.0;
		}

		if (vertices.GetSize() == 0)
			continue;

		/*
		The convex hull code should go here below. commit at earliest.
		Also, there is some trouble producing 'jam' files, which will be uploaded at the earliest.
		*/
	}
}
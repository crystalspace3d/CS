#include "VoronoiFrac.h"
using namespace CS::Collisions;


static csVector3 currentVoronoiPoint;
csArray<csVector3> potentialVertexList;

VoronoiFrac::VoronoiFrac()
{}

VoronoiFrac::~VoronoiFrac()
{}

struct pointCmp
{
	bool operator()(const csVector3 &p1, const csVector3 &p2)
	{
		float v1 = (p1 - currentVoronoiPoint).SquaredNorm();
		float v2 = (p2 - currentVoronoiPoint).SquaredNorm();
		bool result0 = v1 < v2;
		return result0;
	}
};


void VoronoiFrac::getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut)
{
	potentialVertexList.SetSize(0);
	verticesOut.SetSize(0);
	planeIndicesOut.clear();
	const int numPlanes = planes.GetSize();
	//indices we will use for planes
	int i, j, k, l;	
	//for each plane in the list of sorted planes(by their distances)
	for (i = 0; i < numPlanes; i++)
	{
		//pick a plane
		const csPlane3 &N1 = planes.Get(i);
		//from the 'next-closest' plane onwards
		for (j = i + 1; j < numPlanes; j++)
		{
			const csPlane3 &N2 = planes.Get(j);	//get the 2nd plane
			
			csVector3 n1(N1.A(), N1.B(), N1.C());
			csVector3 n2(N2.A(),N2.B(),N2.C());
			csVector3 n1n2 = n1%n2;			//cross these 2 planes to get line in common with
			//both planes

			if (n1n2.Norm() > 0.000001)		//If they are NOT parallel
			{
				for (k = j + 1; k < numPlanes; k++)		//from the next closest plane onwards..
				{
					const csPlane3 &N3 = planes.Get(k);		//pick a plane
					csVector3 n3(N3.A(),N3.B(),N3.C());

					csVector3 n2n3 = n2%n3;			//cross it with the first plane
					csVector3 n3n1 = n3%n1;			//with 2nd plane too


					// if (N2 and N3) and (N1 and N3) are both NOT parallel i.e if
					// lines common with (N2 and N3) and (N1 and N3) are NOT parallel...
					if (n2n3.Norm()>0.000001 && n3n1.Norm()>0.000001)
					{
						float quotient = n1*n2n3;
						
						//suppose N1 and n2n3 are not parallel...
						if (quotient > 0.000001)
						{
							//vectors are re-scaled, then added to form a new vector
							csVector3 potentialVertex(n2n3*(N1.D())+ n3n1*(N2.D()) + n1n2*N3.D());
							potentialVertex*=(float)(1 / quotient);
							potentialVertexList.Push(potentialVertex);

							//for all planes in vector...
							for (l = 0; l < numPlanes; l++)
							{
								const csPlane3 & NP = planes.Get(l);
								const csVector3 np(NP.A(),NP.B(),NP.C());
								float temp = np*potentialVertex , D = NP.DD ;
								//if this 'potentialVertex' lies outside all the planes,
								//then reject this point
								if ((abs(temp) - abs(D)) > 0)
									break;
							}

							//else if the point lies "inside" all the planes then accept it.
							if(l == numPlanes)
							{
								// vertex (three plane intersection) inside all planes
								verticesOut.Push(potentialVertex);
								planeIndicesOut.insert(i);
								planeIndicesOut.insert(j);
								planeIndicesOut.insert(k);
								//these 3 planes(i.e their indices) are the planes whose 
								//intersection yields the right 'potentialVertex'
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

	//Data declaration
	csArray<csVector3> vertices;
	float nlength=0, maxDistance=0, distance=0;
	csArray<csVector3> sortedVoronoiPoints;
	sortedVoronoiPoints = points;		//just set this to input points for now
	csPlane3 plane;
	csVector3 normal;
	csArray<csPlane3> planes;
	std::set<int> planeIndices;
	std::set<int>::iterator planeIndicesIter;
	int numPlaneIndices=0;
	int cellnum = 0;
	int i, j, k;

	int numPoints = points.GetSize();
	for (i = 0; i < numPoints; i++)		//for number of voronoi seed points
	{
		currentVoronoiPoint = points.Get(i);

		planes.SetSize(0);
		planes.Push(csPlane3(csVector3(1, 0, 0), bboxMin.x));
		planes.Push(csPlane3(csVector3(0, 1, 0), bboxMin.y));
		planes.Push(csPlane3(csVector3(0, 0, 1), bboxMin.z));
		planes.Push(csPlane3(csVector3(1, 0, 0), bboxMax.x));
		planes.Push(csPlane3(csVector3(0, 1, 0), bboxMax.y));
		planes.Push(csPlane3(csVector3(0, 0, 1), bboxMax.z));

		maxDistance = INFINITE;
		//sort all vertices by their distance from the current seed point
		sortedVoronoiPoints.Sort(pointCmp());

		for (j = 1; j < numPoints; j++)
		{
			//find normal to closest seed point
			
			csVector3 &v1 = currentVoronoiPoint;
			csVector3 &v2 = sortedVoronoiPoints.Get(j);

			normal.Set(sortedVoronoiPoints.Get(j)-currentVoronoiPoint);
			//currentVoronoiPoint - sortedVoronoiPoints.Get(j);
			csVector3 p;
			if (v2*v1 < 0)
			{
				p.Set((v2 + v1) / 2);
			}
			else
				p.Set((v2 - v1) / 2);

			float D = p*normal;

			if (D>maxDistance)
				break;

			//normalize it
			normal.Normalize();

			//create perpendicular bisector plane
			plane.Set(normal,(float)D);

			planes.Push(plane);
			//this function	calculates voronoi vertices anf planes and pushes them onto corresponding
			//vectors
			getVerticesInsidePlanes(planes,vertices,planeIndices);

			if (vertices.GetSize() == 0)
				break;

			//number of voronoi planes that do yield cell points
			numPlaneIndices = planeIndices.size();
			//this loop just resizes the 'planes' set, removing any planes that DON'T result in
			//forming cell vertices
			if (numPlaneIndices != planes.GetSize())
			{
				planeIndicesIter = planeIndices.begin();
				for (k = 0; k < numPlaneIndices; k++)
				{
					if (k != *planeIndicesIter)
						planes.Put(k, planes.Get(*planeIndicesIter));
					planeIndicesIter++;
				}
				planes.SetSize(numPlaneIndices);

			}

			//set max distance to displacement with closest voronoi vertex
			maxDistance = vertices.Get(0).Norm();

			for (k = 1; k < vertices.GetSize(); k++)
			{
				distance = vertices.Get(k).Norm();
				if (maxDistance < distance)
					maxDistance = distance;
			}
			
			maxDistance *= 2.0;
		}
		//by now we have maximum distance = distance to farthest voronoi vertex * 2
		if (vertices.GetSize() == 0)
			continue;


		//This should produce convex shards for the bounding box
		csRef<iConvexDecomposer> CHull;
//		csRef<csTriangleMesh> trimesh = CHull->ConvexHull(vertices);
//		convexShards.Push(trimesh);

	}
}
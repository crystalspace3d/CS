#include "VoronoiFrac.h"
using namespace CS::Collisions;
#include<iostream>
using namespace std;


static csVector3 currentVoronoiPoint;
csArray<csVector3> vertices;
csArray<csPlane3> planes;
std::set<int> planeIndices;
std::set<int>::iterator planeIndicesIter;

VoronoiFrac::VoronoiFrac()
{}

VoronoiFrac::~VoronoiFrac()
{}

bool VoronoiFrac::Initialize(iObjectRegistry* registry)
{
	if (!csInitializer::RequestPlugins(registry,
		CS_REQUEST_PLUGIN("crystalspace.mesh.convexdecompose.hacd", iConvexDecomposer),
		CS_REQUEST_END))
	  return false;//ReportError("Failed to initialize plugins!");

	CHull = csQueryRegistry<iConvexDecomposer>(registry);
	if (!CHull) return false;// ReportError("Failed to locate HACD plugin!");

	return true;
}

struct pointCmp
{
	bool operator()(const csVector3 &p1, const csVector3 &p2)
	{
		float v1 = (p1 - currentVoronoiPoint).SquaredNorm();
		float v2 = (p2 - currentVoronoiPoint).SquaredNorm();
		bool result0 = v1 < v2;
		return result0;
	}
}pointSorter;

struct planeCmp
{
	bool operator()(const csPlane3 &p1, const csPlane3 &p2)
	{
		return (p1.Distance(currentVoronoiPoint) < p2.Distance(currentVoronoiPoint));
	}
}planeSorter;

void VoronoiFrac::getVerticesInsidePlanes(const csArray<csPlane3> &planes, csArray<csVector3> &verticesOut, std::set<int> &planeIndicesOut)
{
	const int numPlanes = planes.GetSize();
	//indices we will use for planes
	int i, j, k, l;

//	cout << "For currentVoronoiPoint :" << currentVoronoiPoint.x<<" "<<currentVoronoiPoint.y<<" "<<currentVoronoiPoint.z << endl;

	//for each plane in the list of sorted planes(by their distances)
	for (i = 0; i < numPlanes; i++)
	{
		//pick a plane
		const csPlane3 &N1 = planes.Get(i);
		//from the 'next-closest' plane onwards
		for (j = i + 1; j < numPlanes; j++)
		{
			//get the 2nd plane
			const csPlane3 &N2 = planes.Get(j);	

			csVector3 n1(N1.A(), N1.B(), N1.C());
			csVector3 n2(N2.A(), N2.B(), N2.C());

			//cross these 2 planes to get line in common with both planes
			csVector3 n1n2(n1%n2);			

			//If they are NOT parallel
			if (n1n2.SquaredNorm() > 0)		
			{
				//from the next closest plane onwards..
				for (k = j + 1; k < numPlanes; k++)		
				{
					//pick a plane
					const csPlane3 &N3 = planes.Get(k);		
					csVector3 n3(N3.A(), N3.B(), N3.C());

					//cross it with the first plane
					csVector3 n2n3(n2%n3);			

					//with 2nd plane too
					csVector3 n3n1(n3%n1);			


					// if (N2 and N3) and (N1 and N3) are both NOT parallel i.e if
					// lines common with (N2 and N3) and (N1 and N3) are NOT parallel...
					if ((n2n3.SquaredNorm() > 0) && (n3n1.SquaredNorm() > 0))
					{
						float quotient = n1*n2n3;

						//suppose N1 and n2n3 are not parallel...
						if (quotient !=0 )
						{
							//vectors are re-scaled, then added to form a new vector
							csVector3 potentialVertex((n2n3*(N1.D())) + (n3n1*(N2.D())) + (n1n2*N3.D()));
							potentialVertex *= (float)(-1 / quotient);
							potentialVertexList.Push(potentialVertex);
							
//							cout <<endl<< "potentialVertex : "<<potentialVertex.x << " " << potentialVertex.y << " " << potentialVertex.z << endl;
//							cout << "--------------------------------"<<endl;
							
							//for all planes in vector...
							for (l = 0; l < numPlanes; l++)
							{
								const csPlane3 & NP = planes.Get(l);
								const csVector3 np(NP.A(), NP.B(), NP.C());
								
								float product1 = ((np.x)*(potentialVertex.x) + (np.y)*(potentialVertex.y) + (np.z)*(potentialVertex.z) - NP.DD);

								float product2 = ((np.x)*(currentVoronoiPoint.x) + (np.y)*(currentVoronoiPoint.y) + (np.z)*(currentVoronoiPoint.z) - NP.DD);

//								cout << "product for plane : " << np.x << " " << np.y << " " << np.z << " (D: " << NP.DD<<")" << " =  " << product1*product2 << endl;

								//if signs of both product 1 and 2 are same then their product
								//should be positive and they both should lie on same side of the 
								//plane. Else neglect the point
								if ((product1*product2)< -0.04f)
								{
									break;
								}
							}

							//else if the point lies "inside" all the planes then accept it.
							if (l == numPlanes)
							{
								// vertex (three plane intersection) inside all planes
//								cout << "++++++++++++++++" << endl;
//								cout << "accepted point : " << potentialVertex.x << " " << potentialVertex.y << " " << potentialVertex.z << endl;
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

void VoronoiFrac::voronoiBBoxFrac(const csArray<csVector3> &points, const csVector3 &bboxMin, const csVector3 &bboxMax, csRef<iMeshWrapper> &mesh)
{

	//Data declaration

	float nlength = 0, maxDistance = 0, distance = 0;
	csArray<csVector3> sortedVoronoiPoints;
	sortedVoronoiPoints = points;		//just set this to input points for now
	csPlane3 plane;
	csVector3 normal;
	int numPlaneIndices = 0;
	int cellnum = 0;
	int i, j, k;

	int numPoints = points.GetSize();
	for (i = 0; i < numPoints; i++)		//for number of voronoi seed points
	{
		currentVoronoiPoint = points.Get(i);

		planes.SetSize(0);
		vertices.Empty();
		planes.Push(csPlane3(csVector3(1, 0, 0), bboxMin.x));
		planes.Push(csPlane3(csVector3(0, 1, 0), bboxMin.y));
		planes.Push(csPlane3(csVector3(0, 0, 1), bboxMin.z));
		planes.Push(csPlane3(csVector3(1, 0, 0), bboxMax.x));
		planes.Push(csPlane3(csVector3(0, 1, 0), bboxMax.y));
		planes.Push(csPlane3(csVector3(0, 0, 1), bboxMax.z));

		maxDistance = 2^32;//INFINITE;
		//sort all vertices by their distance from the current seed point
		sortedVoronoiPoints.Sort(pointSorter);

		for (j = 1; j < numPoints; j++)
		{
			//find normal to closest seed point

			csVector3 &v1 = currentVoronoiPoint;
			csVector3 &v2 = sortedVoronoiPoints.Get(j);

			normal.Set(sortedVoronoiPoints.Get(j) - currentVoronoiPoint);
			//currentVoronoiPoint - sortedVoronoiPoints.Get(j);
			csVector3 p;
			if (v2*v1 <= 0)
			{
				p.Set((v2 + v1) / 2);
			}
			else
				p.Set((v2 - v1) / 2);

			//normalize it
			normal.Normalize();

			float D = p*normal;

			if (D > maxDistance)
				break;

			//create perpendicular bisector plane
			plane.Set(normal, (float)-D);

			planes.Push(plane);
			//this function	calculates voronoi vertices anf planes and pushes them onto corresponding
			//vectors

		}

		//planes.Sort(planeSorter);

		getVerticesInsidePlanes(planes, vertices, planeIndices);

		if (vertices.GetSize() == 0)
			continue;	
	
		//This should produce convex shards for the bounding box
		csRef<iTriangleMesh> trimesh = CHull->ConvexHull(vertices);
		convexShards.Push(trimesh);
	}
}

/*
    Copyright (C) 2010 by Eduardo Poyart

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

#include <iostream>
using namespace std;

#include "csgeom.h"
#include "LodGen.h"

inline float dot(const csVector3& v0, const csVector3& v1) { return v0 * v1; }

/**
 * Returns the squared distance between point P and triangle P0, P1, P2. Squared distance is returned in d2.
 * s and t returns the closest point in parametric form in terms of edges P0P1 and P0P2.
 */
void PointTriangleDistance(const csVector3& P, const csVector3& P0, const csVector3& P1, const csVector3& P2, float& s, float& t, float& d2)
{
  // From http://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
  csVector3 diff = P0 - P;
  csVector3 edge0 = P1 - P0;
  csVector3 edge1 = P2 - P0;
  float a00 = dot(edge0, edge0);
  float a01 = dot(edge0, edge1);
  float a11 = dot(edge1, edge1);
  float b0 = dot(diff, edge0);
  float b1 = dot(diff, edge1);
  float c = dot(diff, diff);
  float det = fabs(a00*a11 - a01*a01);
  s = a01*b1 - a11*b0;
  t = a01*b0 - a00*b1;
  
  if (s + t <= det)
  {
    if (s < 0.0)
    {
      if (t < 0.0)  // region 4
      {
        if (b0 < 0.0)
        {
          t = 0.0;
          if (-b0 >= a00)
          {
            s = 1.0;
            d2 = a00 + 2.0*b0 + c;
          }
          else
          {
            s = -b0/a00;
            d2 = b0*s + c;
          }
        }
        else
        {
          s = 0.0;
          if (b1 >= 0.0)
          {
            t = 0.0;
            d2 = c;
          }
          else if (-b1 >= a11)
          {
            t = 1.0;
            d2 = a11 + 2.0*b1 + c;
          }
          else
          {
            t = -b1/a11;
            d2 = b1*t + c;
          }
        }
      }
      else  // region 3
      {
        s = 0.0;
        if (b1 >= 0.0)
        {
          t = 0.0;
          d2 = c;
        }
        else if (-b1 >= a11)
        {
          t = 1.0;
          d2 = a11 + 2.0*b1 + c;
        }
        else
        {
          t = -b1/a11;
          d2 = b1*t + c;
        }
      }
    }
    else if (t < 0.0)  // region 5
    {
      t = 0.0;
      if (b0 >= 0.0)
      {
        s = 0.0;
        d2 = c;
      }
      else if (-b0 >= a00)
      {
        s = 1.0;
        d2 = a00 + 2.0*b0 + c;
      }
      else
      {
        s = -b0/a00;
        d2 = b0*s + c;
      }
    }
    else  // region 0
    {
      float invDet = 1.0/det;
      s *= invDet;
      t *= invDet;
      d2 = s*(a00*s + a01*t + 2.0*b0) + t*(a01*s + a11*t + 2.0*b1) + c;
    }
  }
  else
  {
    if (s < 0.0)  // region 2
    {
      float tmp0 = a01 + b0;
      float tmp1 = a11 + b1;
      if (tmp1 > tmp0)
      {
        float numer = tmp1 - tmp0;
        float denom = a00 - 2.0*a01 + a11;
        if (numer >= denom)
        {
          s = 1.0;
          t = 0.0;
          d2 = a00 + 2.0*b0 + c;
        }
        else
        {
          s = numer/denom;
          t = 1.0 - s;
          d2 = s*(a00*s + a01*t + 2.0*b0) + t*(a01*s + a11*t + 2.0*b1) + c;
        }
      }
      else
      {
        s = 0.0;
        if (tmp1 <= 0.0)
        {
          t = 1.0;
          d2 = a11 + 2.0*b1 + c;
        }
        else if (b1 >= 0.0)
        {
          t = 0.0;
          d2 = c;
        }
        else
        {
          t = -b1/a11;
          d2 = b1*t + c;
        }
      }
    }
    else if (t < 0.0)  // region 6
    {
      float tmp0 = a01 + b1;
      float tmp1 = a00 + b0;
      if (tmp1 > tmp0)
      {
        float numer = tmp1 - tmp0;
        float denom = a00 - 2.0*a01 + a11;
        if (numer >= denom)
        {
          t = 1.0;
          s = 0.0;
          d2 = a11 + 2.0*b1 + c;
        }
        else
        {
          t = numer/denom;
          s = 1.0 - t;
          d2 = s*(a00*s + a01*t + 2.0*b0) + t*(a01*s + a11*t + 2.0*b1) + c;
        }
      }
      else
      {
        t = 0.0;
        if (tmp1 <= 0.0)
        {
          s = 1.0;
          d2 = a00 + 2.0*b0 + c;
        }
        else if (b0 >= 0.0)
        {
          s = 0.0;
          d2 = c;
        }
        else
        {
          s = -b0/a00;
          d2 = b0*s + c;
        }
      }
    }
    else  // region 1
    {
      float numer = a11 + b1 - a01 - b0;
      if (numer <= 0.0)
      {
        s = 0.0;
        t = 1.0;
        d2 = a11 + 2.0*b1 + c;
      }
      else
      {
        float denom = a00 - 2.0*a01 + a11;
        if (numer >= denom)
        {
          s = 1.0;
          t = 0.0;
          d2 = a00 + 2.0*b0 + c;
        }
        else
        {
          s = numer/denom;
          t = 1.0 - s;
          d2 = s*(a00*s + a01*t + 2.0*b0) + t*(a01*s + a11*t + 2.0*b1) + c;
        }
      }
    }
  }
  
  // Account for numerical round-off error
  if (d2 < 0.0)
  {
    d2 = 0.0;
  }
}
// ----------------------------------------------------------------
// Unit tests for point-triangle distance
void unittest1(const csVector3& p0, const csVector3& p1, const csVector3& p2, const csVector3& p, float expected)
{
  printf("p = %6.4g, %6.4g, %6.4g        ", p.x, p.y, p.z);
  float s, t, d2;
  PointTriangleDistance(p, p0, p1, p2, s, t, d2);
  csVector3 c(p0 + s*(p1-p0) + t*(p2-p0));
  float d = sqrtf(d2);
  printf("d = %6.4g       c = %6.4g, %6.4g, %6.4g\n", d, c.x, c.y, c.z);
  assert(expected == -1.0 || fabs(expected - d) < 0.0001);
}

void unittests(float z)
{
  csVector3 p0(1.0, 1.0, z);
  csVector3 p1(2.0, 2.0, z);
  csVector3 p2(3.0, 1.0, z);
  unittest1(p0, p1, p2, csVector3(1.0, 1.0, z), 0.0);
  unittest1(p0, p1, p2, csVector3(2.0, 2.0, z), 0.0);
  unittest1(p0, p1, p2, csVector3(3.0, 1.0, z), 0.0);
  unittest1(p0, p1, p2, csVector3(2.0, 1.5, z), 0.0);
  unittest1(p0, p1, p2, csVector3(1.0, 1.0, z + 1.0), 1.0);
  unittest1(p0, p1, p2, csVector3(2.0, 2.0, z + 1.0), 1.0);
  unittest1(p0, p1, p2, csVector3(3.0, 1.0, z + 1.0), 1.0);
  unittest1(p0, p1, p2, csVector3(2.0, 1.5, z + 1.0), 1.0);
  
  unittest1(p0, p1, p2, csVector3(0.0, 0.0, z), sqrtf(2.0)); 
  unittest1(p0, p1, p2, csVector3(0.5, 0.5, z), sqrtf(2.0) / 2.0); 
  unittest1(p0, p1, p2, csVector3(0.0, 1.0, z), 1.0); 
  unittest1(p0, p1, p2, csVector3(0.5, 1.0, z), 0.5);  
  unittest1(p0, p1, p2, csVector3(1.5, 1.5, z), 0.0); 
  unittest1(p0, p1, p2, csVector3(1.5, 2.0, z), 0.3536); 
  unittest1(p0, p1, p2, csVector3(2.0, 3.0, z), 1.0); 
  unittest1(p0, p1, p2, csVector3(2.5, 2.0, z), 0.3536); 
  unittest1(p0, p1, p2, csVector3(2.5, 1.5, z), 0.0); 
  unittest1(p0, p1, p2, csVector3(3.5, 1.0, z), 0.5);  
  unittest1(p0, p1, p2, csVector3(4.0, 1.0, z), 1.0); 
  unittest1(p0, p1, p2, csVector3(3.5, 0.5, z), sqrtf(2.0) / 2.0); 
  unittest1(p0, p1, p2, csVector3(4.0, 0.0, z), sqrtf(2.0)); 
  unittest1(p0, p1, p2, csVector3(2.5, 0.0, z), 1.0); 
  unittest1(p0, p1, p2, csVector3(2.0, 1.0, z), 0.0); 
  unittest1(p0, p1, p2, csVector3(1.5, 0.0, z), 1.0); 
}  

void PointTriangleDistanceUnitTests()
{
  unittests(0.0);
  unittests(1.0);
}

// ----------------------------------------------------------------
// LodGen

/** 
 * Initialize array of coincident vertices.
 * For each vertex, create a list of all other vertices that share the same position.
 */
void LodGen::InitCoincidentVertices()
{
  coincident_vertices.SetSize(vertices.GetSize());
  static const float epsilon = 0.00001; 
  for (unsigned int i = 0; i < vertices.GetSize(); i++)
  {
    const csVector3& v = vertices[i]; 
    for (unsigned int j = 0; j < vertices.GetSize(); j++)
    {
      if (i == j)
        continue;
      if (fabs(v[0] - vertices[j][0]) < epsilon && fabs(v[1] - vertices[j][1]) < epsilon && fabs(v[2] - vertices[j][2]) < epsilon)
        coincident_vertices[i].Push(j);
    }
  }
}

/** 
 * Error metric.
 */
float LodGen::SumOfSquareDist(const WorkMesh& k) const
{
  float s, t, d2;
  float sum = 0.0;
  const SlidingWindow& sw = k.GetLastWindow();
  // Sum all vertex-to-mesh distances, from vertices of the original mesh
  // to triangles of the LOD mesh.
  for (unsigned int i = 0; i < vertices.GetSize(); i++)
  {
    const csVector3& v = vertices[i];
    float min_d2 = FLT_MAX;
    for (int j = sw.start_index; j < sw.end_index; j++)
    {
      const csTriangle& tri = k.GetTriangle(j);
      const csVector3& p0 = vertices[tri[0]];
      const csVector3& p1 = vertices[tri[1]];
      const csVector3& p2 = vertices[tri[2]];
      PointTriangleDistance(v, p0, p1, p2, s, t, d2);
      if (d2 < min_d2)
      {
        min_d2 = d2;
        if (min_d2 == 0.0)
          break;
      }
    }
    assert(min_d2 < FLT_MAX);
    sum += min_d2;
  }
  // Sum all barycenter-to-mesh distances, from barycenters of the original mesh
  // to triangles of the LOD mesh.
  const SlidingWindow& sw0 = k.sliding_windows[0];  
  for (int i = sw0.start_index; i < sw0.end_index; i++)
  {
    const csTriangle& tri = k.GetTriangle(i);
    csVector3 b = (vertices[tri[0]] + vertices[tri[1]] + vertices[tri[2]]) / 3.0;
    float min_d2 = FLT_MAX;
    for (int j = sw.start_index; j < sw.end_index; j++)
    {
      const csTriangle& tri0 = k.GetTriangle(j);
      const csVector3& p0 = vertices[tri0[0]];
      const csVector3& p1 = vertices[tri0[1]];
      const csVector3& p2 = vertices[tri0[2]];
      PointTriangleDistance(b, p0, p1, p2, s, t, d2);
      if (d2 < min_d2)
      {
        min_d2 = d2;
        if (min_d2 == 0.0)
          break;
      }
    }
    assert(min_d2 < FLT_MAX);
    sum += min_d2;
  }
  return sum;
}

/** 
 * Quicker and less precise error metric.
 */
float LodGen::SumOfSquareDist(const WorkMesh& k, int start_index) const
{
  float s, t, d2;
  float sum = 0.0;
  const SlidingWindow& sw = k.GetLastWindow();
  const SlidingWindow& sw0 = k.sliding_windows[0];
  int samples_per_triangle = 20 / (sw.end_index - start_index);
  if (samples_per_triangle == 0)
    samples_per_triangle = 1;

  // Loop through the modified triangles, sampling a number of points inside them
  for (int i = start_index; i < sw.end_index; i++)
  {
    const csTriangle& tri = k.GetTriangle(i);
    const csVector3& q0 = vertices[tri[0]];
    const csVector3& q1 = vertices[tri[1]];
    const csVector3& q2 = vertices[tri[2]];
    for (int m = 0; m < samples_per_triangle; m++)
    {
      // Find a sample point inside the triangle with an uniform distribution
      float r0, r1;
      do
      {
        r0 = (float)rand() / RAND_MAX;
        r1 = (float)rand() / RAND_MAX;
      }
      while (r0 + r1 > 1.0);
      float r2 = 1.0 - r0 - r1;
      assert(r0 + r1 + r2 == 1.0);
      csVector3 b = r0 * q0 + r1 * q1 + r2 * q2;
      float min_d2 = FLT_MAX;
      // Find the smallest distance between the sample point and all triangles
      // of the original mesh
      for (int j = sw0.start_index; j < sw0.end_index; j++)
      {
        const csTriangle& tri0 = k.GetTriangle(j);
        const csVector3& p0 = vertices[tri0[0]];
        const csVector3& p1 = vertices[tri0[1]];
        const csVector3& p2 = vertices[tri0[2]];
        PointTriangleDistance(b, p0, p1, p2, s, t, d2);
        if (d2 < min_d2)
        {
          min_d2 = d2;
          if (min_d2 == 0.0)
            break;
        }
      }
      assert(min_d2 < FLT_MAX);
      sum += min_d2;
    }
  }
  return sum;
}

/*
// TODO
float LodGen::SumOfSquareDist(const WorkMesh& k, int start_index) const
{
  float s, t, d2;
  float sum = 0.0;
  const SlidingWindow& sw = k.GetLastWindow();
  const SlidingWindow& sw2 = k.sliding_windows[k.sliding_windows.GetSize()-2];
  const SlidingWindow& sw0 = k.sliding_windows[0];
  assert(start_index == sw2.end_index);
  csArray<csVector3> verts;
  csVector3 c(0.0, 0.0, 0.0);
  if (sw2.end_index < sw.end_index)
  {
    for (int i = sw2.end_index; i < sw.end_index; i++)
    {
      const csTriangle& tri = k.GetTriangle(i);
      for (int j = 0; j < 3; j++)
        verts.Push(vertices[tri[j]]);
    }
  }
  else
  {
    assert(sw2.start_index < sw.start_index);
    for (int i = sw2.start_index; i < sw.start_index; i++)
    {
      const csTriangle& tri = k.GetTriangle(i);
      for (int j = 0; j < 3; j++)
        verts.Push(vertices[tri[j]]);
    }
  }
  
  for (int i = 0; i < verts.GetSize(); i++)
    c += verts[i];
  c /= (float)verts.GetSize();
  float r2 = 0.0;
  for (int i = 0; i < verts.GetSize(); i++)
  {
    float r2b = (verts[i]-c).SquaredNorm();
    if (r2b > r2)
      r2 = r2b;
  }
    
  csArray<int> ntris;
  for (int i = sw0.start_index; i < sw0.end_index; i++)
  {
    const csTriangle& tri = k.GetTriangle(i);
    csVector3 b = (vertices[tri[0]] + vertices[tri[1]] + vertices[tri[2]]) / 3.0;
    if ((b-c).SquaredNorm() <= r2)
      ntris.Push(i);
  }
  
  for (int i = 0; i < ntris.
}
*/    

/**
 * Remove a triangle from the list of incident triangles of each of its 3 vertices.
 */
void LodGen::RemoveTriangleFromIncidentTris(WorkMesh& k, int itri)
{
  csTriangle& tri = k.tri_buffer[itri];
  for (int i = 0; i < 3; i++)
  {
    bool result = k.incident_tris[tri[i]].Delete(itri);
    assert(result);
  }
}

/**
 * Checks if a triangle is degenerate through identical indices.
 */
inline bool LodGen::IsDegenerate(const csTriangle& tri) const
{
  return tri[0] == tri[1] || tri[0] == tri[2] || tri[1] == tri[2];
}

/**
 * Checks if a triangle is coincident with another through comparison of their indices.
 */
bool LodGen::IsTriangleCoincident(const csTriangle& t0, const csTriangle& t1) const
{
  for (int i = 0; i < 3; i++)
    if (t0[i] != t1[0] && t0[i] != t1[1] && t0[i] != t1[2])
      return false;
  return true;
}

/**
 * Checks if a triangle is coincident with any of its neighboring triangles
 */
bool LodGen::IsCoincident(const WorkMesh& k, const csTriangle& tri) const
{
  assert(!IsDegenerate(tri));
  for (int i = 0; i < 3; i++)
  {
    const IncidentTris& incident = k.incident_tris[tri[i]];
    for (unsigned int j = 0; j < incident.GetSize(); j++)
    {
      assert(!IsDegenerate(k.tri_buffer[incident[j]]));
      if (IsTriangleCoincident(k.tri_buffer[incident[j]], tri))
        return true;
    }
  }
  return false;
}

/**
 * Finds a triangle in the sliding window and returns its position in the triangle buffer
 */
int LodGen::FindInWindow(const WorkMesh& k, const SlidingWindow& sw, int itri) const
{
  for (int i = sw.start_index; i < sw.end_index; i++)
    if (k.tri_indices[i] == itri)
      return i;
  assert(0);
}

/**
 * Swaps two triangles.
 */
void LodGen::SwapIndex(WorkMesh& k, int i0, int i1)
{
  int temp = k.tri_indices[i0];
  k.tri_indices[i0] = k.tri_indices[i1];
  k.tri_indices[i1] = temp;
}

/**
 * Perform edge collapse from v0 to v1.
 */
bool LodGen::Collapse(WorkMesh& k, int v0, int v1)
{
  SlidingWindow sw = k.GetLastWindow(); // copy
  
  // For each triangle that is incident to the vertex that will disappear (v0)
  IncidentTris incident = k.incident_tris[v0]; // copy
  for (unsigned int i = 0; i < incident.GetSize(); i++)
  {
    int itri = incident[i];
    // Make sure it's within our work limit
    // (not a triangle that was added before)
    int h = FindInWindow(k, sw, itri);
    if (h >= top_limit)
      return false;
    // Copy this triangle to a new one
    csTriangle new_tri = k.tri_buffer[itri]; // copy
    // This is a triangle that will disappear.
    // Remove from the incident lists of all its vertices
    RemoveTriangleFromIncidentTris(k, itri);
    // Make this triangle be the first in the window,
    // i.e. it will disappear on the next window shift
    SwapIndex(k, sw.start_index, h);
    //cout << "Rem " << itri << " = " << new_tri[0] << " " << new_tri[1] << " " << new_tri[2] << endl;
    // Shift the window
    sw.start_index++;

    assert(incident.GetSize() > k.incident_tris[v0].GetSize());
    // Change vertex v0 in the new triangle to become v1
    for (int j = 0; j < 3; j++)
      if (new_tri[j] == v0)
        new_tri[j] = v1;
    // If this doesn't result in a degenerate triangle, or a triangle coincident with an existing one
    // (yes, it can happen), add it to the end of the sliding window
    if (!IsDegenerate(new_tri) && !IsCoincident(k, new_tri))
    {
      k.AddTriangle(new_tri);
      //cout << "Add " << k.tri_buffer.GetSize()-1 << " = " << new_tri[0] << " " << new_tri[1] << " " << new_tri[2] << endl;
      sw.end_index++;
    }
  }
  k.sliding_windows.Push(sw);

  /*
  for (int i = sw.start_index; i < sw.end_index; i++)
    for (int j = 0; j < 3; j++)
      assert(k.GetTriangle(i)[j] != v0);
  */
  return true;
}

struct MeshVerification
{
  Edge e;
  int num_t;
  bool operator==(const MeshVerification& m) const { return e == m.e; }
};

void LodGen::VerifyMesh(WorkMesh& k)
{
  return;
  csArray<MeshVerification> mvs;
  const SlidingWindow& sw = k.GetLastWindow();
  for (int i = sw.start_index; i < sw.end_index; i++)
  {
    const csTriangle& tri = k.GetTriangle(i);
    for (int j = sw.start_index; j < sw.end_index; j++)
    {
      if (i == j)
        continue;
      const csTriangle& tri2 = k.GetTriangle(j);
      if (IsTriangleCoincident(tri, tri2))
        assert(0);
    }
  }
  /*
  for (int i = sw.start_index; i < sw.end_index; i++)
  {
    const csTriangle& tri = k.GetTriangle(i);
    for (int j = 0; j < 3; j++)
    {
      Edge e(tri[j], tri[(j+1)%3]);
      MeshVerification mv;
      mv.e = e;
      size_t i = mvs.Find(mv);
      if (i == csArrayItemNotFound)
      {
        mv.num_t = 1;
        mvs.Push(mv);
      }
      else
      {
        mvs[i].num_t++;
      }
    }
  }
  for (unsigned int i = 0; i < mvs.GetSize(); i++)
  {
    assert(mvs[i].num_t == 2);
  }
  */
}

/**
 * Main LOD generation method.
 */
void LodGen::GenerateLODs()
{
  InitCoincidentVertices();
  k.incident_tris.SetSize(vertices.GetSize());
  // Add all triangles from input array 'triangles' to work mesh 'k'.
  for (unsigned int i = 0; i < triangles.GetSize(); i++)
    k.AddTriangle(triangles[i]);
 
  // The initial window is the original mesh.
  SlidingWindow sw_initial;
  sw_initial.start_index = 0;
  sw_initial.end_index = triangles.GetSize();
  // Top limit - we never change a triangle that was added above it.
  // The top limit gets bumped up when we replicate indices.
  top_limit = sw_initial.end_index;
  k.sliding_windows.Push(sw_initial);
  int collapse_counter = 0;
  // When to absolutely end the collapses
  int min_num_triangles = triangles.GetSize() / 6;
  // When to perform a replication
  int min_triangles_for_replication = triangles.GetSize() / 2;
  // 'edges' will hold our list of edges to walk through.
  csArray<Edge> edges;
  bool could_not_collapse = false;
  int edge_start = -1;
  
  //int counter = 0;
  
  // Each loop is one collapse. In some cases, it replicates indices too.
  while (1)
  {
    //if (counter == 10)
    //  break;
    //counter++;
    float min_d = FLT_MAX;
    int min_v0, min_v1;    
    SlidingWindow sw = k.GetLastWindow();
    edges.SetSize(0);
    // Add to 'edges' all edges whose origin vertex is not coincident with another one
    for (int itri = sw.start_index; itri < top_limit; itri++)
    {
      const csTriangle& tri = k.GetTriangle(itri);
      for (int iv = 0; iv < 3; iv++)
        if (coincident_vertices[tri[iv]].GetSize() == 0)
          edges.PushSmart(Edge(tri[iv], tri[(iv+1)%3]));
    }
    // This speeds up the algorithm
    int edge_step = edges.GetSize() / 5 + 1;
    edge_start = (edge_start + 1) % edge_step;
    
    // For each edge
    for (unsigned int i = edge_start; i < edges.GetSize(); i += edge_step)
    {
      int v0 = edges[i].v0;
      int v1 = edges[i].v1;
      
      WorkMesh k_prime = k;
      // Attempt to collapse v0 to v1
      bool result = Collapse(k_prime, v0, v1);
      if (result)
      {
        //VerifyMesh(k_prime);
        // Compute error metric of this collapse
        float d = SumOfSquareDist(k_prime);
        // Update minimum
        if (d < min_d)
        {
          min_d = d;
          min_v0 = v0;
          min_v1 = v1;
        }
      }
      // Nothing can be better than this.
      if (min_d == 0.0)
        break;
    }
    if (min_d == FLT_MAX && could_not_collapse)
    {
      // If we couldn't collapse now and couldn't collapse last time either, end.
      cout << "No more triangles to collapse" << endl;
      break;
    }
    if (min_d != FLT_MAX)
    {
      // Found the best vertices to collapse: 'min_v0', 'min_v1'.
      // Collapse them.
      bool result = Collapse(k, min_v0, min_v1);
      assert(result);
      sw = k.GetLastWindow();
      cout << "t: " << sw.end_index-sw.start_index << " d: " << min_d << " v: " << min_v0 << "->" << min_v1 << endl;
      // For debug purposes
      VerifyMesh(k);
      collapse_counter++;
      could_not_collapse = false;
      /*
      cout << "T: ";
      for (unsigned int i = 0; i < k.tri_indices.GetSize(); i++)
        cout << i << "=" << k.tri_indices[i] << " ";
      cout << endl << "W: ";
      for (unsigned int i = 0; i < k.sliding_windows.GetSize(); i++)
        cout << k.sliding_windows[i].start_index << "-" << k.sliding_windows[i].end_index << " ";
      cout << endl << "Top limit = " << top_limit << endl;
      */
    }
    
    int curr_num_triangles = sw.end_index - sw.start_index;
    if (curr_num_triangles < min_num_triangles)
    {
      cout << "Reached minimum number of triangles" << endl;
      break;
    }
    // Is it time to replicate?
    if (curr_num_triangles < min_triangles_for_replication || min_d == FLT_MAX)
    {
      // Replicate index buffer
      if (min_d == FLT_MAX)
        could_not_collapse = true;
      cout << "Replicating: " << curr_num_triangles << endl;
      sw.start_index += curr_num_triangles;
      sw.end_index += curr_num_triangles;
      k.SetLastWindow(sw);
      top_limit = sw.end_index;
      for (int i = sw.start_index; i < sw.end_index; i++)
        k.tri_indices.Push(k.tri_indices[i-curr_num_triangles]);
      VerifyMesh(k);
      min_triangles_for_replication = (sw.end_index - sw.start_index) / 2;
    }
  }
  // Copy work mesh to output
  for (unsigned int i = 0; i < k.tri_indices.GetSize(); i++)
    ordered_tris.Push(k.GetTriangle(i));
  for (unsigned int i = 0; i < ordered_tris.GetSize(); i++)
    for (unsigned int j = 0; j < 3; j++)
      assert(ordered_tris[i][j] >= 0 && ordered_tris[i][j] < (int)vertices.GetSize());
  cout << "End" << endl;
}


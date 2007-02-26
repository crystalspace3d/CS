/*
  Copyright (C) 2005 by Marten Svanfeldt

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

#include "crystalspace.h"
#include "primitive.h"
#include "object.h"

static size_t numElementsTotal = 0;

namespace lighter
{

  void ElementAreas::DeleteAll()
  {
    elementsBits.SetSize (0);
    fractionalElements.DeleteAll ();
    elementCount = 0;
  }

  void ElementAreas::SetSize (size_t count)
  {
    elementCount = count;
    elementsBits.SetSize (2*count);
  }

  void ElementAreas::SetElementArea (size_t element, float area)
  {
    if (area == 0)
    {
      elementsBits.SetBit (2*element);
    }
    else if (fabsf (area - fullArea) < SMALL_EPSILON)
    {
      elementsBits.SetBit (2*element+1);
    }
    else
    {
      ElementFloatPair elem;
      elem.area = area;
      elem.element = element;
      fractionalElements.InsertSorted (elem, ElementFloatPairCompare);
    }
  }

  void ElementAreas::ShrinkBestFit ()
  {
    fractionalElements.ShrinkBestFit ();
  }

  int ElementAreas::ElementFloatPairCompare (const ElementFloatPair& i1,
                                             const ElementFloatPair& i2)
  {
    if (i1.element > i2.element)
      return 1;
    else if (i1.element < i2.element)
      return -1;
    else
      return 0;
  }
  
  int ElementAreas::ElementFloatPairSearch (const ElementFloatPair& item,
                                            const size_t& key)
  {
    if (item.element > key)
      return 1;
    else if (item.element < key)
      return -1;
    else
      return 0;
  }

  float ElementAreas::GetElementArea (size_t element) const
  {
    if (elementsBits.IsBitSet (2*element))
      return 0;
    else if (elementsBits.IsBitSet (2*element+1))
      return fullArea;
    else
    {
      size_t index = fractionalElements.FindSortedKey (
        csArrayCmp<ElementFloatPair, size_t> (element, ElementFloatPairSearch));
      return fractionalElements[index].area;
    }
  }

  //-------------------------------------------------------------------------

  void Primitive::ComputeMinMaxUV (csVector2 &min, csVector2 &max) const
  {
    size_t index = triangle[0];
    min = vertexData.vertexArray[index].lightmapUV;
    max = vertexData.vertexArray[index].lightmapUV;
    for (uint i = 1; i < 3; ++i)
    {
      index = triangle[i];
      const csVector2 &uv = vertexData.vertexArray[index].lightmapUV;
      min.x = csMin (min.x, uv.x);
      min.y = csMin (min.y, uv.y);

      max.x = csMax (max.x, uv.x);
      max.y = csMax (max.y, uv.y);
    }
    max += csVector2(0.5f, 0.5f);
  }

  void Primitive::ComputePlane ()
  {
    //Setup a temporary array of our vertices
    Vector3DArray vertices;
    vertices.Push (vertexData.vertexArray[triangle.a].position);
    vertices.Push (vertexData.vertexArray[triangle.b].position);
    vertices.Push (vertexData.vertexArray[triangle.c].position);

    plane = csPoly3D::ComputePlane (vertices);
  }

  const csVector3 Primitive::GetCenter () const
  {
    csVector3 centroid;
    centroid = vertexData.vertexArray[triangle.a].position;
    centroid += vertexData.vertexArray[triangle.b].position;
    centroid += vertexData.vertexArray[triangle.c].position;
    
    return centroid / 3.0f;
  }

  float Primitive::GetArea () const
  {
    float area = csMath3::DoubleArea3 (
      vertexData.vertexArray[triangle.a].position,
      vertexData.vertexArray[triangle.b].position, 
      vertexData.vertexArray[triangle.c].position);
    return area/2.0f;
  }

  void Primitive::GetExtent (uint axis, float &min, float &max) const
  {
    min = FLT_MAX;
    max = -FLT_MAX;
    for (unsigned int i = 0; i < 3; ++i)
    {
      float val = vertexData.vertexArray[triangle.a].position[axis];
      min = csMin(min, val);
      max = csMax(max, val);
    }
  }


  bool Primitive::Split (const csPlane3& splitPlane, 
                            csArray<Primitive>& front,
                            csArray<Primitive>& back) const
  {
    // Do a split
    
    // Classify all points
    float classification[3];
    
    bool n = false, p = false;
    uint i;

    for (i = 0; i < 3; i++)
    {
      float c = splitPlane.Classify (vertexData.vertexArray[triangle[i]].position);
      if (c > LITEPSILON) p = true;
      else if (c < LITEPSILON) n = true;

      classification[i] = c;
    }

    // Handle special cases
    if (!p && !n)
    {
      if ((plane.Normal () * splitPlane.Normal ()))
      {
        p = true;
      }
      else
      {
        n = true;
      }
    }

    if (!n)
    {
      //No negative, back is empty
      front.Push (*this);
      back.DeleteAll ();
      return true;
    }
    else if (!p)
    {
      //No positive, current is back
      front.DeleteAll ();
      back.Push (*this);
    }
    else
    {
      // Split-time! :)
      SizeTDArray negIndex;
      SizeTDArray posIndex;

      negIndex.SetCapacity (3);
      posIndex.SetCapacity (3);

      // Visit all edges, add vertices to back/front as needed. Add intersection-points
      // if needed
      size_t i0 = 2;
      for (size_t i1 = 0; i1 < 3; ++i1)
      {
        size_t v0 = triangle[i0];
        size_t v1 = triangle[i1];
        
        float d0 = classification[i0];
        float d1 = classification[i1];

        if (d0 < 0 && d1 < 0)
        {
          // neg-neg
          negIndex.Push (v1);
        }
        else if (d0 < 0 && d1 == 0)
        {
          // neg-zero
          negIndex.Push (v1);
          posIndex.Push (v1);
        }
        else if (d0 < 0 && d1 > 0)
        {
          // neg-pos xing
          float D = d0 / (d1 - d0);

          ObjectVertexData::Vertex isec = 
            vertexData.InterpolateVertex (v0, v1, D);

          size_t vi = vertexData.vertexArray.Push (isec);

          negIndex.Push (vi);
          posIndex.Push (vi);
        }
        else if (d0 == 0 && d1 > 0)
        {
          // zero-pos
          posIndex.Push (v1);
        }
        else if (d0 == 0 && d1 < 0)
        {
          // zero-neg
          negIndex.Push (v1);
        }
        else if (d0 > 0 && d1 > 0)
        {
          // pos-pos
          posIndex.Push (v1);
        }
        else if (d0 > 0 && d1 < 0)
        {
          // pos-neg
          float D = d1 / (d0 - d1);
          ObjectVertexData::Vertex isec = 
            vertexData.InterpolateVertex (v0, v1, D);

          size_t vi = vertexData.vertexArray.Push (isec);

          negIndex.Push (vi);
          posIndex.Push (vi);
        }
        else if (d0 > 0 && d1 == 0)
        {
          //pos-zero
          negIndex.Push (v1);
          posIndex.Push (v1);
        }
        else if (d0 == 0 && d1 == 0)
        {
          //zero-zero.. bad case.. shouldn't really happen
          //do nothing..
        }
        //next
        v0 = v1;
      }
      for(uint i = 2; i < posIndex.GetSize(); ++i)
      {
        Primitive newPrim (*this);
        newPrim.triangle.a = posIndex[i-2];
        newPrim.triangle.b = posIndex[i-1];
        newPrim.triangle.c = posIndex[i];
        front.Push (newPrim);
      }
      for(uint i = 2; i < negIndex.GetSize(); ++i)
      {
        Primitive newPrim (*this);
        newPrim.triangle.a = negIndex[i-2];
        newPrim.triangle.b = negIndex[i-1];
        newPrim.triangle.c = negIndex[i];
        back.Push (newPrim);
      }
    }

    return true;
  }


  /* Ok, this is a very strange method.. it is fetched from FSrad
   * and basically computes the world->uv-transform. Check following 
   * explanation from FSrad and see if you understand it better than I do ;) 
   */

  /* ------------------------------------------------------------------------
   * Warning: The following code is bizarre, to say the least. I'll try to 
   * explain it here:
   *
   * A polygon with multiple 'n' coordinates per vertex will exist in 'n' 
   * spaces simultaneously. In other words, a polygon that has a 
   * (1) 3D coordinate per vertex, 
   * (2) a 2D texture coordinate per vertex and 
   * (3) a 2D lightmap coordinate per vertex, will exist
   * in each of these three space simultaneously.
   *
   * Because these three spaces are seemingly arbitrary (chosen by an artist, 
   * for example) there is no immediate way to map from one to the next. For 
   * the following explanation, we are only concerned with two of all possible 
   * spaces: 3-space (the description of the polygon as it exists in 3D) and 
   * 2-space (the description of the polygon as it exists in 2D lightmap space.)
   *
   * For the sake of simplicity, I will refer to these two spaces as "3-space" 
   * and "2-space".
   *
   * Because this code was created for the purpose of lightmapping (at least, 
   * originally) our main focus is the ability to transform from 3-space into 
   * 2-space. In the end, we want this to be as quick as possible, so our 
   * final product will be two 3D vectors.
   *
   * These two vectors represent the direction that you must travel (in 
   * 3-space) in order to follow along each of the two axes in 2-space. These 
   * two vectors can (and should) be given a length that will represent the 
   * distance in 3-space needed to travel one unit in 2-space, along the 
   * associated vector.
   *
   * I hope I've made the academics happy, but for those of you that want it 
   * in plain ol' fashioned English, here's a clearer description: Given a 
   * point in 3-space that maps to the point [15.84, 12.43] in 2-space, if we 
   * add the U-vector (the first of our two 2 vectors) to that 3-space point, 
   * we will end up with a 3-space point that maps to the 2-space coordinate 
   * [16.84, 12.43] (one unit along the U-axis in the positive direction in 
   * textures-space.)
   *
   * Confused? If not, you soon will be, because the method by which we need 
   * to calculate this is rather bizarre. I'll try to be clear about it...
   *
   * Going back to an earlier portion of the text, remember that the polygon 
   * exists in multiple spaces simultaneously. Also note that these spaces are 
   * chosen arbitrarily. Because of this, we actually need to search for the 
   * solution, rather than simply calculate it.
   *
   * It's best to think of the solution (or the search for the solution) in 
   * terms of axes. There are three axes in 3-space and two axes in 2-space. 
   * Our result will be the two (3-space) vectors that point along the 
   * direction of the 2 (2-space) axes. Here's what it looks like:
   *
   *             /\
   *            / |\
   *           /  | \
   *          /   |  \
   *         /    |   \
   *        /     |    \
   *       /____________\_____ V-Vector
   *      /----___|      \
   *              |----___\
   *              |
   *              |
   *              U-Vector
   * 
   * What you're looking at is a 3D polygon as seen from a perpendicular view 
   * to the camera, with the two Vectors (U and V) shown as vertical and 
   * horizontal lines. This is the simplest case, these lines will probably 
   * often be at odd orientations, which is why we must search for them. 
   * Here's how we'll perform that search:
   *
   * 	point3	u1_3, u2_3, v1_3, v2_3;
   * 	point2	u1_2, u2_2, v1_2, v2_2;
   * 	double	maxUDist, maxVDist;
   *
   * 	for (each vertex)
   * 	{
   * 	  for (each edge not sharing that vertex)
   * 	  {
   * 	    // Intercept in U?
   *
   * 	    if (edge UVs form a line that intercepts the U value from the current vertex)
   * 	    {
   * 	      if (interceptDist > maxUDist)
   * 	      {
   * 	        maxUDist = interceptDist
   * 	        u1_3 = current vertex
   * 	        u2_3 = interceptPoint
   * 	        u1_2 = current vertex (UV)
   * 	        u2_2 = interceptPoint (UV)
   * 	      }
   * 	    }
   *
   * 	    // Intercept in V (this code is the same as above, but for the V component)?
   *
   * 	    if (edge UVs form a line that intercepts the V value from the current vertex)
   * 	    {
   * 	      if (interceptDist > maxVDist)
   * 	      {
   * 	        maxVDist = interceptDist
   * 	        v1_3 = current vertex
   * 	        v2_3 = interceptPoint
   * 	        v1_2 = current vertex (UV)
   * 	        v2_2 = interceptPoint (UV)
   * 	      }
   * 	    }
   * 	  }
   * 	}
   *
   * At this point, we have 8 points: four of which are points in 2-space (one 
   * point for each endpoint of a line in 3-space that follows the axis of the 
   * associated 2-space) and the four corresponding 2-space coordinates. From 
   * this, we are able to easily calculate the resulting vectors (see code below.)
   *
   * ---------------------------------------------------------------------------------------------------------------------------------
   */

  void Primitive::ComputeUVTransform ()
  {
    uFormVector.Set (0.0f);
    vFormVector.Set (0.0f);

    // Some temporary holders
    csVector3 u0_3d(0.0f), u1_3d(0.0f), v0_3d(0.0f), v1_3d(0.0f);
    csVector2 u0_2d(0.0f), u1_2d(0.0f), v0_2d(0.0f), v1_2d(0.0f);

    float maxuDist = -1, maxvDist = -1;

    //visit all vertices
    for (uint i = 0; i < 3; i++)
    {
      const ObjectVertexData::Vertex& v = vertexData.vertexArray[triangle[i]];
      const csVector3& c3D = v.position;
      const csVector2& c2D = v.lightmapUV;

      size_t v0 = 2;

      // Visit all edges
      for (size_t v1 = 0; v1 < 3; v1++)
      {
        // Skip those that includes current vertex
        if (v0 != i && v1 != i)
        {
          const ObjectVertexData::Vertex& vert0 = vertexData.vertexArray[triangle[v0]];
          const ObjectVertexData::Vertex& vert1 = vertexData.vertexArray[triangle[v1]];

          const csVector3& v03D = vert0.position;
          const csVector3& v13D = vert1.position;

          const csVector2& v02D = vert0.lightmapUV;
          const csVector2& v12D = vert1.lightmapUV;

          //intercept u
          if ((v02D.x - LITEPSILON <= c2D.x && v12D.x + LITEPSILON >= c2D.x) || 
              (v02D.x + LITEPSILON >= c2D.x && v12D.x - LITEPSILON <= c2D.x))
          {
            float D = (c2D.x - v02D.x) / (v12D.x - v02D.x);
            csVector3 isec3D = v03D + (v13D - v03D) * D;
            csVector2 isec2D = v02D + (v12D - v02D) * D;
            float isecDist3D = (c3D-isec3D).Norm ();
            if (isecDist3D > maxuDist)
            {
              maxuDist = isecDist3D;
              u0_3d = c3D;
              u1_3d = isec3D;
              u0_2d = c2D;
              u1_2d = isec2D;
            }
          }

          //intercept v
          if ((v02D.y - LITEPSILON <= c2D.y && v12D.y + LITEPSILON >= c2D.y) || 
              (v02D.y + LITEPSILON >= c2D.y && v12D.y - LITEPSILON <= c2D.y))
          {
            float D = (c2D.y - v02D.y) / (v12D.y - v02D.y);
            csVector3 isec3D = v03D + (v13D - v03D) * D;
            csVector2 isec2D = v02D + (v12D - v02D) * D;
            float isecDist3D = (c3D-isec3D).Norm ();
            if (isecDist3D > maxvDist)
            {
              maxvDist = isecDist3D;
              v0_3d = c3D;
              v1_3d = isec3D;
              v0_2d = c2D;
              v1_2d = isec2D;
            }
          }
        }
        // Next
        v0 = v1;
      }
    }

    // Calc vectors
    if (maxuDist > 0) vFormVector = (u1_3d - u0_3d) / (u1_2d.y - u0_2d.y);
    if (maxvDist > 0) uFormVector = (v1_3d - v0_3d) / (v1_2d.x - v0_2d.x);
  }


  void Primitive::Prepare ()
  {
    // Reset current data
    elementAreas.DeleteAll ();
    elementAreas.SetFullArea ((uFormVector % vFormVector).Norm());

    // Compute min/max uv
    uint uc, vc;
    int maxu, minu, maxv, minv;
    ComputeMinMaxUV (minu, maxu, minv, maxv);

    uc = maxu - minu + 1;
    vc = maxv - minv + 1;

    minUV.x = minu; minUV.y = minv;
    maxUV.x = maxu; maxUV.y = maxv;

    // Min xyz
    csVector2 d = minUV - 
      (vertexData.vertexArray[triangle.a].lightmapUV + csVector2 (0.5f));
    minCoord = vertexData.vertexArray[triangle.a].position 
      + uFormVector * d.x + vFormVector * d.y;

    // Set some default info
    elementAreas.SetSize (uc * vc);

    // Create our splitplanes
    csPlane3 uCut (plane.Normal () % vFormVector);
    uCut.Normalize ();
    csVector3 uCutOrigin = minCoord;
    uCut.SetOrigin (uCutOrigin);
    
    csPlane3 vCut (plane.Normal () % uFormVector);
    vCut.Normalize ();
    csVector3 vCutOrigin = minCoord;
    vCut.SetOrigin (vCutOrigin);

    // Make sure they face correct way
    csVector3 primCenter = GetCenter ();
    if (uCut.Classify (primCenter) < 0) uCut.Normal () = -uCut.Normal ();
    if (vCut.Classify (primCenter) < 0) vCut.Normal () = -vCut.Normal ();

    // Start slicing
    csPoly3D poly;
    poly.AddVertex (vertexData.vertexArray[triangle.a].position);
    poly.AddVertex (vertexData.vertexArray[triangle.b].position);
    poly.AddVertex (vertexData.vertexArray[triangle.c].position);

    csPlane3 evCut = vCut;
    csPoly3D elRow(4), rest(4);
    csPoly3D el(4), restRow (4);
    size_t elNum = 0;
    for (uint v = 0; v  < vc; v++)
    {
      vCutOrigin += vFormVector;
      evCut.SetOrigin (vCutOrigin);
      
      // Cut of a row      
      if (v < (vc-1)) 
      {
        poly.SplitWithPlane (elRow, rest, evCut);
        poly = rest;
      }
      else
      {
        elRow = rest;
      }

      // Cut into elements
      csPlane3 euCut = uCut;
      csVector3 euOrigin = uCutOrigin;
      for (uint u = 0; u < uc; u++)
      {
        //if (elRow.GetVertexCount () == 0) break; //no idea to try to clip it
        euOrigin += uFormVector;
        euCut.SetOrigin (euOrigin);
        
        if (u < (uc-1))
        {
          elRow.SplitWithPlane (el, restRow, euCut);
          elRow = restRow;
        }
        else
        {
          el = elRow;
        }

        float elArea = el.GetArea ();
        elementAreas.SetElementArea (elNum++, elArea);

      }
    }

    elementAreas.ShrinkBestFit ();

    numElementsTotal += elementAreas.GetSize ();

    ComputeBaryCoeffs();
  }

  bool Primitive::PointInside (const csVector3& pt) const
  {
    float lambda, my;
    ComputeBaryCoords (pt, lambda, my);
    return (lambda >= 0.0f && my >= 0.0f && (lambda + my) < 1.0f);
  }

  int Primitive::Classify (const csPlane3 &plane) const
  {
    size_t i;
    size_t front = 0, back = 0;

    for (i = 0; i < 3; i++)
    {
      float dot = plane.Classify (vertexData.vertexArray[triangle[i]].position);
      if (ABS (dot) < EPSILON) dot = 0;
      if (dot > 0)
        back++;
      else if (dot < 0)
        front++;
    }

    if (back == 0 && front == 0) return CS_POL_SAME_PLANE;
    if (back == 0) return CS_POL_FRONT;
    if (front == 0) return CS_POL_BACK;
    return CS_POL_SPLIT_NEEDED;
  }

  void Primitive::ComputeBaryCoeffs ()
  {
    const csVector3& v1 = vertexData.vertexArray[triangle.a].position;
    const csVector3& v2 = vertexData.vertexArray[triangle.b].position;
    const csVector3& v3 = vertexData.vertexArray[triangle.c].position;

    csVector3 diff1_3 = v1 - v3;
    csVector3 diff2_3 = v2 - v3;

    /* In the divisor, one of both diff1_3 and diff2_3 .x, .y. or .z appear.
     * Pick one where the two are not both zero (can happen with coplanar
     * tris.) */
    int dividingCoord = 0;
    while ((fabsf (diff1_3[dividingCoord]) < SMALL_EPSILON)
      && (fabsf (diff2_3[dividingCoord]) < SMALL_EPSILON)
      && (dividingCoord < 2))
      dividingCoord++;

    float coeff1_3, coeff2_3;
    switch (dividingCoord)
    {
      case 0:
        {
          coeff1_3 = diff2_3[1] + diff2_3[2];
          coeff2_3 = diff1_3[1] + diff1_3[2];
          float lambda_factor = 1.0f / 
            (diff1_3[0]*coeff1_3 - diff2_3[0]*coeff2_3);
          float my_factor = 1.0f / 
            (diff2_3[0]*coeff2_3 - diff1_3[0]*coeff1_3);
          lambdaCoeffTV.Set (
            -coeff1_3 * lambda_factor, 
            diff2_3[0]*lambda_factor, 
            diff2_3[0]*lambda_factor);
          myCoeffTV.Set (
            -coeff2_3 * my_factor, 
            diff1_3[0]*my_factor, 
            diff1_3[0]*my_factor);
        }
        break;
      case 1:
        {
          coeff1_3 = diff2_3[0] + diff2_3[2];
          coeff2_3 = diff1_3[0] + diff1_3[2];
          float lambda_factor = 1.0f / 
            (diff1_3[1]*coeff1_3 - diff2_3[1]*coeff2_3);
          float my_factor = 1.0f / 
            (diff2_3[1]*coeff2_3 - diff1_3[1]*coeff1_3);
          lambdaCoeffTV.Set (
            diff2_3[1]*lambda_factor, 
            -coeff1_3 * lambda_factor, 
            diff2_3[1]*lambda_factor);
          myCoeffTV.Set (
            diff1_3[1]*my_factor, 
            -coeff2_3 * my_factor, 
            diff1_3[1]*my_factor);
        }
        break;
      case 2:
        {
          coeff1_3 = diff2_3[0] + diff2_3[1];
          coeff2_3 = diff1_3[0] + diff1_3[1];
          float lambda_factor = 1.0f / 
            (diff1_3[2]*coeff1_3 - diff2_3[2]*coeff2_3);
          float my_factor = 1.0f / 
            (diff2_3[2]*coeff2_3 - diff1_3[2]*coeff1_3);
          lambdaCoeffTV.Set (
            diff2_3[2]*lambda_factor, 
            diff2_3[2]*lambda_factor, 
            -coeff1_3 * lambda_factor);
          myCoeffTV.Set (
            diff1_3[2]*my_factor, 
            diff1_3[2]*my_factor, 
            -coeff2_3 * my_factor);
        }
        break;
    }
  }

  void Primitive::ComputeBaryCoords (const csVector3& v, float& lambda, 
                                        float& my) const
  {
    csVector3 thirdPosToV = 
      vertexData.vertexArray[triangle.c].position - v;

    lambda = (lambdaCoeffTV * thirdPosToV);
    my = (myCoeffTV * thirdPosToV);
  }

  size_t Primitive::ComputeElementIndex (const csVector3& pt) const
  {
    size_t u = (size_t)(pt * uFormVector);
    size_t v = (size_t)(pt * vFormVector);

    return v*size_t (maxUV.x - minUV.x + 1) + u;
  }


  ElementProxy Primitive::GetElement (size_t index)
  {
    return ElementProxy(*this, index);
  }

  ElementProxy Primitive::GetElement (const csVector3& pt)
  {
    return GetElement (ComputeElementIndex (pt));
  }

  csVector3 Primitive::ComputeElementCenter (size_t index) const
  {
    size_t u, v;

    GetElementUV (index, u, v);

    return minCoord + uFormVector * ((float)u+0.5f) + 
      vFormVector * ((float)v+0.5f);
  }

  csVector3 Primitive::ComputeNormal (const csVector3& point) const
  {
    float lambda, my;
    ComputeBaryCoords (point, lambda, my);

    csVector3 norm;

    norm = lambda * vertexData.vertexArray[triangle.a].normal + 
      my * vertexData.vertexArray[triangle.b].normal + 
      (1 - lambda - my) * vertexData.vertexArray[triangle.c].normal;

    return norm.Unit ();
  }

}

/*
    Copyright (C) 1998 by Jorrit Tyberghein
    Written by Alex Pfaffe

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

#include "sysdef.h"
#include "csengine/colldet/collp.h"
#include "csengine/colldet/collider.h"
#include "csobject/nameobj.h"

int CD_num_cols_alloced = 0;
collision_pair *CD_contact = 0;

#define CD_MAX_COLLISION    1000

static int hits = 0;
// Array of hits.
static csCollider* hitv[CD_MAX_COLLISION][2];
static int currHit;

///
csMatrix3 csCollider::mR;
csVector3 csCollider::mT;
///
int csCollider::trianglesTested = 0;
int csCollider::boxesTested = 0;
int csCollider::testLevel = 0;
bool csCollider::firstHit = true;
int  csCollider::numHits = 0;
float csCollider::minBBoxDiam = 0.0;
///

Moment *Moment::stack = 0;

csCollider::csCollider (csPolygonSet *ps)
{
  _type = POLYGONSET;
  _ps = ps;
  _rm = NULL;

  int i, v;
  for (i = 0 ; i < _ps->num_polygon ; i++)
      {
      csPolygon3D* p = (csPolygon3D*)_ps->polygons[i];
      csPortal* portal = p->GetPortal ();
      // Handle solid walls and mirrors.
      if (portal == NULL || portal->IsSpaceWarped())
        {
        if (!_rm)
          {
          CHK (_rm = new CD_model);
          _rm->b = 0;
          _rm->num_boxes_alloced = 0;

          _rm->tris = 0;
          _rm->num_tris = 0;
          _rm->num_tris_alloced = 0;

          if (!_rm)
            return;  // Error
          } 
      
        // Collision detection only works with triangles.
        int *vt = p->GetVerticesIdx ();
        for (v = 2; v < p->GetNumVertices (); v++)
          {
			_rm->addTriangle(_ps->wor_verts[vt[v-1]],_ps->wor_verts[vt[v]],_ps->wor_verts[vt[0]],v-2);
          }
        } // Is not portal
      } // For num_polygon

  if (_rm)
    {
    _rm->build_hierarchy();
    }
  _cd_state = true;
}

csCollider::csCollider( csSprite3D *sp )
{
  _type = SPRITE3D;
  _sp = sp;

  CHK (_rm = new CD_model);
  if (!_rm)
     return;  // Error
  _rm->b = 0;
  _rm->num_boxes_alloced = 0;

  _rm->tris = 0;
  _rm->num_tris = 0;
  _rm->num_tris_alloced = 0;

  // pick a frame for now.  This should of course be done for the
  // correct frame (some global CD outline or else we need to do it
  // for every frame which is too expensive.
  csFrame *cf = _sp->cur_action->GetFrame (_sp->cur_frame);
  csSpriteLOD* lod = _sp->tpl->GetBaseLOD ();
  for (int i = 0; i < lod->GetNumTriangles (); i++)
    {
    int v[3];
    v[0] = lod->GetTriangles ()[i].a;
    v[1] = lod->GetTriangles ()[i].b;
    v[2] = lod->GetTriangles ()[i].c;
    _rm->addTriangle(cf->GetVertex (v[0]),cf->GetVertex (v[1]),cf->GetVertex (v[2]),i);
    }

  _rm->build_hierarchy();
  _cd_state = true;
}

csCollider::~csCollider(void)
{
  if (_rm)
  {
    // the boxes pointed to should be deleted.
    CHK (delete [] _rm->b);

    // the triangles pointed to should be deleted.
    CHK (delete [] _rm->tris);

    CHK (delete _rm);
    _rm = NULL;
  }
}

void csCollider::Activate(bool on)
{
  _cd_state = on ? 1 : 0;
}

// Return object's name.
char *csCollider::GetName ()
{
  if (_type == POLYGONSET) return csNameObject::GetName(*_ps);
  if (_type == SPRITE3D)   return csNameObject::GetName(*_sp);
  return "UNKNOWN";
}

int csCollider::CollidePair (csCollider *p1, csCollider *p2, csTransform *t1, csTransform *t2)
{
  // Skip inactive combinations.
  if (!p1->_cd_state || !p2->_cd_state) return 0;

  // JTY: also skip objects with _rm NULL. This fixes
  // a bug with bezier curves and collision detection.
  if (!p1->_rm || !p2->_rm) return 0;
  
  //call the low level collision detection routine.
  csCollider::firstHit = true;

  BBox *b1 = p1->_rm->b;
  BBox *b2 = p2->_rm->b;
  
  csMatrix3 tR1, tR2, R, R1, R2;
  csVector3 tT1, tT2, T, T1, T2;
  if (t1)
    {
    T1 = t1->GetO2TTranslation ();
    R1 = t1->GetO2T ();
    }
  if (t2)
    {
    R2 = t2->GetO2T ();
    T2 = t2->GetO2TTranslation ();
    }

  // [R1,T1] and [R2,T2] are how the two triangle sets
  // (i.e. models) are positioned in world space.  [tR1,tT1] and
  // [tR2,tT2] are how the top level boxes are positioned in world
  // space
  
  tR1 = R1 * b1->pR;
  tT1 = (R1 * b1->pT) + T1;
  tR2 = R2 * b2->pR;
  tT2 = (R2 * b2->pT) + T2;
  
  // [R,T] is the placement of b2's top level box within
  // the coordinate system of b1's top level box.

  R = tR1.GetTranspose () * tR2;
  T = tR1.GetTranspose () * (tT2 - tT1);

  // To transform tri's from model1's CS to model2's CS use this:
  //    x2 = mR . x1 + mT

  csCollider::mR = R2.GetTranspose () * R1;
  csCollider::mT = R2.GetTranspose () * (T1 - T2);

  // reset the report fields
  csCollider::numHits = 0;
  csCollider::trianglesTested = 0;
  csCollider::boxesTested = 0;
  // make the call
  if (CollideRecursive (b1, b2, R, T))
    {
      // Error.
    }
  else if (csCollider::numHits != 0)
    {
    hitv[hits][0] = p1;
    hitv[hits][1] = p2;
    hits++;
    return 1;
    }
  return 0;
}

inline float min3(float a, float b, float c)
{ return (a<b?(a<c?a:(c<b?c:b)):(b<c?b:c)); }
inline float max3(float a, float b, float c)
{ return (a>b?(a>c?a:(c>b?c:b)):(b>c?b:c)); }

int project6(csVector3 ax, csVector3 p1, csVector3 p2, csVector3 p3,
             csVector3 q1, csVector3 q2, csVector3 q3)
{
   float P1 = ax * p1;
   float P2 = ax * p2;
   float P3 = ax * p3;
   float Q1 = ax * q1;
   float Q2 = ax * q2;
   float Q3 = ax * q3;

   float mx1 = max3(P1,P2,P3);
   float mn1 = min3(P1,P2,P3);
   float mx2 = max3(Q1,Q2,Q3);
   float mn2 = min3(Q1,Q2,Q3);
   
   if (mn1 > mx2) return 0;
   if (mn2 > mx1) return 0;
   return 1;
}

// very robust triangle intersection test
// uses no divisions
// works on coplanar triangles

bool tri_contact (csVector3 P1, csVector3 P2, csVector3 P3,
		  csVector3 Q1, csVector3 Q2, csVector3 Q3) 
{

  /*
     One triangle is (p1,p2,p3).  Other is (q1,q2,q3).
     Edges are (e1,e2,e3) and (f1,f2,f3).
     Normals are n1 and m1
     Outwards are (g1,g2,g3) and (h1,h2,h3).

     We assume that the triangle vertices are in the same coordinate system.

     First thing we do is establish a new c.s. so that p1 is at (0,0,0).
     */
  
  csVector3 p1 = P1 - P2;
  csVector3 p2 = P2 - P1;
  csVector3 p3 = P3 - P1;
  
  csVector3 q1 = Q1 - P1;
  csVector3 q2 = Q2 - P1;
  csVector3 q3 = Q3 - P1;
  
  csVector3 e1 = p2 - p1;
  csVector3 e2 = p3 - p2;
  csVector3 e3 = p1 - p3;

  csVector3 f1 = q2 - q1;
  csVector3 f2 = q3 - q2;
  csVector3 f3 = q1 - q3;
  
  csVector3 n1 = e1 % e2;
  csVector3 m1 = f1 % f2;

  csVector3 g1 = e1 % n1;
  csVector3 g2 = e2 % n1;
  csVector3 g3 = e3 % n1;
  csVector3 h1 = f1 % m1;
  csVector3 h2 = f2 % m1;
  csVector3 h3 = f3 % m1;

  csVector3 ef11 = e1 % f1;
  csVector3 ef12 = e1 % f2;
  csVector3 ef13 = e1 % f3;

  csVector3 ef21 = e2 % f1;
  csVector3 ef22 = e2 % f2;
  csVector3 ef23 = e2 % f3;

  csVector3 ef31 = e3 % f1;
  csVector3 ef32 = e3 % f2;
  csVector3 ef33 = e3 % f3;

  // now begin the series of tests

  if (!project6(n1, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(m1, p1, p2, p3, q1, q2, q3)) return false;
  
  if (!project6(ef11, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef12, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef13, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef21, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef22, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef23, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef31, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef32, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(ef33, p1, p2, p3, q1, q2, q3)) return false;

  if (!project6(g1, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(g2, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(g3, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(h1, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(h2, p1, p2, p3, q1, q2, q3)) return false;
  if (!project6(h3, p1, p2, p3, q1, q2, q3)) return false;

  return true;
}

int add_collision(CDTriangle *tr1, CDTriangle *tr2)
{
  if (!CD_contact)
    {
      CHK (CD_contact = new collision_pair[10]);
      if (!CD_contact) 
	{
	  return true;
	}
      CD_num_cols_alloced = 10;
      csCollider::numHits = 0;
    }
  
  if (csCollider::numHits == CD_num_cols_alloced)
    {
      CHK (collision_pair *t = new collision_pair[CD_num_cols_alloced*2]);
      if (!t)
	{
	  return true;
	}
      CD_num_cols_alloced *= 2;
      
      for(int i=0; i<csCollider::numHits; i++) t[i] = CD_contact[i];
      CHK (delete [] CD_contact);
      CD_contact = t;
    }
  
  CD_contact[csCollider::numHits].tr1 = tr1;
  CD_contact[csCollider::numHits].tr2 = tr2;
  csCollider::numHits++;

  return false;
}

int obb_disjoint(csMatrix3 B, csVector3 T, csVector3 a, csVector3 b)
{
  register float t, s;
  register int r;
  csMatrix3 Bf;
  const float reps = 1e-6;
  
  // Bf = fabs(B)
  Bf.m11 = ABS(B.m11);  Bf.m11 += reps;
  Bf.m12 = ABS(B.m12);  Bf.m12 += reps;
  Bf.m13 = ABS(B.m13);  Bf.m13 += reps;
  Bf.m21 = ABS(B.m21);  Bf.m21 += reps;
  Bf.m22 = ABS(B.m22);  Bf.m22 += reps;
  Bf.m23 = ABS(B.m23);  Bf.m23 += reps;
  Bf.m31 = ABS(B.m31);  Bf.m31 += reps;
  Bf.m32 = ABS(B.m32);  Bf.m32 += reps;
  Bf.m33 = ABS(B.m33);  Bf.m33 += reps;
  
  // if any of these tests are one-sided, then the polyhedra are disjoint
  r = 1;

  // A1 x A2 = A0
  t = ABS(T.x);
  
  r &= (t <= 
	  (a.x + b.x * Bf.m11 + b.y * Bf.m12 + b.z * Bf.m13));
  if (!r) return 1;
  
  // B1 x B2 = B0
  s = T.x*B.m11 + T.y*B.m21 + T.z*B.m31;
  t = ABS(s);

  r &= ( t <=
	  (b.x + a.y * Bf.m11 + a.y * Bf.m21 + a.z * Bf.m31));
  if (!r) return 2;
    
  // A2 x A0 = A1
  t = ABS(T.y);
  
  r &= ( t <= 
	  (a.y + b.x * Bf.m21 + b.y * Bf.m22 + b.z * Bf.m23));
  if (!r) return 3;

  // A0 x A1 = A2
  t = ABS(T.z);

  r &= ( t <= 
	  (a.z + b.x * Bf.m31 + b.y * Bf.m32 + b.z * Bf.m33));
  if (!r) return 4;

  // B2 x B0 = B1
  s = T.x*B.m12 + T.y*B.m22 + T.z*B.m32;
  t = ABS(s);

  r &= ( t <=
	  (b.y + a.x * Bf.m12 + a.y * Bf.m22 + a.z * Bf.m32));
  if (!r) return 5;

  // B0 x B1 = B2
  s = T.x*B.m13 + T.y*B.m23 + T.z*B.m33;
  t = ABS(s);

  r &= ( t <=
	  (b.z + a.x * Bf.m13 + a.y * Bf.m23 + a.z * Bf.m33));
  if (!r) return 6;

  // A0 x B0
  s = T.z * B.m21 - T.y * B.m31;
  t = ABS(s);
  
  r &= ( t <= 
	(a.y * Bf.m31 + a.z * Bf.m21 +
	 b.y * Bf.m13 + b.z * Bf.m12));
  if (!r) return 7;
  
  // A0 x B1
  s = T.z * B.m22 - T.y * B.m32;
  t = ABS(s);

  r &= ( t <=
	(a.y * Bf.m32 + a.z * Bf.m22 +
	 b.x * Bf.m13 + b.z * Bf.m11));
  if (!r) return 8;

  // A0 x B2
  s = T.z * B.m23 - T.y * B.m33;
  t = ABS(s);

  r &= ( t <=
	  (a.y * Bf.m33 + a.z * Bf.m23 +
	   b.x * Bf.m12 + b.y * Bf.m11));
  if (!r) return 9;

  // A1 x B0
  s = T.x * B.m31 - T.z * B.m11;
  t = ABS(s);

  r &= ( t <=
	  (a.x * Bf.m31 + a.z * Bf.m11 +
	   b.y * Bf.m23 + b.z * Bf.m22));
  if (!r) return 10;

  // A1 x B1
  s = T.x * B.m32 - T.z * B.m12;
  t = ABS(s);

  r &= ( t <=
	  (a.x * Bf.m32 + a.z * Bf.m12 +
	   b.x * Bf.m23 + b.z * Bf.m21));
  if (!r) return 11;

  // A1 x B2
  s = T.x * B.m33 - T.z * B.m13;
  t = ABS(s);

  r &= (t <=
	  (a.x * Bf.m33 + a.z * Bf.m13 +
	   b.x * Bf.m22 + b.y * Bf.m21));
  if (!r) return 12;

  // A2 x B0
  s = T.y * B.m11 - T.x * B.m21;
  t = ABS(s);

  r &= (t <=
	  (a.x * Bf.m21 + a.y * Bf.m11 +
	   b.y * Bf.m33 + b.z * Bf.m32));
  if (!r) return 13;

  // A2 x B1
  s = T.y * B.m12 - T.x * B.m22;
  t = ABS(s);

  r &= ( t <=
	  (a.x * Bf.m22 + a.y * Bf.m12 +
	   b.x * Bf.m33 + b.z * Bf.m31));
  if (!r) return 14;

  // A2 x B2
  s = T.y * B.m13 - T.x * B.m23;
  t = ABS(s);

  r &= ( t <=
	  (a.x * Bf.m23 + a.y * Bf.m13 +
	   b.x * Bf.m32 + b.y * Bf.m31));
  if (!r) return 15;

  return 0;  // should equal 0
}


int csCollider::CollideRecursive (BBox *b1, BBox *b2, csMatrix3 R, csVector3 T)
{

  int rc;      // return codes
  
  if (csCollider::firstHit && (csCollider::numHits > 0)) return false;

  // test top level

  csCollider::boxesTested++;

  int f1;
  
  f1 = obb_disjoint(R, T, b1->d, b2->d);

  if (f1 != 0) 
	{
	  return false;  // stop processing this test, go to top of loop
	}

      // contact between boxes
      if (b1->leaf() && b2->leaf()) 
	{
	  // it is a leaf pair - compare the polygons therein
          // tri_contact uses the model-to-model transforms stored in
	  // csCollider::mR, csCollider::mT.

	  // this will pass along any OUT_OF_MEMORY return codes which
	  // may be generated.
	  return BBox::tri_contact(b1, b2);
	}

      csMatrix3 cR;
      csVector3 cT;
      
      // Currently, the transform from model 2 to model 1 space is
      // given by [B T], where y = [B T].x = B.x + T.

      if (b2->leaf() || (!b1->leaf() && (b1->size() > b2->size())))
	{
	  // here we descend to children of b1.  The transform from
	  // a child of b1 to b1 is stored in [b1->N->pR,b1->N->pT],
	  // but we will denote it [B1 T1] for short.

	  // Here, we compute [B1 T1]'[B T] = [B1'B B1'(T-T1)]
	  // for each child, and store the transform into the collision
	  // test queue.

          cR = b1->N->pR.GetTranspose () * R;
          cT = b1->N->pR.GetTranspose () * (T - b1->N->pT);
 
	  if ((rc = CollideRecursive (b1->N, b2, cR, cT)) != false)
	    return rc;
	  
          cR = b1->P->pR.GetTranspose () * R;
          cT = b1->P->pR.GetTranspose () * (T - b1->P->pT);

	  if ((rc = CollideRecursive (b1->P, b2, cR, cT)) != false)
	    return rc;
	  
	  return false;
	}
      else 
	{
	  // here we descend to the children of b2.  See comments for
	  // other 'if' clause for explanation.

          cR = R * b2->N->pR;
          cT = ( R * b2->N->pT) + T;
	  
	  if ((rc = CollideRecursive (b1, b2->N, cR, cT)) != false)
	    return rc;
	  
          cR = R * b2->P->pR;
          cT = ( R * b2->P->pT) + T;

	  if ((rc = CollideRecursive (b1, b2->P, cR, cT)) != false)
	    return rc;
	  
	  return false; 
	}
  
  return false;
}

void csCollider::CollideReset (void)
{
  hits = 0;
  currHit = 0;
}

int csCollider::Report (csCollider **id1, csCollider **id2)
{
  if (currHit >= hits) return 0;

  *id1 = hitv[currHit][0];
  *id2 = hitv[currHit][1];
  currHit++;
  return 1;
}

csCollider* csCollider::FindCollision (CDTriangle **tr1, CDTriangle **tr2)
{
  int i = 0;
  while (i < hits)
    if      (hitv[i][0] == this)
	{
		if (tr1)
			*tr1 = CD_contact[i].tr2;
		if (tr2)
			*tr2 = CD_contact[i].tr1;
		return hitv[i][1];
	}
    else if (hitv[i][1] == this)
	{
		if (tr1)
			*tr1 = CD_contact[i].tr1;
		if (tr2)
			*tr2 = CD_contact[i].tr2;
		return hitv[i][0];
	}
    else i++;
  return 0;
}

bool BBox::tri_contact(BBox *b1, BBox *b2)
{
	// assume just one triangle in each box.

	// the vertices of the CDTriangle in b2 is in model1 C.S.  The vertices of
	// the other triangle is in model2 CS.  Use csCollider::mR, csCollider::mT, and
	// to transform into model2 CS.

	int rc;  // return code

	csVector3 i1 = (( csCollider::mR * b1->trp->p1) + csCollider::mT);
	csVector3 i2 = (( csCollider::mR * b1->trp->p2) + csCollider::mT);
	csVector3 i3 = (( csCollider::mR * b1->trp->p3) + csCollider::mT);

	csCollider::trianglesTested++;

	bool f = ::tri_contact(i1, i2, i3, b2->trp->p1, b2->trp->p2, b2->trp->p3);

	if (f) 
	{
		// add_collision may be unable to allocate enough memory,
		// so be prepared to pass along an OUT_OF_MEMORY return code.
		if ((rc = add_collision(b1->trp, b2->trp)) != false)
		{
			return rc;
		}
	}

	return false;
}

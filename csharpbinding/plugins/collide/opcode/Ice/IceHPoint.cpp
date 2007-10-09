///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for homogeneous points.
 *	\file		IceHPoint.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Homogeneous point.
 *
 *	Use it:
 *	- for clipping in homogeneous space (standard way)
 *	- to differentiate between points (w=1) and vectors (w=0).
 *	- in some cases you can also use it instead of Point for padding reasons.
 *
 *	\class		HPoint
 *	\author		Pierre Terdiman
 *	\version	1.0
 *	\warning	No cross-product in 4D.
 *	\warning	HPoint *= Matrix3x3 doesn't exist, the matrix is first casted to a 4x4
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"

CS_PLUGIN_NAMESPACE_BEGIN(csOpcode)
{

using namespace IceMaths;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Point Mul = HPoint * Matrix3x3;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Point HPoint::operator*(const Matrix3x3& mat) const
{
	return Point(
	x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0],
	x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1],
	x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HPoint Mul = HPoint * Matrix4x4;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HPoint HPoint::operator*(const Matrix4x4& mat) const
{
	return HPoint(
	x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + w * mat.m[3][0],
	x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + w * mat.m[3][1],
	x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + w * mat.m[3][2],
	x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + w * mat.m[3][3]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HPoint *= Matrix4x4
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HPoint& HPoint::operator*=(const Matrix4x4& mat)
{
	float xp = x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + w * mat.m[3][0];
	float yp = x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + w * mat.m[3][1];
	float zp = x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + w * mat.m[3][2];
	float wp = x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + w * mat.m[3][3];

	x = xp; y = yp; z = zp; w = wp;

	return	*this;
}

}
CS_PLUGIN_NAMESPACE_END(csOpcode)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for a tree collider.
 *	\file		OPC_TreeCollider.h
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __CS_OPC_TREECOLLIDER_H__
#define __CS_OPC_TREECOLLIDER_H__

	class //OPCODE_API
       	AABBTreeCollider : public Collider {
		public:
		// Constructor / Destructor
											AABBTreeCollider();
		virtual								~AABBTreeCollider();
		// Generic collision query

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Generic collision query for generic OPCODE models. After the call, access the results with:
		 *	- GetContactStatus()
		 *	- GetNbPairs()
		 *	- GetPairs()
		 *
		 *	\param		cache			[in] collision cache for model pointers and a colliding pair of primitives
		 *	\param		world0			[in] world matrix for first object, or null
		 *	\param		world1			[in] world matrix for second object, or null
		 *	\return		true if success
		 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							bool			Collide(BVTCache& cache, const Matrix4x4* world0=null, const Matrix4x4* world1=null);

		// Collision queries
							bool			Collide(const AABBCollisionTree* tree0, const AABBCollisionTree* tree1,				const Matrix4x4* world0=null, const Matrix4x4* world1=null, Pair* cache=null);
							bool			Collide(const AABBNoLeafTree* tree0, const AABBNoLeafTree* tree1,					const Matrix4x4* world0=null, const Matrix4x4* world1=null, Pair* cache=null);
							bool			Collide(const AABBQuantizedTree* tree0, const AABBQuantizedTree* tree1,				const Matrix4x4* world0=null, const Matrix4x4* world1=null, Pair* cache=null);
							bool			Collide(const AABBQuantizedNoLeafTree* tree0, const AABBQuantizedNoLeafTree* tree1,	const Matrix4x4* world0=null, const Matrix4x4* world1=null, Pair* cache=null);
		// Settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Settings: selects between full box-box tests or "SAT-lite" tests (where Class III axes are discarded)
		 *	\param		flag		[in] true for full tests, false for coarse tests
		 *	\see		SetFullPrimBoxTest(bool flag)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetFullBoxBoxTest(bool flag)			{ mFullBoxBoxTest		= flag;					}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Settings: selects between full triangle-box tests or "SAT-lite" tests (where Class III axes are discarded)
		 *	\param		flag		[in] true for full tests, false for coarse tests
		 *	\see		SetFullBoxBoxTest(bool flag)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetFullPrimBoxTest(bool flag)			{ mFullPrimBoxTest		= flag;					}

		// Stats

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stats: gets the number of BV-BV overlap tests after a collision query.
		 *	\see		GetNbPrimPrimTests()
		 *	\see		GetNbBVPrimTests()
		 *	\return		the number of BV-BV tests performed during last query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbBVBVTests()				const	{ return mNbBVBVTests;							}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stats: gets the number of Triangle-Triangle overlap tests after a collision query.
		 *	\see		GetNbBVBVTests()
		 *	\see		GetNbBVPrimTests()
		 *	\return		the number of Triangle-Triangle tests performed during last query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbPrimPrimTests()			const	{ return mNbPrimPrimTests;						}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stats: gets the number of BV-Triangle overlap tests after a collision query.
		 *	\see		GetNbBVBVTests()
		 *	\see		GetNbPrimPrimTests()
		 *	\return		the number of BV-Triangle tests performed during last query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbBVPrimTests()				const	{ return mNbBVPrimTests;						}

		// Data access

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets the number of contacts after a collision query.
		 *	\see		GetContactStatus()
		 *	\see		GetPairs()
		 *	\return		the number of contacts / colliding pairs.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbPairs()					const	{ return mPairs.GetNbEntries()>>1;				}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets the pairs of colliding triangles after a collision query.
		 *	\see		GetContactStatus()
		 *	\see		GetNbPairs()
		 *	\return		the list of colliding pairs (triangle indices)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				const Pair*		GetPairs()						const	{ return (const Pair*)mPairs.GetEntries();		}

#ifdef OPC_USE_CALLBACKS
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Callback control: a method to setup callback 0. Must provide triangle-vertices for a given triangle index.
		 *	\param		callback	[in] user-defined callback
		 *	\param		data		[in] user-defined data
		 *	\see		SetCallback1(OPC_CALLBACK callback, udword data)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetCallback0(OPC_CALLBACK callback, udword data)	{ mObjCallback0	= callback;	mUserData0	= data;		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Callback control: a method to setup callback 1. Must provide triangle-vertices for a given triangle index.
		 *	\param		callback	[in] user-defined callback
		 *	\param		data		[in] user-defined data
		 *	\see		SetCallback0(OPC_CALLBACK callback, udword data)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetCallback1(OPC_CALLBACK callback, udword data)	{ mObjCallback1	= callback;	mUserData1	= data;		}
#else
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Pointers control: a method to setup object0 pointers. Must provide access to faces and vertices for a given object.
		 *	\param		faces	[in] pointer to faces
		 *	\param		verts	[in] pointer to vertices
		 *	\see		SetPointers1(const Triangle* faces, const Point* verts)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetPointers0(const IndexedTriangle* faces, const Point* verts)	{ mFaces0 = faces;	mVerts0 = verts;	}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Pointers control: a method to setup object1 pointers. Must provide access to faces and vertices for a given object.
		 *	\param		faces	[in] pointer to faces
		 *	\param		verts	[in] pointer to vertices
		 *	\see		SetPointers0(const Triangle* faces, const Point* verts)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				void			SetPointers1(const IndexedTriangle* faces, const Point* verts)	{ mFaces1 = faces;	mVerts1 = verts;	}
#endif
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Validates current settings. You should call this method after all the settings and callbacks have been defined for a collider.
		 *	\return		null if everything is ok, else a string describing the problem
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		override(Collider)	const char*		ValidateSettings();

		protected:
		// Colliding pairs
							Container		mPairs;				//!< Pairs of colliding primitives
#ifdef OPC_USE_CALLBACKS
		// User callback
							udword			mUserData0;			//!< User-defined data sent to callbacks
							udword			mUserData1;			//!< User-defined data sent to callbacks
							OPC_CALLBACK	mObjCallback0;		//!< Callback for object 0
							OPC_CALLBACK	mObjCallback1;		//!< Callback for object 1
#else
		// User pointers
					const IndexedTriangle*	mFaces0;			//!< User-defined faces
					const IndexedTriangle*	mFaces1;			//!< User-defined faces
							const Point*	mVerts0;			//!< User-defined vertices
							const Point*	mVerts1;			//!< User-defined vertices
#endif
		// Stats
							udword			mNbBVBVTests;		//!< Number of BV-BV tests
							udword			mNbPrimPrimTests;	//!< Number of Primitive-Primitive tests
							udword			mNbBVPrimTests;		//!< Number of BV-Primitive tests
		// Precomputed data
							Matrix3x3		mAR;				//!< Absolute rotation matrix
							Matrix3x3		mR0to1;				//!< Rotation from object0 to object1
							Matrix3x3		mR1to0;				//!< Rotation from object1 to object0
							Point			mT0to1;				//!< Translation from object0 to object1
							Point			mT1to0;				//!< Translation from object1 to object0
		// Dequantization coeffs
							Point			mCenterCoeff0;
							Point			mExtentsCoeff0;
							Point			mCenterCoeff1;
							Point			mExtentsCoeff1;
		// Leaf description
							Point			mLeafVerts[3];		//!< Triangle vertices
							udword			mLeafIndex;			//!< Triangle index
		// Settings
							bool			mFullBoxBoxTest;	//!< Perform full BV-BV tests (true) or SAT-lite tests (false)
							bool			mFullPrimBoxTest;	//!< Perform full Primitive-BV tests (true) or SAT-lite tests (false)
		// Internal methods

			// Standard AABB trees
							void			_Collide(const AABBCollisionNode* b0, const AABBCollisionNode* b1);
			// Quantized AABB trees
							void			_Collide(const AABBQuantizedNode* b0, const AABBQuantizedNode* b1, const Point& a, const Point& Pa, const Point& b, const Point& Pb);
			// No-leaf AABB trees
							void			_CollideTriBox(const AABBNoLeafNode* b);
							void			_CollideBoxTri(const AABBNoLeafNode* b);
							void			_Collide(const AABBNoLeafNode* a, const AABBNoLeafNode* b);
			// Quantized no-leaf AABB trees
							void			_CollideTriBox(const AABBQuantizedNoLeafNode* b);
							void			_CollideBoxTri(const AABBQuantizedNoLeafNode* b);
							void			_Collide(const AABBQuantizedNoLeafNode* a, const AABBQuantizedNoLeafNode* b);
			// Overlap tests
							void			PrimTest(udword id0, udword id1);
			inline_			void			PrimTestTriIndex(udword id1);
			inline_			void			PrimTestIndexTri(udword id0);

			inline_			BOOL			BoxBoxOverlap(const Point& ea, const Point& ca, const Point& eb, const Point& cb);
			inline_			BOOL			TriBoxOverlap(const Point& center, const Point& extents);
			inline_			BOOL			TriTriOverlap(const Point& V0, const Point& V1, const Point& V2, const Point& U0, const Point& U1, const Point& U2);
			// Init methods
							void			InitQuery(const Matrix4x4* world0=null, const Matrix4x4* world1=null);
							bool			CheckTemporalCoherence(Pair* cache);
	};

#endif // __CS_OPC_TREECOLLIDER_H__


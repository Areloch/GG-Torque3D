//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _CONVEX_SHAPE_COMPONENT_H_
#define _CONVEX_SHAPE_COMPONENT_H_

#ifndef _COMPONENTTEMPLATE_H_
#include "component/components/component.h"
#endif

#ifndef _COMPONENTINSTANCE_H_
#include "component/components/componentInstance.h"
#endif

#ifndef _STOCK_INTERFACES_H_
   #include "component/components/stockInterfaces.h"
#endif

#ifndef _RENDER_INTERFACES_H_
   #include "component/components/render/renderInterfaces.h"
#endif

#ifndef _BASE_TOOL_H_
	#include "component/components/baseTool.h"
#endif


//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
GFXDeclareVertexFormat( ConvexShapeVert )
{
   Point3F point;
   GFXVertexColor color;
   Point3F normal;
   Point3F tangent;
   Point2F texCoord;
};
typedef ConvexShapeVert VertexType;

class ConvexShapeComponentInstance;
class ConvexShapeComponentTool;

class ConvexShapeComponent : public Component
{
   typedef Component Parent;

public:
   ConvexShapeComponent();
   virtual ~ConvexShapeComponent();
   DECLARE_CONOBJECT(ConvexShapeComponent);

   //virtual bool onAdd();
   //virtual void onRemove();
   //static void initPersistFields();

   //virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   //virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a ConvexShapeComponentInstance
   virtual ComponentInstance *createInstance();
};

class ConvexShapeComponentInstance : public ComponentInstance, public PrepRenderImageInterface
{
   typedef ComponentInstance Parent;
   friend class ConvexShapeComponentTool;
   friend class ConvexShapeComponentUndo;

public:
	struct Edge
	{
		U32 p0;
		U32 p1;
	};

	struct Triangle
	{
		U32 p0;
		U32 p1;
		U32 p2;

		U32 operator []( U32 index ) const
		{
			AssertFatal( index >= 0 && index <= 2, "index out of range" );
			return *( (&p0) + index );
		}
	};

	struct Face
	{
		Vector< Edge > edges;
		Vector< U32 > points;
		Vector< U32 > winding;
		Vector< Point2F > texcoords;
		Vector< Triangle > triangles;			
		Point3F tangent;
		Point3F normal;
		Point3F centroid;
		F32 area;
		S32 id;
	}; 

	struct MeshGeometry
	{  
		void generate( const Vector< PlaneF > &planes, const Vector< Point3F > &tangents );   

		Vector< Point3F > points;      
		Vector< Face > faces;
	};

protected:
	static bool smRenderEdges;   

	// To prevent bitpack overflows.
	// This is only indirectly enforced by trucation when serializing.
	static const S32 smMaxSurfaces = 100;

public:
   ConvexShapeComponentInstance(Component *btemplate = NULL);
   virtual ~ConvexShapeComponentInstance();

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();
   virtual void inspectPostApply();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   virtual void prepRenderImage(SceneRenderState *state );

   bool buildPolyList( PolyListContext context, AbstractPolyList *plist, const Box3F &box, const SphereF &sphere );

   //
   void _updateMaterial();
   void _updateGeometry( bool updateCollision = false );
   //void _updateCollision();
   void _export( OptimizedPolyList *plist, const Box3F &box, const SphereF &sphere );

   void updateBounds( bool recenter );

   void _renderDebug( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *mat );

   static S32 QSORT_CALLBACK _comparePlaneDist( const void *a, const void *b );

   static bool setSurface( void *object, const char *index, const char *data );

   void renderFaceEdges( S32 faceid, const ColorI &color = ColorI::WHITE, F32 lineWidth = 1.0f );
   //
   void recenter();

   /// Geometry access.
   /// @{
         
      MatrixF getSurfaceWorldMat( S32 faceid, bool scaled = false ) const;
	  void cullEmptyPlanes( Vector< U32 > *removedPlanes ){}
		void exportToCollada();
      void resizePlanes( const Point3F &size );
      void getSurfaceLineList( S32 surfId, Vector< Point3F > &lineList );
      //Geometry& getGeometry() { return mGeometry; }

	  virtual MeshGeometry* getConvexGeometry() { return &mGeometry; }

      Vector<MatrixF>& getSurfaces() { return mSurfaces; }
      void getSurfaceTriangles( S32 surfId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace );

		//we specially override the pack call because we need to store out our surface data
		virtual void packToStream( Stream &stream, U32 tabStop, S32 behaviorID, U32 flags = 0 );

	//
	void setTransform(MatrixF newTransform);

	//FOR THE EDITOR!
	bool castRay( const Point3F &start, const Point3F &end, RayInfo *info );
protected:
	// The name of the Material we will use for rendering
   String            mMaterialName;

   // The actual Material instance
   BaseMatInstance*  mMaterialInst;

   // The GFX vertex and primitive buffers
   GFXVertexBufferHandle< VertexType > mVertexBuffer;
   GFXPrimitiveBufferHandle            mPrimitiveBuffer;

   U32 mVertCount;
   U32 mPrimCount;

   MeshGeometry mGeometry;  

   Vector< PlaneF > mPlanes;

   Vector< MatrixF > mSurfaces;

   Vector< Point3F > mFaceCenters;

   DECLARE_CONOBJECT(ConvexShapeComponentInstance);
};

#endif // _ConvexShapeComponent_H_

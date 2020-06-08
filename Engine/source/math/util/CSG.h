#pragma once
#include "math/mathUtils.h"
#include "collision/concretePolyList.h"
#include <list>
#include <vector>
#include <algorithm>

namespace CSGUtils
{
   struct CSGVector
   {
      float x, y, z;

      CSGVector() : x(0.0f), y(0.0f), z(0.0f) {}
      explicit CSGVector(float x, float y, float z) : x(x), y(y), z(z) {}
   };

   struct CSGVertex
   {
      CSGVector pos;
      CSGVector normal;
      CSGVector uv;
      int faceId;
      int shapeId;
   };

   struct CSGModel
   {
      std::vector<CSGVertex> vertices;
      std::vector<int> indices;

      void setTransform(const MatrixF& mat)
      {
         for (U32 i = 0; i < vertices.size(); i++)
         {
            Point3F pos = Point3F(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
            mat.mulP(pos);

            vertices[i].pos.x = pos.x;
            vertices[i].pos.y = pos.y;
            vertices[i].pos.z = pos.z;
         }
      }
   };

   // public interface - not super efficient, if you use multiple CSG operations you should
   // use BSP trees and convert them into model only once. Another optimization trick is
   // replacing CSGModel with your own class.

   CSGModel csgjs_union(const CSGModel & a, const CSGModel & b);
   CSGModel csgjs_intersection(const CSGModel & a, const CSGModel & b);
   CSGModel csgjs_difference(const CSGModel & a, const CSGModel & b);

   struct CSGPlane;
   struct CSGPolygon;
   struct CSGNode;

   // Represents a plane in 3D space.
   struct CSGPlane
   {
      CSGVector normal;
      float w;

      CSGPlane();
      CSGPlane(const CSGVector & a, const CSGVector & b, const CSGVector & c);
      bool ok() const;
      void flip();
      void splitPolygon(const CSGPolygon & polygon, std::vector<CSGPolygon> & coplanarFront, std::vector<CSGPolygon> & coplanarBack, std::vector<CSGPolygon> & front, std::vector<CSGPolygon> & back) const;
   };

   // Represents a convex polygon. The vertices used to initialize a polygon must
   // be coplanar and form a convex loop. They do not have to be `CSG.Vertex`
   // instances but they must behave similarly (duck typing can be used for
   // customization).
   // 
   // Each convex polygon has a `shared` property, which is shared between all
   // polygons that are clones of each other or were split from the same polygon.
   // This can be used to define per-polygon properties (such as surface color).
   struct CSGPolygon
   {
      std::vector<CSGVertex> vertices;
      CSGPlane plane;
      void flip();

      CSGPolygon();
      CSGPolygon(const std::vector<CSGVertex> & list);
   };

   // Holds a node in a BSP tree. A BSP tree is built from a collection of polygons
   // by picking a polygon to split along. That polygon (and all other coplanar
   // polygons) are added directly to that node and the other polygons are added to
   // the front and/or back subtrees. This is not a leafy BSP tree since there is
   // no distinction between internal and leaf nodes.
   struct CSGNode
   {
      std::vector<CSGPolygon> polygons;
      CSGNode * front;
      CSGNode * back;
      CSGPlane plane;

      CSGNode();
      CSGNode(const std::vector<CSGPolygon> & list);
      ~CSGNode();

      CSGNode * clone() const;
      void clipTo(const CSGNode * other);
      void invert();
      void build(const std::vector<CSGPolygon> & polygon);
      std::vector<CSGPolygon> clipPolygons(const std::vector<CSGPolygon> & list) const;
      std::vector<CSGPolygon> allPolygons() const;
   };

   // std::vector implementation

   inline static CSGVector operator + (const CSGVector & a, const CSGVector & b) { return CSGVector(a.x + b.x, a.y + b.y, a.z + b.z); }
   inline static CSGVector operator - (const CSGVector & a, const CSGVector & b) { return CSGVector(a.x - b.x, a.y - b.y, a.z - b.z); }
   inline static CSGVector operator * (const CSGVector & a, float b) { return CSGVector(a.x * b, a.y * b, a.z * b); }
   inline static CSGVector operator / (const CSGVector & a, float b) { return a * (1.0f / b); }
   inline static float dot(const CSGVector & a, const CSGVector & b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
   inline static CSGVector lerp(const CSGVector & a, const CSGVector & b, float v) { return a + (b - a) * v; }
   inline static CSGVector negate(const CSGVector & a) { return a * -1.0f; }
   inline static float length(const CSGVector & a) { return sqrtf(dot(a, a)); }
   inline static CSGVector unit(const CSGVector & a) { return a / length(a); }
   inline static CSGVector cross(const CSGVector & a, const CSGVector & b) { return CSGVector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

   // Vertex implementation

   // Invert all orientation-specific data (e.g. vertex normal). Called when the
   // orientation of a polygon is flipped.
   inline static CSGVertex flip(CSGVertex v)
   {
      v.normal = negate(v.normal);
      return v;
   }

   // Create a new vertex between this vertex and `other` by linearly
   // interpolating all properties using a parameter of `t`. Subclasses should
   // override this to interpolate additional properties.
   inline static CSGVertex interpolate(const CSGVertex & a, const CSGVertex & b, float t)
   {
      CSGVertex ret;
      ret.pos = lerp(a.pos, b.pos, t);
      ret.normal = lerp(a.normal, b.normal, t);
      ret.uv = lerp(a.uv, b.uv, t);
      return ret;
   }

   // Plane implementation

   // Node implementation

   // Return a new CSG solid representing space in either this solid or in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static CSGNode * csg_union(const CSGNode * a1, const CSGNode * b1)
   {
      CSGNode * a = a1->clone();
      CSGNode * b = b1->clone();
      a->clipTo(b);
      b->clipTo(a);
      b->invert();
      b->clipTo(a);
      b->invert();
      a->build(b->allPolygons());
      CSGNode * ret = new CSGNode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   // Return a new CSG solid representing space in this solid but not in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static CSGNode * csg_subtract(const CSGNode * a1, const CSGNode * b1)
   {
      CSGNode * a = a1->clone();
      CSGNode * b = b1->clone();
      a->invert();
      a->clipTo(b);
      b->clipTo(a);
      b->invert();
      b->clipTo(a);
      b->invert();
      a->build(b->allPolygons());
      a->invert();
      CSGNode * ret = new CSGNode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   // Return a new CSG solid representing space both this solid and in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static CSGNode * csg_intersect(const CSGNode * a1, const CSGNode * b1)
   {
      CSGNode * a = a1->clone();
      CSGNode * b = b1->clone();
      a->invert();
      b->clipTo(a);
      b->invert();
      a->clipTo(b);
      b->clipTo(a);
      a->build(b->allPolygons());
      a->invert();
      CSGNode * ret = new CSGNode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   
   // Public interface implementation

   inline static std::vector<CSGPolygon> CSGModelToPolygons(const CSGModel & model)
   {
      std::vector<CSGPolygon> list;
      int materialIndex = 0;
      for (size_t i = 0; i < model.indices.size(); i += 3)
      {
         std::vector<CSGVertex> triangle;
         for (int j = 0; j < 3; j++)
         {
            CSGVertex v = model.vertices[model.indices[i + j]];
            triangle.push_back(v);
         }
         list.push_back(CSGPolygon(triangle));
         materialIndex++;
      }
      return list;
   }

   inline static CSGModel CSGModelFromPolygons(const std::vector<CSGPolygon> & polygons)
   {
      CSGModel model;
      int p = 0;
      for (size_t i = 0; i < polygons.size(); i++)
      {
         const CSGPolygon & poly = polygons[i];
         for (size_t j = 2; j < poly.vertices.size(); j++)
         {
            model.vertices.push_back(poly.vertices[0]);		model.indices.push_back(p++);
            model.vertices.push_back(poly.vertices[j - 1]);	model.indices.push_back(p++);
            model.vertices.push_back(poly.vertices[j]);		model.indices.push_back(p++);
         }
      }
      return model;
   }

   typedef CSGNode * csg_function(const CSGNode * a1, const CSGNode * b1);

   inline static CSGModel csgjs_operation(const CSGModel & a, const CSGModel & b, csg_function fun)
   {
      CSGNode * A = new CSGNode(CSGModelToPolygons(a));
      CSGNode * B = new CSGNode(CSGModelToPolygons(b));
      CSGNode * AB = fun(A, B);

      //Flip the final normals back around
      AB->invert();

      std::vector<CSGPolygon> polygons = AB->allPolygons();
      delete A; A = 0;
      delete B; B = 0;
      delete AB; AB = 0;
      return CSGModelFromPolygons(polygons);
   }

   CSGModel Union(const CSGModel & a, const CSGModel & b);

   CSGModel Intersect(const CSGModel & a, const CSGModel & b);

   CSGModel Subtract(const CSGModel & a, const CSGModel & b);
}

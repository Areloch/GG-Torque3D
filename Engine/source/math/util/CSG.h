#pragma once
#include "math/mathUtils.h"
#include "collision/concretePolyList.h"
#include <list>
#include <vector>
#include <algorithm>

namespace CSGUtils
{
   struct csgjs_vector
   {
      float x, y, z;

      csgjs_vector() : x(0.0f), y(0.0f), z(0.0f) {}
      explicit csgjs_vector(float x, float y, float z) : x(x), y(y), z(z) {}
   };

   struct csgjs_vertex
   {
      csgjs_vector pos;
      csgjs_vector normal;
      csgjs_vector uv;
   };

   struct csgjs_model
   {
      std::vector<csgjs_vertex> vertices;
      std::vector<int> indices;
   };

   // public interface - not super efficient, if you use multiple CSG operations you should
   // use BSP trees and convert them into model only once. Another optimization trick is
   // replacing csgjs_model with your own class.

   csgjs_model csgjs_union(const csgjs_model & a, const csgjs_model & b);
   csgjs_model csgjs_intersection(const csgjs_model & a, const csgjs_model & b);
   csgjs_model csgjs_difference(const csgjs_model & a, const csgjs_model & b);

   struct csgjs_plane;
   struct csgjs_polygon;
   struct csgjs_node;

   // Represents a plane in 3D space.
   struct csgjs_plane
   {
      csgjs_vector normal;
      float w;

      csgjs_plane();
      csgjs_plane(const csgjs_vector & a, const csgjs_vector & b, const csgjs_vector & c);
      bool ok() const;
      void flip();
      void splitPolygon(const csgjs_polygon & polygon, std::vector<csgjs_polygon> & coplanarFront, std::vector<csgjs_polygon> & coplanarBack, std::vector<csgjs_polygon> & front, std::vector<csgjs_polygon> & back) const;
   };

   // Represents a convex polygon. The vertices used to initialize a polygon must
   // be coplanar and form a convex loop. They do not have to be `CSG.Vertex`
   // instances but they must behave similarly (duck typing can be used for
   // customization).
   // 
   // Each convex polygon has a `shared` property, which is shared between all
   // polygons that are clones of each other or were split from the same polygon.
   // This can be used to define per-polygon properties (such as surface color).
   struct csgjs_polygon
   {
      std::vector<csgjs_vertex> vertices;
      csgjs_plane plane;
      void flip();

      csgjs_polygon();
      csgjs_polygon(const std::vector<csgjs_vertex> & list);
   };

   // Holds a node in a BSP tree. A BSP tree is built from a collection of polygons
   // by picking a polygon to split along. That polygon (and all other coplanar
   // polygons) are added directly to that node and the other polygons are added to
   // the front and/or back subtrees. This is not a leafy BSP tree since there is
   // no distinction between internal and leaf nodes.
   struct csgjs_csgnode
   {
      std::vector<csgjs_polygon> polygons;
      csgjs_csgnode * front;
      csgjs_csgnode * back;
      csgjs_plane plane;

      csgjs_csgnode();
      csgjs_csgnode(const std::vector<csgjs_polygon> & list);
      ~csgjs_csgnode();

      csgjs_csgnode * clone() const;
      void clipTo(const csgjs_csgnode * other);
      void invert();
      void build(const std::vector<csgjs_polygon> & polygon);
      std::vector<csgjs_polygon> clipPolygons(const std::vector<csgjs_polygon> & list) const;
      std::vector<csgjs_polygon> allPolygons() const;
   };

   // std::vector implementation

   inline static csgjs_vector operator + (const csgjs_vector & a, const csgjs_vector & b) { return csgjs_vector(a.x + b.x, a.y + b.y, a.z + b.z); }
   inline static csgjs_vector operator - (const csgjs_vector & a, const csgjs_vector & b) { return csgjs_vector(a.x - b.x, a.y - b.y, a.z - b.z); }
   inline static csgjs_vector operator * (const csgjs_vector & a, float b) { return csgjs_vector(a.x * b, a.y * b, a.z * b); }
   inline static csgjs_vector operator / (const csgjs_vector & a, float b) { return a * (1.0f / b); }
   inline static float dot(const csgjs_vector & a, const csgjs_vector & b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
   inline static csgjs_vector lerp(const csgjs_vector & a, const csgjs_vector & b, float v) { return a + (b - a) * v; }
   inline static csgjs_vector negate(const csgjs_vector & a) { return a * -1.0f; }
   inline static float length(const csgjs_vector & a) { return sqrtf(dot(a, a)); }
   inline static csgjs_vector unit(const csgjs_vector & a) { return a / length(a); }
   inline static csgjs_vector cross(const csgjs_vector & a, const csgjs_vector & b) { return csgjs_vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

   // Vertex implementation

   // Invert all orientation-specific data (e.g. vertex normal). Called when the
   // orientation of a polygon is flipped.
   inline static csgjs_vertex flip(csgjs_vertex v)
   {
      v.normal = negate(v.normal);
      return v;
   }

   // Create a new vertex between this vertex and `other` by linearly
   // interpolating all properties using a parameter of `t`. Subclasses should
   // override this to interpolate additional properties.
   inline static csgjs_vertex interpolate(const csgjs_vertex & a, const csgjs_vertex & b, float t)
   {
      csgjs_vertex ret;
      ret.pos = lerp(a.pos, b.pos, t);
      ret.normal = lerp(a.normal, b.normal, t);
      ret.uv = lerp(a.uv, b.uv, t);
      return ret;
   }

   // Plane implementation

   // Node implementation

   // Return a new CSG solid representing space in either this solid or in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static csgjs_csgnode * csg_union(const csgjs_csgnode * a1, const csgjs_csgnode * b1)
   {
      csgjs_csgnode * a = a1->clone();
      csgjs_csgnode * b = b1->clone();
      a->clipTo(b);
      b->clipTo(a);
      b->invert();
      b->clipTo(a);
      b->invert();
      a->build(b->allPolygons());
      csgjs_csgnode * ret = new csgjs_csgnode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   // Return a new CSG solid representing space in this solid but not in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static csgjs_csgnode * csg_subtract(const csgjs_csgnode * a1, const csgjs_csgnode * b1)
   {
      csgjs_csgnode * a = a1->clone();
      csgjs_csgnode * b = b1->clone();
      a->invert();
      a->clipTo(b);
      b->clipTo(a);
      b->invert();
      b->clipTo(a);
      b->invert();
      a->build(b->allPolygons());
      a->invert();
      csgjs_csgnode * ret = new csgjs_csgnode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   // Return a new CSG solid representing space both this solid and in the
   // solid `csg`. Neither this solid nor the solid `csg` are modified.
   inline static csgjs_csgnode * csg_intersect(const csgjs_csgnode * a1, const csgjs_csgnode * b1)
   {
      csgjs_csgnode * a = a1->clone();
      csgjs_csgnode * b = b1->clone();
      a->invert();
      b->clipTo(a);
      b->invert();
      a->clipTo(b);
      b->clipTo(a);
      a->build(b->allPolygons());
      a->invert();
      csgjs_csgnode * ret = new csgjs_csgnode(a->allPolygons());
      delete a; a = 0;
      delete b; b = 0;
      return ret;
   }

   
   // Public interface implementation

   inline static std::vector<csgjs_polygon> csgjs_modelToPolygons(const csgjs_model & model)
   {
      std::vector<csgjs_polygon> list;
      for (size_t i = 0; i < model.indices.size(); i += 3)
      {
         std::vector<csgjs_vertex> triangle;
         for (int j = 0; j < 3; j++)
         {
            csgjs_vertex v = model.vertices[model.indices[i + j]];
            triangle.push_back(v);
         }
         list.push_back(csgjs_polygon(triangle));
      }
      return list;
   }

   inline static csgjs_model csgjs_modelFromPolygons(const std::vector<csgjs_polygon> & polygons)
   {
      csgjs_model model;
      int p = 0;
      for (size_t i = 0; i < polygons.size(); i++)
      {
         const csgjs_polygon & poly = polygons[i];
         for (size_t j = 2; j < poly.vertices.size(); j++)
         {
            model.vertices.push_back(poly.vertices[0]);		model.indices.push_back(p++);
            model.vertices.push_back(poly.vertices[j - 1]);	model.indices.push_back(p++);
            model.vertices.push_back(poly.vertices[j]);		model.indices.push_back(p++);
         }
      }
      return model;
   }

   typedef csgjs_csgnode * csg_function(const csgjs_csgnode * a1, const csgjs_csgnode * b1);

   inline static csgjs_model csgjs_operation(const csgjs_model & a, const csgjs_model & b, csg_function fun)
   {
      csgjs_csgnode * A = new csgjs_csgnode(csgjs_modelToPolygons(a));
      csgjs_csgnode * B = new csgjs_csgnode(csgjs_modelToPolygons(b));
      csgjs_csgnode * AB = fun(A, B);
      std::vector<csgjs_polygon> polygons = AB->allPolygons();
      delete A; A = 0;
      delete B; B = 0;
      delete AB; AB = 0;
      return csgjs_modelFromPolygons(polygons);
   }

   csgjs_model csgjs_union(const csgjs_model & a, const csgjs_model & b);

   csgjs_model csgjs_intersection(const csgjs_model & a, const csgjs_model & b);

   csgjs_model csgjs_difference(const csgjs_model & a, const csgjs_model & b);
}
// Original CSG.JS library by Evan Wallace (http://madebyevan.com), under the MIT license.
// GitHub: https://github.com/evanw/csg.js/
// 
// C++ port by Tomasz Dabrowski (http://28byteslater.com), under the MIT license.
// GitHub: https://github.com/dabroz/csgjs-cpp/
// 
// Constructive Solid Geometry (CSG) is a modeling technique that uses Boolean
// operations like union and intersection to combine 3D solids. This library
// implements CSG operations on meshes elegantly and concisely using BSP trees,
// and is meant to serve as an easily understandable implementation of the
// algorithm. All edge cases involving overlapping coplanar polygons in both
// solids are correctly handled.
//
// To use this as a header file, define CSGJS_HEADER_ONLY before including this file.
#include "CSG.h"

// `CSG.Plane.EPSILON` is the tolerance used by `splitPolygon()` to decide if a
// point is on the plane.
static const float csgjs_EPSILON = 0.00001f;

namespace CSGUtils
{ 
   // Plane implementation
   CSGPlane::CSGPlane() : normal(), w(0.0f)
   {
   }

   bool CSGPlane::ok() const
   {
      return length(this->normal) > 0.0f;
   }

   void CSGPlane::flip()
   {
      this->normal = negate(this->normal);
      this->w *= -1.0f;
   }

   CSGPlane::CSGPlane(const CSGVector & a, const CSGVector & b, const CSGVector & c)
   {
      this->normal = unit(cross(b - a, c - a));
      this->w = dot(this->normal, a);
   }

   // Split `polygon` by this plane if needed, then put the polygon or polygon
   // fragments in the appropriate lists. Coplanar polygons go into either
   // `coplanarFront` or `coplanarBack` depending on their orientation with
   // respect to this plane. Polygons in front or in back of this plane go into
   // either `front` or `back`.
   void CSGPlane::splitPolygon(const CSGPolygon & polygon, std::vector<CSGPolygon> & coplanarFront, std::vector<CSGPolygon> & coplanarBack, std::vector<CSGPolygon> & front, std::vector<CSGPolygon> & back) const
   {
      enum
      {
         COPLANAR = 0,
         FRONT = 1,
         BACK = 2,
         SPANNING = 3
      };

      // Classify each point as well as the entire polygon into one of the above
      // four classes.
      int polygonType = 0;
      std::vector<int> types;

      for (size_t i = 0; i < polygon.vertices.size(); i++)
      {
         float t = dot(this->normal, polygon.vertices[i].pos) - this->w;
         int type = (t < -csgjs_EPSILON) ? BACK : ((t > csgjs_EPSILON) ? FRONT : COPLANAR);
         polygonType |= type;
         types.push_back(type);
      }

      // Put the polygon in the correct list, splitting it when necessary.
      switch (polygonType)
      {
         case COPLANAR:
         {
            if (dot(this->normal, polygon.plane.normal) > 0)
               coplanarFront.push_back(polygon);
            else
               coplanarBack.push_back(polygon);
            break;
         }
         case FRONT:
         {
            front.push_back(polygon);
            break;
         }
         case BACK:
         {
            back.push_back(polygon);
            break;
         }
         case SPANNING:
         {
            std::vector<CSGVertex> f, b;
            for (size_t i = 0; i < polygon.vertices.size(); i++)
            {
               int j = (i + 1) % polygon.vertices.size();
               int ti = types[i], tj = types[j];
               CSGVertex vi = polygon.vertices[i], vj = polygon.vertices[j];
               if (ti != BACK) f.push_back(vi);
               if (ti != FRONT) b.push_back(vi);
               if ((ti | tj) == SPANNING)
               {
                  float t = (this->w - dot(this->normal, vi.pos)) / dot(this->normal, vj.pos - vi.pos);
                  CSGVertex v = interpolate(vi, vj, t);
                  f.push_back(v);
                  b.push_back(v);
               }
            }
            if (f.size() >= 3)
            {
               CSGPolygon newPoly = CSGPolygon(f);
               newPoly.materialId = polygon.materialId;
               front.push_back(newPoly);
            }
            if (b.size() >= 3)
            {
               CSGPolygon newPoly = CSGPolygon(b);
               newPoly.materialId = polygon.materialId;
               back.push_back(newPoly);
            }
            break;
         }
      }
   }

   // Polygon implementation

   void CSGPolygon::flip()
   {
      std::reverse(vertices.begin(), vertices.end());
      for (size_t i = 0; i < vertices.size(); i++)
         vertices[i].normal = negate(vertices[i].normal);
      plane.flip();
   }

   CSGPolygon::CSGPolygon()
   {
   }

   CSGPolygon::CSGPolygon(const std::vector<CSGVertex> & list) : vertices(list), plane(vertices[0].pos, vertices[1].pos, vertices[2].pos)
   {
   }

   // Convert solid space to empty space and empty space to solid space.
   void CSGNode::invert()
   {
      std::list<CSGNode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         CSGNode *me = nodes.front();
         nodes.pop_front();

         for (size_t i = 0; i < me->polygons.size(); i++)
            me->polygons[i].flip();
         me->plane.flip();
         std::swap(me->front, me->back);
         if (me->front)
            nodes.push_back(me->front);
         if (me->back)
            nodes.push_back(me->back);
      }
   }

   // Recursively remove all polygons in `polygons` that are inside this BSP
   // tree.
   std::vector<CSGPolygon> CSGNode::clipPolygons(const std::vector<CSGPolygon> & list) const
   {
      std::vector<CSGPolygon> result;

      std::list<std::pair<const CSGNode * const, std::vector<CSGPolygon> > > clips;
      clips.push_back(std::make_pair(this, list));
      while (clips.size())
      {
         const CSGNode        *me = clips.front().first;
         std::vector<CSGPolygon> list = clips.front().second;
         clips.pop_front();

         if (!me->plane.ok())
         {
            result.insert(result.end(), list.begin(), list.end());
            continue;
         }

         std::vector<CSGPolygon> list_front, list_back;
         for (size_t i = 0; i < list.size(); i++)
            me->plane.splitPolygon(list[i], list_front, list_back, list_front, list_back);

         if (me->front)
            clips.push_back(std::make_pair(me->front, list_front));
         else
            result.insert(result.end(), list_front.begin(), list_front.end());

         if (me->back)
            clips.push_back(std::make_pair(me->back, list_back));
      }

      return result;
   }

   // Remove all polygons in this BSP tree that are inside the other BSP tree
   // `bsp`.
   void CSGNode::clipTo(const CSGNode * other)
   {
      std::list<CSGNode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         CSGNode *me = nodes.front();
         nodes.pop_front();

         me->polygons = other->clipPolygons(me->polygons);
         if (me->front)
            nodes.push_back(me->front);
         if (me->back)
            nodes.push_back(me->back);
      }
   }

   // Return a list of all polygons in this BSP tree.
   std::vector<CSGPolygon> CSGNode::allPolygons() const
   {
      std::vector<CSGPolygon> result;

      std::list<const CSGNode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         const CSGNode        *me = nodes.front();
         nodes.pop_front();

         result.insert(result.end(), me->polygons.begin(), me->polygons.end());
         if (me->front)
            nodes.push_back(me->front);
         if (me->back)
            nodes.push_back(me->back);
      }

      return result;
   }

   CSGNode * CSGNode::clone() const
   {
      CSGNode * ret = new CSGNode();

      std::list<std::pair<const CSGNode *, CSGNode *> > nodes;
      nodes.push_back(std::make_pair(this, ret));
      while (nodes.size())
      {
         const CSGNode *original = nodes.front().first;
         CSGNode       *clone = nodes.front().second;
         nodes.pop_front();

         clone->polygons = original->polygons;
         clone->plane = original->plane;
         if (original->front)
         {
            clone->front = new CSGNode();
            nodes.push_back(std::make_pair(original->front, clone->front));
         }
         if (original->back)
         {
            clone->back = new CSGNode();
            nodes.push_back(std::make_pair(original->back, clone->back));
         }
      }

      return ret;
   }

   // Build a BSP tree out of `polygons`. When called on an existing tree, the
   // new polygons are filtered down to the bottom of the tree and become new
   // nodes there. Each set of polygons is partitioned using the first polygon
   // (no heuristic is used to pick a good split).
   void CSGNode::build(const std::vector<CSGPolygon> & list)
   {
      if (!list.size())
         return;

      std::list<std::pair<CSGNode *, std::vector<CSGPolygon> > > builds;
      builds.push_back(std::make_pair(this, list));

      U32 maxIterCount = 1000;
      U32 iterCount = 0;
      while (builds.size())
      {
         CSGNode              *me = builds.front().first;
         std::vector<CSGPolygon> list = builds.front().second;
         builds.pop_front();

         if (!me->plane.ok())
            me->plane = list[0].plane;
         std::vector<CSGPolygon> list_front, list_back;
         for (size_t i = 0; i < list.size(); i++)
            me->plane.splitPolygon(list[i], me->polygons, me->polygons, list_front, list_back);
         if (list_front.size())
         {
            if (!me->front)
               me->front = new CSGNode;
            builds.push_back(std::make_pair(me->front, list_front));
         }
         if (list_back.size())
         {
            if (!me->back)
               me->back = new CSGNode;
            builds.push_back(std::make_pair(me->back, list_back));
         }

         iterCount++;

         //sanity check, as some ops cause it to infinite loop
         //TODO: figure out what causes the infinite loop and fix the root issue
         if (iterCount > maxIterCount)
            return;
      }
   }

   CSGNode::CSGNode() : front(0), back(0)
   {
   }

   CSGNode::CSGNode(const std::vector<CSGPolygon> & list) : front(0), back(0)
   {
      build(list);
   }

   CSGNode::~CSGNode()
   {
      std::list<CSGNode *> nodes_to_delete;

      std::list<CSGNode *> nodes_to_disassemble;
      nodes_to_disassemble.push_back(this);
      while (nodes_to_disassemble.size())
      {
         CSGNode *me = nodes_to_disassemble.front();
         nodes_to_disassemble.pop_front();

         if (me->front)
         {
            nodes_to_disassemble.push_back(me->front);
            nodes_to_delete.push_back(me->front);
            me->front = NULL;
         }
         if (me->back)
         {
            nodes_to_disassemble.push_back(me->back);
            nodes_to_delete.push_back(me->back);
            me->back = NULL;
         }
      }

      for (std::list<CSGNode *>::iterator it = nodes_to_delete.begin(); it != nodes_to_delete.end(); ++it)
         delete *it;
   }

   CSGModel Union(const CSGModel & a, const CSGModel & b)
   {
      return csgjs_operation(a, b, csg_union);
   }

   CSGModel Intersect(const CSGModel & a, const CSGModel & b)
   {
      return csgjs_operation(a, b, csg_intersect);
   }

   CSGModel Subtract(const CSGModel & a, const CSGModel & b)
   {
      return csgjs_operation(a, b, csg_subtract);
   }
}

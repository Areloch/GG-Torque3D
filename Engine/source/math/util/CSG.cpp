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
   csgjs_plane::csgjs_plane() : normal(), w(0.0f)
   {
   }

   bool csgjs_plane::ok() const
   {
      return length(this->normal) > 0.0f;
   }

   void csgjs_plane::flip()
   {
      this->normal = negate(this->normal);
      this->w *= -1.0f;
   }

   csgjs_plane::csgjs_plane(const csgjs_vector & a, const csgjs_vector & b, const csgjs_vector & c)
   {
      this->normal = unit(cross(b - a, c - a));
      this->w = dot(this->normal, a);
   }

   // Split `polygon` by this plane if needed, then put the polygon or polygon
   // fragments in the appropriate lists. Coplanar polygons go into either
   // `coplanarFront` or `coplanarBack` depending on their orientation with
   // respect to this plane. Polygons in front or in back of this plane go into
   // either `front` or `back`.
   void csgjs_plane::splitPolygon(const csgjs_polygon & polygon, std::vector<csgjs_polygon> & coplanarFront, std::vector<csgjs_polygon> & coplanarBack, std::vector<csgjs_polygon> & front, std::vector<csgjs_polygon> & back) const
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
            std::vector<csgjs_vertex> f, b;
            for (size_t i = 0; i < polygon.vertices.size(); i++)
            {
               int j = (i + 1) % polygon.vertices.size();
               int ti = types[i], tj = types[j];
               csgjs_vertex vi = polygon.vertices[i], vj = polygon.vertices[j];
               if (ti != BACK) f.push_back(vi);
               if (ti != FRONT) b.push_back(vi);
               if ((ti | tj) == SPANNING)
               {
                  float t = (this->w - dot(this->normal, vi.pos)) / dot(this->normal, vj.pos - vi.pos);
                  csgjs_vertex v = interpolate(vi, vj, t);
                  f.push_back(v);
                  b.push_back(v);
               }
            }
            if (f.size() >= 3) front.push_back(csgjs_polygon(f));
            if (b.size() >= 3) back.push_back(csgjs_polygon(b));
            break;
         }
      }
   }

   // Polygon implementation

   void csgjs_polygon::flip()
   {
      std::reverse(vertices.begin(), vertices.end());
      for (size_t i = 0; i < vertices.size(); i++)
         vertices[i].normal = negate(vertices[i].normal);
      plane.flip();
   }

   csgjs_polygon::csgjs_polygon()
   {
   }

   csgjs_polygon::csgjs_polygon(const std::vector<csgjs_vertex> & list) : vertices(list), plane(vertices[0].pos, vertices[1].pos, vertices[2].pos)
   {
   }

   // Convert solid space to empty space and empty space to solid space.
   void csgjs_csgnode::invert()
   {
      std::list<csgjs_csgnode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         csgjs_csgnode *me = nodes.front();
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
   std::vector<csgjs_polygon> csgjs_csgnode::clipPolygons(const std::vector<csgjs_polygon> & list) const
   {
      std::vector<csgjs_polygon> result;

      std::list<std::pair<const csgjs_csgnode * const, std::vector<csgjs_polygon> > > clips;
      clips.push_back(std::make_pair(this, list));
      while (clips.size())
      {
         const csgjs_csgnode        *me = clips.front().first;
         std::vector<csgjs_polygon> list = clips.front().second;
         clips.pop_front();

         if (!me->plane.ok())
         {
            result.insert(result.end(), list.begin(), list.end());
            continue;
         }

         std::vector<csgjs_polygon> list_front, list_back;
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
   void csgjs_csgnode::clipTo(const csgjs_csgnode * other)
   {
      std::list<csgjs_csgnode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         csgjs_csgnode *me = nodes.front();
         nodes.pop_front();

         me->polygons = other->clipPolygons(me->polygons);
         if (me->front)
            nodes.push_back(me->front);
         if (me->back)
            nodes.push_back(me->back);
      }
   }

   // Return a list of all polygons in this BSP tree.
   std::vector<csgjs_polygon> csgjs_csgnode::allPolygons() const
   {
      std::vector<csgjs_polygon> result;

      std::list<const csgjs_csgnode *> nodes;
      nodes.push_back(this);
      while (nodes.size())
      {
         const csgjs_csgnode        *me = nodes.front();
         nodes.pop_front();

         result.insert(result.end(), me->polygons.begin(), me->polygons.end());
         if (me->front)
            nodes.push_back(me->front);
         if (me->back)
            nodes.push_back(me->back);
      }

      return result;
   }

   csgjs_csgnode * csgjs_csgnode::clone() const
   {
      csgjs_csgnode * ret = new csgjs_csgnode();

      std::list<std::pair<const csgjs_csgnode *, csgjs_csgnode *> > nodes;
      nodes.push_back(std::make_pair(this, ret));
      while (nodes.size())
      {
         const csgjs_csgnode *original = nodes.front().first;
         csgjs_csgnode       *clone = nodes.front().second;
         nodes.pop_front();

         clone->polygons = original->polygons;
         clone->plane = original->plane;
         if (original->front)
         {
            clone->front = new csgjs_csgnode();
            nodes.push_back(std::make_pair(original->front, clone->front));
         }
         if (original->back)
         {
            clone->back = new csgjs_csgnode();
            nodes.push_back(std::make_pair(original->back, clone->back));
         }
      }

      return ret;
   }

   // Build a BSP tree out of `polygons`. When called on an existing tree, the
   // new polygons are filtered down to the bottom of the tree and become new
   // nodes there. Each set of polygons is partitioned using the first polygon
   // (no heuristic is used to pick a good split).
   void csgjs_csgnode::build(const std::vector<csgjs_polygon> & list)
   {
      if (!list.size())
         return;

      std::list<std::pair<csgjs_csgnode *, std::vector<csgjs_polygon> > > builds;
      builds.push_back(std::make_pair(this, list));
      while (builds.size())
      {
         csgjs_csgnode              *me = builds.front().first;
         std::vector<csgjs_polygon> list = builds.front().second;
         builds.pop_front();

         if (!me->plane.ok())
            me->plane = list[0].plane;
         std::vector<csgjs_polygon> list_front, list_back;
         for (size_t i = 0; i < list.size(); i++)
            me->plane.splitPolygon(list[i], me->polygons, me->polygons, list_front, list_back);
         if (list_front.size())
         {
            if (!me->front)
               me->front = new csgjs_csgnode;
            builds.push_back(std::make_pair(me->front, list_front));
         }
         if (list_back.size())
         {
            if (!me->back)
               me->back = new csgjs_csgnode;
            builds.push_back(std::make_pair(me->back, list_back));
         }
      }
   }

   csgjs_csgnode::csgjs_csgnode() : front(0), back(0)
   {
   }

   csgjs_csgnode::csgjs_csgnode(const std::vector<csgjs_polygon> & list) : front(0), back(0)
   {
      build(list);
   }

   csgjs_csgnode::~csgjs_csgnode()
   {
      std::list<csgjs_csgnode *> nodes_to_delete;

      std::list<csgjs_csgnode *> nodes_to_disassemble;
      nodes_to_disassemble.push_back(this);
      while (nodes_to_disassemble.size())
      {
         csgjs_csgnode *me = nodes_to_disassemble.front();
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

      for (std::list<csgjs_csgnode *>::iterator it = nodes_to_delete.begin(); it != nodes_to_delete.end(); ++it)
         delete *it;
   }

   csgjs_model csgjs_union(const csgjs_model & a, const csgjs_model & b)
   {
      return csgjs_operation(a, b, csg_union);
   }

   csgjs_model csgjs_intersection(const csgjs_model & a, const csgjs_model & b)
   {
      return csgjs_operation(a, b, csg_intersect);
   }

   csgjs_model csgjs_difference(const csgjs_model & a, const csgjs_model & b)
   {
      return csgjs_operation(a, b, csg_subtract);
   }
}
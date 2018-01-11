#pragma once
// Copyright (c) 2011 Evan Wallace (http://madebyevan.com/), under the MIT license.
// Copyright (c) 2017 Jeff Hutchinson, under the MIT license. (C++ Port)
// Portions of the C++ port by Tomasz Dabrowski (http://28byteslater.com), under the MIT license.

#include <vector>
#include <algorithm>
#include <memory>

const float CSG_EPSILON = 0.00001f;

struct CSGVector;
struct CSGPolygon;

enum PolygonType : unsigned int
{
   COPLANAR = 0,
   FRONT = 1,
   BACK = 2,
   SPANNING = 3
};

struct CSGVector
{
   float x;
   float y;
   float z;

   inline CSGVector clone() const
   {
      return
      {
         x,
         y,
         z
      };
   }

   inline CSGVector negated() const
   {
      return
      {
         -x,
         -y,
         -z
      };
   }

   inline CSGVector plus(const CSGVector &vec) const
   {
      return
      {
         x + vec.x,
         y + vec.y,
         z + vec.z
      };
   }

   inline CSGVector minus(const CSGVector &vec) const
   {
      return
      {
         x - vec.x,
         y - vec.y,
         z - vec.z
      };
   }

   inline CSGVector times(float a) const
   {
      return
      {
         x * a,
         y * a,
         z * a
      };
   }

   inline CSGVector dividedBy(float a) const
   {
      return
      {
         x / a,
         y / a,
         z / a
      };
   }

   inline float dot(const CSGVector &vec) const
   {
      return x * vec.x + y * vec.y + z * vec.z;
   }

   inline CSGVector lerp(const CSGVector &vec, float time) const
   {
      return plus(vec.minus(*this).times(time));
   }

   inline float length() const
   {
      return sqrtf(dot(*this));
   }

   inline CSGVector unit() const
   {
      return dividedBy(length());
   }

   inline CSGVector cross(const CSGVector &vec) const
   {
      return
      {
         y * vec.z - z * vec.y,
         z * vec.x - x * vec.z,
         x * vec.y - y * vec.x
      };
   }

   inline CSGVector& operator=(const CSGVector &vec)
   {
      x = vec.x;
      y = vec.y;
      z = vec.z;
      return *this;
   }
};

struct CSGVertex
{
   CSGVector position;
   CSGVector normal;
   CSGVector uv;

   CSGVertex() {}

   CSGVertex(const CSGVector &pos, const CSGVector &nor, const CSGVector &_uv)
   {
      position = pos;
      normal = nor;
      uv = _uv;
   }

   inline CSGVertex clone() const
   {
      return CSGVertex(position.clone(), normal.clone(), uv.clone());
   }

   inline void flip()
   {
      normal = normal.negated();
   }

   inline CSGVertex interpolate(const CSGVertex &vex, float time) const
   {
      return
      {
         position.lerp(vex.position, time),
         normal.lerp(vex.normal, time),
         uv.lerp(vex.uv, time)
      };
   }

   inline CSGVertex& operator=(const CSGVertex &vec)
   {
      position = vec.position;
      normal = vec.normal;
      return *this;
   }
};

struct CSGPlane
{
   static CSGPlane fromPoints(const CSGVector &a, const CSGVector &b, const CSGVector &c);

   CSGVector normal;
   float w;

   inline CSGPlane clone() const
   {
      return
      {
         normal.clone(),
         w
      };
   }

   inline void flip()
   {
      normal = normal.negated();
      w = -w;
   }

   void splitPolygon(
      const CSGPolygon &polygon,
      std::vector<CSGPolygon> &coplanerFront,
      std::vector<CSGPolygon> &coplanerBack,
      std::vector<CSGPolygon> &front,
      std::vector<CSGPolygon> &back
   );
};

struct CSGPolygon
{
   std::vector<CSGVertex> vertices;
   bool shared;
   CSGPlane plane;

   CSGPolygon(const std::vector<CSGVertex> &verts, bool isShared)
   {
      vertices = verts;
      shared = isShared;

      // Calculate plane.
      plane = plane.fromPoints(verts[0].position, verts[1].position, verts[2].position);
   }

   inline CSGPolygon clone() const
   {
      // Copy the verts. memcpy
      std::vector<CSGVertex> verts(vertices.size());
      for (size_t i = 0; i < vertices.size(); ++i)
         verts[i] = vertices[i].clone();

      return CSGPolygon(verts, shared);
   }

   inline void flip()
   {
      // Reverse and flip every vert
      std::reverse(vertices.begin(), vertices.end());
      for (CSGVertex &vert : vertices)
         vert.flip();
      plane.flip();
   }
};

struct CSGNode
{
   CSGPlane plane;
   bool planeIsSet;
   std::unique_ptr<CSGNode> front;
   std::unique_ptr<CSGNode> back;
   std::vector<CSGPolygon> polygons;

   CSGNode()
   {
      front = nullptr;
      back = nullptr;
      planeIsSet = false;
   }

   CSGNode(const std::vector<CSGPolygon> &polys)
   {
      front = nullptr;
      back = nullptr;
      planeIsSet = false;
      build(polys);
   }

   std::unique_ptr<CSGNode> clone()
   {
      std::unique_ptr<CSGNode> node = std::make_unique<CSGNode>();
      if (planeIsSet)
         node->plane = plane.clone();
      if (front)
         node->front = front->clone();
      if (back)
         node->back = back->clone();

      // clone polies.
      node->polygons.reserve(polygons.size());
      for (size_t i = 0; i < polygons.size(); ++i)
         node->polygons[i] = polygons[i].clone();
      return node;
   }

   void invert()
   {
      for (CSGPolygon &poly : polygons)
         poly.flip();
      plane.flip();
      if (front)
         front->invert();
      if (back)
         back->invert();

      if (front)
         front.swap(back);
      else if (back)
         back.swap(front);
   }

   std::vector<CSGPolygon> clipPolygons(const std::vector<CSGPolygon> &polyList)
   {
      if (!planeIsSet)
         return polyList; // return copy of polies. No clipping if no plane.
      std::vector<CSGPolygon> frontList;
      std::vector<CSGPolygon> backList;

      // split all polygons
      for (const CSGPolygon &polygon : polyList)
         plane.splitPolygon(polygon, frontList, backList, frontList, backList);
      if (front)
         frontList = front->clipPolygons(frontList);
      if (back)
         backList = back->clipPolygons(backList);
      else
         backList.clear();

      // Concat backlist into frontlist
      frontList.insert(frontList.end(), backList.begin(), backList.end());
      return frontList;
   }

   void clipTo(CSGNode *node)
   {
      polygons = node->clipPolygons(polygons);
      if (front)
         front->clipTo(node);
      if (back)
         back->clipTo(node);
   }

   std::vector<CSGPolygon> allPolygons()
   {
      // Copy polygons
      std::vector<CSGPolygon> polys = polygons;
      if (front)
      {
         std::vector<CSGPolygon> frontGons = front->allPolygons();
         polygons.insert(polygons.end(), frontGons.begin(), frontGons.end());
      }
      if (back)
      {
         std::vector<CSGPolygon> backGons = back->allPolygons();
         polygons.insert(polygons.end(), backGons.begin(), backGons.end());
      }
      return polys;
   }

   void build(const std::vector<CSGPolygon> &polies)
   {
      if (polies.empty())
         return;
      if (!planeIsSet)
      {
         planeIsSet = true;
         plane = polies[0].plane.clone();
      }

      std::vector<CSGPolygon> frontgons;
      std::vector<CSGPolygon> backgons;

      for (const CSGPolygon &poly : polies)
         plane.splitPolygon(poly, polygons, polygons, frontgons, backgons);

      if (frontgons.size())
      {
         if (!front)
            front = std::make_unique<CSGNode>();
         front->build(frontgons);
      }
      if (backgons.size())
      {
         if (!back)
            back = std::make_unique<CSGNode>();
         back->build(backgons);
      }
   }
};

struct CSGModel
{
   std::vector<CSGVertex> vertices;
   std::vector<int> indices;
};

class CSG
{
public:
   static CSG fromPolygons(const std::vector<CSGPolygon> &polies);

   CSG clone() const;
   const std::vector<CSGPolygon>& toPolygons() const;

   CSGModel toModel() const;

   CSG csg_union(const CSG &csg);
   CSG csg_subtract(const CSG &csg);
   CSG csg_intersect(const CSG &csg);
   CSG csg_inverse() const;

private:
   std::vector<CSGPolygon> mPolygons;
};
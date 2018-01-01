// Copyright (c) 2011 Evan Wallace (http://madebyevan.com/), under the MIT license.
// Copyright (c) 2017 Jeff Hutchinson, under the MIT license. (C++ Port)
// Portions of the C++ port by Tomasz Dabrowski (http://28byteslater.com), under the MIT license.

#include "csg.hpp"

CSGPlane CSGPlane::fromPoints(const CSGVector &a, const CSGVector &b, const CSGVector &c)
{
   CSGVector n = b.minus(a).cross(c.minus(a)).unit();

   CSGPlane plane;
   plane.normal = n;
   plane.w = n.dot(a);
   return plane;
}

void CSGPlane::splitPolygon(
   const CSGPolygon &polygon,
   std::vector<CSGPolygon> &coplanerFront,
   std::vector<CSGPolygon> &coplanerBack,
   std::vector<CSGPolygon> &front,
   std::vector<CSGPolygon> &back
)
{
   unsigned int polygonType = 0;
   std::vector<unsigned int> types;

   for (size_t i = 0; i < polygon.vertices.size(); ++i)
   {
      auto t = normal.dot(polygon.vertices[i].position) - w;
      PolygonType type = (t < -CSG_EPSILON) ? PolygonType::BACK : (t > CSG_EPSILON) ? PolygonType::FRONT : PolygonType::COPLANAR;
      polygonType |= type;
      types.push_back(type);
   }

   switch (polygonType)
   {
      case PolygonType::COPLANAR:
         if (normal.dot(polygon.plane.normal) > 0)
            coplanerFront.push_back(polygon);
         else
            coplanerBack.push_back(polygon);
         break;
      case PolygonType::FRONT:
         front.push_back(polygon);
         break;
      case PolygonType::BACK:
         back.push_back(polygon);
         break;
      case PolygonType::SPANNING:
         std::vector<CSGVertex> f;
         std::vector<CSGVertex> b;
         for (size_t i = 0; i < polygon.vertices.size(); ++i)
         {
            size_t j = (i + 1) % polygon.vertices.size();
            unsigned int ti = types[i];
            unsigned int tj = types[j];
            CSGVertex vi = polygon.vertices[i];
            CSGVertex vj = polygon.vertices[j];

            if (ti != PolygonType::BACK)
               f.push_back(vi);
            if (ti != PolygonType::FRONT)
               b.push_back(ti == PolygonType::BACK ? vi.clone() : vi);
            if ((ti | tj) == PolygonType::SPANNING)
            {
               float t = (w - normal.dot(vi.position)) / normal.dot(vj.position.minus(vi.position));
               const CSGVertex &v = vi.interpolate(vj, t);
               f.push_back(v);
               b.push_back(v.clone());
            }
         }
         if (f.size() >= 3)
            front.push_back(CSGPolygon(f, polygon.shared));
         if (b.size() >= 3)
            back.push_back(CSGPolygon(b, polygon.shared));
         break;
   }
}

//-----------------------------------------------------------------------------
// CSG Class
//-----------------------------------------------------------------------------

CSG CSG::fromPolygons(const std::vector<CSGPolygon> &polies)
{
   CSG csg;
   csg.mPolygons = polies;
   return csg;
}

CSG CSG::clone() const
{
   CSG csg;

   // memcpy
   csg.mPolygons.reserve(mPolygons.size());
   for (size_t i = 0; i < mPolygons.size(); ++i)
      csg.mPolygons[i] = mPolygons[i].clone();

   return csg;
}

const std::vector<CSGPolygon>& CSG::toPolygons() const
{
   return mPolygons;
}

CSGModel CSG::toModel() const
{
   CSGModel model;
   int index = 0;
   for (const CSGPolygon poly : mPolygons)
   {
      for (size_t j = 2; j < poly.vertices.size(); ++j)
      {
         model.vertices.push_back(poly.vertices[0]);
         model.indices.push_back(index++);
         model.vertices.push_back(poly.vertices[j - 1]);
         model.indices.push_back(index++);
         model.vertices.push_back(poly.vertices[j]);
         model.indices.push_back(index++);
      }
   }
   return model;
   return model;
}

CSG CSG::csg_union(const CSG &csg)
{
   std::unique_ptr<CSGNode> a = std::make_unique<CSGNode>(clone().mPolygons);
   std::unique_ptr<CSGNode> b = std::make_unique<CSGNode>(csg.clone().mPolygons);
   a->clipTo(b.get());
   b->clipTo(a.get());
   b->invert();
   b->clipTo(a.get());
   b->invert();
   a->build(b->allPolygons());
   return CSG::fromPolygons(a->allPolygons());
}

CSG CSG::csg_subtract(const CSG &csg)
{
   std::unique_ptr<CSGNode> a = std::make_unique<CSGNode>(clone().mPolygons);
   std::unique_ptr<CSGNode> b = std::make_unique<CSGNode>(csg.clone().mPolygons);
   a->invert();
   a->clipTo(b.get());
   b->clipTo(a.get());
   b->invert();
   b->clipTo(a.get());
   b->invert();
   a->build(b->allPolygons());
   a->invert();
   return CSG::fromPolygons(a->allPolygons());
}

CSG CSG::csg_intersect(const CSG &csg)
{
   std::unique_ptr<CSGNode> a = std::make_unique<CSGNode>(clone().mPolygons);
   std::unique_ptr<CSGNode> b = std::make_unique<CSGNode>(csg.clone().mPolygons);
   a->invert();
   b->clipTo(a.get());
   b->invert();
   a->clipTo(b.get());
   b->clipTo(a.get());
   a->build(b->allPolygons());
   a->invert();
   return CSG::fromPolygons(a->allPolygons());
}

CSG CSG::csg_inverse() const
{
   CSG csg = clone();
   for (CSGPolygon &poly : csg.mPolygons)
      poly.flip();
   return CSG();
}
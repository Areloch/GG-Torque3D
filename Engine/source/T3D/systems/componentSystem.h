#pragma once
#include "console/engineAPI.h"

#include "T3D/components/component.h"

template<typename T>
class SystemInterface
{
public:
   bool mIsEnabled;
   bool mIsServer;
   Component* mOwner;

   static Vector<T*> all;

   SystemInterface(Component* owner)
   {
	   this->mOwner = owner;
      all.push_back((T*)this);
   }

   virtual ~SystemInterface()
   {
      for (U32 i = 0; i < all.size(); i++)
      {
         if (all[i] == (T*)this)
         {
            all.erase(i);
            return;
         }
      }
   }

   static T* getByOwner(Component* owner)
   {
	   for (U32 i = 0; i < all.size(); i++)
	   {
		   if (all[i]->mOwner == owner)
			   return (T*)all[i];
	   }

	   return nullptr; //no luck
   }

   static T* getByOwner(Entity* owner)
   {
      for (U32 i = 0; i < all.size(); i++)
      {
         if (all[i]->mOwner->mOwner == owner)
            return (T*)all[i];
      }

      return nullptr; //no luck
   }
};
template<typename T> Vector<T*> SystemInterface<T>::all(0);
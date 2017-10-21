//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------


#include "platform/platform.h"
#include "console/simFieldDictionary.h"

#include "console/console.h"
#include "console/consoleInternal.h"
#include "core/frameAllocator.h"

U32 SimFieldDictionary::getHashValue( StringTableEntry slotName )
{
   return HashPointer( slotName ) % HashTableSize;
}

U32 SimFieldDictionary::getHashValue( const String& fieldName )
{
   return getHashValue( StringTable->insert( fieldName ) );
}

SimFieldDictionary::Entry *SimFieldDictionary::addEntry( U32 bucket, StringTableEntry slotName, ConsoleBaseType* type, char* value )
{
   Entry ret;
   ret.slotName  = slotName;
   ret.type      = type;
   ret.value     = value;

   mNumFields++;
   mVersion++;

   mHashTable[bucket].push_back(std::move(ret));
   return &mHashTable[bucket].back();
}

void SimFieldDictionary::freeEntry(SimFieldDictionary::Entry *ent)
{
   auto &vec = mHashTable[getHashValue(ent->slotName)];

   // Find the slot.
   auto iter = std::find_if(vec.begin(), vec.end(), [&](const Entry &ref) -> bool {
      return ref.slotName == ent->slotName;
   });
   if (iter != vec.end())
   {
      vec.erase(iter);
      mNumFields--;
   }
}

SimFieldDictionary::SimFieldDictionary()
:  mNumFields( 0 ),
   mVersion( 0 )
{

}

SimFieldDictionary::~SimFieldDictionary()
{

}

void SimFieldDictionary::setFieldType(StringTableEntry slotName, const char *typeString)
{
   ConsoleBaseType *cbt = ConsoleBaseType::getTypeByName( typeString );
   setFieldType(slotName, cbt);
}

void SimFieldDictionary::setFieldType(StringTableEntry slotName, const U32 typeId)
{
   ConsoleBaseType *cbt = ConsoleBaseType::getType(typeId);
   setFieldType(slotName, cbt);
}

void SimFieldDictionary::setFieldType(StringTableEntry slotName, ConsoleBaseType *type)
{
   // If the field exists on the object, set the type
   U32 bucket = getHashValue( slotName );

   for( Entry &ref : mHashTable[bucket] )
   {
      if( ref.slotName == slotName )
      {
         // Found and type assigned, let's bail
         ref.type = type;
         return;
      }
   }

   // Otherwise create the field, and set the type. Assign a null value.
   addEntry( bucket, slotName, type );
}

U32 SimFieldDictionary::getFieldType(StringTableEntry slotName) const
{
   U32 bucket = getHashValue( slotName );

   const std::vector<Entry> &vec = mHashTable[bucket];
   size_t size = vec.size();
   for (size_t i = 0; i < size; ++i)
   {
      const Entry &ref = vec[i];
      if (ref.slotName == slotName)
         return ref.type ? ref.type->getTypeID() : TypeString;
   }

   return TypeString;
}

SimFieldDictionary::Entry  *SimFieldDictionary::findDynamicField(const String &fieldName) const
{
   U32 bucket = getHashValue( fieldName );

   const std::vector<Entry> &vec = mHashTable[bucket];
   size_t size = vec.size();
   for (size_t i = 0; i < size; ++i)
   {
      const Entry &ref = vec[i];
      if( fieldName.equal(ref.slotName, String::NoCase) )
         return const_cast<Entry*>(&ref);
   }

   return NULL;
}

SimFieldDictionary::Entry *SimFieldDictionary::findDynamicField( StringTableEntry fieldName) const
{
   U32 bucket = getHashValue( fieldName );

   const std::vector<Entry> &vec = mHashTable[bucket];
   size_t size = vec.size();
   for (size_t i = 0; i < size; ++i)
   {
      if( vec[i].slotName == fieldName )
      {
         return const_cast<Entry*>(&vec[i]);
      }
   }

   return NULL;
}


void SimFieldDictionary::setFieldValue(StringTableEntry slotName, const char *value)
{
   U32 bucket = getHashValue(slotName);

   for (Entry &ref : mHashTable[bucket])
   {
      if (ref.slotName == slotName)
      {
         if (!value || !*value)
         {
            mVersion++;

            if (ref.value)
               dFree(ref.value);

            freeEntry(&ref);
         }
         else
         {
            if (ref.value)
               dFree(ref.value);

            ref.value = dStrdup(value);
         }

         return;
      }
   }

   // no field, add entry.
   addEntry(bucket, slotName, 0, dStrdup(value));
}

const char *SimFieldDictionary::getFieldValue(StringTableEntry slotName)
{
   U32 bucket = getHashValue(slotName);

   for (const Entry &ref : mHashTable[bucket])
      if(ref.slotName == slotName)
         return ref.value;

   return NULL;
}

void SimFieldDictionary::assignFrom(SimFieldDictionary *dict)
{
   mVersion++;

   for(U32 i = 0; i < HashTableSize; i++)
   {
      for (const Entry &ref : mHashTable[i])
      {
         setFieldValue(ref.slotName, ref.value);
         setFieldType(ref.slotName, ref.type);
      }
   }
}

static S32 QSORT_CALLBACK compareEntries(const void* a,const void* b)
{
   SimFieldDictionary::Entry *fa = *((SimFieldDictionary::Entry **)a);
   SimFieldDictionary::Entry *fb = *((SimFieldDictionary::Entry **)b);
   return dStricmp(fa->slotName, fb->slotName);
}

void SimFieldDictionary::writeFields(SimObject *obj, Stream &stream, U32 tabStop)
{
   const AbstractClassRep::FieldList &list = obj->getFieldList();
   Vector<Entry> flist(__FILE__, __LINE__);

   for(U32 i = 0; i < HashTableSize; i++)
   {
      for(const Entry &walk : mHashTable[i])
      {
         // make sure we haven't written this out yet:
         U32 i;
         for(i = 0; i < list.size(); i++)
            if(list[i].pFieldname == walk.slotName)
               break;

         if(i != list.size())
            continue;


         if (!obj->writeField(walk.slotName, walk.value))
            continue;

         flist.push_back(walk);
      }
   }

   // Sort Entries to prevent version control conflicts
   dQsort(flist.address(),flist.size(),sizeof(Entry *),compareEntries);

   // Save them out
   for(const Entry &ref : flist)
   {
      U32 nBufferSize = (dStrlen( ref.value ) * 2) + dStrlen( ref.slotName ) + 16;
      FrameTemp<char> expandedBuffer( nBufferSize );

      stream.writeTabs(tabStop+1);

      const char *typeName = ref.type && ref.type->getTypeID() != TypeString ? ref.type->getTypeName() : "";
      dSprintf(expandedBuffer, nBufferSize, "%s%s%s = \"", typeName, *typeName ? " " : "", ref.slotName);
      if ( ref.value )
         expandEscape((char*)expandedBuffer + dStrlen(expandedBuffer), ref.value);
      dStrcat(expandedBuffer, "\";\r\n");

      stream.write(dStrlen(expandedBuffer),expandedBuffer);
   }

}
void SimFieldDictionary::printFields(SimObject *obj)
{
   const AbstractClassRep::FieldList &list = obj->getFieldList();
   char expandedBuffer[4096];
   Vector<Entry> flist(__FILE__, __LINE__);

   for(U32 i = 0; i < HashTableSize; i++)
   {
      for (const Entry &walk : mHashTable[i])
      {
         // make sure we haven't written this out yet:
         U32 i;
         for(i = 0; i < list.size(); i++)
            if(list[i].pFieldname == walk.slotName)
               break;

         if(i != list.size())
            continue;

         flist.push_back(walk);
      }
   }
   dQsort(flist.address(),flist.size(),sizeof(Entry *),compareEntries);

   for (const Entry &ref : flist)
   {
      const char* type = "string";
      if( ref.type )
         type = ref.type->getTypeClassName();
         
      dSprintf( expandedBuffer, sizeof( expandedBuffer ), "  %s %s = \"", type, ref.slotName );
      if ( ref.value )
         expandEscape(expandedBuffer + dStrlen(expandedBuffer), ref.value);
      Con::printf("%s\"", expandedBuffer);
   }
}

SimFieldDictionary::Entry  *SimFieldDictionary::operator[](U32 index)
{
   AssertFatal ( index < mNumFields, "out of range" );

   if ( index > mNumFields )
      return NULL;

   SimFieldDictionaryIterator itr(this);

   for (S32 i = 0; i < index && *itr; i++)
      ++itr;

   return (*itr);
}

//------------------------------------------------------------------------------
SimFieldDictionaryIterator::SimFieldDictionaryIterator(SimFieldDictionary * dictionary)
{
   mDictionary = dictionary;
   mHashIndex = 0;
   mVecIndex = 1;
   mEntry = NULL;
   operator++();
}

SimFieldDictionary::Entry* SimFieldDictionaryIterator::operator++()
{
   std::vector<SimFieldDictionary::Entry> &vec = mDictionary->mHashTable[mHashIndex];

   if(!mDictionary || vec.size() == 0 || mHashIndex >= SimFieldDictionary::HashTableSize)
   {
      mEntry = NULL;
      return NULL;
   }

   ++mVecIndex;
   if (mVecIndex < vec.size())
   {
      mEntry = &vec[mVecIndex];
      return mEntry;
   }

   ++mHashIndex;

   if (mHashIndex >= SimFieldDictionary::HashTableSize)
   {
      mEntry = NULL;
      return NULL;
   }

   mVecIndex = 0;
   mEntry = &vec[0];

   return mEntry;
}

SimFieldDictionary::Entry* SimFieldDictionaryIterator::operator*()
{
   return mEntry;
}
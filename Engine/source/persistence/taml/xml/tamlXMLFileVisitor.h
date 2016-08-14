//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef TAML_XML_FILE_VISITOR_H
#define TAML_XML_FILE_VISITOR_H

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif

#ifndef _TAML_XMLPARSER_H_
#include "persistence/taml/xml/tamlXmlParser.h"
#endif

//-----------------------------------------------------------------------------

class TamlXmlFileVisitor : public SimObject, public TamlVisitor
{
private:
   typedef SimObject Parent;

protected:
   StringTableEntry mElementName;
   StringTableEntry mAttribName;
   StringTableEntry mAttribValue;
   bool mFound;

protected:
   virtual bool visit(TiXmlElement* pXmlElement, TamlXmlParser& xmlParser)
   {
      //// Notify Script.
      //if( isMethod("onVisitElement") )
      //	Con::executef(this, 1, "onVisitElement");

      StringTableEntry pName = StringTable->insert(StringTable->EmptyString);
      // Iterate attributes.
      for (TiXmlAttribute* pAttribute = pXmlElement->FirstAttribute(); pAttribute; pAttribute = pAttribute->Next())
      {
         if (dStrcmp(pAttribute->Name(), "Name") || dStrcmp(pAttribute->Name(), "name"))
            pName = StringTable->insert(pAttribute->Value());
         // Insert attribute name.
         StringTableEntry attributeName = StringTable->insert(pAttribute->Name());
         StringTableEntry attributeValue = StringTable->insert(pAttribute->Value());
         if (attributeName == mAttribName)
         {
            if (attributeValue == mAttribValue)
            {
               mElementName = pName;
               mFound = true;
               break;
            }
         }
      }
      return true;
   }

   virtual bool visit(TiXmlAttribute* pAttribute, TamlXmlParser& xmlParser)
   {
      // Sanity!
      AssertFatal(pAttribute != NULL, "Cannot search NULL attributes.");

      //// Notify Script.
      //if( isMethod("onVisitAttribute") )
      //	Con::executef(this, 1, "onVisitAttribute");

      StringTableEntry attributeName = StringTable->insert(pAttribute->Name());
      StringTableEntry attributeValue = StringTable->insert(pAttribute->Value());
      if (attributeName == mAttribName && attributeValue == mAttribValue)
         mFound = true;

      return true;
   }

public:
   TamlXmlFileVisitor() : mFound(false), mElementName(StringTable->EmptyString),
      mAttribName(StringTable->EmptyString), mAttribValue(StringTable->EmptyString) {}
   virtual ~TamlXmlFileVisitor() {}

   bool parse(const char* pFilename)
   {
      TamlXmlParser parser;
      return parser.accept(pFilename, *this, false);
   }

   bool findElementByAttribValue(const char*, const char*, const char*);
   void clearFound(void);

   virtual bool onAdd() { if (!Parent::onAdd()) return false; return true; }
   virtual void onRemove() { Parent::onRemove(); }
   static void initPersistFields();

   /// Declare Console Object.
   DECLARE_CONOBJECT(TamlXmlFileVisitor);
};

#endif // _TAML_XML_FILE_VISITOR_H_

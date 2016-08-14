#include "console/engineAPI.h"
#include "persistence/taml/xml/tamlXMLFileVisitor.h"

ConsoleFunction(TamlVisitorParse, bool, 2, 2, "(filename) - Spews a taml file to the console.\n")
{
   // Fetch filename.
   const char* pFilename = argv[1];

   TamlXmlFileVisitor xmlVisitor;
   return xmlVisitor.parse(pFilename);
}

ConsoleFunction(TamlVisitorContainsValue, bool, 4, 4, "(filename, field, value) - Finds the specified field if it contains the specified value.\n"
   "@param filename The file to search in.\n"
   "@param field The field to look for - such as internalName.\n"
   "@param value The value to find - such as New Projectile.\n"
   "@return Whether the write was successful or not.")
{
   // Fetch filename.
   const char* pFilename = argv[1];
   const char* pFieldName = argv[2];
   const char* pFieldValue = argv[3];

   TamlXmlFileVisitor xmlVisitor;
   return xmlVisitor.findElementByAttribValue(pFilename, pFieldName, pFieldValue);
}


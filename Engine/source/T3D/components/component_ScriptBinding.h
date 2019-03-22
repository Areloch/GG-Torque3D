#pragma once

#include "component.h"

//////////////////////////////////////////////////////////////////////////
// Console Methods
//////////////////////////////////////////////////////////////////////////
DefineEngineMethod(ComponentObject, beginGroup, void, (String groupName), ,
   "@brief Starts the grouping for following fields being added to be grouped into\n"
   "@param groupName The name of this group\n"
   "@param desc The Description of this field\n"
   "@param type The DataType for this field (default, int, float, Point2F, bool, enum, Object, keybind, color)\n"
   "@param defaultValue The Default value for this field\n"
   "@param userData An extra data field that can be used for custom data on a per-field basis<br>Usage for default types<br>"
   "-enum: a TAB separated list of possible values<br>"
   "-object: the T2D object type that are valid choices for the field.  The object types observe inheritance, so if you have a t2dSceneObject field you will be able to choose t2dStaticSrpites, t2dAnimatedSprites, etc.\n"
   "@return Nothing\n")
{
   object->beginFieldGroup(groupName);
}

DefineEngineMethod(ComponentObject, endGroup, void, (), ,
   "@brief Ends the grouping for prior fields being added to be grouped into\n"
   "@param groupName The name of this group\n"
   "@param desc The Description of this field\n"
   "@param type The DataType for this field (default, int, float, Point2F, bool, enum, Object, keybind, color)\n"
   "@param defaultValue The Default value for this field\n"
   "@param userData An extra data field that can be used for custom data on a per-field basis<br>Usage for default types<br>"
   "-enum: a TAB separated list of possible values<br>"
   "-object: the T2D object type that are valid choices for the field.  The object types observe inheritance, so if you have a t2dSceneObject field you will be able to choose t2dStaticSrpites, t2dAnimatedSprites, etc.\n"
   "@return Nothing\n")
{
   object->endFieldGroup();
}

DefineEngineMethod(ComponentObject, addComponentField, void, (String fieldName, String fieldDesc, String fieldType, String defValue, String userData, bool hidden),
("", "", "", "", "", false),
"Get the number of static fields on the object.\n"
"@return The number of static fields defined on the object.")
{
   object->addComponentField(fieldName, fieldDesc, fieldType, defValue, userData, hidden);
}

DefineEngineMethod(ComponentObject, getComponentFieldCount, S32, (), ,
   "@brief Get the number of ComponentField's on this object\n"
   "@return Returns the number of BehaviorFields as a nonnegative integer\n")
{
   return object->getComponentFieldCount();
}

// [tom, 1/12/2007] Field accessors split into multiple methods to allow space
// for long descriptions and type data.

DefineEngineMethod(ComponentObject, getComponentField, const char *, (S32 index), ,
   "@brief Gets a Tab-Delimited list of information about a ComponentField specified by Index\n"
   "@param index The index of the behavior\n"
   "@return FieldName, FieldType and FieldDefaultValue, each separated by a TAB character.\n")
{
   ComponentField *field = object->getComponentField(index);
   if (field == NULL)
      return "";

   char *buf = Con::getReturnBuffer(1024);
   dSprintf(buf, 1024, "%s\t%s\t%s\t%s", field->mFieldName, field->mFieldType, field->mDefaultValue, field->mGroup);

   return buf;
}

DefineEngineMethod(ComponentObject, setComponentield, const char *, (S32 index), ,
   "@brief Gets a Tab-Delimited list of information about a ComponentField specified by Index\n"
   "@param index The index of the behavior\n"
   "@return FieldName, FieldType and FieldDefaultValue, each separated by a TAB character.\n")
{
   ComponentField *field = object->getComponentField(index);
   if (field == NULL)
      return "";

   char *buf = Con::getReturnBuffer(1024);
   dSprintf(buf, 1024, "%s\t%s\t%s", field->mFieldName, field->mFieldType, field->mDefaultValue);

   return buf;
}

DefineEngineMethod(ComponentObject, getComponentFieldType, const char *, (String fieldName), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   ComponentField *field = object->getComponentField(fieldName);
   if (field == NULL)
      return "";

   return field->mFieldTypeName;;
}

DefineEngineMethod(ComponentObject, getBehaviorFieldUserData, const char *, (S32 index), ,
   "@brief Gets the UserData associated with a field by index in the field list\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the user data of this field\n")
{
   ComponentField *field = object->getComponentField(index);
   if (field == NULL)
      return "";

   return field->mUserData;
}

DefineEngineMethod(ComponentObject, getComponentFieldDescription, const char *, (S32 index), ,
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   ComponentField *field = object->getComponentField(index);
   if (field == NULL)
      return "";

   return field->mFieldDescription ? field->mFieldDescription : "";
}

DefineEngineMethod(ComponentObject, addDependency, void, (String behaviorName), ,
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   object->addDependency(behaviorName);
}

DefineEngineMethod(ComponentObject, setDirty, void, (), ,
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   object->setMaskBits(ComponentObject::OwnerMask);
}

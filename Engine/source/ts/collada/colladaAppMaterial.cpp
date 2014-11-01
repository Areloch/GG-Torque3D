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

#include "ts/loader/tsShapeLoader.h"
#include "ts/collada/colladaAppMaterial.h"
#include "ts/collada/colladaUtils.h"
#include "ts/tsMaterialList.h"
#include "materials/materialManager.h"

//-JR
#include "core/stream/fileStream.h"
//-JR

using namespace ColladaUtils;

String cleanString(const String& str)
{
   String cleanStr(str);

   // Replace invalid characters with underscores
   const String badChars(" -,.+=*/");
   for (String::SizeType i = 0; i < badChars.length(); i++)
      cleanStr.replace(badChars[i], '_');

   // Prefix with an underscore if string starts with a number
   if ((cleanStr[0] >= '0') && (cleanStr[0] <= '9'))
      cleanStr.insert(0, '_');

   return cleanStr;
}

//------------------------------------------------------------------------------

ColladaAppMaterial::ColladaAppMaterial(const char* matName)
:  mat(0),
   effect(0),
   effectExt(0)
{
   name = matName;

   // Set some defaults
   flags |= TSMaterialList::S_Wrap;
   flags |= TSMaterialList::T_Wrap;

   diffuseColor = ColorF::ONE;
   specularColor = ColorF::ONE;
   specularPower = 8.0f;
   doubleSided = false;
}

ColladaAppMaterial::ColladaAppMaterial(const domMaterial *pMat)
:  mat(pMat),
   diffuseColor(ColorF::ONE),
   specularColor(ColorF::ONE),
   specularPower(8.0f),
   doubleSided(false)
{
   // Get the effect element for this material
   effect = daeSafeCast<domEffect>(mat->getInstance_effect()->getUrl().getElement());
   effectExt = new ColladaExtension_effect(effect);

   // Get the <profile_COMMON>, <diffuse> and <specular> elements
   const domProfile_COMMON* commonProfile = ColladaUtils::findEffectCommonProfile(effect);
   const domCommon_color_or_texture_type_complexType* domDiffuse = findEffectDiffuse(effect);
   const domCommon_color_or_texture_type_complexType* domSpecular = findEffectSpecular(effect);

   // Wrap flags
   if (effectExt->wrapU)
      flags |= TSMaterialList::S_Wrap;
   if (effectExt->wrapV)
      flags |= TSMaterialList::T_Wrap;

   // Set material attributes
   if (commonProfile) {

      F32 transparency = 0.0f;
      if (commonProfile->getTechnique()->getConstant()) {
         const domProfile_COMMON::domTechnique::domConstant* constant = commonProfile->getTechnique()->getConstant();
         diffuseColor.set(1.0f, 1.0f, 1.0f, 1.0f);
         resolveColor(constant->getReflective(), &specularColor);
         resolveFloat(constant->getReflectivity(), &specularPower);
         resolveTransparency(constant, &transparency);
      }
      else if (commonProfile->getTechnique()->getLambert()) {
         const domProfile_COMMON::domTechnique::domLambert* lambert = commonProfile->getTechnique()->getLambert();
         resolveColor(lambert->getDiffuse(), &diffuseColor);
         resolveColor(lambert->getReflective(), &specularColor);
         resolveFloat(lambert->getReflectivity(), &specularPower);
         resolveTransparency(lambert, &transparency);
      }
      else if (commonProfile->getTechnique()->getPhong()) {
         const domProfile_COMMON::domTechnique::domPhong* phong = commonProfile->getTechnique()->getPhong();
         resolveColor(phong->getDiffuse(), &diffuseColor);
         resolveColor(phong->getSpecular(), &specularColor);
         resolveFloat(phong->getShininess(), &specularPower);
         resolveTransparency(phong, &transparency);
      }
      else if (commonProfile->getTechnique()->getBlinn()) {
         const domProfile_COMMON::domTechnique::domBlinn* blinn = commonProfile->getTechnique()->getBlinn();
         resolveColor(blinn->getDiffuse(), &diffuseColor);
         resolveColor(blinn->getSpecular(), &specularColor);
         resolveFloat(blinn->getShininess(), &specularPower);
         resolveTransparency(blinn, &transparency);
      }

      // Normalize specularPower (1-128). Values > 1 are assumed to be
      // already normalized.
      if (specularPower <= 1.0f)
         specularPower *= 128;
      specularPower = mClampF(specularPower, 1.0f, 128.0f);

      // Set translucency
      if (transparency != 0.0f) {
         flags |= TSMaterialList::Translucent;
         if (transparency > 1.0f) {
            flags |= TSMaterialList::Additive;
            diffuseColor.alpha = transparency - 1.0f;
         }
         else if (transparency < 0.0f) {
            flags |= TSMaterialList::Subtractive;
            diffuseColor.alpha = -transparency;
         }
         else {
            diffuseColor.alpha = transparency;
         }
      }
      else
         diffuseColor.alpha = 1.0f;
   }

   // Double-sided flag
   doubleSided = effectExt->double_sided;

   // Get the paths for the various textures => Collada indirection at its finest!
   // <texture>.<newparam>.<sampler2D>.<source>.<newparam>.<surface>.<init_from>.<image>.<init_from>
   diffuseMap = getSamplerImagePath(effect, getTextureSampler(effect, domDiffuse));
   specularMap = getSamplerImagePath(effect, getTextureSampler(effect, domSpecular));
   normalMap = getSamplerImagePath(effect, effectExt->bumpSampler);

   // Set the material name
   name = ColladaUtils::getOptions().matNamePrefix;
   if ( ColladaUtils::getOptions().useDiffuseNames )
   {
      Torque::Path diffusePath( diffuseMap );
      name += diffusePath.getFileName();
   }
   else
   {
      name += _GetNameOrId(mat);
   }
}

void ColladaAppMaterial::resolveFloat(const domCommon_float_or_param_type* value, F32* dst)
{
   if (value && value->getFloat()) {
      *dst = value->getFloat()->getValue();
   }
}

void ColladaAppMaterial::resolveColor(const domCommon_color_or_texture_type* value, ColorF* dst)
{
   if (value && value->getColor()) {
      dst->red = value->getColor()->getValue()[0];
      dst->green = value->getColor()->getValue()[1];
      dst->blue = value->getColor()->getValue()[2];
      dst->alpha = value->getColor()->getValue()[3];
   }
}

//-JR
bool isFile(const char* fileName)
{
   char sgScriptFilenameBuffer[1024];
   String cleanfilename(Torque::Path::CleanSeparators(fileName));
   Con::expandScriptFilename(sgScriptFilenameBuffer, sizeof(sgScriptFilenameBuffer), cleanfilename.c_str());

   Torque::Path givenPath(Torque::Path::CompressPath(sgScriptFilenameBuffer));
   return Torque::FS::IsFile(givenPath);
}

S32 strpos(const char* string, const char* delim, S32 offset)
{
   S32 start = offset;
   U32 sublen = dStrlen(delim);
   U32 strlen = dStrlen(string);
   if (start < 0)
      return -1;
   if (sublen + start > strlen)
      return -1;
   for (; start + sublen <= strlen; start++)
      if (!dStrncmp(string + start, delim, sublen))
         return start;
   return -1;
}

const char* strreplace(const char* source, const char* from, const char* to)
{
   S32 fromLen = dStrlen(from);
   if (!fromLen)
      return source;

   S32 toLen = dStrlen(to);
   S32 count = 0;
   const char *scan = source;
   while (scan)
   {
      scan = dStrstr(scan, from);
      if (scan)
      {
         scan += fromLen;
         count++;
      }
   }
   char *ret = Con::getReturnBuffer(dStrlen(source) + 1 + (toLen - fromLen) * count);
   U32 scanp = 0;
   U32 dstp = 0;
   for (;;)
   {
      const char *scan = dStrstr(source + scanp, from);
      if (!scan)
      {
         dStrcpy(ret + dstp, source + scanp);
         break;
      }
      U32 len = scan - (source + scanp);
      dStrncpy(ret + dstp, source + scanp, len);
      dstp += len;
      dStrcpy(ret + dstp, to);
      dstp += toLen;
      scanp += len + fromLen;
   }
   return ret;
}

const char* getSubStr(const char* str, S32 start, S32 numChars)
{
   S32 baseLen = dStrlen(str);

   if (numChars == -1)
      numChars = baseLen - start;

   if (start < 0 || numChars < 0) {
      Con::errorf(ConsoleLogEntry::Script, "getSubStr(...): error, starting position and desired length must be >= 0: (%d, %d)", start, numChars);

      return "";
   }

   if (baseLen < start)
      return "";

   U32 actualLen = numChars;
   if (start + numChars > baseLen)
      actualLen = baseLen - start;

   char *ret = Con::getReturnBuffer(actualLen + 1);
   dStrncpy(ret, str + start, actualLen);
   ret[actualLen] = '\0';

   return ret;
}

static Vector<String>   sgFindFilesResults;
static U32              sgFindFilesPos = 0;
static char sgScriptFilenameBuffer[1024];

static S32 buildFileList(const char* pattern, bool recurse, bool multiMatch)
{
   static const String sSlash("/");

   sgFindFilesResults.clear();

   String sPattern(Torque::Path::CleanSeparators(pattern));
   if (sPattern.isEmpty())
   {
      Con::errorf("findFirstFile() requires a search pattern");
      return -1;
   }

   if (!Con::expandScriptFilename(sgScriptFilenameBuffer, sizeof(sgScriptFilenameBuffer), sPattern.c_str()))
   {
      Con::errorf("findFirstFile() given initial directory cannot be expanded: '%s'", pattern);
      return -1;
   }
   sPattern = String::ToString(sgScriptFilenameBuffer);

   String::SizeType slashPos = sPattern.find('/', 0, String::Right);
   //    if(slashPos == String::NPos)
   //    {
   //       Con::errorf("findFirstFile() missing search directory or expression: '%s'", sPattern.c_str());
   //       return -1;
   //    }

   // Build the initial search path
   Torque::Path givenPath(Torque::Path::CompressPath(sPattern));
   givenPath.setFileName("*");
   givenPath.setExtension("*");

   if (givenPath.getPath().length() > 0 && givenPath.getPath().find('*', 0, String::Right) == givenPath.getPath().length() - 1)
   {
      // Deal with legacy searches of the form '*/*.*'
      String suspectPath = givenPath.getPath();
      String::SizeType newLen = suspectPath.length() - 1;
      if (newLen > 0 && suspectPath.find('/', 0, String::Right) == suspectPath.length() - 2)
      {
         --newLen;
      }
      givenPath.setPath(suspectPath.substr(0, newLen));
   }

   Torque::FS::FileSystemRef fs = Torque::FS::GetFileSystem(givenPath);
   //Torque::Path path = fs->mapTo(givenPath);
   Torque::Path path = givenPath;

   // Make sure that we have a root so the correct file system can be determined when using zips
   if (givenPath.isRelative())
      path = Torque::Path::Join(Torque::FS::GetCwd(), '/', givenPath);

   path.setFileName(String::EmptyString);
   path.setExtension(String::EmptyString);
   if (!Torque::FS::IsDirectory(path))
   {
      Con::errorf("findFirstFile() invalid initial search directory: '%s'", path.getFullPath().c_str());
      return -1;
   }

   // Build the search expression
   const String expression(slashPos != String::NPos ? sPattern.substr(slashPos + 1) : sPattern);
   if (expression.isEmpty())
   {
      Con::errorf("findFirstFile() requires a search expression: '%s'", sPattern.c_str());
      return -1;
   }

   S32 results = Torque::FS::FindByPattern(path, expression, recurse, sgFindFilesResults, multiMatch);
   if (givenPath.isRelative() && results > 0)
   {
      // Strip the CWD out of the returned paths
      // MakeRelativePath() returns incorrect results (it adds a leading ..) so doing this the dirty way
      const String cwd = Torque::FS::GetCwd().getFullPath();
      for (S32 i = 0; i < sgFindFilesResults.size(); ++i)
      {
         String str = sgFindFilesResults[i];
         if (str.compare(cwd, cwd.length(), String::NoCase) == 0)
            str = str.substr(cwd.length());
         sgFindFilesResults[i] = str;
      }
   }
   return results;
}

String findFirstFile(const char* pattern, bool recurse)
{
   S32 numResults = buildFileList(pattern, recurse, false);

   // For Debugging
   //for ( S32 i = 0; i < sgFindFilesResults.size(); i++ )
   //   Con::printf( " [%i] [%s]", i, sgFindFilesResults[i].c_str() );

   sgFindFilesPos = 1;

   if (numResults < 0)
   {
      Con::errorf("findFirstFile() search directory not found: '%s'", pattern);
      return String();
   }

   return numResults ? sgFindFilesResults[0] : String();
}

String findMaterialMap(Material *mat, const char* filename)
{
   String fileName = filename;

   if (fileName.isEmpty())
      return String("");

   Vector<String> ext;
   ext.push_back(".png");
   ext.push_back(".tga");
   ext.push_back(".jpg");
   ext.push_back(".dds");
   ext.push_back(".bmp");
   ext.push_back(".gif");
   ext.push_back(".jng");

   //-JR
   bool isfile = false;
   if (isFile(filename))
   {
      isfile = true;
   }
   else
   {
      for (U32 i = 0; i < ext.size(); i++)
      {
         String testFileName = fileName + ext[i];
         //%testFileName = fileName @ getWord( %formats, i );
         if (isFile(testFileName))
         {
            isfile = true;
            break;
         }
      }
   }

   // if we didn't grab a proper name, lets use a string logarithm
   if (!isfile)
   {
      String materialDiffuse = fileName;
      String materialDiffuse2 = fileName;

      String materialPath = mat->getFilename();

      if (dStrchr(materialDiffuse, '/') == "")
      {
         S32 k = 0;
         while (strpos(materialPath, "/", k) != -1)
         {
            S32 count = strpos(materialPath, "/", k);
            k = count + 1;
         }

         String materialsCs = getSubStr(materialPath, k, 99);
         fileName = strreplace(materialPath, materialsCs, fileName);
      }
      else
         fileName = strreplace(materialPath, materialPath, fileName);


      // lets test the pathing we came up with
      if (isFile(fileName))
      {
         isfile = true;
      }
      else
      {
         for (U32 i = 0; i < ext.size(); i++)
         {
            String testFileName = fileName + ext[i];
            if (isFile(testFileName))
            {
               isfile = true;
               break;
            }
         }
      }

      // as a last resort to find the proper name
      // we have to resolve using find first file functions very very slow
      if (!isfile)
      {
         S32 k = 0;
         while (strpos(materialDiffuse2, "/", k) != -1)
         {
            S32 count = strpos(materialDiffuse2, "/", k);
            k = count + 1;
         }

         fileName = getSubStr(materialDiffuse2, k, 99);
         for (U32 i = 0; i < ext.size(); i++)
         {
            String searchString = "*" + fileName + ext[i];
            String testFileName = findFirstFile(searchString, true);
            if (isFile(testFileName))
            {
               fileName = testFileName;
               isfile = true;
               break;
            }
         }
      }

      //final test to see if we found anything
      if (fileName == String(filename))
         return mat->getPath() + String(filename);
      else
         return fileName;
   }
   else
      return mat->getPath() + fileName;
}
//-JR

// Generate a new Material object
Material *ColladaAppMaterial::createMaterial(const Torque::Path& path) const
{
   // The filename and material name are used as TorqueScript identifiers, so
   // clean them up first
   String cleanFile = cleanString(TSShapeLoader::getShapePath().getFileName());
   String cleanName = cleanString(getName());

   // Prefix the material name with the filename (if not done already by TSShapeConstructor prefix)
   //-JR
   //if (!cleanName.startsWith(cleanFile))
   //   cleanName = cleanFile + "_" + cleanName;
   //-JR

   // Determine the blend operation for this material
   Material::BlendOp blendOp = (flags & TSMaterialList::Translucent) ? Material::LerpAlpha : Material::None;
   if (flags & TSMaterialList::Additive)
      blendOp = Material::Add;
   else if (flags & TSMaterialList::Subtractive)
      blendOp = Material::Sub;

   // Create the Material definition
   const String oldScriptFile = Con::getVariable("$Con::File");
   Con::setVariable("$Con::File", path.getFullPath());   // modify current script path so texture lookups are correct
   Material *newMat = MATMGR->allocateAndRegister( cleanName, getName() );
   Con::setVariable("$Con::File", oldScriptFile);        // restore script path

   //-JR
   //newMat->mDiffuseMapFilename[0] = diffuseMap;
   //newMat->mNormalMapFilename[0] = normalMap;
   //newMat->mSpecularMapFilename[0] = specularMap;
   newMat->mDiffuseMapFilename[0] = findMaterialMap(newMat, diffuseMap);
   newMat->mNormalMapFilename[0] = findMaterialMap(newMat, normalMap);
   newMat->mSpecularMapFilename[0] = findMaterialMap(newMat, specularMap);
   //-JR

   newMat->mDiffuse[0] = diffuseColor;
   newMat->mSpecular[0] = specularColor;
   newMat->mSpecularPower[0] = specularPower;

   newMat->mDoubleSided = doubleSided;
   newMat->mTranslucent = (bool)(flags & TSMaterialList::Translucent);
   newMat->mTranslucentBlendOp = blendOp;

   return newMat;
}

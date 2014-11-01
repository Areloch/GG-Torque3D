#ifndef _STRINGBUFFER_H_
#include "core/stringBuffer.h"
#endif
#ifndef _UNICODE_H_
#include "core/strings/unicode.h"
#endif
#ifndef _FINDMATCH_H_
#include "core/strings/findMatch.h"
#endif
#ifndef _STRINGUNIT_H_
#include "core/strings/stringUnit.h"
#endif
#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
#endif

#include "core/fileObject.h"

class TSCodeParser
{
	U32 mScanPos;
public:
	enum codeElements{
		Keyword = BIT(0),
		Function = BIT(1),
		LocalVariable = BIT(2),
      GlobalVariable = BIT(3),
		Comment = BIT(4),
      MLComment = BIT(5),
      ClassName = BIT(6),
      StringType = BIT(7),
      Tag = BIT(8),
      Text = BIT(9),
      NewLine = BIT(10),
		Break
	};

   struct LineSegment
	{
		S32 startPos;
		S32 endPos;

      codeElements type;
	};

	struct Line
	{
		S32 startPos;
		S32 endPos;
      S32 eolCharacter;
	};

	struct object
	{
		S32 namePos;
		S32 nameLength;

		S32 classPos;
		S32 classLen;

		struct field
		{
			S32 namePos;
			S32 nameLength;
			S32 valuePos;
			S32 valueLength;
		};

		Vector<field> mFields;
	};

	StringBuffer *mTextBuffer;

	U32 mTextLength;

	Vector<Line> mLines;

   Vector<LineSegment> mSegments;

	Vector<object> mObjects;

	//const char* findNextElement( U32 elementMask, S32 startPosition );
	//const char* findWord( const char* word, S32 startPosition );

	//void findObjectName(const char* objName, S32 startPosition );

	//const char* findAndReplace( const char* objName, S32 startPosition );

public:
	bool loadFile(const char* path);
   bool loadText(StringBuffer &mTextBuffer);

	S32 find( const char* find, S32 startPosition );
	void findAndReplace( const char* fimd, const char* replace, S32 startPosition );

   S32 findOnLine( const char* find, S32 lineNum, S32 startPosition );
   void findAndReplaceOnLine( const char* fimd, const char* replace, S32 lineNum, S32 startPosition );
   S32 findObjectNameOnLine( String &objName, S32 lineNum, S32 startPosition );

   bool findSelectComment( S32 startPosition, S32 &commentStart, S32 &commentEnd );

	//S32 findObjectName( S32 startPosition, const char* objName);
	S32 findObjectName( S32 startPosition, String &objName);
	//S32 getLineFromPosition( S32 startPosition );
	Line* getLineFromPosition( S32 position );
	Line getLine( S32 startPosition );

   S32 getLineNumber( S32 startPosition );

   codeElements getSegmentType( S32 segmentStart );

   void addSegment(S32 startPos, S32 endPos, codeElements type);

	const char* readLine(S32 startPos);

	const char* getCode();

   void clearBuffer() { delete[] &mTextBuffer; mTextLength = 0; }

	void execute();

	void buildObjectList();

	String getObjectField( S32 objectPos, String &fieldName);

   inline static S32 getHexVal(char c)
   {
      if(c >= '0' && c <= '9')
         return c - '0';
      else if(c >= 'A' && c <= 'Z')
         return c - 'A' + 10;
      else if(c >= 'a' && c <= 'z')
         return c - 'a' + 10;
      return -1;
   }

   inline static S32 scanforchar(const char *str, U32 idx, char c)
   {
      U32 startidx = idx;
      while(str[idx] != c && str[idx] && str[idx] != '\n')
         idx++;
      return str[idx] == c && startidx != idx ? idx-startidx : -1;
   }

   inline static S32 scanforword(char *str, const char* word)
   {
	   const char* retpos = dStrstr( str, word );
      if( !retpos )
         return -1;
      
      return retpos - str;
   }

   inline static S32 scanforScriptTerminator(const char *str, U32 idx)
   {
      U32 startidx = idx;
      while(str[idx] && str[idx] != '[' && str[idx] != ']' && str[idx] != ';' && str[idx] != '(' 
	      && str[idx] != ')' && str[idx] != '-' && str[idx] != '*' && str[idx] != '/' && str[idx] != '+' 
	      && str[idx] != '%' && str[idx] != '&' && str[idx] != '@' && str[idx] != '$' && str[idx] != ' ' 
	      && str[idx] != ',' && str[idx] != '=' && str[idx] != '!' && str[idx] != '^' && str[idx] != '\''
	      && str[idx] != '|' && str[idx] != '{' && str[idx] != '}' && str[idx] != '\\' && str[idx] != '\t' 
	      && str[idx] != '.' && str[idx] != '\n')
         idx++;
      return startidx != idx ? idx : 0;
   }

   inline static S32 scanforScriptTerminatorNoSpace(const char *str, U32 idx)
   {
      U32 startidx = idx;
      while(str[idx] && str[idx] != '[' && str[idx] != ']' && str[idx] != ';' && str[idx] != '(' 
	      && str[idx] != ')' && str[idx] != '-' && str[idx] != '*' && str[idx] != '/' && str[idx] != '+' 
	      && str[idx] != '%' && str[idx] != '&' && str[idx] != '@' && str[idx] != '$' && str[idx] != ',' 
         && str[idx] != '=' && str[idx] != '!' && str[idx] != '^' && str[idx] != '\'' && str[idx] != '|' 
         && str[idx] != '{' && str[idx] != '}' && str[idx] != '\\' && str[idx] != '\t' && str[idx] != '.' && str[idx] != '\n')
         idx++;
      return startidx != idx ? idx : 0;
   }

   inline static S32 scanforSegmentTerminator(const char *str, U32 idx)
   {
      U32 startidx = idx;
      while(str[idx] && str[idx] != '*' && str[idx] != '/' && str[idx] != '%' && str[idx] != '$' 
	      && str[idx] != '\'' && /*str[idx] != '\r' &&*/ str[idx] != '.' && str[idx] != '\n')
         idx++;
      return startidx != idx ? idx : 0;
   }

   inline static bool isScriptTerminator(char str)
   {
	   if(str && str != '[' && str != ']' && str != ';' && str != '(' 
	      && str != ')' && str != '-' && str != '*' && str != '/' && str != '+' 
	      && str != '%' && str != '&' && str != '@' && str != '$' && str != ' ' 
	      && str != ',' && str != '=' && str != '!' && str != '^' && str != '\''
	      && str != '|' && str != '{' && str != '}' && str != '\\' && str != '\t' 
	      && str != '.' && str != '\n')
		   return false;
	   else
		   return true;
   }

   inline static bool isTextTerminator(char str)
   {
	   if(str && str != '*' && str != '/' && str != '%' && str != '$' 
	      && str != '\'' && str != '\"' && str != '\n')
		   return false;
	   else
		   return true;
   }

   inline static S32 scanforScriptOperator(const char *str, U32 &idx)
   {
      U32 startidx = idx;
      while(str[idx] && str[idx] != '-' && str[idx] != '*' && str[idx] != '/' && str[idx] != '+' 
	      && str[idx] != '%' && str[idx] != '&' && str[idx] != '@' && str[idx] != '$' && str[idx] != '=' 
	      && str[idx] != '!' && str[idx] != '^' && str[idx] != '|')
         idx++;
      return startidx != idx ? idx : -1;
   }

   inline static U32 getWordCount(const char *string)
   {
     U32 count = 0;
     U8 last = 0;
     const char *set = " \t\n";
     while(*string)
     {
        last = *string++;

        for(U32 i =0; set[i]; i++)
        {
           if(last == set[i])
           {
              count++;
              last = 0;
              break;
           }
        }
     }
     if(last)
        count++;
     return count;
   }

   inline static bool scanforURL(const char *str, U32 &idx, char c)
   {
      U32 startidx = idx;
      while(str[idx] != c && str[idx] && str[idx] != '>' && str[idx] != '\n')
         idx++;
      return str[idx] == c && startidx != idx;
   }

   inline static const char* readLine(const char *str, S32 startPos)
   {
	   if(!str[0])
         return "";

	   char *buf;
	   dStrcpy(buf, str);

      U32 tokPos = startPos;
	   U32 curPos = startPos;
	   U32 len = strlen(str);

      for(;;)
      {
         if(curPos == len)
            break;

         if(buf[curPos] == '\r')
         {
            buf[curPos++] = 0;
            if(buf[curPos] == '\n')
               curPos++;
            break;
         }

         if(buf[curPos] == '\n')
         {
            buf[curPos++] = 0;
            break;
         }

         curPos++;
      }

      return buf + tokPos;

	   //reads through the str from the start, and separates it into the first acceptable line
	   /*U32 pos = startPos;
	   U32 len = strlen(str);

      for(;;)
      {
         if(pos == len)
            break;

		
         if(str[pos] == '\r')
         {
            //str[pos++] = 0;
            if(str[pos++] == '\n')
               pos++;
            break;
         }

         if(str[pos] == '\n')
         {
            //str[pos++] = 0;
            break;
         }

         pos++;
      }

	
	   char returnstr[2048];
	   strncpy( returnstr, str + startPos, pos );
	   returnstr[pos] = '\0';

      return returnstr;*/
   }

   inline static S32 scanForEOL(const char *str, S32 startPos)
   {
	   if(!str[0])
         return -1;

	   //char *buf;
	   //dStrcpy(buf, str);

      U32 tokPos = startPos;
	   U32 curPos = startPos;
	   U32 len = strlen(str);

      for(;;)
      {
         if(curPos == len)
            break;

         if(str[curPos] == '\r')
         {
            if(str[curPos] == '\n')
               curPos++;
            break;
         }

         if(str[curPos] == '\n')
         {
            break;
         }

         curPos++;
      }

      return curPos;
   }
};
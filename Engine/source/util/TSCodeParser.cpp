#include "util/TSCodeParser.h"

const S32 sReservedWords = 15;
const char* keywords[sReservedWords] = 
{
   "function",
   "new",
   "do",
   "while",
   "if",
   "else",
   "else if",
   "for",
   "continue",
   "break",
   "where",
   "%this",
   "true",
   "false",
   "return"
};

const char* TSCodeParser::getCode() 
{ 
	return (const char*)mTextBuffer->createSubstring8(0, mTextLength);
}

const char* TSCodeParser::readLine(S32 startPos)
{
	U32 curPos = startPos;

	while(curPos < mTextLength)
	{
		char chr = (char)mTextBuffer->getChar(curPos);

      char tempA = '\r';
      char tempB = '\n';

      if(chr == '\r')
      {
			curPos++;

			if((char)mTextBuffer->getChar(curPos) == '\n')
				curPos++;

			break;
      }

      if(chr == '\n')
      {
         curPos++;
         break;
      }

      curPos++;
   }

	return (const char*)mTextBuffer->createSubstring8(startPos, curPos-startPos);

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

/*void writeTabs(U32 count)
{
  while(count--)
     insertChars( "\t", 1, mCursorPosition );
}*/

/*const char* TSCodeParser::findNextElement( U32 elementMask, S32 startPosition )
{
	mScanPos = startPosition;

	for(;;)
   {
      UTF16 curChar = mTextBuffer->getChar(mScanPos);

      if(!curChar)
         break;

      if(curChar == '\n')
      {
         textStart = mScanPos;
         len = 1;
         mScanPos++;
         processEmitAtoms();
         emitNewLine(textStart);
         mCurDiv = 0;

		 if(!isSelectCommented)
		 {
			 if(mCurStyle->color != mProfile->mFontColor)
			 {
				if(mCurStyle->used)
					mCurStyle = allocStyle(mCurStyle);
				 mCurStyle->color = mProfile->mFontColor;
			 }
		 }
         continue;
      }

      if(curChar == '\t')
      {
         textStart = mScanPos;
         len = 1;
         mScanPos++;
         processEmitAtoms();
         if(mTabStopCount)
         {
            if(mCurTabStop < mTabStopCount)
            {
				mCurX += 22; //equivalent of 3 spaces
            }
            mCurTabStop++;
         }
         continue;
      }

	  //probably a variable
	  if(curChar == '%' || curChar == '$')
     {
		  //we may be starting a comment
		  const UTF8 *str = mTextBuffer->getPtr8();
		  str = getNthCodepoint(str, mScanPos);
		  U32 index = 1;

		  S32 t = scanforScriptTerminator(str, index);
		  if(t > 0)
		  {
			 //didn't return anything, we're a variable!
			 if(mCurStyle->used)
				mCurStyle = allocStyle(mCurStyle);

			 if(curChar == '%')
				mCurStyle->color = ColorI(0,160,128);
			 else
			    mCurStyle->color = ColorI(196,92,0);

			 //override
			textStart = mScanPos;
			idx = t+1;

			len = idx;
			mScanPos += idx;
			emitTextToken(textStart, len);

			if(mCurStyle->used)
			  mCurStyle = allocStyle(mCurStyle);
			mCurStyle->color = mProfile->mFontColor;
			continue;
		  }
		  else
			goto textemit;
	  }

	  if(curChar == '/')
	  {
		  //we may be starting a comment
		  const UTF8 *str = mTextBuffer->getPtr8();
		  str = getNthCodepoint(str, mScanPos);

		  if(!dStrnicmp(str + 1, "/", 1))
		  {
			  if(mCurStyle->used)
				   mCurStyle = allocStyle(mCurStyle);
			  mCurStyle->color = ColorI(0,100,0);

			  //override
			  textStart = mScanPos;
			  idx = 1;

			  while(mTextBuffer->getChar(mScanPos+idx) && mTextBuffer->getChar(mScanPos+idx) != '\n' )
				 idx++;
			  len = idx;
			  mScanPos += idx;
			  emitTextToken(textStart, len);

			  if(mCurStyle->used)
				  mCurStyle = allocStyle(mCurStyle);
			  mCurStyle->color = mProfile->mFontColor;
			  continue;
		  }
		  else if(!dStrnicmp(str + 1, "*", 1))
		  //else if(scanforchar(str, 1, '*'))
		  {
			//selective comment!
			isSelectCommented = true;
		  }
	  }

	  if(curChar == '\"'/* || curChar == '\''*//*)
	  {
		  //we may be starting a comment
		  const UTF8 *str = mTextBuffer->getPtr8();
		  str = getNthCodepoint(str, mScanPos);

		  if(mCurStyle->used)
			   mCurStyle = allocStyle(mCurStyle);
		  mCurStyle->color = ColorI(160,32,240);

		  //go looking for the end
		  S32 cIdx = scanforchar(str, idx, '\"');
		  if(cIdx == -1)
		  {
			  goto textemit;
		  }
		  else
		  {
			 if(mCurStyle->used)
				mCurStyle = allocStyle(mCurStyle);
			 mCurStyle->color = ColorI(160,32,240);

			 textStart = mScanPos;
			 idx = cIdx+1;

			 len = idx;
			 mScanPos += idx;
			 emitTextToken(textStart, len);

			 if(mCurStyle->used)
				 mCurStyle = allocStyle(mCurStyle);
			 mCurStyle->color = mProfile->mFontColor;
			 continue;
		  }
	  }

	  //check the reserved words
	  /*const UTF8 *str = mTextBuffer->getPtr8();
	  str = getNthCodepoint(str, mScanPos);
	  for(U32 rw = 0; rw < getWordCount(str); rw++)
	  {

	   const char* retpos = dStrstr( string, substring );
	   if( retpos )
	   
	      
	   return retpos - string;

	  StringUnit::
	  if(!dStrnicmp(str + 1, "/", 1))

	  //arbitrary text catch-all
	  if(!isWord && !isString && !isLineCommented && !isSelectCommented)
	  {
		  if(mCurStyle->color != ColorI(0,0,0))
		  {
			  //didn't return anything, we're a variable!
			  if(mCurStyle->used)
			     mCurStyle = allocStyle(mCurStyle);
			  mCurStyle->color = ColorI(0,0,0);
		  }
	  }*/

/*textemit:
      textStart = mScanPos;
      idx = 1;
      while(mTextBuffer->getChar(mScanPos+idx) && !isTextTerminator(mTextBuffer->getChar(mScanPos+idx)))
         idx++;
      len = idx;
      mScanPos += idx;
      emitTextToken(textStart, len);
   }
}*/


TSCodeParser::Line* TSCodeParser::getLineFromPosition( S32 position )
{
	for(U32 i=0; i < mLines.size(); i++)
	{
		if(position >= mLines[i].startPos && position <= mLines[i].endPos)
			return &mLines[i];
	}

	return NULL;
}

/// Finds the first object name from the start position
S32 TSCodeParser::findObjectName( S32 startPosition, String &objName)
{
	mScanPos = startPosition;
	Line* l = getLineFromPosition(startPosition);
	if(l == NULL)
		return -1;

	while(l != NULL)
	{
		S32 start = startPosition > l->startPos ? startPosition : l->startPos;
		char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

		//now, look for 'new'. If we haven't found 'new' in the line, we haven't declared anything.
		S32 pos = scanforword( dStrlwr(line), "new" );

		if(pos != -1)
		{
			//we have a declaration, so parse forward until we find a parenthesis
			S32 parthPos = scanforchar(line, pos+2, '(');

			if(parthPos != -1)
			{
				//found it. Now parse to the closing parinth
				S32 endParthPos = scanforchar(line, pos+2+parthPos+1, ')');

				if(endParthPos != -1)
				{
					//and we've got it!
					objName = (const char*)mTextBuffer->createSubstring8(start+pos+2+parthPos+1, endParthPos);

					return start+pos+2+parthPos+1;
				}
			}
		}
		
		l = getLineFromPosition(l->endPos+1);
	}

	return -1;
}

S32 TSCodeParser::find( const char* find, S32 startPosition )
{
	Line* l = getLineFromPosition(startPosition);
	if(l == NULL)
		return -1;

	while(l != NULL)
	{
		S32 start = startPosition > l->startPos ? startPosition : l->startPos;
		char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

		S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

		if(pos != -1)
		{
			return pos;
		}

		l = getLineFromPosition(l->endPos+1);
	}

	return -1;
	//S32 lineIdx = getLineFromPosition(startPosition);

	//if(lineIdx == -1)
		//return -1;

	/*for(S32 l = lineIdx; l < mLines.size(); l++)
	{
		S32 start = mLines[l];
	   S32 end = (l == mLines.size() - 1) ? mTextLength : mLines[l+1];

		//get the line
		char* line = (char*)mTextBuffer->createSubstring8(start, end-start);

		S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

		if(pos != -1)
		{
			return pos;
		}
	}*/
}

bool TSCodeParser::findSelectComment( S32 startPosition, S32 &commentStart, S32 &commentEnd )
{
	Line* l = getLineFromPosition(startPosition);
	if(l == NULL)
		return -1;

   commentStart = -1;
   commentEnd = -1;

   while(l != NULL)
	{
		S32 start = startPosition > l->startPos ? startPosition : l->startPos;
		char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

      if(commentStart == -1)
		   commentStart = scanforword( strlwr(line), strlwr("/*") );
      
      if(commentEnd == -1)
         commentEnd = scanforword( strlwr(line), strlwr("*/") );

		if(commentStart != -1 && commentEnd != -1)
		{
			return true;
		}

		l = getLineFromPosition(l->endPos+1);
	}

	return false;
}

void TSCodeParser::findAndReplace( const char* find, const char* replace, S32 startPosition )
{
	Line* l = getLineFromPosition(startPosition);
	if(l == NULL)
		return;

	S32 lineAdjust = 0;
	while(l != NULL)
	{
		//Adjust the line info for changes we've made so far
		l->startPos -= lineAdjust;
		l->endPos -= lineAdjust;

		S32 start = startPosition > l->startPos ? startPosition : l->startPos;
		char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

		S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

		if(pos != -1)
		{
			lineAdjust += strlen(find) - strlen(replace);
			mTextLength -= strlen(find) - strlen(replace);

			mTextBuffer->cut(start+pos, strlen(find));
			mTextBuffer->insert(start+pos, replace);
		}

		l = getLineFromPosition(l->endPos+1);
	}
	//S32 lineIdx = getLineFromPosition(startPosition);

	//if(lineIdx == -1)
		//return;

	/*S32 lineAdjust = 0;

	for(S32 l = lineIdx; l < mLines.size(); l++)
	{
		//If we've made modification to the text, adjust the line offsets
		mLines[l] -= lineAdjust;

		S32 start = mLines[l];
	   S32 end = (l == mLines.size() - 1) ? mTextLength : mLines[l+1];

		//get the line
		char* line = (char*)mTextBuffer->createSubstring8(start, end-start);

		S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

		if(pos != -1)
		{
			lineAdjust += strlen(find) - strlen(replace);
			mTextLength -= strlen(find) - strlen(replace);

			mTextBuffer->cut(start+pos, strlen(find));
			mTextBuffer->insert(start+pos, replace);
		}
	}

	S32 jhiia = 0;*/
}

//Only scan our given line
S32 TSCodeParser::findOnLine( const char* find, S32 lineNum, S32 startPosition )
{
   if(lineNum > mLines.size())
      return -1;

	Line l = mLines[lineNum];

	S32 start = startPosition > l.startPos ? startPosition : l.startPos;
	char* line = (char*)mTextBuffer->createSubstring8(start, l.endPos - start);

	S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

	if(pos != -1)
	{
		return pos;
	}
   else
   {
	   return -1;
   }
}

void TSCodeParser::findAndReplaceOnLine( const char* find, const char* replace, S32 lineNum, S32 startPosition )
{
	if(lineNum > mLines.size())
      return;

	Line* l = &mLines[lineNum];

	if(l == NULL)
		return;

	S32 lineAdjust = 0;

	//Adjust the line info for changes we've made so far
	l->startPos -= lineAdjust;
	l->endPos -= lineAdjust;

	S32 start = startPosition > l->startPos ? startPosition : l->startPos;
	char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

	S32 pos = scanforword( strlwr(line), strlwr((char*)find) );

	if(pos != -1)
	{
		lineAdjust += strlen(find) - strlen(replace);
		mTextLength -= strlen(find) - strlen(replace);

		mTextBuffer->cut(start+pos, strlen(find));
		mTextBuffer->insert(start+pos, replace);
	}
}

S32 TSCodeParser::findObjectNameOnLine(String &objName, S32 lineNum, S32 startPosition)
{
	if(lineNum > mLines.size())
      return -1;

	Line l = mLines[lineNum];

	S32 start = startPosition > l.startPos ? startPosition : l.startPos;
	char* line = (char*)mTextBuffer->createSubstring8(start, l.endPos - start);

	//now, look for 'new'. If we haven't found 'new' in the line, we haven't declared anything.
	S32 pos = scanforword( dStrlwr(line), "new" );

	if(pos != -1)
	{
		//we have a declaration, so parse forward until we find a parenthesis
		S32 parthPos = scanforchar(line, pos+2, '(');

		if(parthPos != -1)
		{
			//found it. Now parse to the closing parinth
			S32 endParthPos = scanforchar(line, pos+2+parthPos+1, ')');

			if(endParthPos != -1)
			{
				//and we've got it!
				objName = (const char*)mTextBuffer->createSubstring8(start+pos+2+parthPos+1, endParthPos);

				return start+pos+2+parthPos+1;
			}
		}
	}

	return -1;
}

TSCodeParser::codeElements TSCodeParser::getSegmentType( S32 segmentStart )
{
   if(segmentStart > mTextLength - 1)
      return Break;

   char segmentChar = mTextBuffer->getChar(segmentStart);
   char nextChar = mTextBuffer->getChar(segmentStart+1);

   if(segmentChar == '%')
   {
      S32 segEnd = findOnLine("%this", getLineNumber(segmentStart), segmentStart);

      if(segEnd != -1)
      {
         return Keyword;
      }
      else
      {
         return LocalVariable;
      }
   }
   else if(segmentChar == '$')
   {
      return GlobalVariable;
   }
   else if(segmentChar == '/' && mTextBuffer->getChar(segmentStart+1) == '/')
   {
      return Comment;
   }
   else if(segmentChar == '/' && mTextBuffer->getChar(segmentStart+1) == '*')
   {
      return MLComment;
   }
   else if(segmentChar == '\"')
   {
      return StringType;
   }
   else if(segmentChar == '\'')
   {
      return Tag;
   }
   else
   {
      //lastly, check for any reserved/keywods
      for(S32 w=0; w < sReservedWords; w++)
      {
         S32 segEnd = findOnLine(keywords[w], getLineNumber(segmentStart), segmentStart);    

         if(segEnd != -1)
         {
            return Keyword;
         }
      }
   }

   return Text;
}

S32 TSCodeParser::getLineNumber( S32 position )
{
   for(U32 i=0; i < mLines.size(); i++)
	{
		if(position >= mLines[i].startPos && position <= mLines[i].endPos)
			return i;
	}

   return -1;
}

void TSCodeParser::addSegment(S32 startPos, S32 endPos, codeElements type)
{
   LineSegment newSegment;
   newSegment.startPos = startPos;
   newSegment.endPos = endPos;
   newSegment.type = type;

   mSegments.push_back(newSegment);
}

bool TSCodeParser::loadFile(const char* path)
{
	mTextLength = 0; 

	static char scriptFilenameBuffer[1024];
	Con::expandScriptFilename( scriptFilenameBuffer, sizeof( scriptFilenameBuffer ), path );
	StringTableEntry scriptFileName = StringTable->insert(scriptFilenameBuffer);

	void *data;
   U32 dataSize = 0;
   Torque::FS::ReadFile(scriptFileName, data, dataSize, true);

	const char*	script = (char *)data;

	mTextBuffer->append(script);

	//parse for lines!
	mTextLength = strlen(script);
	S32 charCount = strlen(script);
	S32 curPos = 0;
	while(curPos < charCount)
	{
		const char* line = readLine(curPos);

		S32 numChars = strlen(line);

		Line newLine;

		newLine.startPos = curPos;
		newLine.endPos = curPos + numChars;

		mLines.push_back(newLine);

		curPos += numChars;
	}

	return true;
}

bool TSCodeParser::loadText(StringBuffer &textBuffer)
{
   mTextBuffer = &textBuffer;

	//parse for lines!
   mTextLength = textBuffer.length();
	S32 curPos = 0;
	while(curPos < mTextLength)
	{
		const char* line = readLine(curPos);

		S32 numChars = strlen(line);

      char eolChar = line[numChars-1];

		Line newLine;

		newLine.startPos = curPos;
		newLine.endPos = curPos + numChars;
      newLine.eolCharacter = newLine.endPos-1;

		mLines.push_back(newLine);

		curPos += numChars;
	}

   char *str = const_cast<char*>(mTextBuffer->getPtr8());

   S32 plainTextSegment=0;
   S32 segmentStart = -1;
   S32 segmentEnd = -1;
   codeElements segmentType;

   S32 segmentLength = 0;
   segmentType = Text;

   char r = '\r';
   char n = '\n';

   for (U32 i = 0; i < mTextLength; i++)
   {
      char currentChar = mTextBuffer->getChar(i);

      if(currentChar == '\r')
      {
         if(i < mTextLength - 1)
         {
            //step ahead, see if we have a newline
            char nextChar = mTextBuffer->getChar(i+1);
            if(nextChar == '\n')
            {
               LineSegment newSegment;
               newSegment.startPos = i;
               newSegment.endPos = i + 2;
				   newSegment.type = NewLine;
				   mSegments.push_back(newSegment);
               i += 1;
               continue;
            }
         }
      }

      if(currentChar == '\n')
      {
         LineSegment newSegment;
         newSegment.startPos = i;
         newSegment.endPos = i + 1;
			newSegment.type = NewLine;
			mSegments.push_back(newSegment);
         continue;
      }

	   segmentType = getSegmentType(i);

	   if (segmentType != Text)
	   {
		   if (segmentType == LocalVariable)
		   {
            LineSegment newSegment;

            //check if it's actually "%this"
			   /*bool isThis = scanforword(str, "%this");
			   if (isThis)
			   {
               newSegment.startPos = i;
               newSegment.endPos = i + 5;
				   newSegment.type = Keyword;
				   mSegments.push_back(newSegment);
				   i += 4;
			   }
			   else
			   {*/
				   S32 end = scanforScriptTerminatorNoSpace(str, i+1);

               end = end == 0 ? i+1 : end;

               const char* tempstr = mTextBuffer->createSubstring8(i, end - i);

               newSegment.startPos = i;
               newSegment.endPos = end;

               if(!dStrcmp(tempstr, "%this"))
                  newSegment.type = Keyword;
               else
				      newSegment.type = LocalVariable;

				   mSegments.push_back(newSegment);
				   i = end - 1;
			   //}
		   }
		   else if (segmentType == GlobalVariable)
		   {
			   S32 end = scanforScriptTerminatorNoSpace(str, i+1);

            end = end == 0 ? i+1 : end;

            const char* tempstr = mTextBuffer->createSubstring8(i, end - i);

				LineSegment newSegment;
            newSegment.startPos = i;
            newSegment.endPos = end;
				newSegment.type = GlobalVariable;

				mSegments.push_back(newSegment);
				i = end - 1;
		   }
		   else if (segmentType == Comment)
		   {
			   S32 end = scanForEOL(str, i);

            const char* tempstr = mTextBuffer->createSubstring8(i, end - i);

			   LineSegment newSegment;
            newSegment.startPos = i;
            newSegment.endPos = end;
			   newSegment.type = Comment;

			   mSegments.push_back(newSegment);
			   i = end - 1;
		   }
		   else if (segmentType == MLComment)
		   {
			   S32 len = scanforword(str, "*/");
			   for (U32 s = i; s < len; i++)
			   {
				   S32 eol = scanForEOL(str, s);
				   
               LineSegment newSegment;
               newSegment.startPos = s;
               newSegment.endPos = eol;
				   newSegment.type = MLComment;

				   mSegments.push_back(newSegment);
				   s = eol;
			   }
			   i += len - 1;
		   }
		   else if (segmentType == StringType)
		   {
			   S32 len = scanforword(str, "\"");
			   for (U32 s = i; s < len; i++)
			   {
				   S32 eol = scanForEOL(str, s);
				   
               LineSegment newSegment;
               newSegment.startPos = s;
               newSegment.endPos = eol;
				   newSegment.type = StringType;

				   mSegments.push_back(newSegment);
				   s = eol;
			   }
			   i += len - 1;
		   }
		   else if (segmentType == Tag)
		   {
			   S32 len = scanforword(str, "\'");
			   for (U32 s = i; s < len; i++)
			   {
				   S32 eol = scanForEOL(str, s);
				   
               LineSegment newSegment;
               newSegment.startPos = s;
               newSegment.endPos = eol;
				   newSegment.type = Tag;

				   mSegments.push_back(newSegment);
				   s = eol;
			   }
			   i += len - 1;
		   }
	   }
	   else
	   {
		   S32 terminator = scanforSegmentTerminator(str, i);

         char termChar = mTextBuffer->getChar(terminator);

         S32 offset = terminator - i;
		   if (offset > 0)
		   {
			   LineSegment newSegment;
            newSegment.startPos = i;
            newSegment.endPos = terminator;
				newSegment.type = Text;

            const char* tempstr = mTextBuffer->createSubstring8(i, offset);

			   mSegments.push_back(newSegment);
	         i = terminator - 1;
		   }
	   }
   }
   
   /*   Line *line = getLineFromPosition(c);
      S32 lineNum;

      for(U32 l=0; l<mLines.size(); l++)
      {
         if(mLines[l].startPos == line->startPos)
         {
            lineNum = l;
            break;
         }
      }

     //now, run through the lines and build segments

      S32 lineStart = line->startPos;
      S32 lineEnd = line->endPos;

      UTF16 curChar = mTextBuffer->getChar(c);

      if(curChar == '%')
      {
         //real fast, check if we have a special case with "%this"
         S32 segEnd = findOnLine("%this", lineNum, c);

         if(segEnd != -1)
         {
            char startChar = mTextBuffer->getChar(c);
            char endChar = mTextBuffer->getChar(segEnd);
            segmentStart = c;
            segmentEnd = segEnd;
            segmentType = Keyword;
         }
         else
         {
            //nope, just a local variable
            segEnd = scanforScriptTerminator(str, c);

            char startChar = mTextBuffer->getChar(c);
            char endChar = mTextBuffer->getChar(segEnd);
            
            segmentStart = c;
            segmentEnd = segEnd;
            segmentType = LocalVariable;
         }
      }
      else if(curChar == '$')
      {
         //global variable
         S32 segEnd = scanforScriptTerminator(str, c);

         segmentStart = c;
         segmentEnd = segEnd;
         segmentType = GlobalVariable;
      }
      else if(curChar == '/' && mTextBuffer->getChar(c+1) == '/')
      {
         //line comment
         char startChar = mTextBuffer->getChar(c);
         char endChar = mTextBuffer->getChar(lineEnd-1);
         segmentStart = c;
         segmentEnd = lineEnd-1;
         segmentType = Comment;
      }
      else if(curChar == '/' && mTextBuffer->getChar(c+1) == '*')
      {
         //multiline comment
         //we need to find the end, if there is one
         S32 segEnd = find("*//*", c+2);

         segmentStart = c;

         if(segEnd == -1)
            segmentEnd = mTextBuffer->length();
         else
            segmentEnd = segEnd;

         segmentType = MLComment;
      }
      else if(curChar == '\"')
      {
         //string
         //we need to find the end, if there is one
         S32 segEnd = find("\"", c++);

         segmentStart = c;

         if(segEnd == -1)
            segmentEnd = mTextBuffer->length();
         else
            segmentEnd = segEnd;

         segmentType = StringType;
      }
      else if(curChar == '\'')
      {
         //tag
         //we need to find the end, if there is one
         S32 segEnd = find("\'", c++);

         segmentStart = c;

         if(segEnd == -1)
            segmentEnd = mTextBuffer->length();
         else
            segmentEnd = segEnd;

         segmentType = Tag;
      }
      else if(curChar == '\n')
      {
         if(segmentStart != -1)
         {
            char startChar = mTextBuffer->getChar(segmentStart);
            char endChar = mTextBuffer->getChar(c);

            LineSegment newSegment;
            newSegment.startPos = segmentStart;
            newSegment.endPos = c;
            newSegment.type = segmentType;
            mSegments.push_back(newSegment);

            segmentStart = c++;
            plainTextSegment = 0;
         }
         else if(plainTextSegment != 0)
         {
            char startChar = mTextBuffer->getChar(c - plainTextSegment);
            char endChar = mTextBuffer->getChar(c);

            LineSegment newSegment;
            newSegment.startPos = c - plainTextSegment;
            newSegment.endPos = c;
            newSegment.type = Text;

            mSegments.push_back(newSegment);
            plainTextSegment = 0;
         }
      }
      else
      {
         //lastly, check for any reserved/keywods
         for(S32 w=0; w < sReservedWords; w++)
         {
            S32 segEnd = findOnLine(keywords[w], lineNum, c);    

            if(segEnd != -1)
            {
               segmentStart = c;
               segmentEnd = segEnd;
               segmentType = Keyword;
            }
         }
      }

      //build a segment
      if(segmentStart != -1 && segmentEnd != -1 || c == mTextLength-1)
      {
         //first, build a regular text segment if needed
         if(plainTextSegment != 0)
         {
            char startChar = mTextBuffer->getChar(segmentStart - plainTextSegment);
            char endChar = mTextBuffer->getChar(segmentStart - 1);

            LineSegment newSegment;
            newSegment.startPos = segmentStart - plainTextSegment;
            newSegment.endPos = segmentStart - 1;
            newSegment.type = Text;

            mSegments.push_back(newSegment);
         }

         LineSegment newSegment;
         newSegment.startPos = segmentStart;
         newSegment.endPos = segmentEnd;
         newSegment.type = segmentType;

         mSegments.push_back(newSegment);

         mScanPos = segmentEnd;

         segmentStart = segmentEnd = -1;
         plainTextSegment = 0;
      }
   }*/

   /*for( U32 c = 0; c < mTextLength; c++)
   {
      Line *line = tscp.getLineFromPosition(c);
      //now, run through the lines and build segments

      S32 lineStart = line->startPos;
      S32 lineEnd = line->endPos;

      UTF16 curChar = mTextBuffer->getChar(c);

      if(curChar == '%')
      {
         //real fast, check if we have a special case with "%this"
         S32 segEnd = findOnLine("%this", i, c);

         if(segEnd != -1)
         {
            //yup, was our keyword
               
            newSegment.startPos = c;
            newSegment.endPos = segEnd;
            newSegment.type = Keyword;

            mSegments.push_back(newSegment);
         }
         else
         {
            //nope, just a local variable
            segEnd = scanforScriptTerminator(str, c);

            
            newSegment.startPos = c;
            newSegment.endPos = segEnd;
            newSegment.type = LocalVariable;

            mSegments.push_back(newSegment);
         }
      }
      else if(curChar == '$')
      {
         //global variable
         S32 segEnd = scanforScriptTerminator(str, c);

         LineSegment newSegment;
         newSegment.startPos = c;
         newSegment.endPos = segEnd;
         newSegment.type = GlobalVariable;

         mSegments.push_back(newSegment);
      }
      else if(curChar == '/' && curChar++ == '/')
      {
         //line comment
         LineSegment newSegment;
         newSegment.startPos = c;
         newSegment.endPos = end;
         newSegment.type = Comment;

         mSegments.push_back(newSegment);
      }
      else if(curChar == '/' && curChar++ == '*')
      {
         //multiline comment
         //we need to find the end, if there is one
         S32 segEnd = find("*//*", c+2);

         LineSegment newSegment;
         newSegment.startPos = c;

         if(segEnd == -1)
            newSegment.endPos = mTextBuffer->length();
         else
            newSegment.endPos = segEnd;

         newSegment.type = MLComment;

         mSegments.push_back(newSegment);
      }
      else if(curChar == '\"')
      {
         //string
         //we need to find the end, if there is one
         S32 segEnd = find("\"", c++);

         LineSegment newSegment;
         newSegment.startPos = c;

         if(segEnd == -1)
            newSegment.endPos = mTextBuffer->length();
         else
            newSegment.endPos = segEnd;

         newSegment.type = StringType;

         mSegments.push_back(newSegment);
      }
      else if(curChar == '\'')
      {
         //tag
         //we need to find the end, if there is one
         S32 segEnd = find("\'", c++);

         LineSegment newSegment;
         newSegment.startPos = c;

         if(segEnd == -1)
            newSegment.endPos = mTextBuffer->length();
         else
            newSegment.endPos = segEnd;

         newSegment.type = Tag;

         mSegments.push_back(newSegment);
      }

      //lastly, check for any reserved/keywods
      for(S32 w=0; w < sReservedWords; w++)
      {
         S32 segEnd = findOnLine(keywords[w], i, c);    

         if(segEnd != -1)
         {
            LineSegment newSegment;
            newSegment.startPos = c;
            newSegment.endPos = segEnd;
            newSegment.type = Keyword;

            mSegments.push_back(newSegment);
         }
      }

      //regular text, so increment
      plainTextSegment++;
   }*/

	return true;
}

void TSCodeParser::buildObjectList()
{
	S32 startPos = 0;

	//first, we build our list of objects
	//for(U32 i
	//S32 TSCodeParser::findObjectName( S32 startPosition, String &objName)
}

String TSCodeParser::getObjectField( S32 objectPos, String &fieldName)
{
	/*mScanPos = startPosition;
	Line* l = getLineFromPosition(startPosition);
	if(l == NULL)
		return -1;

	while(l != NULL)
	{
		S32 start = startPosition > l->startPos ? startPosition : l->startPos;
		char* line = (char*)mTextBuffer->createSubstring8(start, l->endPos - start);

		//now, look for 'new'. If we haven't found 'new' in the line, we haven't declared anything.
		S32 pos = scanforword( dStrlwr(line), "new" );

		if(pos != -1)
		{
			//we have a declaration, so parse forward until we find a parenthesis
			S32 parthPos = scanforchar(line, pos+2, '(');

			if(parthPos != -1)
			{
				//found it. Now parse to the closing parinth
				S32 endParthPos = scanforchar(line, pos+2+parthPos+1, ')');

				if(endParthPos != -1)
				{
					//and we've got it!
					objName = (const char*)mTextBuffer->createSubstring8(start+pos+2+parthPos+1, endParthPos);

					return start+pos+2+parthPos+1;
				}
			}
		}
		
		l = getLineFromPosition(l->endPos+1);
	}

	return -1;*/
	return "";
}

void TSCodeParser::execute()
{
	Con::evaluate(getCode());
}
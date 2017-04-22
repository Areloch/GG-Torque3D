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

#include <stdarg.h>

#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/codeBlock.h"

//#include <WinBase.h>
#include <Windows.h>
#include <Dbghelp.h>
#include <process.h>
#include <iostream>

extern ExprEvalState gEvalState;

//-------------------------------------- STATIC Declaration
PlatformAssert *PlatformAssert::platformAssert = NULL;

//--------------------------------------
PlatformAssert::PlatformAssert()
{
   processing = false;
   ignoreAll = false;
}

//--------------------------------------
PlatformAssert::~PlatformAssert()
{
}

//--------------------------------------
void PlatformAssert::create( PlatformAssert* newAssertClass )
{
   if (!platformAssert)
      platformAssert = newAssertClass ? newAssertClass : new PlatformAssert;
}


//--------------------------------------
void PlatformAssert::destroy()
{
   if (platformAssert)
      delete platformAssert;
   platformAssert = NULL;
}


//--------------------------------------
bool PlatformAssert::displayMessageBox(const char *title, const char *message, bool retry)
{
   if (retry)
      return Platform::AlertRetry(title, message);

   Platform::AlertOK(title, message);
   return false;
}

static const char *typeName[] = { "Unknown", "Fatal-ISV", "Fatal", "Warning" };

//--------------------------------------

bool PlatformAssert::process(Type         assertType,
                             const char  *filename,
                             U32          lineNumber,
                             const char  *message)
{
    // If we're somehow recursing, just die.
    if(processing)
        Platform::debugBreak();
    
    processing = true;
    bool ret = false;
    
    // always dump to the Assert to the Console
    if (Con::isActive())
    {
        if (assertType == Warning)
            Con::warnf(ConsoleLogEntry::Assert, "%s(%ld,0): {%s} - %s", filename, lineNumber, typeName[assertType], message);
        else
            Con::errorf(ConsoleLogEntry::Assert, "%s(%ld,0): {%s} - %s", filename, lineNumber, typeName[assertType], message);
    }
    
    // if not a WARNING pop-up a dialog box
    if (assertType != Warning)
    {
        // used for processing navGraphs (an assert won't botch the whole build)
        if(Con::getBoolVariable("$FP::DisableAsserts", false) == true)
            Platform::forceShutdown(1);
        
        char buffer[2048];
        dSprintf(buffer, 2048, "%s: (%s @ %ld)", typeName[assertType], filename, lineNumber);
        if( !ignoreAll )
        {
            // Display message box with Debug, Ignore, Ignore All, and Exit options
            switch( Platform::AlertAssert(buffer, message) )
            {
                case Platform::ALERT_ASSERT_DEBUG:				
                    ret = true;
                    break;
                case Platform::ALERT_ASSERT_IGNORE:
                    ret = false;
                    break;
                case Platform::ALERT_ASSERT_IGNORE_ALL:
                    ignoreAll = true;
                    ret = false;
                    break;
                default:
                case Platform::ALERT_ASSERT_EXIT:
                    Platform::forceShutdown(1);
                    break;
            }
        }
    }
    
    processing = false;
    
    return ret;
}

bool PlatformAssert::processingAssert()
{
   return platformAssert ? platformAssert->processing : false;
}

//--------------------------------------
bool PlatformAssert::processAssert(Type        assertType,
                                   const char  *filename,
                                   U32         lineNumber,
                                   const char  *message)
{
   if (platformAssert)
      return platformAssert->process(assertType, filename, lineNumber, message);
   else // when platAssert NULL (during _start/_exit) try direct output...
      dPrintf("\n%s: (%s @ %ld) %s\n", typeName[assertType], filename, lineNumber, message);

   // this could also be platform-specific: OutputDebugString on PC, DebugStr on Mac.
   // Will raw printfs do the job?  In the worst case, it's a break-pointable line of code.
   // would have preferred Con but due to race conditions, it might not be around...
   // Con::errorf(ConsoleLogEntry::Assert, "%s: (%s @ %ld) %s", typeName[assertType], filename, lineNumber, message);

   return true;
}

//--------------------------------------
const char* avar(const char *message, ...)
{
   static char buffer[4096];
   va_list args;
   va_start(args, message);
   dVsprintf(buffer, sizeof(buffer), message, args);
   va_end(args);
   return( buffer );
}

#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024

//-----------------------------------------------------------------------------
//Callstack dumping for logging purposes when we assert/crash
void dumpCallstack()
{
   const U32 MaxCommandSize = 2048;

   char buffer[MaxCommandSize];
   char scope[MaxCommandSize];

   S32 last = 0;

   Con::printf("Dumping Console Callstack now:");

   for (S32 i = (S32)gEvalState.getStackDepth() - 1; i >= last; i--)
   {
      CodeBlock *code = gEvalState.stack[i]->code;
      const char *file = "<none>";
      if (code && code->name && code->name[0])
         file = code->name;

      Namespace *ns = gEvalState.stack[i]->scopeNamespace;
      scope[0] = 0;
      if (ns) {

         if (ns->mParent && ns->mParent->mPackage && ns->mParent->mPackage[0]) {
            dStrcat(scope, ns->mParent->mPackage);
            dStrcat(scope, "::");
         }
         if (ns->mName && ns->mName[0]) {
            dStrcat(scope, ns->mName);
            dStrcat(scope, "::");
         }
      }

      const char *function = gEvalState.stack[i]->scopeName;
      if ((!function) || (!function[0]))
         function = "";
      dStrcat(scope, function);

      U32 line = 0, inst;
      U32 ip = gEvalState.stack[i]->ip;
      if (code)
         code->findBreakLine(ip, line, inst);
      dSprintf(buffer, MaxCommandSize, "File: \"%s\" Line: %d Function: \"%s\"", file, line, scope);
      Con::printf(buffer);
   }

   Con::printf("Finished Dumping Console Callstack");
   Con::printf("");

   //Engine stacktrace
   Con::printf("Dumping Engine Callstack now:");
#ifdef TORQUE_OS_WIN
   void *stack[TRACE_MAX_STACK_FRAMES];

   HANDLE process = GetCurrentProcess();
   SymInitialize(process, NULL, TRUE);

   WORD count = CaptureStackBackTrace(0, TRACE_MAX_STACK_FRAMES, stack, NULL);
   SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO) + (TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR));
   symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
   symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

   IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
   line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

   DWORD dwDisplacement;

   for (int i = 0; i < count; i++)
   {
      DWORD64 address = (DWORD64)(stack[i]);
      SymFromAddr(process, address, NULL, symbol);
      if (SymGetLineFromAddr64(process, address, &dwDisplacement, line))
      {
         Con::printf("At %s in %s: line: %lu: address: 0x%0X", symbol->Name, line->FileName, line->LineNumber, symbol->Address);
      }
   }
#endif

   Con::printf("Finished Dumping Engine Callstack");
   Con::printf("");

   bool herpadoo = true;
}
//-----------------------------------------------------------------------------

ConsoleFunction( Assert, void, 3, 3, "(condition, message) - Fatal Script Assertion" )
{
    // Process Assertion.
    AssertISV( dAtob(argv[1]), argv[2] );
}

ConsoleFunction(barf, void, 2, 2, "")
{
   SimObject* bob = NULL;
   bob->getId();

   dumpCallstack();
}

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

#include "console/simBase.h"
#include "console/telnetDebugger.h"
#include "console/codeInterpreter.h"
#include "console/stringStack.h"
#include "console/compiler.h"
#include "console/consoleInternal.h"

using namespace Compiler;

enum EvalConstants
{
   MaxStackSize = 1024
};

namespace Con
{
   // Current script file name and root, these are registered as
   // console variables.
   extern StringTableEntry gCurrentFile;
   extern StringTableEntry gCurrentRoot;
   extern S32 gObjectCopyFailures;
}


extern ExprEvalState gEvalState;

char sTraceBuffer[1024];

StringStack sSTR;
ConsoleValueStack sCSTK;

U32 _FLT = 0;     ///< Stack pointer for floatStack.
U32 _UINT = 0;    ///< Stack pointer for intStack.
U32 _ITER = 0;    ///< Stack pointer for iterStack.

IterStackRecord iterStack[MaxStackSize];

F64 floatStack[MaxStackSize];
S64 intStack[MaxStackSize];

char curFieldArray[256];
char prevFieldArray[256];

CodeInterpreter::CodeInterpreter(CodeBlock *cb) :
   mCodeBlock(cb),
   mIterDepth(0),
   mCurFloatTable(nullptr),
   mCurStringTable(nullptr),
   mCurStringTableLen(0),
   mThisFunctionName(nullptr),
   mPopFrame(false),
   mTelDebuggerOn(false),
   mVar(nullptr),
   mObjParent(nullptr),
   mFnName(nullptr),
   mFnNamespace(nullptr),
   mFnPackage(nullptr),
   mObjectCreationStackIndex(0),
   mCurrentNewObject(nullptr),
   mFailJump(0),
   mPrevField(nullptr),
   mCurField(nullptr),
   mPrevObject(nullptr),
   mCurObject(nullptr),
   mSaveObject(nullptr),
   mNSEntry(nullptr),
   mNS(nullptr),
   mCurFNDocBlock(nullptr),
   mCurNSDocBlock(nullptr),
   mNSDocLength(0),
   mCallArgc(0),
   mCallArgv(nullptr),
   mSaveCodeBlock(nullptr),
   mVal(nullptr),
   mRetValue(0)
{
   // clear exec argument storage.
   memset(&mExec, 0, sizeof(mExec));

   // clear objCreationStack
   memset(mObjectCreationStack, 0, sizeof(mObjectCreationStack));
}

CodeInterpreter::~CodeInterpreter()
{

}

ConsoleValueRef CodeInterpreter::exec(U32 ip,
                                      StringTableEntry functionName,
                                      Namespace *thisNamespace,
                                      U32 argc, 
                                      ConsoleValueRef *argv,
                                      bool noCalls,
                                      StringTableEntry packageName,
                                      S32 setFrame) 
{
   mExec.ip = ip;
   mExec.functionName = functionName;
   mExec.argc = argc;
   mExec.argv = argv;
   mExec.noCalls = noCalls;
   mExec.packageName = packageName;
   mExec.setFrame = setFrame;

   sSTR.clearFunctionOffset(); // ensures arg buffer offset is back to 0

   // Lets load up our function arguments.
   parseArgs(ip);

   // Grab the state of the telenet debugger here once
   // so that the push and pop frames are always balanced.
   const bool telDebuggerOn = TelDebugger && TelDebugger->isConnected();
   if (telDebuggerOn && setFrame < 0)
      TelDebugger->pushStackFrame();

   mSaveCodeBlock = CodeBlock::smCurrentCodeBlock;
   CodeBlock::smCurrentCodeBlock = mCodeBlock;
   if (mCodeBlock->name)
   {
      Con::gCurrentFile = mCodeBlock->name;
      Con::gCurrentRoot = mCodeBlock->modPath;
   }

   while (true)
   {
      U32 instruction = mCodeBlock->code[ip++];
      mNSEntry = nullptr;

   breakContinue:
      switch (instruction)
      {
      case OP_FUNC_DECL:
         if (!op_func_decl(ip))
            goto exitLabel;
         break;
      case OP_CREATE_OBJECT:
         if (!op_create_object(ip))
            goto exitLabel;
         break;
      case OP_ADD_OBJECT:
         if (!op_add_object(ip))
            goto exitLabel;
         break;
      case OP_END_OBJECT:
         if (!op_end_object(ip))
            goto exitLabel;
         break;
      case OP_FINISH_OBJECT:
         if (!op_end_object(ip))
            goto exitLabel;
         break;
      case OP_JMPIFFNOT:
         if (!op_jmpiffnot(ip))
            goto exitLabel;
         break;
      case OP_JMPIFF:
         if (!op_jmpiff(ip))
            goto exitLabel;
         break;
      case OP_JMPIF:
         if (!op_jmpif(ip))
            goto exitLabel;
         break;
      case OP_JMPIFNOT_NP:
         if (!op_jmpifnot_np(ip))
            goto exitLabel;
         break;
      case OP_JMPIF_NP:
         if (!op_jmpif_np(ip))
            goto exitLabel;
         break;
      case OP_JMP:
         if (!op_jmp(ip))
            goto exitLabel;
         break;
      case OP_RETURN_VOID:
         if (!op_return_void(ip))
            goto exitLabel;
         break;
      case OP_RETURN:
         if (!op_return(ip))
            goto exitLabel;
         break;
      case OP_RETURN_FLT:
         if (!op_return_flt(ip))
            goto exitLabel;
         break;
      case OP_RETURN_UINT:
         if (!op_return_uint(ip))
            goto exitLabel;
         break;
      case OP_CMPEQ:
         if (!op_cmpeq(ip))
            goto exitLabel;
         break;
      case OP_CMPGR:
         if (!op_cmpgr(ip))
            goto exitLabel;
         break;
      case OP_CMPGE:
         if (!op_cmpge(ip))
            goto exitLabel;
         break;
      case OP_CMPLT:
         if (!op_cmplt(ip))
            goto exitLabel;
         break;
      case OP_CMPLE:
         if (!op_cmple(ip))
            goto exitLabel;
         break;
      case OP_CMPNE:
         if (!op_cmpne(ip))
            goto exitLabel;
         break;
      case OP_XOR:
         if (!op_xor(ip))
            goto exitLabel;
         break;
      case OP_MOD:
         if (!op_mod(ip))
            goto exitLabel;
         break;
      case OP_BITAND:
         if (!op_bitand(ip))
            goto exitLabel;
         break;
      case OP_BITOR:
         if (!op_bitor(ip))
            goto exitLabel;
         break;
      case OP_NOT:
         if (!op_not(ip))
            goto exitLabel;
         break;
      case OP_NOTF:
         if (!op_notf(ip))
            goto exitLabel;
         break;
      case OP_ONESCOMPLEMENT:
         if (!op_onescomplement(ip))
            goto exitLabel;
         break;
      case OP_SHR:
         if (!op_shr(ip))
            goto exitLabel;
         break;
      case OP_SHL:
         if (!op_shl(ip))
            goto exitLabel;
         break;
      case OP_ADD:
         if (!op_add(ip))
            goto exitLabel;
         break;
      case OP_SUB:
         if (!op_sub(ip))
            goto exitLabel;
         break;
      case OP_MUL:
         if (!op_mul(ip))
            goto exitLabel;
         break;
      case OP_DIV:
         if (!op_div(ip))
            goto exitLabel;
         break;
      case OP_NEG:
         if (!op_div(ip))
            goto exitLabel;
         break;
      case OP_SETCURVAR:
         if (!op_setcurvar(ip))
            goto exitLabel;
         break;
      case OP_SETCURVAR_CREATE:
         if (!op_setcurvar_create(ip))
            goto exitLabel;
         break;
      case OP_SETCURVAR_ARRAY:
         if (!op_setcurvar_array(ip))
            goto exitLabel;
         break;
      case OP_SETCURVAR_ARRAY_CREATE:
         if (!op_setcurvar_array_create(ip))
            goto exitLabel;
         break;
      case OP_LOADVAR_UINT:
         if (!op_loadvar_uint(ip))
            goto exitLabel;
         break;
      case OP_LOADVAR_FLT:
         if (!op_loadvar_flt(ip))
            goto exitLabel;
         break;
      case OP_LOADVAR_STR:
         if (!op_loadvar_str(ip))
            goto exitLabel;
         break;
      case OP_LOADVAR_VAR:
         if (!op_loadvar_var(ip))
            goto exitLabel;
         break;
      case OP_SAVEVAR_UINT:
         if (!op_savevar_uint(ip))
            goto exitLabel;
         break;
      case OP_SAVEVAR_FLT:
         if (!op_savevar_flt(ip))
            goto exitLabel;
         break;
      case OP_SAVEVAR_STR:
         if (!op_savevar_str(ip))
            goto exitLabel;
         break;
      case OP_SAVEVAR_VAR:
         if (!op_savevar_var(ip))
            goto exitLabel;
         break;
      case OP_SETCUROBJECT:
         if (!op_setcurobject(ip))
            goto exitLabel;
         break;
      case OP_SETCUROBJECT_INTERNAL:
         if (!op_setcurobject_internal(ip))
            goto exitLabel;
         break;
      case OP_SETCUROBJECT_NEW:
         if (!op_setcurobject_new(ip))
            goto exitLabel;
         break;
      case OP_SETCURFIELD:
         if (!op_setcurfield(ip))
            goto exitLabel;
         break;
      case OP_SETCURFIELD_ARRAY:
         if (!op_setcurfield_array(ip))
            goto exitLabel;
         break;
      case OP_SETCURFIELD_TYPE:
         if (!op_setcurfield_type(ip))
            goto exitLabel;
         break;
      case OP_LOADFIELD_UINT:
         if (!op_loadfield_uint(ip))
            goto exitLabel;
         break;
      case OP_LOADFIELD_FLT:
         if (!op_loadfield_flt(ip))
            goto exitLabel;
         break;
      case OP_LOADFIELD_STR:
         if (!op_loadfield_str(ip))
            goto exitLabel;
         break;
      case OP_SAVEFIELD_UINT:
         if (!op_savefield_uint(ip))
            goto exitLabel;
         break;
      case OP_SAVEFIELD_FLT:
         if (!op_savefield_flt(ip))
            goto exitLabel;
         break;
      case OP_SAVEFIELD_STR:
         if (!op_savefield_str(ip))
            goto exitLabel;
         break;
      case OP_STR_TO_UINT:
         if (!op_str_to_uint(ip))
            goto exitLabel;
         break;
      case OP_STR_TO_FLT:
         if (!op_str_to_flt(ip))
            goto exitLabel;
         break;
      case OP_STR_TO_NONE:
         if (!op_str_to_none(ip))
            goto exitLabel;
         break;
      case OP_FLT_TO_UINT:
         if (!op_flt_to_uint(ip))
            goto exitLabel;
         break;
      case OP_FLT_TO_STR:
         if (!op_flt_to_str(ip))
            goto exitLabel;
         break;
      case OP_FLT_TO_NONE:
         if (!op_flt_to_none(ip))
            goto exitLabel;
         break;
      case OP_UINT_TO_FLT:
         if (!op_uint_to_flt(ip))
            goto exitLabel;
         break;
      case OP_UINT_TO_STR:
         if (!op_uint_to_str(ip))
            goto exitLabel;
         break;
      case OP_UINT_TO_NONE:
         if (!op_uint_to_none(ip))
            goto exitLabel;
         break;
      case OP_COPYVAR_TO_NONE:
         if (!op_copyvar_to_none(ip))
            goto exitLabel;
         break;
      case OP_LOADIMMED_UINT:
         if (!op_loadimmed_uint(ip))
            goto exitLabel;
         break;
      case OP_LOADIMMED_FLT:
         if (!op_loadimmed_flt(ip))
            goto exitLabel;
         break;
      case OP_TAG_TO_STR:
         if (!op_tag_to_str(ip))
            goto exitLabel;
         break;
      case OP_LOADIMMED_STR:
         if (!op_loadimmed_str(ip))
            goto exitLabel;
         break;
      case OP_DOCBLOCK_STR:
         if (!op_docblock_str(ip))
            goto exitLabel;
         break;
      case OP_LOADIMMED_IDENT:
         if (!op_loadimmed_ident(ip))
            goto exitLabel;
         break;
      case OP_CALLFUNC_RESOLVE:
         if (!op_callfunc_resolve(ip))
            goto exitLabel;
         break;
      case OP_CALLFUNC:
         if (!op_callfunc(ip))
            goto exitLabel;
         break;
      case OP_ADVANCE_STR:
         if (!op_advance_str(ip))
            goto exitLabel;
         break;
      case OP_ADVANCE_STR_APPENDCHAR:
         if (!op_advance_str_appendchar(ip))
            goto exitLabel;
         break;
      case OP_ADVANCE_STR_COMMA:
         if (!op_advance_str_comma(ip))
            goto exitLabel;
         break;
      case OP_ADVANCE_STR_NUL:
         if (!op_advance_str_nul(ip))
            goto exitLabel;
         break;
      case OP_REWIND_STR:
         if (!op_rewind_str(ip))
            goto exitLabel;
         break;
      case OP_TERMINATE_REWIND_STR:
         if (!op_terminate_rewind_str(ip))
            goto exitLabel;
         break;
      case OP_COMPARE_STR:
         if (!op_compare_str(ip))
            goto exitLabel;
         break;
      case OP_PUSH:
         if (!op_push(ip))
            goto exitLabel;
         break;
      case OP_PUSH_UINT:
         if (!op_push_uint(ip))
            goto exitLabel;
         break;
      case OP_PUSH_FLT:
         if (!op_push_flt(ip))
            goto exitLabel;
         break;
      case OP_PUSH_VAR:
         if (!op_push_var(ip))
            goto exitLabel;
         break;
      case OP_PUSH_FRAME:
         if (!op_push_frame(ip))
            goto exitLabel;
         break;
      case OP_ASSERT:
         if (!op_assert(ip))
            goto exitLabel;
         break;
      case OP_BREAK:
         if (!op_break(ip))
            goto exitLabel;
         break;
      case OP_ITER_BEGIN_STR:
         if (!op_iter_begin_str(ip))
            goto exitLabel;
         break;
      case OP_ITER:
         if (!op_iter(ip))
            goto exitLabel;
         break;
      case OP_ITER_END:
         if (!op_iter_end(ip))
            goto exitLabel;
         break;
      case OP_INVALID:
         if (!op_invalid(ip))
            goto exitLabel;
         break;
      default:
         goto exitLabel;
      }
   }

   exitLabel:

   if (telDebuggerOn && setFrame < 0)
      TelDebugger->popStackFrame();

   if (mPopFrame)
      gEvalState.popFrame();

   if (argv)
   {
      if (gEvalState.traceOn)
      {
         sTraceBuffer[0] = 0;
         dStrcat(sTraceBuffer, "Leaving ");

         if (packageName)
         {
            dStrcat(sTraceBuffer, "[");
            dStrcat(sTraceBuffer, packageName);
            dStrcat(sTraceBuffer, "]");
         }
         if (thisNamespace && thisNamespace->mName)
         {
            dSprintf(sTraceBuffer + dStrlen(sTraceBuffer), sizeof(sTraceBuffer) - dStrlen(sTraceBuffer),
               "%s::%s() - return %s", thisNamespace->mName, mThisFunctionName, sSTR.getStringValue());
         }
         else
         {
            dSprintf(sTraceBuffer + dStrlen(sTraceBuffer), sizeof(sTraceBuffer) - dStrlen(sTraceBuffer),
               "%s() - return %s", mThisFunctionName, sSTR.getStringValue());
         }
         Con::printf("%s", sTraceBuffer);
      }
   }

   CodeBlock::smCurrentCodeBlock = mSaveCodeBlock;
   if (mSaveCodeBlock && mSaveCodeBlock->name)
   {
      Con::gCurrentFile = mSaveCodeBlock->name;
      Con::gCurrentRoot = mSaveCodeBlock->modPath;
   }

   mCodeBlock->decRefCount();

#ifdef TORQUE_VALIDATE_STACK
   AssertFatal(!(STR.mStartStackSize > stackStart), "String stack not popped enough in script exec");
   AssertFatal(!(STR.mStartStackSize < stackStart), "String stack popped too much in script exec");
#endif

   return mReturnValue;
}

void CodeInterpreter::parseArgs(U32 &ip)
{
   U32 *code = mCodeBlock->code;

   if (mExec.argc > 0)
   {
      U32 fnArgc = code[ip + 2 + 6];
      mThisFunctionName = Compiler::CodeToSTE(code, ip);
      S32 wantedArgc = getMin(mExec.argc - 1, fnArgc); // argv[0] is func name
      if (gEvalState.traceOn)
      {
         sTraceBuffer[0] = 0x0;
         dStrcat(sTraceBuffer, "Entering ");

         if (mExec.packageName)
         {
            dStrcat(sTraceBuffer, "[");
            dStrcat(sTraceBuffer, mExec.packageName);
            dStrcat(sTraceBuffer, "]");
         }
         if (mExec.thisNamespace && mExec.thisNamespace->mName)
         {
            dSprintf(sTraceBuffer + dStrlen(sTraceBuffer), sizeof(sTraceBuffer) - dStrlen(sTraceBuffer),
               "%s::%s(", mExec.thisNamespace->mName, mThisFunctionName);
         }
         else
         {
            dSprintf(sTraceBuffer + dStrlen(sTraceBuffer), sizeof(sTraceBuffer) - dStrlen(sTraceBuffer),
               "%s(", mThisFunctionName);
         }
         for (S32 i = 0; i < wantedArgc; i++)
         {
            dStrcat(sTraceBuffer, mExec.argv[i + 1]);
            if (i != wantedArgc - 1)
               dStrcat(sTraceBuffer, ", ");
         }
         dStrcat(sTraceBuffer, ")");
         Con::printf("%s", sTraceBuffer);
      }

      gEvalState.pushFrame(mThisFunctionName, mExec.thisNamespace);
      mPopFrame = true;

      for (S32 i = 0; i < wantedArgc; i++)
      {
         StringTableEntry var = Compiler::CodeToSTE(code, ip + (2 + 6 + 1) + (i * 2));
         gEvalState.setCurVarNameCreate(var);

         ConsoleValueRef ref = mExec.argv[i + 1];

         switch (ref.getType())
         {
         case ConsoleValue::TypeInternalInt:
            gEvalState.setIntVariable(ref);
            break;
         case ConsoleValue::TypeInternalFloat:
            gEvalState.setFloatVariable(ref);
            break;
         case ConsoleValue::TypeInternalStringStackPtr:
            gEvalState.setStringStackPtrVariable(ref.getStringStackPtrValue());
            break;
         case ConsoleValue::TypeInternalStackString:
         case ConsoleValue::TypeInternalString:
         default:
            gEvalState.setStringVariable(ref);
            break;
         }
      }

      ip = ip + (fnArgc * 2) + (2 + 6 + 1);
      mCurFloatTable = mCodeBlock->functionFloats;
      mCurStringTable = mCodeBlock->functionStrings;
      mCurStringTableLen = mCodeBlock->functionStringsMaxLen;
   }
   else
   {
      mCurFloatTable = mCodeBlock->globalFloats;
      mCurStringTable = mCodeBlock->globalStrings;
      mCurStringTableLen = mCodeBlock->globalStringsMaxLen;

      // If requested stack frame isn't available, request a new one
      // (this prevents assert failures when creating local
      //  variables without a stack frame)
      if (gEvalState.getStackDepth() <= mExec.setFrame)
         mExec.setFrame = -1;

      // Do we want this code to execute using a new stack frame?
      if (mExec.setFrame < 0)
      {
         gEvalState.pushFrame(NULL, NULL);
         mPopFrame = true;
      }
      else
      {
         // We want to copy a reference to an existing stack frame
         // on to the top of the stack.  Any change that occurs to 
         // the locals during this new frame will also occur in the 
         // original frame.
         S32 stackIndex = gEvalState.getStackDepth() - mExec.setFrame - 1;
         gEvalState.pushFrameRef(stackIndex);
         mPopFrame = true;
      }
   }
}
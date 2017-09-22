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

#ifndef _CODEINTERPRETER_H_
#define _CODEINTERPRETER_H_

#include "console/codeBlock.h"
#include "console/console.h"
#include "core/frameAllocator.h"

/// Frame data for a foreach/foreach$ loop.
struct IterStackRecord
{
   /// If true, this is a foreach$ loop; if not, it's a foreach loop.
   bool mIsStringIter;

   /// The iterator variable.
   Dictionary::Entry* mVariable;

   /// Information for an object iterator loop.
   struct ObjectPos
   {
      /// The set being iterated over.
      SimSet* mSet;

      /// Current index in the set.
      U32 mIndex;
   };

   /// Information for a string iterator loop.
   struct StringPos
   {
      /// The raw string data on the string stack.
      StringStackPtr mString;

      /// Current parsing position.
      U32 mIndex;
   };

   union
   {
      ObjectPos mObj;
      StringPos mStr;
   } mData;
};

class CodeInterpreter
{
public:
   CodeInterpreter(CodeBlock *cb);
   ~CodeInterpreter();

   ConsoleValueRef exec(U32 ip, 
                        StringTableEntry functionName,
                        Namespace *thisNamespace,
                        U32 argc, 
                        ConsoleValueRef *argv, 
                        bool noCalls, 
                        StringTableEntry packageName, 
                        S32 setFrame);

   // Methods
private:
   void parseArgs(U32 &ip);

   /// Group op codes
   /// @{

   bool op_func_decl(U32 &ip);
   bool op_create_object(U32 &ip);
   bool op_add_object(U32 &ip);
   bool op_end_object(U32 &ip);
   bool op_finish_object(U32 &ip);
   bool op_jmpiffnot(U32 &ip);
   bool op_jmpiff(U32 &ip);
   bool op_jmpif(U32 &ip);
   bool op_jmpifnot_np(U32 &ip);
   bool op_jmpif_np(U32 &ip);
   bool op_jmp(U32 &ip);
   bool op_return_void(U32 &ip);
   bool op_return(U32 &ip);
   bool op_return_flt(U32 &ip);
   bool op_return_uint(U32 &ip);
   bool op_cmpeq(U32 &ip);
   bool op_cmpgr(U32 &ip);
   bool op_cmpge(U32 &ip);
   bool op_cmplt(U32 &ip);
   bool op_cmple(U32 &ip);
   bool op_cmpne(U32 &ip);
   bool op_xor(U32 &ip);
   bool op_mod(U32 &ip);
   bool op_bitand(U32 &ip);
   bool op_bitor(U32 &ip);
   bool op_not(U32 &ip);
   bool op_notf(U32 &ip);
   bool op_onescomplement(U32 &ip);
   bool op_shr(U32 &ip);
   bool op_shl(U32 &ip);
   bool op_and(U32 &ip);
   bool op_or(U32 &ip);
   bool op_add(U32 &ip);
   bool op_sub(U32 &ip);
   bool op_mul(U32 &ip);
   bool op_div(U32 &ip);
   bool op_neg(U32 &ip);
   bool op_setcurvar(U32 &ip);
   bool op_setcurvar_create(U32 &ip);
   bool op_setcurvar_array(U32 &ip);
   bool op_setcurvar_array_create(U32 &ip);
   bool op_loadvar_uint(U32 &ip);
   bool op_loadvar_flt(U32 &ip);
   bool op_loadvar_str(U32 &ip);
   bool op_loadvar_var(U32 &ip);
   bool op_savevar_uint(U32 &ip);
   bool op_savevar_flt(U32 &ip);
   bool op_savevar_str(U32 &ip);
   bool op_savevar_var(U32 &ip);
   bool op_setcurobject(U32 &ip);
   bool op_setcurobject_internal(U32 &ip);
   bool op_setcurobject_new(U32 &ip);
   bool op_setcurfield(U32 &ip);
   bool op_setcurfield_array(U32 &ip);
   bool op_setcurfield_type(U32 &ip);
   bool op_loadfield_uint(U32 &ip);
   bool op_loadfield_flt(U32 &ip);
   bool op_loadfield_str(U32 &ip);
   bool op_savefield_uint(U32 &ip);
   bool op_savefield_flt(U32 &ip);
   bool op_savefield_str(U32 &ip);
   bool op_str_to_uint(U32 &ip);
   bool op_str_to_flt(U32 &ip);
   bool op_str_to_none(U32 &ip);
   bool op_flt_to_uint(U32 &ip);
   bool op_flt_to_str(U32 &ip);
   bool op_flt_to_none(U32 &ip);
   bool op_uint_to_flt(U32 &ip);
   bool op_uint_to_str(U32 &ip);
   bool op_uint_to_none(U32 &ip);
   bool op_copyvar_to_none(U32 &ip);
   bool op_loadimmed_uint(U32 &ip);
   bool op_loadimmed_flt(U32 &ip);
   bool op_tag_to_str(U32 &ip);
   bool op_loadimmed_str(U32 &ip);
   bool op_docblock_str(U32 &ip);
   bool op_loadimmed_ident(U32 &ip);
   bool op_callfunc_resolve(U32 &ip);
   bool op_callfunc(U32 &ip);
   bool op_advance_str(U32 &ip);
   bool op_advance_str_appendchar(U32 &ip);
   bool op_advance_str_comma(U32 &ip);
   bool op_advance_str_nul(U32 &ip);
   bool op_rewind_str(U32 &ip);
   bool op_terminate_rewind_str(U32 &ip);
   bool op_compare_str(U32 &ip);
   bool op_push(U32 &ip);
   bool op_push_uint(U32 &ip);
   bool op_push_flt(U32 &ip);
   bool op_push_var(U32 &ip);
   bool op_push_frame(U32 &ip);
   bool op_assert(U32 &ip);
   bool op_break(U32 &ip);
   bool op_iter_begin_str(U32 &ip);
   bool op_iter_begin(U32 &ip);
   bool op_iter(U32 &ip);
   bool op_iter_end(U32 &ip);
   bool op_invalid(U32 &ip);

   /// @}

private:
   CodeBlock *mCodeBlock;

   /// Group exec arguments.
   struct
   {
      StringTableEntry functionName;
      Namespace *thisNamespace;
      U32 argc;
      ConsoleValueRef *argv;
      bool noCalls;
      StringTableEntry packageName;
      S32 setFrame;
   } mExec;

   U32 mIterDepth;
   F64 *mCurFloatTable;
   char *mCurStringTable;
   S32 mCurStringTableLen; ///< clint to ensure we dont overwrite it
   StringTableEntry mThisFunctionName;
   bool mPopFrame;
   bool mTelDebuggerOn;

   StringTableEntry mVar;
   StringTableEntry mObjParent;
   StringTableEntry mFnName;
   StringTableEntry mFnNamespace;
   StringTableEntry mFnPackage;

   // Add local object creation stack [7/9/2007 Black]
   static const U32 objectCreationStackSize = 32;
   U32 mObjectCreationStackIndex;
   struct 
   {
      SimObject *newObject;
      U32 failJump;
   } mObjectCreationStack[objectCreationStackSize];

   SimObject *mCurrentNewObject;
   U32 mFailJump;
   StringTableEntry mPrevField;
   StringTableEntry mCurField;
   SimObject *mPrevObject;
   SimObject *mCurObject;
   SimObject *mSaveObject;
   Namespace::Entry *mNSEntry;
   Namespace *mNS;
   StringTableEntry mCurFNDocBlock;
   StringTableEntry mCurNSDocBlock;
   S32 mNSDocLength;
   U32 mCallArgc;
   ConsoleValueRef *mCallArgv;
   CodeBlock *mSaveCodeBlock;
   StringTableEntry mVal;
   StringStackPtr mRetValue;

   // note: anything returned is pushed to CSTK and will be invalidated on the next exec()
   ConsoleValueRef mReturnValue;

   // The frame temp is used by the variable accessor ops (OP_SAVEFIELD_* and
   // OP_LOADFIELD_*) to store temporary values for the fields.
   //static S32 VAL_BUFFER_SIZE = 1024;
   //FrameTemp<char> valBuffer(VAL_BUFFER_SIZE);
};

#endif
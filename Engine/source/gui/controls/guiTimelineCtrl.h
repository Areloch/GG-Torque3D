//-----------------------------------------------------------------------------
// Verve
// Copyright (C) 2014 - Violent Tulip
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
#pragma once

#ifndef GUITIMELINECTRL_H
#define GUITIMELINECTRL_H

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

//-----------------------------------------------------------------------------

class guiTimelineCtrl : public GuiControl
{
   typedef GuiControl Parent;

public:

   struct TimelineController
   {
      S32 mTime;
      bool mIsPlaying;

      bool isPlaying() { return mIsPlaying; }
      F32 getDuration() { return 10; }
      F32 getTime() { return mTime; }
      void setTime(S32 _time) { mTime = _time; }

      void reset(S32 _time) { }

      void stop(bool reset) 
      { 
         mIsPlaying = false;

         if (reset)
            mTime = 0;
      }

      TimelineController() : mTime(0), mIsPlaying(false)
      {

      }
   };

   struct sSelection
   {
      bool   Active;
      S32    StartTime;
      S32    EndTime;
   };

   bool            mIsController;
   TimelineController    *mController;

   S32             mDurationOffset;

   sSelection      mSelection;

public:

   guiTimelineCtrl(void);

   static void     initPersistFields(void);

   // Mouse.

   virtual void    onMouseDown(const GuiEvent &pEvent);
   virtual void    onMouseUp(const GuiEvent &pEvent);
   virtual void    onMouseDragged(const GuiEvent &pEvent);

   virtual void    onRightMouseDown(const GuiEvent &pEvent);

   void            onMouseEvent(const char *pEventName, const GuiEvent &pEvent);

   // Rendering.

   void            onPreRender(void);
   void            onRender(Point2I offset, const RectI &updateRect);

   // Console Declaration.

   DECLARE_CONOBJECT(guiTimelineCtrl);

public:

   S32             toTime(const S32 &pPoint);
   S32             toPoint(const S32 &pTime);

   void            updateDuration(void);
};

//-----------------------------------------------------------------------------

#endif // _VT_VTIMELINECONTROL_H_
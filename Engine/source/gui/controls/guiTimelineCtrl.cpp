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
#include "gui/controls/guiTimelineCtrl.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"

//-----------------------------------------------------------------------------

const S32 gUnitsPerSec = 200;

//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(guiTimelineCtrl);
//-----------------------------------------------------------------------------

guiTimelineCtrl::guiTimelineCtrl(void) :
   mIsController(true),
   mController(NULL),
   mDurationOffset(50)
{
   mSelection.Active = false;
   mSelection.StartTime = 0;
   mSelection.EndTime = 0;

   mController = new TimelineController();
}

void guiTimelineCtrl::initPersistFields(void)
{
   Parent::initPersistFields();

   addField("IsController", TypeBool, Offset(mIsController, guiTimelineCtrl));
   //addField("Controller", TYPEID<VController>(), Offset(mController, guiTimelineCtrl));
   addField("DurationOffset", TypeS32, Offset(mDurationOffset, guiTimelineCtrl));
}

//-----------------------------------------------------------------------------
//
// Mouse Methods.
//
//-----------------------------------------------------------------------------

void guiTimelineCtrl::onMouseDown(const GuiEvent &pEvent)
{
   Parent::onMouseDown(pEvent);

   if (!mIsController || !mController || mController->isPlaying())
   {
      return;
   }

   if (!isMouseLocked())
   {
      GuiCanvas *canvas = getRoot();
      if (canvas->getMouseLockedControl())
      {
         GuiEvent event;
         canvas->getMouseLockedControl()->onMouseLeave(event);
         canvas->mouseUnlock(canvas->getMouseLockedControl());
      }

      // Lock.
      mouseLock();
   }

   // Calculate Time.
   const Point2I hitPoint = globalToLocalCoord(pEvent.mousePoint);
   const S32     time = mClamp(toTime(hitPoint.x), 0, mController->getDuration());

   // Selection?
   if (pEvent.modifier & SI_SHIFT)
   {
      if (!mSelection.Active)
      {
         // Selection Active.
         mSelection.Active = true;
         mSelection.StartTime = mController->getTime();
         mSelection.EndTime = time;
      }
      else
      {
         // Update Selection.
         mSelection.EndTime = time;
      }

      // Callback.
      Con::executef(this, "onSelectionUpdate");
   }
   else
   {
      if (mSelection.Active)
      {
         // Selection Invalid.
         mSelection.Active = false;

         // Callback.
         Con::executef(this, "onSelectionUpdate");
      }
   }

   // Set First Responder.
   setFirstResponder();

   if (pEvent.modifier & SI_CTRL)
   {
      // Set Time, No Reset.
      mController->setTime(time);
   }
   else
   {
      // Reset.
      mController->reset(time);
   }
}

void guiTimelineCtrl::onMouseUp(const GuiEvent &pEvent)
{
   if (isMouseLocked())
   {
      // Unlock.
      mouseUnlock();
   }

   if (mIsController && mController && !mController->isPlaying())
   {
      // Stop without Reset.
      mController->stop(false);
   }
}

void guiTimelineCtrl::onMouseDragged(const GuiEvent &pEvent)
{
   Parent::onMouseDragged(pEvent);

   if (!mIsController || !mController || mController->isPlaying())
   {
      return;
   }

   // Calculate Time.
   const Point2I hitPoint = globalToLocalCoord(pEvent.mousePoint);
   const S32     time = mClamp(toTime(hitPoint.x), 0, mController->getDuration());

   if (pEvent.modifier & SI_SHIFT)
   {
      if (mSelection.Active)
      {
         // Update Selection.
         mSelection.EndTime = time;

         // Callback.
         Con::executef(this, "onSelectionUpdate");
      }
   }
   else
   {
      if (mSelection.Active)
      {
         // Selection Invalid.
         mSelection.Active = false;

         // Callback.
         Con::executef(this, "onSelectionUpdate");
      }
   }

   if (pEvent.modifier & SI_CTRL)
   {
      // Set Time, No Reset.
      mController->setTime(time);
   }
   else if (!mSelection.Active)
   {
      // Reset.
      mController->reset(time);
   }
}

void guiTimelineCtrl::onRightMouseDown(const GuiEvent &pEvent)
{
   Parent::onRightMouseDown(pEvent);

   if (!mIsController || !mController || mController->isPlaying())
   {
      return;
   }

   // Calculate Time.
   const Point2I hitPoint = globalToLocalCoord(pEvent.mousePoint);
   const S32     time = mClamp(toTime(hitPoint.x), 0, mController->getDuration());

   // Set First Responder.
   setFirstResponder();

   if (mSelection.Active)
   {
      const S32 minTime = getMin(mSelection.StartTime, mSelection.EndTime);
      const S32 maxTime = getMax(mSelection.StartTime, mSelection.EndTime);
      if (time >= minTime && time <= maxTime)
      {
         // Callback.
         onMouseEvent("onSelectionRightClick", pEvent);

         // Don't Update Time.
         return;
      }
      else
      {
         if (mSelection.Active)
         {
            // Selection Invalid.
            mSelection.Active = false;

            // Callback.
            Con::executef(this, "onSelectionUpdate");
         }
      }
   }

   // Reset.
   mController->reset(time);
}

void guiTimelineCtrl::onMouseEvent(const char *pEventName, const GuiEvent &pEvent)
{
   // Argument Buffers.
   char argBuffer[3][32];

   // Format Event-Position Buffer.
   dSprintf(argBuffer[0], 32, "%d %d", pEvent.mousePoint.x, pEvent.mousePoint.y);

   // Format Event-Modifier Buffer.
   dSprintf(argBuffer[1], 32, "%d", pEvent.modifier);

   // Format Mouse-Click Count Buffer.
   dSprintf(argBuffer[2], 32, "%d", pEvent.mouseClickCount);

   // Call Scripts.
   Con::executef(this, pEventName, argBuffer[0], argBuffer[1], argBuffer[2]);
}

//-----------------------------------------------------------------------------
//
// Render Methods.
//
//-----------------------------------------------------------------------------

void guiTimelineCtrl::onPreRender(void)
{
   setUpdate();
}

void guiTimelineCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   if (!mController)
   {
      // Default Render.
      Parent::onRender(offset, updateRect);

      // Quit.
      return;
   }

   // Render Properties.
   const S32 tickOffset = toPoint(0);
   const S32 timeLineWidth = toPoint(mController->getDuration()) - tickOffset;
   const F32 tickStep = 0.5f;
   const S32 tickInterval = (mIsController) ? getWidth() : timeLineWidth;
   const S32 tickIntervalCount = (S32)mFloor(tickInterval / (gUnitsPerSec * tickStep)) + 1;

   // Tick Render Proeprties.
   const Point2I tickExtent(0, getHeight() - 1);

   // Text Render Properties.
   const Point2I textExtent(gUnitsPerSec, mProfile->mFontSize);
   const Point2I textOffset(4, -mProfile->mFontSize);

   // Render Border.
   GFX->getDrawUtil()->drawRectFill(RectI(offset + Point2I(tickOffset + 1, 1), Point2I(timeLineWidth - 1, getHeight() - 1)), mProfile->mFillColorHL);

   // Font Color.
   GFX->getDrawUtil()->setBitmapModulation(mProfile->mFontColor);

   for (S32 i = 0; i < tickIntervalCount; i++)
   {
      // Tick Position.
      const Point2I tickPosition = offset + Point2I(tickOffset + i * (gUnitsPerSec * tickStep), 0);

      // Line Color.
      const ColorI lineColor = ((i % 2)) ? mProfile->mBorderColorHL : mProfile->mBorderColor;

      // Draw Line.
      GFX->getDrawUtil()->drawLine(tickPosition, tickPosition + tickExtent, lineColor);

      if (mIsController)
      {
         // Render Times.
         renderJustifiedText(tickPosition + tickExtent + textOffset, textExtent, avar("%.2f", (F32)(i * tickStep)));
      }
   }

   // Render Children
   renderChildControls(offset, updateRect);

   if (mSelection.Active)
   {
      // Selection Width.
      const S32 selectionWidth = mCeil(mAbs(toPoint(mSelection.EndTime) - toPoint(mSelection.StartTime)));

      // Selection Position.
      const S32 selectionPositionX = toPoint(getMin(mSelection.StartTime, mSelection.EndTime));

      // Selection Properties.
      const Point2I selectionExtent(selectionWidth, getHeight());
      const Point2I selectionPosition = offset + Point2I(selectionPositionX, 0);

      // Render Time Cue.
      GFX->getDrawUtil()->drawRectFill(RectI(selectionPosition, selectionExtent), ColorI(0, 0, 0, 128));

      if (mIsController)
      {
         // Buffer.
         char buffer[2][128];
         dSprintf(buffer[0], 128, "%.2f", (F32)(mSelection.StartTime / 1000.f));
         dSprintf(buffer[1], 128, "%.2f", (F32)(mSelection.EndTime / 1000.f));

         if (mSelection.StartTime < mSelection.EndTime)
         {
            // Fetch Width.
            const S32 textWidth = mProfile->mFont->getStrWidth(buffer[0]);

            // Text Position.
            const Point2I startText = Point2I(getMax((S32)(selectionPosition.x - (textWidth + 2)), updateRect.point.x + 4), selectionPosition.y + 2);
            const Point2I endText = Point2I(getMin((S32)(selectionPosition.x + selectionWidth + 4), updateRect.point.x + updateRect.extent.x - (textWidth + 2)), selectionPosition.y + 2);

            // Render Time Text.
            renderJustifiedText(startText, textExtent, buffer[0]);
            renderJustifiedText(endText, textExtent, buffer[1]);
         }
         else
         {
            // Fetch Width.
            const S32 textWidth = mProfile->mFont->getStrWidth(buffer[1]);

            // Text Position.
            const Point2I startText = Point2I(getMax((S32)(selectionPosition.x - (textWidth + 2)), updateRect.point.x + 4), selectionPosition.y + 2);
            const Point2I endText = Point2I(getMin((S32)(selectionPosition.x + selectionWidth + 4), updateRect.point.x + updateRect.extent.x - (textWidth + 2)), selectionPosition.y + 2);

            // Render Time Text.
            renderJustifiedText(startText, textExtent, buffer[1]);
            renderJustifiedText(endText, textExtent, buffer[0]);
         }
      }
   }

   if (mController && !mSelection.Active)
   {
      // Time Cue Properties.
      const Point2I timeCueExtent((mIsController) ? 4 : 2, getHeight());
      const Point2I timeCuePosition = offset + Point2I(toPoint(mController->getTime()) - (timeCueExtent.x / 2), 0);

      // Render Time Cue.
      GFX->getDrawUtil()->drawRectFill(RectI(timeCuePosition, timeCueExtent), ColorI(0, 0, 0, 128));

      if (mIsController)
      {
         // Buffer.
         char buffer[128];
         dSprintf(buffer, 128, "%.2f", (F32)(mController->getTime() / 1000.f));

         // Fetch Width.
         const S32 textWidth = mProfile->mFont->getStrWidth(buffer);

         // Text Position.
         const Point2I textPosition(getMin(getMax(timeCuePosition.x + 6, updateRect.point.x + 4), updateRect.point.x + updateRect.extent.x - (textWidth + 2)), timeCuePosition.y + 2);

         // Render Time Text.
         renderJustifiedText(textPosition, textExtent, buffer);
      }
   }
}

//-----------------------------------------------------------------------------
//
// Console Methods.
//
//-----------------------------------------------------------------------------

ConsoleMethod(guiTimelineCtrl, toPoint, S32, 3, 3, "( pTime )")
{
   return object->toPoint(dAtoi(argv[2]));
}

S32 guiTimelineCtrl::toTime(const S32 &pPoint)
{
   return ((S32)(1000.f * (F32)pPoint / gUnitsPerSec) - mDurationOffset);
}

ConsoleMethod(guiTimelineCtrl, toTime, S32, 3, 3, "( pPoint )")
{
   return object->toTime(dAtoi(argv[2]));
}

S32 guiTimelineCtrl::toPoint(const S32 &pTime)
{
   return (S32)(gUnitsPerSec * ((F32)(pTime + mDurationOffset) / 1000.f));
}

ConsoleMethod(guiTimelineCtrl, getSelection, const char *, 2, 2, "( )")
{
   const S32 minTime = getMin(object->mSelection.StartTime, object->mSelection.EndTime);
   const S32 maxTime = getMax(object->mSelection.StartTime, object->mSelection.EndTime);

   // Fetch Return Buffer.
   char *retBuffer = Con::getReturnBuffer(256);

   // Write.
   dSprintf(retBuffer, 256, "%d %d %d", object->mSelection.Active, minTime, maxTime - minTime);

   // Return.
   return retBuffer;
}

ConsoleMethod(guiTimelineCtrl, setSelection, void, 3, 5, "( pActive, [pTime, pDuration] )")
{
   object->mSelection.Active = dAtob(argv[2]);
   if (argc > 3)
   {
      object->mSelection.StartTime = dAtoi(argv[3]);
      object->mSelection.EndTime = object->mSelection.StartTime + dAtoi(argv[4]);
   }
}

ConsoleMethod(guiTimelineCtrl, updateDuration, void, 2, 2, "( )")
{
   object->updateDuration();
}

void guiTimelineCtrl::updateDuration(void)
{
   if (!mController)
   {
      // No Controller.
      return;
   }

   // Add 500ms.
   const S32 length = toPoint(mController->getDuration() + 500);

   // Set Min Extent.
   setMinExtent(Point2I(length, getHeight()));

   if (getWidth() < length)
   {
      // Conform to Min Extent.
      setExtent(length, getHeight());
   }
}
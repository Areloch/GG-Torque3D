#ifndef _BASE_TOOL_H_
#define _BASE_TOOL_H_

#ifndef _EDITTSCTRL_H_
#include "gui/worldEditor/editTSCtrl.h"
#endif
#ifndef _UNDO_H_
#include "util/undo.h"
#endif
#ifndef _GIZMO_H_
#include "gui/worldEditor/gizmo.h"
#endif

class EditTSCtrl;

class BaseTool : public SimObject
{
public:

   enum EventResult
   {
      NotHandled = 0,
      Handled = 1,
      Done = 2,
      Failed = 3
   };

   BaseTool() {}
   ~BaseTool() {}

   virtual void onActivated( BaseTool *prevTool ){}
   virtual void onDeactivated( BaseTool *newTool ){}

   virtual EventResult onKeyDown( const GuiEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseDown( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseUp( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseMove( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseDragged( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseEnter( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseLeave( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseDown( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseUp( const Gui3DMouseEvent &event ) { return NotHandled; }

   virtual void updateGizmo() {}
   
   virtual void renderScene(const RectI & updateRect) {}
   virtual void render2D() {} 

   bool isDone() { return mDone; }

public:
	WorldEditor *mEditor;

protected:
   bool mDone;

   //DECLARE_CONOBJECT(BaseTool);
};

#endif
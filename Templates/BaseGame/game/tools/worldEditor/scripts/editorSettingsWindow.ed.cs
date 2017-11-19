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

function ESettingsWindow::startup( %this )
{
   ESettingsWindowTabBook.selectPage( 0 );
   ESettingsWindowList.setSelectedById( 0 );
}

function ESettingsWindow::onWake( %this )
{
   %this.editorSettings = new ScriptObject();
   
   %this.addPage("AxisGizmo", "Axis Gizmo");  
   %this.addPage("CameraSettings", "CameraSettings");
   %this.addPage("GeneralSettings", "General Settings");
   %this.addPage("ObjectEditor", "Object Editor");
   %this.addPage("RenderSettings", "Render Settings");
}

function ESettingsWindow::hideDialog( %this )
{
   %this.setVisible(false);
}

function ESettingsWindow::ToggleVisibility()
{
   if ( ESettingsWindow.visible  )
   {
      ESettingsWindow.setVisible(false);
   }
   else
   {
      ESettingsWindow.setVisible(true);
      ESettingsWindow.selectWindow();
      ESettingsWindow.setCollapseGroup(false);
   }
}

function ESettingsWindow::addTabPage( %this, %page )
{
}

function ESettingsWindow::addPage(%this, %pageName, %pageText)
{
   if(!isObject(%this.pageList))
      %this.pageList = new ArrayObject();
      
   ESettingsWindowList.addRow( %this.pageList.count(), %pageText );
   ESettingsWindowList.sort(0);
   
   %idx = ESettingsWindowList.findTextIndex(%pageText);
   %textIdx = ESettingsWindowList.findTextIndex(%pageText);
   %this.pageList.insert(%pageName, "", %textIdx);   
}

//-----------------------------------------------------------------------------

function ESettingsWindowList::onSelect( %this, %id, %text )
{
   //ESettingsWindowTabBook.selectPage( %id );
   %text = ESettingsWindow.pageList.getKey(%id);
   %pageCommand = "ESettingsWindow.on" @ ESettingsWindow.pageList.getKey(%id) @ "PageSelect();";
   eval(%pageCommand);
}

//-----------------------------------------------------------------------------
// Standard settings GUI classes.  Editors may define their own methods of
// working with settings and are not required to use these.
//-----------------------------------------------------------------------------

function ESettingsWindowCheckbox::onWake( %this )
{
   %this.setStateOn( EditorSettings.value( %this.editorSettingsValue ));
}

function ESettingsWindowCheckbox::onClick( %this )
{
   EditorSettings.setValue( %this.editorSettingsValue, %this.getValue() );
   eval(%this.editorSettingsRead);
}

//-----------------------------------------------------------------------------

function ESettingsWindowTextEdit::onWake( %this )
{
   %this.setText( EditorSettings.value( %this.editorSettingsValue ));
}

function ESettingsWindowTextEdit::onValidate( %this )
{
   EditorSettings.setValue( %this.editorSettingsValue, %this.getValue() );
   eval(%this.editorSettingsRead);
}

function ESettingsWindowTextEdit::onGainFirstResponder( %this )
{
   %this.selectAllText();
}

//-----------------------------------------------------------------------------

function ESettingsWindowColor::apply( %this, %color )
{
   EditorSettings.setValue( %this.editorSettingsValue, %color );
   eval(%this.editorSettingsRead);

   %this.findObjectByInternalName("ColorEdit", true).setText( %color);
   %this.findObjectByInternalName("ColorButton", true).color = ColorIntToFloat( %color );
}

function ESettingsWindowColorEdit::onWake( %this )
{
   %this.setText( EditorSettings.value( %this.getParent().editorSettingsValue ));
}

function ESettingsWindowColorEdit::onValidate( %this )
{
   %this.getParent().apply( %this.getValue() );
}

function ESettingsWindowColorEdit::onGainFirstResponder( %this )
{
   %this.selectAllText();
}

function ESettingsWindowColorButton::onWake( %this )
{
   %this.color = ColorIntToFloat( EditorSettings.value( %this.getParent().editorSettingsValue ) );
}

function ESettingsWindowColorButton::onClick( %this )
{
   getColorI( ColorFloatToInt( %this.color ), %this.getId() @ ".apply", %this.getRoot() );
   //EditorSettings.setValue( %this.editorSettingsValue, %this.getValue() );
   //eval(%this.editorSettingsRead);
}

function ESettingsWindowColorButton::apply( %this, %color )
{
   %this.getParent().apply(%color);
   echo("ESettingsWindowColorButton::apply(): " @ %color);
}

function ESettingsWindow::onAxisGizmoPageSelect(%this)
{
   EditorSettingsInspector.clearFields();
   
   EditorSettingsInspector.startGroup("Gizmo");
   EditorSettingsInspector.addField("mouseRotateScalar", "Rotate Scalar", "float", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("mouseScaleScalar", "Scale Scalar", "float", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("renderWhenUsed", "Render Manipulated", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("renderInfoText", "Render Tool Text", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();
   
   EditorSettingsInspector.startGroup("Grid");
   EditorSettingsInspector.addField("renderPlane", "Render Plane", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("renderPlaneHashes", "Render Plane Hashes", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("planeDim", "Plane Size", "float", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("gridColor", "Plane Color", "color", "", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();
}

function ESettingsWindow::onCameraSettingsPageSelect(%this)
{
   EditorSettingsInspector.clearFields();
   
   EditorSettingsInspector.startGroup("Mouse Control");
   EditorSettingsInspector.addField("mouseRotateScalar", "Invert Y Axis", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("mouseScaleScalar", "Invert X Axis", "bool", "", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();

   /*EditorSettingsInspector.startGroup("Grid");
   EditorSettingsInspector.addField("renderPlane", "Render Plane", "bool", "", "", EditorSettings);
   EditorSettingsInspector.addField("renderPlaneHashes", "Render Plane Hashes", "bool", "", "", EditorSettings);
   EditorSettingsInspector.addField("planeDim", "Plane Size", "float", "", "", EditorSettings);
   EditorSettingsInspector.addField("gridColor", "Plane Color", "color", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();*/
}

function ESettingsWindow::onGeneralSettingsPageSelect(%this)
{
   EditorSettingsInspector.clearFields();
   
   EditorSettingsInspector.startGroup("Paths");
   EditorSettingsInspector.addField("newLevelTemplatePath", "New Level", "filename", "", "", "", EditorSettings);
   EditorSettingsInspector.addField("TorsionPath", "Torsion Path", "fileName", "", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();
   
   EditorSettingsInspector.startGroup("Asset Edit Modes");
   EditorSettingsInspector.addField("materialEditMode", "Material Edit Mode", "list", "", "MaterialEditor", "MaterialEditor,ShaderGraphEditor", EditorSettings);
   //EditorSettingsInspector.addField("TorsionPath", "Torsion Path", "fileName", "", "", "", EditorSettings);
   EditorSettingsInspector.endGroup();
}

function ESettingsWindow::onRenderSettingsPageSelect(%this)
{
   EditorSettingsInspector.clearFields();
   
   EditorSettingsInspector.startGroup("Render Mode");
   EditorSettingsInspector.addField("renderMode", "Renderer Mode", "list", "", "Deferred", "Forward,Deferred", EditorSettings);
   EditorSettingsInspector.addField("lightingModel", "Lighting Model", "list", "", "PBR", "Legacy,PBR", EditorSettings);
   EditorSettingsInspector.endGroup();
}

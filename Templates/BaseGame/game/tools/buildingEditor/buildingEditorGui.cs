
$building::wireframe = true;
$building::showSpline = true;
$building::showReflectPlane = false;
$building::showRoad = true;
$building::breakAngle = 3.0;
   
function buildingEditorGui::onWake( %this )
{   
   $building::EditorOpen = true; 
   
   buildingEditorToolbar-->objectGridSnapBtn.setStateOn(%this.getGridSnapping());
   buildingEditorToolbar-->gridSnapSizeEdit.setText(%this.getGridSnapSize());
   
   buildingEditorToolbar-->objectAngleSnapBtn.setStateOn(%this.getAngleSnapping());
   
   %count = EWorldEditor.getSelectionSize();
   for ( %i = 0; %i < %count; %i++ )
   {
      %obj = EWorldEditor.getSelectedObject(%i);
      if ( %obj.getClassName() !$= "building" )
         EWorldEditor.unselectObject();
      else
         %this.setSelectedRoad( %obj );
   }      
   
   //%this-->TabBook.selectPage(0);
     
   %this.onNodeSelected(-1);
}

function buildingEditorGui::onSleep( %this )
{
   $building::EditorOpen = false;    
}

function buildingEditorGui::paletteSync( %this, %mode )
{
   %evalShortcut = "ToolsPaletteArray-->" @ %mode @ ".setStateOn(1);";
   eval(%evalShortcut);
}   
function buildingEditorGui::onEscapePressed( %this )
{
   if( %this.getMode() $= "BuildingEditorAddNodeMode" )
   {
      %this.prepSelectionMode();
      return true;
   }
   return false;
}
function buildingEditorGui::onRoadSelected( %this, %road )
{
   %this.road = %road;
   
   // Update the materialEditorList
   if( isObject( %road ) )
      $Tools::materialEditorList = %road.getId();
   
   buildingInspector.inspect( %road );  
   buildingTreeView.buildVisibleTree(true);
   if( buildingTreeView.getSelectedObject() != %road )
   {
      buildingTreeView.clearSelection();
      %treeId = buildingTreeView.findItemByObjectId( %road );
      buildingTreeView.selectItem( %treeId );  
   }
}

function buildingEditorGui::onNodeSelected( %this, %nodeIdx )
{
   if ( %nodeIdx == -1 )
   {
      buildingEditorOptionsWindow-->position.setActive( false );
      buildingEditorOptionsWindow-->position.setValue( "" );    
      
      buildingEditorOptionsWindow-->rotation.setActive( false );
      buildingEditorOptionsWindow-->rotation.setValue( "" );
      
      buildingEditorOptionsWindow-->width.setActive( false );
      buildingEditorOptionsWindow-->width.setValue( "" ); 
      
      buildingEditorOptionsWindow-->depth.setActive( false );
      buildingEditorOptionsWindow-->depth.setValue( "" );  
   }
   else
   {
      buildingEditorOptionsWindow-->position.setActive( true );
      buildingEditorOptionsWindow-->position.setValue( %this.getNodePosition() );    
      
      buildingEditorOptionsWindow-->rotation.setActive( true );
      buildingEditorOptionsWindow-->rotation.setValue( %this.getNodeNormal() );
      
      buildingEditorOptionsWindow-->width.setActive( true );
      buildingEditorOptionsWindow-->width.setValue( %this.getNodeWidth() ); 
      
      buildingEditorOptionsWindow-->depth.setActive( true );
      buildingEditorOptionsWindow-->depth.setValue( %this.getNodeDepth() );  
   }
}


function buildingEditorGui::onNodeModified( %this, %nodeIdx )
{
   buildingEditorOptionsWindow-->position.setValue( %this.getNodePosition() );    
   buildingEditorOptionsWindow-->rotation.setValue( %this.getNodeNormal() );
   buildingEditorOptionsWindow-->width.setValue( %this.getNodeWidth() ); 
   buildingEditorOptionsWindow-->depth.setValue( %this.getNodeDepth() );   
}

function buildingEditorGui::editNodeDetails( %this )
{
   
   %this.setNodePosition( buildingEditorOptionsWindow-->position.getText() );
   %this.setNodeNormal( buildingEditorOptionsWindow-->rotation.getText() );
   %this.setNodeWidth( buildingEditorOptionsWindow-->width.getText() );
   %this.setNodeDepth( buildingEditorOptionsWindow-->depth.getText() );
}

function buildingEditorGui::onBrowseClicked( %this )
{
   //%filename = RETextureFileCtrl.getText();

   %dlg = new OpenFileDialog()
   {
      Filters        = "All Files (*.*)|*.*|";
      DefaultPath    = buildingEditorGui.lastPath;
      DefaultFile    = %filename;
      ChangePath     = false;
      MustExist      = true;
   };
         
   %ret = %dlg.Execute();
   if(%ret)
   {
      buildingEditorGui.lastPath = filePath( %dlg.FileName );
      %filename = %dlg.FileName;
      buildingEditorGui.setTextureFile( %filename );
      buildingEditorTextureFileCtrl.setText( %filename );
   }
   
   %dlg.delete();
}

function buildingInspector::inspect( %this, %obj )
{
   %name = "";
   if ( isObject( %obj ) )
      %name = %obj.getName();   
   else
      MeshFieldInfoControl.setText( "" );
   
   //RiverInspectorNameEdit.setValue( %name );
   Parent::inspect( %this, %obj );  
}

function buildingInspector::onInspectorFieldModified( %this, %object, %fieldName, %arrayIndex, %oldValue, %newValue )
{
   // Same work to do as for the regular WorldEditor Inspector.
   Inspector::onInspectorFieldModified( %this, %object, %fieldName, %arrayIndex, %oldValue, %newValue );   
}

function buildingInspector::onFieldSelected( %this, %fieldName, %fieldTypeStr, %fieldDoc )
{
   MeshFieldInfoControl.setText( "<font:ArialBold:14>" @ %fieldName @ "<font:ArialItalic:14> (" @ %fieldTypeStr @ ") " NL "<font:Arial:14>" @ %fieldDoc );
}

function buildingTreeView::onInspect(%this, %obj)
{
   buildingInspector.inspect(%obj);   
}

function buildingTreeView::onSelect(%this, %obj)
{
   buildingEditorGui.road = %obj; 
   buildingInspector.inspect( %obj );
   if(%obj != buildingEditorGui.getSelectedRoom())
   {
      buildingEditorGui.setSelectedRoom( %obj );
   }
}

function buildingEditorGui::prepSelectionMode( %this )
{
   %mode = %this.getMode();
   
   if ( %mode $= "BuildingEditorAddNodeMode"  )
   {
      if ( isObject( %this.getSelectedRoom() ) )
         %this.deleteNode();
   }
   
   %this.setMode( "BuildingEditorSelectMode" );
   ToolsPaletteArray-->buildingEditorSelectMode.setStateOn(1);
}

function buildingEditorGui::on3DMouseUp(%this, %matID, %objID, %point, %norm)
{
   if($dragDropMat !$= "")
   {
       if(%matID)
       {
          archEditorGui::copyMaterials( $dragDropMat.getId(), %matID);

          %matID.flush();
          %matID.reload();
       }

         $dragDropMat = "";
         $dragDropLastMatID = 0;

         dragDropCur(false);
       
         %curPos = Canvas.getCursorPos();
         Canvas.setCursorPos(getWord(%curPos,0), getWord(%curPos,1));
   }
/*
   else if($dragDropObjPath !$= "")
   {
      if(%point !$= "")
      {
      
      }

      $dragDropObjPath = "";
      $dragDropObjID = 0;
      $objOffset = 0;
   }
   else if(%objID)
   {
//      %objID = LocalClientConnection.getGhostID(%objID);
//      %objID = ServerConnection.resolveGhostID(%objID);

      if(%objID.isSelected())
      {
         archRotGizmo.setScale("0 0 0");
         %objID.setSelection(0);
      }
      else
      {
         %box = %objID.getObjectBox();
         %diag = getWord(%box,3) - getWord(%box,0) SPC getWord(%box,4) - getWord(%box,1) SPC "0";
         archRotGizmo.setTransform(%objID.getTransform());
         archRotGizmo.setScale(VectorScale("1 1 1", VectorLen(%diag)));
         %objID.setSelection(1);
      }
   }
*/
}

function buildingEditorGui::on3DMouseDrag(%this, %matID, %objID, %point, %norm)
{
    if($dragDropMat !$= "")
    {
       if(%matID != $dragDropLastMatID)
       {
          if($dragDropLastMatID)
          {
             archEditorGui::copyMaterials( matBkgndCopy.getId(), $dragDropLastMatID );

             $dragDropLastMatID.flush();
             $dragDropLastMatID.reload();
          }

          if(%matID)
          {
             archEditorGui::copyMaterials( %matID, matBkgndCopy.getID() );
             archEditorGui::copyMaterials( $dragDropMat.getID(), %matID );

             %matID.flush();
             %matID.reload();
          }

          $dragDropLastMatID = %matID;
       }
    }
    
/*
    else if($dragDropObjPath !$= "")
    {
       if(%point !$= "" && %norm !$= "")
       {
/////// comment out below
          // The cross product is our axis of rotation (rot axis) to align the
          // object to the surface normal.
          %cross = vectorCross(%norm, "0 0 1");
          
          // The arcSin of the cross product is the angle that we need to rotate
          // the object about the "rot axis" to align it to the surface normal.
          // This angle is only accurate between 0 and 90 degrees.
          %angle = mAsin(clamp(vectorLen(%cross), -1, 1));
          
          // Check the sign of the dot product to see if the angle if is greater
          // than 90 degrees.  If it is, we need to transform the angle to be
          // between 90 and 180 degrees.
          if(vectorDot(%norm, "0 0 1") < 0)
             %angle += 2*($halfPI - %angle);

          // If angle is identically 0 or 180 degrees, it will have a length of
          // zero, so we cannot normalize it.  So jsut set "rot axis" = "+y-axis".
          // Else just normalize the cross product, which is the "rot axis"
          if(vectorLen(%cross) == 0)
              %cross = "0 1 0";
          else
              %cross = VectorNormalize(%cross);
///////

          %rot = %this.calcAngAxis(getWord(%norm,0), getWord(%norm,1), getWord(%norm,2));

          if($dragDropObjID)
          {
             %offsetVec = vectorScale(%norm, $objOffset);
             %point = vectorAdd(%point, %offsetVec);
             
//             $dragDropObjID.setTransform(%point SPC %cross SPC %angle);
             $dragDropObjID.setTransform(%point SPC %rot);
             $dragDropObjID.setScale($dragDropObjID.getScale());
          }
          else
          {
             $dragDropObjID = new TSStatic()
             {
                shapeName = $dragDropObjPath;
//                position = %point;
                parentGroup = MissionGroup;
//                rotation = %cross SPC %angle;
             };
             
             $objOffset = $dragDropObjID.getObjectBox();
             $objOffset = -getWord($objOffset, 2);
             
             %offsetVec = vectorScale(%norm, $objOffset);
             %point = vectorAdd(%point, %offsetVec);
             
//             $dragDropObjID.setTransform(%point SPC %cross SPC %angle);
             $dragDropObjID.setTransform(%point SPC %rot);
             $dragDropObjID.setScale($dragDropObjID.getScale());
          }
       }
    }
*/
}

function buildingEditorGui::addRoomAtEyePos(%this)
{
    /*%eyeVec = %obj.getEyeVector();  
  
    %startPos = %obj.getEyePoint();  
    %endPos = VectorAdd(%startPos, VectorScale(%eyeVec, 2));  
        
    %target = ContainerRayCast(%startPos, %endPos, 2000, %obj);  
    %col = firstWord(%target);  
     
    if(%col == 0)   
        return;  
      
    echo(%col);  */
      //return the ID of what we've hit  
      
   %this.addRoomAtPos("0 0 0");
}

function buildingEditorGui::toggleGridSnap(%this)
{
	%this.toggleGridSnapping();
}

function BuildingEditorGridSnapSizeFld::onReturn(%this)
{
   buildingEditorGui.setGridSnapSize(%this.getText());
}
//------------------------------------------------------------------------------
function EbuildingEditorSelectModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorAddModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorMoveModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorRotateModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorScaleModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorInsertModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function EbuildingEditorRemoveModeBtn::onClick(%this)
{
   EditorGuiStatusBar.setInfo(%this.ToolTip);
}

function buildingDefaultWidthSliderCtrlContainer::onWake(%this)
{
   buildingDefaultWidthSliderCtrlContainer-->slider.setValue(buildingDefaultWidthTextEditContainer-->textEdit.getText());
}

function buildingDefaultDepthSliderCtrlContainer::onWake(%this)
{
   buildingDefaultDepthSliderCtrlContainer-->slider.setValue(buildingDefaultDepthTextEditContainer-->textEdit.getText());
}
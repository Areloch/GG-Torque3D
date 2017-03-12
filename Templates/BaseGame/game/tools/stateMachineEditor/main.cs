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
singleton GuiControlProfile( StateMachineEditorProfile )
{
   canKeyFocus = true;
   opaque = true;
   fillColor = "192 192 192 192";
   category = "Editor";
};

function initializeStateMachineEditor()
{
   echo(" % - Initializing State Machine Editor");  
   
   new ScriptObject( StateMachineEditorPlugin )
   {
      superClass = "EditorPlugin";
   };
   
   exec("./interface/textField.cs");
   exec("./interface/nodeNameField.cs");
   exec("./interface/stateFuncField.cs");
   exec("./interface/smField.cs");
   exec("./interface/conditionField.cs");
   
   exec("./scripts/stateMachineEditor.ed.cs");   
   exec("./gui/stateMachineDlg.ed.gui");
   
   %map = new ActionMap();      
   %map.bindCmd( keyboard, "delete", "StateMachineGraph.deleteSelected();", "" );  
   
   StateMachineEditorPlugin.map = %map;
   
   StateMachineGraph.transitions = new ArrayObject();
   StateMachineGraph.stateVarsArray = new ArrayObject();
   StateMachineInspector.clear();
}

function StateMachineEditorPlugin::onWorldEditorStartup( %this )
{ 
   // Add ourselves to the toolbar.
   //StateMachineEditor.addToolbarButton();
}

function StateMachineEditor::addToolbarButton(%this)
{
	%filename = expandFilename("tools/gui/images/iconOpen");
	%button = new GuiBitmapButtonCtrl() {
		canSaveDynamicFields = "0";
		internalName = AssetBrowserBtn;
		Enabled = "1";
		isContainer = "0";
		Profile = "ToolsGuiButtonProfile";
		HorizSizing = "right";
		VertSizing = "bottom";
		position = "180 0";
		Extent = "25 19";
		MinExtent = "8 2";
		canSave = "1";
		Visible = "1";
		Command = "Canvas.pushDialog(StateMachineEditor);";
		tooltipprofile = "ToolsGuiToolTipProfile";
		ToolTip = "Asset Browser";
		hovertime = "750";
		bitmap = %filename;
		bitmapMode = "Centered";
		buttonType = "PushButton";
		groupNum = "0";
		useMouseEvents = "0";
	};
	ToolsToolbarArray.add(%button);
	EWToolsToolbar.setExtent((25 + 8) * (ToolsToolbarArray.getCount()) + 12 SPC "33");
}

function StateMachine::deleteSelected(%this)
{
   
}

function StateMachine::renameNode(%this)
{
   
}

function StateMachineGraph::onRightMouseDown(%this, %mousePoint, %nodeIdx)
{
    %this.lastRMBClickPos = %mousePoint;
    
    %this.lastRMBClickNode = %nodeIdx;

    %popup = StateMachinePopup;      
    if( !isObject( %popup ) )
    {
        %popup = new PopupMenu( StateMachinePopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            item[ 0 ] = "New State" TAB "" TAB %this @ ".addState();";
            item[ 1 ] = "New Trasition" TAB "" TAB %this @ ".createTransition();";
            item[ 2 ] = "Set as Root" TAB "" TAB %this @ ".setStateAsRoot();";
            item[ 3 ] = "Edit State's Callback Script" TAB "" TAB %this @ ".openStateScript();";
            item[ 4 ] = "--" TAB "" TAB "";
            item[ 5 ] = "Recenter Graph" TAB "" TAB %this @ ".recenterGraph();";
            
            object = -1;
        };
    }
    
    %nodeContextEnabled = %nodeIdx == -1 ? false : true;
    
    %popup.enableItem( 1, %nodeContextEnabled );
    %popup.enableItem( 2, %nodeContextEnabled );
           
    %popup.showPopup( Canvas );
}

function StateMachineGraph::addState(%this, %stateName, %statePos)
{
   if(%stateName $= "")
      %stateName = "NewState";
      
   %newNode = %this.addNode(%stateName);
   
   if(%statePos $= "")
      %statePos = %this.lastRMBClickPos.x - %this.getGraphCenter().x SPC %this.lastRMBClickPos.y - %this.getGraphCenter().y;
   
   %this.setNodePosition(%newNode, %statePos);
   
   %this.setNodeField(%newNode, "scriptFunction", "on"@%stateName);
   
   //it's our first node!
   if(%newNode == 0)
   {
      %this.rootNode = %newNode; 
      %this.setNodeColor(%newNode, "0 180 0"); 
      %this.useNodeColor(%newNode, true); 
   }
   
   //sanity check the node!
   for(%i=0; %i < %this.getNodeCount(); %i++)
   {
      if(%i == %newNode)
         continue;
         
      if(%stateName $= %this.getNodeName(%i))
      {
         %this.setNodeError(%newNode, true); 
         %this.setNodeError(%i, true); 
      }
      else
      { 
         %this.setNodeError(%i, false); 
      }
   }
   
   //%defaultPage-->Graph.addSocketToNode(%ohNoNode, "0 0");
}

function StateMachineGraph::setStateAsRoot(%this)
{
   if(%this.rootNode !$= "")
   {
      %this.useNodeColor(%this.rootNode, false);
   }
   
   %this.setNodeColor(%this.lastRMBClickNode, "0 180 0");
   %this.useNodeColor(%this.lastRMBClickNode, true);
   
   %this.rootNode = %this.lastRMBClickNode; 
}

function StateMachineGraph::createTransition(%this)
{
   %this.createConnection(%this.lastRMBClickNode);
}

function StateMachineGraph::openStateScript(%this)
{
   //%this.lastRMBClickNode
}

function StateMachineGraph::onNewConnection(%this, %connectionIdx)
{
     //create a new array object for the various transition rules
   %transitionArray = new ArrayObject();
   %this.transitions.add(%connectionIdx, %transitionArray);
   
   %this.setConnectionError(%connectionIdx);
}

function StateMachineGraph::onNodeSelected(%this, %nodeIdx)
{
   StateMachineInspector.clear();
   
   %group = new GuiInspectorGroup()
   {
      Caption = "Node Properties";
   }; 
   StateMachineInspector.add(%group);
   
   StateMachineGraph.selectedNode = %nodeIdx;
   
   %field = StateMachineInspector.createNodeNameField(%group, "State Name");
   %field-->Field.setText(StateMachineGraph.getNodeName(StateMachineGraph.selectedNode));
   
   %field = StateMachineInspector.createStateFunctionField(%group, "Callback Name");
   
   %callbackName = StateMachineGraph.getNodeField(StateMachineGraph.selectedNode, "scriptFunction");
   if(%callbackName $= "")
      %callbackName = "on" @ StateMachineGraph.getNodeName(StateMachineGraph.selectedNode);
      
   %field-->Field.setText(%callbackName);
   
   %stack = %group.getObject(0);
   %ctrl = new GuiControl()
   {
      position = "0 0";
      extent = %stack.extent.x SPC 20;
      
      new GuiBitmapButtonCtrl()
      {
         bitmap = "tools/gui/images/iconAdd";
         position = %stack.extent.x - 20 SPC 0;
         extent = "20 20";
         command = StateMachineInspector @ ".createNodeField(" @ %group @ ", \"\");";
      };
   };
   %stack.add(%ctrl);
   
   StateMachineGraph.selectedNode = %nodeIdx;
}

function StateMachineGraph::onNodeUnselected(%this)
{
   StateMachineInspector.clear();
   
   StateMachineGraph.selectedNode = -1;
}

function StateMachineGraph::onConnectionSelected(%this, %connectionIdx)
{
   StateMachineInspector.clear();
   
   %group = new GuiInspectorGroup()
   {
      Caption = "Transition Properties";
   }; 
   StateMachineInspector.add(%group);
   
   %SMPropStack = %group.getObject(0);
   
   %ctrl = new GuiControl()
   {
      position = "0 0";
      extent = %SMPropStack.extent.x SPC 20;
      
      new GuiBitmapButtonCtrl()
      {
         bitmap = "tools/gui/images/iconAdd";
         position = %SMPropStack.extent.x - 20 SPC 0;
         extent = "20 20";
         command = StateMachineInspector @ ".createConditionField("@%group@", \"\");";
      };
   };
   
   %SMPropStack.add(%ctrl); 
   
   //now, iterate through our rules
   %transitionIdx = StateMachineGraph.transitions.getIndexFromKey(%connectionIdx);
   
   %ruleList = StateMachineGraph.transitions.getValue(%transitionIdx);
   
   %count = %ruleList.count();
   for(%i=0; %i < %count; %i++)
   {
      %rule = %ruleList.getValue(%i);
      
      %field = StateMachineInspector.createConditionField(%group,"");
      
      %fieldName = StateMachineGraph.getSMFieldName(getField(%rule, 0));
      
      %field-->field.setText(%fieldName);
      
      %comparitor = getField(%rule, 1);
      //Translate the comparitor
      if(%comparitor $= "GreaterOrEqual")
         %comparitor = ">=";
      else if(%comparitor $= "GreaterThan")
         %comparitor = ">";
      else if(%comparitor $= "Equals") 
         %comparitor = "==";
      else if(%comparitor $= "LessThan")
         %comparitor = "<";
      else if(%comparitor $= "LessOrEqual")
         %comparitor = "<=";
      else if(%comparitor $= "DoesNotEqual")
         %comparitor = "!=";
      
      %field-->comparitor.setText(%comparitor);
      %field-->value.setText(getField(%rule, 2));
   }
   
   StateMachineGraph.selectedConnection = %connectionIdx;
}

function StateMachineGraph::onConnectionUnselected(%this)
{
   StateMachineInspector.clear();
   
   StateMachineGraph.selectedConnection = -1;
}

function StateMachineGraph::getSMFieldName(%this, %index)
{
   %count = %this.stateVarsArray.count();
   
   if(%index < %count && %index >= 0)
      return %this.stateVarsArray.getKey(%index);
      
   return "";
}

function StateMachineInspector::clear(%this)
{
   if(!isObject(%this-->graphGroup))
   {
      %group = new GuiInspectorGroup(StateMachinePropertiesGroup)
      {
         Caption = "State Machine Properties";
         internalName = "graphGroup";
      }; 
      StateMachineInspector.add(%group);  
      
      %SMPropStack = %group.getObject(0);
      
      //add the State Machine fields thing here
      %ctrl = new GuiControl()
      {
         position = "0 0";
         extent = %SMPropStack.extent.x SPC 20;
         
         new GuiBitmapButtonCtrl()
         {
            bitmap = "tools/gui/images/iconAdd";
            position = %SMPropStack.extent.x - 20 SPC 0;
            extent = "20 20";
            command = %this @ ".createSMField(StateMachinePropertiesGroup, \"\");";
         };
      };
      
      %SMPropStack.add(%ctrl);      
   }
   
   for(%i=1; %i < StateMachineInspector.getCount(); %i++)
   {
      %obj = StateMachineInspector.getObject(%i);
      StateMachineInspector.remove(%obj);
      %obj.delete();
   }
}

function StateMachinePropertiesGroup::clear(%this)
{
   %count = %this.getObject(0).getCount();
   
   for(%i=1; %i < %count; %i++)
   {
      %obj = %this.getObject(0).getObject(%i);
      %this.getObject(0).remove(%obj);  
   }
}

function SaveStateMachineBtn::onClick(%this)
{
   /*%dlg = new SaveFileDialog()
   {
      Filters        = "ShaderGraph Files (*.xml)|*.xml|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt   = true;
   };

   %ret = %dlg.Execute();
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %exportFile = %dlg.FileName;
   }

   if( fileExt( %exportFile ) !$= ".xml" )
      %exportFile = %exportFile @ ".xml";*/
      
   if(StateMachineEditor.loadedFile $= "")
      return;
      
   %xmlDoc = new SimXMLDocument();
   
   %xmlDoc.pushNewElement("StateMachine");
   
      %xmlDoc.pushNewElement("Fields");
      
      %stateFieldCount = StateMachineGraph.stateVarsArray.count();
      
      for(%f=0; %f<%stateFieldCount; %f++)
      {
         %fieldName = StateMachineGraph.stateVarsArray.getKey(%f);
         %fieldDefaultValue = StateMachineGraph.stateVarsArray.getValue(%f);
         
         if(%fieldName $= "stateTime")
         {
            //this is an innate field of state machines and doesn't get saved to file, to avoid issues
            continue;  
         }
         
         %xmlDoc.pushNewElement("Field");
         %xmlDoc.setAttribute("name", %fieldName);
         %xmlDoc.setAttribute("defaultValue", %fieldDefaultValue);
         %xmlDoc.popElement();
      }
      
      %xmlDoc.popElement();
   
      %xmlDoc.pushNewElement("States");
      
      %nodeCount = StateMachineGraph.getNodeCount();
      
      for(%n=0; %n<%nodeCount; %n++)
      {
         %nodeName = StateMachineGraph.getNodeName(%n);
         %nodePos = StateMachineGraph.getNodePosition(%n);
         %nodeCallback = StateMachineGraph.getNodeField(%n, "scriptFunction");
         
         %xmlDoc.pushNewElement("State");
         %xmlDoc.setAttribute("name", %nodeName);
         %xmlDoc.setAttribute("scriptFunction", %nodeCallback);
         %xmlDoc.setAttribute("pos", %nodePos);
         
         //
         %xmlDoc.popElement();
      }
      
      %xmlDoc.popElement();
   
      %xmlDoc.pushNewElement("Transitions");
      
      %conCount = StateMachineGraph.getConnectionCount();
      for(%c=0; %c<%conCount; %c++)
      {
         %connStart = StateMachineGraph.getConnectionOwnerNode(%c);
         %connEnd = StateMachineGraph.getConnectionEndNode(%c);
         
         %xmlDoc.pushNewElement("Transition");
         %xmlDoc.setAttribute("ownerState", %connStart);
         %xmlDoc.setAttribute("stateTarget", %connEnd);
         
         %transitionIdx = StateMachineGraph.transitions.getIndexFromKey(%c);
         %ruleList = StateMachineGraph.transitions.getValue(%transitionIdx);
         
         %ruleCount = %ruleList.count();
         for(%r=0; %r < %ruleCount; %r++)
         {
            %rule = %ruleList.getValue(%r);
            
            %xmlDoc.pushNewElement("Rule");
            %xmlDoc.setAttribute("fieldId", getField(%rule, 0));
            %xmlDoc.setAttribute("comparitor", getField(%rule, 1));
            %xmlDoc.setAttribute("value", getField(%rule, 2));
            
            %xmlDoc.popElement();
         }
      
         %xmlDoc.popElement();
      }
      
      %xmlDoc.popElement();
   
   %xmlDoc.popElement();
   
   %xmlDoc.saveFile(StateMachineEditor.loadedFile);
   
   if(StateMachineGraph.loadedAsset !$= "")
   {
      StateMachineGraph.loadedAsset.notifyAssetChanged();
   }
}

function LoadStateMachineBtn::onClick(%this)
{
   %dlg = new OpenFileDialog()
   {
      Filters        = "ShaderGraph Files (*.xml)|*.xml|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt   = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %fullPath = makeRelativePath( %dlg.FileName, getMainDotCSDir() );
      %file = fileBase( %fullPath );
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;
      
   StateMachineEditor.loadStateMachineFile(%fullPath);
}

function StateMachineEditor::loadStateMachineAsset(%this, %asset)
{
   StateMachineGraph.loadedAsset = AssetDatabase.acquireAsset(%asset);
   %file = StateMachineGraph.loadedAsset.stateMachineFile;
   %this.loadStateMachineFile(%file);
}

function StateMachineEditor::loadStateMachineFile(%this, %file)
{
   StateMachineGraph.clear();
   StateMachineGraph.transitions.empty();
   StateMachineGraph.stateVarsArray.empty();
   
   StateMachinePropertiesGroup.clear();
      
   //we had a real file, time to parse it and load our shader graph
   %xmlDoc = new SimXMLDocument();
   
   if(isFile(%file))
      StateMachineEditor.loadedFile = %file;
      
   if(%xmlDoc.loadFile(%file))
   {
      //StateMachine element
      %xmlDoc.pushChildElement(0);
      
      //We *always* have a state timeout field
      StateMachineGraph.stateVarsArray.add("stateTime", "0");
      %field = StateMachineInspector.createSMField(StateMachinePropertiesGroup, "stateTime", "0");
      %field.getObject(0).active = false;
      %field.getObject(1).active = false;  
      
      //disable and hide the remove button          
      %field.getObject(2).active = false;
      %field.getObject(2).hidden = true;
      
      //Fields
      if(%xmlDoc.pushFirstChildElement("Fields"))
      {
         %fieldCount = 0;
         while(%xmlDoc.pushChildElement(%fieldCount))
         {
            %fieldName = %xmlDoc.attribute("name");
            %fieldDefaultVal = %xmlDoc.attribute("defaultValue");
            
            %field = StateMachineInspector.createSMField(StateMachinePropertiesGroup, %fieldName, %fieldDefaultVal);
            
            StateMachineGraph.stateVarsArray.add(%fieldName, %fieldDefaultVal);
            
            %xmlDoc.popElement();
            %fieldCount++;
         }
         
         %xmlDoc.popElement();
      }
      
      //States
      if(%xmlDoc.pushFirstChildElement("States"))
      {
         %stateCount = 0;
         while(%xmlDoc.pushChildElement(%stateCount))
         {
            %stateName = %xmlDoc.attribute("name");
            %scriptFunc = %xmlDoc.attribute("scriptFunction");
            %statePos = %xmlDoc.attribute("pos");
            
            StateMachineGraph.addState(%stateName, %statePos);
            
            %isStarting = %xmlDoc.attribute("starting");
            if(%isStarting !$= "")
            {
               //set it as starting state! 
               if(StateMachineGraph.rootNode !$= "")
               {
                  StateMachineGraph.useNodeColor(StateMachineGraph.rootNode, false);
               }
               
               StateMachineGraph.setNodeColor(%stateCount, "0 180 0");
               StateMachineGraph.useNodeColor(%stateCount, true);
               StateMachineGraph.rootNode = %stateCount;
            }
            
            StateMachineGraph.setNodeField(%stateCount, "scriptFunction", %scriptFunc);
            
            %xmlDoc.popElement();
            %stateCount++;         
         }
         %xmlDoc.popElement();
      }
      
      if(%xmlDoc.pushFirstChildElement("Transitions"))
      {      
         %transitionCount = 0;
         while(%xmlDoc.pushChildElement(%transitionCount))
         {
            %ownerState = %xmlDoc.attribute("ownerState");
            %stateTarget = %xmlDoc.attribute("stateTarget");
            
            %connectionIdx = StateMachineGraph.addConnection(%ownerState, %stateTarget);
            
            %ruleList = StateMachineGraph.transitions.getValue(%connectionIdx);
            
            %ruleCount = 0;
            while(%xmlDoc.pushChildElement(%ruleCount))
            {
               %fieldId = %xmlDoc.attribute("fieldId");
               %comparitor = %xmlDoc.attribute("comparitor");
               %value = %xmlDoc.attribute("value");
               
               %ruleList.add(%ruleCount, %fieldId TAB %comparitor TAB %value);
               
               StateMachineGraph.setConnectionError(%connectionIdx, false);
               
               %xmlDoc.popElement();
               %ruleCount++;
            }

            %xmlDoc.popElement();
            %transitionCount++;
         }
         
         %xmlDoc.popElement();
      }
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.delete();
}
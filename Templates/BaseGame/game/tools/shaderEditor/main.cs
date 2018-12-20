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
function initializeShaderEditor()
{
   echo(" % - Initializing Shader Editor");  
   
   new ScriptObject( ShaderEditorPlugin )
   {
      superClass = "EditorPlugin";
   };
   
   exec("./interface/textField.cs");
   exec("./interface/nodeNameField.cs");
   exec("./interface/smField.cs");
   exec("./interface/conditionField.cs");
   
   exec("./gui/shaderEditor.ed.gui");
   exec("./scripts/shaderEditor.ed.cs");
   
   $CurrentShaderGraph = ShaderEditorGraph;
   
   %map = new ActionMap();      
   %map.bindCmd( keyboard, "delete", "ShaderEditorGraph.deleteSelected();", "" );  
   
   ShaderEditorPlugin.map = %map;
   
   ShaderEditorGraph.transitions = new ArrayObject();
   ShaderEditorGraph.stateVarsArray = new ArrayObject();
}

function ShaderEditorPlugin::onWorldEditorStartup( %this )
{ 
   // Add ourselves to the toolbar.
   //ShaderEditor.addToolbarButton();
}

function ShaderEditor::addToolbarButton(%this)
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
		Command = "Canvas.pushDialog(ShaderEditor);";
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

function ShaderEditorGraph::onRightMouseDown(%this, %mousePoint, %nodeIdx)
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
            
            item[ 0 ] = "New Node" TAB "" TAB %this @ ".addState();";
            item[ 1 ] = "New Trasition" TAB "" TAB %this @ ".createTransition();";
            item[ 2 ] = "Rename Node" TAB "" TAB %this @ ".renameNode();";
            item[ 3 ] = "Set as Root" TAB "" TAB %this @ ".setStateAsRoot();";
            
            object = -1;
        };
        
        /*%nodeListPopup = new PopupMenu( NodeListPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            item[ 0 ] = "2D Texture" TAB "" TAB %this @ ".addTextureNode();";
            item[ 1 ] = "Float" TAB "" TAB %this @ ".addFloatNode();";
            item[ 2 ] = "- " TAB "" TAB "";
            item[ 3 ] = "Multiply" TAB "" TAB %this @ ".addMulNode();";
            item[ 4 ] = "- " TAB "" TAB "";
            item[ 5 ] = "PBR Material" TAB "" TAB %this @ ".addPBRMatEndNode();";
            
            object = -1;
        };*/
        
        //%popup.insertSubmenu(0, "New Node", %nodeListPopup);
        //%popup.insertItem(3, "New Folder", "", %this @ ".addNewFolder();");
    }
    
    %nodeContextEnabled = %nodeIdx == -1 ? false : true;
    
    %popup.enableItem( 1, %nodeContextEnabled );
    %popup.enableItem( 2, %nodeContextEnabled );
    %popup.enableItem( 3, %nodeContextEnabled );
           
    %popup.showPopup( Canvas );
}

function ShaderEditorGraph::addState(%this)
{
   %newNode = %this.addNode("NewState");
   
   %this.setNodePosition(%newNode, %this.lastRMBClickPos);
   
   //it's our first node!
   if(%newNode == 0)
   {
      %this.rootNode = %newNode; 
      %this.setNodeColor(%newNode, "0 180 0"); 
   }
   
   //%defaultPage-->Graph.addSocketToNode(%ohNoNode, "0 0");
}

function ShaderEditorGraph::setStateAsRoot(%this)
{
   %this.setNodeColor(%this.lastRMBClickNode, "0 180 0");
   %this.rootNode = %this.lastRMBClickNode; 
}

function ShaderEditorGraph::createTransition(%this)
{
   %this.createConnection(%this.lastRMBClickNode);
}

function ShaderEditorGraph::onNewConnection(%this, %connectionIdx)
{
     //create a new array object for the various transition rules
   %transitionArray = new ArrayObject();
   %this.transitions.add(%connectionIdx, %transitionArray);
}

function ShaderEditorGraph::onNodeSelected(%this, %nodeIdx)
{
   StateMachineInspector.clear();
   
   %group = new GuiInspectorGroup()
   {
      Caption = "Node Properties";
   }; 
   StateMachineInspector.add(%group);
   
   ShaderEditorGraph.selectedNode = %nodeIdx;
   
   %field = StateMachineInspector.createNodeNameField(%group, "State Name");
   %field-->Field.setText(ShaderEditorGraph.getNodeName(ShaderEditorGraph.selectedNode));
   
   ShaderEditorGraph.selectedNode = %nodeIdx;
}

function ShaderEditorGraph::onNodeUnselected(%this)
{
   StateMachineInspector.clear();
   
   ShaderEditorGraph.selectedNode = -1;
}

function ShaderEditorGraph::onConnectionSelected(%this, %connectionIdx)
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
   %transitionIdx = ShaderEditorGraph.transitions.getIndexFromKey(%connectionIdx);
   
   %ruleList = ShaderEditorGraph.transitions.getValue(%transitionIdx);
   
   %count = %ruleList.count();
   for(%i=0; %i < %count; %i++)
   {
      %rule = %ruleList.getValue(%i);
      
      %field = StateMachineInspector.createConditionField(%group,"");
      %field-->field.setText(getField(%rule, 0));
      %field-->comparitor.setText(getField(%rule, 1));
      %field-->value.setText(getField(%rule, 2));
   }
   
   ShaderEditorGraph.selectedConnection = %connectionIdx;
}

function ShaderEditorGraph::onConnectionUnselected(%this)
{
   StateMachineInspector.clear();
   
   ShaderEditorGraph.selectedConnection = -1;
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

function SaveStateMachineBtn::onClick(%this)
{
   %dlg = new SaveFileDialog()
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
      %exportFile = %exportFile @ ".xml";
      
   %xmlDoc = new SimXMLDocument();
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
      
   //we had a real file, time to parse it and load our shader graph
   %xmlDoc = new SimXMLDocument();
   
   if(%xmlDoc.loadFile(%fullPath))
   {
      %this.pushChildElement(0);
      
      while(true)
      {
         //get our name
         %this.pushChildElement(0);
         %name = %this.getData();
         
         %this.nextSiblingElement("TAMLPath");
         %tamlPath = %this.getData();
         
         %this.nextSiblingElement("ScriptPath");
         %scriptPath = %this.getData();
         %this.popElement();
         
         %this.pushBack(%name, %tamlPath, %scriptPath);
         
         if (!%this.nextSiblingElement("GameObject")) 
            break;
      }
   }
   
   %xmlDoc.delete();
}
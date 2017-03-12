//
//
//
//

function ShaderEditorGraph::onDoubleClickNode(%this, %nodeIdx)
{
    %defaultPage = ShaderGen.addWorkspacePage();
    %defaultPage.setText("New Page");    
}

function ShaderEditorGraph::onRightMouseDown(%this, %mousePoint)
{
    %this.lastRMBClickPos = %mousePoint;

    %popup = ShaderGenPopup;      
    if( !isObject( %popup ) )
    {
        %popup = new PopupMenu( ShaderGenPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            //item[ 0 ] = "New Node" TAB "" TAB %this @ ".addNewNode();";
            //item[ 1 ] = "New Folder" TAB "" TAB %this @ ".addNewFolder();";
            //item[ 2 ] = "Inspect" TAB "" TAB "inspectObject( %this.object );";
            
            object = -1;
        };
        
        %inNodeListPopup = new PopupMenu( InNodeListPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            item[ 0 ] = "2D Texture" TAB "" TAB %this @ ".addTextureNode();";
            item[ 1 ] = "Float" TAB "" TAB %this @ ".addFloatNode();";
            
            object = -1;
        };
        
        %outNodeListPopup = new PopupMenu( OutNodeListPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            item[ 0 ] = "PBR Material" TAB "" TAB %this @ ".addPBRMatEndNode();";
            
            object = -1;
        };
        
        %functionNodeListPopup = new PopupMenu( FunctionNodeListPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            item[ 0 ] = "Multiply" TAB "" TAB %this @ ".addMulNode();";
            
            object = -1;
        };
        
        %nodeListPopup = new PopupMenu( NodeListPopup )
        {
            superClass = "MenuBuilder";
            isPopup = "1";
            
            object = -1;
        };
        
        %nodeListPopup.insertSubmenu(0, "Add Input Node", %inNodeListPopup);
        %nodeListPopup.insertSubmenu(1, "Add Function Node", %functionNodeListPopup);
        %nodeListPopup.insertSubmenu(2, "Add Output Node", %outNodeListPopup);
        
        %popup.insertSubmenu(0, "Add Node", %nodeListPopup);
        %popup.insertItem(1, "--", "", "");
        %popup.insertItem(2, "New Folder", "", %this @ ".addNewFolder();");
        %popup.insertItem(3, "--", "", "");
        %popup.insertItem(4, "Recenter Graph", "", %this @ ".recenterGraph();");
    }
    
    //%popup.enableItem( 7, false );
            
    %popup.showPopup( Canvas );
}

function ShaderEditorGraph::addNewFolder(%this)
{
   %newFolder = %this.addFolder("Folder");
   
   %this.setFolderPosition(%newFolder, %this.lastRMBClickPos);
   
   //%defaultPage-->Graph.addSocketToNode(%ohNoNode, "0 0");
}

function ShaderEditorGraph::loadGraph(%this, %file)
{
   ShaderGen::loadGraph(%file);
	
	//Now, add nodes and populate the graph
	for(%i = 0; %i < $ShaderGen.nodeCount; %i++)
	{
	   %type = getField($ShaderGen.NODES[%i], 0);
	   %pos = getField($ShaderGen.NODES[%i], 3);
	   
	   %pos.x = mRound(%this.position.x + %this.extent.x / 2 + %pos.x);
	   %pos.y = mRound(%this.position.y + %this.extent.y / 2 + %pos.y);
	   
	   if(%type $= "Texture2DNode")
	   {
	      %newNode = %this.addTextureNode();
	      %this.setNodePosition(%i, %pos);
	   }
	   else if(%type $= "floatNode")
	   {
	      %newNode = %this.addFloatNode();
	      %this.setNodePosition(%i, %pos);
	   }
	   else if(%type $= "DeferredMaterialNode")
	   {
	      %newNode = %this.addPBRMatEndNode();
	      %this.setNodePosition(%i, %pos);
	      $ShaderGen.endNode = %i;
	   }
	}
	
	for(%i = 0; %i < $ShaderGen.connCount; %i++)
	{
	   %startNode = getField($ShaderGen.CONNECTIONS[%i], 0);
	   %endNode = getField($ShaderGen.CONNECTIONS[%i], 1);
	   %startSocket = getField($ShaderGen.CONNECTIONS[%i], 2);
	   %endSocket = getField($ShaderGen.CONNECTIONS[%i], 3);
	   
	   %this.addConnection(%startNode, %startSocket, %endNode, %endSocket);
	}
}

function CompileShaderGraphBtn::onClick(%this)
{
   //time to compile!
   ShaderGen::compileShader($ShaderGen::targetShaderFile);
}

//
//
function ShaderEditorGraph::addFloatNode(%this)
{
   $CurrentShaderGraph = %this;
   
   %node = $CurrentShaderGraph.addNode("Float");
   
   %node.addSocketToNode(1, "");
   
   if(%nodePos $= "")
      %nodePos = $CurrentShaderGraph.lastRMBClickPos.x - %this.getGraphCenter().x SPC $CurrentShaderGraph.lastRMBClickPos.y - %this.getGraphCenter().y;
   
   $CurrentShaderGraph.setNodePosition(%node.nodeIndex, %nodePos);
   
   $ShaderGen.UNIFORM[$ShaderGen.uniformCount] = "float" TAB "" TAB $ShaderGen.uniformCount;
   $ShaderGen.uniformCount++;
   
   $ShaderGen.NODES[$ShaderGen.nodeCount] = "floatNode" TAB "" TAB %node.nodeIndex TAB $CurrentShaderGraph.lastRMBClickPos;
   $ShaderGen.nodeCount++;
}

function ShaderEditorGraph::addMulNode(%this)
{
   $CurrentShaderGraph = %this;
   
   %node = $CurrentShaderGraph.addNode("Multiply");
   
   %node.addSocketToNode(0, "A");   
   %node.addSocketToNode(0, "B");
   %node.addSocketToNode(1, "");
   
   if(%nodePos $= "")
      %nodePos = $CurrentShaderGraph.lastRMBClickPos.x - %this.getGraphCenter().x SPC $CurrentShaderGraph.lastRMBClickPos.y - %this.getGraphCenter().y;
   
   $CurrentShaderGraph.setNodePosition(%node.nodeIndex, %nodePos);
   
   $ShaderGen.NODES[$ShaderGen.nodeCount] = "mulNode" TAB "" TAB %node.nodeIndex TAB $CurrentShaderGraph.lastRMBClickPos;
   $ShaderGen.nodeCount++;
}

function ShaderEditorGraph::addTextureNode(%this)
{
   $CurrentShaderGraph = %this;
   
   %node = $CurrentShaderGraph.addNode("Texture");
   
   %node.addSocketToNode(0, "UVs");
   
   %node.addSocketToNode(1, "RGBA");
   %node.addSocketToNode(1, "Red");
   %node.addSocketToNode(1, "Green");
   %node.addSocketToNode(1, "Blue");
   %node.addSocketToNode(1, "Alpha");
   
   %node.image = "blar.jpg";
   
   %node.code = "float4 texSample = TORQUE_SAMPLE2D(tex, uv);\n";
   
   if(%nodePos $= "")
      %nodePos = $CurrentShaderGraph.lastRMBClickPos.x - %this.getGraphCenter().x SPC $CurrentShaderGraph.lastRMBClickPos.y - %this.getGraphCenter().y;
   
   $CurrentShaderGraph.setNodePosition(%node.nodeIndex, %nodePos);
   
   $ShaderGen.UNIFORM[$ShaderGen.uniformCount] = "sampler2D" TAB "" TAB $ShaderGen.uniformCount;
   $ShaderGen.uniformCount++;
   
   $ShaderGen.NODES[$ShaderGen.nodeCount] = "Texture2DNode" TAB "" TAB %node.nodeIndex TAB $CurrentShaderGraph.lastRMBClickPos;
   $ShaderGen.nodeCount++;
}

function ShaderEditorGraph::addPBRMatEndNode(%this)
{
   $CurrentShaderGraph = %this;
   
   %node = $CurrentShaderGraph.addNode("PBRMaterial");
   
   %node.addSocketToNode(0, "Albedo");
   %node.addSocketToNode(0, "Normal");
   %node.addSocketToNode(0, "Metalness");
   %node.addSocketToNode(0, "Roughness");
   %node.addSocketToNode(0, "Ambient Occlusion");
   %node.addSocketToNode(0, "Composite");
   
   %node.codeFile = "./PBRMatNodeCode.txt";
      
   if(%nodePos $= "")
      %nodePos = $CurrentShaderGraph.lastRMBClickPos.x - %this.getGraphCenter().x SPC $CurrentShaderGraph.lastRMBClickPos.y - %this.getGraphCenter().y;
   
   $CurrentShaderGraph.setNodePosition(%node.nodeIndex, %nodePos);
   
   $CurrentShaderGraph.endNode = %node;
   
   //Vert input stuff?
   //
   $ShaderGen.IN[$ShaderGen.inCount] = "float2" TAB "texCoord" TAB "TEXCOORD0";
   $ShaderGen.inCount++;
   $ShaderGen.IN[$ShaderGen.inCount] = "float4" TAB "screenspacePos" TAB "TEXCOORD1";
   $ShaderGen.inCount++;
   $ShaderGen.IN[$ShaderGen.inCount] = "float2" TAB "vpos" TAB "VPOS";
   $ShaderGen.inCount++;
   //
   
   //Output channels
   $ShaderGen.OUT[$ShaderGen.outCount] = "float4" TAB "color" TAB "COLOR0";
   $ShaderGen.outCount++;
   $ShaderGen.OUT[$ShaderGen.outCount] = "float4" TAB "normal" TAB "COLOR1";
   $ShaderGen.outCount++;
   
   $ShaderGen.NODES[$ShaderGen.nodeCount] = "DeferredMaterialNode" TAB "" TAB %node.nodeIndex TAB $CurrentShaderGraph.lastRMBClickPos;
   $ShaderGen.nodeCount++;
}

//save/load
function SaveShaderGraphBtn::onClick(%this)
{
   /*%dlg = new SaveFileDialog()
   {
      Filters        = "ShaderGraph Files (*.sgf)|*.sgf|";
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

   if( fileExt( %exportFile ) !$= ".sgf" )
      %exportFile = %exportFile @ ".sgf";*/
      
   %xmlDoc = new SimXMLDocument();
   
   %xmlDoc.pushNewElement("ShaderGraph");
   
   %xmlDoc.pushNewElement("PixelShader");
   
   %xmlDoc.pushNewElement("In");
   
   for(%i=0; %i < $ShaderGen.inCount; %i++)
   {
      %type = getField($ShaderGen.IN[%i],0);
      %name = getField($ShaderGen.IN[%i],1);      
      %target = getField($ShaderGen.IN[%i],2);
      
      %xmlDoc.pushNewElement(%type);
      %xmlDoc.setAttribute("name", %name);
      %xmlDoc.setAttribute("target", %target);
      %xmlDoc.popElement();
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.pushNewElement("Out");
   
   for(%i=0; %i < $ShaderGen.outCount; %i++)
   {
      %type = getField($ShaderGen.OUT[%i],0);
      %name = getField($ShaderGen.OUT[%i],1);      
      %target = getField($ShaderGen.OUT[%i],2);
      
      %xmlDoc.pushNewElement(%type);
      %xmlDoc.setAttribute("name", %name);
      %xmlDoc.setAttribute("target", %target);
      %xmlDoc.popElement();
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.pushNewElement("Uniforms");
   
   for(%i=0; %i < $ShaderGen.uniformCount; %i++)
   {
      %type = getField($ShaderGen.UNIFORM[%i],0);
      %name = getField($ShaderGen.UNIFORM[%i],1);      
      %register = getField($ShaderGen.UNIFORM[%i],2);
      
      %xmlDoc.pushNewElement(%type);
      %xmlDoc.setAttribute("name", %name);
      %xmlDoc.setAttribute("register", %register);
      %xmlDoc.popElement();
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.pushNewElement("Nodes");
   
   for(%i=0; %i < $ShaderGen.nodeCount; %i++)
   {
      %type = getField($ShaderGen.NODES[%i],0);
      %name = getField($ShaderGen.NODES[%i],1); 
      %uid = getField($ShaderGen.NODES[%i],2);     
      %pos = getField($ShaderGen.NODES[%i],3);
      
      %xmlDoc.pushNewElement(%type);
      %xmlDoc.setAttribute("name", %name);
      %xmlDoc.setAttribute("uid", %uid);
      %xmlDoc.setAttribute("pos", %pos);
      %xmlDoc.popElement();
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.pushNewElement("Connections");
   
   for(%i=0; %i < $ShaderGen.connCount; %i++)
   {
      %startNodeUid = getField($ShaderGen.CONNECTIONS[%i],0);
      %endNodeUid = getField($ShaderGen.CONNECTIONS[%i],1);      
      %startSocket = getField($ShaderGen.CONNECTIONS[%i],2);
      %endSocket = getField($ShaderGen.CONNECTIONS[%i],2);
      
      %xmlDoc.pushNewElement("Connection");
      %xmlDoc.setAttribute("startNodeUid", %startNodeUid);
      %xmlDoc.setAttribute("endNodeUid", %endNodeUid);
      %xmlDoc.setAttribute("startSocket", %startSocket);
      %xmlDoc.setAttribute("endSocket", %endSocket);
      %xmlDoc.popElement();
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.popElement();
   
   %xmlDoc.popElement();
   
   %xmlDoc.saveFile($ShaderGen::targetShaderFile@".sgf");
}

function LoadShaderGraphBtn::onClick(%this)
{
   %dlg = new OpenFileDialog()
   {
      Filters        = "ShaderGraph Files (*.sgf)|*.sgf|";
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
   $ShaderGen::targetShaderFile = "shaders/procedural/" @ %file;
   $CurrentShaderGraph.loadGraph(%fullPath);
}

//parser
function ShaderEditorGraph::findNextInject(%this)
{
   
}

//
//
//
function EditStateMachineBtn::onClick(%this)
{
   Canvas.pushDialog(ShaderEditor);
   ShaderEditor.behavior = %this.behavior;
   
   ShaderEditor.open();
}

function ShaderEditor::open(%this)
{
   //check our behavior and see if we have any existing state/field info to work with
   //if we do, load those up first
   for(%i = 0; %i < %this.behavior.stateMachine.count(); %i++)
   {
      %stateName = %this.behavior.stateMachine.getKey(%i);
      
      %this.addState(%stateName);
   }    
}

function ShaderEditor::addState(%this, %stateName)
{
   if(%stateName $= "")
      %stateName = "New State";
      
   %state = new GuiControl() {
      position = "0 0";
      extent = "285 50";
      horizSizing = "horizResizeWidth";
      vertSizing = "vertResizeTop";
      isContainer = "1";
      
      new GuiTextEditCtrl() {
         position = "0 0";
         extent = "100 15";
         text = %stateName;
      };
      
      new GuiButtonCtrl() {
         //buttonMargin = "4 4";
         text = "Remove State";
         position = "184 0";
         extent = "100 15";
         //profile = "GuiButtonProfile";
         command = "ScriptEditorGui.save();";
      };
      
      new GuiSeparatorCtrl() {
         position = "0 15";
         extent = %this.extent.x SPC "10";
         type = "horizontal";
      };
      
      new GuiStackControl(%stateName@"StateStack")
      {
         //Profile = "EditorContainerProfile";
         HorizSizing = "right";
         VertSizing = "bottom";
         Position = "0 25";
         Extent = "285 20";
         padding = 4;
         
         new GuiButtonCtrl() {
            text = "Add field";
            position = "3 0";
            extent = "280 20";
            horizSizing = "left";
            vertSizing = "top";
            command = "ShaderEditor.addField("@%stateName@");";
         };
      };
   };
   
   %this-->Stack.add(%state);
   //%this-->stateStackScroll.computeSizes();
}

function ShaderEditor::addField(%this, %stateName)
{
   %index = %this.behavior.stateMachine.count();
   %field = new GuiControl() {
      position = "0 0";
      extent = "285 20";
      horizSizing = "width";
      vertSizing = "height";
      isContainer = "1";
      fieldValueCtrl = "";
      fieldID = %index++;
   };
      
   %fieldList = new GuiPopUpMenuCtrlEx() 
   {
      class = "stateMachineFieldList";
      Profile = "GuiPopupMenuProfile";
      HorizSizing = "width";
      VertSizing = "bottom";
      position = "0 1";
      Extent = "120 18";
      behavior = %this.behavior;
   };
   
   %field.add(%fieldList);
   %fieldList.refresh();
   
    (%stateName@"StateStack").addToStack(%field);
      
   %this-->Stack.updateStack();
   %this-->stateStackScroll.computeSizes();
   
   %this.behavior.addStateField(%stateName, "", "");
   
   return %field;
}

//==============================================================================
function stateMachineFieldList::refresh(%this)
{
   %this.clear();
   
   // Find all the types.
   %count = getWordCount(%this.behavior.stateFields);   
   %index = 0;
   for (%j = 0; %j < %count; %j++)
   {
      %item = getWord(%this.behavior.stateFields, %j);
      %this.add(%item, %index);
      %this.fieldType[%index] = %item;
      %index++;
   }
}

function stateMachineFieldList::onSelect(%this)
{
   //if(%this.getParent().fieldValueCtrl $= "")
   %this.fieldType = %this.fieldType[%this.getSelected()];
   
   if(%this.fieldType $= "transitionOnAnimEnd" || %this.fieldType $= "transitionOnAnimTrigger" 
      || %this.fieldType $= "transitionOnTimeout")
   {
      %fieldCtrl = new GuiPopUpMenuCtrlEx() 
      {
         class = "stateMachineFieldList";
         Profile = "GuiPopupMenuProfile";
         HorizSizing = "width";
         VertSizing = "bottom";
         position = "124 1";
         Extent = "120 18";
      };
   }
   else if(%this.fieldType $= "animation")
   {
      %fieldCtrl = new GuiPopUpMenuCtrlEx() 
      {
         class = "stateMachineFieldList";
         Profile = "GuiPopupMenuProfile";
         HorizSizing = "width";
         VertSizing = "bottom";
         position = "124 1";
         Extent = "120 18";
      };
      
      %index = 0;
      %animBhvr = %this.behavior.owner.getBehavior("AnimationController");
      for(%i = 0; %i < %animBhvr.getAnimationCount(); %i++)
      {
         %item = %animBhvr.getAnimationName(%i);
         %fieldCtrl.add(%item, %index);
         %fieldCtrl.fieldValue[%index] = %item;
         %index++;
      }
   }
   else
   {
      %fieldCtrl = new GuiTextEditCtrl() {
         position = "124 1";
         extent = "120 10";
         text = "";
      };
   }
   
   //get the state machine entry
   %index = %this.getParent().fieldID;
   
   %oldValue = %this.behavior.stateMachine.getValue(%index);
   %this.behavior.stateMachine.setValue(%fieldType SPC %oldValue.y);
   
   %this.getParent().add(%fieldCtrl);
}

//==============================================================================

//Now for the unique field types
/*function stateMachineFieldList::refresh(%this)
{
   
}*/
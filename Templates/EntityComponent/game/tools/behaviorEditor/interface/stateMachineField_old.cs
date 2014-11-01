function BehaviorFieldStack::createStateMachineEditor(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.template.getBehaviorField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %button = new GuiButtonCtrl()
   {
      class = EditStateMachineBtn;
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "0 2";
      Extent = (%this.extent.x - 8) SPC 13;
      text = "Edit States";
      tooltip = "Open window to edit the state machine";
      behavior = %behavior;
   };
   %this.add(%button);
}

/*function BehaviorFieldStack::createSMStates(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.template.getBehaviorField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   new GuiTextCtrl() {
      position = "0 0";
      extent = "100 15";
      text = "Starting State:";
   };
      
   %fieldList = new GuiPopUpMenuCtrlEx() 
   {
      class = "stateMachineStatesList";
      Profile = "GuiPopupMenuProfile";
      HorizSizing = "width";
      VertSizing = "bottom";
      position = "0 1";
      Extent = "120 18";
      behavior = %this.behavior;
   };
   %this.add(%fieldList);
}*/

function EditStateMachineBtn::onClick(%this)
{
   Canvas.pushDialog(StateMachineEditor);
   StateMachineEditor.behavior = %this.behavior;
   
   StateMachineEditor.open();
}

function StateMachineEditor::open(%this)
{
   //check our behavior and see if we have any existing state/field info to work with
   //if we do, load those up first
   for(%i = 0; %i < %this.behavior.getStateCount(); %i++)
   {
      %stateName = %this.behavior.getStateByIndex(%i);
      
      %ddhdhhd = 0;
      //%this.addState(%stateName);
   }   
}

function StateMachineEditor::addNewNode(%this, %stateName)
{
   if(%stateName $= "")
      %stateName = "New State";
      
    %node = new visualWebNode();
	//{
	//	profile = "GuiMenuButtonProfile";
	//};
	stateMachineNodeTree.add(%node);
	
	%node.nodeName = %stateName;
	
	//%node.addInputSocket("In");
   
    %this.behavior.addState(%stateName);
}

function stateMachineNodeTree::onSocketClicked(%this, %nodeID, %socketID)
{
	echo("clicked on a socket!");
	%this.socketRClicked = true;	
	%this.selectedSocket = %socketID;
	%this.selectedNode = %nodeID;
}

function stateMachineNodeTree::onRightMouseUp(%this, %modifier, %mousePos, %mouseClickCount)
{
	if(%this.socketRClicked)
	{
		%popup = SocketContextPopup;
		if( !isObject( %popup ) )
			%popup = new PopupMenu( SocketContextPopup )
			{
				superClass = "MenuBuilder";
				isPopup = "1";
	
				item[ 0 ] = "Edit transition" TAB "" TAB "StateMachineEditor.editTransition("@%this@".selectedSocket);";
				
				object = -1;
			};
	}
	else
	{
		%node = %this.getNodeAtPosition(%mousePos);
		if(isObject(%node))
		{
			echo("Context-clicked on node: "@%node.getId());
			%popup = NodeContextPopup;
			if( !isObject( %popup ) )
				%popup = new PopupMenu( NodeContextPopup )
				{
					superClass = "MenuBuilder";
					isPopup = "1";
					
					item[ 0 ] = "Add transition" TAB "" TAB "%this.node.addOutputSocket(\"Transition\");";
					item[ 1 ] = "Rename State" TAB "" TAB "StateMachineEditor.renameNode("@%node@");";
					
					node = -1;
				};

			NodeContextPopup.node = %node;

		}
		else
		{
			%popup = StateMachineEditorContextPopup;
			if( !isObject( %popup ) )
				%popup = new PopupMenu( StateMachineEditorContextPopup )
				{
					superClass = "MenuBuilder";
					isPopup = "1";
					
					item[ 0 ] = "Add new state" TAB "" TAB "StateMachineEditor.addState();";
					item[ 1 ] = "Recenter view" TAB "" TAB "stateMachineNodeTree.recenter();";
				};	
		}
	}
		
	/*if( %haveObjectEntries )
   {         
      %popup.enableItem( 0, %obj.isNameChangeAllowed() && %obj.getName() !$= "MissionGroup" );
      %popup.enableItem( 1, %obj.getName() !$= "MissionGroup" );
      if( %haveLockAndHideEntries )
      {
         %popup.checkItem( 4, %obj.locked );
         %popup.checkItem( 5, %obj.hidden );
      }
      %popup.enableItem( 7, %this.isItemSelected( %itemId ) );
   }*/
   
   %popup.showPopup( Canvas );

   %this.socketRClicked = false;
}

function stateMachineNodeTree::onConnectionChanged(%this, %nodeA, %socketAID, %nodeB, %socketBID)
{
	echo("Changed a connection!");
}

function StateMachineEditor::addState(%this)
{
	%this.setupRenamer();
	
	%node = stateMachineNodeTree.selectedNode;
	
	stateMachineRenamer-->windowTitle.text = "Add State";
	stateMachineRenamer-->textFieldName.setText("State Name:");
	stateMachineRenamer-->confirmBtn.command = "StateMachineEditor.addNewNode(stateMachineRenamer-->textField.getText()); Canvas.popDialog(stateMachineRenamer);";

	stateMachineRenamer-->textField.setText("");
}

function StateMachineEditor::editTransition(%this, %socketId)
{
	%this.setupRenamer();
	
	%node = stateMachineNodeTree.selectedNode;
	
	stateMachineRenamer-->windowTitle.text = "Edit Transition";
	stateMachineRenamer-->textFieldName.setText("Set Transition:");
	stateMachineRenamer-->confirmBtn.command = %node @ ".setSocketName(" @ %socketId @ ", stateMachineRenamer-->textField.getText()); Canvas.popDialog(stateMachineRenamer);";

	stateMachineRenamer-->textField.setText("");
}

function StateMachineEditor::renameNode(%this, %node)
{
	%this.setupRenamer();
	
	stateMachineRenamer-->windowTitle.text = "Rename State";
	stateMachineRenamer-->textFieldName.setText("State name:");
	stateMachineRenamer-->confirmBtn.command = %node @ ".nodeName = stateMachineRenamer-->textField.getText(); Canvas.popDialog(stateMachineRenamer);";

	stateMachineRenamer-->textField.setText("");
}

function StateMachineEditor::setupRenamer(%this)
{
	if(!isObject(stateMachineRenamer))
	{
		%guiContent = new GuiControl(stateMachineRenamer) {
			profile = "ToolsGuiDefaultProfile";
			horizSizing = "right";
			vertSizing = "bottom";
			position = "0 0";
			extent = "800 600";
			minExtent = "8 8";
			visible = "1";
			setFirstResponder = "0";
			modal = "1";
			helpTag = "0";
			
			new GuiWindowCtrl() {
				profile = "ToolsGuiWindowProfile";
				horizSizing = "center";
				vertSizing = "center";
				position = "384 205";
				extent = "256 75";
				minExtent = "256 8";
				visible = "1";
				setFirstResponder = "0";
				modal = "1";
				helpTag = "0";
				resizeWidth = "1";
				resizeHeight = "1";
				canMove = "1";
				canClose = "0";
				canMinimize = "0";
				canMaximize = "0";
				minSize = "50 50";
				text = "Rename";
				internalName = "windowTitle";
			
				new GuiTextCtrl() {
					profile = "GuiCenterTextProfile";
					horizSizing = "right";
					vertSizing = "bottom";
					position = "9 26";
					extent = "84 16";
					minExtent = "8 8";
					visible = "1";
					setFirstResponder = "0";
					modal = "1";
					helpTag = "0";
					text = "Object Name:";
					internalName = "textFieldName";
				};
				new GuiTextEditCtrl() {
					class = ObjectBuilderGuiTextEditCtrl;
					profile = "ToolsGuiTextEditProfile";
					horizSizing = "width";
					vertSizing = "bottom";
					position = "78 26";
					extent = "172 18";
					minExtent = "8 8";
					visible = "1";
					setFirstResponder = "0";
					modal = "1";
					helpTag = "0";
					historySize = "0";
					internalName = "textField";
				};
				new GuiIconButtonCtrl() {
					buttonMargin = "4 4";
					iconBitmap = "tools/gui/images/iconCancel.png";
					iconLocation = "Left";
					textLocation = "Center";
					textMargin = "4";
					text = "Close";
					position = "90 50";
					extent = "80 20";
					horizSizing = "left";
					vertSizing = "top";
					command = "Canvas.popDialog(stateMachineRenamer);";
				};
				new GuiIconButtonCtrl() {
					buttonMargin = "4 4";
					iconBitmap = "tools/gui/images/iconAccept.png";
					iconLocation = "Left";
					textLocation = "Center";
					textMargin = "4";
					text = "Save";
					position = "175 50";
					extent = "80 20";
					horizSizing = "left";
					vertSizing = "top";
					command = "Canvas.popDialog(stateMachineRenamer);";
					internalName = "confirmBtn";
				};
			};
		};
	}
	
	Canvas.pushdialog(stateMachineRenamer);
}
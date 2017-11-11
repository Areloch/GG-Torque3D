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
function CreateAssetButton::onClick(%this)
{
   AddNewAssetPopup.showPopup(Canvas);
}

function AssetBrowser_newAsset::onWake(%this)
{
   NewAssetPackageList.refresh();
   NewComponentParentClass.setText("Component");
}

function NewAssetTypeList::onWake(%this)
{
   %this.refresh();
}

function NewAssetTypeList::refresh(%this)
{
   %this.clear();
   
   //TODO: make this more automated
   //%this.add("GameObject", 0);
   %this.add("Component", 0);
   %this.add("Image", 1);
   %this.add("Material", 2);
   %this.add("Shape", 3);  
   %this.add("Sound", 4);
   %this.add("State Machine", 5);
}

function NewAssetTypeList::onSelected(%this)
{
   %assetType = %this.getText();
   
   if(%assetType $= "Component")
   {
      NewComponentAssetSettings.hidden = false;
   }
}

function NewAssetPackageBtn::onClick(%this)
{
   Canvas.pushDialog(AssetBrowser_AddPackage);
   AssetBrowser_addPackageWindow.selectWindow();
}

function CreateNewAsset()
{
   %assetName = NewAssetName.getText();
   
   if(%assetName $= "")
	{
		error("Attempted to make a new asset with no name!");
		Canvas.popDialog(AssetBrowser_newAsset);
		return;
	}
	
	if(NewAssetTypeList.getText() $= "")
	{
	   error("Attempted to make a new asset with no type!");
		Canvas.popDialog(AssetBrowser_newAsset);
		return;
	}
	
	//get the selected module data
   %moduleName = NewAssetPackageList.getText();
   
   %path = "data/" @ %moduleName;
	
	%assetType = NewAssetTypeList.getText();
	if(%assetType $= "Component")
	{
	   Canvas.popDialog(AssetBrowser_newComponentAsset); 
	   AssetBrowser_newComponentAsset-->AssetBrowserPackageList.setText(AssetBrowser.selectedModule);
	   //%assetFilePath = createNewComponentAsset(%assetName, %path);
	}
	else if(%assetType $= "Material")
	{
	   %assetFilePath = createNewMaterialAsset(%assetName, %path);
	}
	else if(%assetType $= "State Machine")
	{
	   %assetFilePath = createNewStateMachineAsset(%assetName, %path);
	}
	
	Canvas.popDialog(AssetBrowser_newAsset);
	
	//Load it
	%moduleDef = ModuleDatabase.findModule(%moduleName,1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %assetFilePath);
	
	AssetBrowser.loadFilters();
}

function createNewComponentAsset()
{
   %moduleName = AssetBrowser_newComponentAssetWindow-->NewComponentPackageList.getText();
   %modulePath = "data/" @ %moduleName;
   
   if(%modulePath $= "")
      %modulePath = "data/" @ AssetBrowser.selectedModule;
      
   %assetName = NewComponentName.getText();
   
   %tamlpath = %modulePath @ "/components/" @ %assetName @ ".asset.taml";
   %scriptPath = %modulePath @ "/components/" @ %assetName @ ".cs";
   
   %asset = new ComponentAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      componentName = NewComponentName.getText();
      componentClass = ParentComponentList.getText();
      friendlyName = NewComponentFriendlyName.getText();
      componentType = NewComponentGroup.getText();
      description = NewComponentDescription.getText();
      scriptFile = %scriptPath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   %file = new FileObject();
	
	if(%file.openForWrite(%scriptPath))
	{
		%file.writeline("function " @ %assetName @ "::onAdd(%this)\n{\n\n}\n");
		%file.writeline("function " @ %assetName @ "::onRemove(%this)\n{\n\n}\n");
		%file.writeline("function " @ %assetName @ "::onClientConnect(%this, %client)\n{\n\n}\n");
		%file.writeline("function " @ %assetName @ "::onClientDisonnect(%this, %client)\n{\n\n}\n");
		%file.writeline("function " @ %assetName @ "::Update(%this)\n{\n\n}\n");
		
		%file.close();
	}
	
	Canvas.popDialog(AssetBrowser_newComponentAsset);
	
	%moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "Components");
	
	AssetBrowserFilterTree.onSelect(%smItem);
	
	return %tamlpath;
}

function createNewMaterialAsset(%assetName, %moduleName)
{
   %assetName = NewAssetName.getText();
   
   %tamlpath = "data/" @ %moduleName @ "/materials/" @ %assetName @ ".asset.taml";
   %sgfPath = "data/" @ %moduleName @ "/materials/" @ %assetName @ ".sgf";
   
   %asset = new MaterialAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      shaderData = "";
      shaderGraph = %sgfPath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   %moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "Materials");
	
	AssetBrowserFilterTree.onSelect(%smItem);
   
	return %tamlpath;
}

function createNewStateMachineAsset(%assetName, %moduleName)
{
   if(%assetName $= "")
      %assetName = NewAssetName.getText();
      
   %assetQuery = new AssetQuery();
   
   %matchingAssetCount = AssetDatabase.findAssetName(%assetQuery, %assetName);
   
   %i=1;
   while(%matchingAssetCount > 0)
   {
      %newAssetName = %assetName @ %i;
      %i++;
      
      %matchingAssetCount = AssetDatabase.findAssetName(%assetQuery, %newAssetName);
   }
   
   %assetName = %newAssetName;
   
   %assetQuery.delete();
   
   %tamlpath = "data/" @ %moduleName @ "/stateMachines/" @ %assetName @ ".asset.taml";
   %smFilePath = "data/" @ %moduleName @ "/stateMachines/" @ %assetName @ ".xml";
   
   %asset = new StateMachineAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      stateMachineFile = %smFilePath;
   };
   
   %xmlDoc = new SimXMLDocument();
   %xmlDoc.saveFile(%smFilePath);
   %xmlDoc.delete();
   
   TamlWrite(%asset, %tamlpath);
   
   //Now write our XML file
   %xmlFile = new FileObject();
	%xmlFile.openForWrite(%smFilePath);
	%xmlFile.writeLine("<StateMachine>");
	%xmlFile.writeLine("</StateMachine>");
	%xmlFile.close();
   
   %moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "StateMachines");
	
	AssetBrowserFilterTree.onSelect(%smItem);
   
	return %tamlpath;
}

function AssetBrowser::createNewGUIAsset(%this, %newAssetName, %selectedModule)
{
   %moduleName = %selectedModule;
   %modulePath = "data/" @ %selectedModule;
   
   if(%modulePath $= "")
      %modulePath = "data/" @ AssetBrowser.selectedModule;
      
   %assetName = %newAssetName;
   
   %tamlpath = "data/" @ %moduleName @ "/GUIs/" @ %assetName @ ".asset.taml";
   %guipath = %modulePath @ "/GUIs/" @ %assetName @ ".gui";
   %scriptPath = %modulePath @ "/GUIs/" @ %assetName @ ".cs";
   
   %asset = new GUIAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      scriptFilePath = %scriptPath;
      guiFilePath = %guipath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   %file = new FileObject();
   
   if(%file.openForWrite(%guipath))
	{
	   %file.writeline("//--- OBJECT WRITE BEGIN ---");
		%file.writeline("%guiContent = new GuiControl(" @ %assetName @ ") {");
		%file.writeline("   position = \"0 0\";");
		%file.writeline("   extent = \"100 100\";");
		%file.writeline("};");
		%file.writeline("//--- OBJECT WRITE END ---");
		
		%file.close();
	}
	
	if(%file.openForWrite(%scriptPath))
	{
		%file.writeline("function " @ %assetName @ "::onWake(%this)\n{\n\n}\n");
		%file.writeline("function " @ %assetName @ "::onSleep(%this)\n{\n\n}\n");
		
		%file.close();
	}
	
	//load the gui
	exec(%guipath);
	exec(%scriptPath);
	
	%moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "GUIs");
	
	AssetBrowserFilterTree.onSelect(%smItem);
	
	return %tamlpath;
}

function ParentComponentList::onWake(%this)
{
   %this.refresh();
}

function ParentComponentList::refresh(%this)
{
   %this.clear();
   
   %assetQuery = new AssetQuery();
   if(!AssetDatabase.findAssetType(%assetQuery, "ComponentAsset"))
      return; //if we didn't find ANY, just exit
   
   // Find all the types.
   %count = %assetQuery.getCount();

   /*%categories = "";
   for (%i = 0; %i < %count; %i++)
   {
      %assetId = %assetQuery.getAsset(%i);
      
      %componentAsset = AssetDatabase.acquireAsset(%assetId);
      %componentName = %componentAsset.componentName;
      
      if(%componentName $= "")
         %componentName = %componentAsset.componentClass;
      
      %this.add(%componentName, %i);
   }*/
   
   %categories = "";
   for (%i = 0; %i < %count; %i++)
   {
      %assetId = %assetQuery.getAsset(%i);
      
      %componentAsset = AssetDatabase.acquireAsset(%assetId);
      %componentClass = %componentAsset.componentClass;
      if (!isInList(%componentClass, %categories))
         %categories = %categories TAB %componentClass;
   }
   
   %categories = trim(%categories);
   
   %index = 0;
   %categoryCount = getFieldCount(%categories);
   for (%i = 0; %i < %categoryCount; %i++)
   {
      %category = getField(%categories, %i);
      %this.addCategory(%category);
      
      for (%j = 0; %j < %count; %j++)
      {
         %assetId = %assetQuery.getAsset(%j);
      
         %componentAsset = AssetDatabase.acquireAsset(%assetId);
         %componentName = %componentAsset.componentName;
         %componentClass = %componentAsset.componentClass;
      
         if (%componentClass $= %category)
         {
            if(%componentName !$= "")
               %this.add("   "@%componentName, %i);
         }
      }
   }
}
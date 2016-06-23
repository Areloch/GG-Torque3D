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

function AssetBrowser_newAsset::onWake(%this)
{
   NewAssetPackageList.refresh();
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
   //%this.add("Shape", 1);   
   //%this.add("Image", 1);  
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
   
   %path = "modules/" @ %moduleName;
	
	%assetType = NewAssetTypeList.getText();
	if(%assetType $= "Component")
	{
	   %assetFilePath = createNewComponentAsset(%assetName, %path);
	}
	
	Canvas.popDialog(AssetBrowser_newAsset);
	
	//Load it
	%moduleDef = ModuleDatabase.findModule(%moduleName,1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %assetFilePath);
	
	AssetBrowser.loadFilters();
}

function createNewComponentAsset(%assetName, %modulePath)
{
   %assetName = NewAssetName.getText();
   
   %tamlpath = %modulePath @ "/components/" @ %assetName @ ".asset.taml";
   %scriptPath = %modulePath @ "/components/" @ %assetName @ ".cs";
   
   %asset = new ComponentAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      componentName = %assetName;
      componentClass = "Component";
      friendlyName = NewComponentFriendName.getText();
      componentType = NewComponentGroupName.getText();
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
	
	return %tamlpath;
}
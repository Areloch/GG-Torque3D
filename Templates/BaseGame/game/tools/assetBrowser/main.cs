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
if( !isObject( ToolsGuiDefaultNonModalProfile ) )
new GuiControlProfile (ToolsGuiDefaultNonModalProfile : ToolsGuiDefaultProfile)
{
   modal = false;
};

function initializeAssetBrowser()
{
   echo(" % - Initializing Asset Browser");  
   
   exec("./scripts/guis/assetBrowser.gui");
   exec("./scripts/guis/addPackageWindow.gui");
   exec("./scripts/guis/gameObjectCreator.gui");
   exec("./scripts/guis/newAsset.gui");
   exec("./scripts/guis/newComponentAsset.gui");
   exec("./scripts/guis/editAsset.gui");
   exec("./scripts/guis/assetImport.gui");

   exec("./scripts/assetBrowser.cs");
   exec("./scripts/addPackageWindow.cs");
   exec("./scripts/assetImport.cs");
   exec("./scripts/gameObjectCreator.cs");
   exec("./scripts/newAsset.cs");
   exec("./scripts/editAsset.cs");
   exec("./scripts/fieldTypes.cs");
   
   new ScriptObject( AssetBrowserPlugin )
   {
      superClass = "EditorPlugin";
   };
   
   Input::GetEventManager().subscribe( AssetBrowser, "BeginDropFiles" );
   Input::GetEventManager().subscribe( AssetBrowser, "DropFile" );
   Input::GetEventManager().subscribe( AssetBrowser, "EndDropFiles" );
   
   AssetBrowser.buildPopupMenus();
}

function AssetBrowserPlugin::onWorldEditorStartup( %this )
{ 
   // Add ourselves to the toolbar.
   AssetBrowser.addToolbarButton();
}
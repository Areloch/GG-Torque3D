function UI::onCreate( %this )
{
}

function UI::onDestroy( %this )
{
}

function UI::initClient(%this)
{
   //Load UI stuff
   //Profiles
   exec("./scripts/profiles.cs");
   exec("./scripts/profiles.ed.cs");
   
   //Now gui files
   exec("./guis/mainMenu.gui");
   exec("./guis/mainMenu.cs");
   exec("./guis/startupGui.gui");
   exec("./guis/startupGui.cs");
   
   exec("./guis/buttonTemplates.gui");
   
   exec("./scripts/cursors.cs");
   
   // Load Editor Dialogs
   exec("./guis/messageBoxOk.gui");
   exec("./guis/messageBoxYesNo.gui");
   
   //Load scripts
   exec("./scripts/messageBoxes.cs");
   exec("./scripts/cursors.cs");
   
   exec("./scripts/projectManagement.cs");
   exec("./scripts/engineManagement.cs");
  
   loadStartup();
}
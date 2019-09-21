function loadProjectsPage()
{
   PanelGrid.clear();
   updateNewPanelBtn();
   
   $Mode = "Projects";
}

function makeNewProject()
{
   ManagePanelInspector.startGroup("New Project");
   ManagePanelInspector.addField("ProjectName", "Project Name", "String", "", "", "", %assetConfigObj);
   ManagePanelInspector.addField("ProjectDir", "Project Directory", "filename", "", "", "", %assetConfigObj);
   ManagePanelInspector.endGroup();
   
   ManagePanelInspector.startGroup("Engine Build");
   ManagePanelInspector.addField("IsSource", "Use Binary Build", "bool", "", "1", "", %assetConfigObj);
   ManagePanelInspector.addField("Build", "Engine Build", "list", "", "Preview4_0", "", %assetConfigObj);
   ManagePanelInspector.endGroup();
   
   ManagePanelInspector.startGroup("Template");
   ManagePanelInspector.addField("Template", "Template", "list", "", "BaseGame", "", %assetConfigObj);
   ManagePanelInspector.addField("StarterContent", "Use Starter Content", "bool", "", "1", "", %assetConfigObj);
   ManagePanelInspector.endGroup();
}

function AddProjectBtn::onClick(%this)
{
   makeProject("T3D_4_0", "Test");
}

function makeProject(%engineBuild, %projectName)
{
   if(%projectName $= "")
      %projectName = "TestProject";
      
   %srcPath = makeFullPath("data/EngineBuilds/" @ %engineBuild);
   if(%srcPath $= "")
      %srcPath = "C:/gd/T3DMIT/PR_Testing/PMCLTest";
      
   %bldPath = %srcPath @ "/MyProjects/" @ %projectName @ "/Build/";
   
   if(!isDirectory(%bldPath))
      createPath(%bldPath);
      
      //Visual Studio 14 2015
      //Visual Studio 15 2017
      //Visual Studio 16 2019
   //%command = "cd \"" @ %bldPath @ "\" && cmake \"" @ %srcPath @ "\" -G \"Visual Studio 16 2019\" -DTORQUE_APP_NAME:STRING=" @ %projectName @ " -DCMAKE_GENERATOR_PLATFORM=x64";
   
   %command = "cd \"D:/gd/PM/EngineBuilds/T3D_4_0/MyProjects/Test/Build/\" && cmake \"D:/gd/PM/EngineBuilds/T3D_4_0\" -G \"Visual Studio 16 2019\" -DTORQUE_APP_NAME:STRING=Test -DCMAKE_GENERATOR_PLATFORM=x64";
   systemCommand(%command, "makeProjectComplete");  
   
   %openCommand = "start \"D:/gd/PM/EngineBuilds/T3D_4_0/MyProjects/Test/Build/Test.sln\"";
   systemCommand(%openCommand, "");
}

function makeProjectComplete(%resultCode)
{
   error("Running makeProject resulted in a return code of " @ %resultCode);
}
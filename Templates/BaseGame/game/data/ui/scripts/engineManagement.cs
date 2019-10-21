function addEngineVerListButton()
{
   %newEngineButton = EngineListButtonTemplate.deepClone();
   
   %newEngineButton-->editButton.command = "echo(\"AAAAAAAAAAAAAAAAAAAAAAAA\");";
   
   MainList.add(%newEngineButton);
   
   return %newEngineButton;
}

function loadEnginesData()
{
   if(!isObject(EnginesList))
      new ArrayObject(EnginesList);
   else
      EnginesList.empty();
      
   %xmlDoc = new SimXMLDocument();
   if(%xmlDoc.loadFile("data/engineVersions.xml"))
   {  
      if(!%xmlDoc.pushFirstChildElement("EngineVersions"))
      {
         error("Invalid Engine Data file");
         return;  
      }
      
      %configCount = 0;
      %hasGroup = %xmlDoc.pushFirstChildElement("Version");
      while(%hasGroup)
      {
         %projectData = new ScriptObject();
         %projectData.name = %xmlDoc.attribute("name");
         %projectData.localPath = %xmlDoc.attribute("localPath");
         %projectData.gitPath = %xmlDoc.attribute("gitPath");
         %projectData.gitBranch = %xmlDoc.attribute("gitBranch");
         
         EnginesList.add(%projectData);
         
         %hasGroup = %xmlDoc.nextSiblingElement("Version");
      }
      
      %xmlDoc.popElement();
   }
   %xmlDoc.delete();
   
   MainList.clear();
   
   for(%i=0; %i < EnginesList.count(); %i++)
   {
      %engineData = EnginesList.getKey(%i);
      
      %engineButton = addEngineVerListButton();
      %engineButton-->EntryName.Text = %engineData.name;
      %engineButton-->gitPath.Text = "Git Path:" SPC %engineData.gitPath;
      %engineButton-->branch.Text = "Git Branch:" SPC %engineData.gitBranch;
      %engineButton-->localPath.Text = "Local Path:" SPC %engineData.localPath;
      
      if(%engineData.localPath $= "")
      {
         %engineButton-->isDownloaded.visible = false;
         %engineButton-->addProject.visible = false;
         %engineButton-->deleteEngineVer.visible = false;
         
         %localPath = "\"EngineBuilds/" @ %engineData.name @ "\"";
         
         %downloadCommand = "downloadEngineBuild("@ %localPath @","@
                                                    %engineData @ ");";
                                                    
         
         %engineButton-->downloadEngineVer.command = %downloadCommand;
      }
      else
      {
         %engineButton-->downloadEngineVer.visible = false;
         
         %newProjCommand = "createNewProject();";
                                                    
         
         %engineButton-->addProject.command = %newProjCommand;
      }
      
      %engineButton-->showInExplorer.visible = false;
      
      MainList.add(%engineButton);
   }
}

function saveEnginesData()
{
   %xmlDoc = new SimXMLDocument();
    
   %xmlDoc.pushNewElement("EngineVersions");
   
   for(%i=0; %i < EnginesList.count(); %i++)
   {
      %engineData = EnginesList.getKey(%i);
      
      %xmlDoc.pushNewElement("Version");
      %xmlDoc.setAttribute("name", %engineData.name);
      %xmlDoc.setAttribute("localPath", %engineData.localPath);
      %xmlDoc.setAttribute("gitPath", %engineData.gitPath);
      %xmlDoc.setAttribute("gitBranch", %engineData.gitBranch);
      %xmlDoc.popElement();
      
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.saveFile("data/engineVersions.xml");
}

function downloadEngineBuild(%path, %engineData)
{
   $currentDownloadingEngineBuild = %engineData;
   
   %path = makeFullPath(%path);
   
   $currentDownloadingPath = %path;
   
   %command = "git clone --branch " @ %engineData.gitBranch SPC %engineData.gitPath SPC "\"" @ %path @ "\"";
      
   systemCommand(%command, "downloadEngineBuildComplete");  
}

function downloadEngineBuildComplete(%resultCode)
{
   if(%resultCode == 0)
   {
      $currentDownloadingEngineBuild.localPath = $currentDownloadingPath;
      
      saveEnginesData();
      loadEnginesData();
   }
   else
      error("Running downloadEngineBuild resulted in a return code of " @ %resultCode);
}

function addProjectListButton()
{
   %newProjectButton = ProjectListButtonTemplate.deepClone();
   
   %newProjectButton-->editButton.command = "echo(\"AAAAAAAAAAAAAAAAAAAAAAAA\");";
   
   MainList.add(%newProjectButton);
   
   return %newProjectButton;
}

function createNewProject()
{
   openProjectsPage();
   %newProjectButton = addProjectListButton();
   %newProjectButton-->EntryName.Text = "New Project";
}

function locateExistingProject()
{
   %dlg = new OpenFolderDialog()
   {
      Title = "Select Project Folder";
      Filters = %filter;
      DefaultFile = %currentFile;
      ChangePath = false;
      MustExist = true;
      MultipleFiles = false;
   };

   if(filePath( %currentFile ) !$= "")
      %dlg.DefaultPath = filePath(%currentFile);
   else
      %dlg.DefaultPath = getMainDotCSDir();
   
   if(%dlg.Execute())
   {
      %projectButton = addProjectListButton();
      
      %projectButton-->EntryName.Text = "Unnamed Project";
   }

   %dlg.delete();
}

function loadProjectsData()
{
   if(!isObject(ProjectsList))
      new ArrayObject(ProjectsList);
   else
      ProjectsList.empty();
      
   %xmlDoc = new SimXMLDocument();
   if(%xmlDoc.loadFile("data/projects.xml"))
   {  
      if(!%xmlDoc.pushFirstChildElement("Projects"))
      {
         error("Invalid Projects Data file");
         return;  
      }
      
      %configCount = 0;
      %hasGroup = %xmlDoc.pushFirstChildElement("Project");
      while(%hasGroup)
      {
         %projectData = new ScriptObject();
         %projectData.name = %xmlDoc.attribute("name");
         %projectData.path = %xmlDoc.attribute("path");
         %projectData.exeName = %xmlDoc.attribute("exeName");
         %projectData.engineVersion = %xmlDoc.attribute("engineVersion");
         
         ProjectsList.add(%projectData);
         
         %hasGroup = %xmlDoc.nextSiblingElement("Project");
      }
      
      %xmlDoc.popElement();
   }
   %xmlDoc.delete();
   
   MainList.clear();
   
   for(%i=0; %i < ProjectsList.count(); %i++)
   {
      %projData = ProjectsList.getKey(%i);
      
      %projectButton = addProjectListButton();
      %projectButton-->EntryName.Text = %projectData.name;
      %projectButton-->ProjectPath.Text = %projectData.path;
      %projectButton-->EngineVer.Text = %projectData.engineVersion;
      
      MainList.add(%projectButton);
   }
}
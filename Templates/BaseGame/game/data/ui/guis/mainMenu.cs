function MainMenuGui::onWake(%this)
{
   ActionsMenu.visible = false;
}

function openConfigMenu()
{
   ActionsMenu.visible = !ActionsMenu.visible;
}

function openEngineVersionsPage()
{
   ProjectsActionBar.visible = false;
   EngineVerActionBar.visible = true;
   loadEnginesData();
}

function openProjectsPage()
{
   ProjectsActionBar.visible = true;
   EngineVerActionBar.visible = false;
   loadProjectsData();
}
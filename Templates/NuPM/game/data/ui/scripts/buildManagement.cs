function AddEngineBldBtn::onClick(%this)
{
   downloadEngineBuild();
}

function downloadEngineBuild()
{
   %path = makeFullPath("data/EngineBuilds/T3D_4_0");
      
   systemCommand("git clone --branch Preview4_0 https://github.com/Areloch/Torque3D \"" @ %path @ "\"", "downloadEngineBuildComplete");  
}

function downloadEngineBuildComplete(%resultCode)
{
   error("Running downloadEngineBuild resulted in a return code of " @ %resultCode);
}

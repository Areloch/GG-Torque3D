
singleton TSShapeConstructor(Red_GuyDae)
{
   baseShape = "./Red_Guy.dae";
   loadLights = "0";
};

function Red_GuyDae::onLoad(%this)
{
   %this.addSequence("ambient", "ReadyIdle", "2", "3", "1", "0");
   %this.addSequence("modules/Characters/Animations/RedGuy_ReadySword.dae", "ReadySword", "0", "1", "1", "0");
   %this.addSequence("modules/Characters/Animations/RedGuy_CrouchIdle.cached.dts", "CrouchIdle", "0", "1", "1", "0");
   %this.addSequence("modules/Characters/Animations/GroundMoveTest.dae", "groundmovetest", "0", "-1", "1", "0");
   %this.addSequence("modules/Characters/Animations/RedGuy_JogF.dae", "JogF", "0", "-1", "1", "0");
}

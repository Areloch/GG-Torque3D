
singleton TSShapeConstructor(UpdatedRedGuyDae)
{
   baseShape = "./UpdatedRedGuy.dae";
};

function UpdatedRedGuyDae::onLoad(%this)
{
   %this.addSequence("data/Characters/Animations/RedGuy_StandRifleLow.dae", "StandRifleLow", "0", "19", "1", "0");
   %this.addSequence("data/Characters/Animations/RedGuy_StandRifleShouldered.dae", "StandRifleShouldered", "0", "19", "1", "0");
   %this.addSequence("data/Characters/Animations/CrouchRifleLow.dae", "CrouchRifleLow", "0", "19", "1", "0");
   %this.addSequence("data/Characters/Animations/StandRifleLow_HLook.dae", "StandRifleLow_HLook", "0", "29", "1", "0");
   %this.addSequence("data/Characters/Animations/StandRifleShouldered_Look.dae", "StandRifleShouldered_Look", "0", "29", "1", "0");
   %this.setSequenceCyclic("StandRifleShouldered_Look", "0");
   %this.setSequenceBlend("StandRifleLow_HLook", "1", "StandRifleLow", "0");
   %this.setSequenceBlend("StandRifleShouldered_Look", "1", "StandRifleShouldered", "0");
   %this.addSequence("data/Characters/Animations/ProneRifle.dae", "ProneRifleLow", "0", "-1", "1", "0");
   %this.addSequence("data/Characters/Animations/ProneRifle_HLook.dae", "ProneRifleLow_HLook", "0", "29", "1", "0");
   %this.setSequenceCyclic("ProneRifleLow_HLook", "0");
}

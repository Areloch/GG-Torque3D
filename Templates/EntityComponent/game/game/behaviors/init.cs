//we'll try and load some of the editor scripts pre-emptively here, if they exist.
//some behaviors may try and set up custom editor field types, and we need to be
//ready
//if(isFile("tools/behaviorEditor/scripts/behaviorEditor.ed.cs"))
//   exec("tools/behaviorEditor/scripts/behaviorEditor.ed.cs");

//Supporting code
exec("./behaviorManagement.cs");

// Native, 'Component' Behaviors:
exec("./nativeBehaviors.cs");

//render
//exec("./render/renderShape.cs");
exec("./render/clientVisibility.cs");

//game
exec("./game/controlObject.cs");
exec("./game/takesDamage.cs");
exec("./game/itemRotate.cs");
exec("./game/ambientAnimation.cs");
exec("./game/AnimationController.cs");

exec("./game/StateMachine.cs");

exec("./game/PlayerStateController.cs");

exec("./game/mountToNode.cs");

exec("./game/playerController.cs");

//Camera
exec("./game/camera.cs");
exec("./game/MountedCamera.cs");

//input
exec("./input/inputManager.cs");
exec("./input/FPSControls.cs");
exec("./input/SpectatorControls.cs");
exec("./input/EditorControls.cs");
//exec("./input/SideScrollControls.cs");

//Collisions
exec("./collision/boxCollider.cs");

//triggers
exec("./trigger/triggerComponent.cs");
exec("./trigger/visibilityTrigger.cs");
exec("./trigger/slenderTrigger.cs");

//Physics
//exec("./physics/simplePhysics.cs");

//GUI
//exec("./GUI/ServerGuiTestBehavior.cs");
//exec("./GUI/ClientGuiTestBehavior.cs");
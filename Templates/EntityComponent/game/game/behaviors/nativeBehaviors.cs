//These are declarations for behavior objects that are set up in the engine.
//Since they don't need any setup other than declaration,  we merely declare them here

singleton RenderShapeBehavior(RenderShape);

singleton ConvexShapeComponent(ConvexShapeComp);

singleton LevelTileComponent(LevelTileComp);

singleton StateMachineBehavior(StateMachine);
StateMachine.addBehaviorField(playerStates, "States of the player controller", "stateMachine", "");

function StateMachine::onAdd(%this)
{
	/*%this.addState("State1");
	%this.addTransition("State1", "State2", "stateTime", ">=", 2000);
	
	//%this.addTransition("State1", "State2", "timeout", ">=", 2000 );
	//%this.addTransition("State1", "State2", "testBool", "true" );
	//%this.addTransition("State1", "State2", "testString", "!=", "what" );
	//%this.addTransition("State1", "State2", "testString", "==", "what" );
	
	%this.addState("State2");
	%this.addTransition("State2", "State1", "stateTime", ">=", 2000);
	
	%this.setState("State1");*/
	
	
}

function StateMachine::onStateChange(%this)
{
	echo(%this.getState());	
}

singleton RenderSpriteBehavior(RenderSprite);

singleton SimplePhysicsBehavior(SimplePhysics);

singleton SpotLightBehavior(SpotLight);

singleton BrushShapeBehavior(BrushShape);

singleton ConvexShapeBehavior(ConvexShapeBhvr);

singleton MeshColliderBehavior(MeshCollider);

singleton SpotLightBehavior(SpotLightBhvr);

function setupBrush()
{
   %this = brush.getBehavior(BrushShape);
   
   %size = "4 4 4";
   // Determine the maximum and minimum dimensions from the input size.
   %maxX =  getWord(%size, 0) / 2; 
   %minX = -%maxX;
   
   %maxY =  getWord(%size, 1) / 2; 
   %minY = -%maxY;
   
   %maxZ =  getWord(%size, 2);     
   %minZ = 0;

   //
   %this.addSurfacePlane(0, 0, 0, 1, "0 0 " @ %maxZ);
   %this.addSurfacePlane(0, 1, 0, 0, "0 0 " @ %minZ);
   
   %this.addSurfacePlane(0.707107, 0, 0, 0.707107, "0 " @  %maxY @ " 0");
   %this.addSurfacePlane(0, 0.707107, -0.707107, 0, "0 " @ %minY @ " 0");

   %this.addSurfacePlane(0.5, 0.5, -0.5, 0.5, %minX @ " 0 0");
   %this.addSurfacePlane(0.5, -0.5, 0.5, 0.5, %maxX @ " 0 0");
   
   %this.updateGeometry();
   
   setupSubBrush();
}

function setupSubBrush()
{
   %brush = subBrush.getBehavior(BrushShape);
   %brush.subtract = true;
   
   %size = "2 2 2";
   // Determine the maximum and minimum dimensions from the input size.
   %maxX =  getWord(%size, 0) / 2; 
   %minX = -%maxX;
   
   %maxY =  getWord(%size, 1) / 2; 
   %minY = -%maxY;
   
   %maxZ =  getWord(%size, 2);     
   %minZ = 0;

   //
   %brush.addSurfacePlane(0, 0, 0, 1, "0 0 " @ %maxZ);
   %brush.addSurfacePlane(0, 1, 0, 0, "0 0 " @ %minZ);
   
   %brush.addSurfacePlane(0.707107, 0, 0, 0.707107, "0 " @  %maxY @ " 0");
   %brush.addSurfacePlane(0, 0.707107, -0.707107, 0, "0 " @ %minY @ " 0");

   %brush.addSurfacePlane(0.5, 0.5, -0.5, 0.5, %minX @ " 0 0");
   %brush.addSurfacePlane(0.5, -0.5, 0.5, 0.5, %maxX @ " 0 0");
   
   %brush.updateGeometry();
}


function Entity::onInspect(%this)
{
	echo("Well, hello there entity!");	
}
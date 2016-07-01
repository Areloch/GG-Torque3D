//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
function execGameObjects()
{
   //find all GameObjectAssets
   %assetQuery = new AssetQuery();
   if(!AssetDatabase.findAssetType(%assetQuery, "GameObjectAsset"))
      return; //if we didn't find ANY, just exit
      
   %count = %assetQuery.getCount();
      
	for(%i=0; %i < %count; %i++)
	{
	   %assetId = %assetQuery.getAsset(%i);
      
      %gameObjectAsset = AssetDatabase.acquireAsset(%assetId);
      
      if(isFile(%gameObjectAsset.scriptFilePath))
         exec(%gameObjectAsset.scriptFilePath);
	}
}

function findGameObject(%name)
{
   //find all GameObjectAssets
   %assetQuery = new AssetQuery();
   if(!AssetDatabase.findAssetType(%assetQuery, "GameObjectAsset"))
      return 0; //if we didn't find ANY, just exit
      
   %count = %assetQuery.getCount();
      
	for(%i=0; %i < %count; %i++)
	{
	   %assetId = %assetQuery.getAsset(%i);
      
      %gameObjectAsset = AssetDatabase.acquireAsset(%assetId);
      
      if(%gameObjectAsset.gameObjectName $= %name)
		{
		   if(isFile(%gameObjectAsset.TAMLFilePath))
         {
            return %gameObjectAsset;
         }
		}
	}
		
	return 0;
}

function spawnGameObject(%name, %addToMissionGroup)
{
   if(%addToMissionGroup $= "")
		%addToMissionGroup = true;
		
   //First, check if this already exists in our GameObjectPool
   if(isObject(GameObjectPool))
   {
      %goCount = GameObjectPool.countKey(%name);
      
      //if we have some already in the pool, pull it out and use that
      if(%goCount != 0)
      {
         %goIdx = GameObjectPool.getIndexFromKey(%name);
         %go = GameObjectPool.getValue(%goIdx);
         
         %go.setHidden(false);
         %go.setScopeAlways();
         
         if(%addToMissionGroup == true) //save instance when saving level
            MissionGroup.add(%go);
         else // clear instance on level exit
            MissionCleanup.add(%go);
            
         //remove from the object pool's list
         GameObjectPool.erase(%goIdx);
         
         return %go;
      }
   }
   
	//We have no existing pool, or no existing game objects of this type, so spawn a new one
		
   %gameObjectAsset = findGameObject(%name);
   
   if(isObject(%gameObjectAsset))
   {
      %newSGOObject = TamlRead(%gameObjectAsset.TAMLFilePath);
            
      if(%addToMissionGroup == true) //save instance when saving level
         MissionGroup.add(%newSGOObject);
      else // clear instance on level exit
         MissionCleanup.add(%newSGOObject);
         
      return %newSGOObject;
   }
		
	return 0;
}

function saveGameObject(%name, %tamlPath, %scriptPath)
{
	%gameObjectAsset = findGameObject(%name);
   
   //find if it already exists. If it does, we'll update it, if it does not, we'll  make a new asset
   if(isObject(%gameObjectAsset))
   {
      %assetID = %gameObjectAsset.getAssetId();
      
      %gameObjectAsset.TAMLFilePath = %tamlPath;
      %gameObjectAsset.scriptFilePath = %scriptPath;
      
      TAMLWrite(%gameObjectAsset, AssetDatabase.getAssetFilePath(%assetID));
      AssetDatabase.refreshAsset(%assetID);
   }
   else
   {
      //Doesn't exist, so make a new one
      %gameObjectAsset = new GameObjectAsset()
      {
         assetName = %name @ "Asset";
         gameObjectName = %name;
         TAMLFilePath = %tamlPath;
         scriptFilePath = %scriptPath;
      };
      
      //Save it alongside the taml file
      %path = filePath(%tamlPath);
      
      TAMLWrite(%gameObjectAsset, %path @ "/" @ %name @ ".asset.taml");
      AssetDatabase.refreshAllAssets(true);
   }
}

//Allocates a number of a game object into a pool to be pulled from as needed
function allocateGameObjects(%name, %amount)
{
   //First, we need to make sure our pool exists
   if(!isObject(GameObjectPool))
   {
      new ArrayObject(GameObjectPool);  
   }
   
   //Next, we loop and generate our game objects, and add them to the pool
   for(%i=0; %i < %amount; %i++)
   {
      %go = spawnGameObject(%name, false);
      
      //When our object is in the pool, it's not "real", so we need to make sure 
      //that we don't ghost it to clients untill we actually spawn it.
      %go.clearScopeAlways();
      
      //We also hide it, so that we don't 'exist' in the scene until we spawn
      %go.hidden = true;
      
      //Lastly, add us to the pool, with the key being our game object type
      GameObjectPool.add(%name, %go);
   }
}

function Entity::delete(%this)
{
   //we want to intercept the delete call, and add it to our GameObjectPool
   //if it's a game object
   if(%this.gameObjectAsset !$= "")
   {
      %this.setHidden(true);
      %this.clearScopeAlways();
      
      if(!isObject(GameObjectPool))
      {
         new ArrayObject(GameObjectPool);
      }
      
      GameObjectPool.add(%this.gameObjectAsset, %this);
      
      %missionSet = %this.getGroup();
      %missionSet.remove(%this);
   }
   else
   {
      %this.superClass.delete();
   }
}

function clearGameObjectPool()
{
   if(isObject(GameObjectPool))
   {
      %count = GameObjectPool.count();
      
      for(%i=0; %i < %count; %i++)
      {
         %go = GameObjectPool.getValue(%i);
         
         %go.superClass.delete();
      }
      
      GameObjectPool.empty();
   }
}
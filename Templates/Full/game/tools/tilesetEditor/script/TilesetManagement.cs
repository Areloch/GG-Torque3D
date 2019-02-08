function TilesetEditorPlugin::loadTilesInfo(%this)
{
   //we had a real file, time to parse it and load our shader graph
   %xmlDoc = new SimXMLDocument();
   
   TilesetEditor_TilesetsTree.clear();
   TilesetEditor_TilesTree.clear();
   
   //Load the tiles/tilesets
   TilesetEditor_TilesetsTree.insertItem(0, "Tilesets", "");
   TilesetEditor_TilesTree.insertItem(0, "Tiles", "");
   TilesetEditor_TilesTree.insertItem(1, "Placeholder_Floor", "");
   TilesetEditor_TilesTree.insertItem(1, "Placeholder_Wall", "");
   TilesetEditor_TilesTree.insertItem(1, "Placeholder_Ceiling", "");
   TilesetEditor_TilesTree.insertItem(1, "Placeholder_Full", "");
   
   %file = "tools/tilesetEditor/tilesetData.xml";
   
   if(!isFile(%file))
      return;
      
   if(%xmlDoc.loadFile(%file))
   {
      //<TilesetData>
      %xmlDoc.pushChildElement(0); 
      
      //TileElements
      if(%xmlDoc.pushFirstChildElement("TileElements"))
      {
         %fieldCount = 0;
         while(%xmlDoc.pushChildElement(%fieldCount))
         {
            %tileElementName = %xmlDoc.attribute("Name");
            %shapeFile = %xmlDoc.attribute("shapeFile");
            
            TilesetEditor_TilesTree.insertItem(1, %tileElementName, "");

            %xmlDoc.popElement();
            %fieldCount++;
         }
         
         %xmlDoc.popElement();
      }
      
      //TileSets
      if(%xmlDoc.pushFirstChildElement("TileSets"))
      {
         %stateCount = 0;
         while(%xmlDoc.pushChildElement(%stateCount))
         {
            %TilesetName = %xmlDoc.attribute("Name");
            
            %tilesetIdx = TilesetEditor_TilesetsTree.insertItem(1, %TilesetName, "");
   
            %setElementCount = 0;
            while(%xmlDoc.pushChildElement(%setElementCount))
            {
               %tilesetSubElement = %xmlDoc.attribute("TileElement");
               
               TilesetEditor_TilesetsTree.insertItem(%tilesetIdx, %tilesetSubElement, "");
               
               %tmp = true;
            }
            
            %xmlDoc.popElement();
            %stateCount++;         
         }
         %xmlDoc.popElement();
      }
   }
   
   %xmlDoc.popElement();
   
   %xmlDoc.delete();  
}

//Tiles/Tileset Management
function TilesetEditor_AddTilesetElement::onClick(%this)
{
   //TilesetEditorSettingsWindow-->TilesetManagementBook.selectPage(0);
   TilesetEditor_TilesTree.insertItem(1, "NewTileElement", "");
   TilesetEditor_TilesTree.buildVisibleTree(true);
}

function TilesetEditor_TilesTree::onSelect(%this, %item)
{
   %itemName = TilesetEditor_TilesTree.getItemText(%item);
   
   TilesetEditor_TilesProperties.startGroup("ElementProperties");
   TilesetEditor_TilesProperties.addField("Name", "Name", "string", "Name for this particular tile element", %itemName, "");
   
   TilesetEditor_TilesProperties.addField("ShapeFile", "Shape File", "shape", "Shape for this particular tile element", "", "");
   TilesetEditor_TilesProperties.addField("collidable", "Can Collide", "bool", "Does this tile element collide?", "", "");
   TilesetEditor_TilesProperties.endGroup();
}

singleton Material(door_tex_warehouse_windows)
{
   mapTo = "tex_warehouse_windows";
   diffuseMap[0] = "data/TheFactory/Images/Warehouse_Windows_d.dds";
   smoothness[0] = "1";
   metalness[0] = "10";
   translucentBlendOp = "LerpAlpha";
   specularMap[0] = "data/TheFactory/Images/Warehouse_Windows_s.dds";
   translucent = "1";
   alphaRef = "16";
   pixelSpecular0 = "0";
};

singleton Material(door_tex_structure_props)
{
   mapTo = "tex_structure_props";
   diffuseMap[0] = "data/TheFactory/Images/Structure_Props_d.dds";
   smoothness[0] = "1";
   metalness[0] = "10";
   translucentBlendOp = "None";
   normalMap[0] = "data/TheFactory/Images/Structure_Props_n.dds";
   specularMap[0] = "data/TheFactory/Images/Structure_Props_s.dds";
   pixelSpecular0 = "0";
};


singleton Material(sprinkler_01_tex_floor)
{
   mapTo = "tex_floor";
   diffuseMap[0] = "data/TheFactProjects/FPS Tutorial/game/art/shapes/GG_Graveyard/MausoleumTextures/Floor_d";
   smoothness[0] = "1";
   metalness[0] = "10";
   translucentBlendOp = "None";
};

singleton Material(fire_extinguisher_tex_misc_props)
{
   mapTo = "tex_misc_props";
   diffuseMap[0] = "Misc_Props_d";
   smoothness[0] = "1";
   metalness[0] = "10";
   translucentBlendOp = "None";
};

singleton Material(hvac_duct_straight_tex_ducts_pillars)
{
   mapTo = "tex_ducts_pillars";
   diffuseMap[0] = "data/TheFactory/Modern_warehouse/Textures/Ducts_Pillars_d";
   smoothness[0] = "1";
   metalness[0] = "10";
   translucentBlendOp = "None";
};

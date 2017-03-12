/*singleton CustomMaterial(PlaceholderTestMat)
{
   shader = MeshAssetShader;
   version = 3.0;
   
   sampler["diffuseMap"] = "art/ribbons/ribTex.png";
   sampler["bumpMap"] = "art/ribbons/ribTex.png";
   sampler["specularMap"] = "art/ribbons/ribTex.png";
   
   preload = true;
};*/

singleton Material(SmDamRoof1)
{
   mapTo = "SmDamRoof1";
   diffuseMap[0] = "art/radarMesh/SmDamRoof1.tga";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   diffuseColor[1] = "White";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   specularMap[0] = "art/radarMesh/materialLib/brick/brickComposite.png";
   pixelSpecular0 = "0";
};

singleton Material(wall141)
{
   mapTo = "wall141";
   diffuseMap[0] = "art/radarMesh/wall141.tga";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   specularMap[0] = "art/radarMesh/materialLib/brick/brickComposite.png";
   pixelSpecular0 = "0";
};

singleton Material(CorrugatedWall6)
{
   mapTo = "CorrugatedWall6";
   diffuseMap[0] = "art/radarMesh/CorrugatedWall6.tga";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   specularMap[0] = "art/radarMesh/materialLib/metal/metalComposite.dds";
   pixelSpecular0 = "0";
};

singleton Material(gr55)
{
   mapTo = "gr55";
   diffuseMap[0] = "art/radarMesh/gr55.tga";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   specularMap[0] = "art/radarMesh/materialLib/gravel/gravelComposite.dds";
   pixelSpecular0 = "0";
};

singleton Material(wood15)
{
   mapTo = "wood15";
   diffuseMap[0] = "art/radarMesh/wood15.tga";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   normalMap[0] = "art/radarMesh/materialLib/flatNormal.dds";
   effectColor[1] = "InvisibleBlack";
   specularMap[0] = "art/radarMesh/materialLib/brick/brickComposite.png";
   pixelSpecular0 = "0";
};

singleton Material(Radar_sky21_001)
{
   mapTo = "sky21.001";
   diffuseColor[0] = "0.64 0.64 0.64 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
};

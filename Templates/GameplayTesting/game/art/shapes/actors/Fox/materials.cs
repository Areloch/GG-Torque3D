
singleton Material(Fox_Material)
{
   mapTo = "Material";
   diffuseColor[0] = "0.8 0.8 0.8 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
};

singleton Material(Red)
{
   mapTo = "Red";
   diffuseColor[0] = "0.996078 0.988235 0.988235 1";
   smoothness[0] = "1";
   metalness[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "modules/Characters/Images/SkinAlbedo.png";
   normalMap[0] = "modules/Characters/Images/FlatNormal.png";
   specularMap[0] = "modules/Characters/Images/SkinComposite.png";
   pixelSpecular0 = "0";
   castDynamicShadows = "1";
};

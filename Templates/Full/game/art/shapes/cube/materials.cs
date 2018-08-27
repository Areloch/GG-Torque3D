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

//--- cube.dae MATERIALS BEGIN ---
/*singleton Material(cube_GridMaterial)
{
	mapTo = "GridMaterial";

	diffuseMap[0] = "grid";
	normalMap[0] = "";
	specularMap[0] = "";

	diffuseColor[0] = "1 1 1 1";
	specular[0] = "0.9 0.9 0.9 1";
	specularPower[0] = 0.415939;
	pixelSpecular[0] = false;
	emissive[0] = false;

	doubleSided = false;
	translucent = false;
	translucentBlendOp = "None";
};*/

/*singleton CustomShaderFeatureData(FlatColorFeature)
{
   
};*/

singleton Material(cube_GridMaterial)
{
   mapTo = "GridMaterial";
   
   CustomShaderFeature[0] = "Vi+ö";
   diffuseMap[0] = "core/art/grids/512_grey.png";
   //CustomShaderFeatureUniforms[FlatColorFeature,0] = "TestFloat";
};

//--- cube.dae MATERIALS END ---

//Voodoo!
function FlatColorFeature::processVertHLSL(%this)
{
   //%this.addVertTexCoord("texCoord2");
   %this.addConnector("outTexCoord2", "float4", "RT_TEXCOORD");
   
   // setup language elements to output incoming tex coords to output
   %this.writeLine("   //Connector test");
   %this.writeLine("   @ = float4(@,1);", "outTexCoord2", "position");
}

function FlatColorFeature::processPixelHLSL(%this)
{
   %this.addConnector("texCoord2", "float4", "RT_TEXCOORD");
   
   %this.addUniform("overrideColor", "float4");
   /*%this.addSampler("strudelMap");
   %this.addTexture("strudelTex", "Texture2D", "strudelMap");
   
   %this.addVariable("bobsyeruncle", "float", 15.915);
   %this.addVariable("chimmychanga", "float");
   
   %this.writeLine("   @ = @ * 2;", "chimmychanga", "bobsyeruncle");
   %this.writeLine("   @ *= @;", "bobsyeruncle", "strudel");
   %this.writeLine("   @ *= @;", "chimmychanga", "strudel");
   
   %this.writeLine("");
   
   %this.addVariable("sprangle", "float4");
   %this.writeLine("   @ = @.Sample(@,@);", "sprangle", "strudelTex", "strudelMap", "strudel");*/
   
   if(%this.hasFeature("MFT_isDeferred"))
      %col = "col1";
   else
      %col = "col";
      
   //%this.addVariable("testadoo", "float2");
   //%this.writeLine("   @ = @;", "testadoo", "texCoord2");
   
   //%this.writeLine("   @ = @;", %col, "texCoord2");
   
   //%this.writeLine("   @ = @;", %col, "overrideColor");
}

function FlatColorFeature::setTextureResources(%this)
{
   
}

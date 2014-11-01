if( !isObject( GuiMenuButtonProfile ) )
new GuiControlProfile( GuiMenuButtonProfile )
{
   opaque = true;
   border = false;
   fontSize = 18;
   fontType = "Arial Bold";
   fontColor = "50 50 50";
   fontColorHL = "0 0 0";
   fontColorNA = "200 200 200";
   //fontColorSEL ="0 0 0";
   fixedExtent = false;
   justify = "center";
   canKeyFocus = false;
	bitmap = "./images/selector-button";
   hasBitmapArray = false;
   category = "Core";
};

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

if( !isObject( GuiSolidDefaultProfile ) )
new GuiControlProfile (GuiSolidDefaultProfile)
{
   opaque = true;
   border = true;
   category = "Tools";
};

if( !isObject( GuiTransparentProfile ) )
new GuiControlProfile (GuiTransparentProfile)
{
   opaque = false;
   border = false;
   category = "Tools";
};

if( !isObject( GuiGroupBorderProfile ) )
new GuiControlProfile( GuiGroupBorderProfile )
{
   border = false;
   opaque = false;
   hasBitmapArray = true;
   bitmap = "data/ui/images/group-border";
   category = "Tools";
};

if( !isObject( GuiTabBorderProfile ) )
new GuiControlProfile( GuiTabBorderProfile )
{
   border = false;
   opaque = false;
   hasBitmapArray = true;
   bitmap = "data/ui/images/tab-border";
   category = "Tools";
};

if( !isObject( GuiToolTipProfile ) )
new GuiControlProfile (GuiToolTipProfile)
{
   // fill color
   fillColor = EditorSettings.value("Theme/tooltipBGColor");

   // border color
   borderColor   = EditorSettings.value("Theme/tooltipDivColor");

   // font
   fontType = "Noto Sans";
   fontSize = 14;
   fontColor = EditorSettings.value("Theme/tooltipTextColor");

   category = "Tools";
};

if( !isObject( GuiModelessDialogProfile ) )
new GuiControlProfile( GuiModelessDialogProfile )
{
   modal = false;
   category = "Tools";
};

if( !isObject( GuiFrameSetProfile ) )
new GuiControlProfile (GuiFrameSetProfile)
{
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   borderColor = "246 245 244";
   border = 1;
   opaque = true;
   border = true;
   category = "Tools";
};

if( !isObject( GuiWindowProfile ) )
new GuiControlProfile (GuiWindowProfile)
{
   opaque = false;
   border = 1;
   fillColor = EditorSettings.value("Theme/tabsColor");
   fillColorHL = EditorSettings.value("Theme/tabsColor");
   fillColorNA = EditorSettings.value("Theme/tabsColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/headerTextColor");
   bevelColorHL = "255 255 255";
   bevelColorLL = "0 0 0";
   text = "untitled";
   bitmap = "data/ui/images/window";
   textOffset = "10 4";
   hasBitmapArray = true;
   justify = "left";
   category = "Tools";
};

if( !isObject( GuiToolbarWindowProfile ) )
new GuiControlProfile(GuiToolbarWindowProfile : GuiWindowProfile)
{
      bitmap = "data/ui/images/toolbar-window";
      text = "";
      category = "Tools";
}; 

if( !isObject( GuiWindowCollapseProfile ) )
new GuiControlProfile (GuiWindowCollapseProfile : GuiWindowProfile)
{
   category = "Tools";
};

if( !isObject( GuiTextProfile ) )
new GuiControlProfile (GuiTextProfile)
{
   opaque = true;
   justify = "left";
   fontColor = EditorSettings.value("Theme/headerTextColor");
   category = "Tools";
};

if( !isObject( GuiTextBoldCenterProfile ) )
new GuiControlProfile (GuiTextBoldCenterProfile : GuiTextProfile)
{
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontType = "Noto Sans Bold";
   fontSize = 16;
   justify = "center";
   category = "Tools";
};

if( !isObject( GuiTextRightProfile ) )
new GuiControlProfile (GuiTextRightProfile : GuiTextProfile)
{
   justify = "right";
   category = "Tools";
};

if( !isObject( GuiTextCenterProfile ) )
new GuiControlProfile (GuiTextCenterProfile : GuiTextProfile)
{
   justify = "center";
   category = "Tools";
};

if( !isObject( GuiInspectorTitleTextProfile ) )
new GuiControlProfile (GuiInspectorTitleTextProfile)
{
   fontColor = EditorSettings.value("Theme/headerTextColor");
   category = "Tools";
};

if( !isObject( GuiAutoSizeTextProfile ) )
new GuiControlProfile (GuiAutoSizeTextProfile)
{
   fontColor = "215 215 215";
   autoSizeWidth = true;
   autoSizeHeight = true;   
   category = "Tools";
};

if( !isObject( GuiMLTextProfile ) )
new GuiControlProfile( GuiMLTextProfile )
{
   fontColorLink = "100 100 100";
   fontColorLinkHL = "255 255 255";
   autoSizeWidth = true;
   autoSizeHeight = true;  
   border = false;
   category = "Tools";
};

if( !isObject( GuiTextArrayProfile ) )
new GuiControlProfile( GuiTextArrayProfile : GuiTextProfile )
{
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextSELColor");
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");
   fillColorSEL = EditorSettings.value("Theme/fieldBGSELColor");
   border = false;
   category = "Tools";
};

if( !isObject( GuiTextListProfile ) )
new GuiControlProfile( GuiTextListProfile : GuiTextProfile ) 
{
   tab = true;
   canKeyFocus = true;
   category = "Tools";
};

if( !isObject( GuiTextEditProfile ) )
new GuiControlProfile( GuiTextEditProfile )
{
   opaque = true;
   bitmap = "data/ui/images/textEditFrame";
   hasBitmapArray = true; 
   border = -2; // fix to display textEdit img
   //borderWidth = "1";  // fix to display textEdit img
   //borderColor = "100 100 100";
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextSELColor");
   fontColorNA = "200 200 200";
   textOffset = "4 2";
   autoSizeWidth = false;
   autoSizeHeight = true;
   justify = "left";
   tab = true;
   canKeyFocus = true;   
   category = "Tools";
};

if( !isObject( GuiNumericTextEditProfile ) )
new GuiControlProfile( GuiNumericTextEditProfile : GuiTextEditProfile )
{
   numbersOnly = true;
   category = "Tools";
};

if( !isObject( GuiNumericDropSliderTextProfile ) )
new GuiControlProfile( GuiNumericDropSliderTextProfile : GuiTextEditProfile )
{
   bitmap = "data/ui/images/textEditSliderBox";
   category = "Tools";
};

if( !isObject( GuiRLProgressBitmapProfile ) )
new GuiControlProfile( GuiRLProgressBitmapProfile )
{
   border = false;
   hasBitmapArray = true;
   bitmap = "data/ui/images/rl-loadingbar";
   category = "Tools";
};

if( !isObject( GuiProgressTextProfile ) )
new GuiControlProfile( GuiProgressTextProfile )
{
   fontSize = "14";
	fontType = "Noto Sans";
   fontColor = "215 215 215";
   justify = "center";
   category = "Tools";   
};

if( !isObject( GuiButtonProfile ) )
new GuiControlProfile( GuiButtonProfile )
{
   opaque = true;
   border = true;
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fixedExtent = false;
   justify = "center";
   canKeyFocus = false;
	bitmap = "data/ui/images/button";
   hasBitmapArray = false;
   category = "Tools";
};

if( !isObject( GuiThumbHighlightButtonProfile ) )
new GuiControlProfile( GuiThumbHighlightButtonProfile : GuiButtonProfile )
{
   bitmap = "data/ui/images/thumbHightlightButton";
   category = "Tools";
};

if( !isObject( GuiIconButtonProfile ) )
new GuiControlProfile( GuiIconButtonProfile )
{
   opaque = true;
   border = true;
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fixedExtent = false;
   justify = "center";
   canKeyFocus = false;
	bitmap = "data/ui/images/iconbutton";
   hasBitmapArray = true;
   category = "Tools";
};

if( !isObject( GuiIconButtonSmallProfile ) )
new GuiControlProfile( GuiIconButtonSmallProfile : GuiIconButtonProfile )
{
   bitmap = "data/ui/images/iconbuttonsmall";
   category = "Tools";
};

if( !isObject( GuiEditorTabPage ) )
new GuiControlProfile(GuiEditorTabPage)
{
   opaque = true;
   border = false;
   fillColor = EditorSettings.value("Theme/tabsColor");
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   fixedExtent = false;
   justify = "left";
   canKeyFocus = false;
   bitmap = "data/ui/images/tab";
   hasBitmapArray = true;
   category = "Tools";
};

if( !isObject( GuiCheckBoxProfile ) )
new GuiControlProfile( GuiCheckBoxProfile )
{
   opaque = false;
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   border = false;
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   fontSize = 14;
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
	fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fixedExtent = true;
   justify = "left";
   bitmap = "data/ui/images/checkbox";
   hasBitmapArray = true;
   category = "Tools";
};

if( !isObject( GuiCheckBoxListProfile ) )
new GuiControlProfile( GuiCheckBoxListProfile : GuiCheckBoxProfile)
{
   bitmap = "data/ui/images/checkbox-list";
   category = "Tools";
};

if( !isObject( GuiCheckBoxListFlipedProfile ) )
new GuiControlProfile( GuiCheckBoxListFlipedProfile : GuiCheckBoxProfile)
{
   bitmap = "data/ui/images/checkbox-list_fliped";
   category = "Tools";
};

if( !isObject( GuiInspectorCheckBoxTitleProfile ) )
new GuiControlProfile( GuiInspectorCheckBoxTitleProfile : GuiCheckBoxProfile ){
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   category = "Tools";
};

if( !isObject( GuiRadioProfile ) )
new GuiControlProfile( GuiRadioProfile )
{
   fontSize = 14;
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fixedExtent = true;
   bitmap = "data/ui/images/radioButton";
   hasBitmapArray = true;
   category = "Tools";
};

if( !isObject( GuiScrollProfile ) )
new GuiControlProfile( GuiScrollProfile )
{
   opaque = true;
   fillColor = EditorSettings.value("Theme/tabsColor");
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   border = true;
   bitmap = "data/ui/images/scrollBar";
   hasBitmapArray = true;
   category = "Tools";
};

if( !isObject( GuiOverlayProfile ) )
new GuiControlProfile( GuiOverlayProfile )
{
   opaque = true;
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextGLColor");
   category = "Tools";
};

if( !isObject( GuiSliderProfile ) )
new GuiControlProfile( GuiSliderProfile )
{
   bitmap = "data/ui/images/slider";
   category = "Tools";
};

if( !isObject( GuiSliderBoxProfile ) )
new GuiControlProfile( GuiSliderBoxProfile )
{
   bitmap = "data/ui/images/slider-w-box";
   category = "Tools";
};

if( !isObject( GuiPopupMenuItemBorder ) )
new GuiControlProfile( GuiPopupMenuItemBorder : GuiButtonProfile )
{
   opaque = true;
   border = true;
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextGLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fixedExtent = false;
   justify = "center";
   canKeyFocus = false;
   bitmap = "data/ui/images/button";
   category = "Tools";
};

if( !isObject( GuiPopUpMenuDefault ) )
new GuiControlProfile( GuiPopUpMenuDefault : GuiDefaultProfile )
{
   opaque = true;
   mouseOverSelected = true;
   textOffset = "3 3";
   border = 0;
   borderThickness = 0;
   fixedExtent = true;
   bitmap = "data/ui/images/scrollbar";
   hasBitmapArray = true;
   profileForChildren = GuiPopupMenuItemBorder;
   fillColor = EditorSettings.value("Theme/fieldBGColor");//"255 255 255";//100
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");//"91 101 116";
   fillColorSEL = EditorSettings.value("Theme/fieldBGSELColor");//"91 101 116";
   // font color is black
   fontColor = EditorSettings.value("Theme/fieldTextColor");//"215 215 215";
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");//"215 215 215";
   fontColorSEL = EditorSettings.value("Theme/fieldTextSELColor");//"215 215 215";
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   category = "Tools";
};

if( !isObject( GuiPopUpMenuProfile ) )
new GuiControlProfile( GuiPopUpMenuProfile : GuiPopUpMenuDefault )
{
   textOffset         = "6 4";
   bitmap             = "data/ui/images/dropDown";
   hasBitmapArray     = true;
   border             = 1;
   profileForChildren = GuiPopUpMenuDefault;
   category = "Tools";
};

if( !isObject( GuiPopUpMenuTabProfile ) )
new GuiControlProfile( GuiPopUpMenuTabProfile : GuiPopUpMenuDefault )
{
   bitmap             = "data/ui/images/dropDown-tab";
   textOffset         = "6 4";
   canKeyFocus        = true;
   hasBitmapArray     = true;
   border             = 1;
   profileForChildren = GuiPopUpMenuDefault;
   category = "Tools";
};

if( !isObject( GuiPopUpMenuEditProfile ) )
new GuiControlProfile( GuiPopUpMenuEditProfile : GuiPopUpMenuDefault )
{
   textOffset         = "6 4";
   canKeyFocus        = true;
   bitmap             = "data/ui/images/dropDown";
   hasBitmapArray     = true;
   border             = 1;
   profileForChildren = GuiPopUpMenuDefault;
   category = "Tools";
};

if( !isObject( GuiListBoxProfile ) )
new GuiControlProfile( GuiListBoxProfile )
{
   fillColorHL = EditorSettings.value("Theme/windowBackgroundColor");
   fillColorNA = EditorSettings.value("Theme/windowBackgroundColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   
   tab = true;
   canKeyFocus = true;
   category = "Tools";
};

if( !isObject( GuiTabBookProfile ) )
new GuiControlProfile( GuiTabBookProfile )
{
   fillColorHL = EditorSettings.value("Theme/windowBackgroundColor");
   fillColorNA = EditorSettings.value("Theme/windowBackgroundColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fontType = "Noto Sans";
   fontSize = 14;
   justify = "center";
   bitmap = "data/ui/images/tab";
   tabWidth = 65;
   tabHeight = 25;
   tabPosition = "Top";
   tabRotation = "Horizontal";
   textOffset = "10 0";
   tab = true;
   cankeyfocus = true;
   category = "Tools";
};

if( !isObject( GuiTabBookNoBitmapProfile ) )
new GuiControlProfile( GuiTabBookNoBitmapProfile : GuiTabBookProfile )
{
   bitmap = "";
   category = "Tools";
};

if( !isObject( GuiTabPageProfile ) )
new GuiControlProfile( GuiTabPageProfile : GuiDefaultProfile )
{
   fontType = "Noto Sans";
   fontSize = 10;
   justify = "center";
   bitmap = "data/ui/images/tab";
   opaque = false;
   fillColor = "240 239 238";
   category = "Tools";
};

if( !isObject( GuiTreeViewProfile ) )
new GuiControlProfile( GuiTreeViewProfile )
{  
   bitmap = "data/ui/images/treeView";
   autoSizeHeight = true;
   canKeyFocus = true;
   fillColor = EditorSettings.value("Theme/windowBackgroundColor"); 
   fillColorHL = "116 116 116";
   fillColorSEL = "91 101 116";
   fillColorNA = "40 40 40";
   fontColor = "215 215 215";
   fontColorHL = "240 240 240";   
   fontColorSEL= "240 240 240";
   fontColorNA = "150 150 150";
   borderColor = "34 34 34";
   borderColorHL = "34 34 34";
   fontSize = 14;   
   opaque = false;
   border = false;
   category = "Tools";
};

if( !isObject( GuiTextPadProfile ) )
new GuiControlProfile( GuiTextPadProfile )
{
   fontType = ($platform $= "macos") ? "Monaco" : "Lucida Console";
   fontSize = ($platform $= "macos") ? 13 : 12;
   tab = true;
   canKeyFocus = true;
   
   // Deviate from the Default
   opaque=true;  
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");   
   border = 0;
   category = "Tools";
};

if( !isObject( GuiFormProfile ) )
new GuiControlProfile( GuiFormProfile : GuiTextProfile )
{
   opaque = false;
   border = 5;
   justify = "center";
   profileForChildren = GuiButtonProfile;
   opaque = false;
   hasBitmapArray = true;
   bitmap = "data/ui/images/button";
   category = "Tools";
};

// ----------------------------------------------------------------------------

singleton GuiControlProfile( GuiEditorClassProfile )
{
   opaque = true;
   fillColor = "232 232 232";
   border = 1;
   borderColor   = "42 42 42 140";
   borderColorHL = "127 127 127";
   fontColor = "215 215 215";
   fontColorHL = "50 50 50";
   fixedExtent = true;
   justify = "center";
   bitmap = "tools/gui/images/scrollBar";
   hasBitmapArray = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiBackFillProfile )
{
   opaque = true;
   fillColor = "0 94 94";
   border = true;
   borderColor = "255 128 128";
   fontType = "Noto Sans";
   fontSize = 12;
   fontColor = "215 215 215";
   fontColorHL = "50 50 50";
   fixedExtent = true;
   justify = "center";
   category = "Editor";
};

singleton GuiControlProfile( GuiControlListPopupProfile )
{
   opaque = true;
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   fillColorHL = "91 101 116";
   border = false;
   //borderColor = "0 0 0";
   fontColor = "215 215 215";
   fontColorHL = "240 240 240";
   fontColorNA = "50 50 50";
   textOffset = "0 2";
   autoSizeWidth = false;
   autoSizeHeight = true;
   tab = true;
   canKeyFocus = true;
   bitmap = "tools/gui/images/scrollBar";
   hasBitmapArray = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiSceneGraphEditProfile )
{
   canKeyFocus = true;
   tab = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorButtonProfile : GuiButtonProfile )
{
   //border = 1;
   justify = "Center";
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorSwatchButtonProfile )
{
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   borderColorNA = EditorSettings.value("Theme/dividerMidColor");
   fillColorNA = EditorSettings.value("Theme/fieldBGColor");
   borderColorHL = EditorSettings.value("Theme/dividerLightColor");
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorTextEditProfile )
{
   // Transparent Background
   opaque = true;
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");

   // No Border (Rendered by field control)
   border = false;

   tab = true;
   canKeyFocus = true;

   // font
   fontType = "Noto Sans";
   fontSize = 14;

   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextSELColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   category = "Editor";
};
singleton GuiControlProfile( GuiDropdownTextEditProfile :  GuiTextEditProfile )
{
   bitmap = "tools/gui/images/dropdown-textEdit";
   category = "Editor";
};
singleton GuiControlProfile( GuiInspectorTextEditRightProfile : GuiInspectorTextEditProfile )
{
   justify = "right";
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorGroupProfile )
{
   fontType    = "Noto Sans";
   fontSize    = "14";
   
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   
   justify = "left";
   opaque = false;
   border = false;
  
   bitmap = "tools/editorClasses/gui/images/rollout";
   
   textOffset = "20 0";

   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorFieldProfile)
{
   // fill color
   opaque = true;
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");
   fillColorNA = EditorSettings.value("Theme/fieldBGSELColor");

   // border color
   border = false;
   borderColor   = EditorSettings.value("Theme/dividerDarkColor");
   borderColorHL = EditorSettings.value("Theme/dividerMidColor");
   borderColorNA = EditorSettings.value("Theme/dividerLightColor");
   
   //bevelColorHL = "255 255 255";
   //bevelColorLL = "0 0 0";

   // font
   fontType = "Noto Sans";
   fontSize = 14;

   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   textOffset = "10 0";

   tab = true;
   canKeyFocus = true;
   category = "Editor";
};

/*
singleton GuiControlProfile( GuiInspectorMultiFieldProfile : GuiInspectorFieldProfile )
{
   opaque = true;
   fillColor = "50 50 230 30";
};
*/

singleton GuiControlProfile( GuiInspectorMultiFieldDifferentProfile : GuiInspectorFieldProfile )
{
   border = true;
   borderColor = EditorSettings.value("Theme/dividerMidColor");
};

singleton GuiControlProfile( GuiInspectorDynamicFieldProfile : GuiInspectorFieldProfile )
{
   // Transparent Background
   opaque = true;
   fillColor = EditorSettings.value("Theme/fieldBGColor");
   fillColorHL = EditorSettings.value("Theme/fieldBGHLColor");

   // No Border (Rendered by field control)
   border = false;

   tab = true;
   canKeyFocus = true;

   // font
   fontType = "Noto Sans";
   fontSize = 14;

   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextSELColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   category = "Editor";
};

singleton GuiControlProfile( GuiRolloutProfile )
{
   border = 0;
   borderColor = EditorSettings.value("Theme/dividerLightColor");
   
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   
   hasBitmapArray = true;
   bitmap = "tools/editorClasses/gui/images/rollout";
   
   textoffset = "17 0";
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorRolloutProfile0 )
{
   // font
   fontType = "Noto Sans";
   fontSize = 14;

   fontColor = "32 32 32";
   fontColorHL = "32 100 100";
   fontColorNA = "215 215 215";
   
   justify = "left";
   opaque = false;
   
   border = 0;
   borderColor   = "190 190 190";
   borderColorHL = "156 156 156";
   borderColorNA = "64 64 64";
  
   bitmap = "tools/editorclasses/gui/images/rollout_plusminus_header";
   
   textOffset = "20 0";
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorStackProfile )
{
   opaque = false;
   border = false;
   category = "Editor";
   
   fillColor = EditorSettings.value("Theme/tabsColor");
   fillColorHL = EditorSettings.value("Theme/tabsHLColor");
   
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
};

singleton GuiControlProfile( GuiInspectorProfile  : GuiInspectorFieldProfile )
{
   opaque = true;
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   border = 0;
   cankeyfocus = true;
   tab = true;
   category = "Editor";
};
singleton GuiControlProfile( GuiInspectorInfoProfile  : GuiInspectorFieldProfile )
{
   opaque = true;
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   border = 0;
   cankeyfocus = true;
   tab = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorBackgroundProfile : GuiInspectorFieldProfile )
{
   border = 0;
   cankeyfocus=true;
   tab = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorTypeFileNameProfile )
{
   // Transparent Background
   opaque = false;

   // No Border (Rendered by field control)
   border = 0;

   tab = true;
   canKeyFocus = true;

   // font
   fontType = "Noto Sans";
   fontSize = 14;
   
   // Center text
   justify = "center";

   fontColor = "240 240 240";
   fontColorHL = "240 240 240";
   fontColorNA = "215 215 215";

   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   fillColorHL = "91 101 116";
   fillColorNA = "244 244 244";

   borderColor   = "190 190 190";
   borderColorHL = "156 156 156";
   borderColorNA = "64 64 64";
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorColumnCtrlProfile : GuiInspectorFieldProfile )
{
   opaque = true;
   fillColor = "210 210 210"; 
   border = 0;
   category = "Editor";
};

singleton GuiControlProfile( InspectorTypeEnumProfile : GuiInspectorFieldProfile )
{
   mouseOverSelected = true;
   bitmap = "tools/gui/images/scrollBar";
   hasBitmapArray = true;
   opaque=true;
   border=true;
   textOffset = "4 0";
   category = "Editor";
};

singleton GuiControlProfile( InspectorTypeCheckboxProfile : GuiInspectorFieldProfile )
{
   bitmap = "tools/gui/images/checkBox";
   hasBitmapArray = true;
   opaque=false;
   border=false;
   textOffset = "4 0";
   category = "Editor";
};

singleton GuiControlProfile( GuiToolboxButtonProfile : GuiButtonProfile )
{
   justify = "center";
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   border = 0;
   textOffset = "0 0";   
   category = "Editor";
};

singleton GuiControlProfile( GuiDirectoryTreeProfile : GuiTreeViewProfile )
{
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorSEL= EditorSettings.value("Theme/fieldTextSELColor"); 
   fillColorHL = EditorSettings.value("Theme/fieldBGColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fontType = "Noto Sans";
   fontSize = 14;
   category = "Editor";
};

singleton GuiControlProfile( GuiDirectoryFileListProfile )
{
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorSEL= EditorSettings.value("Theme/fieldTextSELColor"); 
   fillColorHL = EditorSettings.value("Theme/fieldBGColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   fontType = "Noto Sans";
   fontSize = 14;
   category = "Editor";
};

singleton GuiControlProfile( GuiDragAndDropProfile )
{
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorFieldInfoPaneProfile )
{
   opaque = false;
   fillcolor = GuiInspectorBackgroundProfile.fillColor;
   borderColor = GuiDefaultProfile.borderColor;
   border = 1;
   category = "Editor";
};

singleton GuiControlProfile( GuiInspectorFieldInfoMLTextProfile : GuiMLTextProfile )
{
   opaque = false;   
   border = 0;   
   textOffset = "5 0";
   category = "Editor";
   
   fontColor = EditorSettings.value("Theme/fieldTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextSELColor");
};

singleton GuiControlProfile( GuiEditorScrollProfile )
{
   opaque = true;
   fillcolor = EditorSettings.value("Theme/windowBackgroundColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   border = 1;
   bitmap = "tools/gui/images/scrollBar";
   hasBitmapArray = true;
   category = "Editor";
};

singleton GuiControlProfile( GuiCreatorIconButtonProfile )
{
   opaque = true;       
   fillColor = "225 243 252 255";
   fillColorHL = "225 243 252 0";
   fillColorNA = "225 243 252 0";
   fillColorSEL = "225 243 252 0";
      
   //tab = true;
   //canKeyFocus = true;

   fontType = "Noto Sans";
   fontSize = 14;

   fontColor = "215 215 215";
   fontColorSEL = "43 107 206";
   fontColorHL = "244 244 244";
   fontColorNA = "100 100 100";
   
   border = 1;
   borderColor   = "153 222 253 255";
   borderColorHL = "156 156 156";
   borderColorNA = "153 222 253 0";
   
   //bevelColorHL = "255 255 255";
   //bevelColorLL = "0 0 0";
   category = "Editor";
};

singleton GuiControlProfile( GuiMenuBarProfile )
{
   fillColor = EditorSettings.value("Theme/headerColor");
   fillcolorHL = EditorSettings.value("Theme/tabsSELColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
   borderColorHL = EditorSettings.value("Theme/dividerMidColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorSEL = EditorSettings.value("Theme/fieldTextSELColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   fontColorNA = EditorSettings.value("Theme/fieldTextSELColor");
   border = 0;
   borderThickness = 1;
   opaque = true;
   mouseOverSelected = true;
   category = "Editor";
   bitmap = "tools/gui/images/checkbox-menubar";
};

singleton GuiControlProfile( ToolsMenubarProfile : GuiDefaultProfile ) 
{
   bitmap = "./menubar";
   category = "Editor";
   
   fillColor = EditorSettings.value("Theme/headerColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
};

singleton GuiControlProfile (menubarProfile) 
{
   opaque = false;
   border = -2;
   category = "Editor";
   
   bitmap = "./menubar";
   category = "Editor";
   
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
};

singleton GuiControlProfile (editorMenubarProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./editor-menubar";
   category = "Editor";
};
singleton GuiControlProfile (editorMenu_wBorderProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./menu-fullborder";
   category = "Editor";
};
singleton GuiControlProfile (inspectorStyleRolloutProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./inspector-style-rollout";
   category = "Editor";
};
singleton GuiControlProfile (inspectorStyleRolloutListProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./inspector-style-rollout-list";
   category = "Editor";
};
singleton GuiControlProfile (inspectorStyleRolloutDarkProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./inspector-style-rollout-dark";
   
   fillColor = EditorSettings.value("Theme/windowBackgroundColor");
   fontColor = EditorSettings.value("Theme/headerTextColor");
   fontColorHL = EditorSettings.value("Theme/fieldTextHLColor");
   borderColor = EditorSettings.value("Theme/dividerDarkColor");
};
singleton GuiControlProfile (inspectorStyleRolloutInnerProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./inspector-style-rollout_inner";
   category = "Editor";
};
singleton GuiControlProfile (inspectorStyleRolloutNoHeaderProfile)
{
   border = -2;
   category = "Editor";
   bitmap = "./inspector-style-rollout-noheader";
   category = "Editor";
};
singleton GuiControlProfile (IconDropdownProfile) 
{
   border = -2;
   category = "Editor";
   bitmap = "./icon-dropdownbar";
   category = "Editor";
};
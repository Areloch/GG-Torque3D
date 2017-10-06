//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//------------------------------------------------
function orderMove(%val)
{
  if (%val)
  {
    commandToServer('OrderMove');
  }
}
moveMap.bind(keyboard, y, orderMove);

//------------------------------------------------
function orderROE(%val)
{
  if (%val)
  {
    commandToServer('OrderROE');
  }
}
moveMap.bind(keyboard, o, orderROE);

//------------------------------------------------
function orderRegroup(%val)
{
  if (%val)
  {
    commandToServer('OrderRegroup');
  }
}
moveMap.bind(keyboard, t, orderRegroup);

//------------------------------------------------
function orderHold(%val)
{
  if (%val)
  {
    commandToServer('OrderHold');
  }
}
moveMap.bind(keyboard, i, orderHold);

//------------------------------------------------
function TAIKGuiToggle(%val)
{
  if (%val)
  {
    if (TAIKGui.isAwake())
    {
      Canvas.popDialog(TAIKGui);
    }
    else
    {
      Canvas.pushDialog(TAIKGui);
    }
  }
}
moveMap.bind( keyboard, "alt a", TAIKGuiToggle);
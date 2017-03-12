
singleton TSShapeConstructor(M4DAE)
{
   baseShape = "./M4.DAE";
};

function M4DAE::onLoad(%this)
{
   %this.setDetailLevelSize("-32", "32");
   %this.setDetailLevelSize("-64", "64");
   %this.setDetailLevelSize("-128", "128");
   %this.setDetailLevelSize("-256", "256");
   %this.setDetailLevelSize("-512", "512");
}

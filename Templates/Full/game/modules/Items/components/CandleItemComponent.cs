function CandleItemComponent::onAdd(%this)
{

}

function CandleItemComponent::onInteracted(%this, %user)
{
   //this shouldn't be hardcoded, but meh
   %light = %this.owner.getObject(0);
   
   %light.setLightEnabled(false);
}

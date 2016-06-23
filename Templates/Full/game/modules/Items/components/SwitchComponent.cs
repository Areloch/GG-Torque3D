function SwitchComponent::onAdd(%this)
{
   %this.addComponentField(targetEntity, "Entity to be activated by switch", "object", "0", "");
}


function SwitchComponent::onInteracted(%this)
{
   %this.targetEntity.notify("onInteracted");
}


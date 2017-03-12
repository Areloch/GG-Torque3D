function InteractableComponent::onAdd(%this)
{

}

function InteractableComponent::onRemove(%this)
{

}

function InteractableComponent::onClientConnect(%this, %client)
{

}

function InteractableComponent::onClientDisonnect(%this, %client)
{

}

function InteractableComponent::Update(%this)
{

}

function InteractableComponent::use(%this, %user)
{
   %this.owner.notify("onInteracted", %user);
}

function InteractableComponent::setHighlighted(%this, %isHighlighted)
{
   %this.highlighted = %isHighlighted;
   
   commandToClient(localclientConnection, 'SetClientItemHighlight', %isHighlighted);
}

function clientCmdSetClientItemHighlight(%highlighted)
{
   if(%highlighted)
   {
      centerPrintDlg.hidden = false;
      CenterPrintText.setText("AN OBJECT IS HIGHLIGHTED!");
   }
   else
   {
      centerPrintDlg.hidden = true;
   }
}
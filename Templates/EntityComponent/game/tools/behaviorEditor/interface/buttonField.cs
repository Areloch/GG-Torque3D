function buildButtonField(%name, %position, %extent)
{
    %container = new GuiControl();
	%fieldButton = new GuiButtonCtrl();

	//container->mProfile = "EditorContainerProfile";
	%container.position = %position;
	%container.extent = %extent;

	%fieldButton.position = "0 0";
	%fieldButton.extent = %extent;
	%fieldButton.internalName = "data";
	%fieldButton.setText(%name);

	%container.add(%fieldButton);

	return %container;
}
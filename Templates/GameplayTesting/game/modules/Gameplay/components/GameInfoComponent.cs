function GameInfoComponent::onAdd(%this)
{

}

function GameInfoComponent::onRemove(%this)
{

}

function GameInfoComponent::onGameStart(%this)
{
   allocateGameObjects(ArrowProjectile, 100);
}

function GameInfoComponent::onGameEnd(%this)
{
   clearGameObjectPool();
}

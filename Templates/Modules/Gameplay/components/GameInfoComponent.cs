function GameInfoComponent::onAdd(%this)
{

}

function GameInfoComponent::onRemove(%this)
{

}

function GameInfoComponent::onGameStart(%this)
{
   echo("Beginning Game!");
   
   allocateGameObjects(ArrowProjectile, 100);
}

function GameInfoComponent::onGameEnd(%this)
{
   clearGameObjectPool();
}

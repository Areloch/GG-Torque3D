#include "./aipathnode.h"
#include "./aipathgroup.h"
#include "T3D/missionMarker.h"

extern bool gEditingMission;

IMPLEMENT_CO_DATABLOCK_V1(AIPathNodeData);

IMPLEMENT_CO_NETOBJECT_V1(AIPathNode);

AIPathNode::AIPathNode(void): MissionMarker()
{
}

AIPathNode::~AIPathNode(void)
{
}

bool AIPathNode::onAdd()
{
   if(!Parent::onAdd() || !mDataBlock)
      return(false);

    return(true);
}

void AIPathNode::onRemove()
{
   //Con::executef(2,"ProperlyRemoveNode",scriptThis());
   Parent::onRemove();
}

bool AIPathNode::onNewDataBlock(GameBaseData * dptr, bool reload)
{
   mDataBlock = dynamic_cast<AIPathNodeData*>(dptr);
   
   if(!mDataBlock || !Parent::onNewDataBlock(dptr,reload)) // Issue?
      return(false);
   scriptOnNewDataBlock();
   
   return(true);
}




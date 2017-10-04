#pragma once
#include "./T3D/missionmarker.h"

class AIPathNodeData : public MissionMarkerData
{
   private:
      typedef MissionMarkerData Parent;
	public:
      DECLARE_CONOBJECT(AIPathNodeData);
};

class AIPathNode :
	public MissionMarker
{
private:
	typedef MissionMarker Parent;

   protected:
      enum MaskBits {
         PositionMask = Parent::NextFreeMask,
         NextFreeMask = Parent::NextFreeMask << 1
      };

      AIPathNodeData*        mDataBlock;
   
public:
    bool onNewDataBlock(GameBaseData *dptr, bool reload);
    bool onAdd();
    void onRemove();

	AIPathNode(void);
	~AIPathNode(void);

	DECLARE_CONOBJECT(AIPathNode);    
};

#ifndef _AIGROUP_H_
#define _AIGROUP_H_

#include "aipathnode.h"
#include "core/stream/filestream.h"
#include "./core/util/tvector.h"
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif



class SimGroup;

enum ADJTYPES {
	ALL,
	NONE,
	LOS,
	HEURISTIC,
	DISTANCE
};

enum NODETRAVERSALMETHODS {
	ASTAR,
	BESTFIRST,
	DIJKSTRA
};

struct AStarNode {
	U32 parentIndex;
	F32 F;
	F32 G;
	F32 H;
	bool open;
};

struct DijkstraNode {
	U32 parentIndex;
	F32 currentCost;
	bool open;
};


class AIPathGroup :
	public SimGroup
{
protected:
	bool mIsValidAdjData;
	bool mIsValidPathData;
	bool mRenderAllAdjs;

    F32* mAdjs;
	bool* mLastPath;
	S32* mPathData;
	U32 mNodeCount;

	void resetLastPath();

	void renderLine(AIPathNode* _obj1, AIPathNode* _obj2, bool _pathed);
	void expandArray(U32 _size);
	void shrinkArray(U32 _size, U32 _pos);
	void createNodesAdjs(U32 _index, ADJTYPES _heuristic, const char* _params);
	F32 calculateWeighting(U32 _index1, U32 _index2);
	bool LOSPass(U32 _index1, U32 _index2);
	bool distancePass(U32 _index1, U32 _index2, const char* _params);
	bool heuristicPass(U32 _index1, U32 _index2, const char* _params);
	U32 findNearestNode(Point3F _location, F32 _random);

	Vector<U32>* generatedPathsFind(U32 _startIndex, U32 _targetIndex);
	Vector<U32>* aStarFind(U32 _startIndex, U32 _targetIndex, F32 _random);
	F32 aStarHFunction(U32 _currentIndex, U32 _targetIndex);
	Vector<U32>* dijkstraFind(U32 _startIndex, U32 _targetIndex, F32 _random);
	Vector<U32>* bestFirstFind(U32 _startIndex, U32 _targetIndex, F32 _random);
	void bfRecursive(U32 _start, U32 _target, Vector<U32>* _stack, bool* _openClosedList, F32 _random);
	F32 bfCost(U32 _start, U32 _next, U32 _target);


public:
	typedef SimGroup Parent;

	void createAdjPair(SimObjectId _node1, SimObjectId _node2);
	void removeAdjPair(SimObjectId _node1, SimObjectId _node2);
	
	void createAdjs(ADJTYPES _how, const char* _params);
	bool loadAdjs(const char* fname);
	void saveAdjs(const char* fname);

	Vector<U32>* findPath(Point3F _start, Point3F _target, NODETRAVERSALMETHODS _method, F32 _random);
	bool loadPathData(const char* fname);
	void savePathData(const char* fname);
	void generatePathData(NODETRAVERSALMETHODS _method);
	
	void renderPaths(AIPathNode* _node);
	void setRenderMode(bool _all);

	void addObject(SimObject* obj);
	void addObjectSafe(SimObject* _obj, ADJTYPES _heuristic, const char* _params);
	void removeObjectSafe(SimObject* _obj); 
	S32 findObject(SimObject* _obj);

	AIPathGroup(void);
	~AIPathGroup(void);
	
	DECLARE_CONOBJECT(AIPathGroup);
};

#endif //_AIGROUP_H_

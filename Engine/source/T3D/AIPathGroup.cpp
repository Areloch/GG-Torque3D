#include "T3D/aipathgroup.h"
#include <string.h>
#include "math/mRandom.h"
#include "./aipathnode.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "T3D/gamebase/gameConnection.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/sim/debugDraw.h"

IMPLEMENT_CONOBJECT(AIPathGroup);


AIPathGroup::AIPathGroup(void) : SimGroup()
{
	mIsValidAdjData=false;
	mRenderAllAdjs=false;
	mIsValidPathData=false;
	mAdjs=NULL;
	mNodeCount=0;
	mLastPath=NULL;
	mPathData=NULL;

	SimObject* temp=new SimObject();
	objectList.push_back(temp);
	objectList.remove(temp);
	delete temp;
}

void AIPathGroup::setRenderMode(bool _all)
{
	mRenderAllAdjs=_all;
}

AIPathGroup::~AIPathGroup(void)
{
   lock();
   for (iterator itr = begin(); itr != end(); itr++)
      mNameDictionary.remove(*itr);

   objectList.sortId();
   while (!objectList.empty()) 
   {
      delete objectList.last();
      objectList.decrement();
   }

   unlock();
}

Vector<U32>* AIPathGroup::generatedPathsFind(U32 _startIndex, U32 _targetIndex)
{
	Vector<U32>* pathList=new Vector<U32>;
	U32 index=_startIndex;
	
	do
	{
		pathList->push_front(index);
		index=mPathData[_targetIndex*mNodeCount+index];
	}
	while (pathList->front() != _targetIndex);
	
	return pathList;
}
Vector<U32>* AIPathGroup::findPath(Point3F _start, Point3F _target, NODETRAVERSALMETHODS _method, F32 _random)
{
	PROFILE_START(FINDPATH);

	if (!mIsValidAdjData)
	{
		Con::errorf("AIPathGroup::findPath -- Adjaceny matrix invalid --see createAdjs(...)");
		PROFILE_END();	//findpath
		return NULL;
	}

	PROFILE_START(FINDNEARESTNODES);
	U32 startIndex=findNearestNode(_start, _random);
	U32 targetIndex=findNearestNode(_target, 0);
	PROFILE_END();

	Vector<U32>* path=NULL;

	if ((startIndex==-1) || (targetIndex==-1))
	{
		// this is regarded as console spam now
		// since most areas do not have nodes...
		//Con::errorf("AIPathGroup::FindPath - Cannot find suitable nodes near start or target locations)");
		PROFILE_END();	//findpath
		return NULL;
	}
	
	if(mIsValidPathData)
	{
		PROFILE_START(SAVED_DATA_PATHFINDING);
		path=generatedPathsFind(startIndex, targetIndex);
		PROFILE_END();
	}
	else
	{
		PROFILE_START(PATHFINDING);
		switch (_method)
		{
			case ASTAR:  path=aStarFind(startIndex, targetIndex, _random);
			break;
			case BESTFIRST: path=bestFirstFind(startIndex, targetIndex, _random);
			break;
			case DIJKSTRA: path=dijkstraFind(startIndex, targetIndex, _random);
			break;
		}
		PROFILE_END(); //pathfinding
	}
	
	if (!path) 
	{
		PROFILE_END(); //findpath
		return NULL;
	}

	resetLastPath();

	Vector<U32>* idList=new Vector<U32>;
	Vector<U32>::iterator iter=path->begin();

	U32 prevIndex=-1;
	
	while (iter != path->end())
	{
		U32 index=*iter;
		idList->push_front(objectList[index]->getId());
		
		if (prevIndex !=-1)
		{
			mLastPath[prevIndex*mNodeCount+index]=true;
			mLastPath[index*mNodeCount+prevIndex]=true;
		}
		prevIndex=index;
		
		iter++;
	}

	path->clear();
	delete path;

	PROFILE_END(); //findpath
	return idList;
}
void AIPathGroup::resetLastPath()
{
	bool* temp=mLastPath;
	U32 size=mNodeCount*mNodeCount;
	mLastPath=new bool[size];
	for (int i=0; i<size; i++)
		mLastPath[i]=false;

	if (temp) 
		delete[] temp;
}

F32 AIPathGroup::aStarHFunction(U32 _currentIndex, U32 _targetIndex)
{
	ShapeBase* startNode=(ShapeBase*)objectList[_currentIndex];
	ShapeBase* targetNode=(ShapeBase*)objectList[_targetIndex];

	Point3F vec=startNode->getBoxCenter()-targetNode->getBoxCenter();
	return vec.len(); 
}

Vector<U32>* AIPathGroup::aStarFind(U32 _startIndex, U32 _targetIndex, F32 _random)
{
	AStarNode* list=new AStarNode[mNodeCount];
	for (U32 i=0; i<mNodeCount; i++)
	{
		list[i].open=false;
	}

	Vector<U32> openList;
	openList.push_back(_startIndex);

	list[_startIndex].parentIndex=_startIndex;
	list[_startIndex].G=0;
	list[_startIndex].H=list[_startIndex].F=aStarHFunction(_startIndex, _targetIndex);

	F32 F;
	F32 G;
	F32 H;
	U32 currentNode;
	bool found=false;

	do
	{
		currentNode=openList.front();
		openList.pop_front();

		for (U32 j=0; j<mNodeCount; j++)
		{
			if ((mAdjs[currentNode*mNodeCount+j]>0) && (j != currentNode))
			{
				G=list[currentNode].G+mAdjs[currentNode*mNodeCount+j]+gRandGen.randF(0, _random);
				H=aStarHFunction(j, _targetIndex);
				F=G+H;
				
				if (!list[j].open)
				{
					list[j].parentIndex=currentNode;
					list[j].open=true;
					list[j].G=G;
					list[j].H=H;
					list[j].F=F;

					bool foundPos=false;
					U32 minIndex=0;
					U32 maxIndex=openList.size();
					S32 index=minIndex+(maxIndex-minIndex)/2;;
					
					if (openList.size()>0)
					{
						while (!foundPos)
						{
							if (list[openList[index]].F<=F)
								minIndex=index+1;
							else
								maxIndex=index;
							
							index=minIndex+(maxIndex-minIndex)/2;
						
							if (minIndex==maxIndex) foundPos=true;
						}
					}

                    openList.insert(index);
					openList[index]=j;
				}
				else
				if (G<list[j].G)
				{
					list[j].parentIndex=currentNode;
					list[j].G=G;
					list[j].F=G+H;
				}
			}
		}

		if (currentNode==_targetIndex)
			found=true;
	} 
	while ((openList.size()>0) && (!found));

	if (!found) return NULL; //orphaned

	Vector<U32>* pathList=new Vector<U32>;
	
	U32 currentIndex=_targetIndex;
	do
	{
		pathList->push_back(currentIndex);	
		currentIndex=list[currentIndex].parentIndex;
	}
	while (currentIndex != _startIndex);
	pathList->push_back(_startIndex); 

	delete[] list;
	return pathList;
}

Vector<U32>* AIPathGroup::dijkstraFind(U32 _startIndex, U32 _targetIndex, F32 _random)
{
	DijkstraNode* list=new DijkstraNode[mNodeCount];
	for (U32 i=0; i<mNodeCount; i++)
		list[i].open=false;
	

	Vector<U32> openList;
	openList.push_back(_startIndex);
	list[_startIndex].currentCost=0;
	list[_startIndex].parentIndex=_startIndex;
	F32 newCost;
	U32 currentNode;
	bool found=false;

	do
	{
		currentNode=openList.front();
		openList.pop_front();

		for (U32 j=0; j<mNodeCount; j++)
		{
			if ((mAdjs[currentNode*mNodeCount+j]>0) && (j != currentNode))
			{
				newCost=list[currentNode].currentCost+mAdjs[currentNode*mNodeCount+j]+gRandGen.randF(0, _random);
				
				if (!list[j].open)
				{
					list[j].parentIndex=currentNode;
					list[j].open=true;
					list[j].currentCost=newCost;
					openList.push_back(j);
				}
				else
				if (newCost<list[j].currentCost)
				{
					list[j].parentIndex=currentNode;
					list[j].currentCost=newCost;
				}
			}
		}

		if (currentNode==_targetIndex)
			found=true;
	} 
	while ((openList.size()>0) && (!found));

	if (!found) return NULL; //orphaned

	Vector<U32>* pathList=new Vector<U32>;

	U32 currentIndex=_targetIndex;
	do
	{
		pathList->push_back(currentIndex);	
		currentIndex=list[currentIndex].parentIndex;
	}
	while (currentIndex != _startIndex);
	pathList->push_back(_startIndex);

	delete[] list;
	return pathList;
}

Vector<U32>* AIPathGroup::bestFirstFind(U32 _startIndex, U32 _targetIndex, F32 _random)
{
	Vector<U32>*  pathStack=new Vector<U32>;
	bool* openList=new bool[mNodeCount];
	for (U32 i=0; i<mNodeCount; i++)
		openList[i]=true;

	bfRecursive(_startIndex, _targetIndex, pathStack, openList, _random);

	delete[] openList;
	return pathStack;
}

F32 AIPathGroup::bfCost(U32 _start, U32 _next, U32 _target)
{	
	ShapeBase* currentNode=(ShapeBase*)objectList[_start];
	ShapeBase* nextNode=(ShapeBase*)objectList[_next];
	ShapeBase* targetNode=(ShapeBase*)objectList[_target];

	Point3F currentVec=targetNode->getBoxCenter()-currentNode->getBoxCenter();
	Point3F nextVec=targetNode->getBoxCenter()-nextNode->getBoxCenter();
	
	
	F32 diffLen=currentVec.len()-nextVec.len();

	F32 cost=-diffLen*1/mAdjs[_start*mNodeCount+_next];

	return cost;
}

void AIPathGroup::bfRecursive(U32 _start, U32 _target, Vector<U32>* _stack, bool* _openClosedList, F32 _random)
{
	_stack->push_front(_start);
	if (_start == _target) return;
	
	bool isOpenAdj=true;

	while (isOpenAdj)
	{
		S32 currentIndex=-1;
		F32 currentWeight=0;
		F32 nextWeight=0;
		
		for (U32 i=0; i<mNodeCount; i++)
		{
			if ((_openClosedList[i]) && (mAdjs[_start*mNodeCount+i] != -1) && (currentIndex != _start))
			{
				nextWeight=bfCost(_start, i, _target)+gRandGen.randF(0, _random);

				if ((nextWeight<currentWeight) || (currentIndex==-1)) 
				{
					currentIndex=i;
					currentWeight=nextWeight;
				}
			}
		}

		if (currentIndex==-1)
			isOpenAdj=false;
		else
		{
			_openClosedList[_start]=false;
			bfRecursive(currentIndex, _target, _stack, _openClosedList, _random);
			if (_stack->front() != _start) return;
		}

	}

	_stack->pop_front();
	_openClosedList[_start]=false;
}

F32 AIPathGroup::calculateWeighting(U32 _index1, U32 _index2) {
	//this can easily be improved upon
	//currently it just uses the total distance between the two nodes
	//as the weighting value

	ShapeBase* obj1=(ShapeBase*)objectList[_index1];
	ShapeBase* obj2=(ShapeBase*)objectList[_index2];
		
	Point3F vector=obj1->getBoxCenter()-obj2->getBoxCenter();
	return vector.len();
}

void AIPathGroup::savePathData(const char* fname) {
	if (!mIsValidPathData)
	{
		Con::errorf("AIPathGroup::SavePathData - Cannot write Path data file without first generating path data --see createPathData(...)/loadPathData(...)");
		return;
	}
	
	FileStream file;
	file.open(fname, Torque::FS::File::Write);
	
	U32 size=mNodeCount*mNodeCount;

	file.write(size);
	file.write(size*4, mPathData);

	file.close();
}

bool AIPathGroup::loadPathData(const char* fname) {
	FileStream file;

	//AssertFatal(file.open(fname, FileStream::AccessMode::Read), "Error, Path data File not found");
	if (!file.open(fname, Torque::FS::File::Read))
	{
		Con::errorf("AIPathGroup::LoadPathData - Error, Path data File not found");
		file.close();
		return false;		
	}

	mNodeCount=size();
	U32 size=0;
	file.read(&size);
    
	if (size != mNodeCount*mNodeCount)
	{
		Con::errorf("AIPathGroup::LoadPathData - Path data file corruption");
		file.close();
		return false;
	}

	S32* buff=new S32[size];


	//AssertFatal(file.read(size*4, buff), "Error, Path data file corruption");
	if (!file.read(size*4, buff))
	{
		Con::errorf("AIPathGroup::LoadPathData - Error, Path data file corruption");
		file.close();
		return false;		
	}
	
	S32* temp=mPathData;
	mPathData=buff;

	if (temp) delete[] temp;

	mIsValidPathData=true;
	file.close();
	return true;

}


void AIPathGroup::generatePathData(NODETRAVERSALMETHODS _method) {
	if (!mIsValidAdjData)
	{
		Con::errorf("AIPathGroup::GeneratePathData - Cannot create Path data file without first generating adjacenicies --see createAdjs(...)/loadAdjs(...)");
		return;
	}

	Con::printf("Generating path data.....");
	U32 startTime=Platform::getRealMilliseconds();

	S32* temp=mPathData;
	U32 size=mNodeCount*mNodeCount;
	mPathData=new S32[size];
	if (temp)
		delete[] temp;

	for (U32 i=0; i<size; i++)
		mPathData[i]=-1;


	Vector<U32>* path=NULL;

	for (U32 start=0; start<mNodeCount; start++)
		for (U32 target=0; target<mNodeCount; target++)
			if (mPathData[target*mNodeCount+start] == -1)
			{
				switch (_method)
				{
					case ASTAR:  path=aStarFind(start, target, 0);	
					break;
					case BESTFIRST: path=bestFirstFind(start, target, 0);
					break;
					case DIJKSTRA: path=dijkstraFind(start, target, 0);
					break;
				}

				if (path)
				{
					U32 index;
					
					while (path->size()>1)
					{
						index=path->last();
						path->pop_back();
						mPathData[target*mNodeCount+index]=path->last();
					}
				
					path->clear();
					delete path;
					path=NULL;
				}
			}


	Con::printf(".....Finished");

	
	U32 totalTime=Platform::getRealMilliseconds()-startTime;
	Con::printf("Time taken: %u millisecs", totalTime);
	

	mIsValidPathData=true;
}

void AIPathGroup::addObject(SimObject* obj) {
   lock();

   // Make sure we aren't adding ourself.  This isn't the most robust check
   // but it should be good enough to prevent some self-foot-shooting.
   if(obj == this) {
      Con::errorf("SimGroup::addObject - (%d) can't add self!", getIdString());
      unlock();
      return;
   }
   

   if (obj->mGroup != this) {
	  if (obj->mGroup) {
         obj->mGroup->removeObject(obj);
	  }
      
	  mNameDictionary.insert(obj);
      obj->mGroup = this;
      objectList.push_back(obj); // force it into the object list
                                 // doesn't get a delete notify
	  
	  obj->onGroupAdd();
   }
   
   if (mIsValidAdjData) {
      Con::errorf("(IGNORE DURING LOADING) AIPathGroup::addObject - Do not use the editor to add objects as it will mess up existing adjaceny data ---See addObjectSafe()");
      mIsValidAdjData=mIsValidPathData=false;
   }

   unlock();
}

S32 AIPathGroup::findObject(SimObject* _obj) {
	S32 index=0;
	SimSet::iterator i;
			
	for (i=begin(); i<end()+1; i++) {
		if (*(i)==_obj) return index;
		index++;
	}

	return -1;
}

void AIPathGroup::renderPaths(AIPathNode *_node) {
	if (!mIsValidAdjData) return;

	if (mRenderAllAdjs) {
		for (U32 i=0; i<mNodeCount-1; i++) {
			for (U32 j=i+1; j<mNodeCount; j++) {
				if (mAdjs[i*mNodeCount+j] != -1) {
					bool pathed=false;
					if (mLastPath) {
                        pathed=mLastPath[i*mNodeCount+j];
					}
					renderLine((AIPathNode *) objectList[i], (AIPathNode *) objectList[j], pathed);
				}
			}
		}
	} else {
		U32 i=findObject(_node);
		for (U32 j=0; j<mNodeCount; j++) {
			if (mAdjs[i*mNodeCount+j] != -1) {
				renderLine((AIPathNode *) objectList[i], (AIPathNode *) objectList[j], false);
			}
		}
	}
}

U32 AIPathGroup::findNearestNode(Point3F _location,F32 _random) {
	U32 index=-1;
	F32 closestDist=0;

	for (U32 i=0; i<mNodeCount; i++)
	{
		ShapeBase* obj=(ShapeBase*)objectList[i];
		Point3F target=obj->getBoxCenter();

		RayInfo rInfo;
		//Container * currentContainer =  &gServerContainer;
		SceneContainer* currentContainer = &gServerContainer;
	
		U32 mask=  StaticObjectType | StaticShapeObjectType | TerrainObjectType;
		bool pass=!currentContainer->castRay(_location, target, mask, &rInfo);
	
		if (pass)
		{
			Point3F vector=_location-target;
			F32 dist=vector.len()+gRandGen.randF(0, _random);

			if ((index ==-1) || (dist<closestDist))
			{
				index=i;
				closestDist=dist;
			}
		}
	}

	return index;
}


bool AIPathGroup::LOSPass(U32 _index1, U32 _index2) {
	ShapeBase* obj1=(ShapeBase*)objectList[_index1];
	ShapeBase* obj2=(ShapeBase*)objectList[_index2];
	
	Point3F pos1=obj1->getBoxCenter();
	Point3F pos2=obj2->getBoxCenter();

	RayInfo rInfo;
	//Container * currentContainer =  &gServerContainer;
	SceneContainer* currentContainer = &gServerContainer;
	
	U32 mask=  StaticObjectType | StaticShapeObjectType | TerrainObjectType;

	bool pass=!currentContainer->castRay(pos1, pos2, mask, &rInfo);
		
	if (pass)
		return true;

	return false;
}

bool AIPathGroup::distancePass(U32 _index1, U32 _index2, const char* _params) {
	ShapeBase* obj1=(ShapeBase*)objectList[_index1];
	ShapeBase* obj2=(ShapeBase*)objectList[_index2];
		
	Point3F vector=obj1->getBoxCenter()-obj2->getBoxCenter();
	F32 maxDistance;
	dSscanf(_params, "%g", &maxDistance);

	if (vector.len()<=maxDistance)
		return true;

	return false;
}

bool AIPathGroup::heuristicPass(U32 _index1, U32 _index2, const char* _params) {
	//this can be inproved as needed
	//currently just checks distance and LOS
	if ((distancePass(_index1, _index2, _params)) && (LOSPass(_index1, _index2)))
		return true;

	return false;
}

void AIPathGroup::createNodesAdjs(U32 _index, ADJTYPES _heuristic, const char* _params) {
	for (U32 i=0; i<mNodeCount; i++)
   {
		{
			bool pass=false;
			switch (_heuristic)
			{
				case LOS: pass=this->LOSPass(i,_index);
				break;
				case DISTANCE: pass=this->distancePass(i,_index,_params);
				break;
				case HEURISTIC: pass=heuristicPass(i, _index, _params);
				break;
				case ALL: pass=true;
				break;
				//default is none pass=false already
			}

			if ((pass) && (i!=_index))
			{
				F32 weight=calculateWeighting(i,_index);
				mAdjs[i*mNodeCount+_index]=weight;
				mAdjs[_index*mNodeCount+i]=weight;
			}
			else
			{
				mAdjs[i*mNodeCount+_index]=-1;
				mAdjs[_index*mNodeCount+i]=-1;
			}
		}
   }
}


void AIPathGroup::addObjectSafe(SimObject* _obj, ADJTYPES _heuristic, const char* _params) {
	lock();

   // Make sure we aren't adding ourself.  This isn't the most robust check
   // but it should be good enough to prevent some self-foot-shooting.
   if(_obj == this)
   {
      Con::errorf("SimGroup::addObject - (%d) can't add self!", getIdString());
      unlock();
      return;
   }
   

   if (_obj->mGroup != this) 
   {
      if (_obj->mGroup)
         _obj->mGroup->removeObject(_obj);
      
	  mNameDictionary.insert(_obj);
      _obj->mGroup = this;
      objectList.push_back(_obj); // force it into the object list
                                 // doesn't get a delete notify
	  
	  _obj->onGroupAdd();
   }

   unlock();
   
   if (mIsValidAdjData)
   {
      mNodeCount=size();
	   expandArray(mNodeCount-1);
	   createNodesAdjs(mNodeCount-1, _heuristic, _params);
   }
   else
      createAdjs(_heuristic, _params);

   mIsValidPathData=false;
   resetLastPath();
}


void AIPathGroup::renderLine(AIPathNode* _obj1, AIPathNode* _obj2, bool _pathed) {
	Point3F pos1=_obj1->getBoxCenter();
	Point3F pos2=_obj2->getBoxCenter();

	bool displayNonPathed = false;
	F32 maxdist = 80.0f;
	GameConnection* connection = GameConnection::getConnectionToServer();
	ShapeBase *obj = dynamic_cast<ShapeBase*>(connection->getControlObject());
	if ( obj ) {
		Point3F objpos = obj->getBoxCenter();
		VectorF v = pos1 - objpos;
		F32 vd = mSqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
		if (vd<=maxdist) {
			VectorF v = pos2 - objpos;
			F32 vd = mSqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
			if (vd<=maxdist) {
				displayNonPathed = true;
			}
		}
	}

	if ((!displayNonPathed) && (!_pathed))
		return;

	if (_pathed) 
	{
      //GFX->getDrawUtil()->drawLine(pos1,pos2,ColorI(0,255,0));
	   DebugDrawer *ddraw = DebugDrawer::get();
	   if ( ddraw )
	   {
		  ddraw->drawLine( pos1, pos2, ColorI(0,255,0,255) );
	   }
	} 
	else 
	{
      //GFX->getDrawUtil()->drawLine(pos1,pos2,ColorI(0,0,255));
	   DebugDrawer *ddraw = DebugDrawer::get();
	   if ( ddraw )
	   {
		  ddraw->drawLine( pos1, pos2, ColorI(0,0,255,255) );
	   }
	}
}

void AIPathGroup::createAdjs(ADJTYPES _how, const char* _params) {
	Con::printf("Generating path data.....");
	U32 startTime=Platform::getRealMilliseconds();

	mIsValidAdjData=true;
	F32* temp=mAdjs;
	mNodeCount=size();
	mAdjs=new F32[mNodeCount*mNodeCount];
	if (temp) delete[] temp;
		
	for (U32 i=0; i<mNodeCount; i++)
		createNodesAdjs(i, _how, _params);

	Con::printf(".....Finished");
	
	U32 totalTime=Platform::getRealMilliseconds()-startTime;
	Con::printf("Time taken: %u millisecs", totalTime);
}

void AIPathGroup::saveAdjs(const char* fname) {
	if (!mIsValidAdjData)
	{
		Con::errorf("AIPathGroup::SaveAdjs - Cannot write Adjs file without first generating Adjs --see createAdjs(...)/loadAdjs(...)");
		return;
	}
	
	FileStream file;
	file.open(fname, Torque::FS::File::Write);
	
	U32 size=mNodeCount*mNodeCount;

	file.write(size);
	file.write(size*4, mAdjs);

	file.close();
}


bool AIPathGroup::loadAdjs(const char* fname) {
	FileStream file;

	//AssertFatal(file.open(fname, FileStream::AccessMode::Read), "Error, Adj File not found");
	if (!file.open(fname, Torque::FS::File::Read))
   {
      Con::errorf("AIPathGroup::LoadAdjs - Adj file %s does not exist", fname);
		return false;
   }

   U32 expectedSize=size();
	mNodeCount=expectedSize;
	expectedSize*=expectedSize;
	U32 size=0;
	file.read(&size);
    
	if (size != expectedSize)
	{
		Con::errorf("AIPathGroup::LoadAdjs - Adj file corruption");
		Con::errorf("Expected: %i", expectedSize);
		Con::errorf("Got: %i", size);
		file.close();
		return false;
	}

	if (mAdjs) delete[] mAdjs;
	mAdjs=new F32[size];

	//AssertFatal(file.read(size, buff), "Error, Adj file corruption");
	file.read(size*4, mAdjs);
	
	mIsValidAdjData=true;

	file.close();
	return true;
}

void AIPathGroup::createAdjPair(SimObjectId _node1, SimObjectId _node2) {
	if (!mIsValidAdjData)
	{
		Con::errorf("AIPathGroup::creatAdjPair - Cannot create pairs without initializing matrix--see createAdjs(...)");
		return;
	}	

	S32 index1=findObject(Sim::findObject(_node1));
	S32 index2=findObject(Sim::findObject(_node2));

	if ((index1==-1) || (index2==-1))
	{
		Con::errorf("AIPathGroup::SaveAdjs - Cannot find one of the nodes");
		return;
	}
	else
	{
		F32 weight=calculateWeighting(index1, index2);
		mAdjs[index1*mNodeCount+index2]=weight;
		mAdjs[index2*mNodeCount+index1]=weight;
	}
}

void AIPathGroup::removeAdjPair(SimObjectId _node1, SimObjectId _node2) {
	if (!mIsValidAdjData)
	{
		Con::errorf("AIPathGroup::removeAdjPair - Cannot remove pairs without an initialized matrix--see createAdjs(...)");
		return;
	}	

	S32 index1=findObject(Sim::findObject(_node1));
	S32 index2=findObject(Sim::findObject(_node2));

	if ((index1==-1) || (index2==-1))
	{
		Con::errorf("AIPathGroup::SaveAdjs - Cannot find one of the nodes");
		return;
	}
	else
	{
		mAdjs[index1*mNodeCount+index2]=-1; //need to do a proper weighting
		mAdjs[index2*mNodeCount+index1]=-1;
	}
}

void AIPathGroup::expandArray(U32 _size) {
	U32 newSize=_size+1;
	F32* buff=new F32[newSize*newSize];
	
	for (U32 i=0; i<_size; i++)
	{
		for (U32 j=0; j<_size; j++)
			buff[i*newSize+j]=mAdjs[i*_size+j];
		
		buff[i*newSize+_size]=-1;
	}

	for (U32 i=0; i<newSize; i++)
		buff[_size*newSize+i]=-1;
	
    F32* temp=mAdjs;
	mAdjs=buff;
	delete[] temp;
	
	mNodeCount=newSize;
}

void AIPathGroup::shrinkArray(U32 _size, U32 _pos) {
	U32 newSize=_size-1;
	F32* buff=new F32[newSize*newSize];
	U32 row=0;

	for (U32 i=0; i<_size; i++)
	{
		if (i != _pos)
		{
			for (U32 j=0; j<_pos; j++)
				buff[row*newSize+j]=mAdjs[i*_size+j];

			for (U32 j=_pos; j<newSize; j++)
				buff[row*newSize+j]=mAdjs[i*_size+j+1];
			
			row++;
		}
	}
	
	F32* temp=mAdjs;
	mAdjs=buff;
	delete[] temp;

	mNodeCount=newSize;
}

void AIPathGroup::removeObjectSafe(SimObject* _obj) {
	U32 pos=findObject(_obj);

	if (mIsValidAdjData)
		shrinkArray(mNodeCount, pos);

	removeObject(_obj);
	SimGroup* misGrp=(SimGroup *)Sim::findObject("MissionGroup");
	misGrp->addObject(_obj);

	mIsValidPathData=false;
	resetLastPath();
}


ConsoleMethod(AIPathGroup, removeObjectSafe, void, 3, 3, "(objID)" "-- the id of the object to remove from the group") 
{
	SimObjectId id;
	dSscanf(argv[2], "%u", &id);
	SimObject* obj=Sim::findObject(id);

	object->removeObjectSafe(obj);
}

ConsoleMethod(AIPathGroup, addObjectSafe, void, 4, 5, "(objID, HEURISTIC, PARAMS)" "-- the id of the object to add to the path group, with details of the default adjs")
{
	SimObjectId id;
	dSscanf(argv[2], "%u", &id);
	SimObject* obj=Sim::findObject(id);

	ADJTYPES type;
	char* buff=new char[64];
	strcpy(buff, argv[3]);
	buff=dStrupr(buff);

	if (strcmp(buff, "ALL")==0) type=ALL;
	else
	if (strcmp(buff, "NONE")==0) type=NONE;
	else
	if (strcmp(buff, "DISTANCE")==0) type=DISTANCE;
	else
	if (strcmp(buff, "LOS")==0) type=LOS;
	else
	if (strcmp(buff, "HEURISTIC")==0) type=HEURISTIC;
	else
	{
		Con::errorf("AIPathGroup::CreateAdj - invalid heuristic type");
		delete[] buff;
		return;
	}
	object->addObjectSafe(obj,type,argv[4]);
	delete[] buff;
}

ConsoleMethod(AIPathGroup, createAdjs, void, 3,4, "(heuristic, params)--- can be [ALL, NONE, LOS, DISTANCE, HEURISTIC]" "")
{
	ADJTYPES type;
	char* buff=new char[64];
	strcpy(buff, argv[2]);
	buff=dStrupr(buff);

	if (strcmp(buff, "ALL")==0) type=ALL;
	else
	if (strcmp(buff, "NONE")==0) type=NONE;
	else
	if (strcmp(buff, "DISTANCE")==0) type=DISTANCE;
	else
	if (strcmp(buff, "LOS")==0) type=LOS;
	else
	if (strcmp(buff, "HEURISTIC")==0) type=HEURISTIC;
	else

	{
		Con::errorf("AIPathGroup::CreateAdj - invalid heuristic type");
		delete[] buff;
		return;
	}

	object->createAdjs(type, argv[3]);
	delete[] buff;
}

ConsoleMethod(AIPathGroup, renderAll, void, 3,3, "(bool)" "--Set whether to render all adjs or just the ones from the selected object")
{
	bool val=false;
	
	if (!strcmp(argv[2], "1")) val=true;
	
	object->setRenderMode(val);
}

ConsoleMethod(AIPathGroup, saveAdjs, void, 3,3, "(char* fname)" "--Save the adjacency data to a file")
{
	object->saveAdjs(argv[2]);
}

ConsoleMethod(AIPathGroup, loadAdjs, bool, 3,3, "(char* fname)" "--Load the adjacency data from a file")
{
	return object->loadAdjs(argv[2]);
}

ConsoleMethod(AIPathGroup, savePathData, void, 3,3, "(char* fname)" "--Save the adjacency data to a file")
{
	object->savePathData(argv[2]);
}

ConsoleMethod(AIPathGroup, loadPathData, bool, 3,3, "(char* fname)" "--Load the adjacency data from a file")
{
	return object->loadPathData(argv[2]);
}

ConsoleMethod(AIPathGroup, createAdjPair, void, 4,4, "(ObjID1, ObjID2)" "--Create an adjanceny between two nodes")
{
	SimObjectId id1,id2;
	dSscanf(argv[2], "%u", &id1);
	dSscanf(argv[3], "%u", &id2);

	object->createAdjPair(id1, id2);
}

ConsoleMethod(AIPathGroup, removeAdjPair, void, 4,4, "(ObjID1, ObjID2)" "--Remove an adjanceny between two nodes")
{
	SimObjectId id1,id2;
	dSscanf(argv[2], "%u", &id1);
	dSscanf(argv[3], "%u", &id2);

	object->removeAdjPair(id1, id2);
}

ConsoleMethod(AIPathGroup, generatePathData, void, 3,3, "(pathfinding_method)" "--Pre-generate path data")
{
	char* buffer=new char[64];
	strcpy(buffer, argv[2]);
	buffer=dStrupr(buffer);
	NODETRAVERSALMETHODS pathFindType;

	if (strcmp(buffer, "ASTAR")==0) pathFindType=ASTAR;
	else
	if (strcmp(buffer, "BESTFIRST")==0) pathFindType=BESTFIRST;
	else
	if (strcmp(buffer, "DIJKSTRA")==0) pathFindType=DIJKSTRA;
	else
	{
		Con::errorf("AIPathGroup::generatePathData - invalid pathfinding type");
		delete[] buffer;
		return;
	}

	object->generatePathData(pathFindType);
	delete[] buffer;
}

ConsoleMethod(AIPathGroup, findPath, const char *, 5,6, "(StartPos, EndPos, PathMethod, RandomFactor)" "--Returns -1 if there is an error, else returns word spaced list of objectIDs (nodes)")
{
	Point3F start, end;
	dSscanf(argv[2], "%g %g %g", &start.x, &start.y, &start.z);
	dSscanf(argv[3], "%g %g %g", &end.x, &end.y, &end.z);
	
	char* buffer=new char[64];
	dStrcpy(buffer, argv[4]);
	buffer=dStrupr(buffer);	

	F32 randFact=0;
	dSscanf(argv[5], "%g", &randFact);
	
	NODETRAVERSALMETHODS pathFindType;

	if (dStrcmp(buffer, "ASTAR")==0) pathFindType=ASTAR;
	else
	if (dStrcmp(buffer, "BESTFIRST")==0) pathFindType=BESTFIRST;
	else
	if (dStrcmp(buffer, "DIJKSTRA")==0) pathFindType=DIJKSTRA;
	else
	{
		Con::errorf("AIPathGroup::FindPath - invalid pathfinding type");
		delete[] buffer;
		return "-1";
	}

	Vector<U32>* path=object->findPath(start, end, pathFindType, randFact);
	if (!path)
	{
		return "-1";
	}

	char* buff=Con::getReturnBuffer(path->size()*7);
	
	Vector<U32>::iterator iter=path->begin();
	char* id=new char[8];
	
	dSprintf(buff, 32, "%u ", *iter);
	iter++;	

	while(iter != path->end())
	{
		dSprintf(id, 32, "%u ", *iter);
		buff=strcat(buff, id);
		iter++;
	}
	path->clear();
	delete path;

	delete[] buffer;
	delete[] id;
	return buff;
}

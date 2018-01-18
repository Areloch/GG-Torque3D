#include "renderPipeline.h"

RenderPipeline::RenderPipeline()
{
	mPipelineName = StringTable->insert("NULL Pipeline");
}

void RenderPipeline::setupBuffers()
{
	mSupportGBuffer = true;

	//Albedo
	GBuffer::Target AlebdoTarget;
	AlebdoTarget.targetName = StringTable->insert("Albedo");
	if (!mGBuffer.setTargetChannels(&AlebdoTarget, GBuffer::RenderTargets::COLOR0,
		GBuffer::TargetChannels::R | GBuffer::TargetChannels::G | GBuffer::TargetChannels::B))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, AlebdoTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(AlebdoTarget);

	//Normal
	GBuffer::Target NormalTarget;
	NormalTarget.targetName = StringTable->insert("Normals");
	if (!mGBuffer.setTargetChannels(&NormalTarget, GBuffer::RenderTargets::COLOR1, GBuffer::TargetChannels::R | GBuffer::TargetChannels::G))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, NormalTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(NormalTarget);

	//Depth
	GBuffer::Target DepthTarget;
	DepthTarget.targetName = StringTable->insert("Normals");
	if (!mGBuffer.setTargetChannels(&DepthTarget, GBuffer::RenderTargets::COLOR1, GBuffer::TargetChannels::B | GBuffer::TargetChannels::A))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, DepthTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(DepthTarget);

	//Metalness
	GBuffer::Target MetalnessTarget;
	MetalnessTarget.targetName = StringTable->insert("Metalness");
	if (!mGBuffer.setTargetChannels(&MetalnessTarget, GBuffer::RenderTargets::COLOR2, GBuffer::TargetChannels::R))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, MetalnessTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(MetalnessTarget);

	//AO
	GBuffer::Target AOTarget;
	AOTarget.targetName = StringTable->insert("AO");
	if (!mGBuffer.setTargetChannels(&AOTarget, GBuffer::RenderTargets::COLOR2, GBuffer::TargetChannels::G))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, AOTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(AOTarget);

	//Roughness
	GBuffer::Target RoughnessTarget;
	RoughnessTarget.targetName = StringTable->insert("AO");
	if (!mGBuffer.setTargetChannels(&RoughnessTarget, GBuffer::RenderTargets::COLOR2, GBuffer::TargetChannels::B))
	{
		Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
			mPipelineName, RoughnessTarget.targetName);
		return;
	}
	mGBuffer.targets.push_back(RoughnessTarget);
}

//
void RenderPipeline::renderViews()
{
	//Walk through all our active cameras, and init a render from them
	for (;;)
	{
		render();
	}
}

void RenderPipeline::render()
{
	//Step 1 process all our render systems to draw the scene
	//RenderStaticMeshSytem::render();
	//RenderMeshSystem::render();

	//render any special snowflake non-e/c/s based classes
	//Scene::render();

	//Now that we've written all the visible geometry to our gBuffer, render lights
	//LightManager::renderLights();

	//Initiate our post process effects, starting with the deferred combine(as this mockup is currently written as Deferred)
	//PostFX::render();

	//Frame complete, ensure targets are resolved and that cleanup happens!
	//cleanup();
}
//

bool RenderPipeline::GBuffer::setTargetChannels(Target *_target, RenderPipeline::GBuffer::RenderTargets _renderTarget, U32 _channels)
{
	//find the RTData if it's already had channels assigned
	bool found = false;
	RenderTargetData* data = nullptr;

	for (U32 i = 0; i < rtData.size(); ++i)
	{
		if (rtData[i].id == _renderTarget)
			data = &rtData[i];
	}

	if (!found)
	{
		RenderTargetData newData;
		newData.id = _renderTarget;
		rtData.push_back(newData);

		data = &newData;
	}

	//Now that we have our data, find out if we have any overlap in the channels requested, if so, fail out
	if (data->channelsInUse & _channels)
	{
		//yep, failure
		return false;
	}
	else
	{
		data->channelsInUse |= _channels;

		_target->channels = _channels;
		_target->renderTarget = _renderTarget;

		return true;
	}
}
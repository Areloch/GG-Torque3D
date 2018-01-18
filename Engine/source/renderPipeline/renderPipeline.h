#pragma once
#include "console/engineAPI.h"

class RenderPipeline
{
protected:
	StringTableEntry mPipelineName;

	struct GBuffer
	{
		enum RenderTargets
		{
			COLOR0,
			COLOR1,
			COLOR2,
			COLOR3,
			COLOR4,
			COLOR5
		};

		enum TargetChannels
		{
			R = 0,
			G = 0 << 1,
			B = 0 << 2,
			A = 0 << 3,
		};

		struct RenderTargetData
		{
			RenderTargets id;
			U32 channelsInUse;
		};

		struct Target
		{
			U32 channels;
			StringTableEntry targetName;
			RenderTargets renderTarget;
		};

		Vector<RenderTargetData> rtData;
		Vector<Target> targets;

		U32 getTargetSize(U32 targetIndex);
		U32 getGBufferSize();

		//Used to validate our targets so we don't have multiple channels overlapped
		bool setTargetChannels(Target *_target, RenderTargets _renderTarget, U32 _channels);
	};

	bool mSupportGBuffer;
	GBuffer mGBuffer;

	//Lighting settings
	bool mSupportSSR;
	bool mSupportSSAO;
	bool mSupportHDR;

public:
	RenderPipeline();

	virtual void render();

	virtual void setupBuffers();
};
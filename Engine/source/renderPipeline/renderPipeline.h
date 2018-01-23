#pragma once
#include "console/engineAPI.h"

class RenderPipeline
{
protected:
	StringTableEntry mPipelineName;
   StringTableEntry mDescription;

   static RenderPipeline* smRenderPipeline;

	struct GBuffer
	{
		enum RenderTargets
		{
			TARGET0,
			TARGET1,
			TARGET2,
			TARGET3,
			TARGET4,
			TARGET5
		};

		enum TargetChannels
		{
			R2 = 0,
         R8 = 0 << 1,
         R16 = 0 << 2,
         R32 = 0 << 3,
         G2 = 0 << 4,
         G8 = 0 << 5,
         G16 = 0 << 6,
         G32 = 0 << 7,
         B2 = 0 << 8,
         B8 = 0 << 9,
         B16 = 0 << 10,
         B32 = 0 << 11,
         A2 = 0 << 12,
         A8 = 0 << 13,
         A16 = 0 << 14,
         A32 = 0 << 15,
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

   static RenderPipeline* getRenderPipeline()
   {
      if (smRenderPipeline == nullptr)
      {
         smRenderPipeline = new RenderPipeline();
      }

      return smRenderPipeline;
   }

   static bool supportsGBuffer()
   {
      if (smRenderPipeline == nullptr)
      {
         return false;
      }

      return smRenderPipeline->mSupportGBuffer;
   }
};
#include "renderPipeline/deferredPipeline/deferredRenderPipeline.h"

DeferredRenderPipeline::DeferredRenderPipeline()
{
   mPipelineName = StringTable->insert("Deferred");
   mDescription = StringTable->insert("Combined-pass Deferred Shading rendering");
}

void DeferredRenderPipeline::setupBuffers()
{
   /*mSupportGBuffer = true;

   //Albedo
   GBuffer::Target AlebdoTarget;
   AlebdoTarget.targetName = StringTable->insert("Albedo");
   if (!mGBuffer.setTargetChannels(&AlebdoTarget, GBuffer::RenderTargets::TARGET0,
      GBuffer::TargetChannels::R8 | GBuffer::TargetChannels::G8 | GBuffer::TargetChannels::B8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, AlebdoTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(AlebdoTarget);

   //Normal
   GBuffer::Target NormalTarget;
   NormalTarget.targetName = StringTable->insert("Normals");
   if (!mGBuffer.setTargetChannels(&NormalTarget, GBuffer::RenderTargets::TARGET1, GBuffer::TargetChannels::R8 | GBuffer::TargetChannels::G8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, NormalTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(NormalTarget);

   //Depth
   GBuffer::Target DepthTarget;
   DepthTarget.targetName = StringTable->insert("Normals");
   if (!mGBuffer.setTargetChannels(&DepthTarget, GBuffer::RenderTargets::TARGET1, GBuffer::TargetChannels::B8 | GBuffer::TargetChannels::A8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, DepthTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(DepthTarget);

   //Metalness
   GBuffer::Target MetalnessTarget;
   MetalnessTarget.targetName = StringTable->insert("Metalness");
   if (!mGBuffer.setTargetChannels(&MetalnessTarget, GBuffer::RenderTargets::TARGET2, GBuffer::TargetChannels::R8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, MetalnessTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(MetalnessTarget);

   //AO
   GBuffer::Target AOTarget;
   AOTarget.targetName = StringTable->insert("AO");
   if (!mGBuffer.setTargetChannels(&AOTarget, GBuffer::RenderTargets::TARGET2, GBuffer::TargetChannels::G8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, AOTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(AOTarget);

   //Roughness
   GBuffer::Target RoughnessTarget;
   RoughnessTarget.targetName = StringTable->insert("AO");
   if (!mGBuffer.setTargetChannels(&RoughnessTarget, GBuffer::RenderTargets::TARGET2, GBuffer::TargetChannels::B8))
   {
      Con::errorf("RenderPipeline - %s - Failied to set the GBuffer channel target %s! Channels already in use!",
         mPipelineName, RoughnessTarget.targetName);
      return;
   }
   mGBuffer.targets.push_back(RoughnessTarget);*/
}
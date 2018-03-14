new RenderPipeline(DeferredRenderPipeline)
{
   supportGbuffer = true;
   supportHDR = true;
   supportSSAO = true;
   supportSSR = false;
   
   renderPassManager = DiffuseRenderPassManager;
};

function DeferredRenderPipeline::setupBuffers(%this)
{
   echo("================================================");
   echo(%this.pipelineName @ " - setting up the buffers!");
   echo("================================================");
   
   %success = %this.setGBufferTarget("color", "deferred", "GFXFormatR16G16B16A16");
   
   %success = %this.setGBufferTarget("normal", "normal", "GFXFormatR16G16B16A16");
   %success = %this.setGBufferTarget("matinfo", "deferred", "GFXFormatR16G16B16A16");
   
   %success = %this.setGBufferTarget("lightInfo", "lightInfo", "GFXFormatR16G16B16A16F");
}

function DeferredRenderPipeline::setupRenderBins(%this)
{
   //Set up our bins
   //Core bins
   //new RenderDeferredMgr( DeferredBin );
   //new AdvancedLightBinManager( LightBin );
   
   %this.addRenderBin(new RenderObjectMgr(SkyBin) { bintype = "Sky";  });
   %this.addRenderBin(new RenderTerrainMgr(TerrainBin)   {  });
   %this.addRenderBin(new RenderMeshMgr(MeshBin)         { bintype = "Mesh";  });
   %this.addRenderBin(new RenderImposterMgr(ImposterBin) {  });
   %this.addRenderBin(new RenderObjectMgr(ObjectBin)     { bintype = "Object";  });
   %this.addRenderBin(new RenderMeshMgr(DecalRoadBin)    { bintype = "DecalRoad";  });
   %this.addRenderBin(new RenderMeshMgr(DecalBin)        { bintype = "Decal";  });
   %this.addRenderBin(new RenderOcclusionMgr(OccluderBin){ bintype = "Occluder";  });
   %this.addRenderBin(new RenderObjectMgr(ObjTranslucentBin) { bintype = "ObjectTranslucent";  });
   %this.addRenderBin(new RenderObjectMgr(WaterBin)          { bintype = "Water";  });
   %this.addRenderBin(new RenderObjectMgr(FoliageBin)        { bintype = "Foliage";  });
   %this.addRenderBin(new RenderParticleMgr(ParticleBin)    {  });
   %this.addRenderBin(new RenderTranslucentMgr(TranslucentBin){  });
   %this.addRenderBin(new RenderObjectMgr(FogBin){ bintype = "ObjectVolumetricFog";  }); 
   %this.addRenderBin(new RenderGlowMgr(GlowBin) {  });
   %this.addRenderBin(new RenderObjectMgr(EditorBin) { bintype = "Editor";  });
}

function DeferredRenderPipeline::setupPasses(%this)
{
   %this.addRenderPass(new RenderPassManager(GBufferRenderPass)
   {
      materialHook = "deferred";
      bufferTarget[0] = "color";
      bufferTarget[1] = "deferred";
      bufferTarget[2] = "matinfo";
   });
   
   GBufferRenderPass.addManager(TerrainBin);
   GBufferRenderPass.addManager(MeshBin);
   GBufferRenderPass.addManager(ObjectBin);
   GBufferRenderPass.addManager(DecalRoadBin);
   GBufferRenderPass.addManager(DecalBin);
   
   %this.addRenderPass(new RenderPassManager( LightRenderPass )
   {
      bufferTarget[0] = "lightInfo";
   });
   
   //LightRenderPass.addManager(LightBin);
   LightRenderPass.addManager(TerrainBin);
   LightRenderPass.addManager(MeshBin);
   LightRenderPass.addManager(ObjectBin);
   LightRenderPass.addManager(DecalRoadBin);
   LightRenderPass.addManager(DecalBin);
   
   %this.addRenderPass(new RenderPassManager( ForwardRenderPass )
   {
      forwardPass = true;
      bufferTarget[0] = "depth";
      bufferTarget[1] = "backbuffer";
   });
   
   ForwardRenderPass.addManager(SkyBin);
   ForwardRenderPass.addManager(TerrainBin);
   ForwardRenderPass.addManager(MeshBin);
   ForwardRenderPass.addManager(ImposterBin);
   ForwardRenderPass.addManager(ObjectBin);
   ForwardRenderPass.addManager(ShadowBin);
   ForwardRenderPass.addManager(DecalRoadBin);
   ForwardRenderPass.addManager(DecalBin);
	ForwardRenderPass.addManager(OccluderBin);
   ForwardRenderPass.addManager(ObjTranslucentBin);
   ForwardRenderPass.addManager(WaterBin);
   ForwardRenderPass.addManager(FoliageBin);
   ForwardRenderPass.addManager(ParticleBin);
   ForwardRenderPass.addManager(TranslucentBin);
   ForwardRenderPass.addManager(FogBin);   
   ForwardRenderPass.addManager(GlowBin);   
   ForwardRenderPass.addManager(EditorBin);   
}
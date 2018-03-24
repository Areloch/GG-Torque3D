new RenderPipeline(DeferredRenderPipeline)
{
   supportGbuffer = true;
   supportHDR = true;
   supportSSAO = true;
   supportSSR = false;
   supportPostEffects = true;
};

function DeferredRenderPipeline::setupBuffers(%this)
{
   echo("================================================");
   echo(%this.pipelineName @ " - setting up the buffers!");
   echo("================================================");
   
   %success = %this.addRenderTarget("backBuffer", "GFXFormatR8G8B8A8_SRGB");
   
   %success = %this.addRenderTarget("color", "GFXFormatR16G16B16A16");
   
   %success = %this.addRenderTarget("deferred", "GFXFormatR16G16B16A16");
   %success = %this.addRenderTarget("matinfo", "GFXFormatR16G16B16A16");
   
   %success = %this.addRenderTarget("lightInfo", "GFXFormatR16G16B16A16F");
}

function DeferredRenderPipeline::setupRenderBins(%this)
{
   //Set up our bins
   //Core bins
   %this.addRenderBin(new RenderDeferredMgr( DeferredBin ));
   %this.addRenderBin(new AdvancedLightBinManager( LightBin ));
   
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
   new RenderFormatToken(DeferredRPFormatToken)
   {
      enabled = "false";
      //When hdr is enabled this will be changed to the appropriate format
      format = "GFXFormatR8G8B8A8_SRGB";
      depthFormat = "GFXFormatD24S8";
      aaLevel = 0; // -1 = match backbuffer
      
      // The contents of the back buffer before this format token is executed
      // is provided in $inTex
      copyEffect = "AL_FormatCopy";
      
      // The contents of the render target created by this format token is
      // provided in $inTex
      resolveEffect = "AL_FormatCopy";
   };
   
   //GBuffer assembly pass
   new RenderPassManager(GBufferRenderPass);
   
   GBufferRenderPass.addRenderTarget("deferred");
   GBufferRenderPass.addRenderTarget("color");
   GBufferRenderPass.addRenderTarget("matinfo");
   
   GBufferRenderPass.addManager(DeferredBin);
   
   /*GBufferRenderPass.addManager(TerrainBin);
   GBufferRenderPass.addManager(MeshBin);
   GBufferRenderPass.addManager(ObjectBin);
   GBufferRenderPass.addManager(DecalRoadBin);
   GBufferRenderPass.addManager(DecalBin);*/
   
   GBufferRenderPass.addPostEffect(SSAOPostFx);
   
   %this.addRenderPass(GBufferRenderPass);
   
   //Lighting pass
   new RenderPassManager( LightRenderPass );
   
   LightRenderPass.addRenderTarget("lightInfo");
   
   LightRenderPass.addManager(LightBin);
   /*LightRenderPass.addManager(TerrainBin);
   LightRenderPass.addManager(MeshBin);
   LightRenderPass.addManager(ObjectBin);
   LightRenderPass.addManager(DecalRoadBin);
   LightRenderPass.addManager(DecalBin);*/
   
   LightRenderPass.addPostEffect(AL_DeferredShading);
   
   //%this.addRenderPass(LightRenderPass);
   
   //Forward Rendering pass
   new RenderPassManager( ForwardRenderPass )
   {
      forwardPass = true;
      formatToken = DeferredRPFormatToken;
   };
   
   ForwardRenderPass.addRenderTarget("backBuffer");
   //ForwardRenderPass.addRenderTarget("depth");
   
   ForwardRenderPass.addManager(SkyBin);
   ForwardRenderPass.addManager(TerrainBin);
   ForwardRenderPass.addManager(MeshBin);
   ForwardRenderPass.addManager(ImposterBin);
   ForwardRenderPass.addManager(ObjectBin);
   ForwardRenderPass.addManager(ShadowBin);
   ForwardRenderPass.addManager(DecalRoadBin);
   ForwardRenderPass.addManager(DecalBin);
   ForwardRenderPass.addManager(OccluderBin);
	
   ForwardRenderPass.addPostEffect(EdgeDetectPostEffect);
   ForwardRenderPass.addPostEffect(UnderwaterFogPostFx);
   ForwardRenderPass.addPostEffect(FogPostFx);
	
   ForwardRenderPass.addManager(ObjTranslucentBin);
   ForwardRenderPass.addManager(WaterBin);
   ForwardRenderPass.addManager(FoliageBin);
   ForwardRenderPass.addManager(ParticleBin);
   ForwardRenderPass.addManager(TranslucentBin);
   ForwardRenderPass.addManager(FogBin);   
   ForwardRenderPass.addManager(GlowBin);   
   
   ForwardRenderPass.addPostEffect(GlowPostFx);
   ForwardRenderPass.addPostEffect(VolFogGlowPostFx);
   ForwardRenderPass.addPostEffect(DOFPostEffect);
   ForwardRenderPass.addPostEffect(VignettePostEffect);
   ForwardRenderPass.addPostEffect(LightRayPostFX);
   ForwardRenderPass.addPostEffect(GammaPostFX);
   ForwardRenderPass.addPostEffect(HDRPostFX);
   
   ForwardRenderPass.addManager(EditorBin); 
   
   //%this.addRenderPass(ForwardRenderPass);
}
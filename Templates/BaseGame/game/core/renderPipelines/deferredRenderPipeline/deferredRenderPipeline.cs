new RenderPipeline(DeferredRenderPipeline)
{
   supportGbuffer = true;
   supportHDR = true;
   supportSSAO = true;
   supportSSR = false;
   
   shapeFile = "art/shapes/cube/cube.dae";
};

function DeferredRenderPipeline::setupBuffers(%this)
{
   echo("================================================");
   echo(%this.pipelineName @ " - setting up the buffers!");
   echo("================================================");
}
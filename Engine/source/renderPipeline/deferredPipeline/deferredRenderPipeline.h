#pragma once

#include "renderPipeline/renderPipeline.h"

class DeferredRenderPipeline : public RenderPipeline
{
protected:


public:
   DeferredRenderPipeline();

   virtual void setupBuffers();

   static RenderPipeline* initPipeline()
   {
      if (smRenderPipeline == nullptr)
      {
         smRenderPipeline = new DeferredRenderPipeline();
      }
      else
      {
         Con::errorf("Attempted to init the Deferred Render Pipeline, but a pipeline was already activated!");
      }

      return smRenderPipeline;
   }
};
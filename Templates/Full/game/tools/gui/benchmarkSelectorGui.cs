//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

function BenchmarkSelectorGui::onWake()
{
   BenchmarkPathList.clear();
   
   if(!isObject(Benchmark))
   {
      error("BenchmarSelectorGui::onWake, no Benchmarks group to get paths from!");
      return;
   }
   
   %pathCount = Benchmark.getCount();
   
   for(%i=0; %i < %pathCount; %i++)
   {
      %path = Benchmark.getObject(%i);    
      
      if(!%path.isMemberOfClass("Path"))
         continue;
         
      if(%path.getName() !$= "")
         %pathName = %path.getName();
      else
         %pathName = "Path(" @ %path.getId() @ ")";
         
      BenchmarkPathList.addRow(%i, %pathName);
   }
   BenchmarkPathList.setSelectedRow(0);
   BenchmarkPathList.scrollVisible(0);
}

function BenchmarkSelectorGui::StartSelectedBenchmark()
{
   // first unit is filename
   %sel = BenchmarkPathList.getSelectedId();
   %rowText = BenchmarkPathList.getRowTextById(%sel);
   
   //vouch the name
   if(getSubStr(%rowText, 0, 5) !$= "Path(")
   {
      //looks like a unique name, so just fetch that
      %objId = %rowText;
   }
   else
   {
      //unnamed path, so peel out the object id  
      %objId = getSubStr(%rowText, 5, strlen(%rowText)-6);
      
   }
   
   Canvas.popDialog(BenchmarkSelectorGui);
   
   Benchmark(%objId, BenchmarkRunCountTxt.getText(), BenchmarkOutputSummary.isStateOn(), BenchmarkOutputReport.isStateOn());
}
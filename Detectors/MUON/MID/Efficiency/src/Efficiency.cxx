//#include "MIDWorkflow/TrackReaderSpec.h"

#include "MIDEfficiency/Efficiency.h"


//#include "DPLUtils/RootTreeReader.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/Lifetime.h"
#include "Framework/Logger.h"
#include "Framework/Task.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "DataFormatsMID/Cluster.h"
#include "DataFormatsMID/Track.h"
#include "DataFormatsMID/MCClusterLabel.h"
#include <iostream>
#include <vector>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bits/stdc++.h>
#include <TFile.h>
#include <TH1F.h>

#include <Math/SMatrix.h>
#include <Math/SVector.h>

namespace o2
{
namespace mid
{

//_________________________________________________________________________________________________
/// run the efficiency algorithm

void Efficiency::process(gsl::span<const mid::Track>& midTracks){

int Nfired ;
int Ntot ;

for (auto& track : midTracks) 
{
  if (track.getEfficiencyFlag()==2) {

   auto deIdMT11 = track.getFiredDeId();

   for(int icath=0; icath<2; ++icath) {

     for(int ich=0; ich<4;++ich) {

         if (track.isFiredChamber(ich,icath)) {

          auto deId = deIdMT11 + 9*ich;

          mNFired [deId] = Nfired++;

          mVector.push_back(std::make_pair(deId, Nfired));
          
        }          

       // mNTot [deId] = Ntot++;
       }
     }
   } 
  }


} //void eff
} // namespace mid
} // namespace o2








#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include <gsl/span>
#include <TFile.h>

#include "Framework/CallbackService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/Task.h"
#include "Framework/Logger.h"
#include "CommonUtils/ConfigurableParam.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "CommonUtils/ConfigurableParam.h"
#include "DataFormatsMID/Track.h"
#include "MIDEfficiency/Efficiency.h"
#include "MIDWorkflow/TrackerSpec.h"
#include "MIDWorkflow/TrackReaderSpec.h"
#include "MIDWorkflow/EfficiencySpec.h"


namespace o2
{
namespace mid
{

using namespace o2::framework;


class EfficiencyTask 
{
 public:

   EfficiencyTask(bool useMC = false) : mUseMC(useMC) {}
  
  //_________________________________________________________________________________________________
  /// prepare the efficiency
  void init(InitContext& ic)
  {
    LOG(info) << "initializing efficiency";

    auto config = ic.options().get<std::string>("mid-eff");
    if (!config.empty()) {
      conf::ConfigurableParam::updateFromFile(config, "MIDEff", true);
    }

    auto stop = [this](){
    auto RPCEfficiency = mEfficiency.getNFired();
    auto TotFired = mEfficiency.getNTot();
    auto Vettore = mEfficiency.getVector();
  
    TFile fout;
    fout.Open("mid-efficiency.root", "RECREATE");
    fout.WriteObject(&RPCEfficiency, "RPCEfficiency");
    fout.WriteObject(&TotFired, "TotFired");
    fout.WriteObject(&Vettore, "VettoreProva");
    fout.Close(); 


    };
    ic.services().get<o2::framework::CallbackService>().set(o2::framework::CallbackService::Id::Stop, stop);

      
  }

  //_________________________________________________________________________________________________
  /// run the efficiency
  void run(ProcessingContext& pc)
  {
     auto midTracks = pc.inputs().get<gsl::span<mid::Track>>("midtracks");
     mEfficiency.process(midTracks);




  }

 private:
  Efficiency mEfficiency{};   
  bool mUseMC = false;              
}; 

//_________________________________________________________________________________________________
DataProcessorSpec getEfficiencySpec(const char* name)
{

  return DataProcessorSpec{
    "Efficiency",
    Inputs{InputSpec{"midtracks", "MID", "TRACKS", 0, Lifetime::Timeframe}},
  // return DataProcessorSpec{
   // name,
   // Inputs{InputSpec{"midtracks", "MID", "TRACKS", 0, Lifetime::Timeframe}},
    Outputs{},
    AlgorithmSpec{adaptFromTask<EfficiencyTask>()},
    Options{{"mid-eff", VariantType::String, "mid-efficiency.root", {"Root MID RPCs Efficiency"}}}}; 
   
}



} // namespace mid
} // namespace o2
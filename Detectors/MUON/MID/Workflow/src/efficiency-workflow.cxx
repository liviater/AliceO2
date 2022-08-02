#include <string>
#include <vector>
#include <unordered_map>
#include <TH1F.h>
#include <TFile.h>
#include "Framework/CompletionPolicyHelpers.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/ConfigContext.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/CallbacksPolicy.h"
#include "CommonUtils/ConfigurableParam.h"
#include "DetectorsRaw/HBFUtilsInitializer.h"
#include "MIDWorkflow/EfficiencySpec.h"
#include "DPLUtils/MakeRootTreeWriterSpec.h"
#include "DataFormatsMID/Track.h"
#include "MIDEfficiency/Efficiency.h"
#include "MIDWorkflow/TrackerSpec.h"
#include "MIDWorkflow/TrackReaderSpec.h"


using namespace o2::framework;

// ------------------------------------------------------------------
/*void customize(std::vector<o2::framework::CallbacksPolicy>& policies)
{
  o2::raw::HBFUtilsInitializer::addNewTimeSliceCallback(policies);
}

void customize(std::vector<CompletionPolicy>& policies)
{
  // ordered policies for the writers
  policies.push_back(CompletionPolicyHelpers::consumeWhenAllOrdered(".*(?:MUON|muon).*[W,w]riter.*"));
} */

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  // option allowing to set parameters
  workflowOptions.emplace_back("disable-root-input", VariantType::Bool, false,
                               ConfigParamSpec::HelpString{"disable root-files input reader"});
  workflowOptions.emplace_back("disable-root-output", VariantType::Bool, false,
                               ConfigParamSpec::HelpString{"do not write output root file"});
  workflowOptions.emplace_back("disable-mc", VariantType::Bool, false,
                               ConfigParamSpec::HelpString{"disable MC propagation even if available"});
  workflowOptions.emplace_back("configKeyValues", VariantType::String, "",
                               ConfigParamSpec::HelpString{"Semicolon separated key=value strings"});
  o2::raw::HBFUtilsInitializer::addConfigOption(workflowOptions);
}

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(const ConfigContext& configcontext)
{
  WorkflowSpec effspecs{};

  auto disableRootOutput = configcontext.options().get<bool>("disable-root-output");
  auto disableRootInput = configcontext.options().get<bool>("disable-root-input");
  auto useMC = !configcontext.options().get<bool>("disable-mc");
  
  o2::conf::ConfigurableParam::updateFromString(configcontext.options().get<std::string>("configKeyValues"));
 

  effspecs.emplace_back(o2::mid::getEfficiencySpec("mid-efficiency.root"));                                                
 
  
  // write the configuration used for the workflow
  o2::conf::ConfigurableParam::writeINI("o2mideff-workflow_configuration.ini");

  return effspecs;

  //return WorkflowSpec{o2::mid::getEfficiencySpec("mid-efficiency.root")};
}
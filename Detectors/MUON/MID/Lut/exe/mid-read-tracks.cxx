#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
// ROOT
#include "TFile.h"
#include "TTree.h"
// O2
#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsMID/ROFRecord.h"
#include "DataFormatsMID/Track.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "MIDSimulation/MCLabel.h"

#include "ReadTracks.h"


#include "boost/program_options.hpp"

namespace po = boost::program_options;
typedef std::chrono::high_resolution_clock myclock;


int main(int argc, char* argv[])
{
  std::string inFilenameO2 = "mid-tracks.root";
 // std::string inFilenameO2simKine = "o2sim_Kine.root";
 // std::string outFilename = "MIDReadTracks.root";
 

  po::options_description generic("Generic options");

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("o2-tracks-filename",po::value<std::string>(&inFilenameO2),"input o2 tracks");
       //   ("o2-sim-filename",po::value<std::string>(&inFilenameO2simKine),"input o2sim track");
      //    ("output", po::value<std::string>(&outFilename),"basename of output file");

  // po::options_description hidden("hidden options");
  // hidden.add_options()
  //         ("input", po::value<std::vector<std::string>>(),"converted track refs filename");
  // clang-format on

  po::options_description cmdline;
  cmdline.add(generic); //.add(hidden);

  // po::positional_options_description pos;
  // pos.add("input", -1);

  po::variables_map vm;
  // po::store(po::command_line_parser(argc, argv).options(cmdline).positional(pos).run(), vm);
  po::store(po::command_line_parser(argc, argv).options(cmdline).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: " << argv[0] << " [options]\n";
    std::cout << generic << std::endl;
    return 2;
  }


//Reconstructed tracks__________________________________________
//______________________________________________________________

  TFile* file = TFile::Open(inFilenameO2.c_str());
  TTree* tree = static_cast<TTree*>(file->Get("midtracklabels"));
  if (!tree) {
	std::cout << "Error: no tree found in" << inFilenameO2 << std::endl;
	return 0;
	}
  std::vector<o2::mid::ROFRecord>* rofRecords = nullptr;
  tree->SetBranchAddress("MIDTrackROF", &rofRecords);
 

  auto tracksBranch = tree->GetBranch("MIDTrack");
  std::vector<o2::mid::Track>* tracks = nullptr;
  tracksBranch->SetAddress(&tracks);

 
  o2::dataformats::MCTruthContainer<o2::MCCompLabel>* trackLabelsMC = nullptr;
  tree->SetBranchAddress("MIDTrackLabels", &trackLabelsMC);




//Simulated tracks______________________________________________
//______________________________________________________________
 // TFile* mcFile = TFile::Open(inFilenameO2simKine.c_str());
//  TTree* mcTree = static_cast<TTree*>(mcFile->Get("o2sim"));
//  std::vector<o2::MCTrackT<float>>* mcTracks = nullptr;
//  mcTree->SetBranchAddress("MCTrack", &mcTracks);


 o2::mid::ReadTracks readTracks;

//Loop
  for (auto ievt = 0; ievt < tree->GetEntries(); ++ievt) {
    tree->GetEvent(ievt);


    readTracks.process(*tracks, *rofRecords, *trackLabelsMC);

   }  //ievt



  return 0;
}

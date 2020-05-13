// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.


#include "ReadTracks.h"

#include "TFile.h"
#include "TSystem.h"
#include <sstream>
#include <string>
#include <iostream>
#include "TTree.h"
#include <vector>
#include "THnBase.h"
#include "THnSparse.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "SimulationDataFormat/MCTrack.h"
#include "TChain.h"


namespace o2
{
namespace mid
{

ReadTracks::ReadTracks() : mHistos()
{
  /// Default constructor 
  initHistos();
}

void ReadTracks::initHistos()
{
  /// Initializes histograms

}


void ReadTracks::process(TTree& MCTree, const std::vector<Track>& o2tracks, const std::vector<ROFRecord>& rofRecord, const std::vector<MCTrackT<float>>& MCTracks, const dataformats::MCTruthContainer<MCCompLabel>& trackLabelMC)
{
	/*const std::string path = "./";
	const std::string simfilename = "o2sim_Kine.root";
 	TChain mcTree("o2sim");
    mcTree.AddFile((path + simfilename).data());

	std::vector<o2::MCTrack> *MCTracks = nullptr;
  	mcTree.SetBranchAddress("MCTrack", &MCTracks); */


  const int ndimension = 4;
  int bins[ndimension] = {30, 30, 10, 10};
  double xmin[ndimension] = {-270.5, -300.5, -0.2, 0.};
  double xmax[ndimension] = {270.5, 300.5, 0.2, 8.}; 


  THnSparse* Histos = new THnSparseD("x-y-yDir-pt", "x-y-yDir-pt", ndimension, bins, xmin, xmax);
    
   
  for (auto& rof : rofRecord) {
   	  for (size_t itrack = rof.firstEntry; itrack < rof.firstEntry + rof.nEntries; ++itrack) {
   	    auto labels = trackLabelMC.getLabels(itrack);
        auto o2tr = o2tracks[itrack];
        if (labels.size() == 1) {
		   for (auto& label : labels) {
		   	 if ( !label.isSet() ) {
           		continue;
         	  }
            if(MCTree.GetReadEntry() != label.getEventID()){
         	 MCTree.GetEntry(label.getEventID());
            }
       
          auto mcTrack = MCTracks[label.getTrackID()];
        
          double xPos = o2tr.getPositionX();
          std::cout << "X:" << xPos << std::endl;
          
          double yPos = o2tr.getPositionY();
          std::cout << "Y:" << yPos << std::endl;

          double yDir = o2tr.getDirectionY();
          std::cout << "Y direction:" << yDir << std::endl;                       

          double p_t = mcTrack.GetPt();
          std::cout  << "pt:" << p_t << std::endl; 

       
         double ff[ndimension] = {xPos, yPos, yDir, p_t};
         // Histos->Rebin(2);
          Histos->GetAxis(0)->SetTitle("x position (cm)");
          Histos->GetAxis(1)->SetTitle("y position (cm)");
          Histos->GetAxis(2)->SetTitle("y direction");
          Histos->GetAxis(3)->SetTitle("p_t (GeV/c)");
          Histos->Fill(ff); 
                         
            
          } //for label
        } //if label
      } //itrack
     }  //rof

     TH2D* h2proj = Histos -> Projection(2, 3);
     h2proj->SetDirectory(0);
     h2proj->SetTitle("H(p_t, theta)");
     h2proj->GetXaxis()->SetTitle("p_t");
     h2proj->GetYaxis()->SetTitle("Theta");
    // h2proj->SetLineColor(kRed);
     h2proj->SetMarkerStyle(kDot);
     h2proj->SetMarkerSize(6);
     h2proj->Draw(); 

     int k = h2proj -> GetXaxis () -> FindBin(1);
     std::cout << "k: " << k << std::endl;

     TH1D* projectionTheta = h2proj -> TH2::ProjectionY("Theta_integrato", k, k+1);

    
	 TFile Istogrammi("MIDReadTracks.root", "recreate");
 
     Histos -> Write();
     h2proj->Write();
     projectionTheta->Write();

     Istogrammi.Write();
     Istogrammi.Close(); 

}




} // namespace mid
} // namespace o2

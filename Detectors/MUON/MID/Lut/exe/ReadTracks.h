// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.



#ifndef O2_MID_READTRACKS_H
#define O2_MID_READTRACKS_H

#include "DataFormatsMID/Track.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsMID/ROFRecord.h"
#include "TH1.h"
#include "TH2.h"
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "THnSparse.h"
#include "TChain.h"

namespace o2
{
namespace mid
{

/// Class to read the traks data for MID
class ReadTracks 
{
 public:
  ReadTracks();
 

  void process(const std::vector<Track>& o2tracks, const std::vector<ROFRecord>& rofRecords, const dataformats::MCTruthContainer<MCCompLabel>& trackLabelMC);

//  bool saveResults(const char* filename) const;


 private:
  void initHistos();

  std::vector<THnSparseD> mHistos; /// Output histograms


};

} // namespace mid
} // namespace o2

#endif /* O2_MID_READTRACKS_H */

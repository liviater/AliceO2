#ifndef O2_MID_EFFICIENCY_H_
#define O2_MID_EFFICIENCY_H_

#include <vector>
#include<bits/stdc++.h>
#include <unordered_map>

#include <gsl/span>

#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsMID/ROFRecord.h"
#include "DataFormatsMID/Track.h"
#include "ReconstructionDataFormats/TrackMCHMID.h"

namespace o2
{
namespace mid
{

/// Class Eff
class Efficiency
{
 public:
  Efficiency() = default;
  ~Efficiency() = default;

  void init();
  void process(gsl::span<const mid::Track>& midTracks);

  /// get MID tracks
  const std::unordered_map<int, uint64_t>& getNFired() const { return mNFired; }
  const std::unordered_map<int, uint64_t>& getNTot() const { return mNTot; }
  const std::vector<std::pair <int,uint64_t>>& getVector() const {return mVector;}


 private:

   std::unordered_map<int, uint64_t> mNFired{}; 
   std::unordered_map<int, uint64_t> mNTot{};
   std::vector<std::pair <int,uint64_t>> mVector{};
};

} // namespace mid
} // namespace o2

#endif // O2_MID_EFFICIENCY_H_

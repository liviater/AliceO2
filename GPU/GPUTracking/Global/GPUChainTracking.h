// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GPUChainTracking.h
/// \author David Rohr

#ifndef GPUCHAINTRACKING_H
#define GPUCHAINTRACKING_H

#include "GPUChain.h"
#include "GPUReconstructionHelpers.h"
#include "GPUDataTypes.h"
#include <atomic>
#include <array>

namespace o2
{
namespace trd
{
class TRDGeometryFlat;
}
} // namespace o2

namespace o2
{
namespace tpc
{
struct ClusterNativeAccess;
struct ClusterNative;
} // namespace tpc
} // namespace o2

namespace o2
{
namespace base
{
class MatLayerCylSet;
}
} // namespace o2

namespace GPUCA_NAMESPACE
{
namespace gpu
{
//class GPUTRDTrackerGPU;
class GPUTPCGPUTracker;
class GPUDisplay;
class GPUQA;
class GPUTPCClusterStatistics;
class GPUTRDGeometry;
class TPCFastTransform;
class TPCdEdxCalibrationSplines;
class GPUTrackingInputProvider;

class GPUChainTracking : public GPUChain, GPUReconstructionHelpers::helperDelegateBase
{
  friend class GPUReconstruction;

 public:
  ~GPUChainTracking() override;
  void RegisterPermanentMemoryAndProcessors() override;
  void RegisterGPUProcessors() override;
  int Init() override;
  int PrepareEvent() override;
  int Finalize() override;
  int RunChain() override;
  void MemorySize(size_t& gpuMem, size_t& pageLockedHostMem) override;

  // Structures for input and output data
  GPUTrackingInOutPointers& mIOPtrs;

  struct InOutMemory {
    InOutMemory();
    ~InOutMemory();
    InOutMemory(InOutMemory&&);
    InOutMemory& operator=(InOutMemory&&);

    std::unique_ptr<char[]> tpcZSpages;
    std::unique_ptr<GPUTrackingInOutZS> tpcZSmeta;
    std::unique_ptr<GPUTrackingInOutZS::GPUTrackingInOutZSMeta> tpcZSmeta2;
    std::unique_ptr<o2::tpc::Digit[]> tpcDigits[NSLICES];
    std::unique_ptr<GPUTPCClusterData[]> clusterData[NSLICES];
    std::unique_ptr<AliHLTTPCRawCluster[]> rawClusters[NSLICES];
    std::unique_ptr<o2::tpc::ClusterNative[]> clustersNative;
    std::unique_ptr<GPUTPCTrack[]> sliceTracks[NSLICES];
    std::unique_ptr<GPUTPCHitId[]> sliceClusters[NSLICES];
    std::unique_ptr<AliHLTTPCClusterMCLabel[]> mcLabelsTPC;
    std::unique_ptr<GPUTPCMCInfo[]> mcInfosTPC;
    std::unique_ptr<GPUTPCGMMergedTrack[]> mergedTracks;
    std::unique_ptr<GPUTPCGMMergedTrackHit[]> mergedTrackHits;
    std::unique_ptr<GPUTRDTrackletWord[]> trdTracklets;
    std::unique_ptr<GPUTRDTrackletLabels[]> trdTrackletsMC;
    std::unique_ptr<GPUTRDTrackGPU[]> trdTracks;
  } mIOMem;

  // Read / Dump / Clear Data
  void ClearIOPointers();
  void AllocateIOMemory();
  using GPUChain::DumpData;
  void DumpData(const char* filename);
  using GPUChain::ReadData;
  int ReadData(const char* filename);
  void DumpSettings(const char* dir = "") override;
  void ReadSettings(const char* dir = "") override;

  // Converter / loader functions
  int ConvertNativeToClusterData();
  void ConvertNativeToClusterDataLegacy();
  void ConvertRun2RawToNative();
  void ConvertZSEncoder(bool zs12bit);
  void ConvertZSFilter(bool zs12bit);

  // Getters for external usage of tracker classes
  GPUTRDTrackerGPU* GetTRDTracker() { return &processors()->trdTracker; }
  GPUTPCTracker* GetTPCSliceTrackers() { return processors()->tpcTrackers; }
  const GPUTPCTracker* GetTPCSliceTrackers() const { return processors()->tpcTrackers; }
  const GPUTPCGMMerger& GetTPCMerger() const { return processors()->tpcMerger; }
  GPUTPCGMMerger& GetTPCMerger() { return processors()->tpcMerger; }
  GPUDisplay* GetEventDisplay() { return mEventDisplay.get(); }
  const GPUQA* GetQA() const { return mQA.get(); }
  GPUQA* GetQA() { return mQA.get(); }
  int ForceInitQA();

  // Processing functions
  int RunTPCClusterizer();
  void ForwardTPCDigits();
  int RunTPCTrackingSlices();
  int RunTPCTrackingMerger();
  int RunTRDTracking();
  int DoTRDGPUTracking();
  int RunTPCCompression();

  // Getters / setters for parameters
  const TPCFastTransform* GetTPCTransform() const { return processors()->calibObjects.fastTransform; }
  const TPCdEdxCalibrationSplines* GetdEdxSplines() const { return processors()->calibObjects.dEdxSplines; }
  const o2::base::MatLayerCylSet* GetMatLUT() const { return processors()->calibObjects.matLUT; }
  const GPUTRDGeometry* GetTRDGeometry() const { return (GPUTRDGeometry*)processors()->calibObjects.trdGeometry; }
  const o2::tpc::ClusterNativeAccess* GetClusterNativeAccess() const { return mClusterNativeAccess.get(); }
  void SetTPCFastTransform(std::unique_ptr<TPCFastTransform>&& tpcFastTransform);
  void SetdEdxSplines(std::unique_ptr<TPCdEdxCalibrationSplines>&& dEdxSplines);
  void SetMatLUT(std::unique_ptr<o2::base::MatLayerCylSet>&& lut);
  void SetTRDGeometry(std::unique_ptr<o2::trd::TRDGeometryFlat>&& geo);
  void SetTPCFastTransform(const TPCFastTransform* tpcFastTransform) { processors()->calibObjects.fastTransform = tpcFastTransform; }
  void SetdEdxSplines(const TPCdEdxCalibrationSplines* dEdxSplines) { processors()->calibObjects.dEdxSplines = dEdxSplines; }
  void SetMatLUT(const o2::base::MatLayerCylSet* lut) { processors()->calibObjects.matLUT = lut; }
  void SetTRDGeometry(const o2::trd::TRDGeometryFlat* geo) { processors()->calibObjects.trdGeometry = geo; }
  void LoadClusterErrors();
  void SetOutputControlCompressedClusters(GPUOutputControl* v) { mOutputCompressedClusters = v; }

  const void* mConfigDisplay = nullptr; // Abstract pointer to Standalone Display Configuration Structure
  const void* mConfigQA = nullptr;      // Abstract pointer to Standalone QA Configuration Structure

 protected:
  struct GPUTrackingFlatObjects : public GPUProcessor {
    GPUChainTracking* mChainTracking = nullptr;
    GPUCalibObjects mCalibObjects;
    char* mTpcTransformBuffer = nullptr;
    char* mdEdxSplinesBuffer = nullptr;
    char* mMatLUTBuffer = nullptr;
    short mMemoryResFlat = -1;
    void* SetPointersFlatObjects(void* mem);
  };

  struct eventStruct // Must consist only of void* ptr that will hold the GPU event ptrs!
  {
    void* selector[NSLICES];
    void* stream[GPUCA_MAX_STREAMS];
    void* init;
    void* constructor;
  };

  GPUChainTracking(GPUReconstruction* rec, unsigned int maxTPCHits = GPUCA_MAX_CLUSTERS, unsigned int maxTRDTracklets = GPUCA_MAX_TRD_TRACKLETS);

  int ReadEvent(unsigned int iSlice, int threadId);
  void WriteOutput(int iSlice, int threadId);
  int GlobalTracking(unsigned int iSlice, int threadId);
  void PrepareEventFromNative();
  void UpdateShadowProcessors();

  int PrepareProfile();
  int DoProfile();
  void PrintMemoryRelations();
  void PrintMemoryStatistics() override;
  void PrepareDebugOutput();
  void PrintDebugOutput();

  bool ValidateSteps();
  bool ValidateSettings();

  // Pointers to tracker classes
  GPUTrackingFlatObjects mFlatObjectsShadow; // Host copy of flat objects that will be used on the GPU
  GPUTrackingFlatObjects mFlatObjectsDevice; // flat objects that will be used on the GPU
  std::unique_ptr<GPUTrackingInputProvider> mInputsHost;
  std::unique_ptr<GPUTrackingInputProvider> mInputsShadow;

  // Display / QA
  std::unique_ptr<GPUDisplay> mEventDisplay;
  bool mDisplayRunning = false;
  std::unique_ptr<GPUQA> mQA;
  std::unique_ptr<GPUTPCClusterStatistics> mCompressionStatistics;

  // Ptr to reconstruction detector objects
  std::unique_ptr<o2::tpc::ClusterNativeAccess> mClusterNativeAccess; // Internal memory for clusterNativeAccess
  std::unique_ptr<GPUTrackingInOutDigits> mDigitMap;                  // Internal memory for digit-map, if needed
  std::unique_ptr<TPCFastTransform> mTPCFastTransformU;               // Global TPC fast transformation object
  std::unique_ptr<TPCdEdxCalibrationSplines> mdEdxSplinesU;           // TPC dEdx calibration splines
  std::unique_ptr<o2::base::MatLayerCylSet> mMatLUTU;                 // Material Lookup Table
  std::unique_ptr<o2::trd::TRDGeometryFlat> mTRDGeometryU;            // TRD Geometry
  std::unique_ptr<unsigned long long int[]> mTPCZSBuffer;             // Memory to store TPC ZS pages
  std::unique_ptr<unsigned int[]> mTPCZSSizes;                        // Array with TPC ZS numbers of pages
  std::unique_ptr<void*[]> mTPCZSPtrs;                                // Array with pointers to TPC ZS pages
  std::unique_ptr<GPUTrackingInOutZS> mTPCZS;                         // TPC ZS Data Structure

  GPUOutputControl* mOutputCompressedClusters = nullptr;

  // Upper bounds for memory allocation
  unsigned int mMaxTPCHits = 0;
  unsigned int mMaxTRDTracklets = 0;

  unsigned int mTPCMaxTimeBin = 0;

  // Debug
  std::ofstream mDebugFile;

  // Synchronization and Locks
  eventStruct* mEvents = nullptr;
  VOLATILE int mSliceSelectorReady = 0;
  std::array<char, NSLICES> mWriteOutputDone;

 private:
  int RunTPCTrackingSlices_internal();
  void RunTPCClusterizer_compactPeaks(GPUTPCClusterFinder& clusterer, GPUTPCClusterFinder& clustererShadow, int stage, bool doGPU, int lane);
  unsigned int TPCClusterizerDecodeZSCount(unsigned int iSlice, unsigned int minTime, unsigned int maxTime);
  void RunTPCTrackingMerger_MergeBorderTracks(char withinSlice, char mergeMode, GPUReconstruction::krnlDeviceType deviceType);

  std::atomic_flag mLockAtomic = ATOMIC_FLAG_INIT;

  int HelperReadEvent(int iSlice, int threadId, GPUReconstructionHelpers::helperParam* par);
  int HelperOutput(int iSlice, int threadId, GPUReconstructionHelpers::helperParam* par);
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif

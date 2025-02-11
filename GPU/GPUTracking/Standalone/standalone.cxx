// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file standalone.cxx
/// \author David Rohr

#include "utils/qconfig.h"
#include "GPUReconstruction.h"
#include "GPUReconstructionTimeframe.h"
#include "GPUReconstructionConvert.h"
#include "GPUChainTracking.h"
#include "GPUTPCDef.h"
#include "GPUQA.h"
#include "GPUDisplayBackend.h"
#include "genEvents.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <tuple>
#include <algorithm>
#ifdef WITH_OPENMP
#include <omp.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fenv.h>
#include <clocale>
#include <sys/stat.h>
#endif
#include "utils/timer.h"
#include "utils/qmaths_helpers.h"

#include "TPCFastTransform.h"
#include "GPUTPCGMMergedTrack.h"
#include "GPUSettings.h"
#include <vector>
#include <xmmintrin.h>

#include "GPUO2DataTypes.h"
#ifdef HAVE_O2HEADERS
#include "GPUChainITS.h"
#include "TPCClusterDecompressor.h"
#endif

#ifdef GPUCA_BUILD_EVENT_DISPLAY
#ifdef _WIN32
#include "GPUDisplayBackendWindows.h"
#else
#include "GPUDisplayBackendX11.h"
#include "GPUDisplayBackendGlfw.h"
#endif
#include "GPUDisplayBackendGlut.h"
#endif

using namespace GPUCA_NAMESPACE::gpu;

//#define BROKEN_EVENTS

GPUReconstruction *rec, *recAsync;
GPUChainTracking *chainTracking, *chainTrackingAsync;
#ifdef HAVE_O2HEADERS
GPUChainITS *chainITS, *chainITSAsync;
#endif
std::unique_ptr<char[]> outputmemory;
std::unique_ptr<GPUDisplayBackend> eventDisplay;
std::unique_ptr<GPUReconstructionTimeframe> tf;
int nEventsInDirectory = 0;

void SetCPUAndOSSettings()
{
#ifdef FE_DFL_DISABLE_SSE_DENORMS_ENV // Flush and load denormals to zero in any case
  fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
#else
#ifndef _MM_FLUSH_ZERO_ON
#define _MM_FLUSH_ZERO_ON 0x8000
#endif
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_ON 0x0040
#endif
  _mm_setcsr(_mm_getcsr() | (_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON));
#endif
}

int ReadConfiguration(int argc, char** argv)
{
  int qcRet = qConfigParse(argc, (const char**)argv);
  if (qcRet) {
    if (qcRet != qConfig::qcrHelp) {
      printf("Error parsing command line parameters\n");
    }
    return 1;
  }
  if (configStandalone.printSettings) {
    qConfigPrint();
  }

#ifndef _WIN32
  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "");
  if (configStandalone.affinity != -1) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(configStandalone.affinity, &mask);

    printf("Setting affinitiy to restrict on CPU %d\n", configStandalone.affinity);
    if (0 != sched_setaffinity(0, sizeof(mask), &mask)) {
      printf("Error setting CPU affinity\n");
      return 1;
    }
  }
  if (configStandalone.fifo) {
    printf("Setting FIFO scheduler\n");
    sched_param param;
    sched_getparam(0, &param);
    param.sched_priority = 1;
    if (0 != sched_setscheduler(0, SCHED_FIFO, &param)) {
      printf("Error setting scheduler\n");
      return 1;
    }
  }
  if (configStandalone.fpe) {
    feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
  }
  if (configStandalone.flushDenormals) {
    disable_denormals();
  }

#else
  if (configStandalone.affinity != -1) {
    printf("Affinity setting not supported on Windows\n");
    return 1;
  }
  if (configStandalone.fifo) {
    printf("FIFO Scheduler setting not supported on Windows\n");
    return 1;
  }
  if (configStandalone.fpe) {
    printf("FPE not supported on Windows\n");
    return 1;
  }
#endif
#ifndef HAVE_O2HEADERS
  configStandalone.configRec.runTRD = configStandalone.configRec.rundEdx = configStandalone.configRec.runCompression = configStandalone.configRec.runTransformation = configStandalone.testSyncAsync = 0;
  configStandalone.configRec.ForceEarlyTPCTransform = 1;
#endif
#ifndef GPUCA_TPC_GEOMETRY_O2
  configStandalone.configRec.mergerReadFromTrackerDirectly = 0;
#endif
#ifndef GPUCA_BUILD_QA
  if (configStandalone.qa || configStandalone.eventGenerator) {
    printf("QA not enabled in build\n");
    return 1;
  }
#endif
  if (configStandalone.qa) {
    if (getenv("LC_NUMERIC")) {
      printf("Please unset the LC_NUMERIC env variable, otherwise ROOT will not be able to fit correctly\n"); // BUG: ROOT Problem
      return 1;
    }
  }
#ifndef GPUCA_BUILD_EVENT_DISPLAY
  if (configStandalone.eventDisplay) {
    printf("EventDisplay not enabled in build\n");
    return 1;
  }
#endif
  if (configStandalone.configTF.bunchSim && configStandalone.configTF.nMerge) {
    printf("Cannot run --MERGE and --SIMBUNCHES togeterh\n");
    return 1;
  }
  if (configStandalone.configTF.bunchSim > 1) {
    configStandalone.configTF.timeFrameLen = 1.e9 * configStandalone.configTF.bunchSim / configStandalone.configTF.interactionRate;
  }
  if (configStandalone.configTF.nMerge) {
    double len = configStandalone.configTF.nMerge - 1;
    if (configStandalone.configTF.randomizeDistance) {
      len += 0.5;
    }
    if (configStandalone.configTF.shiftFirstEvent) {
      len += 0.5;
    }
    configStandalone.configTF.timeFrameLen = (len * configStandalone.configTF.averageDistance / GPUReconstructionTimeframe::TPCZ + 1) * GPUReconstructionTimeframe::DRIFT_TIME;
  }
  if (configStandalone.configQA.inputHistogramsOnly && configStandalone.configQA.compareInputs.size() == 0) {
    printf("Can only produce QA pdf output when input files are specified!\n");
    return 1;
  }
  if (configStandalone.timeFrameTime && configStandalone.DebugLevel == 0) {
    printf("tfTime needs debug >= 1 currently\n");
    return 1;
  }
  if (configStandalone.eventDisplay) {
    configStandalone.noprompt = 1;
  }
  if (configStandalone.DebugLevel >= 4) {
    configStandalone.OMPThreads = 1;
  }

#ifdef WITH_OPENMP
  if (configStandalone.OMPThreads != -1) {
    omp_set_num_threads(configStandalone.OMPThreads);
  } else {
    configStandalone.OMPThreads = omp_get_max_threads();
  }
  if (configStandalone.OMPThreads != omp_get_max_threads()) {
    printf("Cannot set number of OMP threads!\n");
    return 1;
  }
#else
  configStandalone.OMPThreads = 1;
#endif
  if (configStandalone.outputcontrolmem) {
    outputmemory.reset(new char[configStandalone.outputcontrolmem]);
  }

#if !(defined(CUDA_ENABLED) || defined(OPENCL1_ENABLED) || defined(HIP_ENABLED))
  if (configStandalone.runGPU) {
    printf("GPU disables at build time!\n");
    printf("Press a key to exit!\n");
    getchar();
    return 1;
  }
#endif
  return (0);
}

int SetupReconstruction()
{
  if (!configStandalone.eventGenerator) {
    char filename[256];
    snprintf(filename, 256, "events/%s/", configStandalone.EventsDir);
    if (rec->ReadSettings(filename)) {
      printf("Error reading event config file\n");
      return 1;
    }
    printf("Read event settings from dir %s (solenoidBz: %f, home-made events %d, constBz %d, maxTimeBin %d)\n", filename, rec->GetEventSettings().solenoidBz, (int)rec->GetEventSettings().homemadeEvents, (int)rec->GetEventSettings().constBz, rec->GetEventSettings().continuousMaxTimeBin);
    if (configStandalone.testSyncAsync) {
      recAsync->ReadSettings(filename);
    }
  }

  GPUSettingsEvent ev = rec->GetEventSettings();
  GPUSettingsRec recSet;
  GPUSettingsDeviceProcessing devProc;
  GPURecoStepConfiguration steps;

  if (configStandalone.eventGenerator) {
    ev.homemadeEvents = true;
  }
  if (configStandalone.solenoidBz != -1e6f) {
    ev.solenoidBz = configStandalone.solenoidBz;
  }
  if (configStandalone.constBz) {
    ev.constBz = true;
  }
  if (configStandalone.configTF.nMerge || configStandalone.configTF.bunchSim) {
    if (ev.continuousMaxTimeBin) {
      printf("ERROR: requested to overlay continuous data - not supported\n");
      return 1;
    }
    if (!configStandalone.cont) {
      printf("Continuous mode forced\n");
      configStandalone.cont = true;
    }
    if (chainTracking->GetTPCTransform()) {
      ev.continuousMaxTimeBin = configStandalone.configTF.timeFrameLen * ((double)GPUReconstructionTimeframe::TPCZ / (double)GPUReconstructionTimeframe::DRIFT_TIME) / chainTracking->GetTPCTransform()->getVDrift();
    }
  }
  if (configStandalone.cont && ev.continuousMaxTimeBin == 0) {
    ev.continuousMaxTimeBin = -1;
  }
  if (rec->GetDeviceType() == GPUReconstruction::DeviceType::CPU) {
    printf("Standalone Test Framework for CA Tracker - Using CPU\n");
  } else {
    printf("Standalone Test Framework for CA Tracker - Using GPU\n");
  }

  recSet.SetMinTrackPt(GPUCA_MIN_TRACK_PT_DEFAULT);
  recSet.NWays = configStandalone.nways;
  recSet.NWaysOuter = configStandalone.nwaysouter;
  recSet.RejectMode = configStandalone.rejectMode;
  recSet.SearchWindowDZDR = configStandalone.dzdr;
  recSet.GlobalTracking = configStandalone.configRec.globalTracking;
  recSet.DisableRefitAttachment = configStandalone.configRec.disableRefitAttachment;
  recSet.ForceEarlyTPCTransform = configStandalone.configRec.ForceEarlyTPCTransform;
  recSet.fwdTPCDigitsAsClusters = configStandalone.configRec.fwdTPCDigitsAsClusters;
  recSet.dropLoopers = configStandalone.configRec.dropLoopers;
  if (configStandalone.configRec.mergerCovSource != -1) {
    recSet.mergerCovSource = configStandalone.configRec.mergerCovSource;
  }
  if (configStandalone.configRec.mergerInterpolateErrors != -1) {
    recSet.mergerInterpolateErrors = configStandalone.configRec.mergerInterpolateErrors;
  }
  if (configStandalone.referenceX < 500.) {
    recSet.TrackReferenceX = configStandalone.referenceX;
  }
  recSet.tpcZSthreshold = configStandalone.zsThreshold;
  if (configStandalone.configRec.fitInProjections != -1) {
    recSet.fitInProjections = configStandalone.configRec.fitInProjections;
  }
  if (configStandalone.configRec.fitPropagateBzOnly != -1) {
    recSet.fitPropagateBzOnly = configStandalone.configRec.fitPropagateBzOnly;
  }
  if (configStandalone.configRec.retryRefit != -1) {
    recSet.retryRefit = configStandalone.configRec.retryRefit;
  }
  if (configStandalone.configRec.loopInterpolationInExtraPass != -1) {
    recSet.loopInterpolationInExtraPass = configStandalone.configRec.loopInterpolationInExtraPass;
  }
  recSet.mergerReadFromTrackerDirectly = configStandalone.configRec.mergerReadFromTrackerDirectly;
  if (!recSet.mergerReadFromTrackerDirectly) {
    devProc.fullMergerOnGPU = false;
  }

  if (configStandalone.OMPThreads != -1) {
    devProc.nThreads = configStandalone.OMPThreads;
  }
  devProc.deviceNum = configStandalone.cudaDevice;
  devProc.forceMemoryPoolSize = (configStandalone.forceMemorySize == 1 && configStandalone.eventDisplay) ? 2 : configStandalone.forceMemorySize;
  devProc.debugLevel = configStandalone.DebugLevel;
  devProc.deviceTimers = configStandalone.DeviceTiming;
  devProc.runQA = configStandalone.qa;
  devProc.runMC = configStandalone.configProc.runMC;
  devProc.runCompressionStatistics = configStandalone.compressionStat;
  devProc.memoryScalingFactor = configStandalone.memoryScalingFactor;
  if (configStandalone.eventDisplay) {
#ifdef GPUCA_BUILD_EVENT_DISPLAY
#ifdef _WIN32
    if (configStandalone.eventDisplay == 1) {
      printf("Enabling event display (windows backend)\n");
      eventDisplay.reset(new GPUDisplayBackendWindows);
    }

#else
    if (configStandalone.eventDisplay == 1) {
      eventDisplay.reset(new GPUDisplayBackendX11);
      printf("Enabling event display (X11 backend)\n");
    }
    if (configStandalone.eventDisplay == 3) {
      eventDisplay.reset(new GPUDisplayBackendGlfw);
      printf("Enabling event display (GLFW backend)\n");
    }

#endif
    else if (configStandalone.eventDisplay == 2) {
      eventDisplay.reset(new GPUDisplayBackendGlut);
      printf("Enabling event display (GLUT backend)\n");
    }

#endif
    devProc.eventDisplay = eventDisplay.get();
  }
  devProc.nDeviceHelperThreads = configStandalone.helperThreads;
  devProc.globalInitMutex = configStandalone.gpuInitMutex;
  devProc.gpuDeviceOnly = configStandalone.oclGPUonly;
  devProc.memoryAllocationStrategy = configStandalone.allocationStrategy;
  devProc.registerStandaloneInputMemory = configStandalone.registerInputMemory;
  if (configStandalone.configRec.tpcReject != -1) {
    recSet.tpcRejectionMode = configStandalone.configRec.tpcReject;
  }
  if (configStandalone.configRec.tpcRejectThreshold != 0.f) {
    recSet.tpcRejectQPt = 1.f / configStandalone.configRec.tpcRejectThreshold;
  }
  recSet.tpcCompressionModes = configStandalone.configRec.tpcCompression;
  recSet.tpcCompressionSortOrder = configStandalone.configRec.tpcCompressionSort;

  if (configStandalone.configProc.nStreams >= 0) {
    devProc.nStreams = configStandalone.configProc.nStreams;
  }
  if (configStandalone.configProc.constructorPipeline >= 0) {
    devProc.trackletConstructorInPipeline = configStandalone.configProc.constructorPipeline;
  }
  if (configStandalone.configProc.selectorPipeline >= 0) {
    devProc.trackletSelectorInPipeline = configStandalone.configProc.selectorPipeline;
  }
  devProc.mergerSortTracks = configStandalone.configProc.mergerSortTracks;
  devProc.tpcCompressionGatherMode = configStandalone.configProc.tpcCompressionGatherMode;

  steps.steps = GPUReconstruction::RecoStep::AllRecoSteps;
  if (configStandalone.configRec.runTRD != -1) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TRDTracking, configStandalone.configRec.runTRD > 0);
  }
  if (configStandalone.configRec.rundEdx != -1) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCdEdx, configStandalone.configRec.rundEdx > 0);
  }
  if (configStandalone.configRec.runCompression != -1) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCCompression, configStandalone.configRec.runCompression > 0);
  }
  if (configStandalone.configRec.runTransformation != -1) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCConversion, configStandalone.configRec.runTransformation > 0);
  }
  if (!configStandalone.merger) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCMerging, false);
    steps.steps.setBits(GPUReconstruction::RecoStep::TRDTracking, false);
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCdEdx, false);
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCCompression, false);
  }
  if (configStandalone.configTF.bunchSim || configStandalone.configTF.nMerge) {
    steps.steps.setBits(GPUReconstruction::RecoStep::TRDTracking, false);
  }
  steps.inputs.set(GPUDataTypes::InOutType::TPCClusters, GPUDataTypes::InOutType::TRDTracklets);
  if (ev.needsClusterer) {
    steps.inputs.setBits(GPUDataTypes::InOutType::TPCRaw, true);
    steps.inputs.setBits(GPUDataTypes::InOutType::TPCClusters, false);
  } else {
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCClusterFinding, false);
  }

  if (configStandalone.configProc.recoSteps >= 0) {
    steps.steps &= configStandalone.configProc.recoSteps;
  }
  if (configStandalone.configProc.recoStepsGPU >= 0) {
    steps.stepsGPUMask &= configStandalone.configProc.recoStepsGPU;
  }

  steps.outputs.clear();
  steps.outputs.setBits(GPUDataTypes::InOutType::TPCSectorTracks, steps.steps.isSet(GPUReconstruction::RecoStep::TPCSliceTracking) && !recSet.mergerReadFromTrackerDirectly);
  steps.outputs.setBits(GPUDataTypes::InOutType::TPCMergedTracks, steps.steps.isSet(GPUReconstruction::RecoStep::TPCMerging));
  steps.outputs.setBits(GPUDataTypes::InOutType::TPCCompressedClusters, steps.steps.isSet(GPUReconstruction::RecoStep::TPCCompression));
  steps.outputs.setBits(GPUDataTypes::InOutType::TRDTracks, steps.steps.isSet(GPUReconstruction::RecoStep::TRDTracking));
  steps.outputs.setBits(GPUDataTypes::InOutType::TPCClusters, steps.steps.isSet(GPUReconstruction::RecoStep::TPCClusterFinding));

  if (configStandalone.testSyncAsync) {
    // Set settings for synchronous
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCdEdx, 0);
    devProc.eventDisplay = nullptr;
  }
  rec->SetSettings(&ev, &recSet, &devProc, &steps);
  if (configStandalone.testSyncAsync) {
    // Set settings for asynchronous
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCdEdx, 1);
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCCompression, false);
    steps.outputs.setBits(GPUDataTypes::InOutType::TPCCompressedClusters, 0);
    steps.steps.setBits(GPUReconstruction::RecoStep::TPCClusterFinding, false);
    steps.inputs.setBits(GPUDataTypes::InOutType::TPCRaw, false);
    steps.inputs.setBits(GPUDataTypes::InOutType::TPCClusters, true);
    devProc.runMC = false;
    devProc.runQA = false;
    devProc.eventDisplay = eventDisplay.get();
    devProc.runCompressionStatistics = 0;
    recSet.DisableRefitAttachment = 0xFF;
    recSet.loopInterpolationInExtraPass = 0;
    recSet.MaxTrackQPt = CAMath::Min(recSet.MaxTrackQPt, recSet.tpcRejectQPt);
    recAsync->SetSettings(&ev, &recSet, &devProc, &steps);
  }
  if (rec->Init()) {
    printf("Error initializing GPUReconstruction!\n");
    return 1;
  }
  if (configStandalone.outputcontrolmem && rec->IsGPU() && rec->registerMemoryForGPU(outputmemory.get(), configStandalone.outputcontrolmem)) {
    printf("ERROR registering memory for the GPU!!!\n");
  }
  if (configStandalone.DebugLevel >= 4) {
    rec->PrintKernelOccupancies();
  }
  return (0);
}

int ReadEvent(int n)
{
  char filename[256];
  snprintf(filename, 256, "events/%s/" GPUCA_EVDUMP_FILE ".%d.dump", configStandalone.EventsDir, n);
  int r = chainTracking->ReadData(filename);
  if (r) {
    return r;
  }
  if (chainTracking->mIOPtrs.clustersNative && (configStandalone.configTF.bunchSim || configStandalone.configTF.nMerge || !configStandalone.configRec.runTransformation)) {
    if (configStandalone.DebugLevel >= 2) {
      printf("Converting Native to Legacy ClusterData for overlaying - WARNING: No raw clusters produced - Compression etc will not run!!!\n");
    }
    chainTracking->ConvertNativeToClusterDataLegacy();
  }
  return 0;
}

void OutputStat(GPUChainTracking* t, long long int* nTracksTotal = nullptr, long long int* nClustersTotal = nullptr)
{
  int nTracks = 0, nClusters = 0, nAttachedClusters = 0, nAttachedClustersFitted = 0, nAdjacentClusters = 0;
  for (int k = 0; k < t->GetTPCMerger().NOutputTracks(); k++) {
    if (t->GetTPCMerger().OutputTracks()[k].OK()) {
      nTracks++;
      nAttachedClusters += t->GetTPCMerger().OutputTracks()[k].NClusters();
      nAttachedClustersFitted += t->GetTPCMerger().OutputTracks()[k].NClustersFitted();
    }
  }
  for (unsigned int k = 0; k < t->GetTPCMerger().NMaxClusters(); k++) {
    int attach = t->GetTPCMerger().ClusterAttachment()[k];
    if (attach & GPUTPCGMMergerTypes::attachFlagMask) {
      nAdjacentClusters++;
    }
  }

  nClusters = t->GetTPCMerger().NClusters();
  if (nTracksTotal && nClustersTotal) {
    *nTracksTotal += nTracks;
    *nClustersTotal += nClusters;
  }

  char trdText[1024] = "";
  if (t->GetRecoSteps() & GPUReconstruction::RecoStep::TRDTracking) {
    int nTracklets = 0;
    for (int k = 0; k < t->GetTRDTracker()->NTracks(); k++) {
      auto& trk = t->GetTRDTracker()->Tracks()[k];
      nTracklets += trk.GetNtracklets();
    }
    snprintf(trdText, 1024, " - TRD Tracker reconstructed %d tracks (%d tracklets)", t->GetTRDTracker()->NTracks(), nTracklets);
  }
  printf("Output Tracks: %d (%d / %d / %d / %d clusters (fitted / attached / adjacent / total))%s\n", nTracks, nAttachedClustersFitted, nAttachedClusters, nAdjacentClusters, t->GetTPCMerger().NMaxClusters(), trdText);
}

int main(int argc, char** argv)
{
  std::unique_ptr<GPUReconstruction> recUnique, recUniqueAsync;

  SetCPUAndOSSettings();

  if (ReadConfiguration(argc, argv)) {
    return 1;
  }

  recUnique.reset(GPUReconstruction::CreateInstance(configStandalone.runGPU ? configStandalone.gpuType : GPUReconstruction::DEVICE_TYPE_NAMES[GPUReconstruction::DeviceType::CPU], configStandalone.runGPUforce));
  rec = recUnique.get();
  if (configStandalone.testSyncAsync) {
    recUniqueAsync.reset(GPUReconstruction::CreateInstance(configStandalone.runGPU ? configStandalone.gpuType : GPUReconstruction::DEVICE_TYPE_NAMES[GPUReconstruction::DeviceType::CPU], configStandalone.runGPUforce, rec));
    recAsync = recUniqueAsync.get();
  }
  if (rec == nullptr || (configStandalone.testSyncAsync && recAsync == nullptr)) {
    printf("Error initializing GPUReconstruction\n");
    return 1;
  }
  rec->SetDebugLevelTmp(configStandalone.DebugLevel);
  chainTracking = rec->AddChain<GPUChainTracking>();
  if (configStandalone.testSyncAsync) {
    recAsync->SetDebugLevelTmp(configStandalone.DebugLevel);
    chainTrackingAsync = recAsync->AddChain<GPUChainTracking>();
  }
#ifdef HAVE_O2HEADERS
  chainITS = rec->AddChain<GPUChainITS>(0);
  if (configStandalone.testSyncAsync) {
    chainITSAsync = recAsync->AddChain<GPUChainITS>(0);
  }
#endif

  if (SetupReconstruction()) {
    return 1;
  }

  // hlt.SetRunMerger(configStandalone.merger); //TODO!

  if (configStandalone.seed == -1) {
    std::random_device rd;
    configStandalone.seed = (int)rd();
    printf("Using random seed %d\n", configStandalone.seed);
  }

  srand(configStandalone.seed);

  for (nEventsInDirectory = 0; true; nEventsInDirectory++) {
    std::ifstream in;
    char filename[256];
    snprintf(filename, 256, "events/%s/" GPUCA_EVDUMP_FILE ".%d.dump", configStandalone.EventsDir, nEventsInDirectory);
    in.open(filename, std::ifstream::binary);
    if (in.fail()) {
      break;
    }
    in.close();
  }

  if (configStandalone.configTF.bunchSim || configStandalone.configTF.nMerge) {
    tf.reset(new GPUReconstructionTimeframe(chainTracking, ReadEvent, nEventsInDirectory));
  }

  if (configStandalone.eventGenerator) {
    genEvents::RunEventGenerator(chainTracking);
    return 1;
  } else {
    int nEvents = configStandalone.NEvents;
    if (configStandalone.configTF.bunchSim) {
      nEvents = configStandalone.NEvents > 0 ? configStandalone.NEvents : 1;
    } else {
      if (nEvents == -1 || nEvents > nEventsInDirectory) {
        if (nEvents >= 0) {
          printf("Only %d events available in directors %s (%d events requested)\n", nEventsInDirectory, configStandalone.EventsDir, nEvents);
        }
        nEvents = nEventsInDirectory;
      }
      if (configStandalone.configTF.nMerge > 1) {
        nEvents /= configStandalone.configTF.nMerge;
      }
    }

    for (int j2 = 0; j2 < configStandalone.runs2; j2++) {
      if (configStandalone.configQA.inputHistogramsOnly) {
        chainTracking->ForceInitQA();
        break;
      }
      if (configStandalone.runs2 > 1) {
        printf("RUN2: %d\n", j2);
      }
      long long int nTracksTotal = 0;
      long long int nClustersTotal = 0;
      int nEventsProcessed = 0;

      for (int iEvent = configStandalone.StartEvent; iEvent < nEvents; iEvent++) {
        if (iEvent != configStandalone.StartEvent) {
          printf("\n");
        }
        HighResTimer timerLoad;
        timerLoad.Start();
        if (configStandalone.configTF.bunchSim) {
          if (tf->LoadCreateTimeFrame(iEvent)) {
            break;
          }
        } else if (configStandalone.configTF.nMerge) {
          if (tf->LoadMergedEvents(iEvent)) {
            break;
          }
        } else {
          if (ReadEvent(iEvent)) {
            break;
          }
        }
        bool encodeZS = configStandalone.encodeZS == -1 ? (chainTracking->mIOPtrs.tpcPackedDigits && !chainTracking->mIOPtrs.tpcZS) : (bool)configStandalone.encodeZS;
        bool zsFilter = configStandalone.zsFilter == -1 ? (!encodeZS && chainTracking->mIOPtrs.tpcPackedDigits) : (bool)configStandalone.zsFilter;
        if (encodeZS || zsFilter) {
          if (!chainTracking->mIOPtrs.tpcPackedDigits) {
            printf("Need digit input to run ZS\n");
            goto breakrun;
          }
          if (zsFilter) {
            chainTracking->ConvertZSFilter(configStandalone.zs12bit);
          }
          if (encodeZS) {
            chainTracking->ConvertZSEncoder(configStandalone.zs12bit);
          }
        }
        if (!configStandalone.configRec.runTransformation) {
          chainTracking->mIOPtrs.clustersNative = nullptr;
        } else {
          for (int i = 0; i < chainTracking->NSLICES; i++) {
            if (chainTracking->mIOPtrs.rawClusters[i]) {
              if (configStandalone.DebugLevel >= 2) {
                printf("Converting Legacy Raw Cluster to Native\n");
              }
              chainTracking->ConvertRun2RawToNative();
              break;
            }
          }
        }

        if (configStandalone.stripDumpedEvents) {
          if (chainTracking->mIOPtrs.tpcZS) {
            chainTracking->mIOPtrs.tpcPackedDigits = nullptr;
          }
        }
        if (configStandalone.dumpEvents) {
          char fname[1024];
          sprintf(fname, "event.%d.dump", nEventsProcessed);
          chainTracking->DumpData(fname);
          if (nEventsProcessed == 0) {
            rec->DumpSettings();
          }
        }

        if (configStandalone.overrideMaxTimebin && (chainTracking->mIOPtrs.clustersNative || chainTracking->mIOPtrs.tpcPackedDigits || chainTracking->mIOPtrs.tpcZS)) {
          GPUSettingsEvent ev = rec->GetEventSettings();
          ev.continuousMaxTimeBin = chainTracking->mIOPtrs.tpcZS ? GPUReconstructionConvert::GetMaxTimeBin(*chainTracking->mIOPtrs.tpcZS) : chainTracking->mIOPtrs.tpcPackedDigits ? GPUReconstructionConvert::GetMaxTimeBin(*chainTracking->mIOPtrs.tpcPackedDigits) : GPUReconstructionConvert::GetMaxTimeBin(*chainTracking->mIOPtrs.clustersNative);
          printf("Max time bin set to %d\n", (int)ev.continuousMaxTimeBin);
          rec->UpdateEventSettings(&ev);
        }
        if (!rec->GetParam().earlyTpcTransform && chainTracking->mIOPtrs.clustersNative == nullptr && chainTracking->mIOPtrs.tpcPackedDigits == nullptr && chainTracking->mIOPtrs.tpcZS == nullptr) {
          printf("Need cluster native data for on-the-fly TPC transform\n");
          goto breakrun;
        }

        printf("Loading time: %'d us\n", (int)(1000000 * timerLoad.GetCurrentElapsedTime()));

        printf("Processing Event %d\n", iEvent);
        GPUTrackingInOutPointers ioPtrSave = chainTracking->mIOPtrs;
        for (int j1 = 0; j1 < configStandalone.runs; j1++) {
          if (configStandalone.runs > 1) {
            printf("Run %d\n", j1 + 1);
          }
          if (configStandalone.outputcontrolmem) {
            rec->SetOutputControl(outputmemory.get(), configStandalone.outputcontrolmem);
          }
          rec->SetResetTimers(j1 < configStandalone.runsInit);

          if (configStandalone.testSyncAsync) {
            recAsync->SetResetTimers(j1 < configStandalone.runsInit);
            printf("Running synchronous phase\n");
          }
          chainTracking->mIOPtrs = ioPtrSave;
          if (configStandalone.controlProfiler && j1 == configStandalone.runs - 1) {
            rec->startGPUProfiling();
          }
          int tmpRetVal = rec->RunChains();
          if (configStandalone.controlProfiler && j1 == configStandalone.runs - 1) {
            rec->endGPUProfiling();
          }
          nEventsProcessed += (j1 == 0);

          if (tmpRetVal == 0 || tmpRetVal == 2) {
            OutputStat(chainTracking, j1 == 0 ? &nTracksTotal : nullptr, j1 == 0 ? &nClustersTotal : nullptr);
            if (configStandalone.memoryStat) {
              rec->PrintMemoryStatistics();
            } else if (configStandalone.DebugLevel >= 2) {
              rec->PrintMemoryOverview();
            }
          }

#ifdef HAVE_O2HEADERS
          if (tmpRetVal == 0 && configStandalone.testSyncAsync) {
            if (configStandalone.testSyncAsync) {
              printf("Running asynchronous phase\n");
            }

            TPCClusterDecompressor decomp;
            o2::tpc::ClusterNativeAccess clNativeAccess;
            std::vector<o2::tpc::ClusterNative> clBuffer;
            HighResTimer timerDecompress;
            timerDecompress.ResetStart();
            if (decomp.decompress(chainTracking->mIOPtrs.tpcCompressedClusters, clNativeAccess, clBuffer, recAsync->GetParam())) {
              printf("Error decompressing clusters\n");
              goto breakrun;
            }
            printf("Cluster decompression time: %'d us\n", (int)(timerDecompress.GetCurrentElapsedTime() * 1000000));
            chainTrackingAsync->mIOPtrs = ioPtrSave;
            chainTrackingAsync->mIOPtrs.tpcZS = nullptr;
            chainTrackingAsync->mIOPtrs.tpcPackedDigits = nullptr;
            chainTrackingAsync->mIOPtrs.mcInfosTPC = nullptr;
            chainTrackingAsync->mIOPtrs.nMCInfosTPC = 0;
            chainTrackingAsync->mIOPtrs.mcLabelsTPC = nullptr;
            chainTrackingAsync->mIOPtrs.nMCLabelsTPC = 0;
            for (int i = 0; i < chainTracking->NSLICES; i++) {
              chainTrackingAsync->mIOPtrs.clusterData[i] = nullptr;
              chainTrackingAsync->mIOPtrs.nClusterData[i] = 0;
              chainTrackingAsync->mIOPtrs.rawClusters[i] = nullptr;
              chainTrackingAsync->mIOPtrs.nRawClusters[i] = 0;
            }
            chainTrackingAsync->mIOPtrs.clustersNative = &clNativeAccess;
            if (configStandalone.controlProfiler && j1 == configStandalone.runs - 1) {
              rec->startGPUProfiling();
            }
            tmpRetVal = recAsync->RunChains();
            if (configStandalone.controlProfiler && j1 == configStandalone.runs - 1) {
              rec->endGPUProfiling();
            }
            if (tmpRetVal == 0 || tmpRetVal == 2) {
              OutputStat(chainTrackingAsync, nullptr, nullptr);
              if (configStandalone.memoryStat) {
                recAsync->PrintMemoryStatistics();
              }
            }
            recAsync->ClearAllocatedMemory();
          }
#endif
          rec->ClearAllocatedMemory();

          if (tmpRetVal == 2) {
            configStandalone.continueOnError = 0; // Forced exit from event display loop
            configStandalone.noprompt = 1;
          }
          if (tmpRetVal && !configStandalone.continueOnError) {
            if (tmpRetVal != 2) {
              printf("Error occured\n");
            }
            goto breakrun;
          }
        }
        if (configStandalone.timeFrameTime) {
          double nClusters = chainTracking->GetTPCMerger().NMaxClusters();
          if (nClusters > 0) {
            double nClsPerTF = 550000. * 1138.3;
            double timePerTF = rec->GetStatKernelTime() / 1000000. * nClsPerTF / nClusters;
            double nGPUsReq = timePerTF / 0.02277;
            char stat[1024];
            snprintf(stat, 1024, "Sync phase: %.2f sec per 256 orbit TF, %.1f GPUs required", timePerTF, nGPUsReq);
            if (configStandalone.testSyncAsync) {
              timePerTF = recAsync->GetStatKernelTime() / 1000000. * nClsPerTF / nClusters;
              snprintf(stat + strlen(stat), 1024 - strlen(stat), " - Async phase: %f sec per TF", timePerTF);
            }
            printf("%s (Extrapolated from %d clusters to %d)\n", stat, (int)nClusters, (int)nClsPerTF);
          }
        }
      }
      if (nEventsProcessed > 1) {
        printf("Total: %lld clusters, %lld tracks\n", nClustersTotal, nTracksTotal);
      }
    }
  }
breakrun:

#ifndef _WIN32
  if (configStandalone.qa && configStandalone.fpe) {
    fedisableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
  }
#endif

  rec->Finalize();
  if (configStandalone.outputcontrolmem && rec->IsGPU()) {
    rec->unregisterMemoryForGPU(outputmemory.get());
  }
  rec->Exit();

  if (!configStandalone.noprompt) {
    printf("Press a key to exit!\n");
    getchar();
  }
  return (0);
}

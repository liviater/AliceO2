// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GPUReconstructionKernels.h
/// \author David Rohr

// No header protection, this may be used multiple times
#include "GPUReconstructionKernelMacros.h"

// clang-format off
GPUCA_KRNL((GPUTPCNeighboursFinder                       ), (single, REG, (GPUCA_LB_GPUTPCNeighboursFinder)), (), ())
GPUCA_KRNL((GPUTPCNeighboursCleaner                      ), (single, REG, (GPUCA_LB_GPUTPCNeighboursCleaner)), (), ())
GPUCA_KRNL((GPUTPCStartHitsFinder                        ), (single, REG, (GPUCA_LB_GPUTPCStartHitsFinder , 1)), (), ())
GPUCA_KRNL((GPUTPCStartHitsSorter                        ), (single, REG, (GPUCA_LB_GPUTPCStartHitsSorter)), (), ())
GPUCA_KRNL((GPUTPCTrackletConstructor, singleSlice       ), (single, REG, (GPUCA_LB_GPUTPCTrackletConstructor)), (), ())
GPUCA_KRNL((GPUTPCTrackletConstructor, allSlices         ), (single, REG, (GPUCA_LB_GPUTPCTrackletConstructor)), (), ())
GPUCA_KRNL((GPUTPCTrackletSelector                       ), (both, REG, (GPUCA_LB_GPUTPCTrackletSelector)), (), ())
GPUCA_KRNL((GPUMemClean16                                ), (simple, REG, (GPUCA_THREAD_COUNT, 1)), (, GPUPtr1(void*, ptr), unsigned long size), (, GPUPtr2(void*, ptr), size))
GPUCA_KRNL((GPUTPCGlobalTrackingCopyNumbers              ), (single), (, int n), (, n))
#if !defined(GPUCA_OPENCL1) && (!defined(GPUCA_ALIROOT_LIB) || !defined(GPUCA_GPUCODE))
GPUCA_KRNL((GPUTPCCreateSliceData                        ), (single, REG, (GPUCA_LB_GPUTPCCreateSliceData)), (), ())
GPUCA_KRNL((GPUTPCGlobalTracking                         ), (single, REG, (GPUCA_LB_GPUTPCGlobalTracking)), (), ())
GPUCA_KRNL((GPUTPCGMMergerTrackFit                       ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerTrackFit)), (, int mode), (, mode))
GPUCA_KRNL((GPUTPCGMMergerFollowLoopers                  ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerFollowLoopers)), (), ())
GPUCA_KRNL((GPUTPCGMMergerUnpackResetIds                 ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerUnpackResetIds)), (, int iSlice), (, iSlice))
GPUCA_KRNL((GPUTPCGMMergerSliceRefit                     ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerSliceRefit)), (, int iSlice), (, iSlice))
GPUCA_KRNL((GPUTPCGMMergerUnpackGlobal                   ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerUnpackGlobal)), (, int iSlice), (, iSlice))
GPUCA_KRNL((GPUTPCGMMergerUnpackSaveNumber               ), (simple), (, int id), (, id))
GPUCA_KRNL((GPUTPCGMMergerResolve                        ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerResolve)), (, char useOrigTrackParam, char mergeAll), (, useOrigTrackParam, mergeAll))
GPUCA_KRNL((GPUTPCGMMergerClearLinks                     ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerClearLinks)), (, char nOutput), (, nOutput))
GPUCA_KRNL((GPUTPCGMMergerMergeWithinPrepare             ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerMergeWithinPrepare)), (), ())
GPUCA_KRNL((GPUTPCGMMergerMergeSlicesPrepare             ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerMergeSlicesPrepare)), (, int border0, int border1, char useOrigTrackParam), (, border0, border1, useOrigTrackParam))
GPUCA_KRNL((GPUTPCGMMergerMergeBorders, step0            ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerMergeBorders_step0)), (, int iSlice, char withinSlice, char mergeMode), (, iSlice, withinSlice, mergeMode))
GPUCA_KRNL((GPUTPCGMMergerMergeBorders, step1            ), (simple), (, int iSlice, char withinSlice, char mergeMode), (, iSlice, withinSlice, mergeMode))
GPUCA_KRNL((GPUTPCGMMergerMergeBorders, step2            ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerMergeBorders_step2)), (, int iSlice, char withinSlice, char mergeMode), (, iSlice, withinSlice, mergeMode))
GPUCA_KRNL((GPUTPCGMMergerMergeCE                        ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerMergeCE)), (), ())
GPUCA_KRNL((GPUTPCGMMergerLinkGlobalTracks               ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerLinkGlobalTracks)), (), ())
GPUCA_KRNL((GPUTPCGMMergerCollect                        ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerCollect)), (), ())
GPUCA_KRNL((GPUTPCGMMergerSortTracks                     ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerSortTracks)), (), ())
GPUCA_KRNL((GPUTPCGMMergerSortTracksQPt                  ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerSortTracksQPt)), (), ())
GPUCA_KRNL((GPUTPCGMMergerSortTracksPrepare              ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerSortTracksPrepare)), (), ())
GPUCA_KRNL((GPUTPCGMMergerPrepareClusters, step0         ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerPrepareClusters_step0)), (), ())
GPUCA_KRNL((GPUTPCGMMergerPrepareClusters, step1         ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerPrepareClusters_step1)), (), ())
GPUCA_KRNL((GPUTPCGMMergerPrepareClusters, step2         ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerPrepareClusters_step2)), (), ())
GPUCA_KRNL((GPUTPCGMMergerFinalize, step0                ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerFinalize_step0)), (), ())
GPUCA_KRNL((GPUTPCGMMergerFinalize, step1                ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerFinalize_step1)), (), ())
GPUCA_KRNL((GPUTPCGMMergerFinalize, step2                ), (simple, REG, (GPUCA_LB_GPUTPCGMMergerFinalize_step2)), (), ())
#ifdef HAVE_O2HEADERS
GPUCA_KRNL((GPUTRDTrackerKernels                         ), (simple, REG, (GPUCA_LB_GPUTRDTrackerKernels)), (), ())
GPUCA_KRNL((GPUITSFitterKernel                           ), (simple, REG, (GPUCA_LB_GPUITSFitterKernel)), (), ())
GPUCA_KRNL((GPUTPCConvertKernel                          ), (simple, REG, (GPUCA_LB_GPUTPCConvertKernel)), (), ())
GPUCA_KRNL((GPUTPCCompressionKernels,   step0attached    ), (simple, REG, (GPUCA_LB_GPUTPCCompressionKernels_step0attached)), (), ())
GPUCA_KRNL((GPUTPCCompressionKernels,   step1unattached  ), (simple, REG, (GPUCA_LB_GPUTPCCompressionKernels_step1unattached)), (), ())
GPUCA_KRNL((GPUTPCCompressionKernels,   step2gather      ), (simple, REG, (GPUCA_LB_GPUTPCCompressionKernels_step2gather)), (), ())
GPUCA_KRNL((GPUTPCCFChargeMapFiller,    fillIndexMap     ), (single, REG, (GPUCA_LB_GPUTPCCFChargeMapFiller)), (), ())
GPUCA_KRNL((GPUTPCCFChargeMapFiller,    fillFromDigits   ), (single, REG, (GPUCA_LB_GPUTPCCFChargeMapFiller)), (), ())
GPUCA_KRNL((GPUTPCCFChargeMapFiller,    resetMaps        ), (single, REG, (GPUCA_LB_GPUTPCCFChargeMapFiller)), (), ())
GPUCA_KRNL((GPUTPCCFChargeMapFiller,    findFragmentStart), (single, REG, (GPUCA_LB_GPUTPCCFChargeMapFiller)), (), ())
GPUCA_KRNL((GPUTPCCFPeakFinder                           ), (single, REG, (GPUCA_LB_GPUTPCCFPeakFinder)), (), ())
GPUCA_KRNL((GPUTPCCFNoiseSuppression,   noiseSuppression ), (single, REG, (GPUCA_LB_GPUTPCCFNoiseSuppression)), (), ())
GPUCA_KRNL((GPUTPCCFNoiseSuppression,   updatePeaks      ), (single, REG, (GPUCA_LB_GPUTPCCFNoiseSuppression)), (), ())
GPUCA_KRNL((GPUTPCCFDeconvolution                        ), (single, REG, (GPUCA_LB_GPUTPCCFDeconvolution)), (), ())
GPUCA_KRNL((GPUTPCCFClusterizer                          ), (single, REG, (GPUCA_LB_GPUTPCCFClusterizer)), (), ())
GPUCA_KRNL((GPUTPCCFMCLabelFlattener,   setRowOffsets    ), (single, REG, (GPUCA_LB_GPUTPCCFMCLabelFlattener)), (), ())
GPUCA_KRNL((GPUTPCCFMCLabelFlattener,   flatten          ), (single, REG, (GPUCA_LB_GPUTPCCFMCLabelFlattener)), (, unsigned int row, GPUPtr1(GPUTPCLinearLabels*, out)), (, row, GPUPtr2(GPUTPCLinearLabels*, out)))
GPUCA_KRNL((GPUTPCCFStreamCompaction,   nativeScanUpStart), (single, REG, (GPUCA_LB_GPUTPCCFStreamCompaction_nativeScanUpStart)), (, int iBuf, int stage), (, iBuf, stage))
GPUCA_KRNL((GPUTPCCFStreamCompaction,   nativeScanUp     ), (single, REG, (GPUCA_LB_GPUTPCCFStreamCompaction_nativeScanUp)), (, int iBuf, int nElems), (, iBuf, nElems))
GPUCA_KRNL((GPUTPCCFStreamCompaction,   nativeScanTop    ), (single, REG, (GPUCA_LB_GPUTPCCFStreamCompaction_nativeScanTop)), (, int iBuf, int nElems), (, iBuf, nElems))
GPUCA_KRNL((GPUTPCCFStreamCompaction,   nativeScanDown   ), (single, REG, (GPUCA_LB_GPUTPCCFStreamCompaction_nativeScanDown)), (, int iBuf, unsigned int offset, int nElems), (, iBuf, offset, nElems))
GPUCA_KRNL((GPUTPCCFStreamCompaction,   compact          ), (single, REG, (GPUCA_LB_GPUTPCCFStreamCompaction_compact)), (, int iBuf, int stage, GPUPtr1(ChargePos*, in), GPUPtr1(ChargePos*, out)), (, iBuf, stage, GPUPtr2(ChargePos*, in), GPUPtr2(ChargePos*, out)))
GPUCA_KRNL((GPUTPCCFDecodeZS                             ), (single, REG, (GPUCA_LB_GPUTPCCFDecodeZS)), (, int firstHBF), (, firstHBF))
#endif
#endif
// clang-format on

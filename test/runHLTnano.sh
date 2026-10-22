#!/bin/bash
# Re-emulate the Phase-2 L1T + HLT menu and write an HLTnanoAOD file.
#
# Usage:
#   runHLTnano.sh [nEvents] [inputFile] [outputFile]
#
# Defaults: -n 10, a RelVal TTbar PU sample, file:hltnano.root
#
# The --mc flag is mandatory: the input is an MC sample.

NEV=${1:-10}
INPUT=${2:-/store/relval/CMSSW_20_0_0/RelValTTbar_14TeV/GEN-SIM-DIGI-RAW/PU_150X_mcRun4_realistic_v1_STD_D127_RegeneratedGS_PU_16Aug26-v2/2590000/00f3dc39-4e7d-4438-863b-565ffb3094f0.root}
OUTPUT=${3:-file:hltnano.root}

cmsDriver.py step2 --mc \
  --datatier NANOAODSIM --eventcontent NANOAODSIM \
  --filein ${INPUT} \
  --fileout ${OUTPUT} \
  --customise SLHCUpgradeSimulations/Configuration/aging.customise_aging_1000,HLTrigger/HLTnanoAOD/hltPhase2TriggerNano_cff.customiseHLTnano \
  --era Phase2C26I13M9 --geometry ExtendedRun4D127 \
  --conditions auto:phase2_realistic_T35 \
  --step L1TrackTrigger,L1,L1P2GT,HLT:@relvalRun4 \
  --processName HLT2 \
  -n ${NEV} --nThreads 8 \
  --python_filename hltnano_cfg.py \
  --customise_commands 'process.load("SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff")\ndel process.simEcalUnsuppressedDigis\ndel process.simHcalUnsuppressedDigis\ndel process.simHGCalUnsuppressedDigis\ndel process.simSiPixelDigis\nprocess.TTClusterAssociatorFromPixelDigis.digiSimLinks = cms.InputTag("simSiPixelDigis","Tracker")'

import FWCore.ParameterSet.Config as cms

# Reuse the converter from DPGAnalysis/Phase2L1TNanoAOD (already in the release):
# it turns the Phase-2 Global Trigger algorithm block decisions into an
# edm::TriggerResults product, with one path per L1 algo named
#   <prefix><algoName>_<decision>  ==  "L1_<algo>_final" by default
# so that the L1 decisions are handled with the same TriggerResults
# machinery (and stored in the nano output) as the HLT ones.
l1tP2GTTrigConvert = cms.EDProducer("P2GTTriggerResultsConverter",
    src = cms.InputTag("l1tGTAlgoBlockProducer"),
    prefix = cms.string("L1_"),
    decision = cms.string("final"),  # "beforeBxMaskAndPrescale", "beforePrescale" or "final"
)

# L1 (P2GT) algorithm decisions -> singleton table "L1" with one bool
# branch per algo: L1_<algo>_final
hltNanoL1BitsTable = cms.EDProducer("TriggerResultsFlatTableProducer",
    src = cms.InputTag("l1tP2GTTrigConvert"),
    name = cms.string("L1"),
    doc = cms.string("Phase-2 Level-1 (P2GT) algorithm decisions, one bool per algo"),
    stripFromBranchName = cms.string("L1_"),
)

# HLT path decisions -> singleton table "HLT" with one bool branch per
# path: HLT_<path>. The process name is resolved by the customise hook
# in hltPhase2TriggerNano_cff.py (it must work also when the process was
# renamed with --processName).
hltNanoHLTBitsTable = cms.EDProducer("TriggerResultsFlatTableProducer",
    src = cms.InputTag("TriggerResults"),
    name = cms.string("HLT"),
    doc = cms.string("HLT path decisions, one bool per trigger path"),
    stripFromBranchName = cms.string("HLT_"),
)

hltNanoBitsTask = cms.Task(
    l1tP2GTTrigConvert,
    hltNanoL1BitsTable,
    hltNanoHLTBitsTable,
)

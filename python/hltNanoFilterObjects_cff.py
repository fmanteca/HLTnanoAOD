import FWCore.ParameterSet.Config as cms

# Objects passing a configurable list of HLT filters, read from the
# trigger::TriggerEvent product ("hltTriggerSummaryAOD").
#
# To store more (or fewer) filters, simply edit the `filters` list below
# before running, or override it from the HLT reprocessing config, e.g.:
#   process.hltNanoFilterObjectsTable.filters.append("hltMyOtherFilter")
#
# Only filters configured with saveTags=True appear in TriggerEvent;
# missing ones produce an empty table plus a one-time warning.
# The process name of the InputTag is set by the customise hook
# in hltPhase2TriggerNano_cff.py.
hltNanoFilterObjectsTable = cms.EDProducer("HLTFilterObjectsTableProducer",
    src = cms.InputTag("hltTriggerSummaryAOD"),
    # process name the stored filter tags must carry; set to the current
    # process by the customise hook (protects against picking up filter
    # objects from a previous HLT pass of the input file)
    filterProcess = cms.string(""),
    filters = cms.vstring(
        "hltL3crIsoL1TkSingleMu22L3f24QL3trkIsoRegionalNewFiltered0p07EcalHcalHgcalTrk",
        "hltEle32WPTightGsfTrackIsoL1SeededFilter",
        "hltPhoton187HEL1SeededFilter",
        "hltSingleAK4PFPuppiJet520",
        "hltPFPuppiHT1070",
        "hltHpsPFTau150LooseTauWPDeepTau",
    ),
)

hltNanoFilterObjectsTask = cms.Task(hltNanoFilterObjectsTable)

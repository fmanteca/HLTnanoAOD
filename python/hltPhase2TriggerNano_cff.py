import FWCore.ParameterSet.Config as cms

# Phase-2 HLTnanoAOD: nanoAOD ntuple of trigger information produced while
# re-emulating the Phase-2 L1T + HLT menu. Content:
#   1. one bool branch per Phase-2 L1 (P2GT) algorithm decision  ("L1_<algo>_final")
#   2. one bool branch per HLT path decision                     ("HLT_<path>")
#   3. pt/eta/phi/mass/id of the objects passing a configurable
#      list of HLT filters                                       ("hlt<Filter>_pt", ...)
#
# Entry point: customiseHLTnano(process), for use as a customise of a
# cmsDriver command whose step runs L1TrackTrigger,L1,L1P2GT,HLT, e.g.
#
#   cmsDriver.py step2 --mc -n 10 \
#     --era Phase2C26I13M9 --geometry ExtendedRun4D127 \
#     --conditions auto:phase2_realistic_T35 \
#     --step L1TrackTrigger,L1,L1P2GT,HLT:@relvalRun4 \
#     --processName HLT2 \
#     --datatier NANOAODSIM --eventcontent NANOAODSIM \
#     --fileout file:hltnano.root \
#     --filein <LFN of a Phase-2 GEN-SIM-DIGI-RAW file> \
#     --customise SLHCUpgradeSimulations/Configuration/aging.customise_aging_1000,HLTrigger/HLTnanoAOD/hltPhase2TriggerNano_cff.customiseHLTnano
#
# The nanoAOD output file is owned by cmsDriver (--eventcontent NANOAODSIM
# makes it create a NanoAODOutputModule); this hook only inserts the table
# producers in the schedule and fixes their process-name dependent InputTags.
# NOTE: the modules write nanoaod::FlatTable products, which the
# NanoAODOutputModule turns into branches automatically; keep the module
# label suffix "...Table" so the default "keep nanoaodFlatTable_*Table_*_*"
# output commands match.

from PhysicsTools.NanoAOD.nano_cff import nanoMetadata
from HLTrigger.HLTnanoAOD.hltNanoBits_cff import hltNanoBitsTask
from HLTrigger.HLTnanoAOD.hltNanoFilterObjects_cff import hltNanoFilterObjectsTask

hltPhase2TriggerNanoTask = cms.Task(nanoMetadata)
hltPhase2TriggerNanoTask.add(hltNanoBitsTask, hltNanoFilterObjectsTask)
hltPhase2TriggerNanoPath = cms.Path(cms.Sequence(hltPhase2TriggerNanoTask))


def customiseHLTnano(process):
    """Append the Phase-2 HLT nanoAOD table producers to a process that runs
    the Phase-2 L1T emulation + HLT menu, to be written out by the
    cmsDriver-created NanoAODOutputModule (--eventcontent NANOAODSIM)."""
    process.load("HLTrigger.HLTnanoAOD.hltNanoBits_cff")
    process.load("HLTrigger.HLTnanoAOD.hltNanoFilterObjects_cff")
    process.load("HLTrigger.HLTnanoAOD.hltPhase2TriggerNano_cff")

    # TriggerResults and trigger::TriggerEvent are produced inside this
    # process, whose name may have been changed with --processName. Since the
    # menu's TriggerSummaryProducerAOD packs filter tags of ALL processes, the
    # object tables must also require the current process name.
    proc = process.name_()
    process.hltNanoHLTBitsTable.src = cms.InputTag("TriggerResults", "", proc)
    process.hltNanoFilterObjectsTable.src = cms.InputTag("hltTriggerSummaryAOD", "", proc)
    process.hltNanoFilterObjectsTable.filterProcess = proc

    # Insert the nano path before the first end path (the NanoAODOutputModule
    # is already on one), so the tables exist when the event is written.
    indexes = [i for i, p in enumerate(process.schedule) if isinstance(p, cms.EndPath)]
    if indexes:
        process.schedule.insert(indexes[0], process.hltPhase2TriggerNanoPath)
    else:
        process.schedule.append(process.hltPhase2TriggerNanoPath)

    return process

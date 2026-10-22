import FWCore.ParameterSet.Config as cms

_exec_globals = {}
exec(open("hltnano_cfg.py").read(), _exec_globals)
process = _exec_globals["process"]

print("process name:", process.name_())

prods = process.producers_()
print("\n== key producers ==")
for label in ["TriggerResults", "hltTriggerSummaryAOD", "l1tGTAlgoBlockProducer",
              "l1tP2GTTrigConvert", "hltNanoL1BitsTable", "hltNanoHLTBitsTable",
              "hltNanoFilterObjectsTable"]:
    m = prods.get(label, None)
    print(f"{label}: {'MISSING' if m is None else m.type_()}")

if "hltTriggerSummaryAOD" in prods:
    print("  hltTriggerSummaryAOD.processName =", prods["hltTriggerSummaryAOD"].processName)
if "hltNanoHLTBitsTable" in prods:
    print("  hltNanoHLTBitsTable.src =", prods["hltNanoHLTBitsTable"].src)
    print("  hltNanoFilterObjectsTable.src =", prods["hltNanoFilterObjectsTable"].src)

print("\n== six HLT filters (saveTags flag) ==")
for f in ["hltL3crIsoL1TkSingleMu22L3f24QL3trkIsoRegionalNewFiltered0p07EcalHcalHgcalTrk",
          "hltEle32WPTightGsfTrackIsoL1SeededFilter", "hltPhoton187HEL1SeededFilter",
          "hltSingleAK4PFPuppiJet520", "hltPFPuppiHT1070", "hltHpsPFTau150LooseTauWPDeepTau"]:
    m = getattr(process, f, None)
    print(f"{f}: {'MISSING' if m is None else m.type_() + ' saveTags=' + str(getattr(m, 'saveTags', 'n/a'))}")

print("\n== schedule (last entries) ==")
sched = process.schedule
if sched is None:
    print("no explicit schedule")
else:
    for i, p in enumerate(sched):
        print(i, p.label() if hasattr(p, 'label')
              else 'unknown', 'EndPath' if isinstance(p, cms.EndPath) else 'Path')

print("\n== output module ==")
om = getattr(process, "NANOAODSIMoutput", None)
print("NANOAODSIMoutput:", om.type_() if om else "MISSING")
if om:
    print("keeps (first 6):", list(om.outputCommands)[:6])

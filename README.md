# HLTrigger/HLTnanoAOD — Phase-2 HLTnanoAOD

nanoAOD ntuple of **trigger information** produced while re-emulating the
Phase-2 L1T + HLT menu (steps `L1TrackTrigger,L1,L1P2GT,HLT`) in one job.
The L1 analogue for reference is `DPGAnalysis/Phase2L1TNanoAOD`.

## Output content (branches of the `Events` tree)

| Branch pattern | Producer | Meaning |
|---|---|---|
| `L1_<algoName>_final` | `l1tP2GTTrigConvert` + `hltNanoL1BitsTable` | one `bool` per Phase-2 L1 (P2GT) algorithm, final post-prescale decision |
| `HLT_<pathName>` | `hltNanoHLTBitsTable` | one `bool` per HLT path decision |
| `n<filter>`, `<filter>_pt/eta/phi/mass/id` | `hltNanoFilterObjectsTable` | kinematics of the trigger objects passing each configured HLT filter |

The naming is the standard nanoAOD `TableName_column` convention; the bit
tables are singleton tables, so `L1_...` / `HLT_...` branches are scalars,
while the object tables are per-event collections (`n<Filter>` counts).

## How it works

1. **L1 bits** — the Phase-2 GT emulator (`l1tGTAlgoBlockProducer`) writes a
   `P2GTAlgoBlockMap` per event. `P2GTTriggerResultsConverter` (reused from
   `DPGAnalysis/Phase2L1TNanoAOD`) converts it to an `edm::TriggerResults`
   with one path named `L1_<algo>_final` per algorithm.
2. **HLT bits** — the job-wide HLT path decisions are stored (by the framework)
   in `edm::TriggerResults`.
3. Both products are flattened to one-bool-column singleton `nanoaod::FlatTable`s
   by `TriggerResultsFlatTableProducer` (`plugins/`). Columns are frozen at the
   first event; if the menu changes across runs of the input the decisions are
   recovered by path name.
4. **Filter objects** — `HLTFilterObjectsTableProducer` (`plugins/`) reads the
   `trigger::TriggerEvent` product (`hltTriggerSummaryAOD`), looks up the
   user-configured filter labels, and writes a `nanoaod::FlatTable`
   (`pt/eta/phi/mass/id`) per filter. Only filters with `saveTags=True`
   appear in `TriggerEvent`; missing ones give an empty table + one-time warning.
   The lookup matches on label *and* process name (`filterProcess` parameter):
   the menu's summary producer runs with `processName="*"`, so the filter map
   contains each filter both for this job (`HLT2`) and for the input file's
   old HLT pass (`HLT`) — without process matching one could silently store
   the old menu's objects. (The summary producer also fires one harmless
   `LogError` per event, "Mismatch in number of filter tags", because of this
   double counting.)

`NanoAODOutputModule` picks up the `nanoaod::FlatTable` products automatically
(NANOAODSIM output commands keep `nanoaodFlatTable_*Table_*_*`), so no
EDM-product output configuration is needed.

## Configuration

All user knobs live in python:

* `python/hltNanoBits_cff.py` — L1/HLT bits tables. L1 product tag
  (`l1tGTAlgoBlockProducer`) and decision flavour
  (`final` | `beforePrescale` | `beforeBxMaskAndPrescale`) are set here.
* `python/hltNanoFilterObjects_cff.py` — editable choices:
  ```python
  hltNanoFilterObjectsTable = cms.EDProducer("HLTFilterObjectsTableProducer",
      src = cms.InputTag("hltTriggerSummaryAOD"),
      filters = cms.vstring(
          "hltL3crIsoL1TkSingleMu22L3f24QL3trkIsoRegionalNewFiltered0p07EcalHcalHgcalTrk",
          "hltEle32WPTightGsfTrackIsoL1SeededFilter",
          "hltPhoton187HEL1SeededFilter",
          "hltSingleAK4PFPuppiJet520",
          "hltPFPuppiHT1070",
          "hltHpsPFTau150LooseTauWPDeepTau",
      ),
  )
  ```
  Add or remove filters by editing this list (saveTags=True filter labels of
  the HLT menu), or override at run time, e.g. add to the cmsDriver command:
  `--customise_commands 'process.hltNanoFilterObjectsTable.filters += ["hltMyNewFilter"]'`
* `python/hltPhase2TriggerNano_cff.py` — glues the tasks and provides the
  `customiseHLTnano(process)` hook used by cmsDriver.

Process-name independent: `TriggerResults` and `hltTriggerSummaryAOD` tags are
resolved to the current process name, so `--processName HLT2` (or any other)
works unchanged.

Validated end-to-end on 30 events of `RelValTTbar_14TeV` (PU, CMSSW_20_0_0
input) with menu `@relvalRun4` in CMSSW_20_1_0_pre4: 80 L1-bit branches, 149
HLT-path-bit branches, and all six default filter tables filled in the events
where the corresponding path fires (see `../../../HLTnanoAOD.md` for the
numbers). Filter labels are menu-dependent — re-check with
`test/check_cfg.py` if your menu/release differs.

## Running

See `test/runHLTnano.sh` and the instructions in `../../../HLTnanoAOD.md`.
In short (build must already be done: `cmsenv && scram b`):

```bash
cmsDriver.py step2 --mc \
  --datatier NANOAODSIM --eventcontent NANOAODSIM \
  --filein /store/relval/CMSSW_20_0_0/RelValTTbar_14TeV/GEN-SIM-DIGI-RAW/PU_150X_mcRun4_realistic_v1_STD_D127_RegeneratedGS_PU_16Aug26-v2/2590000/00f3dc39-4e7d-4438-863b-565ffb3094f0.root \
  --fileout file:hltnano.root \
  --customise SLHCUpgradeSimulations/Configuration/aging.customise_aging_1000,HLTrigger/HLTnanoAOD/hltPhase2TriggerNano_cff.customiseHLTnano \
  --era Phase2C26I13M9 --geometry ExtendedRun4D127 \
  --conditions auto:phase2_realistic_T35 \
  --step L1TrackTrigger,L1,L1P2GT,HLT:@relvalRun4 \
  --processName HLT2 \
  -n 10 --nThreads 8 \
  --customise_commands 'process.load("SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff")\ndel process.simEcalUnsuppressedDigis\ndel process.simHcalUnsuppressedDigis\ndel process.simHGCalUnsuppressedDigis\ndel process.simSiPixelDigis\nprocess.TTClusterAssociatorFromPixelDigis.digiSimLinks = cms.InputTag("simSiPixelDigis","Tracker")'
```

Notes on the differences w.r.t. naive `FEVTDEBUGHLT` attempts:
* `--mc` is **required** (MC input conditions); its absence is the most common
  reason for the command to fail.
* `--eventcontent NANOAODSIM` makes cmsDriver create a `NanoAODOutputModule`
  (nanoAOD flat file) instead of an EDM ROOT output.
* The customisation appends the table producers before the output end-path.

## Files

```
HLTrigger/HLTnanoAOD/
├── BuildFile.xml
├── plugins/BuildFile.xml
├── plugins/TriggerResultsFlatTableProducer.cc   # TriggerResults -> singleton FlatTable (bits)
├── plugins/HLTFilterObjectsTableProducer.cc     # TriggerEvent -> per-filter object tables
├── python/hltNanoBits_cff.py                    # L1 + HLT bits (configs)
├── python/hltNanoFilterObjects_cff.py           # configurable filter list
├── python/hltPhase2TriggerNano_cff.py           # task, path, customiseHLTnano
└── test/runHLTnano.sh                           # one-liner driver (see header)
```

## Towards the CMSSW PR

* package is fully self-contained under `HLTrigger/` as requested;
* only external re-use is `P2GTTriggerResultsConverter` from
  `DPGAnalysis/Phase2L1TNanoAOD` (already in the release);
* natural follow-up: register an `autoNANO` handle (e.g. `@Phase2HLTnano`) in
  `PhysicsTools/NanoAOD/python/autoNANO.py` so the step becomes
  `NANO:@Phase2HLTnano`, mirroring `NANO:@Phase2L1DPGwithGen`.

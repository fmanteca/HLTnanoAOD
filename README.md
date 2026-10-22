# HLTrigger/HLTnanoAOD — Phase-2 HLTnanoAOD

nanoAOD ntuple of **trigger information** for Phase-2, produced together with
the re-emulation of the Phase-2 L1T + HLT menu (steps
`L1TrackTrigger,L1,L1P2GT,HLT`) in one job.
The L1 analogue for reference is `DPGAnalysis/Phase2L1TNanoAOD`.

Developed/validated in `CMSSW_20_1_0_pre4` (el9_amd64_gcc14) on
`RelValTTbar_14TeV` PU (`PU_150X_mcRun4_realistic_v1_STD_D127` input), menu
`@relvalRun4`.

## Output content (branches of the `Events` tree)

| Branch pattern | Producer | Meaning |
|---|---|---|
| `L1_<algoName>_final` | `l1tP2GTTrigConvert` + `hltNanoL1BitsTable` | one `bool` per Phase-2 L1 (P2GT) algorithm, final post-prescale decision; algo names from the loaded GT menu (`step1_2024`), incl. its `<algo>_pHLT2` duplicates |
| `HLT_<pathName>` | `hltNanoHLTBitsTable` | one `bool` per path of the process: HLT menu paths (`_pHLT`/`_pHLT2` variants included), plus step/L1-emulator paths (`HLT_L1TrackTrigger_step`, `HLT_pDoubleEGEle37_24`, ...) |
| `n<filter>`, `<filter>_pt/eta/phi/mass/id` | `hltNanoFilterObjectsTable` | kinematics of the trigger objects passing each configured HLT filter |

Naming follows the standard nanoAOD `TableName_column` convention; the bit
tables are singleton tables (`L1_...`/`HLT_...` branches are scalars), the
object tables are per-event collections (`n<Filter>` counters).

Verified end-to-end on 30 RelVal TTbar+PU events (menu `@relvalRun4`):

* 80 `L1_` trigger-bit branches (60 with at least one event passing),
* 149 `HLT_` path-bit branches (106 ever passed), e.g.
  `HLT_Ele32_WPTight_L1Seeded` (5 events), `HLT_IsoMu24_FromL1TkMuon` (3),
  `HLT_AK4PFPuppiJet520` (1), `HLT_LooseDeepTauPFTauHPS150_L1NN_eta2p1` (1);
* object tables non-empty exactly where the matching path fires: muon L3 filter
  3 events (pT 82.9/55.9/66.8 GeV), Ele32 5, `HT1070` 13, HPS tau 1 (pT
  216.7), AK4PFPuppiJet520 1 (pT 529.3), Photon187 0 (path never fires in 30
  events — expected).

## Setup from scratch

```bash
# 1. create the release area
cmsrel CMSSW_20_1_0_pre4
cd CMSSW_20_1_0_pre4/src
cmsenv

# 2. install the package
git clone https://github.com/fmanteca/HLTnanoAOD.git HLTrigger/HLTnanoAOD
#   (or ssh: git clone git@github.com:fmanteca/HLTnanoAOD.git HLTrigger/HLTnanoAOD)
#   Once the package is merged/available on a cmssw branch, use the usual
#   git cms-init + git cms-checkout-topic flow instead.

# 3. build
scram b -j8
```

Quick end-to-end test (30 events, output `hltnano.root`):

```bash
cd HLTrigger/HLTnanoAOD/test
./runHLTnano.sh 30        # writes hltnano_cfg.py and runs cmsRun on it
# or: ./runHLTnano.sh <nEvents> <inputFileOrLFN> <outputFile>
```

A reference `hltnano.root` (30 events) and its `hltnano_run.log` are kept in
`test/` — regenerate anytime. To introspect a generated config (which menu
filters have `saveTags=True`, modules/schedule), run
`python3 check_cfg.py` (needs a `hltnano_cfg.py` next to it).

## Full working example (cmsDriver)

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
  -n 30 --nThreads 8 \
  --python_filename hltnano_cfg.py \
  --customise_commands 'process.load("SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff")\ndel process.simEcalUnsuppressedDigis\ndel process.simHcalUnsuppressedDigis\ndel process.simHGCalUnsuppressedDigis\ndel process.simSiPixelDigis\nprocess.TTClusterAssociatorFromPixelDigis.digiSimLinks = cms.InputTag("simSiPixelDigis","Tracker")'
```

Drop `--python_filename` to run directly; keep it + `cmsRun hltnano_cfg.py`
to rerun the identical job. Note that `--mc` is mandatory here (the input is
MC), and `--eventcontent NANOAODSIM` is what makes cmsDriver create a
`NanoAODOutputModule` (nanoAOD flat file) instead of an EDM ROOT output.

## How it works

1. **L1 bits** — the Phase-2 GT emulator (`l1tGTAlgoBlockProducer`) writes a
   `P2GTAlgoBlockMap` per event. `P2GTTriggerResultsConverter` (reused from
   `DPGAnalysis/Phase2L1TNanoAOD`) converts it to an `edm::TriggerResults`
   with one path named `L1_<algo>_final` per algorithm.
2. **HLT bits** — the job-wide path decisions live in the framework-produced
   `edm::TriggerResults` of the process (all executed paths are included).
3. Both products are flattened to one-bool-column singleton
   `nanoaod::FlatTable`s by `TriggerResultsFlatTableProducer` (`plugins/`).
   Columns are frozen at the first event; if the menu changes across runs of
   the input, decisions are recovered by path name.
4. **Filter objects** — `HLTFilterObjectsTableProducer` (`plugins/`) reads the
   `trigger::TriggerEvent` product (`hltTriggerSummaryAOD`), looks up the
   user-configured filter labels, and writes one `nanoaod::FlatTable`
   (`pt/eta/phi/mass/id`) per filter. Only filters with `saveTags=True`
   appear in `TriggerEvent`; missing ones give an empty table + a one-time
   warning. The lookup matches on label **and** process name (`filterProcess`
   parameter, set by the customise hook): the menu's summary producer runs
   with `processName="*"`, so the filter map contains each filter both for
   this job (`HLT2`) **and** for the input file's previous HLT pass (`HLT`) —
   without process matching one would silently store the old menu's objects.
   (The summary producer also prints one harmless `LogError` per event,
   "Mismatch in number of filter tags: 159!=299", because of this double
   counting; the product is complete regardless.)

`NanoAODOutputModule` picks up the `nanoaod::FlatTable` products automatically
(the NANOAODSIM output commands keep `nanoaodFlatTable_*Table_*_*`), so no
EDM-product output configuration is needed.

## User configuration

Everything is steered from python (no recompilation):

* **HLT object list** — edit `python/hltNanoFilterObjects_cff.py`, `filters`
  vstring (labels of menu filters with `saveTags=True`):
  ```python
  hltNanoFilterObjectsTable = cms.EDProducer("HLTFilterObjectsTableProducer",
      src = cms.InputTag("hltTriggerSummaryAOD"),
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
  ```
  Add/remove filters by editing the list, or per job after the customise:
  `--customise_commands 'process.hltNanoFilterObjectsTable.filters += ["hltMyFilter"]'`
* **L1 decision flavour** — `decision = final | beforePrescale |
  beforeBxMaskAndPrescale` in `l1tP2GTTrigConvert`
  (`python/hltNanoBits_cff.py`); branch suffix changes accordingly.
* **L1 product tag** — P2GT algo-block collection tag
  (default `l1tGTAlgoBlockProducer`), same file.

Process-name independent: `TriggerResults`/`hltTriggerSummaryAOD` tags and the
`filterProcess` guard are resolved by the customise hook to the current
process name, so `--processName HLT2` (or anything else) works unchanged.

Caveats:

* filter labels are menu-dependent — the six defaults are validated for
  `@relvalRun4` of CMSSW_20_1_0_pre4 (all present, `saveTags=True`); re-check
  with `test/check_cfg.py` when changing menu/release;
* empty object tables + once-per-stream `HLTnanoAOD` LogWarning for a filter
  just mean the filter was not executed in those events (path not run), not
  an error.

## Files

```
HLTnanoAOD/
├── BuildFile.xml
├── plugins/BuildFile.xml
├── plugins/TriggerResultsFlatTableProducer.cc   # TriggerResults -> singleton FlatTable (bits)
├── plugins/HLTFilterObjectsTableProducer.cc     # TriggerEvent  -> per-filter object tables
├── python/hltNanoBits_cff.py                    # L1 + HLT bits (configs)
├── python/hltNanoFilterObjects_cff.py           # configurable filter list
├── python/hltPhase2TriggerNano_cff.py           # task, path, customiseHLTnano
├── test/runHLTnano.sh                           # one-liner driver (see header)
├── test/check_cfg.py                            # config/menu introspection utility
└── test/hltnano.root, hltnano_run.log           # reference 30-event output
```

## Towards the CMSSW PR

* the package is fully self-contained under `HLTrigger/`;
* the only re-used package is `DPGAnalysis/Phase2L1TNanoAOD`
  (`P2GTTriggerResultsConverter`, already in the release);
* natural follow-up: register an `autoNANO` handle (e.g. `@Phase2HLTnano`) in
  `PhysicsTools/NanoAOD/python/autoNANO.py` so the step becomes
  `NANO:@Phase2HLTnano`, mirroring `NANO:@Phase2L1DPGwithGen`.

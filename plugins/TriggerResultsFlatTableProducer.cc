//
// TriggerResultsFlatTableProducer
//
// Flattens an edm::TriggerResults product into a singleton nanoaod::FlatTable:
// one bool column per trigger path, named after the path (optionally
// stripped of a configurable prefix). Columns are frozen at the first
// processed event, so the output branches stay stable for the whole job
// even if the trigger menu changes between runs of the input.
//

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/NanoAOD/interface/FlatTable.h"

#include <string>
#include <vector>

class TriggerResultsFlatTableProducer : public edm::stream::EDProducer<> {
public:
  explicit TriggerResultsFlatTableProducer(const edm::ParameterSet&);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void produce(edm::Event&, const edm::EventSetup&) override;

  const edm::EDGetTokenT<edm::TriggerResults> resultsToken_;
  const std::string tableName_;
  const std::string doc_;
  const std::string stripFromBranchName_;

  // frozen at the first event: path names as stored in TriggerResults
  // and the corresponding (possibly stripped) column names
  std::vector<std::string> pathNames_;
  std::vector<std::string> columnNames_;
};

TriggerResultsFlatTableProducer::TriggerResultsFlatTableProducer(const edm::ParameterSet& params)
    : resultsToken_(consumes<edm::TriggerResults>(params.getParameter<edm::InputTag>("src"))),
      tableName_(params.getParameter<std::string>("name")),
      doc_(params.getParameter<std::string>("doc")),
      stripFromBranchName_(params.getParameter<std::string>("stripFromBranchName")) {
  produces<nanoaod::FlatTable>();
}

void TriggerResultsFlatTableProducer::produce(edm::Event& event, const edm::EventSetup&) {
  const auto& results = event.get(resultsToken_);
  const auto& names = event.triggerNames(results);

  if (columnNames_.empty()) {
    // freeze the column definition at the first event of the job
    pathNames_.reserve(names.size());
    columnNames_.reserve(names.size());
    for (unsigned int i = 0; i < names.size(); ++i) {
      std::string column = names.triggerName(i);
      pathNames_.push_back(column);
      if (!stripFromBranchName_.empty()) {
        const auto pos = column.find(stripFromBranchName_);
        if (pos != std::string::npos) {
          column.erase(pos, stripFromBranchName_.size());
        }
      }
      columnNames_.push_back(std::move(column));
    }
  }

  // fast path: same paths as at the first event, same order
  bool directMap = (names.size() == pathNames_.size());
  if (directMap) {
    for (unsigned int i = 0; i < names.size(); ++i) {
      if (names.triggerName(i) != pathNames_[i]) {
        directMap = false;
        break;
      }
    }
  }

  auto out = std::make_unique<nanoaod::FlatTable>(1, tableName_, true);
  out->setDoc(doc_);
  for (unsigned int icol = 0; icol < columnNames_.size(); ++icol) {
    bool decision = false;
    if (directMap) {
      decision = results.accept(icol);
    } else {
      // menu changed between runs: recover by path name, missing paths get false
      const unsigned int idx = names.triggerIndex(pathNames_[icol]);
      if (idx < names.size()) {
        decision = results.accept(idx);
      }
    }
    out->addColumnValue<bool>(columnNames_[icol], decision, "decision of trigger path " + pathNames_[icol]);
  }
  event.put(std::move(out));
}

void TriggerResultsFlatTableProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("src", edm::InputTag("TriggerResults"))
      ->setComment("TriggerResults product to flatten into one bool column per trigger path");
  desc.add<std::string>("name", "TriggerBits")
      ->setComment("FlatTable name (used as branch prefix by the NanoAODOutputModule)");
  desc.add<std::string>("doc", "trigger decisions")->setComment("documentation string of the table");
  desc.add<std::string>("stripFromBranchName", "")
      ->setComment("substring stripped from path names to form branch names, e.g. \"HLT_\"");
  descriptions.addWithDefaultLabel(desc);
}

DEFINE_FWK_MODULE(TriggerResultsFlatTableProducer);

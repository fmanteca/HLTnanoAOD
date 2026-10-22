//
// HLTFilterObjectsTableProducer
//
// Writes one nanoaod::FlatTable per configured HLT filter, containing the
// kinematics of the trigger objects that passed the filter, as recorded
// in the trigger::TriggerEvent product ("hltTriggerSummaryAOD").
//
// NOTE: only filters configured with saveTags=True appear in the
// TriggerEvent product. Filters not present get an empty table and a
// one-time LogWarning.
//

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/HLTReco/interface/TriggerObject.h"
#include "DataFormats/NanoAOD/interface/FlatTable.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

class HLTFilterObjectsTableProducer : public edm::stream::EDProducer<> {
public:
  explicit HLTFilterObjectsTableProducer(const edm::ParameterSet&);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void produce(edm::Event&, const edm::EventSetup&) override;

  const edm::EDGetTokenT<trigger::TriggerEvent> triggerEventToken_;
  const std::string filterProcess_;  // required process name of the stored filter tags, empty = any
  const std::vector<std::string> filterLabels_;
  std::vector<bool> warned_;  // one-time "filter not found in TriggerEvent" warnings
};

HLTFilterObjectsTableProducer::HLTFilterObjectsTableProducer(const edm::ParameterSet& params)
    : triggerEventToken_(consumes<trigger::TriggerEvent>(params.getParameter<edm::InputTag>("src"))),
      filterProcess_(params.getParameter<std::string>("filterProcess")),
      filterLabels_(params.getParameter<std::vector<std::string>>("filters")),
      warned_(filterLabels_.size(), false) {
  for (const auto& filterLabel : filterLabels_) {
    produces<nanoaod::FlatTable>(filterLabel);
  }
}

void HLTFilterObjectsTableProducer::produce(edm::Event& event, const edm::EventSetup&) {
  const auto& triggerEvent = event.get(triggerEventToken_);
  const auto& triggerObjects = triggerEvent.getObjects();

  for (unsigned int ifilter = 0; ifilter < filterLabels_.size(); ++ifilter) {
    const auto& filterLabel = filterLabels_[ifilter];

    trigger::Keys keys;
    // find the filter: match the module label and, when filterProcess is
    // set, the process name of the stored filter tag. The latter is needed
    // because hltTriggerSummaryAOD is configured with processName="*": the
    // filter map then also contains tags from a previous HLT pass of the
    // input file (e.g. process "HLT"), pointing to the old menu's objects.
    // An exact InputTag match can never work either, because the
    // TriggerEvent itself reports usedProcessName()="*". "Last filter wins".
    trigger::size_type index = triggerEvent.sizeFilters();
    for (trigger::size_type i = 0; i != triggerEvent.sizeFilters(); ++i) {
      const auto& tag = triggerEvent.filterTag(i);
      if (tag.label() == filterLabel && (filterProcess_.empty() || tag.process() == filterProcess_)) {
        index = i;
      }
    }
    if (index >= triggerEvent.sizeFilters()) {
      if (!warned_[ifilter]) {
        edm::LogWarning("HLTnanoAOD")
            << "filter \"" << filterLabel
            << "\" not found in the TriggerEvent product of this event: it is either not in the menu, not "
               "executed (path not run in this event), or not configured with saveTags=True. "
               "Producing an empty table.";
        warned_[ifilter] = true;
      }
    } else {
      keys = triggerEvent.filterKeys(index);
    }

    std::vector<float> pt, eta, phi, mass;
    std::vector<int> id;
    pt.reserve(keys.size());
    eta.reserve(keys.size());
    phi.reserve(keys.size());
    mass.reserve(keys.size());
    id.reserve(keys.size());
    // row order follows the TriggerEvent insertion order; sort by pT (descending)
    // so downstream users can safely cut on array position (e.g. leading object)
    std::vector<trigger::size_type> order(keys.size());
    std::iota(order.begin(), order.end(), trigger::size_type(0));
    std::sort(order.begin(), order.end(), [&](const trigger::size_type a, const trigger::size_type b) {
      return triggerObjects[keys[a]].pt() > triggerObjects[keys[b]].pt();
    });
    for (const auto i : order) {
      const auto& obj = triggerObjects[keys[i]];
      pt.push_back(obj.pt());
      eta.push_back(obj.eta());
      phi.push_back(obj.phi());
      mass.push_back(obj.mass());
      id.push_back(obj.id());
    }

    auto table = std::make_unique<nanoaod::FlatTable>(pt.size(), filterLabel, false);
    const std::string doc = "trigger objects passing the HLT filter " + filterLabel;
    table->setDoc(doc);
    table->addColumn<float>("pt", pt, "object pT");
    table->addColumn<float>("eta", eta, "object pseudorapidity");
    table->addColumn<float>("phi", phi, "object azimuthal angle");
    table->addColumn<float>("mass", mass, "object mass");
    table->addColumn<int>("id", id, "trigger::TriggerObject id (pdgId channel)");
    event.put(std::move(table), filterLabel);
  }
}

void HLTFilterObjectsTableProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("src", edm::InputTag("hltTriggerSummaryAOD"))
      ->setComment("trigger::TriggerEvent product to read the filter objects from");
  desc.add<std::string>("filterProcess", "")
      ->setComment("require the stored filter tags to carry this process name (empty = any process)");
  desc.add<std::vector<std::string>>("filters", {})
      ->setComment("HLT filter labels (saveTags=True) whose passing objects are stored, one table per filter");
  descriptions.addWithDefaultLabel(desc);
}

DEFINE_FWK_MODULE(HLTFilterObjectsTableProducer);

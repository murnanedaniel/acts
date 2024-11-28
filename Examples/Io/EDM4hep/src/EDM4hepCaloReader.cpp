#include "ActsExamples/Io/EDM4hep/EDM4hepCaloReader.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/EventData/EDM4hepCaloHit.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/SimCalorimeterHitCollection.h>
#include <podio/Frame.h>
#include <tbb/enumerable_thread_specific.h>

namespace ActsExamples {

EDM4hepCaloReader::EDM4hepCaloReader(const Config& cfg, Acts::Logging::Level level)
    : IReader(),
      m_cfg(cfg),
      m_logger(Acts::getDefaultLogger("EDM4hepCaloReader", level)) {
  if (m_cfg.inputPath.empty()) {
    throw std::invalid_argument("Missing input filename");
  }
  if (m_cfg.inputCaloHits.empty()) {
    throw std::invalid_argument("Missing input collection names");
  }
  if (m_cfg.outputCaloHits.empty()) {
    throw std::invalid_argument("Missing output collection name");
  }

  m_eventsRange = std::make_pair(0, reader().getEntries("events"));
  m_outputCaloHits.initialize(m_cfg.outputCaloHits);
}

std::pair<std::size_t, std::size_t> EDM4hepCaloReader::availableEvents() const {
  return m_eventsRange;
}

ProcessCode EDM4hepCaloReader::read(const AlgorithmContext& ctx) {
  podio::Frame frame = reader().readEntry("events", ctx.eventNumber);

  std::vector<Acts::EDM4hepCaloHit> caloHits;
  caloHits.reserve(1000);

  for (const auto& colName : m_cfg.inputCaloHits) {
    const auto& hitCollection = frame.get<edm4hep::SimCalorimeterHitCollection>(colName);
    if (!hitCollection.isValid()) {
      ACTS_WARNING("Collection " << colName << " not found");
      continue;
    }
    
    for (const auto& hit : hitCollection) {
      Acts::EDM4hepCaloHit caloHit;
      caloHit.position = Acts::Vector3(hit.getPosition().x, hit.getPosition().y, hit.getPosition().z);
      caloHit.energy = hit.getEnergy();
      caloHit.time = 0.0;
      caloHit.cellID = hit.getCellID();
      
      caloHits.push_back(std::move(caloHit));
    }
  }

  ACTS_DEBUG("Read " << caloHits.size() << " calorimeter hits");
  m_outputCaloHits(ctx, std::move(caloHits));

  return ProcessCode::SUCCESS;
}

Acts::PodioUtil::ROOTReader& EDM4hepCaloReader::reader() {
  bool exists = false;
  auto& reader = m_reader.local(exists);
  if (!exists) {
    reader.openFile(m_cfg.inputPath);
  }
  return reader;
}

} // namespace ActsExamples 
#include "ActsExamples/Io/EDM4hep/EDM4hepCaloHitWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <stdexcept>

#include <edm4hep/SimCalorimeterHit.h>
#include <podio/Frame.h>

namespace ActsExamples {

EDM4hepCaloHitWriter::EDM4hepCaloHitWriter(const Config& config,
                                           Acts::Logging::Level level)
    : WriterT(config.inputCaloHits, "EDM4hepCaloHitWriter", level),
      m_cfg(config),
      m_writer(config.outputPath) {
  ACTS_VERBOSE("Created output file " << config.outputPath);

  if (m_cfg.inputCaloHits.empty()) {
    throw std::invalid_argument("Missing input calorimeter hits collection");
  }

  if (m_cfg.outputCaloHits.empty()) {
    throw std::invalid_argument("Missing output calorimeter hits name");
  }
}

ProcessCode EDM4hepCaloHitWriter::writeT(
    const AlgorithmContext& ctx,
    const std::vector<Acts::EDM4hepCaloHit>& caloHits) {
  
  // Convert ACTS EDM4hepCaloHits to EDM4hep SimCalorimeterHits
  for (const auto& hit : caloHits) {
    auto edm4hepHit = m_hitCollection.create();
    
    // Set the position
    edm4hepHit.setPosition({
        static_cast<float>(hit.position.x()),
        static_cast<float>(hit.position.y()),
        static_cast<float>(hit.position.z())
    });
    
    // Set the energy
    edm4hepHit.setEnergy(hit.energy);
    
    // Combine event ID and cell ID
    uint64_t combinedID = (static_cast<uint64_t>(ctx.eventNumber) << 32) | 
                         (hit.cellID & 0xFFFFFFFF);
    edm4hepHit.setCellID(combinedID);
  }

  // Only write if not using event store
  if (!m_cfg.useEventStore) {
    podio::Frame frame;
    frame.put(std::move(m_hitCollection), m_cfg.outputCaloHits);
    
    std::lock_guard lock{m_writeMutex};
    m_writer.writeFrame(frame, "events");
    m_hitCollection.clear();
  }

  return ProcessCode::SUCCESS;
}

ProcessCode EDM4hepCaloHitWriter::finalize() {
  if (m_cfg.useEventStore) {
    podio::Frame frame;
    frame.put(std::move(m_hitCollection), m_cfg.outputCaloHits);
    
    std::lock_guard lock{m_writeMutex};
    m_writer.writeFrame(frame, "events");
  }
  m_writer.finish();
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples 
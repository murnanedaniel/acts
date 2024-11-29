#include "ActsExamples/Io/Root/RootCaloHitWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <ios>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootCaloHitWriter::RootCaloHitWriter(
    const ActsExamples::RootCaloHitWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputCaloHits, "RootCaloHitWriter", level), m_cfg(config) {
  // inputCaloHits is already checked by base constructor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file path");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // Open output file and create tree
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open output file '" + m_cfg.filePath +
                                "'");
  }
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // Setup the branches
  m_outputTree->Branch("event_id", &m_eventId);
  m_outputTree->Branch("cell_id", &m_cellId, "cell_id/l");
  m_outputTree->Branch("x", &m_x);
  m_outputTree->Branch("y", &m_y);
  m_outputTree->Branch("z", &m_z);
  m_outputTree->Branch("energy", &m_energy);
}

ActsExamples::RootCaloHitWriter::~RootCaloHitWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootCaloHitWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_VERBOSE("Wrote hits to tree '" << m_cfg.treeName << "' in '"
                                     << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootCaloHitWriter::writeT(
    const AlgorithmContext& ctx,
    const std::vector<Acts::EDM4hepCaloHit>& hits) {
  // Ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // Get the event number
  m_eventId = ctx.eventNumber;
  for (const auto& hit : hits) {
    m_cellId = hit.cellID;
    // Write hit position
    m_x = hit.position.x() / Acts::UnitConstants::mm;
    m_y = hit.position.y() / Acts::UnitConstants::mm;
    m_z = hit.position.z() / Acts::UnitConstants::mm;
    // Write energy
    m_energy = hit.energy / Acts::UnitConstants::GeV;
    // Fill the tree
    m_outputTree->Fill();
  }
  return ProcessCode::SUCCESS;
} 
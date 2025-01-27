// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootParticleFlatWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <cstdint>
#include <ios>
#include <ostream>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

ActsExamples::RootParticleFlatWriter::RootParticleFlatWriter(
    const ActsExamples::RootParticleFlatWriter::Config& cfg,
    Acts::Logging::Level lvl)
    : WriterT(cfg.inputParticles, "RootParticleFlatWriter", lvl), m_cfg(cfg) {
  // inputParticles is already checked by base constructor
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file path");
  }
  if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // open root file and create the tree
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
  m_outputFile->cd();
  m_outputTree = new TTree(m_cfg.treeName.c_str(), m_cfg.treeName.c_str());
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // setup the branches
  m_outputTree->Branch("event_id", &m_eventId);
  m_outputTree->Branch("particle_id", &m_particleId);
  m_outputTree->Branch("particle_type", &m_particleType);
  m_outputTree->Branch("process", &m_process);
  m_outputTree->Branch("vx", &m_vx);
  m_outputTree->Branch("vy", &m_vy);
  m_outputTree->Branch("vz", &m_vz);
  m_outputTree->Branch("vt", &m_vt);
  m_outputTree->Branch("px", &m_px);
  m_outputTree->Branch("py", &m_py);
  m_outputTree->Branch("pz", &m_pz);
  m_outputTree->Branch("m", &m_m);
  m_outputTree->Branch("q", &m_q);
  m_outputTree->Branch("eta", &m_eta);
  m_outputTree->Branch("phi", &m_phi);
  m_outputTree->Branch("pt", &m_pt);
  m_outputTree->Branch("p", &m_p);
  m_outputTree->Branch("vertex_primary", &m_vertexPrimary);
  m_outputTree->Branch("vertex_secondary", &m_vertexSecondary);
  m_outputTree->Branch("particle", &m_particle);
  m_outputTree->Branch("generation", &m_generation);
  m_outputTree->Branch("sub_particle", &m_subParticle);

  m_outputTree->Branch("e_loss", &m_eLoss);
  m_outputTree->Branch("total_x0", &m_pathInX0);
  m_outputTree->Branch("total_l0", &m_pathInL0);
  m_outputTree->Branch("number_of_hits", &m_numberOfHits);
  m_outputTree->Branch("outcome", &m_outcome);
}

ActsExamples::RootParticleFlatWriter::~RootParticleFlatWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::RootParticleFlatWriter::finalize() {
  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  ACTS_INFO("Wrote particles to tree '" << m_cfg.treeName << "' in '"
                                        << m_cfg.filePath << "'");

  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::RootParticleFlatWriter::writeT(
    const AlgorithmContext& ctx, const SimParticleContainer& particles) {
  // ensure exclusive access to tree/file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  for (const auto& particle : particles) {
    m_eventId = ctx.eventNumber;
    m_particleId = particle.particleId().value();
    m_particleType = particle.pdg();
    m_process = static_cast<std::uint32_t>(particle.process());
    
    // position
    m_vx = Acts::clampValue<float>(particle.fourPosition().x() / Acts::UnitConstants::mm);
    m_vy = Acts::clampValue<float>(particle.fourPosition().y() / Acts::UnitConstants::mm);
    m_vz = Acts::clampValue<float>(particle.fourPosition().z() / Acts::UnitConstants::mm);
    m_vt = Acts::clampValue<float>(particle.fourPosition().w() / Acts::UnitConstants::mm);
    
    // momentum
    const auto p = particle.absoluteMomentum() / Acts::UnitConstants::GeV;
    m_p = Acts::clampValue<float>(p);
    m_px = Acts::clampValue<float>(p * particle.direction().x());
    m_py = Acts::clampValue<float>(p * particle.direction().y());
    m_pz = Acts::clampValue<float>(p * particle.direction().z());
    
    // particle constants
    m_m = Acts::clampValue<float>(particle.mass() / Acts::UnitConstants::GeV);
    m_q = Acts::clampValue<float>(particle.charge() / Acts::UnitConstants::e);
    
    // derived kinematic quantities
    m_eta = Acts::clampValue<float>(Acts::VectorHelpers::eta(particle.direction()));
    m_phi = Acts::clampValue<float>(Acts::VectorHelpers::phi(particle.direction()));
    m_pt = Acts::clampValue<float>(p * Acts::VectorHelpers::perp(particle.direction()));
    
    // decoded barcode components
    m_vertexPrimary = particle.particleId().vertexPrimary();
    m_vertexSecondary = particle.particleId().vertexSecondary();
    m_particle = particle.particleId().particle();
    m_generation = particle.particleId().generation();
    m_subParticle = particle.particleId().subParticle();

    m_eLoss = Acts::clampValue<float>(particle.energyLoss() / Acts::UnitConstants::GeV);
    m_pathInX0 = Acts::clampValue<float>(particle.pathInX0() / Acts::UnitConstants::mm);
    m_pathInL0 = Acts::clampValue<float>(particle.pathInL0() / Acts::UnitConstants::mm);
    m_numberOfHits = particle.numberOfHits();
    m_outcome = static_cast<std::uint32_t>(particle.outcome());

    m_outputTree->Fill();
  }

  return ProcessCode::SUCCESS;
}

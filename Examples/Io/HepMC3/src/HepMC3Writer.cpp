// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/HepMC3/HepMC3Writer.hpp"
#include "ActsExamples/Io/HepMC3/HepMC3Event.hpp"

namespace ActsExamples {

HepMC3AsciiWriter::HepMC3AsciiWriter(const Config& config,
                                     Acts::Logging::Level level)
    : WriterT(config.inputParticles, "HepMC3AsciiWriter", level), 
      m_cfg(config) {
  if (m_cfg.outputStem.empty()) {
    throw std::invalid_argument("Missing output stem file name");
  }
  
  // Initialize the vertex data handle
  m_inputVertices.initialize(config.inputVertices);
}

ProcessCode HepMC3AsciiWriter::writeT(
    const AlgorithmContext& ctx,
    const SimParticleContainer& particles) {
    
  const auto& vertices = m_inputVertices(ctx);
  
  ACTS_DEBUG("Processing event " << ctx.eventNumber);
  ACTS_DEBUG("Number of particles: " << particles.size());
  ACTS_DEBUG("Number of vertices: " << vertices.size());

  // Add debug logging
  ACTS_DEBUG("Processing vertices: " << vertices.size());
  for (const auto& vertex : vertices) {
    ACTS_DEBUG("Vertex position: " << vertex.position4.transpose());
  }

  // Create GenEvent
  HepMC3::GenEvent event;
  event.set_units(HepMC3::Units::GEV, HepMC3::Units::MM);

  // Use existing HepMC3Event utilities
  for (const auto& vertex : vertices) {
    ACTS_VERBOSE("Adding vertex at position: " 
                 << vertex.position().x() << ", "
                 << vertex.position().y() << ", "
                 << vertex.position().z());
    HepMC3Event::addVertex(event, std::make_shared<SimVertex>(vertex));
  }

  for (const auto& particle : particles) {
    ACTS_VERBOSE("Adding particle with ID: " << particle.particleId());
    HepMC3Event::addParticle(event, std::make_shared<SimParticle>(particle));
  }

  // Before writing
  ACTS_DEBUG("Event structure:");
  ACTS_DEBUG("  Vertices: " << event.vertices().size());
  for (const auto& v : event.vertices()) {
    ACTS_DEBUG("  Vertex at: " << v->position().x() << ", " 
               << v->position().y() << ", " << v->position().z());
  }

  // Write the converted event
  auto path = perEventFilepath(m_cfg.outputDir, m_cfg.outputStem + ".hepmc3",
                             ctx.eventNumber);
  HepMC3::WriterAscii writer(path);
  writer.write_event(event);
  
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples

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

  // Debug particle container contents
  ACTS_DEBUG("First few particles in container:");
  int count = 0;
  for (const auto& particle : particles) {
    ACTS_DEBUG("  Particle ID: " << particle.particleId() 
               << " (raw value: " << particle.particleId().value() << ")");
    if (++count >= 5) break;
  }

  // Create GenEvent
  HepMC3::GenEvent event;
  event.set_units(HepMC3::Units::GEV, HepMC3::Units::MM);

  // Add vertices with their connected particles
  for (const auto& vertex : vertices) {
    ACTS_DEBUG("Adding vertex at " << vertex.position4.transpose());
    ACTS_DEBUG("  Incoming particles: " << vertex.incoming.size());
    ACTS_DEBUG("  Outgoing particles: " << vertex.outgoing.size());
    
    if (!vertex.outgoing.empty()) {
      auto firstId = *vertex.outgoing.begin();
      auto iter = particles.find(firstId);
      if (iter != particles.end()) {
        ACTS_DEBUG("  Found first outgoing particle with ID " << firstId.value());
      } else {
        ACTS_DEBUG("  Could not find outgoing particle " << firstId.value() 
                  << " in container");
      }
    }
    
    HepMC3Event::addVertex(event, std::make_shared<SimVertex>(vertex), particles);
  }

  // Write the converted event
  auto path = perEventFilepath(m_cfg.outputDir, m_cfg.outputStem + ".hepmc3",
                             ctx.eventNumber);
  ACTS_DEBUG("Writing event to " << path);
  HepMC3::WriterAscii writer(path);
  writer.write_event(event);
  
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples

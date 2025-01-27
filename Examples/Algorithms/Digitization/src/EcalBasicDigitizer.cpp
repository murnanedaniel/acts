// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/EcalBasicDigitizer.hpp"

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <stdexcept>

ActsExamples::EcalBasicDigitizer::EcalBasicDigitizer(Config cfg, Acts::Logging::Level level)
    : IAlgorithm("EcalBasicDigitizer", level), m_cfg(std::move(cfg)) {
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing input hits collection");
  }
  if (m_cfg.outputDigiHits.empty()) {
    throw std::invalid_argument("Missing output hits collection");
  }
  if (m_cfg.energyThreshold < 0) {
    throw std::invalid_argument("Energy threshold must be non-negative");
  }

  m_inputHits.initialize(m_cfg.inputSimHits);
  m_outputHits.initialize(m_cfg.outputDigiHits);
}

ActsExamples::ProcessCode ActsExamples::EcalBasicDigitizer::execute(
    const AlgorithmContext& ctx) const {
  // Read input hits
  const auto& simHits = m_inputHits(ctx);
  ACTS_DEBUG("Processing " << simHits.size() << " hits");

  // Create output container
  DigiHitContainer digiHits;
  digiHits.reserve(simHits.size());

  // Process hits
  for (const auto& simHit : simHits) {
    // Apply energy threshold
    if (simHit.energy() < m_cfg.energyThreshold) {
      ACTS_VERBOSE("Hit below threshold: " << simHit.energy() << " GeV");
      continue;
    }

    // Create digitized hit (for now, just copy the hit)
    // TODO: Create proper digi hit with only the necessary information
    digiHits.push_back(simHit);
    ACTS_VERBOSE("Created digi hit with energy: " << simHit.energy() << " GeV");
  }

  ACTS_DEBUG("Created " << digiHits.size() << " digitized hits");
  m_outputHits(ctx).swap(digiHits);

  return ProcessCode::SUCCESS;
} 
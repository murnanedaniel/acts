// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <string>

#include <HepMC3/GenEvent.h>
#include <HepMC3/WriterAscii.h>

namespace ActsExamples {

/// HepMC3 event writer that takes SimParticles/SimVertices as input
class HepMC3AsciiWriter final : public WriterT<SimParticleContainer> {
 public:
  struct Config {
    std::string outputDir;
    std::string outputStem;
    std::string inputParticles;    // SimParticle collection
    std::string inputVertices;     // SimVertex collection
  };

  /// Construct the writer.
  ///
  /// @param [in] config Config of the writer
  /// @param [in] level The level of the logger
  HepMC3AsciiWriter(const Config& config, Acts::Logging::Level level);

  /// Writing events to file.
  ///
  /// @param [in] ctx The context of this algorithm
  /// @param [in] particles The SimParticles to write
  ///
  /// @return Code describing whether the writing was successful
  ProcessCode writeT(const ActsExamples::AlgorithmContext& ctx,
                    const SimParticleContainer& particles) override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// The configuration of this writer
  Config m_cfg;

  // Data handle for vertices
  ReadDataHandle<SimVertexContainer> m_inputVertices{this, "InputVertices"};
};

}  // namespace ActsExamples

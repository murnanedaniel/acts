// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <string>

namespace ActsExamples {

/// Algorithm to digitize ECAL hits with basic effects.
/// 
/// This is a minimal implementation that only applies energy thresholds.
class EcalBasicDigitizer final : public IAlgorithm {
 public:
  /// Algorithm configuration
  struct Config {
    /// Input collection of simulated hits
    std::string inputSimHits = "simhits";
    /// Output collection of digitized hits
    std::string outputDigiHits = "digihits";
    /// Energy threshold in GeV
    double energyThreshold = 0.1;
  };

  /// Construct the digitizer.
  ///
  /// @param config is the algorithm configuration
  /// @param level is the logging level
  EcalBasicDigitizer(Config config, Acts::Logging::Level level);

  /// Run the digitization algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  
  ReadDataHandle<SimHitContainer> m_inputHits{this, "InputHits"};
  WriteDataHandle<DigiHitContainer> m_outputHits{this, "OutputHits"};
}; 
// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/EDM4hepCaloHit.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <string>
#include <mutex>

#include <edm4hep/SimCalorimeterHitCollection.h>

namespace ActsExamples {

/// Write out a calorimeter hit collection to EDM4hep.
class EDM4hepCaloHitWriter final : public WriterT<std::vector<Acts::EDM4hepCaloHit>> {
 public:
  struct Config {
    /// Input collection name
    std::string inputCaloHits;
    /// Where to write the output file
    std::string outputPath;
    /// Name of the calorimeter hit collection in EDM4hep
    std::string outputCaloHits = "ActsSimCaloHits";
  };

  /// Construct the writer
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  EDM4hepCaloHitWriter(const Config& config, Acts::Logging::Level level);

  /// Finalize the writer
  ProcessCode finalize() final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  ProcessCode writeT(const AlgorithmContext& ctx,
                    const std::vector<Acts::EDM4hepCaloHit>& caloHits) final;

  Config m_cfg;                           ///< The config object
  std::mutex m_writeMutex;               ///< Mutex used to protect multi-threaded writes
  Acts::PodioUtil::ROOTWriter m_writer;  ///< The writer object for the output file
};

}  // namespace ActsExamples 
// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace ActsExamples {
struct AlgorithmContext;

/// Write out particles as a flat TTree.
///
/// Each entry in the TTree corresponds to one particle for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-saftey issues,
/// the writer must be the sole owner of the underlying file. Thus, the
/// output file pointer can not be given from the outside.
class RootParticleFlatWriter final : public WriterT<SimParticleContainer> {
 public:
  struct Config {
    /// Input particle collection to write.
    std::string inputParticles;
    /// Path to the output file.
    std::string filePath;
    /// Output file access mode.
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file.
    std::string treeName = "particles";
  };

  /// Construct the particle writer.
  ///
  /// @params cfg is the configuration object
  /// @params lvl is the logging level
  RootParticleFlatWriter(const Config& cfg, Acts::Logging::Level lvl);

  /// Ensure underlying file is closed.
  ~RootParticleFlatWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// Type-specific write implementation.
  ///
  /// @param[in] ctx is the algorithm context
  /// @param[in] particles are the particle to be written
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const SimParticleContainer& particles) override;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;

  TFile* m_outputFile = nullptr;
  TTree* m_outputTree = nullptr;

  /// Event identifier.
  std::uint32_t m_eventId = 0;
  /// Event-unique particle identifier a.k.a barcode.
  std::uint64_t m_particleId = 0;
  /// Particle type a.k.a. PDG particle number
  std::int32_t m_particleType = 0;
  /// Production process type, i.e. what generated the particle.
  std::uint32_t m_process = 0;
  /// Production position components in mm.
  float m_vx = 0, m_vy = 0, m_vz = 0, m_vt = 0;
  /// Total momentum in GeV
  float m_p = 0;
  /// Momentum components in GeV.
  float m_px = 0, m_py = 0, m_pz = 0;
  /// Mass in GeV.
  float m_m = 0;
  /// Charge in e.
  float m_q = 0;
  // Derived kinematic quantities
  /// Direction pseudo-rapidity.
  float m_eta = 0;
  /// Direction angle in the transverse plane.
  float m_phi = 0;
  /// Transverse momentum in GeV.
  float m_pt = 0;
  // Decoded particle identifier; see Barcode definition for details.
  std::uint32_t m_vertexPrimary = 0;
  std::uint32_t m_vertexSecondary = 0;
  std::uint32_t m_particle = 0;
  std::uint32_t m_generation = 0;
  std::uint32_t m_subParticle = 0;

  /// Total energy loss in GeV.
  float m_eLoss = 0;
  /// Accumulated material
  float m_pathInX0 = 0;
  /// Accumulated material
  float m_pathInL0 = 0;
  /// Number of hits.
  std::int32_t m_numberOfHits = 0;
  /// Particle outcome
  std::uint32_t m_outcome = 0;
};

}  // namespace ActsExamples

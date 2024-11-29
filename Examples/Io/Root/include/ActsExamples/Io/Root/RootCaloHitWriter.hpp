#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/EDM4hepCaloHit.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <mutex>
#include <string>

class TFile;
class TTree;

namespace ActsExamples {

/// Write out calorimeter hits as a flat TTree.
///
/// Each entry in the TTree corresponds to one hit for optimum writing
/// speed. The event number is part of the written data.
///
/// Safe to use from multiple writer threads. To avoid thread-safety issues,
/// the writer must be the sole owner of the underlying file.
class RootCaloHitWriter final : public WriterT<std::vector<Acts::EDM4hepCaloHit>> {
 public:
  struct Config {
    /// Input calo hit collection to write
    std::string inputCaloHits;
    /// Path to the output file
    std::string filePath;
    /// Output file access mode
    std::string fileMode = "RECREATE";
    /// Name of the tree within the output file
    std::string treeName = "calohits";
  };

  /// Construct the writer
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  RootCaloHitWriter(const Config& config, Acts::Logging::Level level);

  /// Ensure underlying file is closed
  ~RootCaloHitWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  ProcessCode writeT(const AlgorithmContext& ctx,
                    const std::vector<Acts::EDM4hepCaloHit>& hits) final;

  Config m_cfg;                   ///< The config object
  std::mutex m_writeMutex;       ///< Mutex used to protect multi-threaded writes
  TFile* m_outputFile{nullptr};  ///< The output file
  TTree* m_outputTree{nullptr};  ///< The output tree

  /// Event identifier
  uint32_t m_eventId{};
  /// Cell identifier
  uint64_t m_cellId{};
  /// Hit position components
  float m_x{}, m_y{}, m_z{};
  /// Hit energy
  float m_energy{};
};

}  // namespace ActsExamples 
#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/EventData/EDM4hepCaloHit.hpp"

#include <tbb/enumerable_thread_specific.h>
#include <string>
#include <vector>

namespace ActsExamples {

class DD4hepDetector;  // Forward declaration

class EDM4hepCaloReader final : public IReader {
public:
  struct Config {
    std::string inputPath;
    std::vector<std::string> inputCaloHits;
    std::string outputCaloHits;
    std::shared_ptr<const DD4hepDetector> dd4hepDetector;
  };

  EDM4hepCaloReader(const Config& config, Acts::Logging::Level level);

  const Config& config() const { return m_cfg; }

  std::pair<std::size_t, std::size_t> availableEvents() const override;
  ProcessCode read(const AlgorithmContext& ctx) override;
  std::string name() const override { return "EDM4hepCaloReader"; }

private:
  const Acts::Logger& logger() const { return *m_logger; }
  Acts::PodioUtil::ROOTReader& reader();

  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange{0, 0};
  std::unique_ptr<const Acts::Logger> m_logger;
  tbb::enumerable_thread_specific<Acts::PodioUtil::ROOTReader> m_reader;
  
  WriteDataHandle<std::vector<Acts::EDM4hepCaloHit>> m_outputCaloHits{this, "OutputCaloHits"};
};

} // namespace ActsExamples
// CaloDigitizationAlgorithm.hpp
#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

namespace ActsExamples {

struct NoiseConfig {
    double noise = 0.0;  // in MeV
};

class CaloDigitizationAlgorithm final : public IAlgorithm {
public:
    struct Config {
        // Input/output collections
        std::string inputCaloHits = "calohits";
        std::string outputDigiHits = "digicalohits";
        
        // Noise configuration per detector region
        Acts::GeometryHierarchyMap<NoiseConfig> noiseConfigs;
        
        // Random numbers tool
        std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;
        
        // Energy threshold
        double minEnergy = 0.0;
        
        // Gaussian smearing parameters
        double energyResolution = 0.1;  // 10% energy resolution
    };

    CaloDigitizationAlgorithm(Config cfg, Acts::Logging::Level lvl);
    
    ProcessCode execute(const AlgorithmContext& ctx) const final;
    
    const Config& config() const { return m_cfg; }

private:
    Config m_cfg;
};

}
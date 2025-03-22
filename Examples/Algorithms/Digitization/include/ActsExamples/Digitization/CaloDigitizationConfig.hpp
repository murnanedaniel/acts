// CaloDigitizationConfig.hpp
#pragma once

#include "ActsExamples/Digitization/CaloDigitizationAlgorithm.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"

namespace ActsExamples {

// Constants matching DD4hep detector IDs from OpenDataDetectorIdentifiers.xml
constexpr uint64_t ODD_ECal_ID = 15;  // Correct ECal ID
constexpr uint64_t ODD_HCal_ID = 18;  // Correct HCal ID

CaloDigitizationAlgorithm::Config createCaloDigiConfig() {
    CaloDigitizationAlgorithm::Config cfg;
    
    std::vector<std::pair<Acts::GeometryIdentifier, NoiseConfig>> noiseInputs;
    
    // ECAL noise (40 MeV constant for now)
    Acts::GeometryIdentifier ecalId;
    ecalId.setVolume(ODD_ECal_ID);
    noiseInputs.push_back({ecalId, NoiseConfig{40.0}});
    
    // HCAL noise (75 MeV constant for now)
    Acts::GeometryIdentifier hcalId;
    hcalId.setVolume(ODD_HCal_ID);
    noiseInputs.push_back({hcalId, NoiseConfig{75.0}});
    
    // Create the hierarchy map
    cfg.noiseConfigs = Acts::GeometryHierarchyMap<NoiseConfig>(noiseInputs);
    
    return cfg;
}

}  // namespace ActsExamples
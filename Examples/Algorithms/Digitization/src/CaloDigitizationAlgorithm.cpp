// CaloDigitizationAlgorithm.cpp
#include "ActsExamples/Digitization/CaloDigitizationAlgorithm.hpp"

ProcessCode CaloDigitizationAlgorithm::execute(const AlgorithmContext& ctx) const {
    // Get input hits
    const auto& caloHits = m_inputCaloHits(ctx);
    
    // Prepare output container
    DigiCaloHitContainer outputHits;
    outputHits.reserve(caloHits.size());
    
    // Setup RNG
    auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);
    
    // Process hits
    for (const auto& hit : caloHits) {
        // Get noise for this cell based on detector region
        Acts::GeometryIdentifier geoID = hit.cellID;  // Assuming cellID is GeometryIdentifier
        double noise = 0.0;
        
        // Get noise config for this detector region
        const auto* noiseConfig = m_cfg.noiseConfigs.find(geoID);
        if (noiseConfig != nullptr) {
            noise = noiseConfig->noise;
        }
        
        // Apply Gaussian smearing to energy
        double resolution = m_cfg.energyResolution * std::sqrt(hit.energy);
        double totalNoise = std::sqrt(noise*noise + resolution*resolution);
        double smearedEnergy = rng.gauss(hit.energy, totalNoise);
        
        // Apply threshold
        if (smearedEnergy < m_cfg.minEnergy) {
            continue;
        }
        
        // Create digitized hit
        outputHits.push_back({
            hit.cellID,
            hit.position,
            smearedEnergy,
        });
    }
    
    // Write output
    m_outputDigiHits(ctx, std::move(outputHits));
    
    return ProcessCode::SUCCESS;
}
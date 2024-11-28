#pragma once

#include "Acts/Definitions/Algebra.hpp"

namespace Acts {

struct EDM4hepCaloHit {
  /// Position of the hit
  Acts::Vector3 position;
  /// Energy deposited
  float energy;
  /// Time of the hit
  float time;
  /// Cell identifier
  uint64_t cellID;
};

} // namespace Acts 
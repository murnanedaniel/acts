// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ACTFW/DD4hepDetector/DD4hepDetectorOptions.hpp"
#include "ACTFW/DD4hepDetector/DD4hepGeometryService.hpp"
#include "ACTFW/Options/CommonOptions.hpp"
#include "ActsExamples/Geant4DD4hep/DD4hepDetectorConstruction.hpp"
#include "../GeantinoRecordingBase.hpp"

#include <boost/program_options.hpp>

using namespace ActsExamples;
using namespace FW;

int main(int argc, char* argv[]) {
  // Setup and parse options
  auto desc = Options::makeDefaultOptions();
  Options::addSequencerOptions(desc);
  Options::addOutputOptions(desc);
  Options::addDD4hepOptions(desc);
  Options::addGeant4Options(desc);
  auto vm = Options::parse(desc, argc, argv);
  if (vm.empty()) {
    return EXIT_FAILURE;
  }

  // Setup the DD4hep detector
  auto dd4hepCfg = Options::readDD4hepConfig<po::variables_map>(vm);
  auto geometrySvc = std::make_shared<DD4hep::DD4hepGeometryService>(dd4hepCfg);

  std::unique_ptr<G4VUserDetectorConstruction> g4detector =
      std::make_unique<DD4hepDetectorConstruction>(*geometrySvc->lcdd());

  return runGeantinoRecording(vm, std::move(g4detector));
}
import pytest
import os
from pathlib import Path
import multiprocessing
import logging
import sys

from helpers import (
    geant4Enabled,
    edm4hepEnabled,
    podioEnabled,
    AssertCollectionExistsAlg,
)

import acts
from acts import UnitConstants as u
from acts.examples import Sequencer
from acts.examples.odd import getOpenDataDetector, getOpenDataDetectorDirectory
from acts.examples.edm4hep import EDM4hepCaloReader

# Configure logging
logger = logging.getLogger("EDM4hepCaloTest")
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(logging.Formatter(
    '%(asctime)s %(levelname)-8s %(name)-12s %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
))
logger.addHandler(handler)

# Set log level from environment variable or default to INFO
log_level = os.getenv('ACTS_LOG_LEVEL', 'INFO')
logger.setLevel(getattr(logging, log_level))

def generate_test_calo_data(input_xml, output_path):
    """Generate test calorimeter data using DD4hep simulation"""
    logger.info("Starting calorimeter data generation with DD4hep")
    from DDSim.DD4hepSimulation import DD4hepSimulation

    ddsim = DD4hepSimulation()
    if isinstance(ddsim.compactFile, list):
        ddsim.compactFile = [input_xml]
    else:
        ddsim.compactFile = input_xml
    
    # Configure particle gun for calorimeter test
    ddsim.enableGun = True
    ddsim.gun.direction = (1, 0, 0)
    ddsim.gun.particle = "e-"  # Use electrons for calo test
    ddsim.gun.energy = 10 * u.GeV  # Higher energy for calo
    ddsim.gun.distribution = "eta"
    ddsim.numberOfEvents = 10
    ddsim.outputFile = output_path
    logger.info(f"Running DD4hep simulation with {ddsim.numberOfEvents} events")
    ddsim.run()
    logger.info("Finished calorimeter data generation")

@pytest.mark.slow
@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_calo_reader(tmp_path):
    logger.info("Starting EDM4hep calorimeter reader test")
    logger.debug("Using temporary path: %s", tmp_path)
    
    tmp_file = str(tmp_path / "calo_hits_edm4hep.root")
    odd_dir = getOpenDataDetectorDirectory()
    odd_xml_file = str(odd_dir / "xml" / "OpenDataDetector.xml")
    
    logger.info("Generating test data with XML file: %s", odd_xml_file)
    
    # Generate test data in separate process
    p = multiprocessing.Process(
        target=generate_test_calo_data, args=(odd_xml_file, tmp_file)
    )
    p.start()
    p.join()

    logger.debug("Test data generation complete")
    assert os.path.exists(tmp_file)
    logger.info("Test file created at: %s", tmp_file)

    # Now read the data back using context manager for detector
    s = Sequencer(numThreads=1)
    logger.info("Created sequencer")
    
    with getOpenDataDetector() as (detector, trackingGeometry, decorators):
        logger.debug("Obtained OpenDataDetector context")
        s.addReader(
            EDM4hepCaloReader(
                level=acts.logging.INFO,
                inputPath=tmp_file,
                inputCaloHits=[
                    "ECalBarrelCollection",
                    "ECalEndcapCollection",
                    "HCalBarrelCollection",
                    "HCalEndcapCollection"
                ],
                outputCaloHits="calohits"
            )
        )
        logger.debug("Added EDM4hepCaloReader to sequencer")

        alg = AssertCollectionExistsAlg(
            "calohits", "check_calohits", acts.logging.WARNING
        )
        s.addAlgorithm(alg)
        logger.info("Starting sequencer run")

        s.run()
        logger.info("Sequencer run complete")

        assert alg.events_seen == 10
        logger.info("Test completed successfully")

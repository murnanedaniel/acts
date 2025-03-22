import os
import logging
from pathlib import Path
import acts
from acts import UnitConstants as u
from acts.examples.odd import getOpenDataDetectorDirectory

# Configure logging similar to test_edm4hep_calo.py
logger = logging.getLogger("EDM4hepGen")
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter(
    '%(asctime)s %(levelname)-8s %(name)-12s %(message)s'
))
logger.addHandler(handler)
logger.setLevel(logging.INFO)

def generate_full_detector_data(output_path, n_events=100):
    """Generate combined tracking and calorimeter data using DD4hep"""
    from DDSim.DD4hepSimulation import DD4hepSimulation
    
    odd_dir = getOpenDataDetectorDirectory()
    odd_xml = odd_dir / "xml" / "OpenDataDetector.xml"
    
    ddsim = DD4hepSimulation()
    # Handle XML file path correctly for DD4hep
    if isinstance(ddsim.compactFile, list):
        ddsim.compactFile = [str(odd_xml)]
    else:
        ddsim.compactFile = str(odd_xml)
    
    # Configure particle gun
    ddsim.enableGun = True
    ddsim.gun.particle = "e-"
    ddsim.gun.energy = "50*GeV"
    ddsim.gun.distribution = "uniform"
    ddsim.gun.thetaMin = 1.57
    ddsim.gun.thetaMax = 1.57
    
    
    # Ensure output directory exists
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    ddsim.numberOfEvents = n_events
    ddsim.outputFile = str(output_path)  # Make sure it's a string
    
    logger.info(f"Using XML file: {odd_xml}")
    logger.info(f"Starting DD4hep simulation with {n_events} events")
    logger.info(f"Output will be written to {output_path}")
    ddsim.run()
    logger.info("Simulation complete")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=str, required=True, help="Output EDM4hep file path")
    parser.add_argument("--events", type=int, default=100, help="Number of events")
    args = parser.parse_args()
    
    generate_full_detector_data(args.output, args.events)

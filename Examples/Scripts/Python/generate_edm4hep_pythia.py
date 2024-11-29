import os
import logging
import subprocess
from pathlib import Path
import acts
from acts.examples import Sequencer
from acts.examples.odd import getOpenDataDetectorDirectory

# Configure logging
logger = logging.getLogger("EDM4hepGen")
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter(
    '%(asctime)s %(levelname)-8s %(name)-12s %(message)s'
))
logger.addHandler(handler)
logger.setLevel(logging.INFO)

def generate_pythia_events(output_path, n_events=100):
    """Generate HepMC3 events using Pythia8"""
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Create sequencer for Pythia8
    s = Sequencer(numThreads=1, events=n_events)
    
    # Set up Pythia8 with HepMC3 output
    from acts.examples.simulation import addPythia8
    addPythia8(
        s,
        nhard=1,
        npileup=0,
        hardProcess=["HardQCD:all = on"],
        outputHepMC=output_path,
    )
    
    logger.info(f"Starting Pythia8 generation of {n_events} events")
    logger.info(f"HepMC3 output will be written to {output_path}")
    s.run()
    logger.info("Pythia8 generation complete")

def run_ddsim(input_hepmc, output_edm4hep, n_events=100):
    """Run DD4hep simulation on HepMC3 input"""
    odd_dir = getOpenDataDetectorDirectory()
    odd_xml = odd_dir / "xml" / "OpenDataDetector.xml"
    
    # Construct ddsim command
    cmd = [
        "ddsim",
        "--compactFile", str(odd_xml),
        "--numberOfEvents", str(n_events),
        "--inputFiles", str(input_hepmc),
        "--outputFile", str(output_edm4hep)
    ]
    
    logger.info(f"Starting DD4hep simulation with {n_events} events")
    logger.info(f"Using XML file: {odd_xml}")
    logger.info(f"Input HepMC3: {input_hepmc}")
    logger.info(f"Output EDM4hep: {output_edm4hep}")
    
    try:
        subprocess.run(cmd, check=True)
        logger.info("DD4hep simulation complete")
    except subprocess.CalledProcessError as e:
        logger.error(f"DD4hep simulation failed with error: {e}")
        raise

def generate_full_detector_data(output_path, n_events=100):
    """Generate combined tracking and calorimeter data using Pythia8 and DD4hep"""
    output_path = Path(output_path)
    work_dir = output_path.parent / "tmp"
    work_dir.mkdir(parents=True, exist_ok=True)
    
    # Step 1: Generate HepMC3 events with Pythia8
    hepmc3_file = work_dir / "pythia8_events.hepmc3"
    generate_pythia_events(hepmc3_file, n_events)
    
    # Step 2: Run DD4hep simulation
    run_ddsim(hepmc3_file, output_path, n_events)
    
    # Cleanup temporary files
    if hepmc3_file.exists():
        hepmc3_file.unlink()
    work_dir.rmdir()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=str, required=True, help="Output EDM4hep file path")
    parser.add_argument("--events", type=int, default=100, help="Number of events")
    args = parser.parse_args()
    
    generate_full_detector_data(args.output, args.events)

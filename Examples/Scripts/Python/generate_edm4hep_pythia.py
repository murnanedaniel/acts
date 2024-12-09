import os
import logging
import subprocess
import time
from pathlib import Path
from datetime import datetime
import pyhepmc as hep
from pyhepmc.io import WriterAscii

import acts
from acts.examples import Sequencer
from acts.examples.odd import getOpenDataDetectorDirectory
from acts.examples.simulation import addPythia8HepMC

from DDSim.DD4hepSimulation import DD4hepSimulation

# Configure logging
logger = logging.getLogger("EDM4hepGen")
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter(
    '%(asctime)s %(levelname)-8s %(name)-12s %(message)s'
))
logger.addHandler(handler)
logger.setLevel(logging.INFO)

def generate_pythia_events(output_path, n_events=10, n_pileup=1, seed=None):
    """Generate HepMC3 events using Pythia8"""
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Create timestamp for file names
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Create separate paths for hard scatter and pileup with timestamps
    hard_scatter_path = output_path.parent / f"{output_path.stem}_{timestamp}.hepmc3"
    pileup_path = output_path.parent / f"{output_path.stem}_{timestamp}_pileup.hepmc3"
    
    # Create sequencer for Pythia8
    s = Sequencer(numThreads=1, events=n_events)
    seed = seed or int(time.time())
    rnd = acts.examples.RandomNumbers(seed=seed)

    # Set up Pythia8 with HepMC3 output
    addPythia8HepMC(
        s,
        npileup=n_pileup,
        hardProcess=["HardQCD:all = on"],
        outputHepMC=hard_scatter_path,
        outputHepMCPileup=pileup_path,
        rnd=rnd,
    )
    
    logger.info(f"Starting Pythia8 generation of {n_events} events")
    logger.info(f"Hard scatter events will be written to {hard_scatter_path}")
    logger.info(f"Pileup events will be written to {pileup_path}")
    s.run()
    logger.info("Pythia8 generation complete")
    
    return hard_scatter_path, pileup_path

def run_ddsim(input_hepmc, output_edm4hep, n_events=10):
    """Run DD4hep simulation on HepMC3 input"""
    # Create DD4hep simulation instance
    ddsim = DD4hepSimulation()
    
    odd_dir = getOpenDataDetectorDirectory()
    odd_xml = odd_dir / "xml" / "OpenDataDetector.xml"
    
    # Handle XML file path correctly for DD4hep
    if isinstance(ddsim.compactFile, list):
        ddsim.compactFile = [str(odd_xml)]
    else:
        ddsim.compactFile = str(odd_xml)
    
    # Configure input/output
    ddsim.inputFiles = [str(input_hepmc)]
    ddsim.outputFile = str(output_edm4hep)
    ddsim.numberOfEvents = n_events
    
    logger.info(f"Starting DD4hep simulation with {n_events} events")
    logger.info(f"Using XML file: {odd_xml}")
    logger.info(f"Input HepMC3: {input_hepmc}")
    logger.info(f"Output EDM4hep: {output_edm4hep}")
    
    try:
        ddsim.run()
        logger.info("DD4hep simulation complete")
    except Exception as e:
        logger.error(f"DD4hep simulation failed with error: {e}")
        raise

def merge_events(signal_event, pileup_events):
    """Merge signal and multiple pileup events into a single event"""
    # Create new event with same units as input
    merged = hep.GenEvent(hep.Units.GEV, hep.Units.MM)
    
    # First add signal event
    sig_vertices = []
    for vertex in signal_event.vertices:
        v1 = hep.GenVertex(vertex.position)
        sig_vertices.append(v1)
    
    for particle in signal_event.particles:
        p1 = hep.GenParticle(
            particle.momentum,
            particle.pid,
            particle.status
        )
        p1.generated_mass = particle.generated_mass
        
        # Handle production vertex
        if particle.production_vertex.id < 0:
            production_vertex = particle.production_vertex.id
            sig_vertices[abs(production_vertex)-1].add_particle_out(p1)
            merged.add_particle(p1)
        else:
            merged.add_particle(p1)
            
        # Handle end vertex if it exists
        if particle.end_vertex:
            end_vertex = particle.end_vertex.id
            sig_vertices[abs(end_vertex)-1].add_particle_in(p1)
    
    # Add all signal vertices
    for vertex in sig_vertices:
        merged.add_vertex(vertex)
        
    # Now add pileup events
    for pileup_event in pileup_events:
        pileup_vertices = []
        for vertex in pileup_event.vertices:
            v1 = hep.GenVertex(vertex.position)
            pileup_vertices.append(v1)
        
        for particle in pileup_event.particles:
            p1 = hep.GenParticle(
                particle.momentum,
                particle.pid,
                particle.status
            )
            p1.generated_mass = particle.generated_mass
            
            if particle.production_vertex.id < 0:
                production_vertex = particle.production_vertex.id
                pileup_vertices[abs(production_vertex)-1].add_particle_out(p1)
                merged.add_particle(p1)
            else:
                merged.add_particle(p1)
                
            if particle.end_vertex:
                end_vertex = particle.end_vertex.id
                pileup_vertices[abs(end_vertex)-1].add_particle_in(p1)
        
        for vertex in pileup_vertices:
            merged.add_vertex(vertex)
    
    return merged

def merge_hepmc_files(signal_path, pileup_path, output_path):
    """Merge signal and pileup HepMC3 files into a single file"""
    logger.info(f"Merging HepMC3 files:")
    logger.info(f"Signal: {signal_path}")
    logger.info(f"Pileup: {pileup_path}")
    logger.info(f"Output: {output_path}")
    
    # Load all signal events
    signal_events = []
    with hep.open(signal_path) as f:
        for event in f:
            signal_events.append(event)
    
    # Load all pileup events
    pileup_events = []
    with hep.open(pileup_path) as f:
        for event in f:
            pileup_events.append(event)
    
    # Calculate pileup events per signal event
    n_pileup_per_signal = len(pileup_events) // len(signal_events)
    logger.info(f"Found {len(signal_events)} signal events with {n_pileup_per_signal} pileup events each")
    
    # Write merged events
    with WriterAscii(str(output_path)) as f:
        for i, signal_event in enumerate(signal_events):
            # Get corresponding pileup events for this signal event
            start_idx = i * n_pileup_per_signal
            end_idx = start_idx + n_pileup_per_signal
            event_pileup = pileup_events[start_idx:end_idx]
            
            # Merge events
            merged_event = merge_events(signal_event, event_pileup)
            merged_event.event_number = i
            
            # Write merged event
            f.write_event(merged_event)
    
    logger.info("Merge complete")
    return output_path

def generate_full_detector_data(output_path, n_events=100, n_pileup=1, seed=None, run_simulation=True):
    """Generate combined tracking and calorimeter data using Pythia8 and optionally DD4hep"""
    output_path = Path(output_path)
    work_dir = output_path.parent / "hepmc3_output"
    work_dir.mkdir(parents=True, exist_ok=True)
    
    # Step 1: Generate separate HepMC3 files for hard scatter and pileup
    hard_scatter_path, pileup_path = generate_pythia_events(
        work_dir / "events", 
        n_events, 
        n_pileup, 
        seed=seed
    )
    
    # Step 2: Merge HepMC3 files
    merged_path = work_dir / f"merged_{hard_scatter_path.stem}.hepmc3"
    merged_path = merge_hepmc_files(hard_scatter_path, pileup_path, merged_path)
    
    # Step 3: Run DD4hep simulation on merged file
    if run_simulation:
        run_ddsim(merged_path, output_path, n_events)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=str, required=True, help="Output EDM4hep file path")
    parser.add_argument("--events", type=int, default=100, help="Number of events")
    parser.add_argument("--pileup", type=int, default=1, help="Number of pileup events")
    parser.add_argument("--seed", type=int, default=None, help="Random seed")
    parser.add_argument("--no-simulation", action="store_true", help="Skip DD4hep simulation")
    args = parser.parse_args()
    
    generate_full_detector_data(args.output, args.events, args.pileup, args.seed, not args.no_simulation)

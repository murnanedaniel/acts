import os
import logging
from pathlib import Path
import time
from datetime import datetime
import yaml
import argparse
import math
import acts
import acts.examples
import acts.examples.edm4hep
from acts.examples import Sequencer
from acts.examples.odd import getOpenDataDetector, getOpenDataDetectorDirectory
from acts.examples.simulation import (
    ParticleSelectorConfig,
    addPythia8,
    addDigitization,
    addParticleSelection,
)
from acts.examples.reconstruction import (
    addSeeding,
    CkfConfig,
    addCKFTracks, 
    TrackSelectorConfig,
    addAmbiguityResolution,
    AmbiguityResolutionConfig,
    addAmbiguityResolutionML,
    AmbiguityResolutionMLConfig,
    addScoreBasedAmbiguityResolution,
    ScoreBasedAmbiguityResolutionConfig,
    addVertexFitting,
    VertexFinder,
)
from DDSim.DD4hepSimulation import DD4hepSimulation
import pyhepmc as hep
from pyhepmc.io import WriterAscii
from contextlib import contextmanager
import traceback

def setup_logging(name="PDA_Chain"):
    """Configure logging for the chain"""
    logger = logging.getLogger(name)
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter(
        '%(asctime)s %(levelname)-8s %(name)-12s %(message)s'
    ))
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    return logger

def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description="Full chain with Pythia8, DD4hep, and ACTS")
    parser.add_argument(
        "--output", "-o",
        help="Output directory",
        type=Path,
        default=Path.cwd() / "pda_output",
    )
    parser.add_argument(
        "--events", "-n",
        help="Number of events",
        type=int,
        default=100,
    )
    parser.add_argument(
        "--pileup",
        help="Number of pile-up events",
        type=int,
        default=200,
    )
    parser.add_argument(
        "--hard-process",
        help="Pythia8 hard process",
        type=str,
        default="HardQCD:all",
    )
    parser.add_argument(
        "--seed",
        help="Random seed. If not specified, uses current time.",
        type=int,
        default=None,
    )
    parser.add_argument(
        "--output-root",
        help="Write ROOT output files",
        action=argparse.BooleanOptionalAction,
        default=True,
    )
    parser.add_argument(
        "--output-csv",
        help="Write CSV output files",
        action=argparse.BooleanOptionalAction,
    )
    parser.add_argument(
        "--config",
        help="YAML configuration file",
        type=Path,
    )
    parser.add_argument(
        "--digi-config",
        help="Digitization configuration file",
        type=Path,
    )
    parser.add_argument(
        "--material-config",
        help="Material map configuration file",
        type=Path,
    )
    parser.add_argument(
        "--ambi-solver",
        help="Ambiguity solver to use",
        choices=["greedy", "scoring", "ML"],
        default="greedy",
    )
    parser.add_argument(
        "--ambi-config",
        help="Score Based ambiguity resolution config",
        type=Path,
        default=Path.cwd() / "ambi_config.json",
    )
    parser.add_argument(
        "--MLSeedFilter",
        help="Use the Ml seed filter to select seed after the seeding step",
        action="store_true",
    )
    parser.add_argument(
        "--reco",
        help="Run reconstruction",
        action="store_true",
        default=True,
    )
    parser.add_argument(
        "--output-subdir",
        help="Output subdirectory (useful for parallel processing)",
        type=str,
        default="",
    )
    return parser.parse_args()

def run_pythia_stage(output_dir, config, logger=None):
    """Run Pythia8 stage to generate HepMC3 files"""
    logger = logger or setup_logging("Pythia8Stage")
    
    # Create sequencer for Pythia8
    s = Sequencer(numThreads=1, events=config.events)
    seed = config.seed or int(time.time())
    rnd = acts.examples.RandomNumbers(seed=seed)
    
    # Create timestamp for file names
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    hard_scatter_path = output_dir / f"events_{timestamp}.hepmc3"
    pileup_path = output_dir / f"events_{timestamp}_pileup.hepmc3"
    
    logger.info(f"Generating {config.events} events with {config.pileup} pileup each")
    addPythia8(
        s,
        npileup=config.pileup,
        hardProcess=[f"{config.hard_process}=on"],
        outputHepMC=hard_scatter_path,
        outputHepMCPileup=pileup_path,
        rnd=rnd,
    )
    
    s.run()
    return hard_scatter_path, pileup_path

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

def merge_hepmc_files(signal_path, pileup_path, output_path, logger=None):
    """Merge signal and pileup HepMC3 files into a single file"""
    logger = logger or setup_logging("MergeHepMC")
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

def run_ddsim_stage(input_path, output_path, config, logger=None):
    """Run DD4hep simulation"""
    logger = logger or setup_logging("DD4hepStage")
    
    odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"
    ddsim = DD4hepSimulation()
    
    if isinstance(ddsim.compactFile, list):
        ddsim.compactFile = [str(odd_xml)]
    else:
        ddsim.compactFile = str(odd_xml)
    
    ddsim.inputFiles = [str(input_path)]
    ddsim.outputFile = str(output_path)
    ddsim.numberOfEvents = config.events
    
    logger.info(f"Running DD4hep simulation with {config.events} events")
    logger.info(f"Input: {input_path}")
    logger.info(f"Output: {output_path}")
    ddsim.run()
    return output_path

def setup_acts_reconstruction(s, input_path, config, output_dir, logger=None):
    """Configure ACTS reconstruction chain"""
    logger = logger or setup_logging("ACTSReco")

    ambi_ML = config.ambi_solver == "ML"
    ambi_scoring = config.ambi_solver == "scoring"
    ambi_config = config.ambi_config
    seedFilter_ML = config.MLSeedFilter
    
    # Get detector and field
    geoDir = getOpenDataDetectorDirectory()
    field = acts.ConstantBField(acts.Vector3(0.0, 0.0, 2.0 * acts.UnitConstants.T))
    oddMaterialMap = (
        config.material_config
        if config.material_config
        else geoDir / "data/odd-material-maps.root"
    )

    oddSeedingSel = geoDir / "config/odd-seeding-config.json"
    oddMaterialDeco = acts.IMaterialDecorator.fromFile(oddMaterialMap)

    detector, trackingGeometry, _ = getOpenDataDetector(
        odd_dir=geoDir,
        mdecorator=oddMaterialDeco
    )

    # Add EDM4hep reader
    edm4hepReader = acts.examples.edm4hep.EDM4hepReader(
        level=acts.logging.INFO,
        config=acts.examples.edm4hep.EDM4hepReader.Config(
            inputPath=str(input_path),
            inputSimHits=[
                "PixelBarrelReadout",
                "PixelEndcapReadout",
                "ShortStripBarrelReadout",
                "ShortStripEndcapReadout",
                "LongStripBarrelReadout",
                "LongStripEndcapReadout"
            ],
            outputParticlesGenerator="particles_input",
            outputParticlesSimulation="particles_simulated",
            outputSimHits="simhits",
            dd4hepDetector=detector,
            trackingGeometry=trackingGeometry
        )
    )
    s.addReader(edm4hepReader)

    edm4hepCaloReader = acts.examples.edm4hep.EDM4hepCaloReader(
        level=acts.logging.INFO,
        config=acts.examples.edm4hep.EDM4hepCaloReader.Config(
            inputPath=str(input_path),
            inputCaloHits=[
                "ECalBarrelCollection",
                "ECalEndcapCollection",
                "HCalBarrelCollection",
                "HCalEndcapCollection"
            ],
            outputCaloHits="calohits"
        )
    )
    s.addReader(edm4hepCaloReader)

    # Add reconstruction components
    add_particle_selection(s, edm4hepReader, config)
    add_digitization(s, trackingGeometry, field, geoDir, config, output_dir)
    
    if config.reco:
        add_tracking(s, trackingGeometry, field, config, output_dir)
        add_vertexing(s, field, config)
    
    return s

def load_config(args):
    """Load and merge configuration from YAML file"""
    if args.config is not None:
        with open(args.config) as f:
            config = yaml.safe_load(f)
        # Update args with config values
        for key, value in config.items():
            setattr(args, key, value)
    return args

def add_particle_selection(s, reader, config):
    """Add particle selection for tracking and calorimeter"""
    # First selection for tracking (charged particles)
    addParticleSelection(
        s,
        config=ParticleSelectorConfig(
            rho=(0.0, 24 * acts.UnitConstants.mm),
            absZ=(0.0, 1.0 * acts.UnitConstants.m),
            eta=(config.g4_post_eta[0], config.g4_post_eta[1]),
            pt=(config.g4_post_pt * acts.UnitConstants.MeV, None),
            removeNeutral=True,
        ),
        inputParticles=reader.config.outputParticlesGenerator,
        outputParticles="particles_selected",
    )
    
    # Second selection for calorimeter (all particles)
    addParticleSelection(
        s,
        config=ParticleSelectorConfig(
            rho=(0.0, 24 * acts.UnitConstants.mm),
            absZ=(0.0, 1.0 * acts.UnitConstants.m),
            eta=(config.g4_post_eta[0], config.g4_post_eta[1]),
            pt=(150 * acts.UnitConstants.MeV, None),
            removeNeutral=False,
        ),
        inputParticles=reader.config.outputParticlesGenerator,
        outputParticles="calo_particles_selected",
    )

def add_digitization(s, tracking_geometry, field, geoDir, config, output_dir):
    """Add digitization to the sequencer"""
    oddDigiConfig = (
        config.digi_config
        if config.digi_config
        else geoDir / "config/odd-digi-smearing-config.json"
    )

    addDigitization(
        s,
        tracking_geometry,
        field,
        digiConfigFile=oddDigiConfig,
        outputDirRoot=output_dir if config.output_root else None,
        outputDirCsv=output_dir if config.output_csv else None,
        rnd=acts.examples.RandomNumbers(seed=config.seed),
    )

def add_tracking(s, tracking_geometry, field, config, output_dir):
    """Add tracking reconstruction chain"""
    # Add seeding
    addSeeding(
        s,
        tracking_geometry,
        field,
        geoSelectionConfigFile=config.seedingConfig,
        outputDirRoot=output_dir if config.output_root else None,
        outputDirCsv=output_dir if config.output_csv else None,
    )

    # Add CKF tracking
    addCKFTracks(
        s,
        tracking_geometry,
        field,
        TrackSelectorConfig(
            pt=(1.0 * acts.UnitConstants.GeV, None),
            absEta=(None, 3.0),
            loc0=(-4.0 * acts.UnitConstants.mm, 4.0 * acts.UnitConstants.mm),
            nMeasurementsMin=7,
            maxHoles=2,
            maxOutliers=2,
        ),
        CkfConfig(
            chi2CutOffMeasurement=15.0,
            chi2CutOffOutlier=25.0,
            numMeasurementsCutOff=10,
            seedDeduplication=True,
            stayOnSeed=True,
            pixelVolumes=[16, 17, 18],
            stripVolumes=[23, 24, 25],
            maxPixelHoles=1,
            maxStripHoles=2,
            constrainToVolumes=[2, 32, 4, 16, 17, 18, 20, 23, 24, 25, 26, 8, 28, 29, 30],
        ),
        outputDirRoot=output_dir if config.output_root else None,
        outputDirCsv=output_dir if config.output_csv else None,
        writeCovMat=True,
    )

    # Add ambiguity resolution
    if config.ambi_solver == "ML":
        addAmbiguityResolutionML(
            s,
            AmbiguityResolutionMLConfig(
                maximumSharedHits=3,
                maximumIterations=1000000,
                nMeasurementsMin=7,
            ),
            outputDirRoot=output_dir if config.output_root else None,
            outputDirCsv=output_dir if config.output_csv else None,
            onnxModelFile=os.path.dirname(__file__) + "/MLAmbiguityResolution/duplicateClassifier.onnx",
        )
    elif config.ambi_solver == "scoring":
        addScoreBasedAmbiguityResolution(
            s,
            ScoreBasedAmbiguityResolutionConfig(
                minScore=0,
                minScoreSharedTracks=1,
                maxShared=2,
                maxSharedTracksPerMeasurement=2,
                pTMax=1400,
                pTMin=0.5,
                phiMax=math.pi,
                phiMin=-math.pi,
                etaMax=4,
                etaMin=-4,
                useAmbiguityFunction=False,
            ),
            outputDirRoot=output_dir if config.output_root else None,
            outputDirCsv=output_dir if config.output_csv else None,
            ambiVolumeFile=config.ambi_config,
            writeCovMat=True,
        )
    else:
        addAmbiguityResolution(
            s,
            AmbiguityResolutionConfig(
                maximumSharedHits=3,
                maximumIterations=1000000,
                nMeasurementsMin=7,
            ),
            outputDirRoot=output_dir if config.output_root else None,
            outputDirCsv=output_dir if config.output_csv else None,
            writeCovMat=True,
        )

def add_vertexing(s, field, config, output_dir):
    """Add vertex fitting to the sequencer"""
    addVertexFitting(
        s,
        field,
        vertexFinder=VertexFinder.AMVF,
        outputDirRoot=output_dir if config.output_root else None,
    )

def write_root_output(s, output_dir, logger):
    """Write ROOT output files"""
    logger = logger or setup_logging("WriteRootOutput")
    logger.info(f"Writing ROOT output to {output_dir}")

    # Write tracking hits
    s.addWriter(acts.examples.RootSimHitWriter(
        config=acts.examples.RootSimHitWriter.Config(
            filePath=str(output_dir / "simhits.root"),
            inputSimHits="simhits"
        ),
        level=acts.logging.INFO
    ))

    # Write calorimeter hits
    s.addWriter(acts.examples.RootCaloHitWriter(
        config=acts.examples.RootCaloHitWriter.Config(
            filePath=str(output_dir / "calohits.root"),
            inputCaloHits="calohits"
        ),
        level=acts.logging.INFO
    ))

    # Write particles
    try:
        s.addWriter(acts.examples.RootParticleWriter(
            config=acts.examples.RootParticleWriter.Config(
                filePath=str(output_dir / "particles.root"),
            inputParticles="particles_selected"
        ),
            level=acts.logging.INFO
        ))
    except Exception as e:
        logger.warning(f"Failed to write particles: {str(e)}")

class TimingRecorder:
    def __init__(self, output_dir):
        self.timings = {}
        self.output_dir = Path(output_dir)  # Ensure it's a Path object
        self.start_time = time.time()
        self.errors = []

    @contextmanager
    def record(self, name):
        start = time.time()
        try:
            yield
        except Exception as e:
            self.errors.append(f"Error in {name}: {str(e)}")
            raise  # Re-raise the exception after logging
        finally:
            end = time.time()
            self.timings[name] = end - start

    def write_report(self):
        try:
            total_time = time.time() - self.start_time
            report = ["Timing Report", "============="]
            
            # Add timing entries
            for name, duration in sorted(self.timings.items()):
                report.append(f"{name:<30} : {duration:>.2f} seconds")
            
            report.append("-" * 50)
            report.append(f"{'Total time':<30} : {total_time:>.2f} seconds")
            
            # Add error section if there were any errors
            if self.errors:
                report.append("\nErrors encountered:")
                report.append("===================")
                for error in self.errors:
                    report.append(error)
            
            # Ensure output directory exists
            self.output_dir.mkdir(parents=True, exist_ok=True)
            
            # Write to file
            report_path = self.output_dir / "timing_report.txt"
            with open(report_path, "w") as f:
                f.write("\n".join(report))
            
            # Print to console
            print("\n".join(report))
            
        except Exception as e:
            print(f"Error writing timing report: {str(e)}")
            print(traceback.format_exc())

def main():
    try:
        # Parse arguments and load config
        args = parse_args()
        config = load_config(args)
        logger = setup_logging()
        
        # Create output directory structure with optional subdirectory
        output_dir = Path(args.output)
        if args.output_subdir:
            output_dir = output_dir / args.output_subdir
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Initialize timing recorder
        timer = TimingRecorder(output_dir)
        
        try:
            # Stage 1: Generate and merge HepMC3 files
            with timer.record("Pythia8 Generation"):
                signal_path, pileup_path = run_pythia_stage(
                    output_dir, config, logger
                )
                if not signal_path.exists() or not pileup_path.exists():
                    raise FileNotFoundError("Pythia8 failed to generate output files")
            
            with timer.record("HepMC3 Merge"):
                merged_path = output_dir / f"merged_{signal_path.stem}.hepmc3"
                merge_hepmc_files(signal_path, pileup_path, merged_path, logger)
                if not merged_path.exists():
                    raise FileNotFoundError("HepMC3 merge failed to generate output file")
            
            # Stage 2: Run DD4hep simulation
            with timer.record("DD4hep Simulation"):
                edm4hep_path = output_dir / "edm4hep.root"
                run_ddsim_stage(merged_path, edm4hep_path, config, logger)
                if not edm4hep_path.exists():
                    raise FileNotFoundError("DD4hep simulation failed to generate output file")
            
            # Stage 3: Run ACTS reconstruction
            s = Sequencer(numThreads=1, events=config.events)
            
            with timer.record("ACTS Setup"):
                setup_acts_reconstruction(s, edm4hep_path, config, output_dir, logger)
                logger.info("Running ACTS reconstruction...")

            # Stage 4: Write output
            if config.output_root:
                with timer.record("ROOT Output Setup"):
                    write_root_output(s, output_dir, logger)

            with timer.record("ACTS Reconstruction"):
                s.run()
                
        except Exception as e:
            logger.error(f"Error during processing: {str(e)}")
            logger.error(traceback.format_exc())
            raise
        
        finally:
            # Always try to write the timing report, even if there was an error
            timer.write_report()
            
    except Exception as e:
        logger.error(f"Fatal error in main: {str(e)}")
        logger.error(traceback.format_exc())
        raise

if __name__ == "__main__":
    main() 
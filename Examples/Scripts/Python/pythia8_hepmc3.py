#!/usr/bin/env python3

from acts.examples import Sequencer
from pathlib import Path
import acts

# Create output directory
output_dir = Path("./pythia8_output")
output_dir.mkdir(exist_ok=True)

s = Sequencer(numThreads=1, events=1)

# Set up Pythia8 with HepMC3 output
outputHepMC = output_dir / "pythia8_events.hepmc3"

from acts.examples.simulation import addPythia8
addPythia8(
    s,
    nhard=1,
    npileup=0,
    hardProcess=["HardQCD:all = on"],
    outputHepMC=outputHepMC,
)

# Run the sequencer
s.run()

print(f"HepMC3 output written to: {outputHepMC}") 
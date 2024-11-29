import pytest
import shutil
from pathlib import Path

from acts.examples import (
    Sequencer,
    AssertCollectionExistsAlg,
)


@pytest.mark.skipif(not hepmc3Enabled, reason="HepMC3 plugin not available")
@pytest.mark.skipif(not pythia8Enabled, reason="Pythia8 not available")
def test_pythia8_hepmc3_output(tmp_path):
    s = Sequencer(numThreads=1)

    # Set up Pythia8 with HepMC3 output
    outputHepMC = tmp_path / "pythia8_events.hepmc3"
    
    from acts.examples.simulation import addPythia8
    addPythia8(
        s,
        nhard=1,
        npileup=0,
        beam=[2212, 2212],  # proton-proton
        cmsEnergy=14000,  # 14 TeV
        hardProcess=["HardQCD:all = on"],
        outputHepMC=outputHepMC,
    )

    # Run the sequencer
    s.run()

    # Verify the output file exists and is not empty
    assert outputHepMC.exists()
    assert outputHepMC.stat().st_size > 0

    # Try reading it back with HepMC3Reader
    s = Sequencer(numThreads=1)
    
    from acts.examples.hepmc3 import HepMC3AsciiReader
    s.addReader(
        HepMC3AsciiReader(
            level=acts.logging.INFO,
            inputDir=str(outputHepMC.parent),
            inputStem=outputHepMC.stem,
            outputEvents="hepmc-events",
        )
    )

    # Add verification
    alg = AssertCollectionExistsAlg(
        "hepmc-events", name="check_events", level=acts.logging.INFO
    )
    s.addAlgorithm(alg)

    s.run() 
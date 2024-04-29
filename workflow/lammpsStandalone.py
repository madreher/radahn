import sys
from pathlib import Path
import argparse
import shutil

try:
    import godrick
except ImportError or ModuleNotFoundError:
    print('Unable to find godrick. Make sure that the godrick python package is installed in your environement.',  file=sys.stderr)
    raise

from godrick.computeResources import ComputeCollection
from godrick.workflow import Workflow
from godrick.task import MPITask, MPIPlacementPolicy
from godrick.launcher import MainLauncher

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)


    parser.add_argument("--workdir",
		                help = "Folder in which to generate the various files.",
		                dest = "workdir",
		                required = True)
    
    args = parser.parse_args()

    # Print the command line for logging purposes
    print("Commandline:", end=" ")
    for i in range(1, len(sys.argv)):
        print(sys.argv[i], end = " ")
    print("")

    cluster = ComputeCollection(name="myCluster")
    cluster.initFromLocalhost(useHT=True)

    workflow = Workflow("LammpsStandAlone")
    benzeneFolder = "/home/matthieu/dev/radahn/models/benzene_reax"
    fileDataPath =  benzeneFolder + "/input.data"
    fileLmpPath = benzeneFolder + "/input.lmp"
    filePotentialFile = benzeneFolder + "/ffield.reax.Fe_O_C_H"
    workFolder = Path(args.workdir)
    assert workFolder.is_dir()
    
    lammps = MPITask(name="lammps", cmdline=f"/home/matthieu/dev/radahn/build/install/bin/lammpsDriver --name lammps --config {workflow.getConfigurationFile()} --initlmp input.lmp", placementPolicy=MPIPlacementPolicy.ONETASKPERCORE, resources=cluster.splitNodesByCoreRange([1])[0])
    lammps.addInputPort("in")
    lammps.addOutputPort("out")

    workflow.declareTask(lammps)

    launcher = MainLauncher()
    launcher.generateOutputFiles(workflow=workflow)

    # Copy the model files to the workdir
    shutil.copy2(fileDataPath, workFolder)
    shutil.copy2(fileLmpPath, workFolder)
    shutil.copy2(filePotentialFile, workFolder)


# Boilerplate name guard
if __name__ == "__main__":
    main()
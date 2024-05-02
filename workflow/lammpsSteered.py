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
from godrick.communicator import MPIPairedCommunicator, MPICommunicatorProtocol

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)


    parser.add_argument("--workdir",
		                help = "Folder in which to generate the various files.",
		                dest = "workdir",
		                required = True)
    parser.add_argument("--nvesteps",
		                help = "Total number of steps to run the NVE process.",
		                dest = "nvesteps",
		                type=int)
    parser.set_defaults(nvesteps=1000)
    parser.add_argument("--frequpdate",
		                help = "Output data every <update> simulation steps.",
		                dest = "frequpdate",
		                type=int)
    parser.set_defaults(frequpdate=100)
    parser.add_argument("--ncores",
		                help = "Number of physical cores assigned to Lammps.",
		                dest = "ncores",
		                type=int)
    parser.set_defaults(ncores=1)
    
    args = parser.parse_args()

    # Print the command line for logging purposes
    print("Commandline:", end=" ")
    for i in range(1, len(sys.argv)):
        print(sys.argv[i], end = " ")
    print("")

    # Initialization of the compute resources available
    # In this example, we will only run on the host machine
    cluster = ComputeCollection(name="myMachine")
    cluster.initFromLocalhost(useHT=True)
    nCoresHost = len(cluster.getListOfCores())

    # Checking the files necessary to run the simulation
    # TODO: extend command line parser to provide the files via cli
    benzeneFolder = "/home/matthieu/dev/radahn/models/benzene_reax"
    fileDataPath =  benzeneFolder + "/input.data"
    fileLmpPath = benzeneFolder + "/input.lmp"
    filePotentialFile = benzeneFolder + "/ffield.reax.Fe_O_C_H"
    workFolder = Path(args.workdir)
    if not workFolder.is_dir():
        raise FileNotFoundError(f"The working directory {workFolder} requested by the user does not exist.")
    
    # Everything is available, copying the model files to the workdir
    shutil.copy2(fileDataPath, workFolder)
    shutil.copy2(fileLmpPath, workFolder)
    shutil.copy2(filePotentialFile, workFolder)

    # Workflow initialization
    workflow = Workflow("LammpsSteered")
    
    # Lammps Task declaration
    lammpsCmd = f"/home/matthieu/dev/radahn/build/install/bin/lammpsDriver --name lammps --config {workflow.getConfigurationFile()}"
    lammpsCmd += " --initlmp input.lmp"
    lammpsCmd += f" --maxnvesteps {args.nvesteps}"
    lammpsCmd += f" --intervalsteps {args.frequpdate}"

    if args.ncores + 1 > nCoresHost:
        raise ValueError(f"User requested {args.ncores+1} physical cores for Lammps and the engine, but the localhost only has {nCoresHost} physical cores.")
    splitResources = cluster.splitNodesByCoreRange([args.ncores, 1])
    lammpsResources = splitResources[0]
    print(f"Number of cores assigned to Lammps: {args.ncores}")

    lammps = MPITask(name="lammps", cmdline=lammpsCmd, placementPolicy=MPIPlacementPolicy.ONETASKPERCORE, resources=lammpsResources)
    lammps.addInputPort("in")
    lammps.addOutputPort("atoms")

    # Engine Task declaration
    engineCmd = f"/home/matthieu/dev/radahn/build/install/bin/engine --name engine --config {workflow.getConfigurationFile()}"
    engineResources = splitResources[1]
    engine = MPITask(name="engine", cmdline=engineCmd, placementPolicy=MPIPlacementPolicy.ONETASKPERCORE, resources=engineResources)
    engine.addInputPort("atoms")
    engine.addOutputPort("motorscmd")

    # Communicator declaration
    simToEngine =  MPIPairedCommunicator(id="simToEngine", protocol=MPICommunicatorProtocol.PARTIAL_BCAST_GATHER)
    simToEngine.connectToInputPort(engine.getInputPort("atoms"))
    simToEngine.connectToOutputPort(lammps.getOutputPort("atoms"))


    # Declaring the tasks and communicators
    workflow.declareTask(lammps)
    workflow.declareTask(engine)
    workflow.declareCommunicator(simToEngine)

    # Process the workflow
    launcher = MainLauncher()
    launcher.generateOutputFiles(workflow=workflow)


# Boilerplate name guard
if __name__ == "__main__":
    main()
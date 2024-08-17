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
from godrick.communicator import MPIPairedCommunicator, MPICommunicatorProtocol, ZMQGateCommunicator, ZMQCommunicatorProtocol, ZMQBindingSide, CommunicatorGateSideFlag, CommunicatorMessageFormat

def main():

    # Default files used for testing.
    # TODO: extend command line parser to provide the files via cli
    benzeneFolder = "/home/matthieu/dev/radahn/models/benzene_reax"
    fileDataPath =  benzeneFolder + "/input.data"
    fileLmpPath = benzeneFolder + "/input.lmp"
    filePotentialFile = benzeneFolder + "/ffield.reax.Fe_O_C_H"
    fileMotorConfig = ""
    fileLmpConfig = ""
    useTestMotorSetup = False
    forceMaxSteps = False
    installFolder = Path(__file__).parent.parent.resolve()

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
    parser.add_argument("--lmpdata",
                        help = "Lammps data file.",
                        dest = "lmpdata",
                        required = True)
    parser.add_argument("--lmpinput",
                        help = "Lammps input file.",
                        dest = "lmpinput",
                        required = True)
    parser.add_argument("--potential",
                        help = "Lammps potential file.",
                        dest = "potential",
                        required = True)
    parser.add_argument("--motorconfig",
                        help = "Motor configuration file.",
                        dest = "motorconfig",
                        required = False)
    parser.add_argument("--lmpconfig",
                        help="Path to configuration file used by Lammps",
                        dest="lmpconfig",
                        required=False)
    parser.add_argument("--testmotorsetup",
                        help = "Use a test motor configuration.",
                        dest = "testmotors",
                        action='store_true',
                        required = False)
    parser.add_argument("--forcemaxsteps",
                        help="Force the simulation to run for the requested number of steps, even if all the motors have completed.",
                        dest="forcemaxsteps",
                        action='store_true',
                        required=False)
    
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
    fileDataPath = Path(args.lmpdata)
    if not fileDataPath.is_file():
        raise FileNotFoundError(f"The data file {fileDataPath} requested by the user does not exist.")
    
    fileLmpPath = Path(args.lmpinput)
    if not fileLmpPath.is_file():
        raise FileNotFoundError(f"The input file {fileLmpPath} requested by the user does not exist.")
    
    filePotentialFile = Path(args.potential)
    if not filePotentialFile.is_file():
        raise FileNotFoundError(f"The potential file {filePotentialFile} requested by the user does not exist.")
    
    if args.motorconfig is not None:
        fileMotorConfig = Path(args.motorconfig)
        if not fileMotorConfig.is_file():
            raise FileNotFoundError(f"The motor configuration file {fileMotorConfig} requested by the user does not exist.")
        
    if args.lmpconfig is not None:
        fileLmpConfig = Path(args.lmpconfig)
        if not fileLmpConfig.is_file():
            raise FileNotFoundError(f"The group configuration file {fileLmpConfig} requested by the user does not exist.")

    if args.testmotors:
        useTestMotorSetup = True
    if args.forcemaxsteps:
        forceMaxSteps = True
    
    if useTestMotorSetup and args.motorconfig is not None:
        raise ValueError("The --testmotors and --motorconfig options are mutually exclusive.")
    
    if not useTestMotorSetup and args.motorconfig is None and not forceMaxSteps:
        raise ValueError("No motor configuration file provided. Please use --testmotors or --motorconfig or --forcemaxsteps.")

    workFolder = Path(args.workdir)
    if not workFolder.is_dir():
        raise FileNotFoundError(f"The working directory {workFolder} requested by the user does not exist.")

    # Everything is available, copying the files in the working directory if needed
    if not fileDataPath.parents[0].samefile(workFolder):
        print(f"Copying {fileDataPath} from {fileDataPath.parents[0]} to {workFolder}")
        shutil.copy2(fileDataPath, workFolder)
    if not fileLmpPath.parents[0].samefile(workFolder):
        shutil.copy2(fileLmpPath, workFolder)
    if not filePotentialFile.parents[0].samefile(workFolder):
        shutil.copy2(filePotentialFile, workFolder)


    # Workflow initialization
    workflow = Workflow("LammpsSteered")
    
    # Lammps Task declaration
    lammpsCmd = f"{installFolder}/bin/lammpsDriver --name lammps --config {workflow.getConfigurationFile()}"
    lammpsCmd += f" --initlmp {fileLmpPath.name}"
    lammpsCmd += f" --maxnvesteps {args.nvesteps}"
    lammpsCmd += f" --intervalsteps {args.frequpdate}"
    if args.lmpconfig is not None:
        lammpsCmd += f" --lmpconfig {fileLmpConfig.name}"


    if args.ncores + 1 > nCoresHost:
        raise ValueError(f"User requested {args.ncores+1} physical cores for Lammps and the engine, but the localhost only has {nCoresHost} physical cores.")
    splitResources = cluster.splitNodesByCoreRange([args.ncores, 1])
    lammpsResources = splitResources[0]
    print(f"Number of cores assigned to Lammps: {args.ncores}")

    lammps = MPITask(name="lammps", cmdline=lammpsCmd, placementPolicy=MPIPlacementPolicy.ONETASKPERCORE, resources=lammpsResources)
    lammps.addInputPort("in")
    lammps.addOutputPort("atoms")

    # Engine Task declaration
    engineCmd = f"{installFolder}/bin/engine --name engine --config {workflow.getConfigurationFile()}"
    if useTestMotorSetup:
        engineCmd += " --testmotors"
    if args.motorconfig is not None:
        engineCmd += f" --motors {fileMotorConfig.name}"
    if forceMaxSteps:
        engineCmd += f" --forcemaxsteps"
    engineResources = splitResources[1]
    engine = MPITask(name="engine", cmdline=engineCmd, placementPolicy=MPIPlacementPolicy.ONETASKPERCORE, resources=engineResources)
    engine.addInputPort("atoms")
    engine.addInputPort("usercmd")
    engine.addOutputPort("motorscmd")
    engine.addOutputPort("kvs")
    engine.addOutputPort("atoms")

    # Communicator declaration
    simToEngine =  MPIPairedCommunicator(id="simToEngine", protocol=MPICommunicatorProtocol.PARTIAL_BCAST_GATHER)
    simToEngine.connectToInputPort(engine.getInputPort("atoms"))
    simToEngine.connectToOutputPort(lammps.getOutputPort("atoms"))

    engineToSim = MPIPairedCommunicator(id="engineToSim", protocol=MPICommunicatorProtocol.PARTIAL_BCAST_GATHER)
    engineToSim.connectToInputPort(lammps.getInputPort("in"))
    engineToSim.connectToOutputPort(engine.getOutputPort("motorscmd"))
    engineToSim.setNbToken(1)

    # Open gates
    outKVSEngine = ZMQGateCommunicator(name="kvsGate", side=CommunicatorGateSideFlag.OPEN_SENDER, protocol=ZMQCommunicatorProtocol.PUB_SUB, bindingSide=ZMQBindingSide.ZMQ_BIND_SENDER, format=CommunicatorMessageFormat.MSG_FORMAT_JSON, port=50000)
    outKVSEngine.connectToOutputPort(engine.getOutputPort("kvs"))
    outAtomsEngine = ZMQGateCommunicator(name="atomsGate", side=CommunicatorGateSideFlag.OPEN_SENDER, protocol=ZMQCommunicatorProtocol.PUB_SUB, bindingSide=ZMQBindingSide.ZMQ_BIND_SENDER, format=CommunicatorMessageFormat.MSG_FORMAT_JSON, port=50001)
    outAtomsEngine.connectToOutputPort(engine.getOutputPort("atoms"))
    inCmdEngine = ZMQGateCommunicator(name="cmdGate", side=CommunicatorGateSideFlag.OPEN_RECEIVER, protocol=ZMQCommunicatorProtocol.PUSH_PULL, bindingSide=ZMQBindingSide.ZMQ_BIND_RECEIVER, format=CommunicatorMessageFormat.MSG_FORMAT_JSON, port=50002, nonblocking=True)
    inCmdEngine.connectToInputPort(engine.getInputPort("usercmd"))

    # Declaring the tasks and communicators
    workflow.declareTask(lammps)
    workflow.declareTask(engine)
    workflow.declareCommunicator(simToEngine)
    workflow.declareCommunicator(engineToSim)
    workflow.declareCommunicator(outKVSEngine)
    workflow.declareCommunicator(outAtomsEngine)
    workflow.declareCommunicator(inCmdEngine)

    # Process the workflow
    launcher = MainLauncher()
    launcher.generateOutputFiles(workflow=workflow)


# Boilerplate name guard
if __name__ == "__main__":
    main()
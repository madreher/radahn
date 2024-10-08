from flask import Flask, render_template, request, send_file
import logging
import threading
from threading import Lock, Event
import zmq
from flask_socketio import SocketIO
import uuid
from pathlib import Path
import os
import numpy as np
import subprocess
import platform
import time
import sys
import platform
from copy import deepcopy
import json
import io
import zipfile

from ase.io import write, read
from ase.atoms import Atoms

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.logger.setLevel(logging.INFO)
socketio = SocketIO(app)
#app = None
#socketio = None

# Threads listening to the KVS messages
thread = None
thread_lock = Lock()

# Threads listening to the atoms messages
threadAtoms = None
thread_lockAtoms = Lock()

# Threads generating inputs
threadGenerateInputs = None
thread_lockGenerateInputs = Lock()

# Thread running Radahn. This generates inputs and execute the simulation on the local host
threadRadahn = None
thread_lockRadahn = Lock()

# Thread opening the job folder
threadOpenJobFolder = None
thread_lockOpenJobFolder = Lock()

# Thread table for all the tasks which can be executed
# Note: not all the tasks are going to be using events
threadTable = {}
threadTable["listenKVS"] = {"thread": None, "lock": Lock(), "event": Event()}
threadTable["listenAtoms"] = {"thread": None, "lock": Lock(), "event": Event()}
threadTable["generateInputs"] = {"thread": None, "lock": Lock(), "event": Event()}
threadTable["runSimulation"] = {"thread": None, "lock": Lock(), "event": Event()}
threadTable["openJobFolder"] = {"thread": None, "lock": Lock(), "event": Event()}


# Default variables
rootJobFolder = Path(os.getenv("HOME") + "/.radahn/jobs")
projectJobFolder = Path(os.getenv("HOME") + "/.radahn/projects")
radahnFolder = Path(os.getenv("HOME") + "/dev/radahn/build/install")
radahnScript = radahnFolder / "workflow" / "lammpsSteered.py"
execEnvironment = "NATIVE"  # Dev note: Will probably be needed when return path of the job folder 
                            # Will probably need some difference when the app is running on native Linux, in Docker Linux, on WSL


# Override default paths if requested 
if "RADAHN_FRONTEND_ENV_PATH" in os.environ:
    try:
        from dotenv import load_dotenv
        load_dotenv(os.environ["RADAHN_FRONTEND_ENV_PATH"])

        rootJobFolder = Path(os.getenv("RADAHN_ROOT_JOB_FOLDER"))
        projectJobFolder = Path(os.getenv("RADAHN_ROOT_PROJECT_FOLDER"))
        radahnFolder = Path(os.getenv("RADAHN_INSTALL_FOLDER"))
        radahnScript = radahnFolder / "workflow" / "lammpsSteered.py"
        execEnvironment = os.getenv("RADAHN_EXEC_ENVIRONMENT")
        app.logger.info(f"Loaded environment variables from {os.environ['RADAHN_FRONTEND_ENV_PATH']}")
    except Exception as e:
        app.logger.warn(f"Unable to load environment variables from {os.environ['RADAHN_FRONTEND_ENV_PATH']}: {e}")


@app.route('/')
def index():
    return render_template('index.html')

@app.route('/download_job_folder')
def download():
    # First check that we can find the job folder
    jobFolder = request.args.get('job_folder')
    if not jobFolder:
        propagateLog({"msg": "job_folder argument not provided when requesting to download a job folder.", "level": "error"})
        return 
    jobFolderPath = rootJobFolder / Path(jobFolder)

    if not jobFolderPath.is_dir():
        propagateLog({"msg": f"Unable to locate the folder for the job {jobFolder}.", "level":"error"})
        return 
    
    # Job folder does exist, zipping it 
    zipFileName = f"{jobFolder}.zip"
    zipFilePath = rootJobFolder / zipFileName

    with zipfile.ZipFile(zipFilePath, "w", zipfile.ZIP_DEFLATED) as zip_file:
        for root, dirs, files in os.walk(jobFolderPath):
            for file in files:
                filePath = os.path.join(root, file)
                arcname = os.path.relpath(filePath, jobFolderPath)
                zip_file.write(filePath, arcname)

    return send_file(zipFilePath, as_attachment=True, download_name=zipFileName, mimetype='application/zip')


def propagateLog(msgConfig):
    level = msgConfig["level"]
    if level == "info":
        app.logger.info(msgConfig["msg"])
    elif level == "warning":
        app.logger.warning(msgConfig["msg"])
    elif level == "error":
        app.logger.error(msgConfig["msg"])
    elif level == "log":
        app.logger.info(msgConfig["msg"])
    else:
        app.logger.info(msgConfig["msg"])
    socketio.emit('log_message', msgConfig)

def convertCmd(cmd):
    """
    Converts the incoming cmd string into a format appropriate
    for the system to pass to subprocess.run
    """
    if (platform.system() == 'Windows'):
        return cmd.split()
    return cmd

def runAsyncProcess(taskname, cmd, cwd, runscript, env = {}):
    """
    Execute a command and check its return code.
    Return the Popen object which can be polled by the caller

    taskname: Name of the task to be used for logging
    cmd: Command to execute
    cwd: Job folder
    runscript: Open file descriptor in which the command will be written
    """
    print("\n\nExecuting: " + cmd)
    runscript.write(cmd + "\n\n")
    stdOut = os.path.join(cwd, taskname + ".stdout.txt")
    fileStdOut = open(stdOut, "w")
    stdErr = os.path.join(cwd, taskname + ".stderr.txt")
    fileStdErr = open(stdErr, "w")

    if len(env) > 0:
        result = subprocess.Popen(
                convertCmd(cmd),
                shell=True, cwd=cwd,
                stdout=fileStdOut, stderr=fileStdErr, env=env)
    else:
        result = subprocess.Popen(
                convertCmd(cmd),
                shell=True, cwd=cwd,
                stdout=fileStdOut, stderr=fileStdErr)

    return result

class JobRunner:
    """
    Process a serie of tasks asynchronously in a given order.

    name: Name of the group of job used to name the runfile
    jobFolder: folder in which the tasks will be executed
    """

    def __init__(self, name, jobFolder):
        self.name = name
        self.runScriptPath = os.path.join(jobFolder, f"{name}_run.sh")
        self.runScriptFile = open(self.runScriptPath, "w")
        self.runScriptFile.write("#! /bin/bash\n\n\n")
        self.jobList = []
        self.currentTask = 0
        self.jobFolder = jobFolder
        self.currentTaskHandler = None

    def addTask(self, name, cmd, env = {}):
        job = {}
        job["name"] = name
        job["cmd"] = cmd
        job["folder"] =  self.jobFolder
        job["env"] = env
        self.jobList.append(job)

    def processTasks(self):

        # All the tasks are completed
        if self.currentTask >= len(self.jobList):
            return True

        # Checking if the current task has already started
        if self.currentTaskHandler == None:
            # Not started yet, starting it
            job = self.jobList[self.currentTask]
            self.currentTaskHandler = runAsyncProcess(job["name"], job["cmd"], job["folder"], self.runScriptFile, job["env"])
            return False
        else:
            # Task is already running, checking if it is completed
            pollResult = self.currentTaskHandler.poll()

            # Task is still running
            if pollResult == None:
                return False
            else:
                # Task is completed. Storing the result and updating the structure
                self.jobList[self.currentTask]["result"] = pollResult
                self.currentTaskHandler = None
                self.currentTask = self.currentTask + 1

                if self.currentTask >= len(self.jobList):
                    return True
                else:
                    # The next task will be started at the next call
                    return False

    def getJobList(self):
        return self.jobList


def getElementsFromData(dataPath: str) -> str:
    """Get elements from data file

    Args:
        data_file: LAMMPS data file

    Returns:
        elements: LAMMPS atom types

    """
    with open(dataPath, 'r') as f:
        lines = f.readlines()
    i = 0
    for line in lines:
        if line[:6] == 'Masses':
            ini = i + 2
        if line[:5] == 'Atoms':
            fin = i - 1
        i += 1
    elements = ''
    for line in lines[ini:fin]:
        elements += ' ' + line.split()[-1]
    return elements

def xyzToLammpsDataPBC(xyzPath:str, dataPath:str) -> Atoms:
    """Convert xyz to LAMMPS data

    Args:
        xyz_file: XYZ file

    Returns:
        data_file: LAMMPS data file

    """
    atoms = read(xyzPath)

    # TODO: decide what to do with the cell. Problematic cases are if the positions are negative and 
    # how to ensure that the box is large enough to avoid self bitting with the period conditions
    # Still set a dummy cell to avoid an error from ASE, will be replaced when rewriting the file
    atoms.set_cell([500, 500, 500])
    #if atoms.get_positions().min() < 0: # ASE always assume the lower corner to be (0,0,0)
    #    atoms.center()
    tempDataPath = str(dataPath) + '.temp'
    write(filename=tempDataPath, images=atoms, format='lammps-data', atom_style='full')

    # ASE always write the bounding starting from 0,0,0
    # We have to rewrite it manually to handle cases where positions can be in the negative
    # Doing it now avoir the need to translate back the trajectory later on to match the user input.
    positions = np.array(atoms.get_positions())

    xcoords = positions[:,0]
    ycoords = positions[:,1]
    zcoords = positions[:,2]

    xmin = xcoords.min()
    ymin = ycoords.min()
    zmin = zcoords.min()

    xmax = xcoords.max()
    ymax = ycoords.max()
    zmax = zcoords.max()

    padding = 50.0 # Adding 50A for now
    propagateLog({ "msg": f"bbox: [{xmin} {ymin} {zmin}] [{xmax} {ymax} {zmax}], padding: {padding} A", "level": "info"})


    chemical_symbols = atoms.get_chemical_symbols()
    masses = atoms.get_masses()
    indexes = np.unique(chemical_symbols, return_index=True)[1]
    masses_u = [masses[index] for index in indexes]
    chemical_symbols_u = [chemical_symbols[index] for index in indexes]

    with open(tempDataPath, 'r') as f:
        lines = f.readlines()

    with open(dataPath, 'w') as f:
        f.write('# Generated by Radahn\n')

        # Find the first line which start with a number
        startOffset = 0
        for i in range(len(lines)):
            if len(lines[i].split()) > 0 and lines[i].split()[0].isdigit():
                startOffset = i
                break

        if startOffset > len(lines)-1:
            raise RuntimeError("Could not find the first line in LAMMPS data file")
        
        # Copy the lines until the line ending with zhi is found
        offset = 0
        for i in range(startOffset, len(lines)):
            #f.write(lines[i])
            #if len(lines[i].split()) > 0 and lines[i].split()[-1] == 'zhi':
            #    break
            if len(lines[i].split()) > 0 and lines[i].split()[-1] == 'xhi':
                f.write(f"{xmin - padding}\t{xmax + padding} xlo xhi\n")
            elif len(lines[i].split()) > 0 and lines[i].split()[-1] == 'yhi':
                f.write(f"{ymin - padding}\t{ymax + padding} ylo yhi\n")
            elif len(lines[i].split()) > 0 and lines[i].split()[-1] == 'zhi':
                f.write(f"{zmin - padding}\t{zmax + padding} zlo zhi\n")
                break
            else:
                f.write(lines[i])
                offset += 1
        if offset > len(lines)-1:
            raise RuntimeError("Could not find zhi in LAMMPS data file")
        

        # Insert the masses in the data file so it doesn't have to be added separatly in the script file
        f.write('\n')
        f.write('Masses\n')
        f.write('\n')
        for i in range(len(masses_u)):
            f.write(f'{i + 1} {masses_u[i]} # {chemical_symbols_u[i]}\n')
        f.write('\n')
        f.write('Atoms # full\n')
        f.write('\n')

        # Find the offset of the Atoms section
        for i in range(offset, len(lines)):
            if lines[i].startswith('Atoms'):
                offset = i+2
                break
        if offset > len(lines)-1:
            raise RuntimeError("Could not find Atoms in LAMMPS data file")

        # Now that the masses are added, we can add the rest of the lines.
        for i in range(offset, len(lines)):
            f.write(lines[i])
    
    return atoms

#def generate_inputs(xyz:str, ffContent:str, ffFileName:str, motors:str) -> str:
def generate_inputs(configTask:dict) -> str:

    # Create the job folder 
    jobID = uuid.uuid4()
    jobFolder = rootJobFolder / str(jobID)
    os.makedirs(jobFolder)
    app.logger.info(f"Job folder created: {jobFolder}")

    try:
        # Create the necessary inputs files 
        xyzFile = jobFolder / "input.xyz"
        with open(xyzFile, "w") as f:
            f.write(configTask["xyz"])
            f.close()

        ffFile = jobFolder / configTask["ffName"]
        with open(ffFile, "w") as f:
            f.write(configTask["ffContent"])
            f.close()

        if len(configTask['motors']) > 0:
            motorsFile = jobFolder / "motors.json"
            with open(motorsFile, "w") as f:
                f.write(configTask["motors"])
                f.close()

        if len(configTask['lmp_config']) > 0:
            lmp_configFile = jobFolder / "lmp_config.json"
            with open(lmp_configFile, "w") as f:
                f.write(configTask["lmp_config"])
                f.close()
            propagateLog({"msg": "Configuration file generated for lammps.", "level": "info"})
        else:
            propagateLog({"msg": "No configuration file generated for lammps.", "level": "info"})

        # Convert the XYZ to a .data file 
        dataFile = jobFolder / "input.data"
        atoms = xyzToLammpsDataPBC(xyzFile, dataFile)
        propagateLog({"msg": "Lammps data file generated.", "level": "info"})
        

        # Generate the base Lammps script
        lammpsScriptFile = jobFolder / "input.lammps"
        useAcks2 = False

        ffExtension = (configTask["ffName"].split("."))[-1]
        validExtensions = ["reax", "rebo", "airebo", "airebo-m"]
        if ffExtension not in validExtensions:
            propagateLog({"msg": "Unrecognized potential file format. Accepted extensions are " + validExtensions, "level": "error"})
        with open(lammpsScriptFile, "w") as f:

            # Extract the simulation box
            cell = atoms.get_cell()
            minCellDim = min([cell[0][0], cell[1][1], cell[2][2]])
            #print(f"Minimum cell dimension: {minCellDim}")

            # Get back the list of elements
            elements = getElementsFromData(dataFile)

            scriptContent = "# -*- mode: lammps -*-\n"
            if ffExtension == "reax":
                scriptContent += 'units          real\n'
            else:
                scriptContent += 'units          metal\n'
            scriptContent += 'atom_style     full\n'
            scriptContent += 'atom_modify    map hash\n'
            scriptContent += 'newton         on\n'
            scriptContent += 'boundary       p p p\n'

            scriptContent += 'read_data      input.data\n'
            #for i in range(len(indexes)):
            #    scriptContent += f'mass           {i + 1} {masses_u[i]}\n' 
            
            if ffExtension == "reax":
                scriptContent += 'pair_style     reaxff NULL mincap 1000\n'
                scriptContent += f'pair_coeff     * * {configTask["ffName"]}{elements}\n'
                if useAcks2:
                    scriptContent += 'fix            ReaxFFSpec all acks2/reaxff 1 0.0 10.0 1e-8 reaxff\n'
                else:
                    scriptContent += 'fix            ReaxFFSpec all qeq/reaxff 1 0.0 10.0 1e-8 reaxff\n'
            elif ffExtension == "rebo":
                scriptContent+='pair_style     rebo\n'
                scriptContent+=f'pair_coeff     * * {configTask["ffName"]}{elements}\n'
            elif ffExtension == 'airebo':
                scriptContent+='pair_style     airebo 3 1 1\n'
                #scriptContent+='pair_style     airebo 3 1 0\n'
                scriptContent+=f'pair_coeff     * * {configTask["ffName"]}{elements}\n'
            elif ffExtension == 'airebo-m':
                scriptContent+='pair_style     airebo/morse 3 1 1\n'
                scriptContent+=f'pair_coeff     * * {configTask["ffName"]}{elements}\n'
            #scriptContent += 'neighbor       2.5 bin\n' 
            # 2.5 is too large for small molecule like benzene. Trying to compute a reasonable cell skin based on the simulation box
            scriptContent += f"neighbor       {min([2.5, minCellDim/2])} bin\n"
            scriptContent += 'neigh_modify   every 1 delay 0 check yes\n'

            if ffExtension in ["airebo"]:
                scriptContent +="""compute 0 all pair airebo
variable REBO     equal c_0[1]
variable LJ       equal c_0[2]
variable TORSION  equal c_0[3]

thermo 1
thermo_style custom step etotal pe ke epair v_REBO v_LJ v_TORSION
"""

            if "minimize_config" in configTask:

                minimizationType = configTask["minimize_config"]["type"]
                scriptContent +="\n### Minimization\n"

                # Check if we have anchors
                anchorAtomIds = []
                if len(configTask["lmp_config"]) > 0:
                    jsonAnchor = json.loads(configTask["lmp_config"])
                    if "anchors" in jsonAnchor.keys():
                        
                        for anchorGrp in jsonAnchor["anchors"]:
                            anchorAtomIds.extend(anchorGrp["selection"])

                # If we have anchors, the atom forces must be set to 0 prior to minimization
                if len(anchorAtomIds) > 0:
                    scriptContent += "group minanchorgrp id"
                    for id in anchorAtomIds:
                        scriptContent += " " + str(id)
                    scriptContent += "\n"
                    scriptContent += "fix minanchorfix minanchorgrp setforce 0.0 0.0 0.0\n"
                
                scriptContent +='min_style      cg\n'
                scriptContent +='minimize       0.0001 1.0e-5 100 100000\n'

                if ffExtension == 'reax':
                    scriptContent +='min_style      hftn\n'
                    scriptContent +='minimize       0.0001 1.0e-5 100 100000\n'

                scriptContent +='min_style      sd\n'
                scriptContent +='minimize       0.0001 1.0e-5 100 100000\n'

                if minimizationType == "deep":
                
                    scriptContent += f'variable       i loop 100\n'
                    scriptContent += f'label          loop1\n'
                    scriptContent += f'variable       ene_min equal pe\n'
                    scriptContent += 'variable       ene_min_i equal ${ene_min}\n'

                    scriptContent += f'min_style      cg\n'
                    scriptContent += f'minimize       1.0e-10 1.0e-10 10000 100000\n'

                    if ffExtension == 'reax':
                        scriptContent += f'min_style      hftn\n'
                        scriptContent += f'minimize       1.0e-10 1.0e-10 10000 100000\n'

                    scriptContent += f'min_style      sd\n'
                    scriptContent += f'minimize       1.0e-10 1.0e-10 10000 100000\n'

                    scriptContent += f'variable       ene_min_f equal pe\n'
                    scriptContent += 'variable       ene_diff equal ${ene_min_i}-${ene_min_f}\n'
                    scriptContent += 'print          "Delta_E = ${ene_diff}"\n'
                    scriptContent += 'if             "${ene_diff}<1e-6" then "jump SELF break1"\n'
                    scriptContent += f'print          "Loop_id = $i"\n'
                    scriptContent += f'next           i\n'
                    scriptContent += f'jump           SELF loop1\n'
                    scriptContent += f'label          break1\n'
                    scriptContent += f'variable       i delete\n'
                
                if len(anchorAtomIds) > 0:
                    scriptContent += "unfix minanchorfix\n"
                    scriptContent += "group minanchorgrp delete\n"


            # Add basic IO setup
            scriptContent += """
####

thermo         50
"""
            if ffExtension in ["airebo"]:
                scriptContent += "thermo_style custom step etotal pe ke epair v_REBO v_LJ v_TORSION"
            else:
                scriptContent += "thermo_style   custom step etotal pe ke temp press pxx pyy pzz lx ly lz"
            scriptContent += """
thermo_modify  flush yes lost error

dump           dump all custom 100 fulltrajectory.dump id type x y z q
dump_modify    dump sort id
dump           xyz all xyz 100 fulltrajectory.xyz
dump_modify    xyz sort id element C H
"""
            if ffExtension == "reax":
                scriptContent +="fix            fixbond all reaxff/bonds 100 bonds_reax.txt"
            scriptContent +="""

####

reset_timestep 0
"""
            if "dt" not in configTask:
                propagateLog({"msg": "Unable to find dt in the task configuration dictionnary.", "level": "error"})
            if "unit_set" not in configTask:
                propagateLog({"msg": "Unable to find the unit set in the task configuration dictionnary.", "level": "error"})
            if ffExtension == 'reax':
                # Reax is real
                if configTask["unit_set"] == "LAMMPS_REAL":
                    scriptContent+=f"timestep       {configTask['dt']}\n"
                elif configTask["unit_set"] == "LAMMPS_METAL":
                    scriptContent+=f"timestep       {configTask['dt'] * 1000.0}\n"
                else:
                    propagateLog({"msg": f"Unit set {configTask['unit_set']} currently not supported.", "level": "error"})
            else:
                # Rebo case, metal unit
                if configTask["unit_set"] == "LAMMPS_REAL":
                    scriptContent+=f"timestep       {configTask['dt'] / 1000.0}\n"
                elif configTask["unit_set"] == "LAMMPS_METAL":
                    scriptContent+=f"timestep       {configTask['dt'] }\n"
                else:
                    propagateLog({"msg": f"Unit set {configTask['unit_set']} currently not supported.", "level": "error"})
            scriptContent += "\n\n"


            f.write(scriptContent)

        # Generate the data file
        #dataFile = jobFolder / "input.data"
        #xyzToLammpsDataPBC(xyzFile, dataFile)
        socketio.emit('job_folder', {'message': jobFolder.absolute().as_posix()})
        app.logger.info(f"Job folder generated: {jobFolder.absolute().as_posix()}")
    except:
        propagateLog({"msg":"Error occured while trying to generate the file inputs.", "level":"error"})
        if "threadName" in configTask:
                threadTable[configTask["threadName"]]["thread"] = None
        return ""
    finally:
        if "threadName" in configTask:
                threadTable[configTask["threadName"]]["thread"] = None
    


    return jobFolder

def stop_simulation():
    propagateLog({"msg": "Received a request to stop the simulation.", "level": "info"})
    try:
        context = zmq.Context()
        socket = context.socket(zmq.PUSH)
        socket.connect("tcp://localhost:50002")

        msg = {}
        msg["header"] = {}
        msg["header"]["format"] = "enginecmd"
        msg["header"]["generator"] = "radahn_frontend"
        msg["header"]["version"] = 0
        cmd = {}
        cmd["type"] = "STOP_SIMULATION"
        msg["cmds"] = [cmd]

        socket.send_json(msg)
    except:
        propagateLog({"msg":"Error occured while trying to stop the simulation.", "level":"error"})

#def launch_simulation(xyz:str, ffContent:str, ffFileName:str, motors:str):
def launch_simulation(configTask:dict):
    try:
        #app.logger.info("Received a request to launch the simulation. Generating the inputs...")
        propagateLog({"msg": "Starting the simulation. Generating inputs...", "level": "info"})
        #jobFolder = generate_inputs(xyz, ffContent, ffFileName, motors)
        #jobFolder = generate_inputs(config['xyz'], config['ff'], config['ffName'], config['motors'])
        jobFolder = generate_inputs(configTask)
        

        #app.logger.info("Input generated. Preparing the tasks...")
        propagateLog({"msg": "Input generated. Starting the tasks.", "level": "info"})
        taskManager = JobRunner("radahn", jobFolder)

        # Prepare the Vitamins input

        # Name convention used by generate_inputs
        dataFile = "input.data"
        lammpsScriptFile = "input.lammps"
        motorsFile = "motors.json"
        lmpConfigFile = "lmp_config.json"

        cmdRadan = f"python3 {radahnScript} --workdir {jobFolder} --nvesteps {configTask['max_timestep']} --ncores {configTask['number_cores']} --frequpdate {configTask['update_frequency']} --lmpdata {dataFile} --lmpinput {lammpsScriptFile} --potential {configTask['ffName']}"
        if len(configTask['motors']) > 0:
            cmdRadan += f" --motorconfig {motorsFile}"
        if configTask['force_timestep']:
            cmdRadan += " --forcemaxsteps"
        if len(configTask['lmp_config']) > 0:
            cmdRadan += f" --lmpconfig {lmpConfigFile}"
        taskManager.addTask("prepRadahn", cmdRadan)

        # Launche simulation 
        cmdLaunch = "./launch.LammpsSteered.sh"
        taskManager.addTask("launchRadahn", cmdLaunch)

        app.logger.info("Tasks prepared. Processing the tasks...")
        # Processing the tasks
        completed = False
        while not completed:
            completed = taskManager.processTasks()
            time.sleep(1)

        app.logger.info("Simulation completed.")
        propagateLog({"msg": "End of tasks execution. Simulation completed.", "level": "info"})
    except:
        propagateLog({"msg": "Something went when launching a simulation.", "level":"error"})
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
            threadTable[configTask["threadName"]]["event"].clear()
    finally:
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None

            # Tell the listener threads to stop
            threadTable["listenKVS"]["event"].clear()
            threadTable["listenAtoms"]["event"].clear()




def listen_to_zmq_socket(configTask:dict):
    propagateLog({"msg": "Start Listening for KVS messages.", "level": "info"})
    try:
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect("tcp://localhost:50000")
        socket.setsockopt(zmq.SUBSCRIBE, b"")
        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)
        app.logger.info("Listening for ZMQ messages on tcp://localhost:50000")

        # This function may be called without a thread, so we need to check if there is a thread context
        checkEvent = "threadName" in configTask
        if checkEvent:
            threadTable[configTask["threadName"]]["event"].set()
        previousIt = -1
        while (checkEvent and threadTable[configTask["threadName"]]["event"].is_set()) or (not checkEvent):
            socks = dict(poller.poll(500)) # ms
            if socket in socks and socks[socket] == zmq.POLLIN:
                message = socket.recv()
                messageStr = message.decode('utf-8')
                msgDict = json.loads(messageStr)
                if "global" not in msgDict.keys():
                    continue
                if "simIt" not in msgDict["global"].keys():
                    continue 
                socketio.emit('zmq_message', {'message': messageStr})
    except:
        propagateLog({"msg": "Something went wrong during listening to the ZMQ socket for scalar data.", "level":"error"})
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
            threadTable[configTask["threadName"]]["event"].clear()
    finally:
        propagateLog({"msg": "End Listening for KVS messages.", "level": "info"})
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
            threadTable[configTask["threadName"]]["event"].clear()

def listen_to_zmq_socketAtoms(configTask:dict):
    propagateLog({"msg": "Start Listening for Atom messages.", "level": "info"})
    try:
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect("tcp://localhost:50001")
        socket.setsockopt(zmq.SUBSCRIBE, b"")
        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)
        checkEvent = "threadName" in configTask
        app.logger.info("Listening for ZMQ messages on tcp://localhost:50001")
        if checkEvent:
            threadTable[configTask["threadName"]]["event"].set()
        while (checkEvent and threadTable[configTask["threadName"]]["event"].is_set()) or (not checkEvent):
            # we use an poller to be able to check the event periodically, allowing external action to stop this infinite loop
            socks = dict(poller.poll(500)) # ms
            if socket in socks and socks[socket] == zmq.POLLIN:
                message = socket.recv()
                socketio.emit('zmq_message_atoms', {'message': message.decode('utf-8')})
    finally:
        propagateLog({"msg": "End Listening for Atom messages.", "level": "info"})
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
            threadTable[configTask["threadName"]]["event"].clear()

def open_job_folder(configTask:dict):
    try:
        if execEnvironment == "NATIVE":
            jobFolder = configTask["jobFolder"]
            if platform.system() == 'Windows':
                os.startfile(jobFolder)
            elif platform.system() == 'Darwin':
                subprocess.call(['open', jobFolder])
            elif platform.system() == 'Linux':
                app.logger.info("Linux platform detected. Opening the folder with xdg-open.")
                subprocess.call(['xdg-open', jobFolder])
            else:
                app.logger.info(f"Unsupported platform when requesting to open a folder: {platform.system()}")
        elif execEnvironment == "DOCKER":
            propagateLog({"msg": "Opening the job folder in Docker is not supported.", "level": "warn"})
        else:
            propagateLog({"msg": "Unsupported execution environment when requesting to open a folder: " + execEnvironment, "level": "warn"})
    except:
        propagateLog({"msg": "Something unexpected happen while trying to open the job folder.", "level":"error"})
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
    finally:
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
        


@socketio.on('send_sim_stop_command')
def handle_send_sim_stop_command():
    stop_simulation()

@socketio.on('open_job_folder')
def handle_open_job_folder(data):
    app.logger.info("Received a request to open the job folder.")

    global threadTable

    with threadTable["openJobFolder"]["lock"]:

        if threadTable["openJobFolder"]["thread"] is None:
            configTask = deepcopy(data)
            configTask["threadName"] = "openJobFolder"
            threadTable["openJobFolder"]["thread"] = socketio.start_background_task(open_job_folder, configTask)
            app.logger.info("Background thread opening job folder started.")
        else:
            app.logger.info("Background task opening job folder already started.")

# Receive the test request from client and send back a test response
@socketio.on('start_listening')
def handle_start_listening():
    app.logger.info("Received a request to start listening to the simulation.")

    global threadTable

    with threadTable["listenKVS"]["lock"]:
        if threadTable["listenKVS"]["thread"] is None:
            configTask = {"threadName": "listenKVS" }
            threadTable["listenKVS"]["thread"] = socketio.start_background_task(listen_to_zmq_socket, configTask)
            app.logger.info("Background thread listening for KVS started.")
        else:
            app.logger.info("Background task listening to KVS already started.")

    with threadTable["listenAtoms"]["lock"]:
        if threadTable["listenAtoms"]["thread"] is None:
            configTask = {"threadName": "listenAtoms" }
            threadTable["listenAtoms"]["thread"] = socketio.start_background_task(listen_to_zmq_socketAtoms, configTask)
            app.logger.info("Background thread listening for atoms started.")
        else:
            app.logger.info("Background task listening to atoms already started.")


@socketio.on('generate_inputs')
def handle_generate_inputs(data):
    app.logger.info("Received a request to generate inputs.")
    
    global threadTable

    with threadTable["generateInputs"]["lock"]:
        #if threadGenerateInputs is None:
        if threadTable["generateInputs"]["thread"] is None:
            configTask = deepcopy(data)
            configTask["threadName"] = "generateInputs"
            threadTable["generateInputs"]["thread"] = socketio.start_background_task(generate_inputs, configTask)
            app.logger.info("Background thread generating inputs started.")
        else:
            app.logger.info("Background task generating inputs already started.")

@socketio.on('launch_simulation')
def handle_launch_simulation(data):
    app.logger.info("Received a request to launch the simulation.")
    global threadTable

    with threadTable["runSimulation"]["lock"]:   
        if threadTable["runSimulation"]["thread"] is None:
            configTask = deepcopy(data)
            configTask["threadName"] = "runSimulation"
            threadTable["runSimulation"]["thread"] = socketio.start_background_task(launch_simulation, configTask)
            app.logger.info("Background thread launching simulation started.")
        else:
            app.logger.info("Background task launching simulation already started.")

    handle_start_listening()

@socketio.on('save_project')
def handle_save_project(data):
    propagateLog({"msg": "Saving project on the server side.", "level": "info"})
    if "projectName" not in data.keys():
        propagateLog({"msg": "Unable to find the project name in the data package. Abording save."})
        return
    
    if len(data["projectName"]) == 0:
        propagateLog({"msg": "Empty project name received. Abording save."})
        return 
    
    filePath = projectJobFolder / (data["projectName"] + ".json")
    with open(filePath, "w") as f:
        json.dump(data, f, indent=4)
        f.close()
    propagateLog({"msg": f"Project saved as {str(filePath)}", "level" : "info"})


if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
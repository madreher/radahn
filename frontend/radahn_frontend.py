from flask import Flask, render_template 
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
from copy import copy, deepcopy

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
radahnFolder = Path(os.getenv("HOME") + "/dev/radahn/build/install")
radahnScript = radahnFolder / "workflow" / "lammpsSteered.py"
execEnvironment = "NATIVE"  # Dev note: Will probably be needed when return path of the job folder 
                            # Will probably need some difference when the app is running on native Linux, in Docker Linux, on WSL


# Override default paths if requested 
if "RADAHN_FRONTEND_ENV_PATH" in os.environ:
    try:
        from dotenv import load_dotenv
        load_dotenv(os.environ["RADAHN_FRONTENT_ENV_PATH"])

        rootJobFolder = Path(os.getenv("RADAHN_ROOT_JOB_FOLDER"))
        radahnFolder = Path(os.getenv("RADAHN_INSTALL_FOLDER"))
        radahnScript = radahnFolder / "workflow" / "lammpsSteered.py"
        execEnvironment = os.getenv("RADAHN_EXEC_ENVIRONMENT")
        app.logger.info(f"Loaded environment variables from {os.environ['RADAHN_FRONTENT_ENV_PATH']}")
    except Exception as e:
        app.logger.warn(f"Unable to load environment variables from {os.environ['RADAHN_FRONTENT_ENV_PATH']}: {e}")


@app.route('/')
def index():
    return render_template('index.html')

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

    if atoms.get_cell().max() == 0:
        if os.path.isfile('cell.txt'):
            cell = np.loadtxt('cell.txt')
        else:
            cell = [500, 500, 500]
        atoms.set_cell([cell[0], cell[1], cell[2]])
    if atoms.get_positions().min() < 0:
        atoms.center()
    write(filename=dataPath, images=atoms, format='lammps-data', atom_style='full')

    chemical_symbols = atoms.get_chemical_symbols()
    masses = atoms.get_masses()
    indexes = np.unique(chemical_symbols, return_index=True)[1]
    masses_u = [masses[index] for index in indexes]
    chemical_symbols_u = [chemical_symbols[index] for index in indexes]

    with open(dataPath, 'r') as f:
        lines = f.readlines()

    with open(dataPath, 'w') as f:
        f.write('# Generated by Radahn\n')
        for i in range(2,7):
            f.write(lines[i])
        f.write('\n')
        f.write('Masses\n')
        f.write('\n')
        for i in range(len(masses_u)):
            f.write(f'{i + 1} {masses_u[i]} # {chemical_symbols_u[i]}\n')
        f.write('\n')
        f.write('Atoms # full\n')
        f.write('\n')
        for i in range(11, len(lines)):
            f.write(lines[i])
    
    return atoms

#def generate_inputs(xyz:str, ffContent:str, ffFileName:str, motors:str) -> str:
def generate_inputs(configTask:dict) -> str:

    # Create the job folder 
    jobID = uuid.uuid4()
    jobFolder = rootJobFolder / str(jobID)
    os.makedirs(jobFolder)

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

        if len(configTask['lmp_groups']) > 0:
            lmp_groupsFile = jobFolder / "lmp_groups.json"
            with open(lmp_groupsFile, "w") as f:
                f.write(configTask["lmp_groups"])
                f.close()


        dataFile = jobFolder / "input.data"

        # Generate the base Lammps script
        lammpsScriptFile = jobFolder / "input.lammps"
        useAcks2 = False
        with open(lammpsScriptFile, "w") as f:
            # Convert the XYZ to a .data file 
            atoms = xyzToLammpsDataPBC(xyzFile, dataFile)

            # Extract the simulation box
            cell = atoms.get_cell()
            minCellDim = min([cell[0][0], cell[1][1], cell[2][2]])
            #print(f"Minimum cell dimension: {minCellDim}")

            # Get back the list of elements
            elements = getElementsFromData(dataFile)

            scriptContent = "# -*- mode: lammps -*-\n"
            scriptContent += 'units          real\n'
            scriptContent += 'atom_style     full\n'
            scriptContent += 'atom_modify    map hash\n'
            scriptContent += 'newton         on\n'
            scriptContent += 'boundary       p p p\n'

            scriptContent += 'read_data      input.data\n'
            #for i in range(len(indexes)):
            #    scriptContent += f'mass           {i + 1} {masses_u[i]}\n' 
            scriptContent += 'pair_style     reaxff NULL mincap 1000\n'
            scriptContent += f'pair_coeff     * * {configTask["ffName"]}{elements}\n'
            if useAcks2:
                scriptContent += 'fix            ReaxFFSpec all acks2/reaxff 1 0.0 10.0 1e-8 reaxff\n'
            else:
                scriptContent += 'fix            ReaxFFSpec all qeq/reaxff 1 0.0 10.0 1e-8 reaxff\n'
            #scriptContent += 'neighbor       2.5 bin\n' 
            # 2.5 is too large for small molecule like benzene. Trying to compute a reasonable cell skin based on the simulation box
            scriptContent += f"neighbor       {min([2.5, minCellDim/2])} bin\n"
            scriptContent += 'neigh_modify   every 1 delay 0 check yes\n'


            # Add basic IO setup
            scriptContent += """####

    thermo         50
    thermo_style   custom step etotal pe ke temp press pxx pyy pzz lx ly lz
    thermo_modify  flush yes lost warn

    dump           dump all custom 100 fulltrajectory.dump id type x y z q
    dump_modify    dump sort id
    dump           xyz all xyz 100 fulltrajectory.xyz
    dump_modify    xyz sort id element C H
    fix            fixbond all reaxff/bonds 100 bonds_reax.txt

    ####

    timestep       0.5"""
            scriptContent += "\n\n"


            f.write(scriptContent)

        # Generate the data file
        dataFile = jobFolder / "input.data"
        xyzToLammpsDataPBC(xyzFile, dataFile)
        socketio.emit('job_folder', {'message': jobFolder.absolute().as_posix()})
        app.logger.info(f"Job folder generated: {jobFolder.absolute().as_posix()}")
    finally:
        if "threadName" in configTask:
                threadTable[configTask["threadName"]]["thread"] = None


    return jobFolder


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
        lmpGroupFile = "lmp_groups.json"

        cmdRadan = f"python3 {radahnScript} --workdir {jobFolder} --nvesteps {configTask['max_timestep']} --ncores {configTask['number_cores']} --frequpdate {configTask['update_frequency']} --lmpdata {dataFile} --lmpinput {lammpsScriptFile} --potential {configTask['ffName']}"
        if len(configTask['motors']) > 0:
            cmdRadan += f" --motorconfig {motorsFile}"
        if configTask['force_timestep']:
            cmdRadan += " --forcemaxsteps"
        if len(configTask['lmp_groups']) > 0:
            cmdRadan += f" --lmpgroups {lmpGroupFile}"
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
        while (checkEvent and threadTable[configTask["threadName"]]["event"].is_set()) or (not checkEvent):
            socks = dict(poller.poll(500)) # ms
            if socket in socks and socks[socket] == zmq.POLLIN:
                message = socket.recv()
                socketio.emit('zmq_message', {'message': message.decode('utf-8')})
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
    finally:
        if "threadName" in configTask:
            threadTable[configTask["threadName"]]["thread"] = None
        


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




if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
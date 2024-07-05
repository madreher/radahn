# Radahn

Radahn is interactive molecular dynamic application allowing users to steer their MD simulations and getting live feedback. The framework is build around LAMMPS and Gromacs as possible simulation backend. 

# Feature set

- Real time visualization and data extraction from a molecular dynamic simulation
- Set of motors to add user interactions in a molecular simulation
- Lammps as simulation backend currently, Gromacs will be added later one.
- Frontend in a web browser to load molecular model, configure a set of motors to apply, and visualize the simulation in real time
- Works on Linux, Windows via WSL

# Related project:

- [LAMMPS](https://docs.lammps.org/Manual.html)
- [Godrick](https://github.com/madreher/Godrick)
- [Conduit](https://llnl-conduit.readthedocs.io/en/latest/index.html)

# Installation

## Docker method

### Install Docker and prepare folders on Ubuntu and WSL2 (first time install only)

This first set of instruction should be executed only the fist time this software gets installed. On Windows, these instructions should be executed within WSL.
```
# Get the sources 
cd /path/to/source
git clone https://github.com/madreher/radahn.git .

# Install docker
sudo docker/ubuntu/install_docker_ubuntu.sh

# Add the current user to the docker group
sudo usermod -aG docker $USER

# **IMPORTANT: RESTART!!**

# Setup Radahn folders and variables
mkdir $HOME/.radahn
mkdir $HOME/.radahn/jobs

# Copy the environment file to the radahn folder
cp docker/ubuntu/env_docker $HOME/.radahn

# Edit the first line of the file env_docker where you copied it to match your system. Ex:
RADAHN_ROOT_JOB_FOLDER=/home/matthieu/.radahn/jobs
```

### Create/Update Docker image

The following instructions should be executed every time the user wants to update its version of Radahn. For Windows, these instructions should be executed within WSL.

**IMPORTANT**: Radahn is currently in very early stages and doesn't provide a proper versioning yet. Versions will starts once a minimal set of features becomes available. However, users can choose to add version number or commit tag to their image name in the meantime if desired. Once versioning officially start, these instructions will be updated to properly reflect the image version.
```
cd /path to source
git pull 
cd docker/ubuntu
# Lammps full build
docker build --no-cache -t radahn-ubuntu:current .
# Lammps min build
docker build --no-cache -f ./Dockerfile_minlmp -t radahn-minlmp-ubuntu:current .
```

Once the docker images, Radahn is ready to be used! 
First start by launching the server:
```
# Run in user space with job folder mounted
# Lammps full build
docker container run --rm -it -p 8080:5000 -v $HOME/.radahn:$HOME/.radahn --workdir /app --user $(id -u):$(id -g) -e RADAHN_FRONTEND_ENV_PATH=$HOME/.radahn/env_docker radahn-ubuntu:current
#Lammps min build
docker container run --rm -it -p 8080:5000 -v $HOME/.radahn:$HOME/.radahn --workdir /app --user $(id -u):$(id -g) -e RADAHN_FRONTEND_ENV_PATH=$HOME/.radahn/env_docker radahn-minlmp-ubuntu:current

# In the container, you can start now start the webserver in two ways:

# Option 1: automatic
./launch_frontend.sh

# Option 2: manual
cd /app/radahn/radahn/build/install/frontend
source /app/radahn/radahn/build/install/docker/ubuntu/activate.sh
flask --app radahn_frontend run --host=0.0.0.0
```

Once the server is launched, open a browser, and go to the page http://localhost:8080/

These instructions have been tested on native Ubuntu 22.04, Windows 11 via WSL2 (Ubuntu 22.04.3)

## Manual compilation 

### Ubuntu 22

TODO

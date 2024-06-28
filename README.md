# Radahn

Radahn is interactive molecular dynamic application allowing users to steer their MD simulations and getting live feedback. The framework is build around LAMMPS and Gromacs as possible simulation backend. 

# Deps

TODO

# Build Docker image

Instruction to perform once
```
# Get the sources 
cd /path/to/source
git clone https://github.com/madreher/radahn.git .

# Install docker
sudo docker/ubuntu/install_docker_ubuntu.sh

# Add the current user to the docker group
sudo usermod -aG docker $USER

# IMPORTANT: RESTART!!

# Setup Radahn folders and variables
mkdir $HOME/.radahn
mkdir $HOME/.radahn/jobs

# Copy the environment file to the radahn folder
cp docker/ubuntu/env_docker $HOME/.radahn

# Edit the first line of the file env_docker where you copied it to match your system. Ex:
RADAHN_ROOT_JOB_FOLDER=/home/matthieu/.radahn/jobs
```

Instructions to create or update the docker images
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

# In the container, start the server with

# Single line
./launch_frontend.sh

# OR manual
cd /app/radahn/radahn/build/install/frontend
source /app/radahn/radahn/build/install/docker/ubuntu/activate.sh
flask --app radahn_frontend run --host=0.0.0.0
```

Once the server is launch, open a browser, and go to the page http://localhost:8080/

These instructions have been tested on native Ubuntu 22.04, Windows 11 via WSL2 (Ubuntu 22.04.3)

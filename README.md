# Radahn

Radahn is interactive molecular dynamic application allowing users to steer their MD simulations and getting live feedback. The framework is build around LAMMPS and Gromacs as possible simulation backend. 

# Deps

TODO

# Build Docker image

Instruction to perform once
```
# Install docker
sudo docker/ubuntu/install_docker_ubuntu.sh

# Add the current user to the docker group
sudo usermod -aG docker $USER

# IMPORTANT: RESTART!!

# Setup Radahn folders and variables
mkdir $HOME/.radahn
mkdir $HOME/.radahn/jobs

# Get the sources 
cd /path/to/source
git clone https://github.com/madreher/radahn.git .

# Copy the environment file to the radahn folder
cp docker/ubuntu/env_docker $HOME/.radahn

# Edit the first line of the file env_docker where you copied it to match your system. Ex:
RADAHN_ROOT_JOB_FOLDER=/home/matthieu/.radahn
```

Instructions to create or update the docker images
```
cd /path to source
git pull 
cd docker/ubuntu
# Lammps full build
docker build -t radahn-ubuntu:current .
# Lammps min build
docker build --no-cache -f ./Dockerfile_minlmp -t radahn-minlmp-ubuntu:current .


# Run in user space with job folder mounted
docker container run --rm -it -v $HOME/.radahn:$HOME/.radahn --workdir /app --user $(id -u):$(id -g) radahn-ubuntu:current

# In the container, start the server
cd /app/radahn/radahn/build/install/frontend
source /app/radahn/radahn/build/install/docker/ubuntu/activate.sh
export RADAHN_FRONTEND_ENV_PATH=/home/matthieu/.radahn/env_docker;flask --app radahn_frontend run --host=0.0.0.0

# In a browser, open the page http://localhost:8080/
```
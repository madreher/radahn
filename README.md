# Radahn

Radahn is interactive molecular dynamic application allowing users to steer their MD simulations and getting live feedback. The framework is build around LAMMPS and Gromacs as possible simulation backend. 

# Deps

TODO

# Build Docker image

```
# Install docker
sudo docker/ubuntu/install_docker_ubuntu.sh

# Add the current user to the docker group
sudo usermod -aG docker $USER


# IMPORTANT: RESTART!!

cd docker/ubuntu
# Lammps full build
docker build -t radahn-ubuntu:current .
# Lammps min build
docker build --no-cache -f ./Dockerfile_minlmp -t radahn-minlmp-ubuntu:current .


# Run in user space with job folder mounted
docker container run --rm -it -v $HOME/.radahn:$HOME/.radahn --workdir /app --user $(id -u):$(id -g) radahn-ubuntu:current
```
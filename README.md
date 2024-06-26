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
sudo docker build -t radahn-ubuntu:current .

# Run the image
docker run -p 5000:5000 -t -i radahn-ubuntu:current

# Run in user space with job folder mounted
docker container run --rm -it -v $HOME/.radahn:$HOME/.radahn --workdir /app --user $(id -u):$(id -g) conduit-ubuntu:current
```
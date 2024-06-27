FROM ubuntu:22.04

# fetch build env
RUN apt-get update && apt-get install -y \
    git \
    curl \
    lsb-release \
    build-essential \
    gcc \
    g++ \
    gfortran \
    zlib1g-dev \
    python3 \
    libpython3.10-dev \
    tar \
    unzip \
    perl \
    libncurses-dev \
    libssl-dev \
    libopenblas-dev \
    zlib1g-dev \
    libgdbm-dev \
    libreadline-dev \
    libsqlite3-dev \
    libbz2-dev \
    openmpi-bin \
    libopenmpi-dev \
    vim \
    cmake \
    python3-pip \
    libhdf5-dev \
    wget \
    xdg-utils


RUN pip3 install -q --no-cache-dir conan conan-package-tools


WORKDIR /root

RUN mkdir -p /root/.conan

# Create a folder for each project 
RUN cd /root && mkdir radahn && cd radahn && mkdir opt conduit godrick lammps radahn

# CONUIT BUILD
RUN cd /root/radahn/conduit && \
    git clone --recursive https://github.com/llnl/conduit.git . && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/root/radahn/opt ../src/ -DCMAKE_BUILD_TYPE=Release -DENABLE_MPI=ON && \
    make -j2 && \
    make install


# GODRICK BUILD
RUN cd /root/radahn/godrick && \
    git clone --recursive https://github.com/madreher/Godrick.git . && \
    export CONDUIT_PREFIX=/root/radahn/opt && \
    mkdir build && cd build && \
    ../run_cmake_script.sh && \
    make -j2 && \
    make install

# LAMMPS BUILD
RUN cd /root/radahn/lammps && \
    wget https://download.lammps.org/tars/lammps-17Apr2024.tar.gz && \
    tar -xf lammps-17Apr2024.tar.gz && \
    cd lammps-17Apr2024/ && \
    mkdir build && cd build && \
    cmake ../cmake -DCMAKE_BUILD_TYPE=RELEASE \
                -DBUILD_MPI=TRUE -DBUILD_OMP=TRUE \
                -DBUILD_SHARED_LIBS=TRUE \
                -DPKG_OPENMP=TRUE \
                -DCMAKE_INSTALL_PREFIX=/root/radahn/opt \
                -DCMAKE_INSTALL_RPATH=/root/radahn/opt/lammps-17Apr2024/lib \
                -DPKG_MOLECULE=TRUE \
                -DPKG_RIGID=TRUE \
                -DPKG_KSPACE=TRUE \
                -DPKG_EXTRA-FIX=TRUE \
                -DPKG_EXTRA-PAIR=TRUE \
                -DPKG_GRANULAR=TRUE \
                -DPKG_INTEL=TRUE \
                -DPKG_KOKKOS=TRUE \
                -DKokkos_ENABLE_OPENMP=yes \
                -DPKG_REAXFF=TRUE \
                -DPKG_H5MD=TRUE \
                -DPKG_ASPHERE=TRUE \
                -DPKG_ATC=TRUE \
                -DPKG_BOCS=TRUE \
                -DPKG_BODY=TRUE \
                -DPKG_BPM=TRUE \
                -DPKG_BROWNIAN=TRUE \
                -DPKG_CG-DNA=TRUE \
                -DPKG_CG-SDK=TRUE \
                -DPKG_CLASS2=TRUE \
                -DPKG_COLLOID=TRUE \
                -DPKG_COLVARS=TRUE \
                -DPKG_COMPRESS=TRUE \
                -DPKG_CORESHELL=TRUE \
                -DPKG_DIELECTRIC=TRUE \
                -DPKG_DIFFRACTION=TRUE \
                -DPKG_DIPOLE=TRUE \
                -DPKG_DPD-BASIC=TRUE \
                -DPKG_DPD-MESO=TRUE \
                -DPKG_DPD-REACT=TRUE \
                -DPKG_DPD-SMOOTH=TRUE \
                -DPKG_DRUDE=TRUE \
                -DPKG_ELECTRODE=TRUE \
                -DPKG_EFF=TRUE \
                -DPKG_EXTRA-COMPUTE=TRUE \
                -DPKG_EXTRA-DUMP=TRUE \
                -DPKG_EXTRA-MOLECULE=TRUE \
                -DPKG_FEP=TRUE \
                -DPKG_INTERLAYER=TRUE \
                -DPKG_LATBOLTZ=TRUE \
                -DPKG_MANIFOLD=TRUE \
                -DPKG_MANYBODY=TRUE \
                -DPKG_MC=TRUE \
                -DPKG_MEAM=TRUE \
                -DPKG_MGPT=TRUE \
                -DPKG_MISC=TRUE \
                -DPKG_ML-HDNNP=TRUE \
                -DPKG_ML-IAP=TRUE \
                -DPKG_ML-RANN=TRUE \
                -DPKG_ML-SNAP=TRUE \
                -DPKG_MOFFF=TRUE \
                -DPKG_MOLFILE=TRUE \
                -DPKG_MPIIO=TRUE \
                -DPKG_MSCG=TRUE \
                -DPKG_OPT=TRUE \
                -DPKG_ORIENT=TRUE \
                -DPKG_PERI=TRUE \
                -DPKG_PHONON=TRUE \
                -DPKG_PLUGIN=TRUE \
                -DPKG_POEMS=TRUE \
                -DPKG_PTM=TRUE \
                -DPKG_PYTHON=TRUE \
                -DPKG_QEQ=TRUE \
                -DPKG_QTB=TRUE \
                -DPKG_REACTION=TRUE \
                -DPKG_REPLICA=TRUE \
                -DPKG_SHOCK=TRUE \
                -DPKG_SMTBQ=TRUE \
                -DPKG_SPH=TRUE \
                -DPKG_SPIN=TRUE \
                -DPKG_SRD=TRUE \
                -DPKG_TALLY=TRUE \
                -DPKG_UEF=TRUE \
                -DPKG_YAFF=TRUE \
                -DPKG_LEPTON=TRUE \
                -DLAMMPS_EXCEPTIONS=TRUE \
                -DPKG_VORONOI=TRUE && \
    make -j4 && \
    make install 

# RADAHN BUILD
RUN cd /root/radahn/radahn && \
    git clone --recursive https://github.com/madreher/radahn.git . && \
    export CONDUIT_PREFIX=/root/radahn/radahn/build/install && \
    export GODRICK_PREFIX=/root/radahn/godrick/build/install && \
    export LAMMPS_PREFIX=/root/radahn/opt && \
    mkdir build && cd build && \
    ../run_cmake_script.sh && \
    make -j2 && \
    make install

# Python packages
RUN cd /root/radahn/radahn && pip3 install -r requirements.txt
RUN cd /root/radahn/godrick && \
    pip3 install -r requirements.txt && \
    pip3 install -e .

EXPOSE 5000

# Create the launch script
RUN cd /app && \
    touch launch_frontend.sh && \
    echo "cd /app/radahn/radahn/build/install/frontend" >> launch_frontend.sh && \
    echo "source /app/radahn/radahn/build/install/docker/ubuntu/activate.sh" >> launch_frontend.sh && \
    echo "flask --app radahn_frontend run --host=0.0.0.0" >> launch_frontend.sh && \
    chmod +x launch_frontend.sh

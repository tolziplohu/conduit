# Pull base image.
FROM ubuntu:18.04

COPY . /opt/conduit

SHELL ["/bin/bash", "-c"]

# Install.
RUN \
  apt-get update -qq \
    && \
  echo "initialized packaging system"

RUN \
  apt-get install -qq \
    curl \
    git \
    gzip \
    unzip \
    tar \
    wget \
    && \
  echo "installed fundamentals"

RUN \
  apt-get install -qq software-properties-common \
    && \
  add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && \
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && \
  apt-add-repository "deb https://apt.llvm.org/xenial/ llvm-toolchain-xenial-7 main" \
    && \
  apt-get update -qq \
    && \
  apt-get upgrade -qq \
    && \
  echo "configured packaging system"

RUN \
  apt-get install -qq \
    g++-8 \
    libclang-7-dev \
    llvm-7 \
    clang-7 \
    libstdc++-7-dev \
    cmake \
    build-essential \
    python-virtualenv \
    python3-virtualenv \
    python-pip \
    python3-pip \
    libpthread-stubs0-dev \
    libc6-dbg \
    gdb \
    && \
  echo "installed core dependencies"

RUN \
  apt-get install -qq \
    libopenmpi-dev \
    libopenmpi2 \
    openmpi-bin \
    openmpi-common \
    openmpi-doc \
    libmpich-dev \
    libmpich12 \
    hdf5-helpers \
    hdf5-tools \
    libhdf5-100 \
    libhdf5-cpp-100 \
    libhdf5-dev \
    libhdf5-doc \
    libhdf5-mpi-dev \
    libhdf5-mpich-100 \
    libhdf5-mpich-dev \
    libhdf5-openmpi-100 \
    libhdf5-openmpi-dev \
    libhdf5-serial-dev \
    python-h5py \
    python3-h5py \
    slurm-client \
    slurm-client-dbg \
    && \
  echo "installed hpc dependencies"

RUN \
  apt-get install -qq \
    nodejs \
    python-pip \
    npm \
    gconf-service \
    libasound2 \
    libatk1.0-0 \
    libc6 \
    libcairo2 \
    libcups2 \
    libdbus-1-3 \
    libexpat1 \
    libfontconfig1 \
    libgcc1 \
    libgconf-2-4 \
    libgdk-pixbuf2.0-0 \
    libglib2.0-0 \
    libgtk-3-0 \
    libnspr4 \
    libpango-1.0-0 \
    libpangocairo-1.0-0 \
    libstdc++6 \
    libx11-6 \
    libx11-xcb1 \
    libxcb1 \
    libxcomposite1 \
    libxcursor1 \
    libxdamage1 \
    libxext6 \
    libxfixes3 \
    libxi6 \
    libxrandr2 \
    libxrender1 \
    libxss1 \
    libxtst6 \
    ca-certificates \
    fonts-liberation \
    libappindicator1 \
    libnss3 \
    lsb-release \
    xdg-utils \
    && \
  echo "installed web dependencies"

# magic from https://github.com/puppeteer/puppeteer/issues/3451#issuecomment-523961368
RUN echo 'kernel.unprivileged_userns_clone=1' > /etc/sysctl.d/userns.conf

RUN \
  apt-get install -qq \
    man \
    vim \
    nano \
    htop \
    && \
  echo "installed creature comforts"

RUN \
  pip3 install \
    osfclient  \
    keyname  \
    pandas  \
    && \
  echo "installed Python packages"

RUN \
  update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 90 \
    && \
  update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-7 90 \
    && \
  update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-7 90 \
    && \
  update-alternatives --install /usr/bin/llvm-profdata llvm-profdata /usr/bin/llvm-profdata-7 90 \
    && \
  update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-7 90 \
    && \
  npm install -g n \
    && \
  n 12.18.2 \
    && \
  export python="/usr/bin/python" \
    && \
  npm install source-map \
    && \
  echo "finalized dependency versions"

RUN \
  cd /opt/conduit/ \
    && \
  cd third-party \
    && \
  ./install_dependencies.sh \
    && \
  echo "installed third party dependencies"

RUN \
  cd /opt/conduit \
    && \
  git remote set-url origin https://github.com/mmore500/conduit.git \
    && \
  echo "switched to https origin remote url"

RUN \
  cd /opt/conduit/ \
    && \
  make \
    && \
  cd /opt/conduit/tests/utility \
    && \
  make \
    && \
  echo "representative compile and tests passed!"

# Define default working directory.
WORKDIR /opt/conduit

# Perform any further action as an unprivileged user.
# adapted from https://stackoverflow.com/a/27703359
# and https://superuser.com/a/235398
RUN \
  useradd --create-home --shell /bin/bash user \
    && \
  groupadd group \
    && \
  gpasswd -a user group \
    && \
  chgrp --recursive user /opt \
    && \
  chmod --recursive g+rwx /opt \
    && \
  echo "user added and granted permissions to /opt"

USER user

CMD ["bash"]

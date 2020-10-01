FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y \
    wget \
    netcat \
    iputils-ping \
    build-essential \
    git \
    ca-certificates \
    python3.8 \
    python3-pip

RUN pip3 install conan==1.29.2

RUN wget --no-check-certificate \
    https://github.com/Kitware/CMake/releases/download/v3.17.4/cmake-3.17.4-Linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.17.4-Linux-x86_64.tar.gz

ENV PATH="/cmake-3.17.4-Linux-x86_64/bin:${PATH}"

RUN mkdir -p lab1/build

COPY install-dependencies.sh conanfile.txt lab1/
RUN cd lab1 && ./install-dependencies.sh

COPY hostfile CMakeLists.txt lab1/
COPY src lab1/src
WORKDIR lab1/build
RUN cmake .. && make

ENV GLOG_logtostderr=1
ENTRYPOINT ["/lab1/build/lab1"]
FROM debian:buster

LABEL maintainer="jgaa@jgaa.com"

RUN DEBIAN_FRONTEND="noninteractive" apt-get -q update &&\
    DEBIAN_FRONTEND="noninteractive" apt-get -y -q --no-install-recommends upgrade &&\
    DEBIAN_FRONTEND="noninteractive" apt-get install -y -q \
        gcc g++ make perl tar libgmp10 libmpc3 libisl19 libzstd-dev git flex curl bzip2 gzip binutils

RUN mkdir /src && \
    cd src && \
    git clone https://github.com/gcc-mirror/gcc.git -b releases/gcc-11 --depth 1 && \
    cd gcc && \
    contrib/download_prerequisites

RUN cd /src/gcc && ls -al

RUN cd /src/gcc && ./configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=/usr/local/gcc-11 --enable-checking=release --enable-languages=c,c++,fortran --disable-multilib --program-suffix=-11 && \
    make -j `nproc` && \
    make install-strip && \
    cd / && \
    rm -rf /src/gcc && \
    echo 'export PATH=/usr/local/gcc-11/bin:$PATH' >> ~/.bashrc && \
    echo 'export LD_LIBRARY_PATH=/usr/local/gcc-11/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc && \
    echo 'export CC=/usr/local/gcc-11/bin/gcc-11' >> ~/.bashrc && \
    echo 'export CXX=/usr/local/gcc-11/bin/g++-11' >> ~/.bashrc && \
    . ~/.bashrc && \
    gcc-11 --version
    
RUN apt-get remove -y -q gcc g++
RUN ln -s /usr/local/gcc-11/bin/g++-11 /usr/bin/g++
RUN ln -s /usr/local/gcc-11/bin/gcc-11 /usr/bin/gcc

# Build boost
RUN curl -L -o /tmp/boost.tar.gz https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz
RUN cd /src && tar -xf /tmp/boost.tar.gz
RUN . ~/.bashrc && cd /src/boost_1_76_0 && ./bootstrap.sh && ./b2

# Build our app
RUN echo
RUN apt-get install -y -q cmake
RUN  cd /src && git clone https://github.com/jgaa/asio-async-model-perftest.git
RUN . ~/.bashrc && mkdir /src/build && cd /src/build && cmake -DBOOST_ROOT=/src/boost_1_76_0  ../asio-async-model-perftest && make -j `nproc`

ENV LD_LIBRARY_PATH=/src/boost_1_76_0/stage/lib:/usr/local/gcc-11/lib64/ 

RUN cp /src/build/bin/aamp /usr/local/bin

# Run our app with default arguments
CMD aamp

# Copyright (c) 2012-2017 Ben Croston ben@croston.org.
# Copyright (c) 2019, NVIDIA CORPORATION.
# Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

FROM nvcr.io/nvidia/l4t-base:r32.4.3 AS base

RUN apt-get update 
RUN apt-get install git build-essential cmake -y

# build 
FROM base AS builder

WORKDIR /source
RUN git clone https://github.com/pjueon/JetsonGPIO.git

WORKDIR /source/JetsonGPIO
RUN mkdir build

WORKDIR /source/JetsonGPIO/build
RUN cmake ../ -DBUILD_EXAMPLES=ON -DJETSON_GPIO_POST_INSTALL=OFF
RUN make install

# copy built results to ./install
COPY docker/copy_built_results.py .
RUN python3 ./copy_built_results.py

# app
FROM base AS app 
WORKDIR /app

# install built results
COPY --from=builder /source/JetsonGPIO/build/install/ /usr/local/

# for debug
COPY --from=builder /source/JetsonGPIO/build/samples/ /samples
COPY --from=builder /source/JetsonGPIO/build/tests/ /tests

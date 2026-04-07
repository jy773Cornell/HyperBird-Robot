# Blackwell-ready base
FROM pytorch/pytorch:2.7.0-cuda12.8-cudnn9-devel

ARG USE_CUDA=1
ENV AM_I_DOCKER=1 \
    BUILD_WITH_CUDA="${USE_CUDA}" \
    # include Blackwell
    TORCH_CUDA_ARCH_LIST="8.0;8.6;8.9;9.0;12.0" \
    TZ=America/New_York

# deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    git wget curl ca-certificates ffmpeg libsm6 libxext6 build-essential ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace

# python deps
RUN python -m pip install --upgrade pip "setuptools>=64" wheel \
    && python -m pip install numpy opencv-python transformers supervision pycocotools addict yapf timm

# Setup
RUN python -m pip install -e .

CMD ["/bin/bash"]

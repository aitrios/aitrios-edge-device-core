# SPDX-FileCopyrightText: 2023-2024 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

FROM mcr.microsoft.com/devcontainers/base:bookworm

ARG NODE_MAJOR=20
ARG LLVM_VERSION=18

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture arm64

RUN apt-get update && apt-get install -y --no-install-recommends \
	ninja-build \
	meson \
	cmake \
	gcc-aarch64-linux-gnu \
	g++-aarch64-linux-gnu \
	qemu-user-static \
	python3-kconfiglib \
	iputils-ping \
	pkg-config:arm64 \
	libbsd-dev:arm64 \
	libjpeg62-turbo-dev:arm64 \
	libsqlite3-dev:arm64 \
	libnm-dev:arm64 \
	libxml2-dev:arm64

# todo: Figure out why we can't just symlink to qemu-aarch64-static and run
# meson env2mfile
COPY arm64-cross /usr/share/meson/arm64-cross

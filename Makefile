# Arsenal Launcher — local development (Prism Launcher / CMake presets + vcpkg)
#
# Prerequisites: CMake 3.28+, Ninja, git, C++23 compiler, Qt 6.8+ on CMAKE_PREFIX_PATH,
# and initialized submodules (see `make submodules`). Full dependency list:
# https://prismlauncher.org/wiki/development/build-instructions/
#
# Quick start (macOS with Homebrew Qt):
#   brew install cmake ninja qt@6 extra-cmake-modules openjdk@17
#   make submodules
#   export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
#   make build run

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

BUILD_DIR ?= build
BUILD_TYPE ?= Debug
JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

APP_BINARY := arsenal
APP_DISPLAY_NAME := Arsenal

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Preset + vcpkg triplet (matches CI where possible; native macOS builds use arch-specific triplets).
ifeq ($(UNAME_S),Darwin)
  CMAKE_PRESET ?= macos
  ifeq ($(UNAME_M),arm64)
    VCPKG_TRIPLET ?= arm64-osx
  else
    VCPKG_TRIPLET ?= x64-osx
  endif
  ifeq ($(UNIVERSAL),1)
    CMAKE_PRESET := macos_universal
    VCPKG_TRIPLET := universal-osx
  endif
else ifeq ($(UNAME_S),Linux)
  CMAKE_PRESET ?= linux
  ifeq ($(UNAME_M),aarch64)
    VCPKG_TRIPLET ?= arm64-linux
  else
    VCPKG_TRIPLET ?= x64-linux
  endif
else
  CMAKE_PRESET ?= windows_msvc
  VCPKG_TRIPLET ?= x64-windows
endif

ARTIFACT_NAME ?= Local-Qt6
BUILD_PLATFORM ?= unofficial

# Optional: pass Microsoft OAuth client ID at configure time (empty in tree by default).
MSA_CLIENT_ID ?=

# JDK for building launcher Java helpers (legacy jars need java.applet — use 17).
ifeq ($(origin JAVA_HOME), undefined)
  BREW_JDK17 := $(shell brew --prefix openjdk@17 2>/dev/null)/libexec/openjdk.jdk/Contents/Home
  ifneq ($(wildcard $(BREW_JDK17)/bin/javac),)
    JAVA_HOME := $(BREW_JDK17)
  else ifeq ($(UNAME_S),Darwin)
    JAVA_HOME := $(shell /usr/libexec/java_home -v 17 2>/dev/null || true)
  endif
endif
export JAVA_HOME

# Launcher data paths (for `make cclear`).
HOME_DIR := $(HOME)
ifeq ($(UNAME_S),Darwin)
  LAUNCHER_DATA_DIR ?= $(HOME_DIR)/Library/Application Support/Arsenal
else ifeq ($(UNAME_S),Linux)
  LAUNCHER_DATA_DIR ?= $(HOME_DIR)/.local/share/Arsenal
else
  LAUNCHER_DATA_DIR ?= $(HOME_DIR)/AppData/Roaming/Arsenal
endif
INSTANCES_DIR ?= $(LAUNCHER_DATA_DIR)/instances

# Ninja Multi-Config output layout: build/$(BUILD_TYPE)/...
BINARY_DIR := $(BUILD_DIR)/$(BUILD_TYPE)
NATIVE_BINARY := $(BINARY_DIR)/$(APP_BINARY)

ifeq ($(UNAME_S),Darwin)
  APP_BUNDLE := $(BINARY_DIR)/$(APP_DISPLAY_NAME).app
  RUN_CMD := open "$(APP_BUNDLE)"
else
  RUN_CMD := "$(NATIVE_BINARY)"
endif

# Qt 6.8+ (not from vcpkg). Auto-detect Homebrew when unset.
ifeq ($(origin CMAKE_PREFIX_PATH), undefined)
  BREW_QT_PREFIX := $(shell brew --prefix qt 2>/dev/null)
  ifneq ($(wildcard $(BREW_QT_PREFIX)/lib/cmake/Qt6/Qt6Config.cmake),)
    CMAKE_PREFIX_PATH := $(BREW_QT_PREFIX)
  else
    BREW_QT6_PREFIX := $(shell brew --prefix qt@6 2>/dev/null)
    ifneq ($(wildcard $(BREW_QT6_PREFIX)/lib/cmake/Qt6/Qt6Config.cmake),)
      CMAKE_PREFIX_PATH := $(BREW_QT6_PREFIX)
    endif
  endif
endif

CMAKE_CONFIG_ARGS := -D Launcher_APP_BINARY_NAME=$(APP_BINARY)
ifneq ($(MSA_CLIENT_ID),)
  CMAKE_CONFIG_ARGS += -D Launcher_MSA_CLIENT_ID=$(MSA_CLIENT_ID)
endif
ifneq ($(CMAKE_PREFIX_PATH),)
  CMAKE_CONFIG_ARGS += -D CMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)"
endif

.PHONY: all help submodules submodules-repair configure reconfigure build run test clean distclean cclear

all: build

help:
	@echo "Arsenal Launcher build targets"
	@echo ""
	@echo "  make submodules     Init git submodules (vcpkg, libnbtplusplus, …)"
	@echo "  make submodules-repair  Fix broken cmake/vcpkg git metadata, then re-init"
	@echo "  make configure      cmake --preset $(CMAKE_PRESET)"
	@echo "  make build          Configure (if needed) and compile ($(BUILD_TYPE))"
	@echo "  make run            Build, then launch the app"
	@echo "  make test           ctest ($(BUILD_TYPE))"
	@echo "  make reconfigure    Remove CMake cache and configure again"
	@echo "  make clean          Remove $(BUILD_DIR)/"
	@echo "  make distclean      clean + remove vcpkg_installed/"
	@echo "  make cclear         Delete all instances under $(INSTANCES_DIR)/"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE=$(BUILD_TYPE)   BUILD_DIR=$(BUILD_DIR)   JOBS=$(JOBS)"
	@echo "  CMAKE_PRESET=$(CMAKE_PRESET)   VCPKG_TRIPLET=$(VCPKG_TRIPLET)"
	@echo "  UNIVERSAL=1         macOS only: universal binary preset"
	@echo "  MSA_CLIENT_ID=…     Microsoft login client ID for CMake"
	@echo "  CMAKE_PREFIX_PATH=…  Qt 6 install prefix (e.g. $$(brew --prefix qt@6))"
	@echo "  JAVA_HOME=$(JAVA_HOME)"

submodules:
	git submodule update --init --recursive

# Partial clone / interrupted init can leave cmake/vcpkg with a gitdir but no real repo under .git/modules.
submodules-repair:
	git submodule deinit -f cmake/vcpkg || true
	rm -rf cmake/vcpkg .git/modules/cmake/vcpkg
	git submodule update --init --recursive

configure: submodules
	@command -v cmake >/dev/null || { echo "error: cmake not found"; exit 1; }
	@command -v ninja >/dev/null || { echo "error: ninja not found"; exit 1; }
	@if echo "$(CMAKE_PREFIX_PATH)" | grep -qi 'path/to'; then \
		echo "error: CMAKE_PREFIX_PATH is still a placeholder ($(CMAKE_PREFIX_PATH))"; \
		echo "  fish: set -e CMAKE_PREFIX_PATH"; \
		echo "  then: export CMAKE_PREFIX_PATH=\"$$(brew --prefix qt)\""; \
		exit 1; \
	fi
	@if [ -z "$(CMAKE_PREFIX_PATH)" ]; then \
		echo "error: Qt 6.8+ not found. Install Qt (brew install qt; brew unlink qt@5 if needed)"; \
		echo "  then: export CMAKE_PREFIX_PATH=\"$$(brew --prefix qt)\""; \
		exit 1; \
	fi
	@if [ -z "$${JAVA_HOME:-}" ]; then \
		echo "warning: JAVA_HOME not set; install JDK 17 (e.g. brew install openjdk@17)"; \
	fi
	ARTIFACT_NAME="$(ARTIFACT_NAME)" BUILD_PLATFORM="$(BUILD_PLATFORM)" \
	cmake --preset "$(CMAKE_PRESET)" \
		-D "VCPKG_HOST_TRIPLET=$(VCPKG_TRIPLET)" \
		-D "VCPKG_TARGET_TRIPLET=$(VCPKG_TRIPLET)" \
		$(CMAKE_CONFIG_ARGS)

reconfigure:
	rm -rf "$(BUILD_DIR)"
	$(MAKE) configure

build: configure
	cmake --build --preset "$(CMAKE_PRESET)" --config "$(BUILD_TYPE)" -j "$(JOBS)"

test: build
	ctest --preset "$(CMAKE_PRESET)" --build-config "$(BUILD_TYPE)"

# Multi-config generators put jars at $(BUILD_DIR)/jars (CMAKE_JAVA_TARGET_OUTPUT_DIR),
# while the .app lives under $(BINARY_DIR)/. Copy into the bundle so getJarPath() finds them.
JARS_SRC_DIR := $(BUILD_DIR)/jars

run: build
ifeq ($(UNAME_S),Darwin)
	@test -d "$(APP_BUNDLE)" || { echo "error: $(APP_BUNDLE) not found"; exit 1; }
	@test -d "$(JARS_SRC_DIR)" || { echo "error: $(JARS_SRC_DIR) not found (Java helper jars missing)"; exit 1; }
	@mkdir -p "$(APP_BUNDLE)/Contents/MacOS/jars"
	@cp -f "$(JARS_SRC_DIR)/"*.jar "$(APP_BUNDLE)/Contents/MacOS/jars/"
	codesign -fs - "$(APP_BUNDLE)" 2>/dev/null || true
	$(RUN_CMD)
else
	@test -x "$(NATIVE_BINARY)" || { echo "error: $(NATIVE_BINARY) not found"; exit 1; }
	@if [ -d "$(JARS_SRC_DIR)" ]; then \
		mkdir -p "$(BINARY_DIR)/jars"; \
		cp -f "$(JARS_SRC_DIR)/"*.jar "$(BINARY_DIR)/jars/"; \
	fi
	$(RUN_CMD)
endif

clean:
	rm -rf "$(BUILD_DIR)"

distclean: clean
	rm -rf vcpkg_installed

cclear:
	@if [ -d "$(INSTANCES_DIR)" ]; then \
		echo "Removing $(INSTANCES_DIR)/"; \
		rm -rf "$(INSTANCES_DIR)"; \
	else \
		echo "No instances folder at $(INSTANCES_DIR) (nothing to do)"; \
	fi

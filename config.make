################################################################################
# CONFIGURE PROJECT MAKEFILE (optional)
#   This file is where we make project specific configurations.
################################################################################

################################################################################
# OF ROOT
#   The location of your root openFrameworks installation
#       (default) OF_ROOT = ../../.. 
################################################################################
# OF_ROOT = ../../..

################################################################################
# PROJECT ROOT
#   The location of the project - a starting place for searching for files
#       (default) PROJECT_ROOT = . (this directory)
################################################################################
# PROJECT_ROOT = .

################################################################################
# PROJECT SPECIFIC CHECKS
#   Detect the current CPU architecture so we can conditionally include or
#   exclude platform-specific code and addons.
#
#   Raspberry Pi 4/5 report: aarch64
#   Raspberry Pi 1 (unsupported) reported: armv6l
#   Desktop Linux/macOS/Windows: x86_64 or arm64
################################################################################
PROJECT_ARCH = $(shell uname -m)

################################################################################
# PROJECT EXTERNAL SOURCE PATHS
################################################################################
# PROJECT_EXTERNAL_SOURCE_PATHS =

################################################################################
# PROJECT EXCLUSIONS
#   Exclude the Raspberry Pi camera addon on all platforms except Pi 1 (armv6l).
#   Pi 4/5 (aarch64) and desktop builds all exclude it — the legacy MMAL/OMX
#   camera stack it depends on was removed in Raspberry Pi OS Bullseye (2021).
#   Use a USB webcam with a numeric device ID instead (e.g. ./bin/creepyportrait 0).
################################################################################
ifneq ($(PROJECT_ARCH),armv6l)
	PROJECT_EXCLUSIONS = $(PROJECT_ROOT)/addons/ofxRPiCameraVideoGrabber%
endif

################################################################################
# PROJECT LINKER FLAGS
################################################################################
# PROJECT_LDFLAGS=-Wl,-rpath=./libs

################################################################################
# PROJECT DEFINES
#   Load all three models on all supported platforms.
#   Pi 4/5 (aarch64, 2GB+ RAM) can hold all three models in memory comfortably.
#   Note: The original armv6l (Pi 1) memory restriction no longer applies —
#   Pi 1 is not a supported platform for this project.
################################################################################
PROJECT_DEFINES = LOAD_SKULL LOAD_JACK_EVIL LOAD_JACK_HAPPY

################################################################################
# PROJECT CFLAGS
#   C++17 is used throughout. OF 0.12+ requires at minimum C++14.
################################################################################
PROJECT_CFLAGS = -std=c++17

################################################################################
# PROJECT OPTIMIZATION CFLAGS
################################################################################
# PROJECT_OPTIMIZATION_CFLAGS_RELEASE =
# PROJECT_OPTIMIZATION_CFLAGS_DEBUG =

################################################################################
# PROJECT COMPILERS
#   No custom compiler needed. Pi 4/5 (aarch64) ships with GCC 10+ which
#   fully supports C++17. The old GCC 4.7 override was only needed for
#   Pi 1 (armv6l / Raspbian Wheezy) which is not supported by this project.
################################################################################
# PROJECT_CXX =
# PROJECT_CC =

# Project name
PROJECT = agent_invade

# OctoPOS release version
RELEASE = current

# Architecture
# Possible values: x86guest leon
ARCH = x86guest

# Variant: choose architecture-specific variant
# Available variants can be found in the release directory of the current
# architecture.
VARIANT = multitile

# If required, set additional CFLAGS, CXXFLAGS, LDFLAGS here
#CFLAGS += -Wno-trigraphs # Supress warnings related to picture data

BASE_DIR := ../../releases/${RELEASE}
include ${BASE_DIR}/extras/mk/octopos-app.mk

multi: 
	make APP_INI="app.ini ../drr-demo/app.ini" multiapp

# Project name
PROJECT = agent_ex

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

run: $(PROJECT)
	qemu-system-x86_64 -m 1G -serial stdio -smp 128 -numa node -numa node -numa node  -numa node -no-reboot -display none -kernel $(PROJECT) -cpu Westmere -enable-kvm
	@rm -rf $(PROJECT) *.o *.a *.dep *.elf64

<p align="justify">
  
# OctoPOS_AgentSystem3

Repository to upload the monitoring interface for the OctoPOS Agent System. This project corresponds to the Computer Engineering carrer TFG. The project is supervised by Ing. M.Sc. Jeferson González Gómez from Karlsruhe Institute of Technology (KIT). 

The project main objective is to design a multi-agent monitoring system for OctoPOS, which improves the resources control and allocation.

The following guide will explain how to compile the OctoPOS system in order to add the Agent System support. Also, it will explain how to run OctoPOS applications.

### Dependencies
In order to install all the needed dependencies, run the installation/dep_install.sh script. 

Moreover, you need to install the aspect++ compiler in your computer. To achieve this, run the following command:
```
mkdir aspectc++ && cd aspectc++
curl https://www4.cs.fau.de/~rabenstein/invasic/aspectc++.tar.xz | tar -x --strip-components=1
```
After that, run the following command:
```
sudo cp aspect++/usr/bin/* /usr/bin
```

## Compile the OS
### 1 - Clone the repository
First, you need to have access to the invasic IRTSS repo. When you have the access, clone the repository with the following command:
```
git clone https://gitlab.cs.fau.de/invasic/irtss.git
```

### 2 - Add the submodules
Before compile the system, you need to add a submodule which is needed for the compilation process. Download the installation/c-capnproto.tar.gz file. Uncompress it and copy its files to the irtss/submodules/c-capnproto route.

Also, you need to generate all the support platform for the system. To do this, run the following command
```
platform/generateVariants
```

### 3 - Enable the Agent System
#### Agent System 1.0
This is the stable version. It allows you to use the Agent System, but it uses a centralized approach. To enable it, go to the irtss/app/release.x64native.multitile route. Then, run the following command:
```
make menuconfig
```

A simple interface will show up, go to the 'Load an Alternate Configuration File' and write the following name in the textfield
```
release.x64native.multitile.config 
```
The previous name corresponds to the config file used by the compile to enable certain features. 

After that, go to the OS configuration/Agent System and press the space bar in the 'Enable Agent System Support' option. Another option called 'Agent Implementation'  will appear, press the space bar in that option. Finally, press the enter button in the 'Agents RPC-client Implementation' and select the 'Heterogeneous implementation' options by pressing the enter button. To end this process, press the exit option, which is located in the bottom, as many times as you need,  until you reach a prompt asking you for save the file. Press the 'yes' option to save the configuration file.

#### Agent System 2.0
This is the Agent System second version, it uses a distributive approach. Currently, this version gives errors in the compilation process.

To enable this, follow the previous steps until you reach the 'Os configuration' main menu. Go to the 'Resource Allocation and Accounting' and press the space bar in the options 'Use one SystemClaim per tile for system services' and 'Use SHARQ as system interface / for the sys-ilet queue of the SystemClaim', exit this menu. Go to the 'Agent System' menu and press the space bar in the 'Enable Agent System 2.0 Support' then, enable the options 'Agent 2.0 Implementation' and 'Libkit headers' by pressing the space bar. Finally, exit the menu and save the file, as you did in the Agent System 1.0 configuration.

You need to replace some files to enable this system. The following list shows the files you need to replace and its corresponding routes in the irtss folder.
* /AgentSystem2.0/src/cface from this repository goes to the irtss/src/os/krn/cface
* /AgentSystem2.0/src/agent2 from this repository goes to the irtss/src/os/agent2
* /AgentSystem2.0/kconf from this repository goes to the irtss/kconf

#### Agent System 3.0
This is the Agent System third version, at this moment, it uses the Agent System 1.0 as a base and implements a monitoring interface over it.

To enable this, first you need to replace some files. The following list shows the files you need to replace and its corresponding routes in the irtss folder.
* /AgentSystem3.0/src/octo_agent3.h and os/src/octo_agent3.cc from this repository goes to the irtss/src/os/krn/cface route.
* /AgentSystem3.0/src/agent3/* from this repository  goes to the irtss/src/os route.
* /AgentSystem3.0/kconf/octoPOS.fm from this repository goes to the irtss irtss/kconf/common/features route. You need to replace the old file with the new one.
* /AgentSystem3.0/kconf/octoPOS.cmp.pl from this repository goes to the irtss irtss/kconf/common/family. You need to replace the old file with the new one.

Finally, to enable this Agent System, you need to follow the same steps described in the Agent System 1.0 section. You need to select the 'Enable Agent System 3.0 Support', 'Agent 3.0 Implementation' and 'Heterogeneous RPC-client implementation' options.

### 4 - Compile the System
To compile the Agent System 1.0, go to the repository root directory and run the following command:
```
tools/bin/build4platform.pl -t platform/release.x64native.generic-debug.pm
```
To compile the Agent System 2.0, go to the repository root directory and run the following command:
```
tools/bin/build4platform.pl -t platform/release.x86guest.generic.pm
```
The previous commands will compile the system with the selected Agent System.

## Run OctoPOS applications
### 1 - Create the needed routes
Create a folder call octopos-apps wherever you want. Inside that folder create another one called releases. Copy this repository apps folder inside your octopos-app folder.

### 2 - Copy the OS 
#### Agent System 1.0
Go to the irtss/build/release.x64native.generic-debug/release.x64native.multitile/release inside the cloned repository. Copy both folders inside your octopos-apps/releases route.

#### Agent System 2.0
Go to the irtss/build/release.x86guest.generic/release.x86guest.multitile/release inside the cloned repository. Copy both folders inside your octopos-apps/releases route.

### 3 - Compile and run
#### Agent System 1.0
Go to the octopos-apps/apps/drr-demos route. To compile the application, run the following command:
```
make ARCH=x64native VARIANT=generic-debug
```

The previous command will compile your code using the corresponding Makefile.

To run the compiled code, you need to install QEMU. To do this, run the following command:
```
sudo apt install qemu-system-x86
```

Finally, to run your application, run the following command:
```
qemu-system-x86_64 -serial stdio -smp 32 -numa node -numa node -m 1G -no-reboot -display none -cpu Westmere -kernel hello
```

The amount of ```-numa node``` options determines how many tiles your system will have.

#### Agent System 2.0
Go to the octopos-apps/apps/drr-demos route. To compile, run the following command:
```
make ARCH=x86guest VARIANT=generic
```

To run your program, you need to run it in a 32 bit system. You can use a virtual machine or a Docker container. Run it as a normal C program, for example:
```
./compiledProgram
```
</p>

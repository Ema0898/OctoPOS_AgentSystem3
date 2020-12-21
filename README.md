<p align="justify">
  
# OctoPOS_AgentSystem3

Repository to upload the monitoring interface for the OctoPOS Agent System. This project correspond to the Computer Engineering carrer TFG. The project is supervised by Ing. M.Sc. Jeferson González Gómez from Karlsruhe Institute of Technology (KIT). 

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
### 1 - Clone the repo
First, you need to have access to the invasic IRTSS repo. When you have the access, clone the repository with the following command:
```
git clone https://gitlab.cs.fau.de/invasic/irtss.git
```

### 2 - Add the submodules
Before compile the system, you need to add a submodule which is needed for the compilation process. Download the installation/c-capnproto.tar.gz file. Uncompress it and copy its files to the irtss/submodules/c-capnproto route.

### 3 - Enable the Agent System
#### Agent System 1.0
This is the stable version. It allows you to use the Agent System but, it uses a centralized approach. To enable it, go to the irtss/app/release.x64native.multitile route. Then run the following command:
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

To enable this, follow the previous steps until you reach the 'Os configuration' main menu. Go to the 'Resource Allocation and Accounting' and press the space bar in the 'Use one SystemClaim per tile for system services' option, exit this menu. Go to the 'Agent System' menu and press the space bar in the 'Enable Agent System 2.0 Support', enable all the new option  by pressing the space bar. Finally exit the menu and save the file, as you did in the Agent System 1.0 configuration.

### 4 - Compile the System
Go to the repository root directory and run the following command:
```
tools/bin/build4platform.pl -t platform/release.x64native.generic-debug.pm
```
The previous command will compile the system with the selected Agent System.

## Run OctoPOS applications
### 1 - Create the needed routes
Create a folder call octopos-apps wherever you want. Inside that folder create another one called releases. Copy this repository apps folder inside your octopos-app folder.

### 2 - Copy the OS
Go to the irtss/build/release.x64native.generic-debug/release.x64native.multitile/release inside the cloned repository. Copy both folders inside your octopos-apps/releases route.

### 3 - Compile and run
Go to the octopos-apps/apps/drr-demos route. To compile the application, run the following command:
```
make ARCH=x64native VARIANT=generic-debug
```
The previous command will compile your code using the corresponding Makefile.

To run the code, you need to install QEMU. To do this, run the following command:
```
sudo apt install qemu-system-x86
```

Finally, to run your application, run the following command:
```
qemu-system-x86_64 -serial stdio -smp 32 -numa node -numa node -m 1G -no-reboot -display none -cpu Westmere -kernel hello
```

The amount of ```-numa node``` options determines how many tiles your system will have.
</p>

CEUSBKWrapper - Userspace USB Driver for Windows CE
===================================================

Contents
========

1. Introduction
2. Supported Features and Platforms
3. Build and Installation
4. Driver Configuration
5. Userspace Applications
6. Cross-Platform Support


1. Introduction
===============

CEUSBKWrapper allows userspace applications on Windows CE to interact with USB
devices. CEUSBKWrapper consists of two components:

* ceusbkwrapperdrv.dll: A Windows CE USB driver, exposing USB functionality to
  userspace.
* ceusbkwrapper.dll: A userspace library, which interacts with the driver and 
  exposes an API to applications. 

The driver component of CEUSBKWrapper is loaded for particular USB devices by 
USBD. These devices are then accessible from userspace applications. USB devices
must be associated with the driver component (via LoadClients registry entries 
for USBD) before userspace applications can interact with them.

CEUSBKWrapper is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free 
Software Foundation; either version 2.1 of the License, or (at your option) 
any later version.
 
CEUSBKWrapper is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more 
details.

Please see LICENSE.txt for further licensing information.


2. Supported Features and Platforms
===================================

The following features are currently supported by CEUSBKWrapper:

* Notification of device connection and disconnection events.
* Retrieval of device and configuration descriptors.
* Control transfers, issued to the Default Control Pipe.
* Bulk transfers.
* Endpoint management (testing for and clearing of halt conditions).

Currently, CEUSBKWrapper requires Windows CE 6.0 or later. This is routinely
tested on the Freescale i.MX51 EVK board, using Freescale's April 2011 Windows
CE 6.0 Board Support Package. Prototype support has also been added for Windows
CE 5.0, but this is not routinely tested.


3. Build and Installation
=========================

These instructions are written for Windows CE 6.0. To build and deploy
CEUSBKWrapper, you must be capable of generating Windows CE OEM images for your
device. USB Host Support (SYSGEN_USB) must be enabled to allow CEUSBKWrapper
to function. 

Note that the driver component of CEUSBKWrapper is designed to be loaded by 
USBD. If USBD is not being used in your image, then the driver component may
require modification.

CEUSBKWrapper is provided as three Platform Builder subprojects, to be 
incorporated into an OS design:

* ceusbkwrapperdrv (drv\ceusbkwrapperdrv.pbpxml): The driver component.

* ceusbkwrapper (lib\ceusbkwrapper.pbpxml): The userspace library component.

* ceusbkwrappertest (test\ceusbkwrappertest.pbpxml): The optional CEUSBKWrapper 
  test utility.

At a minimum, both the ceusbkwrapperdrv and ceusbkwrapper subprojects must be 
incorporated into the OS design. The ceusbkwrappertest project is optional, and
is intended for testing and debugging purposes.

To incorporate each project, open the OS design and, in Solution Explorer, find
the Subprojects node. Right click this, and select "Add Existing Subproject...".
In the Open dialog that appears, navigate to and open the .pbpxml file for the 
subproject. This should then appear under the Subprojects node in Solution 
Explorer.

Once all required subprojects have been added, right-click Solution Explorer 
and select "Set Subproject Build Order". Ensure that the projects are built in 
the following order:

1. ceusbkwrapperdrv
2. ceusbkwrapper
3. ceusbkwrappertest (if using)

Finally, build the subprojects using the "Rebuild All Subprojects" option from 
the Build menu. Depending on your Visual Studio Platform Builder configuration, 
this should also cause a new run-time image to be generated. If it does not,  
use the "Make Run-Time Image" option from the Build menu to regenerate the 
run-time image.


4. Driver Configuration
=======================

The driver component of CEUSBKWrapper has a number of configuration registry 
settings, specified in drv\ceusbkwrapperdrv.reg. These control the USB devices 
to which CEUSBKWrapper shall attach, and USB interfaces for which CEUSBKWrapper 
shall automatically attempt to load alternative drivers. Consult this file for
detailed information on these configuration settings, and to view the default
configuration used.


5. Userspace Applications
=========================

Applications wishing to interact with CEUSBKWrapper should use the API defined
in lib\ceusbkwrapper.h. Please consult this file for API documentation. 
Applications will need to link against, or dynamically load, ceusbkwrapper.dll. 
Applications should not interact directly with the driver component.


6. Cross-Platform Support
=========================

Applications can also interact with CEUSBKWrappper through libusbx for Windows
CE. libusbx provides a single cross-platform API allowing access to USB devices. 
Please refer to the following web page for more information on libusbx:

http://libusbx.org/


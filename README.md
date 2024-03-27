# StarMod

Headstart for building ESP32 applications: printing, file management, persistent data, Wifi, Web, UI and system management works out of the box.
StarMod will integrate with major IOT/network devices and applications.

Everything is a module.

System modules:

* Print: Print to different targets (Serial, file, net)
* Files: File Manager
* Model: Datamodel in json, stored to file, used in ui and network comms
* Network: Wifi 
* Web: Web server
* UI: UI Server
* System: Show and manage ESP32 system

User Modules

* E131/DMX support
* Home Assistant (planned)
* LEDs
* ...

Build apps on top of this

* Led apps
* IO control apps
* IOT apps 
* Any app

By [MoonModules](https://github.com/MoonModules)
LED module inspired by [WLED MM](https://github.com/MoonModules/WLED)

Disclaimer:

Using this software is the users responsibility as it is not bug free. Therefore contributors of this repo can not be held reliable for anything including but not limited to spontaneous combustion of the entire led strip, the house and the inevitable heat death of the universe

GPL V3 License:

You may copy, distribute and modify the software as long as you track changes/dates in source files. Any modifications to or software including (via compiler) GPL-licensed code must also be made available under the GPL along with build & install instructions ([tldrlegal](https://www.tldrlegal.com/license/gnu-general-public-license-v3-gpl-3))
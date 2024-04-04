# StarMod

Update April, 2024: The LEDs part of https://github.com/ewowi/StarMod has been moved to https://github.com/MoonModules/StarModLeds. From now on:

* StarMod (Core) is a generic ESP32 platfom without any notion of LEDs
* StarMod LEDS is a fork of StarMod (Core) which adds LED functionality on top of it.
* StarMod (Core) can be forked by anyone who wants to build an ESP32 application
* Forks of StarMod (Core) should not change System functionality, changes on that should be done on StarMod (Core):
    * Sys Modules 
    * Generic User Modules
    * index.js / html / css
    * platformio.ini
* The following should be changed on forks:
    * App Modules
    * main.cpp
    * app.js 
* Use Github issues for [StarMod (Core)/issues](https://github.com/ewowi/StarMod/issues) or [StarMod Leds/issues](https://github.com/MoonModules/StarModLeds/issues) respectively. Of working with StarMod Leds, also core related matters can be reportinged in [StarMod Leds/issues](https://github.com/MoonModules/StarModLeds/issues).


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
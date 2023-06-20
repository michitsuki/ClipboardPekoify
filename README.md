# ClipboardPekoify
pekoifies all text copied to clipboard and sits resident in system tray

# Features
* Pekoifies all text copied to clipboard
* Supports Japanese
* Can be disabled from system tray
* Can be told to not pekoify http and https links

# Building
Should be ready to build out of the box with any modern version of Visual Studio. I used Enterprise 2022 but those with Community should also work.

Use ``SUBSYSTEM:WINDOWS`` with ``WINMAIN`` entry for console-less application and ``SUBSYSTEM:CONSOLE`` with ``MAIN`` entry for console-enabled application.
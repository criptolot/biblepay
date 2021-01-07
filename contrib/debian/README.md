
Debian
====================
This directory contains files used to package estaterod/estatero-qt
for Debian-based Linux systems. If you compile estaterod/estatero-qt yourself, there are some useful files here.

## estatero: URI support ##


estatero-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install estatero-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your estatero-qt binary to `/usr/bin`
and the `../../share/pixmaps/estatero128.png` to `/usr/share/pixmaps`

estatero-qt.protocol (KDE)


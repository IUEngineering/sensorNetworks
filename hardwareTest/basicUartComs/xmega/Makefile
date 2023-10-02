# makefile.xmega.mk
# ^^^^^^^^^^^^^^^^^
# Auteur : Edwin Boer 
# Versie : 20200320
#
# Bestand: Makefile
# Inhoud : de configuratie van makefile.xmega.mk

# Folder voor de broncode
SRCFOLDER = src/
# Folder voor gegenereerde make-bestanden 
MAKEFOLDER = make-data/
# Folder waar bin/ van de AVR toolchain staat (leeghouden indien door $Path o.a. avr-g++ vindbaar is)
AVRFOLDER =
# Folder waar avrdude staat (leeghouden indien door $Path o.a. avrdude vindbaar is)
DUDEFOLDER = 

# Project bestandsnaam
PROJECTNAME = replyrgb
# De AVR microcontroller
MICROCONTROLLER = atxmega256a3u
# De AVR programmer (o.a. avrisp2)
PROGRAMMER = avrisp2
# De (optionele) poort voor de AVR programmer (o.a. usb, /dev/tty.#)
#PORT = usb
# De fuse waardes: low, high, en extended
# Zie http://www.engbedded.com/fusecalc/ voor andere microcontrollers en opties
FUSELOW  = 
FUSEHIGH = 
FUSEEXT  = 
# Broncode (zonder externe bibliotheken)
SOURCE = replyrgb.cpp serialF0.cpp
# Externe bibliotheek folders van .c(pp) en .h(pp) bestanden
EXTFOLDER = 
# Compiler flags
CPPFLAGS = -Wall

# Voer de configuratie uit
include makefile.xmega.mk

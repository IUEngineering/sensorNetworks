# makefile.xmega.mk
# ^^^^^^^^^^^^^^^^^
# Auteur : Edwin Boer 
# Versie : 20200320
#
# Bestand: makefile.xmega.mk
# Inhoud : de configuratie staat in bestand Makefile en koppelt met dit bestand met 'include' 

# Genereer lijst van include-folders
INCLUDELIST := $(foreach dir, $(EXTFOLDER), -I$(dir))
# Genereer lijst van broncode bestanden relatief tot de broncode-folder
CPPLIST     := $(foreach file, $(SOURCE), $(SRCFOLDER)$(file))
# Genereer lijst van bibliotheek .c(pp) bestanden
EXTLIST     := $(foreach dir, $(EXTFOLDER), $(wildcard $(SRCFOLDER)$(dir)/*.c $(SRCFOLDER)$(dir)/*.cpp))
# Genereer lijst van o-bestanden
OBJLIST     := $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(CPPLIST))) $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(EXTLIST))) 
# Genereer lijst van o-bestanden in de make(sub)folders
OBJLIST2    := $(foreach file, $(OBJLIST), $(MAKEFOLDER)$(file))

# Definieer alle tools met hun basisinstellingen
TOOLCPP   = $(AVRFOLDER)avr-g++ $(CPPFLAGS) -Os -mmcu=$(MICROCONTROLLER)
TOOLCOPY  = $(AVRFOLDER)avr-objcopy
TOOLSIZE  = $(AVRFOLDER)avr-size --format=avr --mcu=$(MICROCONTROLLER)
TOOLDUMP  = $(AVRFOLDER)avr-objdump
ifdef PORT
 TOOLDUDE = $(DUDEFOLDER)avrdude -c $(PROGRAMMER) -p $(MICROCONTROLLER) -P $(PORT)
else
 TOOLDUDE = $(DUDEFOLDER)avrdude -c $(PROGRAMMER) -p $(MICROCONTROLLER)
endif

# Openbaar te kiezen doelen

# Doel: make
# Doel: make help
help: _start _help1 _eind

_start:
	@echo
	@echo "#################################"
	@echo "  > Start: makefile.xmega.mk"

_help1: 
	@echo "_________________________________"
	@echo "  > Mogelijke opties:"
	@echo "    make        := Toon dit overzicht."
	@echo "    make help   := Toon dit overzicht."
	@echo "    make all    := Compileer en link de broncode."
	@echo "    make flash  := Compileer en link de broncode en schrijf het gecompileerde hex-bestand naar"
	@echo "                   het flash geheugen van de microcontroller."
	@echo "    make test   := Test de programmer verbinding (ISP) met de microcontroller."
	@echo "    make fuse   := Schrijf de fuse bytes naar de microcontroller."
	@echo "    make disasm := Disassembleeer de code voor debugging."
	@echo "    make clean  := Verwijder alle gegenereerde .hex, .elf, en .o bestanden."
	
_eind:
	@echo
	@echo "#################################"
	@echo "  > Klaar :-)"
	@echo

# Doel: make all
all: _start _all1 $(MAKEFOLDER)$(PROJECTNAME).hex _eind

_all1: 
	@echo "  > Doel: Compileer en link de broncode."
	@echo "  > Makefolder: $(MAKEFOLDER)"
	@mkdir -p $(MAKEFOLDER)

# Doel: make flash
flash: _start _flash1 $(MAKEFOLDER)$(PROJECTNAME).hex _flash2 _eind

_flash1: 
	@echo "  > Doel: Compileer, link de broncode en schrijf het gecompileerde hex-bestand naar het flash geheugen van de microcontroller."
	@echo "  > Makefolder: $(MAKEFOLDER)"
	@mkdir -p $(MAKEFOLDER)

_flash2: 
	@echo "_________________________________"
	@echo " > Start flash"
	$(TOOLDUDE) -e -U flash:w:$(MAKEFOLDER)$(PROJECTNAME).hex

# Doel: make test
test: _start _test1 _eind

_test1:
	@echo "  > Doel: Test de programmer verbinding (ISP) met de microcontroller."
	$(TOOLDUDE) -v

# Doel: make fuse
fuse: _start _fuse1 _eind

_fuse1:
	@echo "  > Doel: Schrijf de fuse bytes naar de microcontroller."
	$(TOOLDUDE) -U lfuse:w:$(FUSELOW):m -U hfuse:w:$(FUSEHIGH):m -U efuse:w:$(FUSEEXT):m

# Doel: make clean
disasm: _start _disasm1 $(MAKEFOLDER)/$(PROJECTNAME).elf _disasm2 _eind

_disasm1: 
	@echo	"  > Doel: Disassembleeer de code voor debugging."

_disasm2: $(MAKEFOLDER)/$(PROJECTNAME).elf
	$(TOOLDUMP) -d $(MAKEFOLDER)$(PROJECTNAME).elf

# Doel: make clean
clean: _start _clean _eind

_clean:
	@echo "  > Doel: Verwijder alle gegenereerde .hex, .elf, en .o bestanden."
	rm -f    $(MAKEFOLDER)$(PROJECTNAME).hex 
	rm -f    $(MAKEFOLDER)$(PROJECTNAME).elf
	rm -f -r $(MAKEFOLDER)$(SRCFOLDER)

# Aanmaken specifieke bestandsformaten
$(MAKEFOLDER)$(PROJECTNAME).hex: $(MAKEFOLDER)$(PROJECTNAME).elf
	@echo "_________________________________"
	@echo "  > Aanmaken hex-bestand"
	rm -f $(MAKEFOLDER)$(PROJECTNAME).hex
	$(TOOLCOPY) -j .text -j .data -O ihex $(MAKEFOLDER)$(PROJECTNAME).elf $(MAKEFOLDER)$(PROJECTNAME).hex
	@echo "_________________________________"
	@echo "  > Geheugengebruik"
	$(TOOLSIZE) $(MAKEFOLDER)$(PROJECTNAME).elf

$(MAKEFOLDER)$(PROJECTNAME).elf: _compiler $(OBJLIST)
	@echo "_________________________________"
	@echo "  > Start linker"
	$(TOOLCPP) -o $(MAKEFOLDER)$(PROJECTNAME).elf $(OBJLIST2)

_compiler:
	@echo "_________________________________"
	@echo "  > Start compiler"

%.o: %.c
	@mkdir -p $(MAKEFOLDER)$(dir $@)
	$(TOOLCPP) $(INCLUDELIST) -c -o $(MAKEFOLDER)$@ $< 

%.o: %.cpp
	@mkdir -p $(MAKEFOLDER)$(dir $@)
	$(TOOLCPP) $(INCLUDELIST) -c -o $(MAKEFOLDER)$@ $< 


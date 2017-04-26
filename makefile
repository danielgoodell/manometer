#Compiler and Linker
CC          := gcc

#The Target Binary Program
TARGET      := manometer

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR      := src
INCDIR      := src
BUILDDIR    := obj
TARGETDIR   := .
RESDIR      := res

SRCEXT      := c
DEPEXT      := d
OBJEXT      := o

#Flags, Libraries and Includes
CFLAGS      = -lprofiler -gdwarf -Wall -pedantic -std=c11 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -lm -Og -lpthread -lrt -D_POSIX_C_SOURCE=200809L $(shell pkg-config --cflags --libs x11 cairo libmodbus)
LDFLAGS     = -lprofiler -lm -lpthread -lrt $(shell pkg-config --libs x11 cairo libmodbus)
INC         := -I$(INCDIR) -I$(SRCDIR)
INCDEP      := -I$(INCDIR) -I$(SRCDIR)

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

#Defauilt Make
all: $(TARGET)

#Remake
remake: cleaner all

#Copy Resources from Resources Directory to Target Directory
resources: directories
	@cp $(RESDIR)/* $(TARGETDIR)/

#Make the Directories
directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

#Clean only Objecst
clean:
	@$(RM) -rf $(BUILDDIR)

#Full Clean, Objects and Binaries
cleaner: clean
	@$(RM) -rf $(TARGETDIR)/$(TARGET)

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Check source files with cppcheck
check:
	@cppcheck -I ./$(SRCDIR)/ --enable=all ./$(SRCDIR)/.

#Link
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LDFLAGS)

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp
	
run: $(TARGET)
	./$(TARGET)

#Non-File Targets
.PHONY: all remake clean cleaner resources

options:
	@echo dmenu build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	
cleanup:
	@clang-format -style=file -i ./$(SRCDIR)/*.c





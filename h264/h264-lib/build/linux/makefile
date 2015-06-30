###
###     Makefile for t264 encoder&decoder
###
###     generated for UNIX/LINUX environments
###     by Thomascatlee
###

NAME=   t264

### include debug information: 1=yes, 0=no
#DBG= 0


DEPEND= dependencies

BINDIR= ../bin
INCLUDE= -I../../encoder  -I../../decoder -I../../encoder/plugins -I../../common -I../../encoder/sse2
ENCODESRCDIR= ../../encoder
DECODESRCDIR= ../../decoder
ADDSRCDIR= ../../common
ASMSRCDIR= ../../encoder/sse2
PLUGINSRCDIR= ../../encoder/plugins

OBJDIR= ../obj
LIBDIR= /usr/X11R6/lib

CC=/usr/local/arm/2.95.3/bin/arm-linux-gcc
#CC= $(shell which gcc)

#AS= $(shell which nasm)

# for linux
ASFLAGS= -f elf -DNOPREFIX
# for cygwin
#ASFLAGS=-f gnuwin32 -DNOPREFIX

LIBS=   -lm
# -lX11 -lXext
CFLAGS=  $(INCLUDE) -D__GCC__

ifdef DBG
SUFFIX= .dbg
CFLAGS+= -g
else
SUFFIX=
CFLAGS+= -O3
endif

OBJSUF= .o$(SUFFIX)

ENCODESRC= $(filter-out $(ENCODESRCDIR)/inter_test.c $(ENCODESRCDIR)/win.c\
,$(wildcard $(ENCODESRCDIR)/*.c))
DECODESRC= $(wildcard $(DECODESRCDIR)/*.c)
PLUGINSRC= $(wildcard $(PLUGINSRCDIR)/*.c)
ADDSRC= $(wildcard $(ADDSRCDIR)/*.c)
#ASMSRC= $(wildcard $(ASMSRCDIR)/*.asm)
OBJ=    $(ENCODESRC:$(ENCODESRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX)) \
$(DECODESRC:$(DECODESRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX)) $(PLUGINSRC:$(PLUGINSRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX)) \
$(ADDSRC:$(ADDSRCDIR)/%.c=$(OBJDIR)/%.o$(SUFFIX)) #$(ASMSRC:$(ASMSRCDIR)/%.asm=$(OBJDIR)/%.o$(SUFFIX))
BIN=    $(BINDIR)/$(NAME)$(SUFFIX)


default: depend bin tags

dependencies:
	@echo "" >dependencies

clean:
	@echo remove all objects
	@rm -f $(OBJDIR)/*

tags:
	@echo update tag table
	@etags $(ENCODESRCDIR)/*.c $(ENCODESRCDIR)/*.h $(DECODESRCDIR)/*.c $(DECODESRCDIR)/*.h \
	$(ADDSRCDIR)/*.c $(ADDSRCDIR)/*.h $(PLUGINSRCDIR)/*.c $(PLUGINSRCDIR)/*.h

bin:    $(OBJ)
	@echo
	@echo 'creating binary "$(BIN)"'
	@$(CC) -o $(BIN) $(OBJ) $(LIBS) 
#-L$(LIBDIR)
	@echo '... done'
	@echo

depend:
	@echo
	@echo 'checking dependencies'
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $(ENCODESRC) $(DECODESRC) $(PLUGINSRC) $(ADDSRC)\
         | sed '\''s@\(.*\)\.o[ :]@$(OBJDIR)/\1.o$(SUFFIX):@g'\''               \
         >$(DEPEND)'
	@echo

$(OBJDIR)/%.o$(SUFFIX): $(ENCODESRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c $(CFLAGS) -o $@ $<
	
$(OBJDIR)/%.o$(SUFFIX): $(DECODESRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c $(CFLAGS) -o $@ $<
	
$(OBJDIR)/%.o$(SUFFIX): $(PLUGINSRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c $(CFLAGS) -o $@ $<		

$(OBJDIR)/%.o$(SUFFIX): $(ADDSRCDIR)/%.c
	@echo 'compiling object file "$@" ...'
	@$(CC) -c $(CFLAGS) -o $@ $<
	
#$(OBJDIR)/%.o$(SUFFIX): $(ASMSRCDIR)/%.asm
#	@echo 'compiling object file "$@" ...'
#	@$(AS) $(ASFLAGS) -o $@ $<


include $(DEPEND)


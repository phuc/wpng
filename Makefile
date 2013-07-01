# Sample makefile for rpng-x / rpng2-x / wpng using gcc and make.
# Greg Roelofs
# Last modified:  2 June 2007
#
#	The programs built by this makefile are described in the book,
#	"PNG:  The Definitive Guide," by Greg Roelofs (O'Reilly and
#	Associates, 1999).  Go buy a copy, eh?  Well, OK, it's not
#	generally for sale anymore, but it's the thought that counts,
#	right?  (Hint:  http://www.libpng.org/pub/png/book/ )
#
# Invoke this makefile from a shell prompt in the usual way; for example:
#
#	make -f Makefile.unx
#
# This makefile assumes libpng and zlib have already been built or downloaded
# and are installed in /usr/local/{include,lib} or as otherwise indicated by
# the PNG* and Z* macros below.  Edit as appropriate--choose only ONE each of
# the PNGINC, PNGLIBd, PNGLIBs, ZINC, ZLIBd and ZLIBs lines.
#
# This makefile builds both dynamically and statically linked executables
# (against libpng and zlib, that is), but that can be changed by modifying
# the "EXES =" line.  (You need only one set, but for testing it can be handy
# to have both.)


# macros --------------------------------------------------------------------

PNGDIR = /usr/X11/lib
PNGINC = -I/usr/X11/include/libpng15
PNGLIBd = -L$(PNGDIR) -lpng15 # dynamically linked, installed libpng
PNGLIBs = $(PNGDIR)/libpng15.dylib # statically linked, installed libpng
# or:
#PNGDIR = ../..#	this one is for libpng-x.y.z/contrib/gregbook builds
##PNGDIR = ../libpng
#PNGINC = -I$(PNGDIR)
#PNGLIBd = -Wl,-rpath,$(PNGDIR) -L$(PNGDIR) -lpng12	# dynamically linked
#PNGLIBs = $(PNGDIR)/libpng.a		# statically linked, local libpng

ZDIR = /usr/lib
#ZDIR = /usr/lib64
ZINC = -I/usr/include
ZLIBd = -L$(ZDIR) -lz			# dynamically linked against zlib
ZLIBs = $(ZDIR)/libz.dylib			# statically linked against zlib
# or:
#ZDIR = ../zlib
#ZINC = -I$(ZDIR)
#ZLIBd = -Wl,-rpath,$(ZDIR) -L$(ZDIR) -lz  # -rpath allows in-place testing
#ZLIBs = $(ZDIR)/libz.a

#XINC = -I/usr/include			# old-style, stock X distributions
#XLIB = -L/usr/lib/X11 -lX11		#  (including SGI IRIX)
#XINC = -I/usr/openwin/include		# Sun workstations (OpenWindows)
#XLIB = -L/usr/openwin/lib -lX11
XINC = -I/usr/X11R6/include		# new X distributions (X.org, etc.)
XLIB = -L/usr/X11R6/lib -lX11
#XLIB = -L/usr/X11R6/lib64 -lX11	# e.g., Red Hat on AMD64

INCS = $(PNGINC) $(ZINC) $(XINC)
RLIBSd = $(PNGLIBd) $(ZLIBd) $(XLIB) -lm
RLIBSs = $(PNGLIBs) $(ZLIBs) $(XLIB) -lm
WLIBSd = $(PNGLIBd) $(ZLIBd) -lm
WLIBSs = $(PNGLIBs) $(ZLIBs)

CC = gcc
LD = gcc
RM = rm -f
CFLAGS = -O $(INCS)
LDFLAGS =
O = .o
E =

WPNG   = wpng

WOBJS  = $(WPNG)$(O)

# implicit make rules -------------------------------------------------------

.c$(O):
	$(CC) -c $(CFLAGS) $<


# dependencies --------------------------------------------------------------

all:  $(WPNG)$(E)

$(WPNG)$(E): $(WOBJS)
	$(LD) $(LDFLAGS) -o $@ $(WOBJS) $(WLIBSd)

$(WPNG)$(O):	$(WPNG).c


# maintenance ---------------------------------------------------------------

clean:
	$(RM) $(EXES) $(ROBJS) $(ROBJS2) $(WOBJS)


SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
topdir = /usr/local/lib/ruby/1.8/i686-linux
hdrdir = $(topdir)
VPATH = $(srcdir)
prefix = $(DESTDIR)/usr/local
exec_prefix = $(prefix)
sitedir = $(prefix)/lib/ruby/site_ruby
rubylibdir = $(libdir)/ruby/$(ruby_version)
builddir = $(ac_builddir)
archdir = $(rubylibdir)/$(arch)
sbindir = $(exec_prefix)/sbin
compile_dir = $(DESTDIR)/usr/local/src/ruby-1.8.2
datadir = $(prefix)/share
includedir = $(prefix)/include
infodir = $(prefix)/info
top_builddir = $(ac_top_builddir)
sysconfdir = $(prefix)/etc
mandir = $(prefix)/man
libdir = $(exec_prefix)/lib
sharedstatedir = $(prefix)/com
oldincludedir = $(DESTDIR)/usr/include
sitearchdir = $(sitelibdir)/$(sitearch)
bindir = $(exec_prefix)/bin
localstatedir = $(prefix)/var
sitelibdir = $(sitedir)/$(ruby_version)
libexecdir = $(exec_prefix)/libexec

CC = gcc
LIBRUBY = $(LIBRUBY_A)
LIBRUBY_A = lib$(RUBY_SO_NAME)-static.a
LIBRUBYARG_SHARED = -Wl,-R -Wl,$(libdir) -L$(libdir) -L.
LIBRUBYARG_STATIC = -l$(RUBY_SO_NAME)-static

CFLAGS   =  -fPIC -g -O2 -DORBIT2=1 -pthread -I/usr/include/evolution-data-server-1.2 -I/usr/include/libbonobo-2.0 -I/usr/include/libgnome-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/orbit-2.0 -I/usr/include/bonobo-activation-2.0 -I/usr/include/gconf/2 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/gnome-vfs-2.0/include -I/usr/include/libxml2   -DORBIT2=1 -pthread -I/usr/include/evolution-data-server-1.2 -I/usr/include/libgnome-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/orbit-2.0 -I/usr/include/gconf/2 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/gnome-vfs-2.0/include -I/usr/include/bonobo-activation-2.0 -I/usr/include/libxml2   -DORBIT2=1 -pthread -I/usr/include/libgnome-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/orbit-2.0 -I/usr/include/gconf/2 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/gnome-vfs-2.0/include -I/usr/include/bonobo-activation-2.0   -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
CPPFLAGS = -I. -I$(topdir) -I$(hdrdir) -I$(srcdir)   -Wall
CXXFLAGS = $(CFLAGS)
DLDFLAGS =  -Wl,--export-dynamic -pthread -Wl,--export-dynamic -pthread -Wl,--export-dynamic -pthread
LDSHARED = $(CC) -shared
AR = ar
EXEEXT =

RUBY_INSTALL_NAME = ruby
RUBY_SO_NAME = $(RUBY_INSTALL_NAME)
arch = i686-linux
sitearch = i686-linux
ruby_version = 1.8
ruby = /usr/local/bin/ruby
RUBY = $(ruby)
RM = $(RUBY) -run -e rm -- -f
MAKEDIRS = $(RUBY) -run -e mkdir -- -p
INSTALL_PROG = $(RUBY) -run -e install -- -vpm 0755
INSTALL_DATA = $(RUBY) -run -e install -- -vpm 0644

#### End of system configuration section. ####


LIBPATH =  -L'$(libdir)' -Wl,-R'$(libdir)'
DEFFILE =

CLEANFILES =
DISTCLEANFILES =

target_prefix =
LOCAL_LIBS =
LIBS =   -lebook-1.2 -ledataserver-1.2 -lgnome-2 -lpopt -lxml2 -lpthread -lz -lgnomevfs-2 -lbonobo-2 -lgconf-2 -lbonobo-activation -lORBit-2 -lm -lgmodule-2.0 -ldl -lgthread-2.0 -lglib-2.0   -lecal-1.2 -ledataserver-1.2 -lgnome-2 -lpopt -lxml2 -lpthread -lz -lgnomevfs-2 -lbonobo-2 -lgconf-2 -lbonobo-activation -lORBit-2 -lm -lgmodule-2.0 -ldl -lgthread-2.0 -lglib-2.0   -lgnome-2 -lpopt -lgnomevfs-2 -lbonobo-2 -lgconf-2 -lbonobo-activation -lORBit-2 -lm -lgmodule-2.0 -ldl -lgthread-2.0 -lglib-2.0   -lglib-2.0   -ldl -lcrypt -lm   -lc
OBJS = revolution.o
TARGET = revolution
DLLIB = $(TARGET).so
STATIC_LIB = $(TARGET).a

RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

CLEANLIBS     = "$(TARGET).{lib,exp,il?,tds,map}" $(DLLIB)
CLEANOBJS     = "*.{o,a,s[ol],pdb,bak}"

all:		$(DLLIB)
static:		$(STATIC_LIB)

clean:
		@$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean:	clean
		@$(RM) Makefile extconf.h conftest.* mkmf.log
		@$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean:	distclean
install: $(RUBYARCHDIR)
install: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB) $(RUBYARCHDIR)
	@$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
$(RUBYARCHDIR):
	@$(MAKEDIRS) $(RUBYARCHDIR)

site-install: install

.SUFFIXES: .c .cc .m .cxx .cpp .C .o

.cc.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cxx.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.C.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(DLLIB): $(OBJS)
	@-$(RM) $@
	$(LDSHARED) $(DLDFLAGS) $(LIBPATH) -o $(DLLIB) $(OBJS) $(LOCAL_LIBS) $(LIBS)

$(STATIC_LIB): $(OBJS)
	$(AR) cru $@ $(OBJS)
	@-ranlib $(DLLIB) 2> /dev/null || true


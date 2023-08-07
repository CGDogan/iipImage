## Process this file with automake to produce Makefile.in

noinst_PROGRAMS =	iipsrv.fcgi


INCLUDES =		@INCLUDES@ @LIBFCGI_INCLUDES@ @JPEG_INCLUDES@ @TIFF_INCLUDES@
LIBS =			@LIBS@ @LIBFCGI_LIBS@ @DL_LIBS@ @JPEG_LIBS@ @TIFF_LIBS@ -lm -lopenslide -lopenjp2 -ljvm -L$(JAVA_HOME)/lib/server -Wl,-rpath $(JAVA_HOME)/lib/server
AM_LDFLAGS =		@LIBFCGI_LDFLAGS@
# jni-md.h should also be included hence the platform paths, see link in https://stackoverflow.com/a/37029528
AM_CPPFLAGS = -I/usr/local/include/openslide -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I$(JAVA_HOME)/include/darwin -I$(JAVA_HOME)/include/windows

iipsrv_fcgi_LDADD = Main.o

if ENABLE_KAKADU
iipsrv_fcgi_LDADD += KakaduImage.o
endif

if ENABLE_PNG
#iipsrv_fcgi_LDADD += PNGCompressor.o PTL.o
endif

if ENABLE_MODULES
iipsrv_fcgi_LDADD += DSOImage.o
endif

EXTRA_iipsrv_fcgi_SOURCES = DSOImage.h DSOImage.cc KakaduImage.h KakaduImage.cc PNGCompressor.h PNGCompressor.cc PTL.cc Main.cc

iipsrv_fcgi_SOURCES = \
			IIPImage.h \
			IIPImage.cc \
			TPTImage.h \
			TPTImage.cc \
			OpenSlideImage.h \
			OpenSlideImage.cc \
			BioFormatsImage.h \
			BioFormatsImage.cc \
			BioFormatsInstance.h \
			BioFormatsInstance.cc \
			BioFormatsThread.h \
			BioFormatsThread.cc \
			JPEGCompressor.h \
			JPEGCompressor.cc \
			RawTile.h \
			Timer.h \
			Cache.h \
			TileManager.h \
			TileManager.cc \
			Tokenizer.h \
			IIPResponse.h \
			IIPResponse.cc \
			BioFormatsManager.h\
			BioFormatsManager.cc \
			View.h \
			View.cc \
			Transforms.h \
			Transforms.cc \
			Environment.h \
			URL.h \
			Writer.h \
			Task.h \
			Task.cc \
			OBJ.cc \
			FIF.cc \
			JTL.cc \
			TIL.cc \
			ICC.cc \
			CVT.cc \
			Zoomify.cc \
			DeepZoom.cc \
			SPECTRA.cc \
			PFL.cc \
			IIIF.cc \
			Watermark.h \
			Watermark.cc \
			Memcached.h

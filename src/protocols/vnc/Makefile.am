#
# Copyright (C) 2013 Glyptodon LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Werror -Wall -pedantic -Iinclude \
			@COMMON_INCLUDE@                  \
			@LIBGUAC_INCLUDE@

lib_LTLIBRARIES = libguac-client-vnc.la

libguac_client_vnc_la_SOURCES = \
    client.c                    \
    convert.c                   \
    guac_handlers.c             \
    vnc_handlers.c              
    
noinst_HEADERS =      \
    client.h          \
    convert.h         \
    guac_handlers.h   \
    vnc_handlers.h    

# Optional PulseAudio support
if ENABLE_PULSE
libguac_client_vnc_la_SOURCES += pulse.c
noinst_HEADERS += pulse.h
endif

libguac_client_vnc_la_LDFLAGS = -version-info 0:0:0 \
	@CAIRO_LIBS@                                    \
	@PULSE_LIBS@                                    \
	@VNC_LIBS@ 

libguac_client_vnc_la_LIBADD  = @LIBGUAC_LTLIB@ @COMMON_LTLIB@


# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as 
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

# 
# Author: Massimo Torquati <torquati@di.unipi.it> 
# Date:   August 2014
#
#

ZMQ_ROOT	= /opt/zmq
#/usr/local/zmq
MAGICK_ROOT 	= /usr/local
#/usr/
MAGICK_VER      = -6
FF_ROOT		= $(HOME)/fastflow

CXX		= g++ -std=c++11 -DNO_DEFAULT_MAPPING #-DBLOCKING_MODE 
INCLUDES	= -I $(FF_ROOT)  -I $(MAGICK_ROOT)/include/ImageMagick$(MAGICK_VER)
CXXFLAGS  	= -Wall -Wno-comment -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICK_THREAD_LIMIT=1

LDFLAGS 	= `Magick++-config --ldflags --libs` -pthread
OPTFLAGS	=  -g #-O3 -finline-functions -DNDEBUG

TARGETS		= img img_pipe img_pipe2 img_pipe+farm img_farm+pipe img_farm img_farm2 img_farm3 img_d

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~ out/*

img		: img.cpp
img_pipe	: img_pipe.cpp
img_pipe2	: img_pipe2.cpp
img_pipe+farm	: img_pipe+farm.cpp
img_farm+pipe	: img_farm+pipe.cpp
img_farm	: img_farm.cpp
img_farm2	: img_farm2.cpp
img_farm3	: img_farm3.cpp
img_d		: img_d.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(ZMQ_ROOT)/include $(OPTFLAGS) -o $@ $< -L$(ZMQ_ROOT)/lib -lzmq $(LDFLAGS) 



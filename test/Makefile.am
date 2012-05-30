AM_CPPFLAGS=-I${top_srcdir}/src
AM_LDFLAGS=-lpthread
LDADD=/usr/local/lib/libgtest.a ${top_srcdir}/src/libyifei.a

noinst_PROGRAMS=yf_base_testor yf_thread_testor yf_proc_testor yf_driver_testor
yf_base_testor_SOURCES=yf_base_testor.cpp
yf_thread_testor_SOURCES=yf_thread_testor.cpp
yf_proc_testor_SOURCES=yf_proc_testor.cpp
yf_driver_testor_SOURCES=yf_driver_testor.cpp


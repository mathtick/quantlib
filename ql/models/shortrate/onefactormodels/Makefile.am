
AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir}

this_includedir=${includedir}/${subdir}
this_include_HEADERS = \
    all.hpp \
    blackkarasinski.hpp \
    coxingersollross.hpp \
    extendedcoxingersollross.hpp \
    hullwhite.hpp \
    vasicek.hpp

libOneFactorModels_la_SOURCES = \
    blackkarasinski.cpp \
    coxingersollross.cpp \
    extendedcoxingersollross.cpp \
    hullwhite.cpp \
    vasicek.cpp

noinst_LTLIBRARIES = libOneFactorModels.la

all.hpp: Makefile.am
	echo "/* This file is automatically generated; do not edit.     */" > $@
	echo "/* Add the files to be included into Makefile.am instead. */" >> $@
	echo >> $@
	for i in $(filter-out all.hpp, $(this_include_HEADERS)); do \
		echo "#include <${subdir}/$$i>" >> $@; \
	done
	echo >> $@
	subdirs='$(SUBDIRS)'; for i in $$subdirs; do \
		echo "#include <${subdir}/$$i/all.hpp>" >> $@; \
	done


AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

# We want to install modules in mesos directory.
pkglibdir = $(libdir)/mesos

# Initialize variables here so we can use += operator everywhere else.
GMOCK_CPPFLAGS =
GTEST_CPPFLAGS =
pkglib_LTLIBRARIES =
bin_PROGRAMS =

# Add compiler and linker flags for pthreads.
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LIBS = $(PTHREAD_LIBS)

# Add top-level include dir to our include path.
MESOS_CPPFLAGS += -I$(top_srcdir)/include
MESOS_CPPFLAGS += -I$(top_srcdir)/serenity

# Setup CPPFLAGS that are used for most source files.
AM_CPPFLAGS = $(MESOS_CPPFLAGS) -Wall -Werror
AM_CPPFLAGS += -I$(builddir)


#protobuf

#%.pb.cc %.pb.h: %.proto
#	$(PROTOC) --proto_path=$(srcdir)/include --cpp_out=$(builddir)/include/ $^

#$(PROTOC) --proto_path=include --cpp_out=build/include include/mesos.proto
#$(PROTOC) --proto_path=/include --cpp_out=$(builddir)/include include/serenity.proto

# ut


# Unit Tests

GMOCK_DIR = lib/gmock-1.7.0
GTEST_DIR = $(GMOCK_DIR)/gtest

GMOCK_CPPFLAGS +=-I ../${GTEST_DIR}/include
#GMOCK_CPPFLAGS +=-I ../${GTEST_DIR}/lib
GMOCK_CPPFLAGS +=-I ../${GTEST_DIR}
GMOCK_CPPFLAGS +=-I ../${GMOCK_DIR}/include
#GMOCK_CPPFLAGS +=-I ../${GMOCK_DIR}/lib
GMOCK_CPPFLAGS +=-I ../${GMOCK_DIR}


GMOCK_CPPFLAGS +=-I ${GTEST_DIR}/include
GMOCK_CPPFLAGS +=-I ${GTEST_DIR}
GMOCK_CPPFLAGS +=-I ${GMOCK_DIR}/include
GMOCK_CPPFLAGS +=-I ${GMOCK_DIR}

#pkglib_LTLIBRARIES += libgmock.la
#libgmock_la_SOURCES = ${GMOCK_DIR}/src/gmock-all.cc
#libgmock_la_CPPFLAGS = $(GMOCK_CPPFLAGS)
#libgmock_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared


# libserenity

pkglib_LTLIBRARIES += libserenity.la
libserenity_la_SOURCES = bus/bus_socket.cpp
#libserenity_la_SOURCES += estimator/serenity_estimator.cpp
libserenity_la_SOURCES += filters/moving_average.cpp
libserenity_la_SOURCES += filters/exponential_moving_average.cpp
libserenity_la_SOURCES += sources/apm_source.cpp
libserenity_la_SOURCES += sources/dummy_source.cpp
libserenity_la_SOURCES += qos_controllers/serenity_qos_controller.cpp
#libserenity_la_SOURCES += ${GMOCK_DIR}/src/gmock-all.cc
#libserenity_la_SOURCES += include/mesos.pb.cc include/mesos.pb.h
#libserenity_la_SOURCES += include/serenity.pb.cc include/serenity.pb.h
libserenity_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared

#pkglib_LTLIBRARIES += libserenity_estimator.la
#libserenity_estimator_la_SOURCES = estimator/serenity_estimator.cpp
#libserenity_estimator_la_LIBADD = libserenity.la $(LDADD)
#libserenity_estimator_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared

pkglib_LTLIBRARIES += libserenity_qos_controller.la
libserenity_qos_controller_la_SOURCES = qos_controllers/serenity_qos_controller.cpp
libserenity_qos_controller_la_LIBADD = libserenity.la $(LDADD)
libserenity_qos_controller_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared

#pkglib_LTLIBRARIES += libdummy_isolator.la
#libdummy_isolator_la_SOURCES = dummyisolator/isolator.cpp
#libdummy_isolator_la_LDFLAGS = -release $(PACKAGE_VERSION) -shared

#pkglibexec_PROGRAMS = pipeline_test
#pipeline_test_SOURCES = pipeline/pipeline_test.cpp
#pipeline_test_LDADD = libserenity.la $(LDADD)
#pipeline_test_CPPFLAGS = $(MESOS_CPPFLAGS)





#mesos_health_check_LDADD = libmesos.la $(LDADD)

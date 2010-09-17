## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

rhod_key_driverdocdir = ${prefix}/doc/rhod_key_driver
rhod_key_driverdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(rhod_key_driverdoc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(rhod_key_driverdocdir)

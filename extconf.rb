require 'mkmf'

pkg_config('libebook-1.2') || pkg_config('libebook-1.0')
pkg_config('libecal-1.2') || pkg_config('libecal-1.0')
pkg_config('evolution-data-server-1.2') || pkg_config('evolution-data-server-1.0')
pkg_config('gobject-2.0')

$CPPFLAGS += " -Wall"

create_makefile("revolution")


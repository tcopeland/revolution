#!/usr/local/bin/ruby

require 'rubygems'

spec = Gem::Specification.new do |s|
	s.name = "revolution"
	s.version = "0.5"
	s.platform = Gem::Platform::RUBY
	s.required_ruby_version = ">=1.8.0"
	s.summary = "Revolution is a binding for the Evolution email client"
	s.files = ["revolution.c","extconf.rb"]
	s.files.concat [ "LICENSE" ]
	s.require_path = ""
	s.autorequire = "revolution"
	s.author = "Tom Copeland"
	s.has_rdoc = false
	s.email = "tom@infoether.com"
	s.extensions << "extconf.rb"
	s.homepage = "http://revolution.rubyforge.org/"
	s.rubyforge_project = "revolution"	
end

#!/usr/local/bin/ruby

Dir.chdir("../")

require 'rubygems'
require 'revolution'

r = Revolution::Revolution.new

last_week = Time.new.to_i-(7*24*60*60)
next_week = Time.new.to_i+(7*24*60*60)

r.get_all_appointments(last_week.to_i, next_week.to_i).each {|appt|
	puts "Appointment"
	puts "\tSummary: #{appt.summary}"
	puts "\tLocation: #{appt.location}"
	puts "\tOrganizer: #{appt.organizer}"
}

r.get_all_contacts.each {|c|
	puts "Name is #{c.first_name} #{c.last_name}, work email address is #{c.work_email}"
}

Dir.chdir("revolution")

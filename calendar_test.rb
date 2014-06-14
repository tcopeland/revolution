#!/usr/local/bin/ruby

require 'revolution'

last_week = Time.new.to_i-(7*24*60*60)
next_week = Time.new.to_i+(7*24*60*60)

Revolution::Revolution.new.get_all_appointments(last_week, next_week).each {|appt|
	puts "Appointment (#{appt.uid})"
	puts " Summary: #{appt.summary}"
	puts " Location: #{appt.location}"
	puts " Organizer: #{appt.organizer}"
	puts " Start time: #{appt.start}"
	puts " End time: #{appt.end}"
	puts " Last modified date: #{appt.last_modification}"
	puts " Alarm is set: #{appt.alarm_set}"
	puts " Busy : #{appt.busy_status}"
	puts " Recurring : #{appt.recurring}"

}

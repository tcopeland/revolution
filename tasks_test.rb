#!/usr/local/bin/ruby

require 'revolution'

Revolution::Revolution.new.get_all_tasks.each {|t|
	puts "Task (#{t.uid})"
	puts " Summary: #{t.summary}" if t.summary
	puts " Description: #{t.description.slice(0,70)}" if t.description
	puts " Start date: #{t.start}" if t.start
	puts " Due date: #{t.due}" if t.due
	puts " Status: #{t.status}" if t.status
	puts " Priority: #{t.priority}" if t.priority
	puts " Last modified date: #{t.last_modification}"
}

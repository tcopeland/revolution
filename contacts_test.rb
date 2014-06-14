#!/usr/local/bin/ruby

require 'revolution'

def p_addr(desc, addr) 
	puts desc
	puts "  POB: #{addr.po}" if addr.po
	puts "  Ext: #{addr.ext}" if addr.ext
	puts "  Street: #{addr.street}" if addr.street
	puts "  Locality: #{addr.locality}" if addr.locality
	puts "  Region: #{addr.region}" if addr.region
	puts "  Code: #{addr.code}" if addr.code
	puts "  Country: #{addr.country}" if addr.country
end

def p_im_addr(addr) 
	puts " #{addr.provider} IM handle"
	puts "  Location: #{addr.location}"
	puts "  Address: #{addr.address}"
end

Revolution::Revolution.new.get_all_contacts.each {|c|
	puts "Contact (#{c.uid})"
	puts " First name: #{c.first_name}" if c.first_name
	puts " Last name: #{c.last_name}" if c.last_name
	puts " Birthday: #{c.birthday}" if c.birthday
	puts " Home phone: #{c.home_phone}" if c.home_phone
	puts " Work phone: #{c.work_phone}" if c.work_phone
	puts " Mobile phone: #{c.mobile_phone}" if c.mobile_phone
	puts " Organization: #{c.organization}" if c.organization
	puts " Title: #{c.title}" if c.title
	c.email_addresses.keys.each {|k|
		puts " #{k} email addresses: #{c.email_addresses[k].join(' ')}"
	}
	p_addr(" Home address: ", c.home_address) if c.home_address
	p_addr(" Work address: ", c.work_address) if c.work_address
	p_addr(" Other address: ", c.other_address) if c.other_address
	c.im_addresses.each {|a| p_im_addr(a) } if !c.im_addresses.empty?
}

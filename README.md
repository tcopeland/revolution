Revolution is a Ruby binding for the <a href="http://www.gnome.org/projects/evolution/">Evolution</a> email client.  It's sponsored - i.e., written on work time and open sourced with a BSD license - by <a href="http://infoether.com/">InfoEther</a>.

<h3>Installation</h3>

If you have <a href="http://rubygems.rubyforge.org/">RubyGems</a> installed, you can install Revolution from the gem server like this:

    [root@hal revolution]# gem install --remote revolution
    Attempting remote installation of 'revolution'
    Updating Gem source index for: http://gems.rubyforge.org
    Building native extensions.  This could take a while...
    ruby extconf.rb install --remote revolution
    creating Makefile

    make
    [ make output elided ]

    make install
    make: Nothing to be done for `install'.
    Successfully installed revolution-0.5
    [root@hal revolution]#

Or you can download the <a href="http://rubyforge.org/frs/?group_id=576">Gem</a> and do the same thing without using the <code>--remote</code> flag.
Note that the build process assumes that you have the evolution-data-server-devel header files and whatnot installed.  I've developed this on Fedora Core 3, so if you get the RPMs for that it should work fine.

Or you can download the <a href="http://rubyforge.org/frs/?group_id=576">zip file</a>; it contains a precompiled shared object library as well as the source code and the extconf.rb necessary to rebuild it.

<h3>Documentation</h3>

To use Revolution, you'll need <a href="http://ruby-lang.org/">Ruby</a>, <a href="http://rubyforge.org/projects/revolution/">Revolution</a> itself, and of course an <a href="http://www.gnome.org/projects/evolution/">Evolution</a> installation.

Here's the <a href="doc/index.html">complete API documentation</a>; below are some simple examples:

    require 'rubygems'
    require 'revolution'

    r = Revolution::Revolution.new

    r.get_all_contacts.each {|c|
     puts "First name: #{c.first_name}"
     puts "Last name: #{c.last_name}"
     c.email_addresses.keys.each {|k|
    	puts " #{k} email addresses: #{c.email_addresses[k].join(' ')}"
     }
    }

    last_week = Time.new.to_i-(7*24*60*60)
    next_week = Time.new.to_i+(7*24*60*60)
    r.get_all_appointments(last_week, next_week).each {|appt|
     puts "Summary: #{appt.summary}"
     puts "Location: #{appt.location}"
     puts "Organizer: #{appt.organizer}"
    }

    r.get_all_tasks.each {|t|
     puts "Summary: #{t.summary}"
     puts "Description: #{t.description.slice(0,70)}"
     puts "Due date: #{t.due}"
    }

Revolution provides an easy way to quickly test the s-expression query format that Evolution uses internally.  Here's an example session from interactive Ruby - irb.
First some initialization:

    [tom@hal revolution]$ irb
    irb(main):001:0> require 'rubygems'
    => true
    irb(main):002:0> require_gem 'revolution'
    => true
    irb(main):003:0> rev = Revolution::Revolution.new
    => #&lt;Revolution::Revolution:0xf6cefa6c&gt;

Now, on to querying all the Copelands:

    irb(main):004:0> rev.get_contacts_with_query("(contains 'full_name' 'Copeland')").each {|c| puts c.first_name }
    Madelyn
    Lee
    Alina
    Dave

We should get the same results when using the <code>beginswith</code> identifier:

    irb(main):009:0> rev.get_contacts_with_query("(beginswith 'full_name' 'Copeland')").each {|c| puts c.first_name }
    Madelyn
    Lee
    Alina
    Dave
    irb(main):010:0>

Yup, right on.  So how many InfoEther folks are in my contact book?

    irb(main):021:0> rev.get_contacts_with_query("(endswith 'email' 'infoether.com')").size
    => 7
    irb(main):022:0>

Hm, and we've only got five employees.  How odd.  

Anyhow, that gives you an idea on how it works.  Perhaps future releases will have a similar querying functionality for appointments and tasks... if that sounds useful to you, please <a href="http://rubyforge.org/tracker/?atid=2291&group_id=576&func=browse">file an RFE</a>.


If there's anything we can add to make this documentation more helpful, please let us know about it by posting to the <a href="http://rubyforge.org/forum/forum.php?forum_id=2489">project forum</a>.  Thanks!


<h3>Credits</h3>
Committers:
<ul>
<li>Tom Copeland - Wrote the initial code</li>
</ul>
Contributors
<ul>
<li>Vicent Segui - Added calendar_to_ical, added get_all_appointments, patch in change from get_all_appointments to get_appointments_between, implemented ESource mechanism for multiple data calendar/task sources, reported bug with get_all_appointments not honoring time parameters</li>
<li>Hans Petter Jansson - helped with adding email addresses to contacts</li>
<li>Leon Breedt - extconf.rb suggestions</li>
<li>Yaakov Selkowitz - extconf.rb patch</li>
<li>Dave Craine - sample code, development ideas</li>
<li>Rich Kilmer - development ideas and discussions</li>
</ul>

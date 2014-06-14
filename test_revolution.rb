#!/usr/local/bin/ruby

require 'revolution'
require 'test/unit'

class TestRevolution < Test::Unit::TestCase
include Revolution
	def setup
		@r = Revolution.new
	end
	def make_contact(first_name="Ron", last_name="Guidry")
		c = Contact.new
		c.first_name = first_name
		c.last_name = last_name
		c
	end
	def make_appt(start=Time.now, endtime=Time.now+(3600*2))
		a = Appointment.new
    a.summary = "test_appt_summary"
		a.start = start
		a.end = endtime
		a
	end
	def test_get_all_uids
		uid = @r.add_contact(make_contact)
    assert_not_nil(@r.get_all_contact_uids.find {|x| x == uid })
		@r.delete_contact(uid)
	end
	def test_get_add_and_get_by_uid
		uid = @r.add_contact(make_contact)
		new_contact = @r.get_contact_by_uid(uid)
		assert_equal(make_contact().last_name, new_contact.last_name, "Last names don't match!")
		@r.delete_contact(uid)
	end
	def test_delete_by_uid
		uid = @r.add_contact(make_contact)
		assert_not_nil(@r.get_contact_by_uid(uid))
		@r.delete_contact(uid)
		assert_raise(RevolutionException) { @r.get_contact_by_uid(uid) }
	end
	def test_delete_non_existent_contact_uid
		assert_raise(RevolutionException) { @r.delete_contact("foobar") }
	end
	def test_get_non_existent_contact_uid
		assert_raise(RevolutionException) { @r.get_contact_by_uid("foobar") }
	end
	def test_get_non_existent_name
		assert(@r.get_contacts_by_name("foobar").empty?)
	end
	def test_get_by_name
		@r.add_contact(make_contact())
		a = @r.get_contacts_by_name("Guidry")
		assert_equal(1, a.size)
		@r.delete_contact(a[0].uid)
	end
	def test_get_contacts_with_sexp
		@r.add_contact(make_contact)
		@r.add_contact(make_contact("Ron", "Guidry2"))
		a = @r.get_contacts_with_query("(contains 'full_name' 'Guidry')")
		assert_equal(2, a.size)
		a = @r.get_contacts_with_query("(beginswith 'full_name' 'Gui')")
		assert_equal(2, a.size)
		a = @r.get_contacts_with_query("(endswith 'full_name' 'dry2')")
		assert_equal(1, a.size)
		a = @r.get_contacts_with_query("(is 'full_name' 'Guidry')")
		assert_equal(1, a.size)
		@r.get_contacts_with_query("(contains 'full_name' 'Guidry')").each {|c| @r.delete_contact(c.uid) }
	end
	def test_add_contact_phone
		c = make_contact
		c.home_phone = "703-444-4444"
		c.work_phone = "703-444-4445"
		c.mobile_phone = "703-444-4446"
		uid = @r.add_contact(c)
		new_contact = @r.get_contact_by_uid(uid)
		@r.delete_contact(uid)
		assert_equal(new_contact.home_phone, c.home_phone)
		assert_equal(new_contact.work_phone, c.work_phone)
		assert_equal(new_contact.mobile_phone, c.mobile_phone)
	end
	def test_add_contact_job_info
		c = make_contact
		c.organization= "InfoEther"
		c.title = "Programmer"
		uid = @r.add_contact(c)
		new_contact = @r.get_contact_by_uid(uid)
		@r.delete_contact(uid)
		assert_equal(new_contact.organization, c.organization)
		assert_equal(new_contact.title, c.title)
	end
	def test_get_all_contacts
		assert(!@r.get_all_contacts.empty?)
	end
	def test_add_contact_birthday
		c = make_contact
		t = Time.new
		c.birthday = t
		uid = @r.add_contact(c)
		new_contact = @r.get_contact_by_uid(uid)
		@r.delete_contact(uid)
		assert_equal(t.year, c.birthday.year)
		assert_equal(t.mon, c.birthday.mon)
		assert_equal(t.day, c.birthday.day)
	end
	def test_add_contact_email
		c = make_contact
		c.email_addresses = {
			"WORK"=>["tom@infoether.com","jim@infoether.com", "bill@infoether.com"],
			"HOME"=>["fred@infoether.com"],
			"OTHER"=>["joe@infoether.com","jeff@infoether.com"],
		}
		uid = @r.add_contact(c)
		new_contact = @r.get_contact_by_uid(uid)
		@r.delete_contact(uid)
		assert_not_nil(new_contact.email_addresses["WORK"], "No work email addresses picked up")
		assert_not_nil(new_contact.email_addresses["HOME"], "No home email addresses picked up")
		assert_not_nil(new_contact.email_addresses["OTHER"], "No 'other' email addresses picked up")
		assert_equal("tom@infoether.com", new_contact.email_addresses["WORK"][0])
		assert_equal("jim@infoether.com", new_contact.email_addresses["WORK"][1])
		assert_equal("bill@infoether.com", new_contact.email_addresses["WORK"][2])
		assert_equal("fred@infoether.com", new_contact.email_addresses["HOME"][0])
		assert_equal("joe@infoether.com", new_contact.email_addresses["OTHER"][0])
		assert_equal("jeff@infoether.com", new_contact.email_addresses["OTHER"][1])
	end
	def test_export_im_handle
		c = make_contact
		c.im_addresses = [ContactIMAddress.new("AIM", "HOME", "tom"), ContactIMAddress.new("AIM", "WORK", "tom")]
		uid = @r.add_contact(c)
		new_contact = @r.get_contact_by_uid(uid)
		@r.delete_contact(uid)
		assert_equal("tom", new_contact.im_addresses[0].address)
		assert_equal("AIM", new_contact.im_addresses[0].provider)
		assert_equal("HOME", new_contact.im_addresses[0].location)
		assert_equal("WORK", new_contact.im_addresses[1].location)
	end
  def test_get_all_appts
    a = make_appt
    uid = @r.add_appointment(a)
    begin 
      assert_equal(1,  @r.get_all_appointments.collect {|x| x.summary == a.summary ? x : nil }.compact.size)
      @r.delete_appointment(uid)
    rescue StandardError => e
      @r.delete_appointment(uid)
      raise e
    end    
  end
  def test_delete_nonexistent_appt
    assert_raise(RevolutionException) {@r.delete_appointment("foo")}
  end
  def test_add_delete_appt
    a = make_appt
    uid = @r.add_appointment(a)
    begin
      a1 = @r.get_appointment_by_uid(uid)
      assert_equal(a1.summary, a.summary)
      @r.delete_appointment(uid)
    rescue StandardError => e
      @r.delete_appointment(uid)
      raise e
    end    
  end
  def test_add_appt_without_start_time
    assert_raise(RevolutionException) { @r.add_appointment(make_appt(nil, Time.now)) }
  end
  def test_add_appt_without_end_time
    assert_raise(RevolutionException) { @r.add_appointment(make_appt(Time.now, nil)) }
  end
  def test_add_appt_specific_time
    start = Time.at(1121180400) # starts Tue Jul 12 11:00:00 EDT 2005
    endtime = start + 3600 #  end at +1 hour
    a = make_appt(start, endtime)
    uid = @r.add_appointment(a)
    begin
      appts = @r.get_all_appointments.collect {|x| x.summary == a.summary ? x : nil }.compact
      assert_equal(a.start, appts[0].start)
      assert_equal(a.end, appts[0].end)
      @r.delete_appointment(uid)
    rescue StandardError => e
      @r.delete_appointment(uid)
      raise e
    end
  end
  def test_get_appointment_by_uid
    uid = @r.add_appointment(make_appt)
    assert(!@r.get_appointment_by_uid(uid).nil?)
    @r.delete_appointment(uid)
  end
  def test_missing_appt_uid
    assert_raise(RevolutionException) { @r.get_appointment_by_uid("foo") }
  end
  def test_add_appt_organizer
    a = make_appt
    a.organizer = "Fred"
    uid = @r.add_appointment(a)
    assert_equal(a.organizer, @r.get_appointment_by_uid(uid).organizer)
    @r.delete_appointment(uid)
  end
  def test_get_appts_specific_time
    # first appt
    start = Time.at(1121180400) # starts Tue Jul 12 11:00:00 EDT 2005
    endtime = start + 3600 #  end at +1 hour
    a = make_appt(start, endtime)
    uid = @r.add_appointment(a)
    # second appt
    start1 = Time.at(1121180400+10000) # starts Tue Jul 12 12:00:00 EDT 2005
    endtime1 = start1 + 3600 #  end at +1 hour
    a1 = make_appt(start1, endtime1)
    uid1 = @r.add_appointment(a1)
    appts = @r.get_appointments_between(start-50, endtime+50)
    begin
      assert_equal(1, appts.size)
      @r.delete_appointment(uid)
      @r.delete_appointment(uid1)
    rescue StandardError => e
      @r.delete_appointment(uid)
      @r.delete_appointment(uid1)
      raise e
    end
  end
  def test_add_delete_task
    t = Task.new
    t.summary = "test_task_summary"
    uid = @r.add_task(t)
    @r.delete_task(uid)
  end
  def test_delete_nonexistent_task
    assert_raise(RevolutionException) {@r.delete_task("foo")}
  end
  def test_get_all_tasks
    t = Task.new
    t.summary = "test_task_summary"
    uid = @r.add_task(t)
    begin
      assert_equal(1, @r.get_all_tasks.collect {|x| x.summary == t.summary ? x : nil }.compact.size)
      @r.delete_task(uid)
    rescue StandardError => e
      @r.delete_task(uid)
      raise e
    end    
  end
end

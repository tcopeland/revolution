#include <stdio.h>
#include <time.h>
#include "ruby.h"
#include <libebook/e-book.h>
#include <libebook/e-contact.h>
#include <libecal/e-cal.h>
#include <libecal/e-cal-time-util.h>

#define COLOR_FORMAT_STRING "%06x"

#define GCONF_CALENDAR_SECTION "/apps/evolution/calendar/sources"
#define GCONF_TASKS_SECTION "/apps/evolution/tasks/sources"
#define GCONF_ADDRESSBOOK_SECTION "/apps/evolution/addressbook/sources"

static VALUE rb_cRevolution;
static VALUE rb_cRevolutionException;
static VALUE rb_mRevolution;
static VALUE rb_cAppointment;
static VALUE rb_cTask;
static VALUE rb_cContact;
static VALUE rb_cContactAddress;
static VALUE rb_cContactIMAddress;
static VALUE rb_cESourceGroup;
static VALUE rb_cESource;

static GHashTable* im_hash;
static EBook* book;
void check_error(GError* error, const char* msg_fmt) {
  if (error) {
    char* msg = g_strdup(error->message);
    g_clear_error(&error);
    rb_raise(rb_cRevolutionException, msg_fmt, msg);
  }
}

//////////////////////////////////////////////////
// Sources related functions
//
//

static ESource* get_source_from_uid(const char* uid, ECalSourceType type) {
  ESourceList* list;
  GError* error=0;
  e_cal_get_sources(&list, type, &error);
  check_error(error, "Unable to retrieve calendar/tasks sources: %s");
  ESource* src = e_source_list_peek_source_by_uid(list, uid);
  if(!src)
	rb_raise(rb_cRevolutionException, "Unable to retrieve calendar/tasks sources %s",uid);
  //Reference src and src group so they don't get cleared
  //when we clear the list
  g_object_ref(src);
  g_object_ref(e_source_peek_group(src));
  //Clear list
  g_object_unref(list);
  return src;
}


static ESource* get_addressbook_source_from_uid(const char* uid) {
  ESourceList* list;
  GError* error = 0;
  e_book_get_addressbooks(&list,  &error);
  check_error(error,"Unable to retrieve calendar/tasks sources %s");
  ESource* src = e_source_list_peek_source_by_uid(list, uid);
  //Reference src and src group so they don't get cleared
  //when we clear the list
  g_object_ref(src);
  g_object_ref(e_source_peek_group(src));
  g_object_unref(list);
  return src;
}

/*
 *  call-seq:
 *    new() -> Revolution
 *
 * Creates a new Revolution object
 */
static VALUE revolution_init(VALUE self) {
  return Qnil;
}

static VALUE revolution_init3(int argc, VALUE *argv, VALUE self) {
  if (argc > 0) {
	if(rb_obj_is_kind_of(argv[0], rb_cESource) == Qtrue)
	  argv[0]=rb_iv_get(argv[0], "@uid");
	rb_iv_set(self, "@calendar_source", argv[0]);
  } else {
    rb_iv_set(self, "@calendar_source", Qnil);
  }

  if (argc > 1) {
	if(rb_obj_is_kind_of(argv[1], rb_cESource) == Qtrue)
	  argv[1]=rb_iv_get(argv[1], "@uid");
	rb_iv_set(self, "@tasks_source", argv[1]);

  } else {
    rb_iv_set(self, "@tasks_source", Qnil);
 }

  if (argc > 2) {
	if(rb_obj_is_kind_of(argv[2], rb_cESource) == Qtrue)
	  argv[2]=rb_iv_get(argv[2], "@uid");
	rb_iv_set(self, "@addressbook_source", argv[2]);
  } else {
    rb_iv_set(self, "@addressbook_source", Qnil);
  }
  return Qnil;
}

int subtract_offset(struct icaltimetype time) {
  int utc = icaltime_as_timet_with_zone(time, icaltimezone_get_utc_timezone());
  int offset = NUM2INT(rb_funcall(rb_funcall( rb_cTime, rb_intern( "now" ), 0), rb_intern("gmt_offset"), 0));
  return utc - offset;
}

int add_offset(VALUE rb_start) {
  //int offset = NUM2INT(rb_funcall(rb_funcall( rb_cTime, rb_intern("now"), 0), rb_intern("gmt_offset"), 0));
  int ticks = NUM2INT(rb_funcall(rb_start, rb_intern("to_i"), 0));
  return ticks/* + offset*/;
}

ECal* open_cal(const char* uid, ECalSourceType type) {
  ECal* cal;
  ESource* src;
  src=get_source_from_uid(uid, type);
  if (!src)
    rb_raise(rb_cRevolutionException, "ERROR: Could not find source \"%s\"", uid);	
  cal = e_cal_new(src, type);
  //Clear the src and the group (we don't need them anymore)
  g_object_unref(src);
  g_object_unref(e_source_peek_group(src));
  if (!cal)
    rb_raise(rb_cRevolutionException, "ERROR: Could not find source with uid \"%s\"", uid);	
  return cal;
}

ECal* open_tasks(VALUE self) {
  ECal* cal;
  GError* error = 0;
  VALUE rb_uid = rb_iv_get(self,"@tasks_source");
  char* uid = RTEST(rb_uid) ? RSTRING(StringValue(rb_uid))->ptr : NULL;
  if(!uid)
	cal = e_cal_new_system_tasks();
  else {
	cal = open_cal(uid, E_CAL_SOURCE_TYPE_TODO);
  }
  e_cal_open(cal, FALSE, &error);
  check_error(error, "Unable to open tasks: %s");
  return cal;
}

ECal* open_calendar(VALUE self) {
  ECal* cal;
  GError* error = 0;
  VALUE rb_uid = rb_iv_get(self,"@calendar_source");
  char* uid = RTEST(rb_uid) ? RSTRING(StringValue(rb_uid))->ptr : NULL;
  if(!uid)
	cal = e_cal_new_system_calendar();
  else {
	cal =open_cal(uid, E_CAL_SOURCE_TYPE_EVENT);
  }
  e_cal_open(cal, FALSE, &error);
  check_error(error, "Unable to open calendar: %s");
  return cal;
}

void copy_summary(const VALUE ruby_obj, ECalComponent* ev_obj) {
  ECalComponentText summary;
  e_cal_component_get_summary(ev_obj, &summary);
  rb_iv_set(ruby_obj, "@summary", rb_str_new2(summary.value ? summary.value : ""));
}

void copy_start(const VALUE ruby_obj, ECalComponent* ev_obj) {
  ECalComponentDateTime start;
  e_cal_component_get_dtstart(ev_obj, &start);
  if (start.value) {	
	  time_t val = icaltime_as_timet_with_zone(*start.value,icaltimezone_get_builtin_timezone_from_tzid(start.tzid));	
	  rb_iv_set(ruby_obj, "@start", rb_funcall( rb_cTime, rb_intern( "at" ), 1, INT2NUM(val)));
    e_cal_component_free_datetime(&start);
  }
}

void copy_last_modification(const VALUE ruby_obj, ECalComponent* ev_obj) {
  struct icaltimetype* last_mod;
  e_cal_component_get_last_modified(ev_obj, &last_mod);
  if (last_mod) {
    rb_iv_set(ruby_obj, "@last_modification", rb_funcall( rb_cTime, rb_intern( "at" ), 1, INT2NUM(icaltime_as_timet_with_zone(*last_mod, icaltimezone_get_utc_timezone()))));
  }
}

void copy_all_day_event(const VALUE ruby_obj, ECalComponent* ev_obj) {
  // This code modeled after the update_time function in calendar/gui/dialogs/event-page.c
  // For more info, also see all_day_event_toggled_cb in the same file.
  ECalComponentDateTime startt;
  ECalComponentDateTime endt;
  e_cal_component_get_dtstart(ev_obj, &startt);
  e_cal_component_get_dtstart(ev_obj, &endt);
  /* If both times are DATE values, we set the 'All Day Event' checkbox.
  Also, if DTEND is after DTSTART, we subtract 1 day from it. */
  gboolean all_day_event = FALSE;
  struct icaltimetype *start_tt, *end_tt, implied_tt;
  start_tt = startt.value;
  end_tt = endt.value;

  // TODO should we really just bail out like this?
  if (!start_tt) {
    return;
  }
  if (!end_tt && start_tt->is_date) {
    end_tt = &implied_tt;
    *end_tt = *start_tt;
    icaltime_adjust (end_tt, 1, 0, 0, 0);
  }

  if (start_tt->is_date && end_tt->is_date) {
    all_day_event = TRUE;
    if (icaltime_compare_date_only (*end_tt, *start_tt) > 0) {
      icaltime_adjust (end_tt, -1, 0, 0, 0);
    }
  }

  if (all_day_event == TRUE) {
    rb_iv_set(ruby_obj, "@all_day_event", Qtrue);
  } else {
    rb_iv_set(ruby_obj, "@all_day_event", Qfalse);
  }
}

void copy_uid(const VALUE ruby_obj, ECalComponent* ev_obj) {
  const char* uid;
  e_cal_component_get_uid(ev_obj, &uid);
  if (uid) {
    rb_iv_set(ruby_obj, "@uid", rb_str_new2(uid));
  }
}


///////////////////////////////////////////////////////
// Tasks
/*
 *  call-seq:
 *    new() -> Task
 *
 * Creates a new Task object
 *
 * Attributes:
 *  @uid [String] a unique id for this task
 *  @summary [String]
 *  @description [String]
 *  @start [Time] the start time
 *  @due [Time] the due time
 *  @status [String] 'Not started', 'In progress', 'Completed', 'Cancelled'
 *  @priority [String] 'Low', 'Medium', 'High', or nil
 *  @last_modification [Time]
 */
static VALUE evTask_init(VALUE self) {
  return Qtrue;
}

void copy_into_task(const VALUE ruby_task, ECalComponent* ev_task) {
    copy_uid(ruby_task, ev_task);
    copy_summary(ruby_task, ev_task);
    copy_last_modification(ruby_task, ev_task);
    copy_start(ruby_task, ev_task);

    // Note that task have only one description, so this singly-linked list
    // has no more than one element.  This is as opposed to a journal object, which
    // has multiple description entries.
    GSList* descriptions = NULL;
    e_cal_component_get_description_list(ev_task, &descriptions);
    if (descriptions) {
      ECalComponentText* desc = (ECalComponentText*)descriptions->data;
      rb_iv_set(ruby_task, "@description", rb_str_new2(desc->value ? desc->value: ""));
    }
    e_cal_component_free_text_list(descriptions);

    ECalComponentDateTime due_date;
    e_cal_component_get_due(ev_task, &due_date);
    if (due_date.value) {
	  time_t val = icaltime_as_timet_with_zone(*due_date.value,icaltimezone_get_builtin_timezone_from_tzid(due_date.tzid));
      rb_iv_set(ruby_task, "@due", rb_funcall( rb_cTime, rb_intern( "at" ), 1, INT2NUM(val)));
      e_cal_component_free_datetime(&due_date);
    }

    // Does this need to be freed?  There's no API call to free it...
    icalproperty_status status;
    e_cal_component_get_status(ev_task, &status);
    if (status == ICAL_STATUS_NONE) {
      rb_iv_set(ruby_task, "@status", rb_str_new2("Not started"));
    } else if (status == ICAL_STATUS_INPROCESS) {
      rb_iv_set(ruby_task, "@status", rb_str_new2("In progress"));
    } else if (status == ICAL_STATUS_COMPLETED) {
      rb_iv_set(ruby_task, "@status", rb_str_new2("Completed"));
    } else if (status == ICAL_STATUS_CANCELLED) {
      rb_iv_set(ruby_task, "@status", rb_str_new2("Cancelled"));
    }

    int* priority;
    e_cal_component_get_priority(ev_task, &priority);
    if (priority) {
      if (*priority == 7) {
        rb_iv_set(ruby_task, "@priority", rb_str_new2("Low"));
      } else if (*priority == 5) {
        rb_iv_set(ruby_task, "@priority", rb_str_new2("Normal"));
      } else if (*priority == 3) {
        rb_iv_set(ruby_task, "@priority", rb_str_new2("High"));
      } /* seems like nil is as good as undefined - else {
        rb_iv_set(ruby_task, "@priority", rb_str_new2("Undefined"));
      }*/
      e_cal_component_free_priority(priority);
    }
}

/*
 *  call-seq:
 *    get_all_tasks() -> Array
 *
 * Fetches all tasks
 */
static VALUE revolution_get_all_tasks(VALUE self) {
  GList* objects, *l;
  GError* error = 0;
  e_cal_get_object_list_as_comp(open_tasks(self), "#t", &objects, &error);
  check_error(error, "Unable to query calendar: %s");
  VALUE result = rb_ary_new();
  for (l = objects; l; l = l->next) {
    ECalComponent *ev_task = E_CAL_COMPONENT (l->data);
    VALUE ruby_task = rb_class_new_instance(0, 0, rb_cTask);
    copy_into_task(ruby_task, ev_task);
    rb_ary_push(result, ruby_task);
    g_object_unref(ev_task);
  }
  g_list_free(objects);
  return result;
}

/*
 *  call-seq:
 *    add_task(Task) -> String
 *
 * Adds a task, returns the task UID
 */
static VALUE revolution_add_task(VALUE self, VALUE rb_task) {
  ECalComponent* e_task = e_cal_component_new();
  e_cal_component_set_new_vtype(e_task, E_CAL_COMPONENT_TODO);

  ECalComponentText* summary = g_new(ECalComponentText, 1);
  VALUE rb_summary = rb_iv_get(rb_task, "@summary");
  if (rb_summary != Qnil) {
    summary->value = RSTRING(StringValue(rb_summary))->ptr;
    summary->altrep = "";
  }
  e_cal_component_set_summary(e_task, summary);

  char* uid;
  GError* error = 0;
  e_cal_create_object(open_tasks(self), e_cal_component_get_icalcomponent(e_task), &uid, &error);
  check_error(error, "Unable to create task item: %s");
  return rb_str_new2(uid);
}

/*
 *  call-seq:
 *    delete_task(uid)
 *
 * Deletes a task from the Evolution task list.
 */
static VALUE revolution_delete_task(VALUE self, VALUE rb_uid) {
  char* uid = RSTRING(StringValue(rb_uid))->ptr;
  GError* error = NULL;
  e_cal_remove_object(open_tasks(self), uid, &error);
  check_error(error, "Unable to remove task: %s");
  return Qnil;
}

///////////////////////////////////////////////////////
// Calendar
/*
 *  call-seq:
 *    new() -> Appointment
 *
 * Creates a new Appointment object
 *
 * Attributes:
 *  @uid [String] a unique id for this task
 *  @summary [String]
 *  @location [String]
 *  @organizer [String]
 *  @start [Time] the start time
 *  @due [Time] the due time
 *  @alarm_set [Boolean] is an alarm set for this appointment
 *  @all_day_event [Boolean] is this an all day event
 *  @busy_status [Boolean]
 *  @recurring [Boolean] is this appointment recurring
 *  @last_modification [Time]
 */
static VALUE evAppointment_init(VALUE self) {
  return Qtrue;
}

void copy_into_appt(const VALUE ruby_appt, ECalComponent* ev_appt) {
    copy_uid(ruby_appt, ev_appt);
    copy_summary(ruby_appt, ev_appt);
    copy_start(ruby_appt, ev_appt);
    copy_last_modification(ruby_appt, ev_appt);
    copy_all_day_event(ruby_appt, ev_appt);

    const char* location;
    e_cal_component_get_location(ev_appt, &location);
    if (location) {
      rb_iv_set(ruby_appt, "@location", rb_str_new2(location));
    }

    // there's probably a good way to combine this with due_date
    ECalComponentDateTime end;
    e_cal_component_get_dtend(ev_appt, &end);
    if (end.value) {
	  time_t val = icaltime_as_timet_with_zone(*end.value,icaltimezone_get_builtin_timezone_from_tzid(end.tzid));
      rb_iv_set(ruby_appt, "@end", rb_funcall(rb_cTime, rb_intern("at"), 1, INT2NUM(val)));
      e_cal_component_free_datetime(&end);
    }

    ECalComponentOrganizer organizer;
    e_cal_component_get_organizer(ev_appt, &organizer);
    if (organizer.value) {
      if (!g_strncasecmp(organizer.value, "mailto:", 7)) {
        organizer.value += 7;
      }
      rb_iv_set(ruby_appt, "@organizer", rb_str_new2(organizer.value?organizer.value: ""));
    }

    ECalComponentTransparency transp;
    e_cal_component_get_transparency(ev_appt, &transp);
    // This should probably be an int or enum or some such rather than just a true/false
    rb_iv_set(ruby_appt, "@busy_status", transp == E_CAL_COMPONENT_TRANSP_OPAQUE ? Qtrue : Qfalse);

    rb_iv_set(ruby_appt, "@alarm_set", e_cal_component_has_alarms(ev_appt) ? Qtrue : Qfalse);
    rb_iv_set(ruby_appt, "@recurring", e_cal_component_has_recurrences(ev_appt) ? Qtrue : Qfalse);
}

/*
 *  call-seq:
 *    delete_appointment(uid)
 *
 * Deletes an appointment from the Evolution calendar.
 */
static VALUE revolution_delete_appointment(VALUE self, VALUE rb_uid) {
  char* uid = RSTRING(StringValue(rb_uid))->ptr;
  GError* error = NULL;
  e_cal_remove_object(open_calendar(self), uid, &error);
  check_error(error, "Unable to remove calendar item: %s");
  return Qnil;
}

/*
 *  call-seq:
 *    add_appointment(Appointment)
 *
 * Adds a new appointment to the Evolution calendar.
 */
static VALUE revolution_add_appointment(VALUE self, VALUE rb_appt) {
  VALUE rb_start = rb_iv_get(rb_appt, "@start");
  VALUE rb_end = rb_iv_get(rb_appt, "@end");
  if (rb_start == Qnil || rb_end == Qnil) {
    rb_raise(rb_cRevolutionException, "", "Must supply both a start and an end time in the Appointment object passed to Revolution.add_appointment");
  }

  ECalComponent* e_appt = e_cal_component_new();
  e_cal_component_set_new_vtype(e_appt, E_CAL_COMPONENT_EVENT);

  ECalComponentText* summary = g_new(ECalComponentText, 1);
  VALUE rb_summary = rb_iv_get(rb_appt, "@summary");
  if (rb_summary != Qnil) {
    summary->value = RSTRING(StringValue(rb_summary))->ptr;
    summary->altrep = "";
  }
  e_cal_component_set_summary(e_appt, summary);

  ECalComponentDateTime dt;
  struct icaltimetype itt = icaltime_from_timet_with_zone(add_offset(rb_start), 0, icaltimezone_get_utc_timezone());
  dt.value = &itt;
  dt.tzid = "UTC";
  e_cal_component_set_dtstart(e_appt, &dt);

  // Is it ok to just reuse the ECalComponentDateTime like I'm doing here?
  // Probably, as long as I fill in all the struct fields
  itt = icaltime_from_timet_with_zone(add_offset(rb_end), 0, icaltimezone_get_utc_timezone());
  dt.value = &itt;
  e_cal_component_set_dtend(e_appt, &dt);

  ECalComponentOrganizer organizer = {NULL, NULL, NULL, NULL};
  VALUE rb_organizer = rb_iv_get(rb_appt, "@organizer");
  if (rb_organizer != Qnil) {
    char* value = RSTRING(StringValue(rb_organizer))->ptr;
    organizer.value = value;
    organizer.cn = value;
    e_cal_component_set_organizer(e_appt, &organizer);
  }

  char* uid;
  GError* error = 0;
  e_cal_component_commit_sequence(e_appt);
  e_cal_create_object(open_calendar(self), e_cal_component_get_icalcomponent (e_appt), &uid, &error);
  check_error(error, "Unable to create calendar item: %s");
  return rb_str_new2(uid);
}

/*
 *  call-seq:
 *  get_all_appointments() -> Array
 *
 * Fetches all appointments.
 */
static VALUE revolution_get_all_appointments(VALUE self) {
  GError* error = 0;
  GList* appts, *l;
  e_cal_get_object_list_as_comp(open_calendar(self), "#t", &appts, &error);
  check_error(error, "Unable to query calendar %s");
  VALUE result = rb_ary_new();
  for (l = appts; l;l = l->next) {
    ECalComponent *ev_appt = E_CAL_COMPONENT(l->data);
    VALUE ruby_appt = rb_class_new_instance(0, 0, rb_cAppointment);
    copy_into_appt(ruby_appt, ev_appt);
    rb_ary_push(result, ruby_appt);
    g_object_unref(ev_appt);
  }
  g_list_free(appts);
  return result;
}

/*
 *  call-seq:
 *  get_appointments_between(start, end) -> Array
 *
 * Fetches all appointments between the given Time values.
 */
static VALUE revolution_get_appointments_between(VALUE self, VALUE rb_start, VALUE rb_end) {
  GError* error = 0;
  GList* appts, *l;
  VALUE start_fmt = rb_funcall(rb_start, rb_intern("strftime"), 1, rb_str_new2("%Y%m%dT%H%M%SZ"));
  char* start_str = RSTRING(StringValue(start_fmt))->ptr;
  VALUE end_fmt = rb_funcall(rb_end, rb_intern("strftime"), 1, rb_str_new2("%Y%m%dT%H%M%SZ"));
  char* end_str = RSTRING(StringValue(end_fmt))->ptr;
  char* query = g_strconcat("(occur-in-time-range? (make-time \"", start_str, "\") (make-time \"", end_str, "\"))", NULL);
  e_cal_get_object_list_as_comp(open_calendar(self), query, &appts, &error);
  check_error(error, "Unable to query calendar %s");
  VALUE result = rb_ary_new();
  for (l = appts; l;l = l->next) {
    ECalComponent *ev_appt = E_CAL_COMPONENT(l->data);
    VALUE ruby_appt = rb_class_new_instance(0, 0, rb_cAppointment);
    copy_into_appt(ruby_appt, ev_appt);
    rb_ary_push(result, ruby_appt);
    g_object_unref(ev_appt);
  }
  g_list_free(appts);
  return result;
}

/*
 *call-seq:
 *get_appointment_by_uid(appt_uid)
 *
 * Gets a single appointment using its unique id
 */
static VALUE revolution_get_appointment_by_uid(VALUE self, VALUE appt_uid) {
  char* uid = RSTRING(StringValue(appt_uid))->ptr;
  GError* error = NULL;
  GList* appt;
  char* query = g_strconcat("(uid? \"", uid, "\")", NULL);
  e_cal_get_object_list_as_comp(open_calendar(self), query, &appt, &error);
  check_error(error, "Unable to query calendar %s");
  if (!appt) {
    rb_raise(rb_cRevolutionException, "Unable to find that UID");
  }
  ECalComponent *ev_appt = E_CAL_COMPONENT(appt->data);
  VALUE rb_appt = rb_class_new_instance(0, 0, rb_cAppointment);
  copy_into_appt(rb_appt, ev_appt);
  g_object_unref(ev_appt);
  g_list_free(appt);
  return rb_appt;
}

/*
 *  call-seq:
 *    get_appointments_with_query(query)
 *
 * Gets appointments with an arbitrary s-expression query,
 * e.g., (occur-in-time-range? (make-time "20050302T000000Z") (make-time "20050305T000000Z"))
 * return any appointments in the specified time range
 *
 * See  calendar/backends/file/e-cal-backend-file.c, line 1372, for example of time_t to iso date
 * See  line 1007 of the same file for various query types
 */
static VALUE revolution_get_appointments_with_query(VALUE self, VALUE rb_query) {
  GError* error = 0;
  GList* appts, *l;
  e_cal_get_object_list_as_comp(open_calendar(self), RSTRING(StringValue(rb_query))->ptr, &appts, &error);
  check_error(error, "Unable to query calendar %s");
  VALUE result = rb_ary_new();
  for (l = appts; l;l = l->next) {
    ECalComponent *ev_appt = E_CAL_COMPONENT(l->data);
    VALUE ruby_appt = rb_class_new_instance(0, 0, rb_cAppointment);
    copy_into_appt(ruby_appt, ev_appt);
    rb_ary_push(result, ruby_appt);
    g_object_unref(ev_appt);
  }
  g_list_free(appts);
  return result;
}


static VALUE calendar_to_ical(ECal* cal) {
  VALUE rb_result;
  GError* error = 0;
  GList* appts, *l;
  char * str;
  rb_result = rb_str_new2("BEGIN:VCALENDAR\nCALSCALE:GREGORIAN\nPRODID:-//Ximian//NONSGML Evolution Calendar//EN\nVERSION:2.0\n");
  e_cal_get_object_list_as_comp(cal, "#t", &appts, &error);
  check_error(error, "Unable to query calendar %s");
  for (l = appts; l;l = l->next) {
	ECalComponent *ev_appt = E_CAL_COMPONENT(l->data);
	e_cal_component_commit_sequence(ev_appt);
	str=e_cal_component_get_as_string(ev_appt);
    rb_str_cat(rb_result, str, strlen(str));
	g_free(str);
	g_object_unref(ev_appt);
  }
  rb_str_concat(rb_result,rb_str_new2("END:VCALENDAR\n"));
  g_list_free(appts);
  g_object_unref(cal);
  return rb_result;
}

static VALUE revolution_get_calendar_ical(VALUE self) {
	return calendar_to_ical(open_calendar(self));
}

static VALUE revolution_get_tasks_ical(VALUE self) {
	return calendar_to_ical(open_tasks(self));
}

///////////////////////////////////////////////////////
// Address Book
/*
 *  call-seq:
 *    new() -> Contact
 *
 * Creates a new Contact object
 *
 * Attributes:
 *  @uid [String] the unique ID for this contact
 *  @first_name [String]
 *  @last_name [String]
 *  @email_addresses [Hash] A Hash of type->Array of addresses, i.e. 'HOME'->['tom@home.com', 'tom@bar.com']
 *  @birthday [Time]
 *  @home_phone [String]
 *  @work_phone [String]
 *  @mobile_phone [String]
 *  @home_address [ContactAddress]
 *  @work_address [ContactAddress]
 *  @other_address [ContactAddress]
 *  @organization [String]
 *  @title [String]
 *  @im_addresses [Array] An Array of ContactIMAddress objects
 */
static VALUE contact_init(VALUE self) {
  rb_iv_set(self, "@im_addresses", rb_ary_new());
  return Qtrue;
}

/*
 *  call-seq:
 *    new() -> ContactIMAddress
 *
 * Creates a new ContactIMAddress object
 *
 * Attributes:
 *  @provider [String] i.e., AOL, Yahoo
 *  @location [String] HOME or WORK
 *  @address [String] i.e., tom_copeland
 */
static VALUE contactIMAddress_init(VALUE self, VALUE provider, VALUE location, VALUE address) {
  rb_iv_set(self, "@provider", provider);
  rb_iv_set(self, "@location", location);
  rb_iv_set(self, "@address", address);
  return Qtrue;
}

/*
 *  call-seq:
 *    new() -> ContactAddress
 *
 * Creates a new ContactAddress object
 *
 * Attributes:
 *  @address_format [String]
 *  @po [String] P.O. Box
 *  @ext [String]
 *  @street [String]
 *  @locality [String]
 *  @region [String]
 *  @code [String]
 *  @country [String]
 */
static VALUE contactAddress_init(VALUE self) {
  return Qtrue;
}

static EBook* open_book() {
  if (!book) {
    GError* error = 0;
    book = e_book_new_default_addressbook(&error);
    check_error(error, "Unable to locate the default Evolution address book: %s");
    e_book_open(book, TRUE, &error);
    check_error(error, "Unable to open the Evolution address book: %s");
    if (!book) {
      rb_raise(rb_cRevolutionException, "Unable to open EBook");
    }
  }
  return book;
}

void address_importer(EContactAddress* address, VALUE ruby_contact, const char* field) {
  if (address) {
    VALUE rb_addr = rb_class_new_instance(0, 0, rb_cContactAddress);
    rb_iv_set(rb_addr, "@address_format", address->address_format ? rb_str_new2(address->address_format) : Qnil);
    rb_iv_set(rb_addr, "@po", address->po ? rb_str_new2(address->po) : Qnil);
    rb_iv_set(rb_addr, "@ext", address->ext ? rb_str_new2(address->ext) : Qnil);
    rb_iv_set(rb_addr, "@street", address->street ? rb_str_new2(address->street) : Qnil);
    rb_iv_set(rb_addr, "@locality", address->locality ? rb_str_new2(address->locality) : Qnil);
    rb_iv_set(rb_addr, "@region", address->region ? rb_str_new2(address->region) : Qnil);
    rb_iv_set(rb_addr, "@code", address->code ? rb_str_new2(address->code) : Qnil);
    rb_iv_set(rb_addr, "@country", address->country ? rb_str_new2(address->country) : Qnil);
    rb_iv_set(ruby_contact, field, rb_addr);
    e_contact_address_free(address);
  }
}

void string_importer(const VALUE ruby_contact, EContact* ev_contact, const char* ruby_iv_name, int ev_field) {
  char* value = e_contact_get(ev_contact, ev_field);
  rb_iv_set(ruby_contact, ruby_iv_name, value ? rb_str_new2(value) : Qnil);
}

void date_importer(const VALUE ruby_contact, EContact* ev_contact, const char* ruby_iv_name, int ev_field) {
  EContactDate* date = e_contact_get(ev_contact, ev_field);
  if (date) {
    rb_iv_set(ruby_contact, ruby_iv_name, rb_funcall(rb_cTime, rb_intern("gm"), 6, INT2NUM(date->year), INT2NUM(date->month), INT2NUM(date->day), INT2NUM(0), INT2NUM(0), INT2NUM(0)));
    e_contact_date_free(date);
  }
}

void email_importer(VALUE ruby_contact, EContact* ev_contact) {
  VALUE email_addresses = rb_hash_new();
  GList* attrs = e_contact_get_attributes(ev_contact, E_CONTACT_EMAIL), *attr = NULL;
  for (attr = attrs; attr; attr = attr->next) {
    GList* l;
    for (l = e_vcard_attribute_get_params(attr->data); l; l = l->next) {
      if (!g_ascii_strcasecmp((char*)e_vcard_attribute_param_get_name(l->data), "TYPE")) {
        VALUE rb_addr_type = rb_str_new2(e_vcard_attribute_param_get_values(l->data)->data);
        if (rb_hash_aref(email_addresses, rb_addr_type) == Qnil) {
          rb_hash_aset(email_addresses, rb_addr_type, rb_ary_new());
        }
        rb_ary_push(rb_hash_aref(email_addresses, rb_addr_type), rb_str_new2(e_vcard_attribute_get_value(attr->data)));
      }
    }
  }
  rb_iv_set(ruby_contact, "@email_addresses", email_addresses);
}

void im_importer(const VALUE ruby_contact, const char* address, const char* provider, const char* location) {
  if (!address) {
    return;
  }
  VALUE args[3] = {rb_str_new2(provider), rb_str_new2(location), rb_str_new2(address)};
  VALUE rb_addr = rb_class_new_instance(3, args, rb_cContactIMAddress);
  VALUE ary = rb_iv_get(ruby_contact, "@im_addresses");
  rb_ary_push(ary, rb_addr);
  rb_iv_set(ruby_contact, "@im_addresses", ary);
}

GList* run_query(EBook* book, EBookQuery* query) {
  GError* error = 0;
  GList* results = NULL;
  e_book_get_contacts(book, query, &results, &error);
  check_error(error, "Unable to query the Evolution address book: %s");
  return results;
}

VALUE copy_contacts(GList* results) {
  VALUE result = rb_ary_new();
  GList* l = NULL;
  for (l = results; l; l = l->next) {
    EContact* ev_contact = E_CONTACT(l->data);
    VALUE ruby_contact = rb_class_new_instance(0, 0, rb_cContact);
    // last mod time will work in v2.16+ :  e_contact_get (ev_contact, E_CONTACT_REV));
    // what's the best way to handle version differences?  #ifdef them in?
    string_importer(ruby_contact, ev_contact, "@uid", E_CONTACT_UID);
    string_importer(ruby_contact, ev_contact, "@first_name", E_CONTACT_GIVEN_NAME);
    string_importer(ruby_contact, ev_contact, "@last_name", E_CONTACT_FAMILY_NAME);
    string_importer(ruby_contact, ev_contact, "@home_phone", E_CONTACT_PHONE_HOME);
    string_importer(ruby_contact, ev_contact, "@work_phone", E_CONTACT_PHONE_BUSINESS);
    string_importer(ruby_contact, ev_contact, "@mobile_phone", E_CONTACT_PHONE_MOBILE);
    string_importer(ruby_contact, ev_contact, "@organization", E_CONTACT_ORG);
    string_importer(ruby_contact, ev_contact, "@title", E_CONTACT_TITLE);
    date_importer(ruby_contact, ev_contact, "@birthday", E_CONTACT_BIRTH_DATE);
    email_importer(ruby_contact, ev_contact);
    address_importer(e_contact_get(ev_contact, E_CONTACT_ADDRESS_HOME), ruby_contact, "@home_address");
    address_importer(e_contact_get(ev_contact, E_CONTACT_ADDRESS_WORK), ruby_contact, "@work_address");
    address_importer(e_contact_get(ev_contact, E_CONTACT_ADDRESS_OTHER), ruby_contact, "@other_address");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_AIM_HOME_1), "AIM", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_AIM_WORK_1), "AIM", "WORK");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_YAHOO_HOME_1), "Yahoo", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_YAHOO_WORK_1), "Yahoo", "WORK");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_GROUPWISE_HOME_1), "Groupwise", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_GROUPWISE_WORK_1), "Groupwise", "WORK");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_JABBER_HOME_1), "Jabber", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_JABBER_WORK_1), "Jabber", "WORK");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_MSN_HOME_1), "MSN", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_MSN_WORK_1), "MSN", "WORK");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_ICQ_HOME_1), "ICQ", "HOME");
    im_importer(ruby_contact, e_contact_get(ev_contact, E_CONTACT_IM_ICQ_WORK_1), "ICQ", "WORK");
    rb_ary_push(result, ruby_contact);
    g_object_unref(ev_contact);
  }
  g_list_free(results);
  return result;
}

/*
 *  call-seq:
 *    get_all_contacts() -> Array
 *
 * Fetches all contacts
 */
static VALUE revolution_get_all_contacts(VALUE self) {
  EBookQuery* query = e_book_query_any_field_contains("");
  VALUE result = copy_contacts(run_query(open_book(), query));
  e_book_query_unref(query);
  return result;
}

/*
 *  call-seq:
 *  get_all_contact_uids() -> Array
 *
 * Fetches all contact unique IDs
 */
static VALUE revolution_get_all_contact_uids(VALUE self) {
  EBookQuery* query = e_book_query_any_field_contains("");
  VALUE result = rb_ary_new();
  GList* results = run_query(open_book(), query), *l;
  for (l = results; l; l = l->next) {
    EContact* ev_contact = E_CONTACT(l->data);
    VALUE value = rb_str_new2(e_contact_get(ev_contact, E_CONTACT_UID));
    rb_ary_push(result, value);
    g_object_unref(ev_contact);
  }
  g_list_free(results);
  e_book_query_unref(query);
  return result;
}

/*
 *  call-seq:
 *    get_contacts_with_query(query)
 *
 * Gets contacts with an arbitrary s-expression query, i.e., (contains "full_name" "Smith") would
 * return any contacts whose named contained "Smith" - Joe Smith, Fred Smithson, etc.
 */
static VALUE revolution_get_contacts_with_query(VALUE self, VALUE rb_query) {
  EBookQuery* query = e_book_query_from_string(RSTRING(StringValue(rb_query))->ptr);
  VALUE result = copy_contacts(run_query(open_book(), query));
  e_book_query_unref(query);
  return result;
}

/*
 *  call-seq:
 *    get_contacts_by_name(name)
 *
 * Gets contacts by name; returns any name that contains the passed in parameter
 */
static VALUE revolution_get_contacts_by_name(VALUE self, VALUE rb_name) {
  EBookQuery* query = e_book_query_from_string(g_strdup_printf ("(contains \"full_name\" \"%s\")", RSTRING(StringValue(rb_name))->ptr));
  VALUE result = copy_contacts(run_query(open_book(), query));
  e_book_query_unref(query);
  return result;
}

/*
 *  call-seq:
 *    get_contact_by_uid(contact_uid)
 *
 * Gets a single contact using its unique id
 */
static VALUE revolution_get_contact_by_uid(VALUE self, VALUE contact_uid) {
  // Doing an 'is uid foobar' query doesn't work on Ev 2.2.2, more info is here:
  // EBookQuery* query = e_book_query_from_string(g_strdup_printf ("(is \"id\" \"%s\")", uid));
  // http://lists.ximian.com/pipermail/evolution-hackers/2005-June/005958.html
  // and the bug is fixed as per here:
  // http://bugzilla.gnome.org/show_bug.cgi?id=309276
  // but using e_book_get_contact works fine too
  char* uid = RSTRING(StringValue(contact_uid))->ptr;
  GError* error = NULL;
  EContact* ev_contact = e_contact_new();
  e_book_get_contact(open_book(), uid, &ev_contact, &error);
  check_error(error, "Unable to load that UID from the Evolution address book: %s");
  GList *tmp = g_list_append(NULL, ev_contact);
  return rb_ary_pop(copy_contacts(tmp));
}

/*
 *  call-seq:
 *    delete_contact(contact_uid)
 *
 * Deletes a contact
 */
static VALUE revolution_delete_contact(VALUE self, VALUE contact_uid) {
  GError* error = 0;
  e_book_remove_contact(open_book(), RSTRING(StringValue(contact_uid))->ptr, &error);
  check_error(error, "Unable to delete contact: %s");
  return Qnil;
}

void export_email_addresses(VALUE email_addresses, GList** email_attrs, const char* type) {
  VALUE addresses = rb_hash_aref(email_addresses, rb_str_new2(type));
  if (addresses != Qnil) {
    addresses = rb_funcall(addresses, rb_intern("reverse"), 0);
    VALUE rb_addr = Qnil;
    while ((rb_addr = rb_ary_pop(addresses)) != Qnil) {
      char* addr = RSTRING(StringValue(rb_addr))->ptr;
      EVCardAttribute* attr = e_vcard_attribute_new (NULL, EVC_EMAIL);
      e_vcard_attribute_add_value(attr, addr);
      e_vcard_attribute_add_param_with_value(attr, e_vcard_attribute_param_new (EVC_TYPE), type);
      *email_attrs = g_list_append(*email_attrs, attr);
    }
  }
}

void string_exporter(const VALUE rb_contact, EContact* ev_contact, const char* ruby_iv_name, int ev_field) {
  VALUE value = rb_iv_get(rb_contact, ruby_iv_name);
  if (value != Qnil) {
    e_contact_set(ev_contact, ev_field, RSTRING(StringValue(value))->ptr);
  }
}

void name_exporter(const VALUE rb_contact, EContact* ev_contact) {
  EContactName* name = e_contact_name_new ();
  VALUE fname = rb_iv_get(rb_contact, "@first_name");
  if (fname) {
    name->given = RSTRING(StringValue(fname))->ptr;
  }
  VALUE lname = rb_iv_get(rb_contact, "@last_name");
  if (lname) {
    name->family = RSTRING(StringValue(lname))->ptr;
  }
  e_contact_set(ev_contact, E_CONTACT_FULL_NAME, e_contact_name_to_string (name));
  // Hm, this causes problems... if I leave it here it segfaults,
  // and if I move to down to after the contact commit call
  // GLib complains about a double free
  //e_contact_name_free(name);
}

void date_exporter(const VALUE rb_contact, EContact* ev_contact, const char* ruby_iv_name, int ev_field) {
  VALUE date = rb_iv_get(rb_contact, ruby_iv_name);
  if (date != Qnil) {
    EContactDate* d = e_contact_date_new();
    d->year = NUM2INT(rb_funcall(date, rb_intern("year"), 0));
    d->month = NUM2INT(rb_funcall(date, rb_intern("mon"), 0));
    d->day = NUM2INT(rb_funcall(date, rb_intern("day"), 0));
    e_contact_set(ev_contact, ev_field, d);
  }
}

void email_exporter(const VALUE rb_contact, EContact* ev_contact) {
  VALUE email_addresses = rb_iv_get(rb_contact, "@email_addresses");
  if (email_addresses != Qnil) {
    GList* email_attrs = NULL;
    export_email_addresses(email_addresses, &email_attrs, "WORK");
    export_email_addresses(email_addresses, &email_attrs, "HOME");
    export_email_addresses(email_addresses, &email_attrs, "OTHER");
    if (g_list_length(email_attrs) > 0) {
      e_contact_set_attributes (ev_contact, E_CONTACT_EMAIL, email_attrs);
    }
    g_list_free(email_attrs);
  }
}

void im_exporter(const VALUE rb_contact, EContact* ev_contact) {
  VALUE im_addresses = rb_iv_get(rb_contact, "@im_addresses");
  if (im_addresses != Qnil) {
    VALUE rb_addr = Qnil;
    while ((rb_addr = rb_ary_pop(im_addresses)) != Qnil) {
      VALUE prov = rb_iv_get(rb_addr, "@provider");
      char* provider = RSTRING(StringValue(prov))->ptr;
      VALUE loc = rb_iv_get(rb_addr, "@location");
      char* location = RSTRING(StringValue(loc))->ptr;
      VALUE ad = rb_iv_get(rb_addr, "@address");
      char* addr = RSTRING(StringValue(ad))->ptr;
      e_contact_set(ev_contact, GPOINTER_TO_INT(g_hash_table_lookup(im_hash, g_strconcat(provider, location, NULL))), addr);
    }
  }
}

/*
 *  call-seq:
 *    add_contact(contact) -> [String] the new contact's Evolution unique ID
 *
 * Adds a contact
 */
static VALUE revolution_add_contact(VALUE self, VALUE rb_contact) {
  EContact* ev_contact = e_contact_new();
  name_exporter(rb_contact, ev_contact);
  string_exporter(rb_contact, ev_contact, "@home_phone", E_CONTACT_PHONE_HOME);
  string_exporter(rb_contact, ev_contact, "@work_phone", E_CONTACT_PHONE_BUSINESS);
  string_exporter(rb_contact, ev_contact, "@mobile_phone", E_CONTACT_PHONE_MOBILE);
  string_exporter(rb_contact, ev_contact, "@organization", E_CONTACT_ORG);
  string_exporter(rb_contact, ev_contact, "@title", E_CONTACT_TITLE);
  date_exporter(rb_contact, ev_contact, "@birthday", E_CONTACT_BIRTH_DATE);
  email_exporter(rb_contact, ev_contact);
  im_exporter(rb_contact, ev_contact);
  GError* error = 0;
  e_book_add_contact(open_book(), ev_contact, &error);
  check_error(error, "Unable to add contact: %s");
  return rb_str_new2(e_contact_get(ev_contact, E_CONTACT_UID));
}


//////////////////////////////////////////////////
// ESourceGroup
//

static VALUE e_source_init(VALUE self) {
  return Qtrue;
}


static void copy_into_esgroup(VALUE rb_group, ESourceGroup* group) {
  rb_iv_set(rb_group, "@uid", rb_str_new2(e_source_group_peek_uid(group)));
  rb_iv_set(rb_group, "@name", rb_str_new2(e_source_group_peek_name(group)));
  rb_iv_set(rb_group, "@base_uri", rb_str_new2(e_source_group_peek_base_uri(group)));
  rb_iv_set(rb_group, "@read_only", e_source_group_get_readonly(group) ? Qtrue : Qfalse);
}

//////////////////////////////////////////////////
//ESource
//
static VALUE esource_group_init(VALUE self) {
  return Qtrue;
}

static void copy_into_esource(VALUE rb_source, ESource* source){
  gboolean has_color;
  guint32 color;
  rb_iv_set(rb_source, "@uid", rb_str_new2(e_source_peek_uid(source)));
  rb_iv_set(rb_source, "@name", rb_str_new2(e_source_peek_name(source)?e_source_peek_name(source):""));
  rb_iv_set(rb_source, "@uri", rb_str_new2(e_source_get_uri(source)?e_source_get_uri(source):""));
  rb_iv_set(rb_source, "@absolute_uri", rb_str_new2(e_source_peek_absolute_uri(source)?e_source_peek_absolute_uri(source):""));
  rb_iv_set(rb_source, "@relative_uri", rb_str_new2(e_source_peek_relative_uri(source)?e_source_peek_relative_uri(source):""));
  rb_iv_set(rb_source, "@read_only", e_source_get_readonly(source) ? Qtrue : Qfalse);
  has_color = e_source_get_color (source, &color);
  if (has_color) {
    char *color_string = g_strdup_printf (COLOR_FORMAT_STRING, color);
    rb_iv_set(rb_source, "@color", rb_str_new2(color_string));
    g_free (color_string);
  }
  else {
    rb_iv_set(rb_source, "@color", Qnil);
  }
}


//////////////////////////////////////////////////
// Sources related functions
//
//

VALUE copy_source_list(ESourceList* list) {
  GSList *p, *s;
  VALUE hash = rb_hash_new();
  for (p = e_source_list_peek_groups(list); p != NULL; p = p->next) {	
    ESourceGroup *group = E_SOURCE_GROUP (p->data);
    VALUE rb_sources=rb_ary_new();
    VALUE rb_group=rb_class_new_instance(0, 0, rb_cESourceGroup);
    for(s = e_source_group_peek_sources (group); s != NULL; s = s->next) {
      if(s->data) {			
        ESource *source =  E_SOURCE (s->data);			
        VALUE rb_source=rb_class_new_instance(0, 0, rb_cESource);
        copy_into_esource(rb_source, source);
        rb_ary_push(rb_sources, rb_source);			         			
      }			
    }
    copy_into_esgroup(rb_group, group);
	rb_hash_aset(hash, rb_group, rb_sources);
  }
  return hash;
}

static VALUE get_cal_sources(VALUE self, ECalSourceType type) {
  ESourceList* list;
  GError* error = 0;
  e_cal_get_sources(&list, type, &error);
  check_error(error, "Unable to retrieve calendar/tasks sources %s");
  VALUE hash = copy_source_list(list);
  g_object_unref(list);
  return hash;
}

static VALUE revolution_get_all_calendar_sources(VALUE self) {
  return get_cal_sources(self, E_CAL_SOURCE_TYPE_EVENT);
}

static VALUE revolution_get_all_tasks_sources(VALUE self) {
  return get_cal_sources(self, E_CAL_SOURCE_TYPE_TODO);
}

static VALUE revolution_get_all_addressbook_sources(VALUE self) {
  ESourceList* list;
  GError* error = 0;
  VALUE hash = rb_hash_new();
  e_book_get_addressbooks(&list,  &error);
  check_error(error, "Unable to retrieve addressbook sources %s");
  hash = copy_source_list(list);
  g_object_unref(list);
  return hash;
}

/*
 * An interface to the Evolution[http://www.gnome.org/projects/evolution/]
 * calendar, contact, and task information.
 */
void Init_revolution() {
  rb_mRevolution = rb_define_module("Revolution");

  rb_cContact = rb_define_class_under(rb_mRevolution, "Contact", rb_cObject);
  rb_define_attr(rb_cContact, "uid", 1, 1);
  rb_define_attr(rb_cContact, "first_name", 1, 1);
  rb_define_attr(rb_cContact, "last_name", 1, 1);
  rb_define_attr(rb_cContact, "home_email", 1, 1);
  rb_define_attr(rb_cContact, "work_email", 1, 1);
  rb_define_attr(rb_cContact, "email_addresses", 1, 1);
  rb_define_attr(rb_cContact, "birthday", 1, 1);
  rb_define_attr(rb_cContact, "home_phone", 1, 1);
  rb_define_attr(rb_cContact, "work_phone", 1, 1);
  rb_define_attr(rb_cContact, "mobile_phone", 1, 1);
  rb_define_attr(rb_cContact, "home_address", 1, 1);
  rb_define_attr(rb_cContact, "work_address", 1, 1);
  rb_define_attr(rb_cContact, "other_address", 1, 1);
  rb_define_attr(rb_cContact, "organization", 1, 1);
  rb_define_attr(rb_cContact, "title", 1, 1);
  rb_define_attr(rb_cContact, "im_addresses", 1, 1);
  rb_define_method(rb_cContact, "initialize", contact_init, 0);
  rb_cContactAddress = rb_define_class_under(rb_mRevolution, "ContactAddress", rb_cObject);
  rb_define_attr(rb_cContactAddress, "address_format", 1, 1);
  rb_define_attr(rb_cContactAddress, "po", 1, 1);
  rb_define_attr(rb_cContactAddress, "ext", 1, 1);
  rb_define_attr(rb_cContactAddress, "street", 1, 1);
  rb_define_attr(rb_cContactAddress, "locality", 1, 1);
  rb_define_attr(rb_cContactAddress, "region", 1, 1);
  rb_define_attr(rb_cContactAddress, "code", 1, 1);

  rb_cESourceGroup = rb_define_class_under(rb_mRevolution, "ESourceGroup", rb_cObject);
  rb_define_attr(rb_cESourceGroup, "uid", 1, 1);
  rb_define_attr(rb_cESourceGroup, "name", 1, 1);
  rb_define_attr(rb_cESourceGroup, "base_uri", 1, 1);
  rb_define_attr(rb_cESourceGroup, "readonly", 1, 1);
  rb_define_method(rb_cESourceGroup, "initialize", esource_group_init , 0);

  rb_cESource = rb_define_class_under(rb_mRevolution, "ESource", rb_cObject);
  rb_define_attr(rb_cESource, "uid", 1, 1);
  rb_define_attr(rb_cESource, "name", 1, 1);
  rb_define_attr(rb_cESource, "uri", 1, 1);
  rb_define_attr(rb_cESource, "absolute_uri", 1, 1);
  rb_define_attr(rb_cESource, "relative_uri", 1, 1);
  rb_define_attr(rb_cESource, "color", 1, 1);
  rb_define_method(rb_cESource, "initialize", e_source_init , 0);
  rb_define_attr(rb_cContactAddress, "country", 1, 1);
  rb_define_module_function(rb_mRevolution, "get_all_calendar_sources", revolution_get_all_calendar_sources, 0);
  rb_define_module_function(rb_mRevolution, "get_all_tasks_sources", revolution_get_all_tasks_sources, 0);
  rb_define_module_function(rb_mRevolution, "get_all_addressbook_sources", revolution_get_all_addressbook_sources, 0);

  rb_define_method(rb_cContactAddress, "initialize", contactAddress_init, 0);
  rb_cContactIMAddress = rb_define_class_under(rb_mRevolution, "ContactIMAddress", rb_cObject);
  rb_define_attr(rb_cContactIMAddress, "provider", 1, 1);
  rb_define_attr(rb_cContactIMAddress, "location", 1, 1);
  rb_define_attr(rb_cContactIMAddress, "address", 1, 1);
  rb_define_method(rb_cContactIMAddress, "initialize", contactIMAddress_init, 3);

  rb_cAppointment = rb_define_class_under(rb_mRevolution, "Appointment", rb_cObject);
  rb_define_attr(rb_cAppointment, "uid", 1, 1);
  rb_define_attr(rb_cAppointment, "summary", 1, 1);
  rb_define_attr(rb_cAppointment, "location", 1, 1);
  rb_define_attr(rb_cAppointment, "organizer", 1, 1);
  rb_define_attr(rb_cAppointment, "start", 1, 1);
  rb_define_attr(rb_cAppointment, "end", 1, 1);
  rb_define_attr(rb_cAppointment, "last_modification", 1, 1);
  rb_define_attr(rb_cAppointment, "alarm_set", 1, 1);
  rb_define_attr(rb_cAppointment, "busy_status", 1, 1);
  rb_define_attr(rb_cAppointment, "recurring", 1, 1);
  rb_define_attr(rb_cAppointment, "all_day_event", 1, 1);
  rb_define_method(rb_cAppointment, "initialize", evAppointment_init, 0);
  // INLINE RUBY CODE define an "all_day" method

  rb_cTask = rb_define_class_under(rb_mRevolution, "Task", rb_cObject);
  rb_define_attr(rb_cTask, "uid", 1, 1);
  rb_define_attr(rb_cTask, "summary", 1, 1);
  rb_define_attr(rb_cTask, "description", 1, 1);
  rb_define_attr(rb_cTask, "start", 1, 1);

  rb_define_attr(rb_cTask, "due", 1, 1);
  rb_define_attr(rb_cTask, "status", 1, 1);
  rb_define_attr(rb_cTask, "priority", 1, 1);
  rb_define_attr(rb_cTask, "last_modification", 1, 1);
  rb_define_method(rb_cTask, "initialize", evTask_init, 0);

  rb_cRevolution = rb_define_class_under(rb_mRevolution, "Revolution", rb_cObject);
  rb_define_method(rb_cRevolution, "initialize", revolution_init, 0);

  rb_define_method(rb_cRevolution, "add_contact", revolution_add_contact, 1);
  rb_define_method(rb_cRevolution, "delete_contact", revolution_delete_contact, 1);
  rb_define_method(rb_cRevolution, "get_contact_by_uid", revolution_get_contact_by_uid, 1);
  rb_define_method(rb_cRevolution, "get_contacts_by_name", revolution_get_contacts_by_name, 1);
  rb_define_method(rb_cRevolution, "get_contacts_with_query", revolution_get_contacts_with_query, 1);
  rb_define_method(rb_cRevolution, "get_all_contacts", revolution_get_all_contacts, 0);
  rb_define_method(rb_cRevolution, "get_all_contact_uids", revolution_get_all_contact_uids, 0);

  rb_define_method(rb_cRevolution, "add_appointment", revolution_add_appointment, 1);
  rb_define_method(rb_cRevolution, "delete_appointment", revolution_delete_appointment, 1);
  rb_define_method(rb_cRevolution, "get_appointment_by_uid", revolution_get_appointment_by_uid, 1);
  rb_define_method(rb_cRevolution, "get_all_appointments", revolution_get_all_appointments, 0);
  rb_define_method(rb_cRevolution, "get_appointments_between", revolution_get_appointments_between, 2);
  rb_define_method(rb_cRevolution, "get_appointments_with_query", revolution_get_appointments_with_query, 1);

  rb_define_method(rb_cRevolution, "get_calendar_ical", revolution_get_calendar_ical, 0);
  rb_define_method(rb_cRevolution, "get_tasks_ical", revolution_get_tasks_ical, 0);

  rb_define_method(rb_cRevolution, "add_task", revolution_add_task, 1);
  rb_define_method(rb_cRevolution, "delete_task", revolution_delete_task, 1);
  rb_define_method(rb_cRevolution, "get_all_tasks", revolution_get_all_tasks, 0);

  rb_define_attr(rb_cRevolution, "calendar_source", 1, 1);
  rb_define_attr(rb_cRevolution, "tasks_source", 1, 1);
  rb_define_attr(rb_cRevolution, "addressbook_source", 1, 1);
  rb_define_method(rb_cRevolution, "initialize", revolution_init3, -1);

  rb_cRevolutionException = rb_define_class_under(rb_mRevolution, "RevolutionException", rb_eStandardError);

  g_type_init();

  im_hash = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_insert(im_hash, "AIMHOME", GINT_TO_POINTER(E_CONTACT_IM_AIM_HOME_1));
  g_hash_table_insert(im_hash, "AIMWORK", GINT_TO_POINTER(E_CONTACT_IM_AIM_WORK_1));
}


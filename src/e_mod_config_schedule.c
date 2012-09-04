#include <e.h>
#include <Elementary.h>
#include "e_mod_config.h"

//Round slider floating point minutes into interger.
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

//After clicking start work, notify the user he has 5min to prepair, real work will start
//after the DELAY_START_MIN time has passed;
#define DELAY_START_MIN 0
#define DEFAULT_WORK_HOURS 3
#define DEFAULT_MINIMUM_WORK_MIN 30; //TODO: not implemented!

static void _start_clock_cb(void *data, Evas_Object *obj, void *event_info);
static void _stop_clock_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_mod_config_schedule_start_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_stop_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_urgent_cb(void *data, Evas_Object *obj,
                                             void *event_info);
static void _e_mod_config_schedule_break_x_time_cb(void *data, Evas_Object *obj,
                                                   void *event_info);
static void _e_mod_config_schedule_break_y_time_cb(void *data, Evas_Object *obj,
                                                   void *event_info);
static void _e_mod_config_schedule_clock_fill_delay(E_Config_Schedule_Data *csd);
static void _e_mod_config_schedule_lock_update(E_Config_Schedule_Data *csd);
static void _e_mod_config_schedule_productivity_conf_update(Config *cfg,
                                                            E_Config_Dialog_Data *cfdata);
static void _e_mod_config_schedule_break_time_label_update(E_Config_Schedule_Data *csd);

static Eina_Bool e_mod_config_schedule_clock_fill_delay(void *data);
static Intervals _e_mod_config_schedule_intervals_conf_get(E_Config_Dialog_Data *cfdata);



Eina_Bool
e_mod_config_schedule_create_data(E_Config_Dialog_Data *cfdata)
{ 
   _e_mod_config_schedule_clock_fill_delay(&cfdata->schedule);
}

Evas_Object *
e_mod_config_schedule_new(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot;
   Evas_Object *bx, *label, *hbx;
   unsigned int digedit;

   ot = e_widget_table_add(evas, EINA_FALSE);

   bx = elm_box_add(ot);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_FILL, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   e_widget_table_object_append(ot, bx, 0, 0, 2, 1, 1, 1, 1, 1);
   evas_object_show(bx);

   label = elm_label_add(bx);
   elm_object_text_set(label, "Time to Start working");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   cfdata->schedule.start_clk = elm_clock_add(bx);
   elm_clock_edit_set(cfdata->schedule.start_clk, EINA_TRUE);
   digedit = ELM_CLOCK_EDIT_HOUR_UNIT | ELM_CLOCK_EDIT_MIN_UNIT | ELM_CLOCK_EDIT_SEC_UNIT;
   elm_clock_edit_mode_set(cfdata->schedule.start_clk, digedit);
   elm_clock_show_seconds_set(cfdata->schedule.start_clk, EINA_FALSE);
   elm_clock_show_am_pm_set(cfdata->schedule.start_clk, EINA_FALSE);
   elm_clock_time_set(cfdata->schedule.start_clk,
                      cfdata->schedule.start_time.hour,
                      cfdata->schedule.start_time.min,
                      cfdata->schedule.start_time.sec);
   evas_object_smart_callback_add(cfdata->schedule.start_clk,
                                  "changed", _start_clock_cb, &cfdata->schedule);
   elm_box_pack_end(bx, cfdata->schedule.start_clk);
   evas_object_show(cfdata->schedule.start_clk);

   label = elm_label_add(bx);
   elm_object_text_set(label, "Time to Stop working");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   cfdata->schedule.stop_clk = elm_clock_add(bx);
   elm_clock_edit_set(cfdata->schedule.stop_clk, EINA_TRUE);
   elm_clock_edit_mode_set(cfdata->schedule.stop_clk, digedit);
   elm_clock_show_seconds_set(cfdata->schedule.stop_clk, EINA_FALSE);
   elm_clock_show_am_pm_set(cfdata->schedule.stop_clk, EINA_FALSE);
   elm_clock_time_set(cfdata->schedule.stop_clk,
                      cfdata->schedule.stop_time.hour,
                      cfdata->schedule.stop_time.min,
                      cfdata->schedule.stop_time.sec);
   evas_object_smart_callback_add(cfdata->schedule.stop_clk,
                                  "changed", _stop_clock_cb, &cfdata->schedule);
   elm_box_pack_end(bx, cfdata->schedule.stop_clk);
   evas_object_show(cfdata->schedule.stop_clk);

   cfdata->schedule.urgent_chk = elm_check_add(bx);
   elm_object_text_set(cfdata->schedule.urgent_chk, "Allow Urgent Windows");
   elm_check_state_set(cfdata->schedule.urgent_chk, cfdata->schedule.urgent);
   elm_box_pack_end(bx, cfdata->schedule.urgent_chk);
   evas_object_show(cfdata->schedule.urgent_chk);
   evas_object_smart_callback_add(cfdata->schedule.urgent_chk,
                                  "changed", _e_mod_config_schedule_urgent_cb,
                                  cfdata);

   cfdata->schedule.label = elm_label_add(bx);
   _e_mod_config_schedule_break_time_label_update(&cfdata->schedule);
   evas_object_resize(cfdata->schedule.label, 200, 25);
   elm_box_pack_end(bx, cfdata->schedule.label);
   evas_object_show(cfdata->schedule.label);

   hbx = elm_box_add(bx);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   elm_box_pack_end(bx, hbx);
   evas_object_show(hbx);

   cfdata->schedule.break_x = elm_slider_add(hbx);
   elm_slider_unit_format_set(cfdata->schedule.break_x, "%1.0f Minutes");
   elm_slider_indicator_format_set(cfdata->schedule.break_x, "%1.0f");
   elm_slider_min_max_set(cfdata->schedule.break_x, 0, 30);
   elm_slider_value_set(cfdata->schedule.break_x,cfdata->schedule.break_min_x);
   elm_slider_span_size_set(cfdata->schedule.break_x, 60);
   elm_box_pack_end(hbx, cfdata->schedule.break_x);
   evas_object_show(cfdata->schedule.break_x);
   evas_object_smart_callback_add(cfdata->schedule.break_x,
                                  "delay,changed", _e_mod_config_schedule_break_x_time_cb,
                                  &cfdata->schedule);

   cfdata->schedule.break_y = elm_slider_add(hbx);
   elm_slider_unit_format_set(cfdata->schedule.break_y, "%1.0f Minutes");
   elm_slider_indicator_format_set(cfdata->schedule.break_y, "%1.0f");
   elm_slider_min_max_set(cfdata->schedule.break_y, 0, 180);
   elm_slider_value_set(cfdata->schedule.break_y,cfdata->schedule.break_min_y);
   elm_slider_span_size_set(cfdata->schedule.break_y, 160);
   elm_box_pack_end(hbx, cfdata->schedule.break_y);
   evas_object_show(cfdata->schedule.break_y);
   evas_object_smart_callback_add(cfdata->schedule.break_y,
                                  "delay,changed", _e_mod_config_schedule_break_y_time_cb,
                                  &cfdata->schedule);

   cfdata->schedule.start_btn = e_widget_button_add(evas, _("Test Start Working"), "list-add",
                                                    _e_mod_config_schedule_start_working_cb,
                                                    &cfdata->schedule, cfdata);

   e_widget_table_object_append(ot, cfdata->schedule.start_btn, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->schedule.stop_btn = e_widget_button_add(evas, _("Test Stop Working"), "list-remove",
                                                   _e_mod_config_schedule_stop_working_cb,
                                                   &cfdata->schedule, cfdata);
   e_widget_table_object_append(ot, cfdata->schedule.stop_btn, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_toolbook_page_append(otb, NULL, _("Schedule"), ot, 1, 1, 1, 1, 0.5, 0.0);

   _e_mod_config_schedule_lock_update(&cfdata->schedule);
}


Eina_Bool
e_mod_config_schedule_save_config(E_Config_Dialog_Data *cfdata)
{
   Month *m, mm;
   Day *d;
   Intervals *iv, new_iv;
   Eina_List *l, *ll, *lll;
   Eina_List *iv_list;
   Eina_Bool dm = EINA_FALSE;

   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);

   //Get start clock time
   elm_clock_time_get(cfdata->schedule.start_clk,
                      &cfdata->schedule.start_time.hour,
                      &cfdata->schedule.start_time.min,
                      &cfdata->schedule.start_time.sec);

   //Get stop clock time
   elm_clock_time_get(cfdata->schedule.stop_clk,
                      &cfdata->schedule.stop_time.hour,
                      &cfdata->schedule.stop_time.min,
                      &cfdata->schedule.stop_time.sec);

   //Get break min time
   cfdata->schedule.break_min_x = 
      round(elm_slider_value_get(cfdata->schedule.break_x));
   cfdata->schedule.break_min_y = 
      round(elm_slider_value_get(cfdata->schedule.break_y));

   cfdata->schedule.urgent = elm_check_state_get(cfdata->schedule.urgent_chk);

   productivity_conf->timestamp = e_mod_timestamp_get();

   EINA_LIST_FOREACH(productivity_conf->month_list, l, m)
     {
        if (tm->tm_mon == m->mon)
          {
             EINA_LIST_FOREACH(m->day_list, ll, d)
               {
                  if (d->mday == tm->tm_mday)
                    {
                       EINA_LIST_FOREACH(d->iv_list, lll, iv)
                         {
                            if (iv->id == cfdata->schedule.id)
                              {
                                 //Remove old list.
                                 /*INF("iv->lock:%d, cfdata->schedule.lock:%d",
                                   iv->lock, cfdata->schedule.lock);*/

                                 d->iv_list = eina_list_remove(d->iv_list, iv);
                              }
                         }

                       m->day.iv = _e_mod_config_schedule_intervals_conf_get(cfdata);
                       d->iv_list = eina_list_append(d->iv_list, 
                                                     &m->day.iv);
                       dm = EINA_TRUE;
                    }
               }
             if(dm == EINA_FALSE)
               {
                  char buf[16];

                  INF("Today is a new day, so we create a new day :)");
                  strftime(buf, 16, "%A", tm);
                  m->day.name = eina_stringshare_add(buf);
                  m->day.mday = tm->tm_mday;
                  m->day_list = eina_list_append(m->day_list, &m->day);
                  m->day.iv = _e_mod_config_schedule_intervals_conf_get(cfdata);
                  m->day.iv_list = eina_list_append(m->day.iv_list, &m->day.iv);
               }
          }
     }
   _e_mod_config_schedule_productivity_conf_update(productivity_conf, cfdata);
   e_mod_main_reload_config();
} 


static void
_start_clock_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;
   time_t tt;
   struct tm *tm;

   if(!(csd = data)) return;
   time(&tt);
   tm = localtime(&tt); 


   elm_clock_time_get(obj, &csd->start_time.hour,
                      &csd->start_time.min,
                      &csd->start_time.sec);
   /*
      CRI("START_REAL:%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
      CRI("START_SELE:%d:%d:%d",csd->start_time.hour, csd->start_time.min,
      csd->start_time.sec);
      */
   /*
      while(csd->start_time.hour < tm->tm_hour)
      {
      csd->start_time.hour++;
      }
      while(csd->start_time.min < tm->tm_min)
      {
      csd->start_time.min++;
      }
      while(csd->start_time.sec < tm->tm_sec)
      {
      csd->start_time.sec++;
      }*/

   elm_clock_time_set(obj, csd->start_time.hour, csd->start_time.min,
                      csd->start_time.sec);
}

static void 
_stop_clock_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;
   time_t tt;
   struct tm *tm;

   if(!(csd = data)) return;
   time(&tt);
   tm = localtime(&tt);

   elm_clock_time_get(obj, &csd->stop_time.hour,
                      &csd->stop_time.min,
                      &csd->stop_time.sec);
   /*
      CRI("STOP_REAL:%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
      CRI("STOP_SELE:%d:%d:%d",csd->stop_time.hour, csd->stop_time.min,
      csd->stop_time.sec);
      */
   /*
      while(csd->stop_time.hour < tm->tm_hour)
      {
      csd->stop_time.hour++;
      }
      if((csd->stop_time.min < tm->tm_min) && (csd->stop_time.hour <= tm->tm_hour))
      {
      while(csd->stop_time.min < tm->tm_min)
      {
      csd->stop_time.min++;
      }
      }
      while(csd->stop_time.sec < tm->tm_sec)
      {
      csd->stop_time.sec++;
      }*/

   elm_clock_time_set(obj, csd->stop_time.hour, csd->stop_time.min,
                      csd->stop_time.sec);

}

static void
_e_mod_config_schedule_start_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   E_Config_Dialog_Data *cfdata;
   unsigned int digedit;

   if(!(csd = data)) return;
   if(!(cfdata = data2)) return;

   if(e_widget_disabled_get(csd->stop_btn) == EINA_TRUE)
     {
        digedit = ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_x, EINA_TRUE);
        elm_object_disabled_set(csd->break_y, EINA_TRUE);
        if(csd->lock == EINA_FALSE)
          {
             csd->lock = EINA_TRUE;
          }
        if(productivity_conf->secs_to_break)
          productivity_conf->secs_to_break = 0;
     }
   _e_mod_config_schedule_lock_update(csd);

   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);
   productivity_conf->unhide = EINA_FALSE;
   e_mod_config_window_manager(productivity_conf->cwl);
   e_config_save_queue();
}

static void
_e_mod_config_schedule_stop_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   E_Config_Dialog_Data *cfdata;
   unsigned int digedit;

   if(!(csd = data)) return;
   if(!(cfdata = data2)) return;

   if(e_widget_disabled_get(csd->start_btn) == EINA_TRUE)
     {
        digedit = ELM_CLOCK_EDIT_HOUR_UNIT |
           ELM_CLOCK_EDIT_MIN_UNIT | ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_x, EINA_FALSE);
        elm_object_disabled_set(csd->break_y, EINA_FALSE);

        if(csd->lock == EINA_TRUE)
          {
             csd->lock = EINA_FALSE;
          }
     }
   _e_mod_config_schedule_lock_update(csd);

   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);
   productivity_conf->unhide = EINA_FALSE;
   e_mod_config_window_manager(productivity_conf->cwl);
   e_config_save_queue();
}

static void
_e_mod_config_schedule_urgent_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   if(!(cfdata = data)) return;

   cfdata->schedule.urgent = elm_check_state_get(obj);
   productivity_conf->cur_iv.urgent = cfdata->schedule.urgent;
   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);
   e_config_save_queue();
}

static void
_e_mod_config_schedule_break_x_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

   csd->break_min_x = round(elm_slider_value_get(obj));    
   _e_mod_config_schedule_break_time_label_update(csd);
}

static void
_e_mod_config_schedule_break_y_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

   csd->break_min_y = round(elm_slider_value_get(obj));
   _e_mod_config_schedule_break_time_label_update(csd);
}

static void
_e_mod_config_schedule_break_time_label_update(E_Config_Schedule_Data *csd)
{
   char buf[1024];

   snprintf(buf, sizeof(buf), "%d Minutes of Break for %d Minutes of Work",
            csd->break_min_x, csd->break_min_y);

   elm_object_text_set(csd->label, buf);
}

static Eina_Bool
e_mod_config_schedule_clock_fill_delay(void *data)
{
   E_Config_Dialog_Data *cfdata;

   if(!(cfdata = data)) return ECORE_CALLBACK_CANCEL;
   _e_mod_config_schedule_clock_fill_delay(&cfdata->schedule);

   cfdata->clock_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_config_schedule_clock_fill_delay(E_Config_Schedule_Data *csd)
{
   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);

   csd->id              = productivity_conf->cur_iv.id;
   csd->lock            = productivity_conf->cur_iv.lock;
   csd->urgent          = productivity_conf->cur_iv.urgent;
   csd->break_min_x     = productivity_conf->cur_iv.break_min_x;
   csd->break_min_y     = productivity_conf->cur_iv.break_min_y;

   csd->start_time.hour = productivity_conf->cur_iv.start.hour;
   csd->start_time.min  = productivity_conf->cur_iv.start.min;
   csd->start_time.sec  = productivity_conf->cur_iv.start.sec;

   csd->stop_time.hour  = productivity_conf->cur_iv.stop.hour;
   csd->stop_time.min   = productivity_conf->cur_iv.stop.min;
   csd->stop_time.sec   = productivity_conf->cur_iv.stop.sec;

   if(csd->lock == 1) return;
   /*
      DBG("\nID:%d, LOCK:%d, Break_X:%d, Break_Y:%d StartH:%d, StartM:%d, 
StartS:%d, StopH:%d, StopM:%d, StopS:%d",csd->id, csd->lock, csd->break_min_x,
csd->break_min_y,csd->start_time.hour, csd->start_time.min, csd->start_time.sec,
csd->stop_time.hour, csd->stop_time.min, csd->stop_time.sec);

csd->start_time.hour = tm->tm_hour;
csd->start_time.min  = tm->tm_min;
csd->start_time.sec  = tm->tm_sec;

   // Start Working initial time.
   if((tm->tm_min + DELAY_START_MIN) < 59)
   {
   csd->start_time.min += DELAY_START_MIN;
   }
   else
   {
   csd->start_time.min += DELAY_START_MIN - 59;
   csd->start_time.hour += 1;
   }
   */
   csd->stop_time.hour = tm->tm_hour;
   csd->stop_time.min  = tm->tm_min;
   csd->stop_time.sec  = tm->tm_sec;

   // Stop Working time, here we calculate the time we need to end work
   // using DEFAULT_WORK_HOURS
   if((tm->tm_hour + DEFAULT_WORK_HOURS) < 23)
     {
        csd->stop_time.hour += DEFAULT_WORK_HOURS;
     }
   else
     {
        csd->stop_time.hour = 23;
        csd->stop_time.min = 59;
        csd->stop_time.sec = 59;
     }
}

static void
_e_mod_config_schedule_lock_update(E_Config_Schedule_Data *csd)
{
   unsigned int digedit;
   //If lock = EINA_TRUE, stop button is enabled!
   if(csd->lock == EINA_TRUE)
     {
        e_widget_disabled_set(csd->stop_btn, EINA_FALSE);
        digedit = ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_x, EINA_TRUE);
        elm_object_disabled_set(csd->break_y, EINA_TRUE);
        elm_object_disabled_set(csd->urgent_chk, EINA_TRUE);

        if(e_widget_disabled_get(csd->start_btn) == EINA_FALSE)
          e_widget_disabled_set(csd->start_btn, EINA_TRUE);
     }
   else if (csd->lock == EINA_FALSE)
     {
        e_widget_disabled_set(csd->stop_btn, EINA_TRUE);
        digedit = ELM_CLOCK_EDIT_HOUR_UNIT |
           ELM_CLOCK_EDIT_MIN_UNIT | ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_x, EINA_FALSE);
        elm_object_disabled_set(csd->break_y, EINA_FALSE);
        elm_object_disabled_set(csd->urgent_chk, EINA_FALSE);

        if(e_widget_disabled_get(csd->start_btn) == EINA_TRUE)
          e_widget_disabled_set(csd->start_btn, EINA_FALSE);
     }
}

static void
_e_mod_config_schedule_productivity_conf_update(Config *cfg,
                                                E_Config_Dialog_Data *cfdata)
{
   cfg->iv.id           = cfdata->schedule.id;
   cfg->iv.lock         = cfdata->schedule.lock;
   cfg->iv.urgent       = cfdata->schedule.urgent;
   cfg->iv.break_min_x  = cfdata->schedule.break_min_x;
   cfg->iv.break_min_y  = cfdata->schedule.break_min_y;

   cfg->iv.start.hour   = cfdata->schedule.start_time.hour;
   cfg->iv.start.min    = cfdata->schedule.start_time.min;
   cfg->iv.start.sec    = cfdata->schedule.start_time.sec;

   cfg->iv.stop.hour    = cfdata->schedule.start_time.hour;
   cfg->iv.stop.min     = cfdata->schedule.start_time.min;
   cfg->iv.stop.sec     = cfdata->schedule.start_time.sec;
}

static Intervals
_e_mod_config_schedule_intervals_conf_get(E_Config_Dialog_Data *cfdata)
{
   Intervals iv;

   iv.id          = cfdata->schedule.id;
   iv.lock        = cfdata->schedule.lock;
   iv.urgent      = cfdata->schedule.urgent;
   iv.break_min_x = cfdata->schedule.break_min_x;
   iv.break_min_y = cfdata->schedule.break_min_y;

   iv.start.hour  = cfdata->schedule.start_time.hour;
   iv.start.min   = cfdata->schedule.start_time.min;
   iv.start.sec   = cfdata->schedule.start_time.sec;

   iv.stop.hour   = cfdata->schedule.stop_time.hour;
   iv.stop.min    = cfdata->schedule.stop_time.min;
   iv.stop.sec    = cfdata->schedule.stop_time.sec;
   return iv;
}



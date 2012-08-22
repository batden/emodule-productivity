#include <e.h>
#include <Elementary.h>
#include "e_mod_config.h"

//Round slider floating point minutes into interger.
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

//After clicking start work, notify the user he has 5min to prepair, real work will start
//after the DELAY_START_MIN time has passed;
#define DELAY_START_MIN 5
#define DEFAULT_WORK_HOURS 9
#define DEFAULT_MINIMUM_WORK_MIN 30; //TODO: not implemented!

static void _start_clock_cb(void *data, Evas_Object *obj, void *event_info);
static void _stop_clock_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_mod_config_schedule_start_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_stop_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_break_time_cb(void *data, Evas_Object *obj,
                                                 void *event_info);
static void _e_mod_config_schedule_clock_fill_delay(E_Config_Schedule_Data *csd);
static Eina_Bool e_mod_config_schedule_clock_fill_delay(void *data);

Eina_Bool
e_mod_config_schedule_create_data(E_Config_Dialog_Data *cfdata)
{ 
   _e_mod_config_schedule_clock_fill_delay(&cfdata->schedule);
}

Evas_Object *
e_mod_config_schedule_new(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot;
   Evas_Object *bx, *label;
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

   label = elm_label_add(bx);
   elm_object_text_set(label, "Minutes of break per Hour");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   cfdata->schedule.break_slider = elm_slider_add(bx);
   elm_slider_unit_format_set(cfdata->schedule.break_slider, "%1.0f Minutes");
   elm_slider_min_max_set(cfdata->schedule.break_slider, 0, 15);
   elm_slider_value_set(cfdata->schedule.break_slider,cfdata->schedule.break_min);
   //evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   //evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, cfdata->schedule.break_slider);
   evas_object_show(cfdata->schedule.break_slider);
   evas_object_smart_callback_add(cfdata->schedule.break_slider,
                                  "delay,changed", _e_mod_config_schedule_break_time_cb,
                                  &cfdata->schedule);

   cfdata->schedule.start_btn = e_widget_button_add(evas, _("Start Working"), "list-add",
                                                    _e_mod_config_schedule_start_working_cb, &cfdata->schedule, NULL);
   e_widget_disabled_set(cfdata->schedule.start_btn, EINA_FALSE);
   e_widget_table_object_append(ot, cfdata->schedule.start_btn, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->schedule.stop_btn = e_widget_button_add(evas, _("Stop Working"), "list-remove",
                                                   _e_mod_config_schedule_stop_working_cb, &cfdata->schedule, NULL);
   e_widget_disabled_set(cfdata->schedule.stop_btn, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->schedule.stop_btn, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_toolbook_page_append(otb, NULL, _("Schedule"), ot, 1, 1, 1, 1, 0.5, 0.0);

   /*
      if (cfdata->clock_delay) ecore_timer_del(cfdata->clock_delay);
      cfdata->clock_delay = ecore_timer_add(0.1, e_mod_config_schedule_clock_fill_delay
      , cfdata);
      */
}

Eina_Bool
e_mod_config_schedule_save_config(E_Config_Dialog_Data *cfdata)
{
   Month *m, mm;
   Day *d;
   Intervals *iv;
   Eina_List *l, *ll, *lll;

   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt); 

   INF("Saving Config");
   productivity_conf->timestamp = e_mod_timestamp_get();
   
  // mm = E_NEW(Month, 1);
   EINA_LIST_FOREACH(productivity_conf->month_list, l, m)
     {
        productivity_conf->month.name = eina_stringshare_add("AGUST");
        productivity_conf->month.mon = m->mon+1;
        productivity_conf->month.day.name = eina_stringshare_add("MADMAN");
        productivity_conf->month.day.mday = 32;
        productivity_conf->month.day_list = eina_list_append(m->day_list,
                                                             &productivity_conf->month.day);
     }

   //productivity_conf->month.day_list = eina_list_append(productivity_conf->month.day_list, d);

   //eina_list_free(productivity_conf->month_list);

   //productivity_conf->month.day_list = eina_list_append(productivity_conf->month.day_list,d);
   //productivity_conf->month.day_list = eina_list_append(productivity_conf->month.day_list,d);
   productivity_conf->month_list = eina_list_append(productivity_conf->month_list,
                                                    &productivity_conf->month);
   productivity_conf->day_list = eina_list_append(productivity_conf->day_list,
                                                  &productivity_conf->month.day);
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

   CRI("START_REAL:%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
   CRI("START_SELE:%d:%d:%d",csd->start_time.hour, csd->start_time.min,
       csd->start_time.sec);

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
     }

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

   CRI("STOP_REAL:%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
   CRI("STOP_SELE:%d:%d:%d",csd->stop_time.hour, csd->stop_time.min,
       csd->stop_time.sec);

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
     }

   elm_clock_time_set(obj, csd->stop_time.hour, csd->stop_time.min,
                      csd->stop_time.sec);

}

static void
_e_mod_config_schedule_start_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   unsigned int digedit;

   if(!(csd = data)) return;

   if(e_widget_disabled_get(csd->stop_btn) == EINA_TRUE)
     {
        e_widget_disabled_set(csd->stop_btn, EINA_FALSE);
        e_widget_disabled_set(csd->start_btn, EINA_TRUE);
        digedit = ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_slider, EINA_TRUE);
        if(csd->lock == EINA_FALSE);
        csd->lock = EINA_TRUE;
        ERR("LOCK??:%d",csd->lock);
        INF("Start Working");
     }
}

static void
_e_mod_config_schedule_stop_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   unsigned int digedit;

   if(!(csd = data)) return;

   if(e_widget_disabled_get(csd->start_btn) == EINA_TRUE)
     {
        e_widget_disabled_set(csd->start_btn, EINA_FALSE);
        e_widget_disabled_set(csd->stop_btn, EINA_TRUE);
        digedit = ELM_CLOCK_EDIT_HOUR_UNIT |
           ELM_CLOCK_EDIT_MIN_UNIT | ELM_CLOCK_EDIT_SEC_UNIT;
        elm_clock_edit_mode_set(csd->start_clk, digedit);
        elm_clock_edit_mode_set(csd->stop_clk, digedit);
        elm_object_disabled_set(csd->break_slider, EINA_FALSE);
        if(csd->lock == EINA_TRUE)
          csd->lock = EINA_FALSE;
        ERR("LOCK??:%d", csd->lock);

        INF("Stop Working");
     }
}

static void
_e_mod_config_schedule_break_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

   csd->break_min = round(elm_slider_value_get(obj));    
   INF("Break Time:%d",csd->break_min);
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

   csd->id = productivity_conf->iv.id;
   csd->lock = (Eina_Bool) productivity_conf->iv.lock;
   ERR("LOCK?:%d",csd->lock);
   csd->break_min = productivity_conf->iv.break_min;



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
        INF("Before:%d",csd->start_time.min);
        csd->start_time.min += DELAY_START_MIN - 59;
        INF("After:%d", csd->start_time.min);
        csd->start_time.hour += 1;
     }

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


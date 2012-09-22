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

static Eina_Bool e_mod_config_schedule_clock_fill_delay(void *data);
static Intervals _e_mod_config_schedule_intervals_conf_get(E_Config_Dialog_Data *cfdata);
static void  _e_mod_config_schedule_label_update(E_Config_Schedule_Data *csd);


Eina_Bool
e_mod_config_schedule_create_data(E_Config_Dialog_Data *cfdata)
{ 
   _e_mod_config_schedule_clock_fill_delay(&cfdata->schedule);
}

Evas_Object *
e_mod_config_schedule_new_v2(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *ol;
   Evas_Object *label;

   ol = e_widget_list_add(evas, 0, 0);
#define S cfdata->schedule
   S.urgent_chk = e_widget_check_add(evas, "Satisfy urgent windows instantly",
                                                    &S.urgent);
   evas_object_smart_callback_add(S.urgent_chk, "changed", _e_mod_config_schedule_urgent_cb, cfdata);
   e_widget_list_object_append(ol, S.urgent_chk, 0, 0, 0.5);

   S.label = e_widget_label_add(evas, NULL);
   _e_mod_config_schedule_label_update(&S); 
   e_widget_list_object_append(ol, S.label, 0, 0, 0.5);

   label = e_widget_label_add(evas, "Break Time");
   e_widget_list_object_append(ol, label, 1, 0, 0.5);

   S.break_x = e_widget_slider_add(evas, 1, 0, _("%1.0f Minutes"), 0.0, 30.0, 1.00, 0, &(S.break_min_x), NULL, 100);
   evas_object_smart_callback_add(S.break_x, "changed", _e_mod_config_schedule_break_x_time_cb, &S); 
   e_widget_list_object_append(ol, S.break_x, 1, 0, 0.5);

   label = e_widget_label_add(evas, "Time to Work before Break");
   e_widget_list_object_append(ol, label, 1, 0, 0.5);

   S.break_y = e_widget_slider_add(evas, 1, 0, _("%1.0f Minutes"), 0.0, 320.0, 1.00, 0, &(S.break_min_y), NULL, 100);
   evas_object_smart_callback_add(S.break_y, "changed", _e_mod_config_schedule_break_y_time_cb, &S); 
   e_widget_list_object_append(ol, S.break_y, 1, 0, 0.5);

   ot = e_widget_table_add(evas, EINA_FALSE);

   S.start_btn = e_widget_button_add(evas, _("Start Working"), "list-add", _e_mod_config_schedule_start_working_cb, &S, cfdata);

   e_widget_table_object_append(ot, S.start_btn, 0, 1, 1, 1, 1, 1, 1, 0);

   S.stop_btn = e_widget_button_add(evas, _("Stop Working"), "list-remove", _e_mod_config_schedule_stop_working_cb, &S, cfdata);
   e_widget_table_object_append(ot, S.stop_btn, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(ol, ot, 1, 1, 0.5);
   _e_mod_config_schedule_lock_update(&S);
#undef S
   e_widget_toolbook_page_append(otb, NULL, _("Schedule v2"), ol, 1, 1, 1, 1, 0.5, 0.0);
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
_e_mod_config_schedule_start_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   E_Config_Dialog_Data *cfdata;
   unsigned int digedit;

   if(!(csd = data)) return;
   if(!(cfdata = data2)) return;

   if(e_widget_disabled_get(csd->stop_btn) == EINA_TRUE)
     {
        e_widget_disabled_set(csd->break_x, EINA_TRUE);
        e_widget_disabled_set(csd->break_y, EINA_TRUE);
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

   productivity_conf->init = E_MOD_PROD_INIT_START;
   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(productivity_conf->cwl);
   ecore_timer_thaw(productivity_conf->wm);

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
        e_widget_disabled_set(csd->break_x, EINA_FALSE);
        e_widget_disabled_set(csd->break_y, EINA_FALSE);

        if(csd->lock == EINA_TRUE)
          {
             csd->lock = EINA_FALSE;
          }
     }
   _e_mod_config_schedule_lock_update(csd);

   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);
   productivity_conf->unhide = EINA_TRUE;

   productivity_conf->init = E_MOD_PROD_INIT_STOP; 
   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(productivity_conf->cwl);
   ecore_timer_thaw(productivity_conf->wm);

   e_config_save_queue();
}

static void
_e_mod_config_schedule_urgent_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   if(!(cfdata = data)) return;

   cfdata->schedule.urgent = e_widget_check_checked_get(obj);
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

   _e_mod_config_schedule_label_update(csd);
}

static void
_e_mod_config_schedule_break_y_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

    _e_mod_config_schedule_label_update(csd);
}

static void
_e_mod_config_schedule_label_update(E_Config_Schedule_Data *csd)
{
   char buf[1024];

   snprintf(buf, sizeof(buf), "%1.0f Minutes of Break for %1.0f Minutes of Work",
            csd->break_min_x, csd->break_min_y);

   e_widget_label_text_set(csd->label, buf);
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
   csd->id              = productivity_conf->cur_iv.id;
   csd->lock            = productivity_conf->cur_iv.lock;
   csd->urgent          = productivity_conf->cur_iv.urgent;
   csd->break_min_x     = productivity_conf->cur_iv.break_min_x;
   csd->break_min_y     = productivity_conf->cur_iv.break_min_y;
}

static void
_e_mod_config_schedule_lock_update(E_Config_Schedule_Data *csd)
{
   if(csd->lock == EINA_TRUE)
     {
        e_widget_disabled_set(csd->stop_btn, EINA_FALSE);
        e_widget_disabled_set(csd->break_x, EINA_TRUE);
        e_widget_disabled_set(csd->break_y, EINA_TRUE);
        e_widget_disabled_set(csd->urgent_chk, EINA_TRUE);

        if(e_widget_disabled_get(csd->start_btn) == EINA_FALSE)
          e_widget_disabled_set(csd->start_btn, EINA_TRUE);
     }
   else if (csd->lock == EINA_FALSE)
     {
        e_widget_disabled_set(csd->stop_btn, EINA_TRUE);
        e_widget_disabled_set(csd->break_x, EINA_FALSE);
        e_widget_disabled_set(csd->break_y, EINA_FALSE);
        e_widget_disabled_set(csd->urgent_chk, EINA_FALSE);

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
   return iv;
}

Eina_Bool e_mod_config_schedule_urgent_get()
{
   return productivity_conf->cur_iv.urgent;
}


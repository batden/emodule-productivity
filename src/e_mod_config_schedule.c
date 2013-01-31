#include <e.h>
#include "e_mod_config.h"

static void _e_mod_config_schedule_start_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_stop_working_cb(void *data, void *data2);
static void _e_mod_config_schedule_urgent_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_mod_config_schedule_obbreak_time_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_mod_config_schedule_obwork_time_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_mod_config_schedule_lock_update(E_Config_Schedule_Data *csd);
static void _e_mod_config_schedule_label_update(E_Config_Schedule_Data *csd);


void
e_mod_config_schedule_create_data(E_Config_Dialog_Data *cfdata)
{  
   cfdata->schedule.lock            = productivity_conf->lock;
   cfdata->schedule.urgent          = productivity_conf->urgent;
   cfdata->schedule.break_min       = productivity_conf->break_min;
   cfdata->schedule.work_min        = productivity_conf->work_min;
}

Evas_Object *
e_mod_config_schedule_new_v2(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *ol;
   Evas_Object *label;

   ol = e_widget_list_add(evas, 0, 0);
#define S cfdata->schedule
   S.oburgent = e_widget_check_add(evas, D_("Satisfy urgent windows instantly"),
                                                    &S.urgent);
   evas_object_smart_callback_add(S.oburgent, "changed", _e_mod_config_schedule_urgent_cb, cfdata);
   e_widget_list_object_append(ol, S.oburgent, 0, 0, 0.5);

   S.oblabel = e_widget_label_add(evas, NULL);
   _e_mod_config_schedule_label_update(&S); 
   e_widget_list_object_append(ol, S.oblabel, 0, 0, 0.5);

   label = e_widget_label_add(evas, D_("Break Time"));
   e_widget_list_object_append(ol, label, 1, 0, 0.5);

   S.obbreak = e_widget_slider_add(evas, 1, 0, D_("%1.0f Minutes"), 0.0, 20.0, 1.00, 0, &(S.break_min), NULL, 100);
   evas_object_smart_callback_add(S.obbreak, "changed", _e_mod_config_schedule_obbreak_time_cb, &S); 
   e_widget_list_object_append(ol, S.obbreak, 1, 0, 0.5);

   label = e_widget_label_add(evas, D_("Time to Work before Break"));
   e_widget_list_object_append(ol, label, 1, 0, 0.5);

   S.obwork = e_widget_slider_add(evas, 1, 0, D_("%1.0f Minutes"), 0.0, 90.0, 1.00, 0, &(S.work_min), NULL, 100);
   evas_object_smart_callback_add(S.obwork, "changed", _e_mod_config_schedule_obwork_time_cb, &S); 
   e_widget_list_object_append(ol, S.obwork, 1, 0, 0.5);

   ot = e_widget_table_add(evas, EINA_FALSE);

   S.obstart = e_widget_button_add(evas, D_("Start Working"), "list-add", _e_mod_config_schedule_start_working_cb, &S, cfdata);

   e_widget_table_object_append(ot, S.obstart, 0, 1, 1, 1, 1, 1, 1, 0);

   S.obstop = e_widget_button_add(evas, D_("Stop Working"), "list-remove", _e_mod_config_schedule_stop_working_cb, &S, cfdata);
   e_widget_table_object_append(ot, S.obstop, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(ol, ot, 1, 1, 0.5);
   _e_mod_config_schedule_lock_update(&S);
#undef S
   e_widget_toolbook_page_append(otb, NULL, D_("Schedule"), ol, 1, 1, 1, 1, 0.5, 0.0);
   return otb;
}


void
e_mod_config_schedule_save_config(E_Config_Dialog_Data *cfdata)
{
   productivity_conf->lock         = cfdata->schedule.lock;
   productivity_conf->urgent       = cfdata->schedule.urgent;
   productivity_conf->break_min  = cfdata->schedule.break_min;
   productivity_conf->work_min  = cfdata->schedule.work_min;
} 

static void
_e_mod_config_schedule_start_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   E_Config_Dialog_Data *cfdata;

   if(!(csd = data)) return;
   if(!(cfdata = data2)) return;

   if(e_widget_disabled_get(csd->obstop) == EINA_TRUE)
     {
        e_widget_disabled_set(csd->obbreak, EINA_TRUE);
        e_widget_disabled_set(csd->obwork, EINA_TRUE);
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

   productivity_conf->init = E_MOD_PROD_INIT_START;
   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(productivity_conf->cwl);
   ecore_timer_thaw(productivity_conf->wm);

   e_config_save();
}

static void
_e_mod_config_schedule_stop_working_cb(void *data, void *data2)
{
   E_Config_Schedule_Data *csd;
   E_Config_Dialog_Data *cfdata;

   if(!(csd = data)) return;
   if(!(cfdata = data2)) return;

   if(e_widget_disabled_get(csd->obstart) == EINA_TRUE)
     {
        e_widget_disabled_set(csd->obbreak, EINA_FALSE);
        e_widget_disabled_set(csd->obwork, EINA_FALSE);

        if(csd->lock == EINA_TRUE)
          {
             csd->lock = EINA_FALSE;
          }
     }
   _e_mod_config_schedule_lock_update(csd);

   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);

   productivity_conf->init = E_MOD_PROD_INIT_STOP; 
   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(productivity_conf->cwl);
   ecore_timer_thaw(productivity_conf->wm);

   e_config_save();
}

static void
_e_mod_config_schedule_urgent_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   if(!(cfdata = data)) return;

   cfdata->schedule.urgent = e_widget_check_checked_get(obj);
   productivity_conf->urgent = cfdata->schedule.urgent;
   e_mod_config_schedule_save_config(cfdata);
   e_mod_config_worktools_save(cfdata);
   e_config_save_queue();
}

static void
_e_mod_config_schedule_obbreak_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

   _e_mod_config_schedule_label_update(csd);
}

static void
_e_mod_config_schedule_obwork_time_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Schedule_Data *csd;

   if(!(csd = data)) return;

    _e_mod_config_schedule_label_update(csd);
}

static void
_e_mod_config_schedule_label_update(E_Config_Schedule_Data *csd)
{
   char buf[1024];

   snprintf(buf, sizeof(buf), D_("%1.0f Minutes of Break per every %1.0f Minutes of Work"),
            csd->break_min, csd->work_min);

   e_widget_label_text_set(csd->oblabel, buf);
}

static void
_e_mod_config_schedule_lock_update(E_Config_Schedule_Data *csd)
{
   if(csd->lock == EINA_TRUE)
     {
        e_widget_disabled_set(csd->obstop, EINA_FALSE);
        e_widget_disabled_set(csd->obbreak, EINA_TRUE);
        e_widget_disabled_set(csd->obwork, EINA_TRUE);
        e_widget_disabled_set(csd->oburgent, EINA_TRUE);

        if(e_widget_disabled_get(csd->obstart) == EINA_FALSE)
          e_widget_disabled_set(csd->obstart, EINA_TRUE);
     }
   else if (csd->lock == EINA_FALSE)
     {
        e_widget_disabled_set(csd->obstop, EINA_TRUE);
        e_widget_disabled_set(csd->obbreak, EINA_FALSE);
        e_widget_disabled_set(csd->obwork, EINA_FALSE);
        e_widget_disabled_set(csd->oburgent, EINA_FALSE);

        if(e_widget_disabled_get(csd->obstart) == EINA_TRUE)
          e_widget_disabled_set(csd->obstart, EINA_FALSE);
     }
}

Eina_Bool e_mod_config_schedule_urgent_get()
{
   return productivity_conf->urgent;
}


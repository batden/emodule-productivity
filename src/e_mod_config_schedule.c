#include <e.h>
#include <Elementary.h>
#include "e_mod_config.h"

static void _start_clock_cb(void *data, Evas_Object *obj, void *event_info);
static void _stop_clock_cb(void *data, Evas_Object *obj, void *event_info);

Evas_Object *
e_mod_config_schedule_new(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *start, *stop;
   Evas_Object *bx, *label, *ck, *sl;
   int hrs, min, sec;

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

   ck = elm_clock_add(bx);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_show_seconds_set(ck, EINA_FALSE);
   elm_clock_show_am_pm_set(ck, EINA_FALSE);
   elm_clock_time_get(ck, &hrs, &min, NULL);

   if ((min+5) <= 59) min = min + 5;
   else min = (min + 5) - 59;
   elm_clock_time_set(ck, hrs, min, 0);
   evas_object_smart_callback_add(ck, "changed", _start_clock_cb, NULL);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   label = elm_label_add(bx);
   elm_object_text_set(label, "Time to Stop working");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   ck = elm_clock_add(bx);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_show_seconds_set(ck, EINA_FALSE);
   elm_clock_show_am_pm_set(ck, EINA_FALSE);
   if ((hrs+9) > 24) hrs = hrs-24;
   else hrs = hrs + 8;
   elm_clock_time_set(ck, hrs, (min+5), 0);
   evas_object_smart_callback_add(ck, "changed", _stop_clock_cb, NULL);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   label = elm_label_add(bx);
   elm_object_text_set(label, "Minutes of break per Hour");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   sl = elm_slider_add(bx);
   elm_slider_unit_format_set(sl, "%1.0f Minutes");
   elm_slider_min_max_set(sl, 0, 15);
   elm_slider_value_set(sl,10);
   //evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   //evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   cfdata->schedule.start = e_widget_button_add(evas, _("Start Working"), "list-add",
                               NULL, &cfdata->schedule, NULL);
   //e_widget_disabled_set(start, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->schedule.start, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->schedule.stop = e_widget_button_add(evas, _("Stop Working"), "list-remove",
                              NULL, &cfdata->schedule, NULL);
   //e_widget_disabled_set(stop, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->schedule.stop, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_toolbook_page_append(otb, NULL, _("Schedule"), ot, 1, 1, 1, 1, 0.5, 0.0);
}

static void
_start_clock_cb(void *data, Evas_Object *obj, void *event_info)
{
   int hrs, min, sec;
   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);

   elm_clock_time_get(obj, &hrs, &min, &sec);

   CRI("%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
   INF("Start_Time: %d:%d:%d", hrs, min, sec);
   
   while(hrs < tm->tm_hour)
     {
        hrs++;
        if(hrs > 23)
          {
             hrs = 0;
             break;
          }
     }
   while(min < tm->tm_min)
     {
        min++;
        if(min > 59)
          {
             min = 0;
             break;
          }
     }
   
   elm_clock_time_set(obj, hrs, min, 0);

}

static void 
_stop_clock_cb(void *data, Evas_Object *obj, void *event_info)
{
   int hrs, min, sec;
   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);

   elm_clock_time_get(obj, &hrs, &min, &sec);

   CRI("%d:%d:%d",tm->tm_hour, tm->tm_min, tm->tm_sec);
   INF("Start_Time: %d:%d:%d", hrs, min, sec);

   /*
   while(hrs < tm->tm_hour)
     {
        hrs++;
        if(hrs > 23)
          {
             hrs = 0;
             break;
          }
     }
   while(min < tm->tm_min)
     {
        min++;
        if(min > 59)
          {
             min = 0;
             break;
          }
     }*/

   elm_clock_time_set(obj, hrs, min, 0);

}

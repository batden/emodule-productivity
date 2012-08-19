#include <e.h>
#include <Elementary.h>
#include "e_mod_config.h"

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
/* External Functions */

/* Function for calling our personal dialog menu */
E_Config_Dialog *
e_int_config_productivity_module(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Productivity", "advanced/productivity")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   /* Icon in the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", productivity_conf->module->dir);

   /* create our config dialog */
   cfd = e_config_dialog_new(con, _("Productivity Module"), "Productivity", 
                             "advanced/productivity", buf, 0, v, NULL);

   e_dialog_resizable_set(cfd->dia, 1);
   e_win_size_min_set(cfd->dia->win, 333, 400);
   e_win_resize(cfd->dia->win, 540, 400);
   productivity_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   char buf[PATH_MAX];
   E_Config_Dialog_Data *cfdata = NULL;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   
   e_mod_config_schedule_create_data(cfdata);
   e_mod_config_worktools_create_data(cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   productivity_conf->cfd = NULL;
   E_FREE(cfdata);
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   /* load a temp copy of the config variables */
}

static void
_item_1_pressed(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bx = data;
   Evas_Object *ck, *label, *sl, *bt0, *bt1, *bt2, *frame, *bx0;
   int hrs, min, sec;

   elm_box_clear(bx);
  
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
   if ((hrs+8) > 24) hrs = hrs-24;
   else hrs = hrs + 8;
   elm_clock_time_set(ck, hrs, (min+5), 0);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
  
   label = elm_label_add(bx);
   elm_object_text_set(label, "Minutes of break per Hour");
   evas_object_resize(label, 200, 25);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   /* with unit label and min - max */
   sl = elm_slider_add(bx);
   elm_slider_unit_format_set(sl, "%1.0f Minutes");
   elm_slider_min_max_set(sl, 0, 15);
   elm_slider_value_set(sl,10);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);
   
   frame = elm_frame_add(bx);
   //elm_object_text_set(frame,"Fill and Stretch Options");
   elm_object_style_set(frame,"outdent_top");
   elm_box_pack_end(bx, frame);
   evas_object_show(frame);
  
   bx0 = elm_box_add(bx);
   elm_box_horizontal_set(bx0, 1);
   elm_box_homogeneous_set(bx0, EINA_TRUE);
   elm_object_content_set(frame, bx0);
//   elm_box_pack_end(bx, bx0);
   evas_object_show(bx0);

   bt0 = elm_button_add(bx0);
   elm_object_text_set(bt0, "Start");
   elm_box_pack_end(bx0, bt0);
   evas_object_show(bt0);

   bt1 = elm_button_add(bx0);
   elm_object_text_set(bt1, "Pause");
   elm_box_pack_end(bx0, bt1);
   evas_object_show(bt1);

   bt2 = elm_button_add(bx0);
   elm_object_text_set(bt2, "Stop");
   elm_box_pack_end(bx0, bt2);
   evas_object_show(bt2);
}

static void
_item_2_pressed(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bx = data;
   Evas_Object *cal;
   
   elm_box_clear(bx);
   
   cal = elm_calendar_add(bx);
   evas_object_size_hint_weight_set(cal, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cal, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, cal);
   evas_object_show(cal);
}

static void
_work_tools_pressed(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bx = data;
   Evas_Object *cal;
   
   elm_box_clear(bx);
   
}



static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL, *tb = NULL;
   Evas_Object *bx, *bg, *obx, *otb, *ot, *wotb;
   Elm_Object_Item *tb_it;

  /* o = e_widget_list_add(evas, 0, 0);
   e_widget_list_homogeneous_set(o, EINA_FALSE);


   bx = elm_box_add(o);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   e_widget_list_object_append(o, bx, 1, 1, 0.5);
   evas_object_show(bx);
 
   obx = elm_box_add(o);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_FILL, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(obx);

   tb = elm_toolbar_add(bx);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_FILL, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   evas_object_show(tb);

   tb_it = elm_toolbar_item_append(tb, "document-print", _("Tickets"), NULL, obx);
   tb_it = elm_toolbar_item_append(tb, "document-print", _("Scedule"), _item_1_pressed, obx);
   elm_toolbar_item_selected_set(tb_it, EINA_TRUE);
   tb_it = elm_toolbar_item_append(tb, "folder-new", _("Calendar"), _item_2_pressed, obx);
   tb_it = elm_toolbar_item_append(tb, "folder-new", _("History"), NULL, obx);
   tb_it = elm_toolbar_item_append(tb, "folder-new", _("Configuration"), NULL, obx);
   tb_it = elm_toolbar_item_append(tb, "folder-new", _("Work Tools"), _work_tools_pressed, obx);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   
   elm_box_pack_end(bx, tb);
   elm_box_pack_end(bx, obx);
   evas_object_show(o);*/

   INF("PROD_LOG_CONFIG:%d\n", _productivity_log);
   otb = e_widget_toolbook_add(evas, 24, 24);
   e_mod_config_schedule_new(otb, evas, cfdata);
   e_mod_config_worktools_new(otb, evas, cfdata);
   e_widget_toolbook_page_show(otb, 0);

/*  ot = e_widget_table_add(evas, EINA_FALSE);
   e_widget_toolbook_page_append(otb, NULL, _("Tickets"), ot, 1, 1, 1, 1, 0.5, 0.0);

   ot = e_widget_table_add(evas, EINA_FALSE);
   e_widget_toolbook_page_append(otb, NULL, _("Scedule"), ot, 2, 1, 1, 1, 0.5, 0.0);

   ot = e_widget_table_add(evas, EINA_FALSE);
   e_widget_toolbook_page_append(otb, NULL, _("Calendar"), ot, 3, 1, 1, 1, 0.5, 0.0);
   
   ot = e_widget_table_add(evas, EINA_FALSE);
   e_widget_toolbook_page_append(otb, NULL, _("History"), ot, 4, 1, 1, 1, 0.5, 0.0);
   
   ot = e_widget_table_add(evas, EINA_FALSE);
   e_widget_toolbook_page_append(otb, NULL, _("Configuration"), ot, 5, 1, 1, 1, 0.5, 0.0);

   ot = e_widget_table_add(evas, EINA_FALSE);

   cfdata->apps_user.o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->apps_user.o_list, EINA_TRUE);
//   e_widget_size_min_get(o_list, &mw, NULL);
 //  if (mw < (200 * e_scale)) mw = (200 * e_scale);
 //  e_widget_size_min_set(cfdata->apps_user.o_list, mw, (75 * e_scale));
   e_widget_table_object_append(ot, cfdata->apps_user.o_list, 0, 0, 2, 1, 1, 1, 1, 1);
   cfdata->apps_user.o_add = e_widget_button_add(evas, _("Add"), "list-add",
                                                 _cb_add, &cfdata->apps_user, NULL);
   e_widget_disabled_set(cfdata->apps_user.o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_add, 0, 1, 1, 1, 1, 1, 1, 0);
   
   cfdata->apps_user.o_del = e_widget_button_add(evas, _("Remove"), "list-remove",
                                                 _cb_del, &cfdata->apps_user, NULL);
   e_widget_disabled_set(cfdata->apps_user.o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_del, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_toolbook_page_append(otb, NULL, _("Work Tools"), ot, 6, 1, 1, 1, 0.5, 0.0);
   
   _fill_order_list(cfdata);*/
/*
   ot = e_widget_table_add(evas, EINA_FALSE);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   _fill_order_list(cfdata);
   e_widget_ilist_multi_select_set(cfdata->o_list, EINA_TRUE);
//   e_widget_size_min_get(o_list, &mw, NULL);
 //  if (mw < (200 * e_scale)) mw = (200 * e_scale);
 //  e_widget_size_min_set(cfdata->apps_user.o_list, mw, (75 * e_scale));
   e_widget_table_object_append(ot, cfdata->o_list, 0, 0, 2, 1, 1, 1, 1, 1);
   o_add = e_widget_button_add(evas, _("Add--"), "list-add",
                                                 NULL, NULL, NULL);
   e_widget_disabled_set(o_add, EINA_TRUE);
   e_widget_table_object_append(ot, o_add, 0, 1, 1, 1, 1, 1, 1, 0);
   o_del = e_widget_button_add(evas, _("Remove"), "list-remove",
                                                 NULL, NULL, NULL);
   e_widget_disabled_set(o_del, EINA_TRUE);
   e_widget_table_object_append(ot, o_del, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_toolbook_page_append(wotb, NULL, _("Work Applications"), ot, 2, 1, 1, 1, 0.5, 0.0);

   e_widget_toolbook_page_append(otb, NULL, _("Work Tools"), wotb, 6, 1, 1, 1, 0.5, 0.0);
*/

   //e_widget_toolbook_page_show(otb, 0);
   //e_win_centered_set(cfd->dia->win, 1);
   
   /*if (cfdata->fill_delay) ecore_timer_del(cfdata->fill_delay);
   cfdata->fill_delay = ecore_timer_add(0.2, _cb_fill_delay, cfdata);*/
   return otb;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_mod_config_worktools_save(cfdata);
   //e_config_save_queue();
   return 1;
}


#include <e.h>
#include "e_mod_config.h"

static Eina_List * _e_mod_config_worktools_load_config(const char *path);
static int         _e_mod_config_worktools_save_config(E_Config_Dialog_Data *cfdata);
static Eina_Bool   _cb_fill_delay(void *data);
static void        _list_items_state_set(E_Config_App_List *apps);
static void        _e_mod_config_worktools_application_selected_list_cb(void *data);
static void        _e_mod_config_worktools_add_cb(void *data, void *data2);
static void        _e_mod_config_worktools_del_cb(void *data, void *data2);


Eina_List *
e_mod_config_worktools_selected_get()
{
   char buf[PATH_MAX];
   
   e_user_dir_concat_static(buf, "applications/productivity/.order");
   if(ecore_file_exists(buf) == EINA_FALSE) return NULL;
   return _e_mod_config_worktools_load_config(buf);
}

/*
 * Creates / Fill data for the application list
 */
Eina_Bool
e_mod_config_worktools_create_data(E_Config_Dialog_Data *cfdata)
{
   char buf[PATH_MAX];

   e_user_dir_concat_static(buf, "applications/productivity");
   //FIXME: check if dir exist before create
   ecore_file_mkdir(buf);
   e_user_dir_concat_static(buf, "applications/productivity/.order");
   cfdata->filename = eina_stringshare_add(buf);
   cfdata->apps_user.cfdata = cfdata;
   cfdata->apps = _e_mod_config_worktools_load_config(buf);
   return EINA_TRUE;
}

/*
 * This function saves the changes made in the application list
 */
Eina_Bool 
e_mod_config_worktools_save(E_Config_Dialog_Data *cfdata)
{
   _e_mod_config_worktools_save_config(cfdata);
   return EINA_TRUE;
}

void
e_mod_config_worktools_free(E_Config_Dialog_Data *cfdata)
{
   Efreet_Desktop *desk;

   if (cfdata->fill_delay)
     ecore_timer_del(cfdata->fill_delay);

   if (cfdata->filename)
     eina_stringshare_del(cfdata->filename);

   evas_object_del(cfdata->apps_user.o_list);
   evas_object_del(cfdata->apps_user.o_add);
   evas_object_del(cfdata->apps_user.o_del);
   evas_object_del(cfdata->apps_user.o_desc);

   EINA_LIST_FREE(cfdata->apps_user.desks, desk)
      efreet_desktop_free(desk);

   EINA_LIST_FREE(cfdata->apps, desk)
      efreet_desktop_free(desk);
}

/* 
 * Configuration object to allow you to select which application you will use
 * during work time
 */
Evas_Object *
e_mod_config_worktools_new(Evas_Object *otb, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot;

   ot = e_widget_table_add(evas, EINA_FALSE);

   cfdata->apps_user.o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->apps_user.o_list, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_list, 0, 0, 2, 1, 1, 1, 1, 1);
   cfdata->apps_user.o_add = e_widget_button_add(evas, D_("Add"), "list-add",
            _e_mod_config_worktools_add_cb, &cfdata->apps_user, cfdata);
   e_widget_disabled_set(cfdata->apps_user.o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_add, 0, 1, 1, 1, 1, 1, 1, 0);
   
   cfdata->apps_user.o_del = e_widget_button_add(evas, D_("Remove"), "list-remove",
            _e_mod_config_worktools_del_cb, &cfdata->apps_user, cfdata);
   e_widget_disabled_set(cfdata->apps_user.o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->apps_user.o_del, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_toolbook_page_append(otb, NULL, D_("Work Tools"), ot, 6, 1, 1, 1, 0.5, 0.0);
   
   if (cfdata->fill_delay) ecore_timer_del(cfdata->fill_delay);
   cfdata->fill_delay = ecore_timer_add(0.2, _cb_fill_delay, cfdata);
   return otb;
}

/*********************************************************************************
                                    INTERNALS!
**********************************************************************************/
static int
_cb_desks_sort(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1, *d2;

   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_e_mod_config_worktools_application_list(E_Config_App_List *apps)
{
   Eina_List *desks = NULL;
   Efreet_Desktop *desk = NULL;

   desks = efreet_util_desktop_name_glob_list("*");
   EINA_LIST_FREE(desks, desk)
     {
        Eina_List *ll;

        if (desk->no_display)
          {
             efreet_desktop_free(desk);
             continue;
          }
        ll = eina_list_search_unsorted_list(apps->desks, _cb_desks_sort, desk);
        if (ll)
          {
             Efreet_Desktop *old;

             old = eina_list_data_get(ll);
             /*
              * This fixes when we have several .desktop with the same name,
              * and the only difference is that some of them are for specific
              * desktops.
              */
             if ((old->only_show_in) && (!desk->only_show_in))
               {
                  efreet_desktop_free(old);
                  eina_list_data_set(ll, desk);
               }
             else
               efreet_desktop_free(desk);
          }
        else
          apps->desks = eina_list_append(apps->desks, desk);
     }
   apps->desks = eina_list_sort(apps->desks, -1, _cb_desks_sort);

   _list_items_state_set(apps);
}

static void
_list_items_state_set(E_Config_App_List *apps)
{
   Evas *evas;
   Eina_List *l;
   Efreet_Desktop *desk;

   if (!apps->o_list)
     return;

   evas = evas_object_evas_get(apps->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(apps->o_list);
   e_widget_ilist_clear(apps->o_list);

   EINA_LIST_FOREACH(apps->desks, l, desk)
     {
        Evas_Object *icon = NULL, *end = NULL;

        end = edje_object_add(evas);
        if (!e_theme_edje_object_set(end, "base/theme/widgets",
                                     "e/widgets/ilist/toggle_end"))
          {
             evas_object_del(end);
             end = NULL;
          }

        if (!end) break;

        if (eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_sort, desk))
          {
             edje_object_signal_emit(end, "e,state,checked", "e");
          }
        else
          {
             edje_object_signal_emit(end, "e,state,unchecked", "e");
          }

        icon = e_util_desktop_icon_add(desk, 24, evas);
        e_widget_ilist_append_full(apps->o_list, icon, end, desk->name,
            _e_mod_config_worktools_application_selected_list_cb, apps, NULL);
     }

   e_widget_ilist_go(apps->o_list);
   e_widget_ilist_thaw(apps->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static Eina_Bool
_cb_fill_delay(void *data)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return ECORE_CALLBACK_CANCEL;
   _e_mod_config_worktools_application_list(&cfdata->apps_user);

   cfdata->fill_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int
_cb_desks_name(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1;
   const char *d2;

   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   return strcmp(d1->name, d2);
}

static void
_e_mod_config_worktools_application_selected_list_cb(void *data)
{
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;
   unsigned int enabled = 0, disabled = 0;

   if (!(apps = data)) return;
   EINA_LIST_FOREACH(eina_list_clone(e_widget_ilist_items_get(apps->o_list)), l, it)
     {
        if ((!it->selected) || (it->header)) continue;
        if (eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_name, it->label))
          enabled++;
        else
          disabled++;
     }

   if (apps->o_desc)
     {
        Efreet_Desktop *desk;
        int sel;

        sel = e_widget_ilist_selected_get(apps->o_list);
        desk = eina_list_nth(apps->desks, sel);
        if (desk)
          e_widget_textblock_markup_set(apps->o_desc, desk->comment);
     }

   e_widget_disabled_set(apps->o_add, !disabled);
   e_widget_disabled_set(apps->o_del, !enabled);
}

static void
_e_mod_config_worktools_add_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;

   if (!(cfdata = data2)) return;
   if (!(apps = data)) return;
   EINA_LIST_FOREACH(eina_list_clone(e_widget_ilist_items_get(apps->o_list)), l, it)
     {
        Efreet_Desktop *desk;

        if ((!it->selected) || (it->header)) continue;
        if (!(desk = eina_list_search_unsorted(apps->desks, _cb_desks_name, it->label))) continue;
        if (!eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_sort, desk))
          {
             Evas_Object *end;

             end = e_widget_ilist_item_end_get(it);
             if (end) edje_object_signal_emit(end, "e,state,checked", "e");
             efreet_desktop_ref(desk);
             apps->cfdata->apps = eina_list_append(apps->cfdata->apps, desk);
          }
     }
   e_widget_ilist_unselect(apps->o_list);
   e_widget_disabled_set(apps->o_add, EINA_TRUE);
   e_widget_disabled_set(apps->o_del, EINA_TRUE);

   e_mod_config_worktools_save(cfdata);
   e_config_save_queue();
}

/*
 * This functions looks for $USER/.e/e/applications/productivity/.order file
 * and set the *.desktop files in .order as selected items
 */
static Eina_List *
_e_mod_config_worktools_load_config(const char *path)
{
   E_Order *order = NULL;
   Eina_List *apps = NULL, *l;
   Efreet_Desktop *desk;

   if (!path) return NULL;
   if (!(order = e_order_new(path))) return NULL;
   EINA_LIST_FOREACH(order->desktops, l, desk)
     {
        efreet_desktop_ref(desk);
        apps = eina_list_append(apps, desk);
     }
   e_object_del(E_OBJECT(order));
   return apps;
}

/*
 * This functions when you click "Ok/Apply" from the dialog, it saves all the 
 * enabled applications to $USER/.e/e/applications/productivity/.order
 */
static int
_e_mod_config_worktools_save_config(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Order *order = NULL;
   Efreet_Desktop *desk;

   if (!(order = e_order_new(cfdata->filename))) return 0;
   e_order_clear(order);
   EINA_LIST_FOREACH(cfdata->apps, l, desk)
     {
        if (!desk) continue;
        e_order_append(order, desk);
     }
   e_object_del(E_OBJECT(order));

   //Remove old list.
   productivity_conf->apps_list = eina_list_remove_list(
      productivity_conf->apps_list,
      productivity_conf->apps_list);

   //Update main list
   productivity_conf->apps_list = e_mod_config_worktools_selected_get();
   return 1;
}

static void
_e_mod_config_worktools_del_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_App_List *apps;
   const E_Ilist_Item *it;
   Eina_List *l;

   if (!(cfdata = data2)) return;
   if (!(apps = data)) return;
   EINA_LIST_FOREACH(eina_list_clone(e_widget_ilist_items_get(apps->o_list)), l, it)
     {
        Efreet_Desktop *desk;

        if ((!it->selected) || (it->header)) continue;
        if ((desk = eina_list_search_unsorted(apps->cfdata->apps, _cb_desks_name, it->label)))
          {
             Evas_Object *end;

             end = e_widget_ilist_item_end_get(it);
             if (end) edje_object_signal_emit(end, "e,state,unchecked", "e");
             apps->cfdata->apps = eina_list_remove(apps->cfdata->apps, desk);
             efreet_desktop_unref(desk);
          }
     }

   e_widget_ilist_unselect(apps->o_list);
   e_widget_disabled_set(apps->o_add, EINA_TRUE);
   e_widget_disabled_set(apps->o_del, EINA_TRUE);

   e_mod_config_worktools_save(cfdata);
   e_config_save_queue();
}



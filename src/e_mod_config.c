#include <e.h>
#include "e_mod_config.h"

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
/* External Functions */

/* Function for calling our personal dialog menu */
E_Config_Dialog *
e_int_config_productivity_module(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Productivity", "extensions/productivity")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _check_changed;

   /* Icon in the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", productivity_conf->module->dir);

   /* create our config dialog */
   cfd = e_config_dialog_new(con, D_("Productivity Settings"), "Productivity",
                             "extensions/productivity", buf, 0, v, NULL);

   e_dialog_resizable_set(cfd->dia, 1);
   e_win_size_min_set(cfd->dia->win, 400, 400);
   e_win_resize(cfd->dia->win, 400, 400);
   productivity_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   
   e_mod_config_schedule_create_data(cfdata);
   e_mod_config_worktools_create_data(cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_mod_config_worktools_save(cfdata);

   productivity_conf->cfd = NULL;
   evas_object_del(cfdata->schedule.obstart);
   evas_object_del(cfdata->schedule.obstop);
   evas_object_del(cfdata->schedule.obbreak);
   evas_object_del(cfdata->schedule.obwork);
   evas_object_del(cfdata->schedule.oburgent);

   e_mod_config_worktools_free(cfdata);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *otb;

   INF("PROD_LOG_CONFIG:%d\n", _productivity_log);
   otb = e_widget_toolbook_add(evas, 24, 24);

   e_mod_config_schedule_new_v2(otb, evas, cfdata);
   e_mod_config_worktools_new(otb, evas, cfdata);

   e_widget_toolbook_page_show(otb, 0);

   return otb;
}

static int 
_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_mod_config_schedule_save_config(cfdata);
   e_config_save(); 
   return 1;
}


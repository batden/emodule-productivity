#include "e_mod_main.h"

typedef struct _E_Config_Schedule_Data E_Config_Schedule_Data;
typedef struct _E_Config_App_List E_Config_App_List;


//    e_mod_config_worktools.c
struct _E_Config_App_List
{
   E_Config_Dialog_Data    *cfdata;
   Evas_Object             *o_list, *o_add, *o_del, *o_desc;
   Eina_List               *desks;
};

//    e_mod_config_schedule.c
struct _E_Config_Schedule_Data
{
   Evas_Object *obstart;
   Evas_Object *obstop;
   Evas_Object *obbreak;
   Evas_Object *obwork;
   Evas_Object *oblabel;
   Evas_Object *oburgent;
   Eina_Bool lock;
   int urgent;
   double break_min;
   double work_min;
};

//    e_mod_config.c
struct _E_Config_Dialog_Data 
{
   // e_mod_config_worktools.c
   const char              *filename;
   Eina_List               *apps;
   Ecore_Timer             *fill_delay;
   E_Config_App_List        apps_user;

   // e_mod_config_schedule.c
   E_Config_Schedule_Data   schedule;
};

//    e_mod_config_worktools.c
Evas_Object *e_mod_config_worktools_new(Evas_Object *otb, Evas *evas,
                                        E_Config_Dialog_Data *cfdata);
Eina_Bool    e_mod_config_worktools_create_data(E_Config_Dialog_Data *cfdata);
Eina_Bool    e_mod_config_worktools_save(E_Config_Dialog_Data *cfdata);
Eina_Bool    e_mod_config_windows_create_data(void *data);
Eina_List   *e_mod_config_worktools_selected_get();



//    e_mod_config_schedule.c
Eina_Bool    e_mod_config_schedule_create_data(E_Config_Dialog_Data *cfdata);
Eina_Bool    e_mod_config_schedule_urgent_get();
Evas_Object *e_mod_config_schedule_new(Evas_Object *otb, Evas *evas,
                                       E_Config_Dialog_Data *cfdata);
Evas_Object *e_mod_config_schedule_new_v2(Evas_Object *otb, Evas *evas,
                                          E_Config_Dialog_Data *cfdata);

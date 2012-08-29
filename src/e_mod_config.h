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
   Evas_Object *start_btn;
   Evas_Object *stop_btn;
   Evas_Object *start_clk;
   Evas_Object *stop_clk;
   Evas_Object *break_x;
   Evas_Object *break_y;
   Evas_Object *label;
   Eina_Bool lock;
   int id;
   int break_min_x;
   int break_min_y;
   struct
   {
       int hour;
       int min;
       int sec;
   }start_time, stop_time;
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
   Ecore_Timer             *clock_delay;
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
Evas_Object *e_mod_config_schedule_new(Evas_Object *otb, Evas *evas,
                                       E_Config_Dialog_Data *cfdata);

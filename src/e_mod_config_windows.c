#include <e.h>
#include "e_mod_config.h"

static Eina_Bool     _e_mod_config_window_event_border_add_cb(void *data,
                                                              int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_remove_cb(void *data,
                                                                 int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_iconify_cb(void *data,
                                                                  int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_uniconify_cb(void *data,
                                                                    int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_property_cb(void *data,
                                                                   int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_focus_in_cb(void *data,
                                                                   int type, void *event);
static Eina_Bool     _e_mod_config_window_event_border_focus_out_cb(void *data,
                                                                    int type, void *event);

static long          _e_mod_config_windows_current_time_get();

Eina_Bool
e_mod_config_windows_create_data(void *data __UNUSED__)
{
   E_Config_Window_List  *cwl;

   cwl = E_NEW(E_Config_Window_List, 1);

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ADD, _e_mod_config_window_event_border_add_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_REMOVE, _e_mod_config_window_event_border_remove_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ICONIFY, _e_mod_config_window_event_border_iconify_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_UNICONIFY, _e_mod_config_window_event_border_uniconify_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_IN, _e_mod_config_window_event_border_focus_in_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_OUT, _e_mod_config_window_event_border_focus_out_cb, cwl));
   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_PROPERTY, _e_mod_config_window_event_border_property_cb, cwl));

   cwl->borders = eina_list_clone(e_border_client_list());

   if(cwl->borders) return EINA_TRUE;

   return EINA_FALSE;
}



Eina_Bool
tasks_cb_window_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;
   E_Border *bd;

   ev = event;
   bd = ev->border;

   INF(bd->client.icccm.name);
   INF(bd->client.icccm.class);
   //INF(bd->lock_user_location);
   INF("Lock_User_Location:%d\n",bd->lock_user_location);
   INF("APP: %s PID:%d\n",bd->client.icccm.name, bd->client.netwm.pid);
   //bd->shape->visible = 0;

   //bd->lock_user_location = 1;
   //bd->visible = 0;
   //bd->iconic = 1;
   //bd->lock_user_iconify = 1;

   if(!bd->iconic)
     {
        e_border_iconify(bd);
        bd->lock_user_iconify = 1;
     }

   e_border_lower(bd);
   //_tasks_signal_emit(ev->border, "e,state,focused", "e");
   return EINA_TRUE;
}

/***********************************************************************************
  INTERNALS
 ************************************************************************************/

static Eina_Bool
_e_mod_config_window_event_border_add_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;
   E_Config_Window_List *cwl;
   E_Config_Window_List_Data *cwldata;

   cwl = data;
   if(cwl->cwldata) E_FREE(cwl->cwldata);
   cwl->cwldata = NULL;
   cwldata = cwl->cwldata;
   ev = event;

   cwl->borders = eina_list_append(cwl->borders, ev->border);

   cwldata = E_NEW(E_Config_Window_List_Data, 1);

   cwldata->name = eina_stringshare_add(ev->border->client.icccm.name);
   cwldata->command = eina_stringshare_add(ev->border->client.icccm.command.argv[0]);
   cwldata->pid = ev->border->client.netwm.pid;
   cwldata->seconds = _e_mod_config_windows_current_time_get();
   cwl->cwldata_list = eina_list_append(cwl->cwldata_list, cwldata);

   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);
   //_tasks_refill_all();
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_remove_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev;
   E_Config_Window_List *cwl;
   E_Config_Window_List_Data *cwldata;
   Eina_List *l;
   E_Border *bd;
   long seconds;

   cwl = data;
   ev = event;

   seconds = _e_mod_config_windows_current_time_get();
   cwl->borders = eina_list_remove(cwl->borders, ev->border);

   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);

   EINA_LIST_FOREACH(cwl->borders, l, bd)
     {
        INF("LIST:%s",bd->client.icccm.name);
     }
   eina_list_free(l);

   EINA_LIST_FOREACH(cwl->cwldata_list, l, cwldata)
     {
        INF("NAME:%s",cwldata->name);
        INF("Command:%s",cwldata->command);
        INF("PID:%d EXIT_PID %d",cwldata->pid,ev->border->client.netwm.pid);
        if(cwldata->pid == ev->border->client.netwm.pid)
          {
             INF("Pid Match Found!");
             INF(":%s ran for %d seconds",cwldata->command, (seconds - cwldata->seconds));
             cwl->cwldata_list = eina_list_remove(cwl->cwldata_list, cwldata);
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_iconify_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;

   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_uniconify_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Uniconify *ev;

   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_in_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);
   INF("Command:%s",ev->border->client.icccm.command.argv[0]);  
   INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_out_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_property_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev;
   E_Border *border;

   ev = event;
   border = ev->border;
   //if (border) _tasks_refill_border(border);
   //INF(border->client.icccm.name);
   //INF(border->client.icccm.class);
   return EINA_TRUE;
}

/*
 * Gets the current time in seconds, it's used to calculate, the time an application opens
 * and the time the application is closed, this allows us to track how long the application was
 * opened.
 */
static long
_e_mod_config_windows_current_time_get()
{
   time_t seconds;

   seconds = time(NULL);
   return (long) seconds;
}


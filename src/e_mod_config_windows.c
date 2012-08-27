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

static unsigned int  _e_mod_config_windows_current_time_get();
static void          _e_mod_config_window_hide(E_Border *bd);
static void          _e_mod_config_window_unhide(E_Border *bd);

Eina_Bool
e_mod_config_windows_create_data(void *data __UNUSED__)
{
   E_Config_Window_List  *cwl;

   cwl = E_NEW(E_Config_Window_List, 1);

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ADD, 
        _e_mod_config_window_event_border_add_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_REMOVE, 
        _e_mod_config_window_event_border_remove_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ICONIFY, 
        _e_mod_config_window_event_border_iconify_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_UNICONIFY, 
        _e_mod_config_window_event_border_uniconify_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_IN, 
        _e_mod_config_window_event_border_focus_in_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_OUT, 
        _e_mod_config_window_event_border_focus_out_cb, cwl));

   cwl->handlers = eina_list_append
      (cwl->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_PROPERTY, 
        _e_mod_config_window_event_border_property_cb, cwl));

   cwl->borders = eina_list_clone(e_border_client_list());

   //Here we start to manage worktools and non-worktools
   e_mod_config_window_manager(cwl);
   if(cwl->borders) return EINA_TRUE;

   return EINA_FALSE;
}
unsigned int
e_mod_timestamp_get()
{
   return _e_mod_config_windows_current_time_get();
}

void
e_mod_config_window_manager(E_Config_Window_List *cwl)
{
   Eina_List *l, *ll;
   Config *cfg;
   Efreet_Desktop *desk;
   E_Border *bd;
   Eina_Bool m = EINA_FALSE;

   //If we cant find config return;
   if(!(cfg = productivity_conf)) return;

   // If the current time[NOW] is not more than [Start_time] and less than [Stop_time]
   // return;
   if(e_mod_main_is_it_time_to_work() == EINA_FALSE)
     {
        EINA_LIST_FOREACH(cwl->borders, l, bd)
          {
             _e_mod_config_window_unhide(bd);
          }
        return;
     }

   EINA_LIST_FOREACH(cwl->borders, l, bd)
     {
        DBG("NAME:%s", bd->client.icccm.name);
        //If application name is E [Enlightenment] we skip to the next app.
        if(bd->client.icccm.name)
          if(strncmp(bd->client.icccm.name, "E", sizeof("E")) == 0)
            continue;

        //If application class is _config:: [Enlightenment Config Dialog] we skip to the next app.
        if(bd->client.icccm.class)
          if(strncmp(bd->client.icccm.class, "_config::", 8) == 0)
            continue;

        //If the user opens an application from the terminal and e17 CAN NOT find the matching 
        //.desktop file for this application OR such .desktop file does not exists the bd->desktop
        //will return NULL;
        if (!bd->desktop)
          {
             //First attempt to find the .desktop file by searching icccm.name
             bd->desktop = efreet_util_desktop_exec_find(bd->client.icccm.name);

             //Second attempt to find the .desktop file by search icccm.name in lowercase
             //this happens e.g with Pidgin, it seems to think it's command is Pidgin but in 
             //real life it's pidgin [lowercase]
             if(!bd->desktop)
               {
                  int i = 0;
                  char buf[PATH_MAX];
                  snprintf(buf, sizeof(buf), "%s",bd->client.icccm.name);

                  while(buf[i] < (sizeof(buf) -1))
                    {
                       buf[i] = tolower(buf[i]);
                       i++;
                    }
                  bd->desktop = efreet_util_desktop_exec_find(buf);
               }

             CRI("!Name:%s , Class:%s", bd->client.icccm.name, bd->client.icccm.class);
             if(bd->client.icccm.command.argv)
               {
                  //CRI("Command:%s",bd->client.icccm.command.argv[0]);
                  EINA_LIST_FOREACH(cfg->apps, ll, desk)
                    {
                       if(desk->exec)
                         {
                            //WRN("cmd:%s ? exec: %s", bd->client.icccm.command.argv[0], desk->exec);
                         }
                    }
               }

             //If we are still unable to get the .desktop just hide the window, user should always use .desktop
             //file to launch worktools :) 
             if(!bd->desktop)
               {
                  ERR("Unable to get a .desktop, giving up, will just hide this app");
                  //_e_mod_config_window_hide(bd);
                  e_border_act_close_begin(bd);
               }

          }

        // If the border has the correct .desktop file, we will do comparison to our worktools application
        // list [cfg->apps], if we find a match [m] = EINA_TRUE.
        if(bd->desktop)
          {
             CRI("Name:%s , Class:%s", bd->client.icccm.name, bd->client.icccm.class);
             EINA_LIST_FOREACH(cfg->apps, ll, desk)
               {
                  if(desk->name)
                    {
                       //DBG("Name:%s , Orig_Path:%s", desk->name, desk->orig_path);
                       if ((strncmp(desk->name,bd->desktop->name, sizeof(desk->name)) == 0 ) &&
                           (strncmp(desk->orig_path,bd->desktop->orig_path, sizeof(desk->orig_path)) == 0 ))
                         m = EINA_TRUE;
                    }
               }

             // if [m] is false , this means the application IS NOT in our worktools list, so we hide it!
             if (m == EINA_FALSE)
               {
                  DBG("HIDING APPLICATON:%s", bd->desktop->name);
                  if(bd) _e_mod_config_window_hide(bd);
               }
          }

        m = EINA_FALSE;
     }
} 

/***********************************************************************************
  INTERNALS
 ************************************************************************************/

static void
_e_mod_config_window_hide(E_Border *bd)
{
   E_Remember *rem;

   if(!bd->iconic)
     {
        e_border_iconify(bd);
        bd->user_skip_winlist = 1;
        bd->client.netwm.state.skip_taskbar = 1;
        bd->client.netwm.state.skip_pager = 1;
        bd->client.netwm.state.hidden = 1;
        bd->client.netwm.update.state = 1;
        bd->lock_user_iconify = 1;
     }

   e_border_lower(bd);

   /* if(!bd->remember)
      {
      rem = e_remember_new();
      bd->remember = rem;
      }

      e_remember_default_match_set(rem, bd);
      */
   /*  
       rem->match = 0;
       rem->apply_first_only = bd->remember->apply_first_only;
       rem->match |= E_REMEMBER_MATCH_NAME | E_REMEMBER_MATCH_CLASS | E_REMEMBER_MATCH_TITLE;
       */
   /* if(bd->desktop)
      {
      rem->prop.desktop_file = eina_stringshare_add(bd->desktop->orig_path);
      }
      */

   //e_border_act_close_begin(bd);
   return;
}

static void
_e_mod_config_window_unhide(E_Border *bd)
{
   if(bd->iconic)
     {
        e_border_uniconify(bd);
        bd->user_skip_winlist = 0;
        bd->client.netwm.state.skip_taskbar = 0;
        bd->client.netwm.state.skip_pager = 0;
        bd->client.netwm.state.hidden = 0;
        bd->client.netwm.update.state = 0;
        bd->lock_user_iconify = 0;
     }
   e_border_raise(bd);
   return;
}


static Eina_Bool
_e_mod_config_window_event_border_add_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;
   E_Config_Window_List *cwl;
   E_Config_Window_List_Data *cwldata;

   cwldata = E_NEW(E_Config_Window_List_Data, 1);
   cwl = data;
   ev = event;

   cwldata->name = eina_stringshare_add(ev->border->client.icccm.name);

   if(ev->border->client.icccm.command.argv)
     cwldata->command = eina_stringshare_add(ev->border->client.icccm.command.argv[0]);
   cwldata->pid = ev->border->client.netwm.pid;
   cwldata->seconds = _e_mod_config_windows_current_time_get();

   cwl->cwldata_list = eina_list_append(cwl->cwldata_list, cwldata);
   cwl->borders = eina_list_append(cwl->borders, ev->border);

   if(ev->border->client.icccm.command.argv)
     {
        INF("NAME:%s CLASS:%s COMMAND:%s PID:%d",ev->border->client.icccm.name,
            ev->border->client.icccm.class,
            ev->border->client.icccm.command.argv[0],
            ev->border->client.netwm.pid);
     }
   else
     {
        INF("NAME:%s CLASS:%s COMMAND:(---) PID:%d",ev->border->client.icccm.name,
            ev->border->client.icccm.class,
            ev->border->client.netwm.pid);
     }

   e_mod_config_window_manager(cwl);
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

   //INF(ev->border->client.icccm.name);
   //INF(ev->border->client.icccm.class);  
   /* INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);

      EINA_LIST_FOREACH(cwl->borders, l, bd)
      {
      DBG("LIST:%s",bd->client.icccm.name);
      }*/
   cwl->borders = eina_list_remove(cwl->borders, ev->border);
   // eina_list_free(l);

   EINA_LIST_FOREACH(cwl->cwldata_list, l, cwldata)
     {
        //WRN("NAME:%s",cwldata->name);
        //WRN("Command:%s",cwldata->command);
        //WRN("PID:%d EXIT_PID %d",cwldata->pid,ev->border->client.netwm.pid);
        if(ev->border->client.netwm.pid)
          if(cwldata->pid == ev->border->client.netwm.pid)
            {
               CRI(":%s ran for %d seconds",cwldata->name, (seconds - cwldata->seconds));
               cwl->cwldata_list = eina_list_remove(cwl->cwldata_list, cwldata);
            }
     }

   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_iconify_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;

   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_uniconify_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Uniconify *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   INF(ev->border->client.icccm.name);
   INF(ev->border->client.icccm.class);  
   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_in_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   //if(ev->border->client.icccm.command.argv[0])
   //   INF("Command:%s",ev->border->client.icccm.command.argv[0]);
   //else
   //  CRI("Unable to get command");

   INF("NAME:%s CLASS:%s PID:%d",ev->border->client.icccm.name,
       ev->border->client.icccm.class,
       ev->border->client.netwm.pid);
   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_out_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   INF("NAME:%s CLASS:%s PID:%d",ev->border->client.icccm.name,
       ev->border->client.icccm.class,
       ev->border->client.netwm.pid);
   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_property_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev;
   E_Border *border;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   border = ev->border;
   //if (border) _tasks_refill_border(border);
   //INF(border->client.icccm.name);
   //INF(border->client.icccm.class);
   //e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

/*
 * Gets the current time in seconds, it's used to calculate, the time an application opens
 * and the time the application is closed, this allows us to track how long the application was
 * opened.
 */
static unsigned int
_e_mod_config_windows_current_time_get()
{
   time_t seconds;

   seconds = time(NULL);
   return (unsigned int) seconds;
}


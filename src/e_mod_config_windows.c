#include <e.h>
#include "e_mod_config.h"

#define EVENT_DBG_()
#define EVENT_DBG()                                                                 \
{                                                                                   \
   if((ev->border->client.icccm.name) &&                                            \
      (ev->border->client.icccm.class))                                             \
     {                                                                              \
        DBG("Name:%s :: Class:%s", ev->border->client.icccm.name,                   \
            ev->border->client.icccm.class);                                        \
        if((ev->border->client.netwm.pid > 0) &&                                    \
           (ev->border->client.icccm.command.argc > 0) &&                           \
           (ev->border->client.icccm.command.argv))                                 \
          {                                                                         \
             DBG(" ::CMD:%s :: PID:%d ", ev->border->client.icccm.command.argv[0],  \
                 ev->border->client.netwm.pid);                                     \
          }                                                                         \
     }                                                                              \
}


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
static Eina_Bool     _e_mod_config_window_event_border_urgent_change_cb(void *data,
                                                                        int type, void *event);
static Eina_Bool     _e_mod_config_window_break_timer(void *data);

static unsigned int  _e_mod_config_windows_current_time_get();
static void          _e_mod_config_window_hide(E_Border *bd);
static void          _e_mod_config_window_unhide(E_Border *bd);
static void          _e_mod_config_window_remember_set(E_Border *bd);
static void          _e_mod_config_window_remember_get(E_Border *bd);
static void          _e_mod_config_window_remember_free(E_Border *bd);

Eina_Bool
e_mod_config_windows_create_data(void *data)
{
   Config *cfg;

   if(!(cfg = data)) return EINA_FALSE;


   cfg->cwl = E_NEW(E_Config_Window_List, 1);

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ADD, 
        _e_mod_config_window_event_border_add_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_REMOVE, 
        _e_mod_config_window_event_border_remove_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_ICONIFY, 
        _e_mod_config_window_event_border_iconify_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_UNICONIFY, 
        _e_mod_config_window_event_border_uniconify_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_IN, 
        _e_mod_config_window_event_border_focus_in_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_FOCUS_OUT, 
        _e_mod_config_window_event_border_focus_out_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_PROPERTY, 
        _e_mod_config_window_event_border_property_cb, cfg->cwl));

   cfg->handlers = eina_list_append
      (cfg->handlers, ecore_event_handler_add
       (E_EVENT_BORDER_URGENT_CHANGE, 
        _e_mod_config_window_event_border_urgent_change_cb, cfg->cwl));

   cfg->cwl->borders = eina_list_clone(e_border_client_list());

   //Here we start to manage worktools and non-worktools
   e_mod_config_window_manager(cfg->cwl);
   productivity_conf->timer = 
      ecore_timer_loop_add(1.00, _e_mod_config_window_break_timer, cfg->cwl);

   if(cfg->cwl->borders) return EINA_TRUE;

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
   Eina_List *l, *ll, *lll, *ul;
   Config *cfg;
   Remember *rem;
   Efreet_Desktop *desk;
   E_Border *bd;
   Eina_Bool m = EINA_FALSE;

   //If we cant find config return;
   if(!(cfg = productivity_conf)) return;

   // If the current time[NOW] is not more than [Start_time] and less than [Stop_time]
   // return;
   if(e_mod_main_is_it_time_to_work() != EINA_TRUE)
     {
        EINA_LIST_FOREACH(cwl->borders, l, bd)
          {
             if(!bd) continue;

             _e_mod_config_window_remember_get(bd);

             if(cfg->go_to_break == EINA_TRUE)
               cfg->unhide = EINA_TRUE;

             if(cfg->unhide == EINA_TRUE)
               _e_mod_config_window_unhide(bd);

             if(cfg->unhide == EINA_FALSE)
               _e_mod_config_window_remember_free(bd);
          }
        if(cfg->unhide == EINA_FALSE)
          e_mod_config_window_remember_free_all();

        cfg->unhide = EINA_FALSE;
        return;
     }

   if((cwl->urgent == EINA_TRUE) && 
      (productivity_conf->cur_iv.urgent == EINA_TRUE))
     {
        EINA_LIST_FOREACH(cwl->urgent_window, ul, bd)
          {
             if(bd)
               {
                  INF("window manager found urgent window:%s",bd->client.icccm.name);
                  _e_mod_config_window_remember_get(bd);
                  _e_mod_config_window_unhide(bd);
                  return;
               }
          }
     }

   EINA_LIST_FOREACH(cwl->borders, l, bd)
     {
        if(!bd) continue;

        if(bd->client.icccm.name)
          if(strncmp(bd->client.icccm.name, "E", sizeof("E")) == 0)
            continue;

        if(bd->client.icccm.class)
          if(strncmp(bd->client.icccm.class, "_config::", 8) == 0)
            continue;

        if (!bd->desktop)
          {
             bd->desktop = efreet_util_desktop_exec_find(bd->client.icccm.name);
          }
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
        if(!bd->desktop)
          {
             int i = 0;
             char buf[PATH_MAX];
             snprintf(buf, sizeof(buf), "%s",bd->client.icccm.class);

             while(buf[i] < (sizeof(buf) -1))
               {
                  buf[i] = tolower(buf[i]);
                  i++;
               }
             bd->desktop = efreet_util_desktop_exec_find(buf);
          }
        if (!bd->desktop)
          {
             if ((bd->client.icccm.name) && (bd->client.icccm.class))
               bd->desktop = efreet_util_desktop_wm_class_find(bd->client.icccm.name,
                                                               bd->client.icccm.class);
          }


        /*//CRI("!Name:%s , Class:%s", bd->client.icccm.name, bd->client.icccm.class);
          if(bd->client.icccm.command.argv)
          {
        //CRI("Command:%s",bd->client.icccm.command.argv[0]);
        EINA_LIST_FOREACH(cfg->apps_list, ll, desk)
        {
        if(desk->exec)
        {
        //WRN("cmd:%s ? exec: %s", bd->client.icccm.command.argv[0], desk->exec);
        }
        }
        }*/

        //If we are still unable to get the .desktop just hide the window, user should always use .desktop
        //file to launch worktools :) 
        if(!bd->desktop)
          {
             ERR("Unable to get a .desktop, giving up, will now close %s",
                 bd->client.icccm.name);
             e_border_act_close_begin(bd);
          }

        // If the border has the correct .desktop file, we will do comparison to our worktools application
        // list [cfg->apps], if we find a match [m] = EINA_TRUE.
        if(bd->desktop)
          {
             //CRI("Name:%s , Class:%s", bd->client.icccm.name, bd->client.icccm.class);
             EINA_LIST_FOREACH(cfg->apps_list, ll, desk)
               {
                  if(desk->name)
                    {
                       if ((strncmp(desk->name,bd->desktop->name, sizeof(desk->name)) == 0 ) &&
                           (strncmp(desk->orig_path,bd->desktop->orig_path, sizeof(desk->orig_path)) == 0 ))
                         {
                            //DBG("Name:%s , Orig_Path:%s", desk->name, desk->orig_path);
                            if(bd->client.netwm.pid > 0)
                            m = EINA_TRUE;

                            EINA_LIST_FOREACH(productivity_conf->remember_list, lll, rem)
                              {
                                 if(!rem) continue;

                                 if(rem->pid == bd->client.netwm.pid)
                                   {
                                      _e_mod_config_window_unhide(bd);
                                      productivity_conf->remember_list = 
                                         eina_list_remove(productivity_conf->remember_list, rem);
                                   }
                              }
                         }
                    }
               }

             // if [m] is false , this means the application IS NOT in our worktools list, so we hide it!
             if (m == EINA_FALSE)
               {
                  //DBG("HIDING APPLICATON:%s", bd->desktop->name);
                  //if(bd) _e_mod_config_window_remember_set(bd); 
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
   if(!bd->iconic)
     e_border_iconify(bd);

   if(!bd->user_skip_winlist)
     bd->user_skip_winlist = 1;

   if(!bd->lock_user_iconify)
     bd->lock_user_iconify = 1;

   return;
}

static void
_e_mod_config_window_unhide(E_Border *bd)
{
   if(bd->client.netwm.state.skip_pager)
     bd->client.netwm.state.skip_pager = 0;

   if(bd->user_skip_winlist)
     bd->user_skip_winlist = 0;

   if(bd->lock_user_iconify)
     bd->lock_user_iconify = 0;

   if(bd->iconic)
     e_border_uniconify(bd);
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

   EVENT_DBG();
   INF("URGENT??:%d", ev->border->client.icccm.urgent); 
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

   //seconds = _e_mod_config_windows_current_time_get();

   //INF(ev->border->client.icccm.name);
   //INF(ev->border->client.icccm.class);  
   //INF("APP: %s PID:%d",ev->border->client.icccm.name, ev->border->client.netwm.pid);
   /*
      EINA_LIST_FOREACH(cwl->borders, l, bd)
      {
      DBG("LIST:%s",bd->client.icccm.name);

      if(strncmp(bd->client.icccm.name, ev->border->client.icccm.name, sizeof(bd->client.icccm.name)) == 0)
      {
      if((ev->border->client.netwm.pid > 0) && (bd->client.netwm.pid == ev->border->client.netwm.pid))
      {
      INF("MATCH");
      cwl->borders = eina_list_remove(cwl->borders, bd);
      }
      }
      }
      */
   cwl->borders = eina_list_remove(cwl->borders, ev->border);
   cwl->urgent_window = eina_list_remove(cwl->urgent_window, ev->border);
   /*

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
   }*/
   EVENT_DBG();
   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_iconify_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;

   //Using EVENT_DBG here causes segf
   //EVENT_DBG();
   e_mod_config_window_manager(cwl);
   _e_mod_config_window_remember_set(ev->border);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_uniconify_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Uniconify *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;

   EVENT_DBG();
   e_mod_config_window_manager(cwl);
   _e_mod_config_window_remember_free(ev->border);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_in_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_out_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;
   E_Config_Window_List *cwl;
   E_Border *bd;
   Eina_List *l;

   if(!(cwl = data)) return;
   ev = event;

   EVENT_DBG();

   if((cwl->urgent == EINA_TRUE ) && (productivity_conf->cur_iv.urgent == EINA_TRUE))
     {
        EINA_LIST_FOREACH(cwl->urgent_window, l, bd)
          {
             if(strncmp(bd->client.icccm.name, ev->border->client.icccm.name, sizeof(bd->client.icccm.name)) == 0)
               {
                  if((ev->border->client.netwm.pid > 0) && (bd->client.netwm.pid == ev->border->client.netwm.pid))
                    {
                        CRI("i am now focused on the urgent window");
                       //cwl->borders = eina_list_remove(cwl->borders, bd);
                        cwl->urgent_window = eina_list_remove(cwl->urgent_window, bd);
                        //cwl->borders = eina_list_append(cwl->borders, bd);
             cwl->urgent = EINA_FALSE;
                    }
               }

             /*
                if(bd == ev->border)
                {
                CRI("i am now focused on the urgent window");
                cwl->urgent_window = eina_list_remove(cwl->urgent_window, bd);
             //cwl->borders = eina_list_append(cwl->borders, bd);
             cwl->urgent = EINA_FALSE;
             }*/
          }
     }

   e_mod_config_window_manager(cwl);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_property_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev;
   E_Border *border;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;

   //EVENT_DBG();
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_urgent_change_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Urgent_Change *ev;
   E_Border *border;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;

   EVENT_DBG();
   cwl->urgent_window = eina_list_append(cwl->urgent_window, ev->border);
   cwl->borders = eina_list_remove(cwl->borders, ev->border);
   cwl->urgent = EINA_TRUE;

   e_mod_config_window_manager(cwl);
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

static void
_e_mod_config_window_remember_set(E_Border *bd)
{
   Remember *rem;
   Eina_List *l;
   Eina_Bool exists = EINA_FALSE;

   EINA_LIST_FOREACH(productivity_conf->remember_list, l, rem)
     {
        if(strncmp(rem->name, "", sizeof("")) == 0)
          {
             productivity_conf->remember_list =
                eina_list_remove(productivity_conf->remember_list,
                                 rem);
          }
        if(rem->pid == 0)
          {
             productivity_conf->remember_list =
                eina_list_remove(productivity_conf->remember_list,
                                 rem);
          }
        if((bd->client.netwm.pid > 0) && 
           (bd->client.netwm.pid == rem->pid))
          {
             exists = EINA_TRUE;
          }
     }

   if(exists == EINA_TRUE) return;

   e_mod_config_window_remember_cleanup();
   if(rem)
     {
        if(rem->name)
          eina_stringshare_del(rem->name);
        if(rem->command)
          eina_stringshare_del(rem->command);
        if(rem->desktop_file)
          eina_stringshare_del(rem->desktop_file);

        rem = NULL;
     }
   else if(!rem)
     {
        rem = E_NEW(Remember, 1);
     }

   rem->name = eina_stringshare_add(bd->client.icccm.name);

   if((bd->client.icccm.command.argc > 0) &&
      (bd->client.icccm.command.argv))
     {
        rem->command = eina_stringshare_add(bd->client.icccm.command.argv[0]);
     }

   if(bd->client.netwm.pid > 0)
     {
        rem->pid = bd->client.netwm.pid;
     }
   rem->zone   = bd->zone->num;
   rem->desk_x = bd->desk->x;
   rem->desk_y = bd->desk->y;

   productivity_conf->remember_list = eina_list_append(
      productivity_conf->remember_list, rem);
}

static void
_e_mod_config_window_remember_get(E_Border *bd)
{
   Remember *rem;
   Eina_List *l;

   if(bd->client.netwm.pid <= 0) return;
   if(bd->client.icccm.command.argc <= 0) return;
   // if(!bd->iconic) return;

   EINA_LIST_FOREACH(productivity_conf->remember_list, l, rem)
     {
        if((bd->client.netwm.pid == rem->pid) &&
           (strncmp(rem->name, bd->client.icccm.name, sizeof(rem->name)) == 0))
          {
             E_Zone *zone;
             E_Desk *desk;

             zone = e_container_zone_number_get(bd->zone->container,
                                                rem->zone);
             desk = e_desk_at_xy_get(bd->zone, rem->desk_x, rem->desk_y);

             if(zone == bd->zone)
               e_border_zone_set(bd, zone);

             if(desk)
               e_border_desk_set(bd, desk);
          }
     }
   e_mod_config_window_remember_cleanup();
}

void
e_mod_config_window_remember_cleanup()
{
   Eina_List *l, *ll;
   Eina_List *bdl;
   Remember *rem;
   E_Border *bd;
   Eina_Bool pass = EINA_FALSE;

   bdl = eina_list_clone(e_border_client_list());

   EINA_LIST_FOREACH(productivity_conf->remember_list, l, rem)
     {     
        EINA_LIST_FOREACH(bdl, ll, bd)
          {
             if(bd->client.netwm.pid > 0)
               if(bd->client.netwm.pid == rem->pid)
                 pass = EINA_TRUE;
          }

        if(pass == EINA_FALSE)
          productivity_conf->remember_list = eina_list_remove(
             productivity_conf->remember_list, rem);
     }
   eina_list_free(bdl);
}

void
e_mod_config_window_remember_free_all()
{
   Remember *rem;

   if(!productivity_conf->remember_list) return;

   EINA_LIST_FREE(productivity_conf->remember_list, rem)
     {
        if(rem->name)
          eina_stringshare_del(rem->name);

        if(rem->command)
          eina_stringshare_del(rem->command);

        if(rem->desktop_file)
          eina_stringshare_del(rem->desktop_file);
     }
   E_FREE(productivity_conf->remember_list);
   productivity_conf->remember_list = NULL;
}

static void
_e_mod_config_window_remember_free(E_Border *bd)
{
   Remember *rem;
   Eina_List *l;

   if(!productivity_conf->remember_list) return;

   EINA_LIST_FOREACH(productivity_conf->remember_list, l, rem)
     {
        if(strncmp(rem->name, bd->client.icccm.name, sizeof(rem->name)) == 0)
          {
             if(rem->pid == bd->client.netwm.pid)
               {
                  if((bd->client.icccm.command.argc > 0) &&
                     (bd->client.icccm.command.argv))
                    {
                       if(strncmp(rem->command, bd->client.icccm.command.argv[0],
                                  sizeof(rem->command)) == 0)
                         {
                            productivity_conf->remember_list = eina_list_remove(
                               productivity_conf->remember_list, rem);
                         }
                    }
               }
          }
     }
}


static Eina_Bool
_e_mod_config_window_break_timer(void *data)
{
   E_Config_Window_List *cwl;
   Config *cfg;
   int sbx, sby; //sb = seconds to break, x= break_min_x, y= break_min_y

   if(!(cfg = productivity_conf)) return EINA_FALSE;
   if(!(cwl = data)) return EINA_FALSE;

   if(!cfg->cur_iv.lock) return EINA_TRUE;

   if(cfg->go_to_break == EINA_TRUE)
     goto break_time;

   if(!cfg->cur_iv.break_min_y) return EINA_TRUE;
   sby = cfg->cur_iv.break_min_y * 59;

   if(!cfg->secs_to_break)
     cfg->secs_to_break = sby;
   else
     cfg->secs_to_break--;

   if(cfg->secs_to_break < 10)
     DBG("Next Break in %d sec",cfg->secs_to_break);

   if(cfg->secs_to_break)
     return EINA_TRUE;

   if(!cfg->secs_to_break)
     {
        CRI("GO TO BREAK!");
        cfg->go_to_break = EINA_TRUE;
        e_mod_config_window_manager(cwl);
     }

break_time:

   if(!cfg->cur_iv.break_min_x) return EINA_TRUE;
   sbx = cfg->cur_iv.break_min_x * 59;

   if(!cfg->secs_to_break)
     cfg->secs_to_break = sbx;
   else
     cfg->secs_to_break--;

   if(cfg->secs_to_break < 10)
     WRN("Break will be over in %dsec", cfg->secs_to_break);

   if(cfg->secs_to_break == 5)
     {
        //Play sound to warn user break is over
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "mpg123 -q %s/data/button.mp3",
                 e_module_dir_get(cfg->module));
        ecore_exe_run(buf, NULL);

     }
   if(cfg->secs_to_break)
     return EINA_TRUE;

   if(!cfg->secs_to_break)
     {
        CRI("GO TO WORK!");
        cfg->go_to_break = EINA_FALSE;
        e_mod_config_window_manager(cwl);
     }
}

void e_mod_config_window_free(void)
{
   E_Config_Window_List *cwl;
   E_Config_Window_List_Data *cwldata;
   Ecore_Event_Handler *eh;
   E_Border *bd = NULL;

   if(!(cwl = productivity_conf->cwl)) return;
   WRN("FREEING E_Config_Window_List");

   EINA_LIST_FREE(productivity_conf->handlers, eh)
      ecore_event_handler_del(eh);

   EINA_LIST_FREE(cwl->borders, bd)
     {
        _e_mod_config_window_remember_get(bd);
        _e_mod_config_window_unhide(bd);
     }

   EINA_LIST_FREE(cwl->cwldata_list, cwldata)
     {
        if(cwldata->name)
          eina_stringshare_del(cwldata->name);

        if(cwldata->command)
          eina_stringshare_del(cwldata->command);
     }

   eina_list_free(cwl->borders);
   eina_list_free(cwl->urgent_window);

   E_FREE(bd);
   E_FREE(eh); 
   E_FREE(cwldata);
   E_FREE(cwl->cwldata);
   E_FREE(cwl);
}

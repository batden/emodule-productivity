#include <e.h>
#include "e_mod_config.h"

#define EVENT_DBG()
#define EVENT_DBG_()                                                                 \
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

#define MINF_(s)
#define MINF(s) \
   INF(s)



typedef struct _E_Config_Border_Data   E_Config_Border_Data;
typedef struct _E_Config_Urgent_Window E_Config_Urgent_Window;
typedef struct _E_Config_Remove_Window E_Config_Remove_Window;
struct _E_Config_Urgent_Window
{
   const char *name;
   const char *command;
   const char *class;
   int pid;
};

struct _E_Config_Remove_Window
{
   int pid;
};

E_Config_Remove_Window *crw = NULL;

struct _E_Config_Border_Data
{
   const char *name;
   const char *command;
   const char *class;
   const char *desktop_name;
   const char *desktop_orig_path;
   int pid;
   Eina_Bool iconic;
   Eina_Bool user_skip_winlist;
   Eina_Bool lock_user_iconify;
   Eina_Bool skip_pager;
   Eina_Bool urgent;
   Eina_Bool focus_out;

   struct 
     {
        int property;
        Eina_Bool hidden;
        Eina_Bool hidden_urgent;
        Eina_Bool was_urgent;
        Border_Event prev_action;
        Border_Event next_action;
     } private;

   struct
     {
        int zone;
        int desk_x;
        int desk_y;
     } rem;


};

E_Config_Border_Data *ev_border = NULL;

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
static void          _e_mod_config_window_remember_set(E_Config_Border_Data *bd);

static E_Config_Urgent_Window * _e_mod_config_window_urgent_set(E_Border *bd);

static Eina_Bool     _e_mod_config_window_urgent_match(Eina_List *urgent, E_Border *bd);
static void          _e_mod_config_window_urgent_remove(Eina_List *urgent, E_Border *bd);

static Eina_Bool        _e_mod_config_window_border_add(E_Config_Window_List *cwl, E_Border *bd);
static Eina_Bool        _e_mod_config_window_border_del(E_Config_Window_List *cwl, E_Config_Border_Data *cbd);
static Efreet_Desktop * _e_mod_config_window_border_find_desktop(E_Border *bd);
E_Config_Border_Data  * _e_mod_config_window_border_create(E_Border *bd);
static void             _e_mod_config_window_border_add_all(E_Config_Window_List *cwl);
static Eina_Bool        _e_mod_config_window_border_match(E_Config_Border_Data *cbd, E_Border *bd);
static Eina_Bool        _e_mod_config_window_border_worktool_match(E_Border *bd, Eina_List *worktools);
static void             _e_mod_config_window_border_urgent_set(E_Config_Window_List *cwl, E_Config_Border_Data *cbd);
static Eina_Bool        _e_mod_config_window_border_worktool_match_v2(E_Config_Border_Data *cbd, Eina_List *worktools);
static void             _e_mod_config_window_remember_show_all(Eina_List *cbd_lst, Eina_List *rem_lst);



Eina_Bool
e_mod_config_windows_create_data(void *data)
{
   Config *cfg;

   if(!(cfg = data)) return EINA_FALSE;

   cfg->cwl = E_NEW(E_Config_Window_List, 1);
   crw = E_NEW(E_Config_Remove_Window, 1);

   ev_border = E_NEW(E_Config_Border_Data, 1); 

   cfg->cwl->event = E_BORDER_NULL;
   _e_mod_config_window_border_add_all(cfg->cwl);
   e_mod_config_window_manager_v2(cfg->cwl);

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




   productivity_conf->wm = 
      ecore_timer_loop_add(0.50, e_mod_config_window_manager_v2, cfg->cwl);

   productivity_conf->brk = 
      ecore_timer_loop_add(1.00, _e_mod_config_window_break_timer, cfg->cwl);

   if(cfg->cwl->borders) return EINA_TRUE;

   return EINA_FALSE;
}
unsigned int
e_mod_timestamp_get()
{
   return _e_mod_config_windows_current_time_get();
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

/***********************************************************************************
  EVENT HANDLERS!!!
 ************************************************************************************/

static Eina_Bool
_e_mod_config_window_event_border_add_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;
   E_Config_Window_List *cwl;
   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   _e_mod_config_window_border_add(cwl, ev->border);

   ev_border = _e_mod_config_window_border_create(ev->border);
   cwl->event = E_BORDER_ADD;

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_remove_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   cwl->event = E_BORDER_REMOVE;
   _e_mod_config_window_border_del(cwl, ev_border);
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_iconify_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   //EVENT_DBG();

   cwl->event = E_BORDER_ICONIFY;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
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

   cwl->event = E_BORDER_UNICONIFY;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
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

   cwl->event = E_BORDER_FOCUS_IN;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_focus_out_cb(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   cwl->event = E_BORDER_FOCUS_OUT;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_property_cb(void *data,
                                              int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev;
   E_Border *border;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   cwl->event = E_BORDER_PROPERTY;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_event_border_urgent_change_cb(void *data,
                                                   int type __UNUSED__, void *event)
{
   E_Event_Border_Urgent_Change *ev;
   E_Border *border;
   E_Config_Window_List *cwl;

   if(!(cwl = data)) return;
   ev = event;
   EVENT_DBG();

   cwl->event = E_BORDER_URGENT;
   ev_border = _e_mod_config_window_border_create(ev->border);

   ecore_timer_freeze(productivity_conf->wm);
   e_mod_config_window_manager_v2(cwl);
   ecore_timer_thaw(productivity_conf->wm);
   return EINA_TRUE;
}
/**********************************************************************************
 * End of Handlers
 * ********************************************************************************/

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
_e_mod_config_window_remember_set(E_Config_Border_Data *cbd)
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
        if((cbd->pid > 0) && 
           (cbd->pid == rem->pid))
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

   rem->name = eina_stringshare_add(cbd->name);

   if(cbd->command)
     {
        rem->command = eina_stringshare_add(cbd->command);
     }

   if(cbd->pid > 0)
     {
        rem->pid = cbd->pid;
     }
   rem->zone   = cbd->rem.zone;
   rem->desk_x = cbd->rem.desk_x;
   rem->desk_y = cbd->rem.desk_y;

   productivity_conf->remember_list = eina_list_append(
      productivity_conf->remember_list, rem);
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

/*
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
   */

static Eina_Bool
_e_mod_config_window_break_timer(void *data)
{
   E_Config_Window_List *cwl;
   Config *cfg;
   int sbx, sby; //sb = seconds to break, x= break_min_x, y= break_min_y

   if(!(cfg = productivity_conf)) return EINA_FALSE;
   if(!(cwl = data)) return EINA_FALSE;

   if(cfg->init == E_MOD_PROD_STOPPED) return EINA_TRUE;

   if(cfg->init == E_MOD_PROD_BREAK)
     goto break_time;

   if(!cfg->break_min_y) return EINA_TRUE;
   sby = cfg->break_min_y * 59;

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
        MINF("GO TO BREAK!");
        productivity_conf->init = E_MOD_PROD_INIT_BREAK;

        ecore_timer_freeze(productivity_conf->wm);
        e_mod_config_window_manager_v2(cwl);
        ecore_timer_thaw(productivity_conf->wm);
     }

break_time:

   if(!cfg->break_min_x) return EINA_TRUE;
   sbx = cfg->break_min_x * 59;

   if(!cfg->secs_to_break)
     cfg->secs_to_break = sbx;
   else
     cfg->secs_to_break--;

   if(cfg->secs_to_break < 10)
     WRN("Break will be over in %dsec", cfg->secs_to_break);

   if((cfg->secs_to_break >= 4) && (cfg->secs_to_break <=5))
     {
        char buf[PATH_MAX];

        INF("Playing sound to warn user break is almost over");
        snprintf(buf, sizeof(buf), "mpg123 -q %s/data/button.mp3",
                 e_module_dir_get(cfg->module));
        ecore_exe_run(buf, NULL);
     }

   if(cfg->secs_to_break)
     return EINA_TRUE;

   if(!cfg->secs_to_break)
     {
        MINF("GO TO WORK!");

        productivity_conf->init = E_MOD_PROD_RESUME;

        ecore_timer_freeze(productivity_conf->wm);
        e_mod_config_window_manager_v2(cwl);
        ecore_timer_thaw(productivity_conf->wm);
     }
}

void e_mod_config_window_free(void)
{
   E_Config_Window_List *cwl;
   Ecore_Event_Handler *eh;
   E_Border *bd = NULL;

   if(!(cwl = productivity_conf->cwl)) return;
   WRN("FREEING E_Config_Window_List");

   EINA_LIST_FREE(productivity_conf->handlers, eh)
      ecore_event_handler_del(eh);
   /*
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
      */
   //eina_list_free(cwl->borders);
   //eina_list_free(cwl->urgent_window);

   //E_FREE(bd);
   E_FREE(eh); 
   //E_FREE(cwldata);
   //E_FREE(cwl->cwldata);
   E_FREE(cwl);
}

static E_Config_Urgent_Window *
_e_mod_config_window_urgent_set(E_Border *bd)
{
   Eina_List *urgent;
   E_Config_Urgent_Window *cuw;

   if(!(bd)) return;
   cuw = E_NEW(E_Config_Urgent_Window, 1);

   if(bd->client.icccm.name)
     cuw->name = eina_stringshare_add(bd->client.icccm.name);

   if(bd->client.icccm.class)
     cuw->class = eina_stringshare_add(bd->client.icccm.class);

   if((bd->client.icccm.command.argc > 0) && (bd->client.icccm.command.argv)) 
     cuw->command = eina_stringshare_add(bd->client.icccm.command.argv[0]);

   if(bd->client.netwm.pid > 0)
     cuw->pid = bd->client.netwm.pid;

   return cuw;
}

static Eina_Bool 
_e_mod_config_window_urgent_match(Eina_List *urgent, E_Border *bd)
{
   Eina_List *l;
   E_Config_Urgent_Window *cuw;
   int match = 0;

   if(!(urgent)) return;
   if(!(bd->client.icccm.name)) return;
   if(!(bd->client.icccm.class)) return;
   if(!(bd->client.icccm.command.argc > 0)) return;
   if(!(bd->client.icccm.command.argv)) return;
   if(!(bd->client.netwm.pid > 0)) return;

   EINA_LIST_FOREACH(urgent, l, cuw)
     {
        if(match > 0) match =  0;

        if(strncmp(cuw->name, bd->client.icccm.name, sizeof(cuw->name)) == 0)
          match += 1;

        if(strncmp(cuw->class, bd->client.icccm.class, sizeof(cuw->class)) == 0)
          match += 1;

        if(strncmp(cuw->command, bd->client.icccm.command.argv[0], sizeof(cuw->command)) == 0)
          match += 1;

        if(cuw->pid == bd->client.netwm.pid)
          match += 1;


        if(match == 4)
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

static void
_e_mod_config_window_urgent_remove(Eina_List *urgent, E_Border *bd)
{ 
   Eina_List *l;
   E_Config_Urgent_Window *cuw;
   int match = 0;

   if(!(urgent)) return;
   if(!(bd->client.icccm.name)) return;
   if(!(bd->client.icccm.class)) return;
   if(!(bd->client.icccm.command.argc > 0)) return;
   if(!(bd->client.icccm.command.argv)) return;
   if(!(bd->client.netwm.pid > 0)) return;

   EINA_LIST_FOREACH(urgent, l, cuw)
     {
        if(match > 0) match =  0;

        if(strncmp(cuw->name, bd->client.icccm.name, sizeof(cuw->name)) == 0)
          match += 1;

        if(strncmp(cuw->class, bd->client.icccm.class, sizeof(cuw->class)) == 0)
          match += 1;

        if(strncmp(cuw->command, bd->client.icccm.command.argv[0], sizeof(cuw->command)) == 0)
          match += 1;

        if(cuw->pid == bd->client.netwm.pid)
          match += 1;


        if(match == 4)
          productivity_conf->cwl->urgent = 
             eina_list_remove(productivity_conf->cwl->urgent, cuw);
     }
}

Eina_Bool 
e_mod_config_window_manager_v2(void *data)
{
   E_Config_Window_List *cwl;
   E_Config_Border_Data *cbd;
   Eina_List *l, *ll;
   E_Border *bd;

   if(!(cwl = data)) return EINA_FALSE;

   switch(productivity_conf->init)
     {
      case E_MOD_PROD_STOPPED:
         MINF("E_MOD_PROD_STOPPED");
         //Do cleanup when we stop.
         return EINA_TRUE;

      case E_MOD_PROD_STARTED:
         MINF("E_MOD_PROD_STARTED");
         //If we need to add this on first start?
         break;

      case E_MOD_PROD_BREAK:
         MINF("E_MOD_PROD_BREAK");
         //It's break time, do nothing, ??
         return EINA_TRUE;

      case E_MOD_PROD_RESUME:
         MINF("E_MOD_PROD_RESUME");
         productivity_conf->init = E_MOD_PROD_INIT_START;
         productivity_conf->brk =
            ecore_timer_loop_add(1.00, _e_mod_config_window_break_timer, cwl);
         return EINA_TRUE;

      case E_MOD_PROD_INIT_STOP:
         MINF("E_MOD_PROD_INIT_STOP");
         _e_mod_config_window_remember_show_all(
            cwl->borders, productivity_conf->remember_list);
         productivity_conf->init = E_MOD_PROD_STOPPED;
         return EINA_TRUE;

      case E_MOD_PROD_INIT_START:
         MINF("E_MOD_PROD_INIT_START");
         productivity_conf->init = E_MOD_PROD_STARTED;
         _e_mod_config_window_border_add_all(cwl);
         e_mod_config_window_manager_v2(cwl);
         return EINA_TRUE;

      case E_MOD_PROD_INIT_BREAK:
         MINF("E_MOD_PROD_INIT_BREAK");
         _e_mod_config_window_remember_show_all(
            cwl->borders, productivity_conf->remember_list);
         productivity_conf->init = E_MOD_PROD_BREAK;
         return EINA_TRUE;
     }

   switch(cwl->event)
     {
      case E_BORDER_ADD:
         MINF("E_BORDER_ADD");
         break;

      case E_BORDER_REMOVE:
         MINF("E_BORDER_REMOVE");
         _e_mod_config_window_border_del(cwl, ev_border);
         break;

      case E_BORDER_ICONIFY:
         MINF("E_BORDER_ICONIFY");
         if(!_e_mod_config_window_border_worktool_match_v2(
               ev_border, productivity_conf->apps_list))
           {
              _e_mod_config_window_remember_set(ev_border);
              e_config_save_queue();
           }
         break;

      case E_BORDER_UNICONIFY:
         MINF("E_BORDER_UNICONIFY");
         break;

      case E_BORDER_FOCUS_IN:
         MINF("E_BORDER_FOCUS_IN");
         break;

      case E_BORDER_FOCUS_OUT:
         MINF("E_BORDER_FOCUS_OUT");
         break;

      case E_BORDER_URGENT:
         MINF("E_BORDER_URGENT");

         //if((e_mod_config_schedule_urgent_get()) && (ev_border->urgent))
         _e_mod_config_window_border_urgent_set(cwl, ev_border);
      case E_BORDER_PROPERTY:
         MINF("E_BORDER_PROPERTY");
         if(ev_border->urgent)
           INF("Urgent:%d", ev_border->urgent);
         break;

      case E_BORDER_NULL:
         MINF("E_BORDER_NULL");
         break;

     }

   EINA_LIST_FOREACH(cwl->borders, l, cbd)
     {
        Eina_Bool match = EINA_FALSE;
        Eina_Bool worktool_match = EINA_FALSE;

        EINA_LIST_FOREACH(e_border_client_list(), ll, bd)
           if(_e_mod_config_window_border_match(cbd, bd))
             {
                match = EINA_TRUE;
                break;
             }
        if(match == EINA_FALSE) continue;

        if(!bd->desktop)
          bd->desktop = _e_mod_config_window_border_find_desktop(bd);

        if(bd->desktop)
          worktool_match =_e_mod_config_window_border_worktool_match(
             bd, productivity_conf->apps_list);


        if((cbd->private.was_urgent == EINA_TRUE ) && ((cwl->event == E_BORDER_FOCUS_IN) ||
                                                       (cwl->event == E_BORDER_NULL) || 
                                                       (cwl->event == E_BORDER_REMOVE)))
          {
             if(cbd->private.property >= 1)
               {
                  INF("Unhiding Urgent:%s ::%d", cbd->name, cbd->private.property);
                  WRN("Iconic:%d :: %d :: %d", cbd->iconic, ev_border->iconic, bd->iconic);

                  if((cbd->private.property == 1) && ((ev_border->iconic == 1) ||
                                                      (bd->iconic == 1)))
                    {
                       cwl->event = E_BORDER_NULL;
                       _e_mod_config_window_unhide(bd);
                    }
               }
          } 

        if((cwl->event == E_BORDER_PROPERTY) && (ev_border->urgent == EINA_TRUE) && 
           (worktool_match == EINA_FALSE) && (cbd->pid == ev_border->pid) &&
           (e_mod_config_schedule_urgent_get() == EINA_TRUE))
          {
             INF("Urgent Window:%s", cbd->name);
             cbd->private.property ++;
             cbd->private.was_urgent = EINA_TRUE;
          }

        if((worktool_match == EINA_FALSE) && ((cwl->event == E_BORDER_FOCUS_IN) ||
                                              (cwl->event == E_BORDER_REMOVE) ||
                                              (cwl->event == E_BORDER_NULL)))
          {
             if((cbd->private.was_urgent == EINA_FALSE) && (cbd->private.property <= 0))
               {
                  INF("Hiding:%s", cbd->name);
                  _e_mod_config_window_hide(bd);
               }
          }

        if((cbd->private.was_urgent == EINA_TRUE) && (cbd->private.property >= 2))
          {
             WRN("Time to un-urgent :%d", cbd->private.property);
             cwl->event == E_BORDER_NULL;
             cbd->private.property = 0;
             cbd->private.was_urgent = EINA_FALSE;
             cbd->private.prev_action = E_BORDER_NULL;
             e_mod_config_window_manager_v2(cwl);
          }

        if((cbd->pid == ev_border->pid) && (cbd->private.was_urgent == EINA_TRUE) && 
           (cbd->private.property >= 1) && (cbd->private.prev_action == E_BORDER_FOCUS_OUT))
          {
             DBG("Time to remove private");
             cbd->private.property++;
             cwl->event = E_BORDER_NULL;
             e_mod_config_window_manager_v2(cwl);
          }


        if((cbd->private.property == 1) && (bd->iconic == 1) && (cbd->pid == ev_border->pid))
          {
             cwl->event = E_BORDER_NULL;
             _e_mod_config_window_unhide(bd);
             e_mod_config_window_manager_v2(cwl);
          }

        cbd->private.prev_action = cwl->event;
     }

   if((cwl->event == E_BORDER_ICONIFY) || (cwl->event == E_BORDER_UNICONIFY))
     {
        cwl->event = E_BORDER_NULL;
        e_mod_config_window_manager_v2(cwl);
     }

   cwl->event = E_BORDER_NULL;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_border_add(E_Config_Window_List *cwl, E_Border *bd)
{
   E_Config_Border_Data *cbd;

   if(bd->client.icccm.name) //ignore borders that relates to e17
     if(strncmp(bd->client.icccm.name, "E", sizeof("E")) == 0)
       return EINA_FALSE;

   if(bd->client.icccm.class) //ignore borders that relates to e17
     if(strncmp(bd->client.icccm.class, "_config::", 8) == 0)
       return EINA_FALSE;

   if(!bd->desktop) return EINA_FALSE; //ignore border without .desktop file

   cbd = E_NEW(E_Config_Border_Data, 1);
   cbd = _e_mod_config_window_border_create(bd);

   cwl->borders = eina_list_append(cwl->borders, cbd);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_border_del(E_Config_Window_List *cwl, E_Config_Border_Data *cbd)
{
   E_Config_Border_Data *cbd_a;
   Eina_List *l;

   if(!(cbd)) return EINA_FALSE;

   EINA_LIST_FOREACH(cwl->borders, l, cbd_a)
      if(strncmp(cbd->name,cbd_a->name, sizeof(cbd->name)) == 0)
        if(strncmp(cbd->class, cbd_a->class, sizeof(cbd->class)) == 0)
          if(cbd->pid == cbd_a->pid)
            cwl->borders = eina_list_remove(cwl->borders, cbd_a);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_config_window_border_match(E_Config_Border_Data *cbd, E_Border *bd)
{
   Eina_List *l;

   if(!(cbd)) return EINA_FALSE;
   if(!(bd)) return EINA_FALSE;

   if(strncmp(cbd->name,bd->client.icccm.name, sizeof(cbd->name)) == 0)
     if(strncmp(cbd->class, bd->client.icccm.class, sizeof(cbd->class)) == 0)
       if(cbd->pid == bd->client.netwm.pid)
         return EINA_TRUE;

   return EINA_FALSE;
}

static void
_e_mod_config_window_border_urgent_set(E_Config_Window_List *cwl, E_Config_Border_Data *cbd)
{
   E_Config_Border_Data *cbd_a;
   Eina_List *l;

   if(!(cbd)) return;

   EINA_LIST_FOREACH(cwl->borders, l, cbd_a)
      if(strncmp(cbd->name,cbd_a->name, sizeof(cbd->name)) == 0)
        if(strncmp(cbd->class, cbd_a->class, sizeof(cbd->class)) == 0)
          if(cbd->pid == cbd_a->pid)
            cbd_a->urgent = EINA_TRUE;

   return;
}

E_Config_Border_Data *
_e_mod_config_window_border_create(E_Border *bd)
{
   E_Config_Border_Data *cbd;

   cbd = E_NEW(E_Config_Border_Data, 1);

   if(bd->client.icccm.name)
     cbd->name = eina_stringshare_add(bd->client.icccm.name);

   if(bd->client.icccm.class)
     cbd->class = eina_stringshare_add(bd->client.icccm.class);

   if(bd->client.netwm.pid > 0)
     cbd->pid = bd->client.netwm.pid;

   if(bd->desktop)
     {
        if(bd->desktop->name)
          cbd->desktop_name = eina_stringshare_add(bd->desktop->name);

        if(bd->desktop->orig_path)
          cbd->desktop_orig_path = eina_stringshare_add(bd->desktop->orig_path);
     }

   cbd->iconic             = bd->iconic;
   cbd->user_skip_winlist  = bd->user_skip_winlist;
   cbd->lock_user_iconify  = bd->lock_user_iconify;
   cbd->skip_pager         = bd->client.netwm.state.skip_pager;
   cbd->urgent             = bd->client.icccm.urgent;

   cbd->rem.zone           = bd->zone->num;
   cbd->rem.desk_x         = bd->desk->x;
   cbd->rem.desk_y         = bd->desk->y;

   return cbd;
}

static void
_e_mod_config_window_border_add_all(E_Config_Window_List *cwl)
{
   Eina_List *l;
   E_Border *bd;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if(!(bd)) continue; //ignore null borders

        _e_mod_config_window_border_add(cwl, bd);
     }
}

static Efreet_Desktop * 
_e_mod_config_window_border_find_desktop(E_Border *bd)
{
   Efreet_Desktop *desktop;

   desktop = efreet_util_desktop_exec_find(bd->client.icccm.name);

   if(!desktop)
     {
        int i = 0;
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%s",bd->client.icccm.name);

        while(buf[i] < (sizeof(buf) -1))
          {
             buf[i] = tolower(buf[i]);
             i++;
          }
        desktop = efreet_util_desktop_exec_find(buf);
     }

   if(!desktop)
     {
        int i = 0;
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%s",bd->client.icccm.class);

        while(buf[i] < (sizeof(buf) -1))
          {
             buf[i] = tolower(buf[i]);
             i++;
          }
        desktop = efreet_util_desktop_exec_find(buf);
     }

   if (!desktop)
     {
        if ((bd->client.icccm.name) && (bd->client.icccm.class))
          desktop = efreet_util_desktop_wm_class_find(bd->client.icccm.name,
                                                      bd->client.icccm.class);
     }

   if(!(desktop)) ERR("Unable to get a .desktop, giving up!");

   return desktop;
}

static Eina_Bool 
_e_mod_config_window_border_worktool_match(E_Border *bd, Eina_List *worktools)
{
   Efreet_Desktop *desktop;
   Eina_List *l;

   if(!(bd)) return EINA_FALSE;
   if(!(worktools)) return EINA_FALSE;

   EINA_LIST_FOREACH(worktools, l, desktop)
     {
        if(!desktop->name) continue;

        if ((strncmp(desktop->name,bd->desktop->name, sizeof(desktop->name)) == 0 ) &&
            (strncmp(desktop->orig_path,bd->desktop->orig_path, sizeof(desktop->orig_path)) == 0 ))
          {
             //DBG("Name:%s , Orig_Path:%s", desk->name, desk->orig_path);
             if(bd->client.netwm.pid > 0)
               return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static Eina_Bool 
_e_mod_config_window_border_worktool_match_v2(E_Config_Border_Data *cbd, Eina_List *worktools)
{
   Efreet_Desktop *desktop;
   Eina_List *l;

   if(!(cbd)) return EINA_FALSE;
   if(!(worktools)) return EINA_FALSE;

   EINA_LIST_FOREACH(worktools, l, desktop)
     {
        if(!desktop->name) continue;

        if ((strncmp(desktop->name,cbd->desktop_name, sizeof(desktop->name)) == 0 ) &&
            (strncmp(desktop->orig_path,cbd->desktop_orig_path, sizeof(desktop->orig_path)) == 0 ))
          {
             //DBG("Name:%s , Orig_Path:%s", desk->name, desk->orig_path);
             if(cbd->pid > 0)
               return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_config_window_border_hider(void *data)
{


   return EINA_TRUE;
}

static void 
_e_mod_config_window_remember_show_all(Eina_List *cbd_lst, Eina_List *rem_lst)
{
   E_Border *bd;
   E_Config_Border_Data *cbd;
   Remember *rem;
   Eina_List *l, *ll, *lll;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if(bd->iconic != EINA_TRUE) continue;

        EINA_LIST_FOREACH(rem_lst, ll, rem)
          {
             if(rem->pid == bd->client.netwm.pid)
               {
                  E_Zone *zone;
                  E_Desk *desk;

                  zone = e_container_zone_number_get(bd->zone->container,
                                                     rem->zone);
                  desk = e_desk_at_xy_get(zone, rem->desk_x, rem->desk_y);

                  _e_mod_config_window_unhide(bd);

                  if(bd->zone->num != rem->zone)
                    e_border_zone_set(bd, zone);

                  if(desk)
                    e_border_desk_set(bd, desk);

                  DBG("Rem:%s: zone:%d desk_x:%d desk_y:%d",rem->name, rem->zone,
                      rem->desk_x, rem->desk_y);
               }
          }
     }
}




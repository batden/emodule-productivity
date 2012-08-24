#include <e.h>
#include "e_mod_main.h"
#include "e_mod_config.h"

/* Local Function Prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);

static void _productivity_conf_new(void);
static void _productivity_conf_free(void);
static Eina_Bool _productivity_conf_timer(void *data);
static Config_Item *_productivity_conf_item_get(const char *id);
static void _productivity_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _productivity_cb_menu_post(void *data, E_Menu *menu);
static void _productivity_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _productivity_mod_menu_add(void *data, E_Menu *m);
static void _productivity_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);

static Month * _e_mod_main_month_conf_item_get();
static Day  _e_mod_main_day_conf_item_get();
static Intervals _e_mod_main_intervals_get();
static void _config_init();
static void _e_mod_main_get_current_config(Config *cfg);

/* Local Structures */
typedef struct _Instance Instance;
struct _Instance 
{
   /* An instance of our item (module) with its elements */

   /* pointer to this gadget's container */
   E_Gadcon_Client *gcc;

   /* evas_object used to display */
   Evas_Object *o_productivity;

   /* popup anyone ? */
   E_Menu *menu;

   /* Config_Item structure. Every gadget should have one :) */
   Config_Item *conf_item;
};

int _productivity_log;

/* Local Variables */
static Eina_List *instances = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
static E_Config_DD *month_edd = NULL;
static E_Config_DD *day_edd = NULL;
static E_Config_DD *intervals_edd = NULL;

Config *productivity_conf = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "productivity", 
   {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, 
      _gc_id_new, NULL, NULL},
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* We set the version and the name, check e_mod_main.h for more details */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Productivity"};

/*
 * Module Functions
 */

/* Function called when the module is initialized */
EAPI void *
e_modapi_init(E_Module *m) 
{
   char buf[PATH_MAX];
   Eina_List *apps;

   _productivity_log = eina_log_domain_register("E_MOD_PRODUCTIVITY",EINA_COLOR_CYAN);
   printf("PROD_LOG:%d\n", _productivity_log);
   eina_log_print_cb_set(e_mod_log_cb, NULL);
   eina_log_domain_level_set("E_MOD_PRODUCTIVITY", EINA_LOG_LEVEL_DBG);
   INF("Initialized Productivity Module");

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", m->dir);


   /* Display this Modules config info in the main Config Panel */

   /* starts with a category, create it if not already exists */
   e_configure_registry_category_add("advanced", 80, _("Advanced"), 
                                     NULL, "preferences-advanced");
   /* add right-side item */
   e_configure_registry_item_add("advanced/productivity", 110, _("Productivity"), 
                                 NULL, buf, e_int_config_productivity_module);

   _config_init();

   /* Tell E to find any existing module data. First run ? */
   productivity_conf = e_config_domain_load("module.productivity", conf_edd);

   if (productivity_conf) 
     {
        /* Check config version */
        if ((productivity_conf->version >> 16) < MOD_CONFIG_FILE_EPOCH) 
          {
             /* config too old */
             _productivity_conf_free();
             ecore_timer_add(1.0, _productivity_conf_timer,
                             _("Productivity Module Configuration data needed "
                               "upgrading. Your old configuration<br> has been"
                               " wiped and a new set of defaults initialized. "
                               "This<br>will happen regularly during "
                               "development, so don't report a<br>bug. "
                               "This simply means the module needs "
                               "new configuration<br>data by default for "
                               "usable functionality that your old<br>"
                               "configuration simply lacks. This new set of "
                               "defaults will fix<br>that by adding it in. "
                               "You can re-configure things now to your<br>"
                               "liking. Sorry for the inconvenience.<br>"));
          }

        /* Ardvarks */
        else if (productivity_conf->version > MOD_CONFIG_FILE_VERSION) 
          {
             /* config too new...wtf ? */
             _productivity_conf_free();
             ecore_timer_add(1.0, _productivity_conf_timer, 
                             _("Your Productivity Module configuration is NEWER "
                               "than the module version. This is "
                               "very<br>strange. This should not happen unless"
                               " you downgraded<br>the module or "
                               "copied the configuration from a place where"
                               "<br>a newer version of the module "
                               "was running. This is bad and<br>as a "
                               "precaution your configuration has been now "
                               "restored to<br>defaults. Sorry for the "
                               "inconvenience.<br>"));
          }
     }

   /* if we don't have a config yet, or it got erased above, 
    * then create a default one */
   if (!productivity_conf) _productivity_conf_new();

   _e_mod_main_get_current_config(productivity_conf);

   //After the config is loaded from the .cfg file, it's all in eina_lists
   //this function should assign what is in the list into our structures
   //but only for the current setting.

   /* create a link from the modules config to the module
    * this is not written */
   productivity_conf->module = m;

   productivity_conf->maug =
      e_int_menus_menu_augmentation_add_sorted("config/1", _("Productivity"),
                                               _productivity_mod_menu_add, NULL, NULL, NULL);

   e_module_delayed_set(m, 3);
   //Load all work applications into productivity_conf->apps.
   productivity_conf->apps = e_mod_config_worktools_selected_get();
   //Creates data, and adds callbacks
   e_mod_config_windows_create_data(NULL);

   /* Tell any gadget containers (shelves, etc) that we provide a module
    * for the user to enjoy */
   e_gadcon_provider_register(&_gc_class);

   /* Give E the module */
   return m;
}

/*
 * Function to unload the module
 */
EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   /* Unregister the config dialog from the main panel */
   e_configure_registry_item_del("advanced/productivity");

   /* Remove the config panel category if we can. E will tell us.
      category stays if other items using it */
   e_configure_registry_category_del("advanced");

   //Remove menu item
   if (productivity_conf->maug)
     e_int_menus_menu_augmentation_del("config/1", productivity_conf->maug);

   /* Kill the config dialog */
   if (productivity_conf->cfd) e_object_del(E_OBJECT(productivity_conf->cfd));
   productivity_conf->cfd = NULL;

   /* Tell E the module is now unloaded. Gets removed from shelves, etc. */
   productivity_conf->module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   /* Cleanup our item list */
   while (productivity_conf->conf_items) 
     {
        Config_Item *ci = NULL;

        /* Grab an item from the list */
        ci = productivity_conf->conf_items->data;

        /* remove it */
        productivity_conf->conf_items = 
           eina_list_remove_list(productivity_conf->conf_items, 
                                 productivity_conf->conf_items);

        /* cleanup stringshares */
        if (ci->id) eina_stringshare_del(ci->id);

        /* keep the planet green */
        E_FREE(ci);
     }

   /* Cleanup the main config structure */
   E_FREE(productivity_conf);

   /* Clean EET */
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);

   INF("Shutting down Productivity");
   eina_log_domain_unregister(_productivity_log);
   _productivity_log = -1;
   return 1;
}

/*
 * Function to Save the modules config
 */ 
EAPI int 
e_modapi_save(E_Module *m) 
{
   //ERR("SAVING!!!!!!!!!!!!");
   productivity_conf->timestamp = e_mod_timestamp_get();
   e_config_domain_save("module.productivity", conf_edd, productivity_conf);
   return 1;
}

/* Local Functions */

/* Called when Gadget Controller (gadcon) says to appear in scene */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst = NULL;
   char buf[PATH_MAX];

   /* theme file */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", 
            productivity_conf->module->dir);

   /* New visual instance, any config ? */
   inst = E_NEW(Instance, 1);
   inst->conf_item = _productivity_conf_item_get(id);

   /* create on-screen object */
   inst->o_productivity = edje_object_add(gc->evas);
   /* we have a theme ? */
   if (!e_theme_edje_object_set(inst->o_productivity, "base/theme/modules/productivity", 
                                "modules/productivity/main"))
     edje_object_file_set(inst->o_productivity, buf, "modules/productivity/main");

   /* Start loading our module on screen via container */
   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_productivity);
   inst->gcc->data = inst;

   /* hook a mouse down. we want/have a popup menu, right ? */
   evas_object_event_callback_add(inst->o_productivity, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _productivity_cb_mouse_down, inst);

   /* add to list of running instances so we can cleanup later */
   instances = eina_list_append(instances, inst);

   /* return the Gadget_Container Client */
   return inst->gcc;
}

/* Called when Gadget_Container says stop */
static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst = NULL;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);

   /* kill popup menu */
   if (inst->menu) 
     {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
   /* delete the visual */
   if (inst->o_productivity) 
     {
        /* remove mouse down callback hook */
        evas_object_event_callback_del(inst->o_productivity, EVAS_CALLBACK_MOUSE_DOWN, 
                                       _productivity_cb_mouse_down);
        evas_object_del(inst->o_productivity);
     }
   E_FREE(inst);
}

/* For when container says we are changing position */
static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

/* Gadget/Module label, name for our module */
static const char *
_gc_label(const E_Gadcon_Client_Class *client_class) 
{
   return _("Productivity");
}

/* so E can keep a unique instance per-container */
static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class) 
{
   Config_Item *ci = NULL;

   ci = _productivity_conf_item_get(NULL);
   return ci->id;
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas) 
{
   Evas_Object *o = NULL;
   char buf[PATH_MAX];

   /* theme */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", productivity_conf->module->dir);

   /* create icon object */
   o = edje_object_add(evas);

   /* load icon from theme */
   edje_object_file_set(o, buf, "icon");

   return o;
}

/* new module needs a new config :), or config too old and we need one anyway */
static void 
_productivity_conf_new(void) 
{
   Config_Item *ci = NULL;
   char buf[128];

   productivity_conf = E_NEW(Config, 1);
   productivity_conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((productivity_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   // IFMODCFG(0x008d);
   // _productivity_conf_item_get(NULL);
   CRI("CREATING NEW CONFIG!!!");
   _e_mod_main_month_conf_item_get();
   // IFMODCFGEND;

   /* update the version */
   productivity_conf->version = MOD_CONFIG_FILE_VERSION;

   /* setup limits on the config properties here (if needed) */

   /* save the config to disk */
   e_config_save_queue();
}

/* This is called when we need to cleanup the actual configuration,
 * for example when our configuration is too old */
static void 
_productivity_conf_free(void) 
{
   /* cleanup any stringshares here */
   while (productivity_conf->conf_items) 
     {
        Config_Item *ci = NULL;

        ci = productivity_conf->conf_items->data;
        productivity_conf->conf_items = 
           eina_list_remove_list(productivity_conf->conf_items, 
                                 productivity_conf->conf_items);
        /* EPA */
        if (ci->id) eina_stringshare_del(ci->id);
        E_FREE(ci);
     }

   E_FREE(productivity_conf);
}

/* timer for the config oops dialog (old configuration needs update) */
static Eina_Bool 
_productivity_conf_timer(void *data) 
{
   e_util_dialog_internal( _("Productivity Configuration Updated"), data);
   return EINA_FALSE;
}

/* function to search for any Config_Item struct for this Item
 * create if needed */
static Config_Item *
_productivity_conf_item_get(const char *id) 
{
   Config_Item *ci;

   GADCON_CLIENT_CONFIG_GET(Config_Item, productivity_conf->conf_items, _gc_class, id);

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->switch2 = 0;
   productivity_conf->conf_items = eina_list_append(productivity_conf->conf_items, ci);
   return ci;
}

static Month *
_e_mod_main_month_conf_item_get()
{
   Month *m;
   char buf[16];
   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);
   strftime(buf, 16, "%B", tm);

   m = E_NEW(Month, 1);
   m->name = eina_stringshare_add(buf);
   m->mon = tm->tm_mon;
   m->day = _e_mod_main_day_conf_item_get();
   m->day_list = eina_list_append(m->day_list, &m->day);

   m->day.iv = _e_mod_main_intervals_get();
   m->day.iv_list = eina_list_append(m->day.iv_list, &m->day.iv);

   productivity_conf->month_list =
      eina_list_append(productivity_conf->month_list, m);
   return m;
}

static Day 
_e_mod_main_day_conf_item_get()
{
   Day d;
   char buf[16];
   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);
   strftime(buf, 16, "%A", tm);

   d.name = eina_stringshare_add(buf);
   d.mday = tm->tm_mday;
   return d;
}

static Intervals  
_e_mod_main_intervals_get()
{
   Intervals iv;

   iv.id = 0;
   iv.lock = 0;
   iv.break_min = 10;
   iv.start.hour = 8;
   iv.start.min = 0;
   iv.start.sec = 0;
   iv.stop.hour = 16;
   iv.stop.min = 0;
   iv.stop.sec = 0;
   return iv;
}


/* Pants On */
static void 
_productivity_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Instance *inst = NULL;
   Evas_Event_Mouse_Down *ev;
   E_Zone *zone = NULL;
   E_Menu_Item *mi = NULL;
   int x, y;

   if (!(inst = data)) return;
   ev = event;
   if ((ev->button == 3) && (!inst->menu)) 
     {
        E_Menu *m;

        /* grab current zone */
        zone = e_util_zone_current_get(e_manager_current_get());

        /* create popup menu */
        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _productivity_cb_menu_configure, NULL);

        /* Each Gadget Client has a utility menu from the Container */
        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_menu_post_deactivate_callback_set(m, _productivity_cb_menu_post, inst);
        inst->menu = m;
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, 
                                          NULL, NULL);

        /* show the menu relative to gadgets position */
        e_menu_activate_mouse(m, zone, (x + ev->output.x), 
                              (y + ev->output.y), 1, 1, 
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button, 
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

/* popup menu closing, cleanup */
static void 
_productivity_cb_menu_post(void *data, E_Menu *menu) 
{
   Instance *inst = NULL;

   if (!(inst = data)) return;
   if (!inst->menu) return;
   e_object_del(E_OBJECT(inst->menu));
   inst->menu = NULL;
}

/* call configure from popup */
static void 
_productivity_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   if (!productivity_conf) return;
   if (productivity_conf->cfd) return;
   e_int_config_productivity_module(mn->zone->container, NULL);
}


/* menu item callback(s) */
static void 
_productivity_mod_run_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_registry_call("advanced/productivity", m->zone->container, NULL);
}

/* menu item add hook */
static void
_productivity_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Productivity"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _productivity_mod_run_cb, NULL);
}

static void
_config_init()
{
#undef T
#undef D
#define T Intervals
#define D intervals_edd
   intervals_edd = E_CONFIG_DD_NEW("Intervals", Intervals);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, lock, INT);
   E_CONFIG_VAL(D, T, break_min, INT);
   E_CONFIG_VAL(D, T, start.hour, INT);
   E_CONFIG_VAL(D, T, start.min, INT);
   E_CONFIG_VAL(D, T, start.sec, INT);
   E_CONFIG_VAL(D, T, stop.hour, INT);
   E_CONFIG_VAL(D, T, stop.min, INT);
   E_CONFIG_VAL(D, T, stop.sec, INT);

#undef T
#undef D
#define T Day
#define D day_edd
   day_edd = E_CONFIG_DD_NEW("Day", Day);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, mday, INT);
   E_CONFIG_VAL(D, T, total_time_worked, DOUBLE); 
   E_CONFIG_LIST(D, T, iv_list, intervals_edd);

#undef T
#undef D
#define T Month
#define D month_edd
   month_edd = E_CONFIG_DD_NEW("Month", Month);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, mon, INT);
   E_CONFIG_LIST(D, T, day_list, day_edd);

#undef T
#undef D
#define T Config
#define D conf_edd
   conf_edd = E_CONFIG_DD_NEW("Config", Config);
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, timestamp, UINT);
   E_CONFIG_LIST(D, T, month_list, month_edd);
   // E_CONFIG_LIST(D, T, day_list, day_edd);
   // E_CONFIG_LIST(D, T, iv_list, intervals_edd);
}

void
e_mod_main_reload_config()
{
   _e_mod_main_get_current_config(productivity_conf);
}

static void
_e_mod_main_get_current_config(Config *cfg)
{
   Eina_List *l, *ll, *lll;
   Eina_List *last;
   Month *m;
   Day *d;
   Intervals *iv;

   time_t tt;
   struct tm *tm;

   time(&tt);
   tm = localtime(&tt);

   EINA_LIST_FOREACH(cfg->month_list, l, m)
     {
        if (tm->tm_mon == m->mon)
          {
             INF("FOUND CURRENT Month:%s",m->name);
             cfg->cur_month.name = eina_stringshare_add(m->name);
             cfg->cur_month.mon = m->mon;

             EINA_LIST_FOREACH(m->day_list, ll, d)
               {
                  if (d->mday == tm->tm_mday)
                    {
                       cfg->cur_day.name = eina_stringshare_add(d->name);
                       cfg->cur_day.mday = d->mday;
                       cfg->cur_day.total_time_worked = d->total_time_worked;
                       /*cfg->cur_day.iv_list = eina_list_clone(
                         eina_list_last(d->iv_list));*/

                       INF("FOUND CURRENT Day:%s",d->name);
                       last = eina_list_last(d->iv_list);

                       EINA_LIST_FOREACH(last, lll, iv)
                         {
                            cfg->cur_iv.id = iv->id;
                            cfg->cur_iv.lock = iv->lock;
                            /*if(iv->lock == EINA_TRUE)
                              {
                              cfg->cur_iv.id += 1;
                            //cfg->cur_iv.lock = 0;
                            }*/
                            cfg->cur_iv.break_min = iv->break_min;
                            cfg->cur_iv.start.hour = iv->start.hour;
                            cfg->cur_iv.start.min = iv->start.min;
                            cfg->cur_iv.start.sec = iv->start.sec;
                            cfg->cur_iv.stop.hour = iv->stop.hour;
                            cfg->cur_iv.stop.min = iv->stop.min;
                            cfg->cur_iv.stop.sec = iv->stop.sec;
                         }
                    }
               }
          }
     }
   //eina_list_free(last);
   E_FREE(m);
   E_FREE(d);
   E_FREE(iv);
}

time_t to_seconds(const char *date)
{
        struct tm storage={0,0,0,0,0,0,0,0,0};
        char *p=NULL;
        time_t retval=0;

        p=(char *)strptime(date,"%d-%b-%Y",&storage);
        if(p==NULL)
        {
                retval=0;
        }
        else
        {
                retval=mktime(&storage);
        }
        return retval;
}

Eina_Bool
e_mod_main_is_it_time_to_work()
{
   Intervals *iv;

   if(!(iv = &productivity_conf->cur_iv)) return EINA_FALSE;

   time_t tt, start_t, stop_t, cur_t;
   struct tm *tm;
   struct tm stm;

   time(&tt);
   tm = localtime(&tt);

   if(iv->lock == EINA_FALSE)
     return EINA_FALSE;
   
   stm.tm_hour = iv->start.hour;
   stm.tm_min = iv->start.min;
   stm.tm_sec = iv->start.sec;
   stm.tm_year = tm->tm_year;
   stm.tm_mon = productivity_conf->cur_month.mon;
   stm.tm_mday = productivity_conf->cur_day.mday;
   start_t = mktime(&stm);

   stm.tm_hour = iv->stop.hour;
   stm.tm_min = iv->stop.min;
   stm.tm_sec = iv->stop.sec;
   stm.tm_year = tm->tm_year;
   stm.tm_mon = productivity_conf->cur_month.mon;
   stm.tm_mday = productivity_conf->cur_day.mday;
   stop_t = mktime(&stm);
   cur_t = mktime(tm);

   DBG("StartT:%d", start_t);
   DBG("Cur_T:%d", cur_t);
   DBG("StopT:%d", stop_t);
  
  if((cur_t > start_t) && (cur_t < stop_t))
   return EINA_TRUE;

   return EINA_FALSE;
}


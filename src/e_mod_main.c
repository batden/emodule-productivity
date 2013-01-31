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
static E_Config_DD *remember_edd = NULL;
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

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj", m->dir);

   /* Display this Modules config info in the main Config Panel */
   /* starts with a category, create it if not already exists */
   e_configure_registry_category_add("extensions", 80, D_("Extensions"),
                                     NULL, "preferences-extensions");
   /* add right-side item */
   e_configure_registry_item_add("extensions/productivity", 110, D_("Productivity"),
                                 NULL, buf, e_int_config_productivity_module);
#undef T
#undef D
#define T Remember
#define D remember_edd
   remember_edd = E_CONFIG_DD_NEW("Remember", Remember);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, command, STR);
   E_CONFIG_VAL(D, T, desktop_file, STR);
   E_CONFIG_VAL(D, T, win, UINT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
#undef T
#undef D
#define T Config
#define D conf_edd
   conf_edd = E_CONFIG_DD_NEW("Config", Config);
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, lock, INT);
   E_CONFIG_VAL(D, T, urgent, INT);
   E_CONFIG_VAL(D, T, break_min, INT);
   E_CONFIG_VAL(D, T, work_min, INT);
   E_CONFIG_LIST(D, T, remember_list, remember_edd);
#undef T
#undef D


   /* Tell E to find any existing module data. First run ? */
   productivity_conf = e_config_domain_load("module.productivity", conf_edd);

   if (productivity_conf) 
     {
        /* Check config version */
        if (!e_util_module_config_check(D_("Productivity"), productivity_conf->version, MOD_CONFIG_FILE_VERSION))
          _productivity_conf_free();
     }

   /* if we don't have a config yet, or it got erased above, 
    * then create a default one */
   if (!productivity_conf) _productivity_conf_new();
   productivity_conf->log_name = eina_stringshare_add("MOD:PROD");

   _productivity_log = eina_log_domain_register(
      productivity_conf->log_name,EINA_COLOR_CYAN);
   //eina_log_print_cb_set(e_mod_log_cb, NULL);
   eina_log_domain_level_set(
      productivity_conf->log_name, EINA_LOG_LEVEL_DBG);
   INF("Initialized Productivity Module");
  
   if(productivity_conf->lock == EINA_TRUE)
     productivity_conf->init = E_MOD_PROD_INIT_START;

   productivity_conf->module = m;

   productivity_conf->maug =
      e_int_menus_menu_augmentation_add_sorted("config/1", D_("Productivity"),
                                               _productivity_mod_menu_add, NULL, NULL, NULL);

   e_module_delayed_set(m, 3);
   //Load all work applications into productivity_conf->apps.
   productivity_conf->apps_list = e_mod_config_worktools_selected_get();

   //Creates data, and adds callbacks
   e_mod_config_windows_create_data(productivity_conf);

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
   e_configure_registry_item_del("extensions/productivity");

   /* Remove the config panel category if we can. E will tell us.
      category stays if other items using it */
   e_configure_registry_category_del("extensions");

   //Remove menu item
   if (productivity_conf->maug)
     e_int_menus_menu_augmentation_del("config/1", productivity_conf->maug);

   /* Kill the config dialog */
   if (productivity_conf->cfd) e_object_del(E_OBJECT(productivity_conf->cfd));
   productivity_conf->cfd = NULL;

   /* Tell E the module is now unloaded. Gets removed from shelves, etc. */
   productivity_conf->module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   ecore_timer_del(productivity_conf->wm);
   ecore_timer_del(productivity_conf->brk);
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

   //    E_Mod_Config_Window -- FREE
   e_mod_config_window_free();

   //    E_Mod_Config_Windows -- FREE -- END
   /* Cleanup the main config structure */
   E_FREE(productivity_conf);

   /* Clean EET */
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
   return D_("Productivity");
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
   /* setup defaults */
   if(!productivity_conf)
     {
        productivity_conf = E_NEW(Config, 1);
        productivity_conf->lock = 0;
        productivity_conf->urgent = 1;
        productivity_conf->break_min = 2;
        productivity_conf->work_min = 15;
     }

   /* update the version */
   productivity_conf->version = MOD_CONFIG_FILE_VERSION;

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
   e_util_dialog_internal( D_("Productivity Configuration Updated"), data);
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
   productivity_conf->conf_items = eina_list_append(productivity_conf->conf_items, ci);
   return ci;
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
        e_menu_item_label_set(mi, D_("Settings"));
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

   if ((ev->button == 1) && (!inst->menu))
     {
        E_Container *con = e_container_current_get(e_manager_current_get());
        e_int_config_productivity_module(con, NULL);
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
   e_configure_registry_call("extensions/productivity", m->zone->container, NULL);
}

/* menu item add hook */
static void
_productivity_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;
   char buf[PATH_MAX];

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, D_("Productivity"));
   snprintf(buf, sizeof(buf), "%s/e-module-productivity.edj",
            productivity_conf->module->dir);
   e_util_menu_item_theme_icon_set(mi, buf);
   e_menu_item_callback_set(mi, _productivity_mod_run_cb, NULL);
}


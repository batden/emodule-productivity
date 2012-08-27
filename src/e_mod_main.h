#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(string) dgettext(PACKAGE, string)
#else
# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,codeset)
# define _(string) (string)
#endif

#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(_productivity_log, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(_productivity_log, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(_productivity_log, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(_productivity_log, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(_productivity_log, __VA_ARGS__)

extern int _productivity_log;
/* Macros used for config file versioning */
/* You can increment the EPOCH value if the old configuration is not
 * compatible anymore, it creates an entire new one.
 * You need to increment GENERATION when you add new values to the
 * configuration file but is not needed to delete the existing conf  */
#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x008e
#define MOD_CONFIG_FILE_VERSION \
   ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

/* More mac/def; Define your own. What do you need ? */
#define CONN_DEVICE_ETHERNET 0

/* We create a structure config for our module, and also a config structure
 * for every item element (you can have multiple gadgets for the same module) */

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;
typedef struct _Config_Schedule_Start Config_Schedule_Start;

typedef struct _Month Month;
typedef struct _Day   Day;
typedef struct _Intervals Intervals;
typedef struct _Remember Remember;

struct _Intervals
{
   int lock;
   int id;
   int break_min;
   struct
     {
        int hour;
        int min;
        int sec;
     }start, stop;
};

struct _Day
{
   const char *name;
   int mday;
   double total_time_worked;
   Intervals iv;
   Eina_List *iv_list;
};


struct _Month
{
   const char *name;
   int mon;
   Day day;
   Eina_List *day_list;
};

struct _Remember
{
   const char *name;
   const char *command;
   const char *desktop_file;
   int pid;
   int zone;
   int desk_x;
   int desk_y;
};

/* Base config struct. Store Item Count, etc
 * 
 * *module (not written to disk) (E Interaction)
 * *cfd (not written to disk) (config dialog)
 * 
 * Store list of your items that you want to keep. (sorting)
 * Can define per-module config properties here.
 * 
 * Version used to know when user config too old */
struct _Config 
{
   E_Module *module;
   E_Int_Menu_Augmentation *maug;
   E_Config_Dialog *cfd;
   Eina_List *conf_items;
   int version;
   unsigned int timestamp;

   /*Work application list*/
   Eina_List *apps;

   Eina_List *iv_list;
   Eina_List *day_list;
   Eina_List *month_list;
   Eina_List *remember_list;
   Intervals iv;
   Day day;
   Month month;
   
   Month cur_month;
   Day cur_day;
   Intervals cur_iv;
};

/* This struct used to hold config for individual items from above list */
struct _Config_Item 
{
   /* unique id for every running gadget, this is managed by gadcon */
   const char *id;

   /* actual config properties independently for every running gadget. */
   int switch2;
};

/* Setup the E Module Version, Needed to check if module can run. */
/* The version is stored at compilation time in the module, and is checked
 * by E in order to know if the module is compatible with the actual version */
EAPI extern E_Module_Api e_modapi;

/* E API Module Interface Declarations
 *
 * e_modapi_init:     it is called when e17 initialize the module, note that
 *                    a module can be loaded but not initialized (running)
 *                    Note that this is not the same as _gc_init, that is called
 *                    when the module appears on his container
 * e_modapi_shutdown: it is called when e17 is closing, so calling the modules
 *                    to finish
 * e_modapi_save:     this is called when e17 or by another reason is requeested
 *                    to save the configuration file                      */
EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

/* Function for calling the module's Configuration Dialog */
E_Config_Dialog *e_int_config_productivity_module(E_Container *con, const char *params);

void e_mod_log_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc, int line, const char *fmt, void *data, va_list args);

extern Config *productivity_conf;
void e_mod_main_reload_config();
Eina_Bool e_mod_main_is_it_time_to_work();
//          e_mod_config_windows.c
Eina_Bool tasks_cb_window_focus_in(void *data, int type, void *event);


#endif

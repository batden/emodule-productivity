#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef ENABLE_NLS
# include <libintl.h>
# define D_(string) dgettext(PACKAGE, string)
#else
# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,codeset)
# define D_(string) (string)
#endif
#define N_(str) (str)

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
#define MOD_CONFIG_FILE_EPOCH 1
#define MOD_CONFIG_FILE_GENERATION 0
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

/* We create a structure config for our module, and also a config structure
 * for every item element (you can have multiple gadgets for the same module) */

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;
typedef struct _Remember Remember;
typedef struct _E_Config_Window_List E_Config_Window_List;

typedef enum _Border_Event
{
   E_BORDER_NULL = -1,
   E_BORDER_ADD = 0,
   E_BORDER_REMOVE = 1,
   E_BORDER_ICONIFY = 2,
   E_BORDER_UNICONIFY = 3,
   E_BORDER_FOCUS_IN = 4,
   E_BORDER_FOCUS_OUT = 5,
   E_BORDER_URGENT = 6,
   E_BORDER_PROPERTY = 7,
} Border_Event;

typedef enum _Initialize
{
   E_MOD_PROD_STOPPED = 0,
   E_MOD_PROD_STARTED = 1,
   E_MOD_PROD_BREAK = 3,
   E_MOD_PROD_INIT_STOP = 4,
   E_MOD_PROD_INIT_START = 5,
   E_MOD_PROD_INIT_BREAK = 6,
   E_MOD_PROD_RESUME = 7,
} Initialize;

struct _Remember
{
   const char *name;
   const char *command;
   const char *desktop_file;
   Ecore_X_Window win;
   int zone;
   int desk_x;
   int desk_y;
};

//    e_mod_config_windows.c
struct _E_Config_Window_List
{
   Eina_List *borders;
   Eina_List *cwldata_list;
   Eina_List *urgent;
   Border_Event event;
   Border_Event previous_event;
   E_Border *ev_border;
};

struct _Config 
{
   E_Module *module;
   E_Int_Menu_Augmentation *maug;
   E_Config_Dialog *cfd;
   Eina_List *conf_items;
   
   int version;
   int lock;
   int urgent;
   int break_min;
   int work_min;
   
   Ecore_Timer *brk;
   Ecore_Timer *wm;
   int secs_to_break;
   Eina_Bool go_to_break;

   /*Work application list*/
   Eina_List *apps_list;
   Eina_List *remember_list;
   Eina_List *handlers;

   // e_mod_config_windows.c
   E_Config_Window_List *cwl;
   Initialize init;
   Initialize previous_init;
   const char *log_name;
};

struct _Config_Item 
{
   const char *id;
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

//    e_mod_config_windows.c
void         e_mod_config_window_manager(E_Config_Window_List *cwl);
Eina_Bool    e_mod_config_window_manager_v2(void *data);
void         e_mod_config_window_remember_cleanup();
void         e_mod_config_window_free(void);
void         e_mod_config_window_remember_free_all(void);
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define IBM 1
#else
    #define APL 1
#endif

#define XPLM200
#define XPLM300

#include "XPLMPlugin.h"
// ... далі весь твій код без змін ...
#define XPLM200
#define XPLM300

#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector> // Додано для динамічних масивів

// ----------------------------------------------------
// ГЛОБАЛЬНІ ЗМІННІ ДЛЯ ЗВ'ЯЗКУ З LUA
// ----------------------------------------------------
XPLMDataRef g_min_wind_ref = NULL;
XPLMDataRef g_max_wind_ref = NULL;
XPLMDataRef g_max_turb_ref = NULL;
XPLMDataRef g_wind_dir_ref = NULL;
XPLMDataRef g_rand_dir_ref = NULL;
XPLMDataRef g_trigger_ref  = NULL;

float g_min_wind = 5.0f;
float g_max_wind = 20.0f;
float g_max_turb = 3.0f;
float g_wind_dir = 0.0f;
int   g_rand_dir = 1;
int   g_trigger  = 0;

// РЕГІОНАЛЬНІ DATAREFS X-PLANE 12
XPLMDataRef xp_change_mode  = NULL;
XPLMDataRef xp_update_immed = NULL;
XPLMDataRef xp_wind_dir     = NULL;
XPLMDataRef xp_wind_spd     = NULL;
XPLMDataRef xp_turb         = NULL;

// ГЕТТЕРИ ТА СЕТТЕРИ
float GetMinWind(void* inRefcon) { return g_min_wind; }
void  SetMinWind(void* inRefcon, float inValue) { g_min_wind = inValue; }
float GetMaxWind(void* inRefcon) { return g_max_wind; }
void  SetMaxWind(void* inRefcon, float inValue) { g_max_wind = inValue; }
float GetMaxTurb(void* inRefcon) { return g_max_turb; }
void  SetMaxTurb(void* inRefcon, float inValue) { g_max_turb = inValue; }
float GetWindDir(void* inRefcon) { return g_wind_dir; }
void  SetWindDir(void* inRefcon, float inValue) { g_wind_dir = inValue; }
int   GetRandDir(void* inRefcon) { return g_rand_dir; }
void  SetRandDir(void* inRefcon, int inValue) { g_rand_dir = inValue; }
int   GetTrigger(void* inRefcon) { return g_trigger; }
void  SetTrigger(void* inRefcon, int inValue) { g_trigger = inValue; }

// ----------------------------------------------------
// ГОЛОВНИЙ ЦИКЛ ОБРОБКИ ПОГОДИ
// ----------------------------------------------------
float WeatherRandomizerLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon) {
    if (g_trigger == 1) {
        
        // ЗАХИСТ: Перевіряємо, чи існують потрібні датарефи
        if (!xp_wind_spd || !xp_wind_dir) {
            g_trigger = 0;
            return 0.1f;
        }

        // 1. ДИНАМІЧНИЙ РОЗМІР: Дізнаємось точну кількість шарів погоди в цій версії XP12
        int num_layers = XPLMGetDatavf(xp_wind_spd, NULL, 0, 0);
        if (num_layers <= 0) num_layers = 14; // Резервний варіант, якщо щось піде не так

        // 2. ГЕНЕРУЄМО ДАНІ (м/с)
        float random_fraction = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float final_dir = (g_rand_dir == 1) ? static_cast<float>(rand() % 360) : g_wind_dir;
        
        float rand_spd_ms = g_min_wind + (random_fraction * (g_max_wind - g_min_wind));
        float rand_turb = (random_fraction * g_max_turb) / 10.0f;

        // ПРИМІТКА: XP12 датареф _msc означає Meters per Second.
        // Якщо раптом у симуляторі вітер знову буде занадто малим (Laminar інколи плутають одиниці всередині),
        // просто допиши тут: float spd_for_sim = rand_spd_ms * 1.94384f;
        float spd_for_sim = rand_spd_ms * 1.94384f;
        
        // 3. Створюємо динамічні масиви ідеально точного розміру
        std::vector<float> dir_val(num_layers, final_dir);
        std::vector<float> spd_val(num_layers, spd_for_sim);
        std::vector<float> turb_val(num_layers, rand_turb);

        // 4. ЗАСТОСОВУЄМО ПОГОДУ
        if (xp_change_mode)  XPLMSetDatai(xp_change_mode, 3); // 3 = Режим Static Weather
        if (xp_update_immed) XPLMSetDatai(xp_update_immed, 1); // 1 = Миттєве застосування

        // Використовуємо .data() для передачі вказівника з вектора у С-стиль API
        XPLMSetDatavf(xp_wind_dir, dir_val.data(), 0, num_layers);
        XPLMSetDatavf(xp_wind_spd, spd_val.data(), 0, num_layers);
        if (xp_turb) XPLMSetDatavf(xp_turb, turb_val.data(), 0, num_layers);

        // Вимикаємо прапорець миттєвого оновлення
        if (xp_update_immed) XPLMSetDatai(xp_update_immed, 0);

        g_trigger = 0; // Скидаємо тригер
    }
    return 0.1f;
}

// ----------------------------------------------------
// ОБОВ'ЯЗКОВІ ФУНКЦІЇ ПЛАГІНА
// ----------------------------------------------------
PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc) {
    strcpy(outName, "RECON Weather Backend");
    strcpy(outSig, "recon.weather.backend");
    strcpy(outDesc, "C++ Core for handling random weather scenarios via Lua");

    srand((unsigned int)time(NULL));

    // ВИПРАВЛЕНІ НАЗВИ РЕГІОНАЛЬНИХ DATAREFS ДЛЯ XP12
    xp_change_mode  = XPLMFindDataRef("sim/weather/region/change_mode");
    xp_update_immed = XPLMFindDataRef("sim/weather/region/update_immediately");
    xp_wind_dir     = XPLMFindDataRef("sim/weather/region/wind_direction_degt"); // БУЛА ПОМИЛКА: wind_dir_degt
    xp_wind_spd     = XPLMFindDataRef("sim/weather/region/wind_speed_msc");
    xp_turb         = XPLMFindDataRef("sim/weather/region/turbulence");

    // Реєструємо канали зв'язку для Lua
    g_min_wind_ref = XPLMRegisterDataAccessor("recon/weather/min_wind", xplmType_Float, 1, NULL, NULL, GetMinWind, SetMinWind, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    g_max_wind_ref = XPLMRegisterDataAccessor("recon/weather/max_wind", xplmType_Float, 1, NULL, NULL, GetMaxWind, SetMaxWind, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    g_max_turb_ref = XPLMRegisterDataAccessor("recon/weather/max_turb", xplmType_Float, 1, NULL, NULL, GetMaxTurb, SetMaxTurb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    g_wind_dir_ref = XPLMRegisterDataAccessor("recon/weather/wind_dir", xplmType_Float, 1, NULL, NULL, GetWindDir, SetWindDir, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    g_rand_dir_ref = XPLMRegisterDataAccessor("recon/weather/rand_dir", xplmType_Int,   1, GetRandDir, SetRandDir, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    g_trigger_ref  = XPLMRegisterDataAccessor("recon/weather/trigger",  xplmType_Int,   1, GetTrigger, SetTrigger, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    XPLMUnregisterDataAccessor(g_min_wind_ref);
    XPLMUnregisterDataAccessor(g_max_wind_ref);
    XPLMUnregisterDataAccessor(g_max_turb_ref);
    XPLMUnregisterDataAccessor(g_wind_dir_ref);
    XPLMUnregisterDataAccessor(g_rand_dir_ref);
    XPLMUnregisterDataAccessor(g_trigger_ref);
}

PLUGIN_API void XPluginDisable(void) { XPLMUnregisterFlightLoopCallback(WeatherRandomizerLoop, NULL); }
PLUGIN_API int XPluginEnable(void) { XPLMRegisterFlightLoopCallback(WeatherRandomizerLoop, 0.1f, NULL); return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) {}

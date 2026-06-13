dataref("ReconMinWind", "recon/weather/min_wind", "writable")
dataref("ReconMaxWind", "recon/weather/max_wind", "writable")
dataref("ReconMaxTurb", "recon/weather/max_turb", "writable")
dataref("ReconWindDir", "recon/weather/wind_dir", "writable")
dataref("ReconRandDir", "recon/weather/rand_dir", "writable")
dataref("ReconTrigger", "recon/weather/trigger", "writable")
dataref("ReconUIOpen", "recon/weather/ui_open", "writable") -- Зв'язуємо новий датареф стану вікна

Recon_wnd = nil

-- Тимчасові дефолтні значення для слайдерів (якщо пам'ять C++ порожня)
Recon_slider_min = 1.0
Recon_slider_max = 8.0
Recon_slider_turb = 3.0
Recon_rand_active = true
Recon_fixed_dir = 0.0

-- ФУНКЦІЯ СИНХРОНІЗАЦІЇ ПАМ'ЯТІ (Спрацьовує один раз при завантаженні/перезавантаженні скрипта)
local initialized = false
function init_recon_defaults_from_ram()
    if not initialized then
        -- Якщо С++ плагін уже має в пам'яті якісь змінені користувачем дані, беремо їх
        if ReconMinWind and ReconMinWind > 0 then
            Recon_slider_min = ReconMinWind
            Recon_slider_max = ReconMaxWind
            Recon_slider_turb = ReconMaxTurb
            Recon_rand_active = (ReconRandDir == 1)
            Recon_fixed_dir = ReconWindDir
        end
        initialized = true
    end
end

function build_recon_weather_window(wnd, x, y)
    init_recon_defaults_from_ram()
    
    imgui.TextUnformatted("Weather Generator (m/s):")
    imgui.Separator()
    
    -- Кожна зміна повзунка тепер ОДРАЗУ зберігається в пам'ять C++ плагіна
    local c1, v1 = imgui.SliderFloat("Min Wind (m/s)", Recon_slider_min, 0.0, 50.0, "%.1f")
    if c1 then 
        Recon_slider_min = v1 
        ReconMinWind = v1 
    end
    
    local c2, v2 = imgui.SliderFloat("Max Wind (m/s)", Recon_slider_max, 0.0, 50.0, "%.1f")
    if c2 then 
        Recon_slider_max = v2 
        ReconMaxWind = v2 
    end
    
    local c3, v3 = imgui.SliderFloat("Max Turbulence", Recon_slider_turb, 0.0, 10.0, "%.1f")
    if c3 then 
        Recon_slider_turb = v3 
        ReconMaxTurb = v3 
    end
    
    imgui.Spacing()
    imgui.Separator()
    imgui.Spacing()
    
    local c_rand, v_rand = imgui.Checkbox("Randomize Direction", Recon_rand_active)
    if c_rand then 
        Recon_rand_active = v_rand 
        ReconRandDir = v_rand and 1 or 0
    end
    
    if not Recon_rand_active then
        local c4, v4 = imgui.SliderFloat("Fixed Dir (deg)", Recon_fixed_dir, 0.0, 360.0, "%.0f")
        if c4 then 
            Recon_fixed_dir = v4 
            ReconWindDir = v4
        end
    end
    
    imgui.Spacing()
    imgui.Separator()
    imgui.Spacing()
    
    if imgui.Button("Generate Random Weather", 250, 30) then
        ReconMinWind = Recon_slider_min
        ReconMaxWind = Recon_slider_max
        ReconMaxTurb = Recon_slider_turb
        ReconRandDir = Recon_rand_active and 1 or 0
        ReconWindDir = Recon_fixed_dir
        ReconTrigger = 1
    end
end

-- Коли користувач закриває віконце СЕРЕД СЕСІЇ (хрестиком), ми кажемо C++, що відкривати його знову не треба
function recon_wnd_on_close(wnd)
    Recon_wnd = nil
    ReconUIOpen = 0
end

function toggle_recon_ui()
    init_recon_defaults_from_ram()
    if Recon_wnd == nil then
        Recon_wnd = float_wnd_create(350, 270, 1, true)
        float_wnd_set_title(Recon_wnd, "RECON Weather Controller")
        float_wnd_set_imgui_builder(Recon_wnd, "build_recon_weather_window")
        float_wnd_set_onclose(Recon_wnd, "recon_wnd_on_close")
        ReconUIOpen = 1 -- Запам'ятовуємо, що вікно відкрите
    else
        float_wnd_destroy(Recon_wnd)
        Recon_wnd = nil
        ReconUIOpen = 0 -- Запам'ятовуємо, що вікно закрите
    end
end

-- ЦИКЛ СЛІДКУВАННЯ: Якщо літак впав, X-Plane видалив вікно, але в C++ ReconUIOpen досі дорівнює 1.
-- Ця функція побачить це і миттєво відновить віконце з твоїми збереженими повзунками!
function check_recon_ui_persistence()
    init_recon_defaults_from_ram()
    if ReconUIOpen == 1 and Recon_wnd == nil then
        Recon_wnd = float_wnd_create(350, 270, 1, true)
        float_wnd_set_title(Recon_wnd, "RECON Weather Controller")
        float_wnd_set_imgui_builder(Recon_wnd, "build_recon_weather_window")
        float_wnd_set_onclose(Recon_wnd, "recon_wnd_on_close")
    end
end

-- Перевіряємо стан вікна часто (декілька разів на секунду)
do_often("check_recon_ui_persistence()")

create_command("FlyWithLua/recon/toggle_weather_ui", "Toggle RECON Weather UI", "toggle_recon_ui()", "", "")
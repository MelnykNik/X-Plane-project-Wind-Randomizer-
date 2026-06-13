dataref("ReconMinWind", "recon/weather/min_wind", "writable")
dataref("ReconMaxWind", "recon/weather/max_wind", "writable")
dataref("ReconMaxTurb", "recon/weather/max_turb", "writable")
dataref("ReconWindDir", "recon/weather/wind_dir", "writable")
dataref("ReconRandDir", "recon/weather/rand_dir", "writable")
dataref("ReconTrigger", "recon/weather/trigger", "writable")

Recon_wnd = nil
Recon_slider_min = 5.0
Recon_slider_max = 20.0
Recon_slider_turb = 3.0
Recon_rand_active = true
Recon_fixed_dir = 0.0

function build_recon_weather_window(wnd, x, y)
    imgui.TextUnformatted("Weather Generator (m/s):")
    imgui.Separator()
    
    local c1, v1 = imgui.SliderFloat("Min Wind (m/s)", Recon_slider_min, 0.0, 50.0, "%.1f")
    if c1 then Recon_slider_min = v1 end
    
    local c2, v2 = imgui.SliderFloat("Max Wind (m/s)", Recon_slider_max, 0.0, 50.0, "%.1f")
    if c2 then Recon_slider_max = v2 end
    
    local c3, v3 = imgui.SliderFloat("Max Turbulence", Recon_slider_turb, 0.0, 10.0, "%.1f")
    if c3 then Recon_slider_turb = v3 end
    
    imgui.Spacing()
    imgui.Separator()
    imgui.Spacing()
    
    local c_rand, v_rand = imgui.Checkbox("Randomize Direction", Recon_rand_active)
    if c_rand then Recon_rand_active = v_rand end
    
    if not Recon_rand_active then
        local c4, v4 = imgui.SliderFloat("Fixed Dir (deg)", Recon_fixed_dir, 0.0, 360.0, "%.0f")
        if c4 then Recon_fixed_dir = v4 end
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

function recon_wnd_on_close(wnd)
    Recon_wnd = nil
end

function toggle_recon_ui()
    if Recon_wnd == nil then
        Recon_wnd = float_wnd_create(350, 270, 1, true)
        float_wnd_set_title(Recon_wnd, "RECON Weather Controller")
        float_wnd_set_imgui_builder(Recon_wnd, "build_recon_weather_window")
        float_wnd_set_onclose(Recon_wnd, "recon_wnd_on_close")
    else
        float_wnd_destroy(Recon_wnd)
        Recon_wnd = nil
    end
end

create_command("FlyWithLua/recon/toggle_weather_ui", "Toggle RECON Weather UI", "toggle_recon_ui()", "", "")
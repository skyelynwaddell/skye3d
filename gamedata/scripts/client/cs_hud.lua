-- cs_hud.lua  — Lua-driven pause / options menus.
-- All locals are declared first so closures in item tables capture
-- the correct upvalues rather than stale nils.

-- ============================================================
--  State (declared before any closures reference them)
-- ============================================================
local pause_open    = false
local options_open  = false

local pause_state   = PanelState.new()
pause_state.x       = 24
pause_state.y       = 24
pause_state.w       = 180

local options_state = PanelState.new()
options_state.x     = 220
options_state.y     = 24
options_state.w     = 420
options_state.h     = 400

-- ============================================================
--  toggle_pause  (declared before item closures reference it)
-- ============================================================
local function toggle_pause()
  pause_open = not pause_open
  if not pause_open then
    options_open = false
  end
  set_menu_mode(pause_open)
end

-- ============================================================
--  Pause menu items
-- ============================================================
local pause_items = {
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Resume Game"
    i.onpress = function() toggle_pause() end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Save Game"
    i.onpress = function() print("save") end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Load Game"
    i.onpress = function() print("load") end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "New Game"
    i.onpress = function() print("new game") end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Find servers"
    i.onpress = function() print("find servers") end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Create server"
    i.onpress = function() print("create server") end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Options"
    i.onpress = function() options_open = not options_open end; return i
  end)(),
  (function()
    local i = MenuItem.new(); i.type = MENU.BUTTON; i.text = "Quit"
    i.onpress = function() print("quit") end; return i
  end)(),
}

-- ============================================================
--  Options items
-- ============================================================
local function new_item(t, text)
  local i = MenuItem.new()
  i.type = t
  i.text = text or ""
  return i
end

local master              = new_item(MENU.SLIDERBAR, "Master Volume")
master.value_f            = 0.75
master.min_f              = 0.0
master.max_f              = 1.0

local music               = new_item(MENU.SLIDERBAR, "Music")
music.value_f             = 0.75
music.min_f               = 0.0
music.max_f               = 1.0

local sfx                 = new_item(MENU.SLIDERBAR, "SFX")
sfx.value_f               = 0.75
sfx.min_f                 = 0.0
sfx.max_f                 = 1.0

local vocals              = new_item(MENU.SLIDERBAR, "Vocals")
vocals.value_f            = 0.75
vocals.min_f              = 0.0
vocals.max_f              = 1.0

local brightness          = new_item(MENU.SLIDERBAR, "Brightness")
brightness.value_f        = 0.5
brightness.min_f          = 0.0
brightness.max_f          = 1.0

local vsync               = new_item(MENU.CHECKBOX, "VSync")
vsync.value_b             = true

local anti_aliasing       = new_item(MENU.CHECKBOX, "MSAA 4X")
anti_aliasing.value_b     = true

local quality             = new_item(MENU.DROPDOWN, "Low Quality;Medium Quality;High Quality")
quality.value_i           = 1

local resolution          = new_item(MENU.DROPDOWN, "640x480;1280x720;1280x960;1920x1080;2560x1920;2560x1440")
resolution.value_i        = 3

local window_size         = new_item(MENU.DROPDOWN, "Windowed;Fullscreen;Fullscreen Borderless")
window_size.value_i       = 1

local texture_filtering   = new_item(MENU.DROPDOWN, "Linear Filtering;Nearest Filtering")
texture_filtering.value_i = 0

local fps                 = new_item(MENU.DROPDOWN, "30 FPS;60 FPS;120 FPS;144 FPS;Unlimited FPS")
fps.value_i               = 1

local gui_scale           = new_item(MENU.SPINNER, "GUI Scale")
gui_scale.value_i         = 2
gui_scale.min_i           = 1
gui_scale.max_i           = 10

local fov                 = new_item(MENU.SPINNER, "FOV")
fov.value_i               = 90
fov.min_i                 = 60
fov.max_i                 = 120

local subtitles           = new_item(MENU.CHECKBOX, "Subtitles")
subtitles.value_b         = false

local view_bob            = new_item(MENU.CHECKBOX, "View Bob")
view_bob.value_b          = true

local view_roll           = new_item(MENU.CHECKBOX, "View Roll")
view_roll.value_b         = true

local apply               = new_item(MENU.BUTTON, "Apply")
apply.onpress             = function()
  print("apply settings")
  options_open = false
end

local cancel              = new_item(MENU.BUTTON, "Cancel")
cancel.onpress            = function()
  options_open = false
end

local name_box            = new_item(MENU.TEXTBOX, "Name")
name_box.value_s          = "Player"

-- ============================================================
--  Controls tab — keybind list
-- ============================================================
--
-- KB_DEFS: { binding_key_in_BINDINGS | nil, action_label }
-- nil binding_key → engine-controlled (shown but cannot be rebound yet)
--
local KB_DEFS             = {
  { "up",         "Move Forward" },
  { "down",       "Move Back" },
  { "left",       "Move Left" },
  { "right",      "Move Right" },
  { "jump",       "Jump" },
  { "crouch",     "Crouch" },
  { "walk",       "Walk" },
  { "interact",   "Use / Interact" },
  { "shoot",      "Fire" },
  { "reload",     "Reload" },
  { "flashlight", "Flashlight" },
  { "scoreboard", "Scoreboard" },
  { "chat",       "Chat" },
  { "console",    "Console" },
}

-- Sync all current BINDINGS into the C++ engine binding table at startup
for bkey, b in pairs(BINDINGS) do
  set_engine_binding(bkey, b.type, b.code)
  if b.alt_code and b.alt_code >= 0 then
    set_engine_binding_alt(bkey, b.alt_type, b.alt_code)
  end
end

local kb_header       = new_item(MENU.KEYBIND_HEADER, "ACTION;KEY / BUTTON;ALTERNATE")
local kb_rows         = {}
local kb_binding_keys = {} -- parallel to kb_rows; nil = engine-controlled

for i, def in ipairs(KB_DEFS) do
  local bkey   = def[1]
  local action = def[2]

  local row    = new_item(MENU.KEYBIND, action)
  row.value_b  = false
  row.onpress  = function() print("Selected: " .. action) end

  -- Populate primary and alternate from BINDINGS
  if bkey and BINDINGS[bkey] then
    row.value_s = BINDINGS[bkey].display or ""
    row.text2   = BINDINGS[bkey].alt_display or ""
  else
    row.value_s = ""
    row.text2   = ""
  end

  kb_rows[i]         = row
  kb_binding_keys[i] = bkey
end
kb_rows[1].value_b = true -- first row selected by default

local controls_items = { kb_header }
for _, row in ipairs(kb_rows) do
  controls_items[#controls_items + 1] = row
end

-- Returns index, binding_key, row for the currently selected kb row
local function get_selected_kb_info()
  for i, row in ipairs(kb_rows) do
    if row.value_b then return i, kb_binding_keys[i], row end
  end
  return nil, nil, nil
end

-- ---- Keybind editing state ----
local kb_editing     = false
local kb_editing_alt = false -- true when editing the alternate column
local kb_editing_row = nil   -- LuaMenuItem being edited
local kb_editing_idx = nil   -- index into kb_rows
local kb_edit_saved  = ""    -- saved value before "Press any key..." shown
local kb_edit_wait   = 0     -- cooldown frames (skip the click frame)

local function cancel_kb_edit()
  if kb_editing_row then
    if kb_editing_alt then
      kb_editing_row.text2 = kb_edit_saved
    else
      kb_editing_row.value_s = kb_edit_saved
    end
  end
  kb_editing     = false
  kb_editing_alt = false
  kb_editing_row = nil
  kb_editing_idx = nil
end

local function start_kb_edit(is_alt)
  local i, bkey, row = get_selected_kb_info()
  if not i then return end
  if not bkey then
    print("This control is engine-controlled and cannot be rebound yet")
    return
  end
  kb_editing     = true
  kb_editing_alt = is_alt or false
  kb_editing_row = row
  kb_editing_idx = i
  if kb_editing_alt then
    kb_edit_saved = row.text2
    row.text2     = "Press any key..."
  else
    kb_edit_saved = row.value_s
    row.value_s   = "Press any key..."
  end
  kb_edit_wait = 2 -- skip the button-click frame + 1 buffer
end

-- Capture a key/button and apply it to the correct field (primary or alt)
local function apply_kb_capture(bkey, row, input_type, code, disp)
  local b = BINDINGS[bkey]
  if kb_editing_alt then
    b.alt_type    = input_type
    b.alt_code    = code
    b.alt_display = disp
    set_engine_binding_alt(bkey, input_type, code)
    row.text2 = disp
  else
    b.type    = input_type
    b.code    = code
    b.display = disp
    set_engine_binding(bkey, input_type, code)
    row.value_s = disp
  end
  print("Bound " .. (kb_editing_alt and "alt " or "") .. bkey .. " → " .. disp)
  kb_editing = false; kb_editing_alt = false
  kb_editing_row = nil; kb_editing_idx = nil
end

-- Call once per frame while options are open
local function poll_kb_edit()
  if not kb_editing then return end

  -- If a different row was clicked, cancel edit (C++ already updated value_b)
  local _, _, cur = get_selected_kb_info()
  if cur ~= kb_editing_row then
    cancel_kb_edit()
    return
  end

  if kb_edit_wait > 0 then
    kb_edit_wait = kb_edit_wait - 1
    return
  end

  local bkey = kb_binding_keys[kb_editing_idx]

  -- Keyboard
  local k = get_key_pressed()
  if k ~= 0 then
    local disp = KEY_NAMES[k] or ("Key " .. tostring(k))
    apply_kb_capture(bkey, kb_editing_row, INPUT_KEY, k, disp)
    return
  end

  -- Mouse buttons (0=left, 1=right, 2=middle)
  for btn = 0, 2 do
    if is_mouse_button_pressed(btn) then
      local disp = MOUSE_NAMES[btn] or ("Mouse " .. tostring(btn))
      apply_kb_capture(bkey, kb_editing_row, INPUT_MOUSE, btn, disp)
      return
    end
  end

  -- Mouse wheel
  local wheel = get_mouse_wheel_move()
  if wheel ~= 0 then
    local code = (wheel > 0) and MOUSE_WHEEL_UP or MOUSE_WHEEL_DOWN
    local disp = MOUSE_NAMES[code] or ("Wheel " .. tostring(code))
    apply_kb_capture(bkey, kb_editing_row, INPUT_MOUSE, code, disp)
    return
  end
end

-- ---- Bottom bar buttons ----
local kb_use_defaults     = new_item(MENU.BUTTON, "Use Defaults")
kb_use_defaults.onpress   = function()
  cancel_kb_edit()
  reset_bindings_to_defaults()
  -- Sync all defaults back to C++
  for bkey, b in pairs(BINDINGS) do
    set_engine_binding(bkey, b.type, b.code)
    if b.alt_code and b.alt_code >= 0 then
      set_engine_binding_alt(bkey, b.alt_type, b.alt_code)
    else
      set_engine_binding_alt(bkey, INPUT_KEY, -1)
    end
  end
  -- Refresh UI rows
  for i, row in ipairs(kb_rows) do
    local bkey  = kb_binding_keys[i]
    local b     = bkey and BINDINGS[bkey]
    row.value_s = b and (b.display or "") or ""
    row.text2   = b and (b.alt_display or "") or ""
  end
  print("Controls reset to defaults")
end

local kb_edit             = new_item(MENU.BUTTON, "Edit Key")
kb_edit.onpress           = function()
  if kb_editing and not kb_editing_alt then
    cancel_kb_edit()
  else
    cancel_kb_edit(); start_kb_edit(false)
  end
end

local kb_edit_alt         = new_item(MENU.BUTTON, "Edit Alt")
kb_edit_alt.onpress       = function()
  if kb_editing and kb_editing_alt then
    cancel_kb_edit()
  else
    cancel_kb_edit(); start_kb_edit(true)
  end
end

local kb_clear            = new_item(MENU.BUTTON, "Clear Key")
kb_clear.onpress          = function()
  cancel_kb_edit()
  local i, bkey, row = get_selected_kb_info()
  if not bkey then
    print("Engine-controlled — cannot clear"); return
  end
  BINDINGS[bkey].code    = -1
  BINDINGS[bkey].display = ""
  set_engine_binding(bkey, INPUT_KEY, -1)
  row.value_s = ""
  print("Cleared primary binding for: " .. row.text)
end

local kb_clear_alt        = new_item(MENU.BUTTON, "Clear Alt")
kb_clear_alt.onpress      = function()
  cancel_kb_edit()
  local i, bkey, row = get_selected_kb_info()
  if not bkey then
    print("Engine-controlled — cannot clear"); return
  end
  BINDINGS[bkey].alt_code    = -1
  BINDINGS[bkey].alt_display = ""
  set_engine_binding_alt(bkey, INPUT_KEY, -1)
  row.text2 = ""
  print("Cleared alternate binding for: " .. row.text)
end

local controls_bottom     = { kb_use_defaults, kb_edit, kb_edit_alt, kb_clear, kb_clear_alt }

-- ============================================================
--  Options tabs — each tab has its own item list + bottom bar
-- ============================================================
local options_tabs        = { "Multiplayer", "Audio", "Controls", "Video", "Accessibility" }

local options_tab_items   = {
  { name_box },                                               -- [1] Multiplayer
  { master,  music, sfx, vocals },                            -- [2] Audio
  controls_items,                                             -- [3] Controls
  { brightness, window_size, fps, vsync, anti_aliasing,
    texture_filtering, quality, resolution, gui_scale, fov }, -- [4] Video
  { subtitles, view_bob, view_roll },                         -- [5] Accessibility
}

local options_tab_bottoms = {
  { apply, cancel }, -- Multiplayer
  { apply, cancel }, -- Audio
  controls_bottom,   -- Controls
  { apply, cancel }, -- Video
  { apply, cancel }, -- Accessibility
}

-- ============================================================
--  hud_draw  — called from draw_gui(dt) every frame
-- ============================================================
local component_height    = 18
local hud_bg_color        = COLOR.BLACK
hud_bg_color.a            = 200

function hud_draw(dt)
  -- ESC: cancel keybind edit first, otherwise toggle pause
  if is_key_pressed(KEY.ESCAPE) then
    if kb_editing then
      cancel_kb_edit()
    else
      toggle_pause()
    end
  end

  if pause_open then
    draw_rectangle(0, 0, 10000, 10000, hud_bg_color)

    if not ui_panel("#132#Pause", pause_items, pause_state, 26) then
      -- X button clicked
      cancel_kb_edit()
      pause_open   = false
      options_open = false
      set_menu_mode(false)
    end
  end

  if options_open then
    -- Poll for key capture while in Controls tab edit mode
    poll_kb_edit()

    local tab_idx    = options_state.active_tab + 1
    local tab_items  = options_tab_items[tab_idx]
    local tab_bottom = options_tab_bottoms[tab_idx]
    if not ui_panel("#142#Options", tab_items, options_state, component_height,
          options_tabs, tab_bottom) then
      cancel_kb_edit()
      options_open = false
    end
  end
end

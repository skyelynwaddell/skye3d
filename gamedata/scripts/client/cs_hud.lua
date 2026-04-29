-- cs_hud.lua  — Lua-driven pause / options menus.

-- ============================================================
--  State
-- ============================================================
local pause_open          = false
local options_open        = false

local defaultpos          = 24
local options_size        = { x = 500, y = 300 }
local options_state       = PanelState.new()
options_state.x           = defaultpos
options_state.y           = defaultpos
options_state.w           = options_size.x
options_state.h           = options_size.y

local video_confirm_size  = { x = 280, h = 80 }
local video_confirm_state = PanelState.new()
video_confirm_state.x     = defaultpos
video_confirm_state.y     = defaultpos
video_confirm_state.w     = video_confirm_size.x
video_confirm_state.h     = video_confirm_size.h

-- ============================================================
--  Forward-declare video helpers so pause_items closures can
--  reference them before the item locals are defined below.
-- ============================================================
local sync_video_settings
local apply_video_settings

-- ============================================================
--  toggle_pause
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
-- ============================================================
--  Text menu helper
--  Draws a menu item using draw_text and returns true on click.
--  Highlights on hover; block_input suppresses hover+click
--  (e.g. when the Options panel is open in front).
-- ============================================================
local MENU_COLOR_NORMAL = COLOR.WHITE
local MENU_COLOR_HOVER  = { r = 163, g = 100, b = 255, a = 255 }

-- size and gap are pre-scaled by the caller so the menu fits any resolution.
-- Hit width is 45% of logical screen width — easily covers the longest label.
local function text_menu_item(label, x, y, size, gap, block_input)
  local scale   = get_gui_scale()
  local mx      = get_mouse_x() / scale
  local my      = get_mouse_y() / scale
  local lw      = get_screen_width() / scale
  local hovered = (not block_input)
      and mx >= x and mx <= x + lw * 0.45
      and my >= y and my <= y + gap - 2
  draw_text(label, x, y, size, hovered and MENU_COLOR_HOVER or MENU_COLOR_NORMAL)
  return hovered and is_mouse_button_pressed(MOUSE_BUTTON.LEFT)
end

-- ============================================================
--  Options items
-- ============================================================
local function new_item(t, text)
  local i = MenuItem.new()
  i.type = t
  i.text = text or ""
  return i
end

local VIDEO_DEFAULTS          = {
  brightness     = 0.3,
  vsync          = true,
  msaa4x         = true,
  fov            = 90,
  gui_scale      = 1.0,
  window_mode    = 1, -- maximized
  texture_filter = 1, -- Linear (bilinear)
  resolution     = "1280x720",
  target_fps     = 60,
}

local master                  = new_item(MENU.SLIDERBAR, "Master Volume")
master.value_f                = 0.75
master.min_f                  = 0.0
master.max_f                  = 1.0

local music                   = new_item(MENU.SLIDERBAR, "Music")
music.value_f                 = 0.75
music.min_f                   = 0.0
music.max_f                   = 1.0

local sfx                     = new_item(MENU.SLIDERBAR, "SFX")
sfx.value_f                   = 0.75
sfx.min_f                     = 0.0
sfx.max_f                     = 1.0

local vocals                  = new_item(MENU.SLIDERBAR, "Vocals")
vocals.value_f                = 0.75
vocals.min_f                  = 0.0
vocals.max_f                  = 1.0

local brightness              = new_item(MENU.SLIDERBAR, "Brightness")
brightness.value_f            = 0.3 -- matches default pp_exposure
brightness.min_f              = 0.0
brightness.max_f              = 1.0

local vsync                   = new_item(MENU.CHECKBOX, "VSync")
vsync.value_b                 = true

local anti_aliasing           = new_item(MENU.CHECKBOX, "MSAA 4X")
anti_aliasing.value_b         = true

local quality                 = new_item(MENU.DROPDOWN, "Low Quality;Medium Quality;High Quality")
quality.value_i               = 1

-- Indexes match set_window_size() string format exactly
local resolution_title        = new_item(MENU.LABEL, "Resolution");
local resolution              = new_item(MENU.DROPDOWN,
  "640x480;720x480;720x576;1176x664;1280x720;1280x768;1280x960;1280x1024;1920x1080")
resolution.value_i            = 4

-- Indexes match global_window_mode: 0=windowed, 1=fullscreen, 2=borderless
local window_size_title       = new_item(MENU.LABEL, "Window Size");
local window_size             = new_item(MENU.DROPDOWN, "Windowed;Fullscreen;Fullscreen Borderless")
window_size.value_i           = 0

-- Index 0=Linear(bilinear), 1=Nearest(point)
local texture_filtering_title = new_item(MENU.LABEL, "Texture Filtering");
local texture_filtering       = new_item(MENU.DROPDOWN,
  "Anisotropic 16x;Anisotropic 8x;Anisotropic 4x;Trilinear Filtering;Linear Filtering;Nearest Filtering")
texture_filtering.value_i     = 4

local fps_title               = new_item(MENU.LABEL, "FPS");
local fps                     = new_item(MENU.DROPDOWN, "30 FPS;60 FPS;120 FPS;144 FPS;Unlimited FPS")
fps.value_i                   = 1

local gui_scale               = new_item(MENU.SPINNERF, "GUI Scale")
gui_scale.value_f             = 2.0
gui_scale.min_f               = 0.5
gui_scale.max_f               = 4.0
gui_scale.step_f              = 0.5

local fov                     = new_item(MENU.SPINNER, "FOV")
fov.value_i                   = 90
fov.min_i                     = 60
fov.max_i                     = 120

local subtitles               = new_item(MENU.CHECKBOX, "Subtitles")
subtitles.value_b             = false

local view_bob                = new_item(MENU.CHECKBOX, "View Bob")
view_bob.value_b              = true

local view_roll               = new_item(MENU.CHECKBOX, "View Roll")
view_roll.value_b             = true

-- ============================================================
--  Video helpers (defined here so they close over the item locals)
-- ============================================================

-- Resolution labels parallel to the dropdown, zero-indexed
local RES_LABELS              = { "640x480", "720x480", "720x576", "1176x664", "1280x720", "1280x768", "1280x960",
  "1280x1024", "1920x1080" }

-- FPS values parallel to the fps dropdown, zero-indexed (0 = unlimited)
local FPS_VALUES              = { 30, 60, 120, 144, 0 }

-- Populate UI items from the current C++ / engine state.
-- Call this whenever the Video tab is about to be shown.
sync_video_settings           = function()
  -- Brightness: slider 0-1 maps 1:1 to pp_exposure
  brightness.value_f        = math.min(1.0, math.max(0.0, get_brightness()))

  -- Checkboxes
  vsync.value_b             = get_vsync()
  anti_aliasing.value_b     = get_msaa4x()

  -- Spinners
  fov.value_i               = get_fov()
  gui_scale.value_f         = math.max(0.5, get_gui_scale())

  -- Window mode dropdown maps directly: 0/1/2
  window_size.value_i       = get_window_mode()

  -- Texture filter: C++ enum 0=Point,1=Bilinear,2=Trilinear,3=Aniso4x,4=Aniso8x,5=Aniso16x
  -- Dropdown is reversed: index 0=Aniso16x … index 5=Nearest, so index = 5 - cpp_value
  local tf                  = get_texture_filter()
  texture_filtering.value_i = math.max(0, math.min(5, 5 - tf))

  -- Resolution: match current screen dims to a dropdown index
  local cur                 = get_configured_width() .. "x" .. get_configured_height()
  resolution.value_i        = 3 -- fallback: 1920x1080
  for idx, label in ipairs(RES_LABELS) do
    if label == cur then
      resolution.value_i = idx - 1
      break
    end
  end

  -- FPS: match current target to a dropdown index
  local cur_fps = get_target_fps()
  fps.value_i = 1 -- fallback: 60
  for idx, val in ipairs(FPS_VALUES) do
    if val == cur_fps then
      fps.value_i = idx - 1
      break
    end
  end
end

-- Pending window resize: populated by apply/restore, consumed by draw() in
-- cs_draw.lua BEFORE BeginDrawing so the FBO can be safely recreated without
-- mid-frame dimension mismatches causing split-screen artifacts.
pending_video_apply           = nil

-- Read UI items and push them to C++ / engine.
-- Called by the Apply button.
apply_video_settings          = function()
  options_state.x = defaultpos
  options_state.y = defaultpos
  options_state.w = options_size.x
  options_state.h = options_size.y

  video_confirm_state.x = defaultpos;
  video_confirm_state.y = defaultpos;
  video_confirm_state.w = video_confirm_size.x
  video_confirm_state.h = video_confirm_size.h

  -- Apply non-window settings immediately (safe mid-frame).
  set_brightness(brightness.value_f)
  set_vsync(vsync.value_b)
  set_msaa4x(anti_aliasing.value_b)
  set_fov(fov.value_i)
  set_gui_scale(gui_scale.value_f)
  set_texture_filter(5 - texture_filtering.value_i)
  local chosen_fps = FPS_VALUES[fps.value_i + 1] or 60
  set_target_fps(chosen_fps)

  -- Defer set_window_size/set_window_mode to next draw() call so they happen
  -- before BeginDrawing, preventing FBO/viewport dimension mismatches.
  local chosen_res = RES_LABELS[resolution.value_i + 1]
  pending_video_apply = { res = chosen_res, mode = window_size.value_i }

  -- NOTE: save_settings() intentionally omitted — called only on explicit confirm.
end

-- ============================================================
--  Video settings snapshot / restore
--
--  Snapshot reads directly from C++ getters so it captures the
--  *committed* engine state, not whatever the UI dropdowns say.
--  Restore writes directly to C++ setters for the same reason,
--  then re-syncs the UI so dropdowns match what was restored.
-- ============================================================
local video_saved             = nil

local function snapshot_video_settings()
  video_saved = {
    brightness     = get_brightness(),
    vsync          = get_vsync(),
    msaa4x         = get_msaa4x(),
    fov            = get_fov(),
    gui_scale      = get_gui_scale(),
    window_mode    = get_window_mode(),
    texture_filter = get_texture_filter(),
    res_w          = get_configured_width(),
    res_h          = get_configured_height(),
    target_fps     = get_target_fps(),
  }
end

local function restore_video_settings()
  if not video_saved then return end
  -- Apply non-window settings immediately (safe mid-frame).
  set_brightness(video_saved.brightness)
  set_vsync(video_saved.vsync)
  set_msaa4x(video_saved.msaa4x)
  set_fov(video_saved.fov)
  set_gui_scale(video_saved.gui_scale)
  set_texture_filter(video_saved.texture_filter)
  set_target_fps(video_saved.target_fps)
  -- Defer window resize to next draw() call.
  pending_video_apply = {
    res  = video_saved.res_w .. "x" .. video_saved.res_h,
    mode = video_saved.window_mode,
  }
  sync_video_settings() -- bring UI dropdowns back in line
end

-- ============================================================
--  Apply / Cancel buttons
-- ============================================================
local video_confirm_open      = false
local video_confirm_timer     = 10.0
local video_confirm_countdown = false

local video_confirm_lbl       = new_item(MENU.LABEL, "Keep these display settings?")
local video_confirm_timer_lbl = new_item(MENU.LABEL, "Reverting in 10s...")

local function close_video_confirm(keep)
  video_confirm_open      = false
  video_confirm_countdown = false
  video_confirm_timer     = 10.0
  if keep then
    save_settings() -- write to disk only on explicit confirm
  else
    restore_video_settings()
  end
end

local video_defaults_button   = new_item(MENU.BUTTON, "Use Defaults")
video_defaults_button.onpress = function()
  set_brightness(VIDEO_DEFAULTS.brightness)
  set_vsync(VIDEO_DEFAULTS.vsync)
  set_msaa4x(VIDEO_DEFAULTS.msaa4x)
  set_fov(VIDEO_DEFAULTS.fov)
  set_gui_scale(VIDEO_DEFAULTS.gui_scale)
  set_texture_filter(VIDEO_DEFAULTS.texture_filter)
  set_target_fps(VIDEO_DEFAULTS.target_fps)
  -- Defer window resize to next draw() call.
  pending_video_apply = {
    res  = VIDEO_DEFAULTS.resolution,
    mode = VIDEO_DEFAULTS.window_mode,
  }
  sync_video_settings() -- bring UI dropdowns back in line
end

local video_changes_confirm   = new_item(MENU.BUTTON, "Keep Settings")
video_changes_confirm.onpress = function()
  close_video_confirm(true)
end

local video_changes_cancel    = new_item(MENU.BUTTON, "Revert")
video_changes_cancel.onpress  = function()
  close_video_confirm(false)
end

-- Pre-declared tables so they aren't rebuilt every frame
local video_confirm_items     = { video_confirm_lbl, video_confirm_timer_lbl }
local video_confirm_btns      = { video_changes_confirm, video_changes_cancel }

local apply                   = new_item(MENU.BUTTON, "Apply")
apply.onpress                 = function()
  local tab_idx = options_state.active_tab + 1
  if tab_idx == 4 then
    -- Video tab: tentative apply then wait for user confirmation
    snapshot_video_settings()
    apply_video_settings()
    video_confirm_timer     = 10.0
    video_confirm_countdown = true
    video_confirm_open      = true
  else
    -- Other tabs: apply and save immediately
    apply_video_settings()
    save_settings()
    options_open = false
  end
end

local cancel                  = new_item(MENU.BUTTON, "Cancel")
cancel.onpress                = function()
  if video_confirm_open then
    close_video_confirm(false)
  else
    sync_video_settings()
    options_open = false
  end
end

local name_box                = new_item(MENU.TEXTBOX, "Name")
name_box.value_s              = "Player"

-- ============================================================
--  Controls tab — keybind list
-- ============================================================
local KB_DEFS                 = {
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
local kb_binding_keys = {}

for i, def in ipairs(KB_DEFS) do
  local bkey   = def[1]
  local action = def[2]
  local row    = new_item(MENU.KEYBIND, action)
  row.value_b  = false
  row.onpress  = function() print("Selected: " .. action) end
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
kb_rows[1].value_b = true

local controls_items = { kb_header }
for _, row in ipairs(kb_rows) do
  controls_items[#controls_items + 1] = row
end

local function get_selected_kb_info()
  for i, row in ipairs(kb_rows) do
    if row.value_b then return i, kb_binding_keys[i], row end
  end
  return nil, nil, nil
end

local kb_editing     = false
local kb_editing_alt = false
local kb_editing_row = nil
local kb_editing_idx = nil
local kb_edit_saved  = ""
local kb_edit_wait   = 0

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
  kb_edit_wait = 2
end

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
  print("Bound " .. (kb_editing_alt and "alt " or "") .. bkey .. " -> " .. disp)
  save_bindings()
  kb_editing     = false
  kb_editing_alt = false
  kb_editing_row = nil
  kb_editing_idx = nil
end

local function poll_kb_edit()
  if not kb_editing then return end
  local _, _, cur = get_selected_kb_info()
  if cur ~= kb_editing_row then
    cancel_kb_edit(); return
  end
  if kb_edit_wait > 0 then
    kb_edit_wait = kb_edit_wait - 1; return
  end

  local bkey = kb_binding_keys[kb_editing_idx]

  local k = get_key_pressed()
  if k ~= 0 then
    local disp = KEY_NAMES[k] or ("Key " .. tostring(k))
    apply_kb_capture(bkey, kb_editing_row, INPUT_KEY, k, disp)
    return
  end

  for btn = 0, 2 do
    if is_mouse_button_pressed(btn) then
      local disp = MOUSE_NAMES[btn] or ("Mouse " .. tostring(btn))
      apply_kb_capture(bkey, kb_editing_row, INPUT_MOUSE, btn, disp)
      return
    end
  end

  local wheel = get_mouse_wheel_move()
  if wheel ~= 0 then
    local code = (wheel > 0) and MOUSE_WHEEL_UP or MOUSE_WHEEL_DOWN
    local disp = MOUSE_NAMES[code] or ("Wheel " .. tostring(code))
    apply_kb_capture(bkey, kb_editing_row, INPUT_MOUSE, code, disp)
    return
  end
end

local kb_use_defaults     = new_item(MENU.BUTTON, "Use Defaults")
kb_use_defaults.onpress   = function()
  cancel_kb_edit()
  reset_bindings_to_defaults()
  for bkey, b in pairs(BINDINGS) do
    set_engine_binding(bkey, b.type, b.code)
    if b.alt_code and b.alt_code >= 0 then
      set_engine_binding_alt(bkey, b.alt_type, b.alt_code)
    else
      set_engine_binding_alt(bkey, INPUT_KEY, -1)
    end
  end
  for i, row in ipairs(kb_rows) do
    local bkey  = kb_binding_keys[i]
    local b     = bkey and BINDINGS[bkey]
    row.value_s = b and (b.display or "") or ""
    row.text2   = b and (b.alt_display or "") or ""
  end
  save_bindings()
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
  save_bindings()
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
  save_bindings()
  print("Cleared alternate binding for: " .. row.text)
end

local controls_bottom     = { kb_use_defaults, kb_edit, kb_edit_alt, kb_clear, kb_clear_alt }

-- ============================================================
--  Options tabs
-- ============================================================
local options_tabs        = { "Multiplayer", "Audio", "Controls", "Video", "Accessibility" }

local options_tab_items   = {
  { name_box },                    --multiplayer
  { master,  music, sfx, vocals }, --audio
  controls_items,                  -- controls
  {                                -- video
    window_size_title, window_size,
    fps_title, fps,
    texture_filtering_title, texture_filtering,
    resolution_title, resolution,
    brightness,
    gui_scale,
    fov,
    vsync,
    anti_aliasing,
  },
  { subtitles, view_bob, view_roll }, -- accessibility
}

local options_tab_bottoms = {
  { apply, cancel },                        -- multiplayer
  { apply, cancel },                        -- audio
  controls_bottom,                          -- controls
  { video_defaults_button, apply, cancel }, -- video
  { apply,                 cancel },        -- accessibility
}

-- ============================================================
--  hud_draw — called from draw_gui(dt) every frame
-- ============================================================
local component_height    = 18
local hud_bg_color        = COLOR.BLACK
hud_bg_color.a            = 200

function hud_draw(dt)
  if is_key_pressed(KEY.ESCAPE) then
    if kb_editing then
      cancel_kb_edit()
    else
      toggle_pause()
    end
  end

  if pause_open then
    draw_rectangle(0, 0, 10000, 10000, hud_bg_color)

    local scale    = get_gui_scale()
    local lw       = get_screen_width() / scale
    local lh       = get_screen_height() / scale
    local sx       = lw / 1280
    local sy       = lh / 720

    local title_sz = math.max(1, math.floor(64 * sy))
    local font_sz  = math.max(1, math.floor(40 * sy))
    local item_x   = math.floor(50 * sx)
    local item_y   = math.floor(164 * sy)
    local gap      = math.floor(64 * sy)

    local blk      = options_open -- block input to pause menu items
    local br       = function() item_y = item_y + gap end

    -----------------------------------------
    ---PAUSE MENU ITEMS----------------------
    -----------------------------------------

    -- Game Title
    draw_text("Game Name", item_x, math.floor(64 * sy), title_sz, COLOR.WHITE)

    -- Resume
    if text_menu_item("Resume", item_x, item_y, font_sz, gap, blk) then
      toggle_pause()
    end;
    br()

    -- Save Game
    if text_menu_item("Save Game", item_x, item_y, font_sz, gap, blk) then
      print("save")
    end;
    br()

    -- Load Game
    if text_menu_item("Load Game", item_x, item_y, font_sz, gap, blk) then
      print("load")
    end;
    br()

    -- New Game
    if text_menu_item("New Game", item_x, item_y, font_sz, gap, blk) then
      print("new game")
    end;
    br()

    -- Find Servers
    if text_menu_item("Find servers", item_x, item_y, font_sz, gap, blk) then
      print("find servers")
    end;
    br()

    -- Create Server
    if text_menu_item("Create server", item_x, item_y, font_sz, gap, blk) then
      print("create server")
    end;
    br()

    -- Options
    if text_menu_item("Options", item_x, item_y, font_sz, gap, blk) then
      options_open = not options_open
      if options_open then
        sync_video_settings()
      end
    end;
    br()

    -- Quit
    if text_menu_item("Quit", item_x, item_y, font_sz, gap, blk) then
      quit_game()
    end
    br()
  end

  if options_open then
    poll_kb_edit()

    local tab_idx    = options_state.active_tab + 1
    local tab_items  = options_tab_items[tab_idx]
    local tab_bottom = options_tab_bottoms[tab_idx]

    if not ui_panel("#142#Options", tab_items, options_state, component_height,
          options_tabs, tab_bottom) then
      cancel_kb_edit()
      if video_confirm_open then close_video_confirm(false) end
      options_open = false
    end

    if video_confirm_open then
      if video_confirm_countdown then
        video_confirm_timer = video_confirm_timer - dt
        if video_confirm_timer <= 0 then
          close_video_confirm(false)
        end
      end
      local secs = math.max(0, math.ceil(video_confirm_timer))
      video_confirm_timer_lbl.text = "Reverting in " .. tostring(secs) .. "s..."

      if not ui_panel("#132#Keep Settings?", video_confirm_items, video_confirm_state,
            component_height, {}, video_confirm_btns) then
        close_video_confirm(false)
      end
    end
  end
end

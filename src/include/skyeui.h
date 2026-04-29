#pragma once
#include "raygui.h"
#include <vector>
#include <string>
#include <functional>

// ---------------------------------------------------------------------------
// MenuItemType — one value per supported raygui control
// ---------------------------------------------------------------------------
enum MenuItemType
{
  MENUITEMTYPE_BUTTON,      // GuiButton          -> onpress()
  MENUITEMTYPE_LABEL,       // GuiLabel           (read-only header / description)
  MENUITEMTYPE_SEPARATOR,   // GuiLine            (divider line, optional text)
  MENUITEMTYPE_SLIDER,      // GuiSlider          -> *value_f  (knob)
  MENUITEMTYPE_SLIDERBAR,   // GuiSliderBar       -> *value_f  (filled bar)
  MENUITEMTYPE_PROGRESSBAR, // GuiProgressBar     -> *value_f  (read-only)
  MENUITEMTYPE_CHECKBOX,    // GuiCheckBox        -> *value_b  -> onpress()
  MENUITEMTYPE_TOGGLE,      // GuiToggle          -> *value_b  -> onpress()
  MENUITEMTYPE_DROPDOWN,    // GuiDropdownBox     -> *value_i  -> onpress()  text = "A;B;C"
  MENUITEMTYPE_SPINNER,     // GuiSpinner         -> *value_i  -> onpress()
  MENUITEMTYPE_SPINNERF,    // float spinner      -> *value_f  -> onpress()  step_f controls increment
  MENUITEMTYPE_TEXTBOX,        // GuiTextBox      -> value_s   (char buffer)
  MENUITEMTYPE_KEYBIND,        // Selectable row  -> text=action, value_s=primary key, text2=alternate, value_b=selected
  MENUITEMTYPE_KEYBIND_HEADER, // Header row      -> text="col1;col2;col3"
};

// ---------------------------------------------------------------------------
// MenuItem — one entry in any menu or HUD panel.
//
// Typical usage (C++20 designated initialisers):
//
//   static float vol = 0.8f;
//   MenuItem m = {
//       .type    = MENUITEMTYPE_SLIDER,
//       .text    = "Volume",
//       .value_f = &vol,
//       .min_f   = 0.f,  .max_f = 1.f,
//       .onpress = []{ ApplyVolume(); }
//   };
//
// Fields you don't list get their defaults (nullptr / 0 / "").
// ---------------------------------------------------------------------------
struct MenuItem
{
  MenuItemType type = MENUITEMTYPE_BUTTON;

  // Primary text:
  //   BUTTON      -> button face label
  //   LABEL       -> displayed string
  //   SEPARATOR   -> optional centred text on the line
  //   SLIDER/BAR  -> left-side label
  //   PROGRESSBAR -> left-side label
  //   CHECKBOX    -> text shown to the right of the tick box
  //   TOGGLE      -> button face label
  //   DROPDOWN    -> semicolon-separated option list  e.g. "Low;Medium;High"
  //   SPINNER     -> left-side label
  //   TEXTBOX     -> optional left label (shrinks textbox to right side)
  std::string text  = "";
  std::string text2 = ""; // KEYBIND: alternate key name; KEYBIND_HEADER: unused (cols go in text)

  // Called when the control fires (click / confirm / value change)
  std::function<void()> onpress = nullptr;

  // Value pointers — point at YOUR variables; SkyeUI reads & writes through them
  float *value_f = nullptr; // SLIDER, SLIDERBAR, PROGRESSBAR
  bool *value_b = nullptr;  // CHECKBOX, TOGGLE
  int *value_i = nullptr;   // DROPDOWN, SPINNER
  char *value_s = nullptr;  // TEXTBOX: editable char buffer
  int buf_size = 64;        // TEXTBOX: length of value_s buffer

  // Numeric ranges
  float min_f = 0.0f;
  float max_f = 1.0f;
  float step_f = 0.1f;   // SPINNERF: increment per +/- button click
  int min_i = 0;
  int max_i = 100;

  // SkyeUI-managed edit state for DROPDOWN / SPINNER / TEXTBOX / SPINNERF.
  // You don't need to touch this.
  bool _edit = false;
  char *_spinnerf_buf = nullptr; // SPINNERF: pointer to external 32-byte buffer (set by lua binding)
};

// ---------------------------------------------------------------------------
// PanelState — persistent state for a moveable/resizable/scrollable panel.
//
// Declare one static instance per panel in your HUD/menu file:
//
//   static PanelState my_panel = {.x=24, .y=24, .w=200, .h=0};
//
// h == 0  → auto-size height to fit all content (no scrollbar).
// h >  0  → fixed viewport height; scroll appears when content is taller.
// ---------------------------------------------------------------------------
struct PanelState
{
  float   x      = 100.f;
  float   y      = 100.f;
  float   w      = 200.f;
  float   h      = 0.f;         // 0 = auto; > 0 = fixed viewport height
  Vector2 scroll = {0.f, 0.f};

  // Set by the tab strip — which tab is currently active (0-based).
  int     active_tab = 0;

  // SkyeUI-managed drag/resize/scroll state — don't touch these.
  bool    _dragging         = false;
  bool    _resizing         = false;
  bool    _scroll_dragging  = false;
  float   _scroll_drag_off  = 0.f;   // mouse Y minus thumb top at drag start
  Vector2 _drag_offset      = {0.f, 0.f};
};

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Call once at startup — loads style and performs any one-time setup.
void SkyeUI_Init();

// Call once per frame BEFORE any SkyeUI_Panel calls (e.g. at the top of DrawGUI).
// Resets the panel z-order tracking so overlapping panels block each other correctly.
void SkyeUI_BeginFrame();

// Draw a vertical stack of MenuItem controls.
//   x, y  — top-left origin
//   w, h  — width and item-row height (separators use a fixed 10 px)
// Pass items by reference so _edit state persists between frames.
// clip_bottom: logical Y the open dropdown must not extend past (0 = no limit).
// When non-zero, SkyeUI_VerticalItemList calls EndScissorMode() before drawing
// the open dropdown so it renders unclipped; the caller's EndScissorMode()
// then becomes a harmless no-op.
void SkyeUI_VerticalItemList(std::vector<MenuItem> &items,
                             float x, float y, float w, float h,
                             float clip_bottom = 0.f);

// Draw a moveable, resizable, scrollable panel window.
//   title  — title bar text
//   items  — content items (the visible set; swap per active_tab in caller)
//   state  — persistent position/size/scroll state (one static per panel)
//   item_h — height of each item row
//   tabs   — optional tab labels; pass nullptr for no tab strip
//             state.active_tab is updated when the user clicks a tab
//   bottom — optional bottom-bar buttons rendered as a horizontal row;
//             pass nullptr for no bottom bar
// Returns false when the X button was clicked (caller should hide the panel).
bool SkyeUI_Panel(const char*                      title,
                  std::vector<MenuItem>&           items,
                  PanelState&                      state,
                  float                            item_h,
                  const std::vector<std::string>*  tabs   = nullptr,
                  std::vector<MenuItem>*           bottom = nullptr);

// Top-level GUI entry point — called from DrawGUI() each frame.
void SkyeUI_DrawGUI();

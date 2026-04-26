#define RAYGUI_IMPLEMENTATION
#include "skyeui.h"
#include <engine.h>
#include <hud.h>
#include <raymath.h>

// ============================================================
//  Layout constants
// ============================================================
static constexpr float ITEM_GAP = 4.0f;       // vertical gap between items
static constexpr float SEP_HEIGHT = 10.0f;    // height used by SEPARATOR rows
static constexpr float PANEL_PADDING = 8.0f;  // inner padding inside SkyeUI_Panel
static constexpr float PANEL_TITLE_H = 24.0f; // height of the panel title bar

// ============================================================
//  SkyeUI_Init
// ============================================================
void SkyeUI_Init()
{
  GuiLoadStyle("src/lib/raygui/styles/custom/skyeblue.rgs");
}

// ============================================================
//  Internal helpers
// ============================================================
static constexpr float LABEL_H = 14.0f; // top-label height above sliders etc.

// Logical GUI dimensions — the coordinate space raygui and GetMousePosition()
// work in after the rlScalef(global_guiscale) transform inside the FBO.
static inline float LogW() { return GUI_WIDTH / global_guiscale; }
static inline float LogH() { return GUI_HEIGHT / global_guiscale; }

// ============================================================
//  Panel z-order input blocking
//
//  Panels are drawn back-to-front (index 0 = furthest back).
//  Each frame we record the draw-index of the LAST (topmost)
//  panel whose rect contained the mouse.  Next frame, any panel
//  with a LOWER index that also contains the mouse is blocked
//  from receiving input — it's underneath the topmost panel.
// ============================================================
static int s_panel_draw_idx = 0;  // incremented per SkyeUI_Panel call, reset each frame
static int s_top_panel_prev = -1; // topmost index that had the mouse, last frame
static int s_top_panel_this = -1; // being built this frame

// True for controls that render their text as a GuiLabel ABOVE the widget
// rather than inline to the left via raygui's textLeft parameter.
static bool HasTopLabel(const MenuItem &item)
{
  return !item.text.empty() &&
         (item.type == MENUITEMTYPE_SLIDER ||
          item.type == MENUITEMTYPE_SLIDERBAR ||
          item.type == MENUITEMTYPE_PROGRESSBAR ||
          item.type == MENUITEMTYPE_SPINNER);
}

// Total vertical space consumed by one item row (used for layout + panel sizing)
static float ItemHeight(const MenuItem &item, float row_h)
{
  if (item.type == MENUITEMTYPE_SEPARATOR)
    return SEP_HEIGHT;
  if (HasTopLabel(item))
    return LABEL_H + row_h;
  return row_h;
}

// ============================================================
//  SkyeUI_VerticalItemList
//
//  Two rendering passes so open dropdowns always draw on top
//  of the items below them (raygui dropdowns expand downward).
// ============================================================
void SkyeUI_VerticalItemList(std::vector<MenuItem> &items,
                             float x, float y, float w, float h)
{
  // ---- Pre-compute each item's top-left Y so both passes share positions ----
  std::vector<float> ys;
  ys.reserve(items.size());
  {
    float cy = y;
    for (auto &item : items)
    {
      ys.push_back(cy);
      cy += ItemHeight(item, h) + ITEM_GAP;
    }
  }

  // Collect dropdown indices — rendered after everything else so their
  // expanded list overlays items below them.  Each dropdown is called
  // exactly ONCE per frame (the standard raygui toggle pattern), which
  // prevents the "opens then immediately closes" double-call bug.
  std::vector<size_t> dropdowns;

  // ---- Main pass ----
  for (size_t i = 0; i < items.size(); i++)
  {
    auto &item = items[i];
    float iy = ys[i];
    Rectangle bounds = {x, iy, w, h};

    switch (item.type)
    {
    // -------------------------------------------------------
    case MENUITEMTYPE_BUTTON:
      if (GuiButton(bounds, item.text.c_str()) && item.onpress)
        item.onpress();
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_LABEL:
      GuiLabel(bounds, item.text.c_str());
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_SEPARATOR:
      GuiLine({x, iy, w, SEP_HEIGHT}, item.text.empty() ? nullptr : item.text.c_str());
      break;

    // -------------------------------------------------------
    // SLIDER / SLIDERBAR / PROGRESSBAR
    // Text is drawn as a GuiLabel ABOVE the widget, not inline to its left.
    // -------------------------------------------------------
    case MENUITEMTYPE_SLIDER:
      if (item.value_f)
      {
        if (HasTopLabel(item))
          GuiLabel({x, iy, w, LABEL_H}, item.text.c_str());
        float cy = iy + (HasTopLabel(item) ? LABEL_H : 0.f);
        if (GuiSlider({x, cy, w, h}, nullptr, nullptr,
                      item.value_f, item.min_f, item.max_f) &&
            item.onpress)
          item.onpress();
      }
      break;

    case MENUITEMTYPE_SLIDERBAR:
      if (item.value_f)
      {
        if (HasTopLabel(item))
          GuiLabel({x, iy, w, LABEL_H}, item.text.c_str());
        float cy = iy + (HasTopLabel(item) ? LABEL_H : 0.f);
        if (GuiSliderBar({x, cy, w, h}, nullptr, nullptr,
                         item.value_f, item.min_f, item.max_f) &&
            item.onpress)
          item.onpress();
      }
      break;

    case MENUITEMTYPE_PROGRESSBAR:
      if (item.value_f)
      {
        if (HasTopLabel(item))
          GuiLabel({x, iy, w, LABEL_H}, item.text.c_str());
        float cy = iy + (HasTopLabel(item) ? LABEL_H : 0.f);
        GuiProgressBar({x, cy, w, h}, nullptr, nullptr,
                       item.value_f, item.min_f, item.max_f);
      }
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_CHECKBOX:
      if (item.value_b)
      {
        // Tick box is square (h x h); text appears to its right automatically
        if (GuiCheckBox({x, iy, h, h}, item.text.c_str(), item.value_b) && item.onpress)
          item.onpress();
      }
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_TOGGLE:
      if (item.value_b)
      {
        if (GuiToggle(bounds, item.text.c_str(), item.value_b) && item.onpress)
          item.onpress();
      }
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_DROPDOWN:
      dropdowns.push_back(i); // always defer — rendered last, one call per frame
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_SPINNER:
      if (item.value_i)
      {
        if (HasTopLabel(item))
          GuiLabel({x, iy, w, LABEL_H}, item.text.c_str());
        float cy = iy + (HasTopLabel(item) ? LABEL_H : 0.f);
        // Pass nullptr for text — label is already drawn above
        if (GuiSpinner({x, cy, w, h}, nullptr,
                       item.value_i, item.min_i, item.max_i, item._edit))
          item._edit = !item._edit;
        if (!item._edit && item.onpress)
          item.onpress();
      }
      break;

    // -------------------------------------------------------
    case MENUITEMTYPE_TEXTBOX:
      if (item.value_s)
      {
        // If a label is provided, draw it left of a narrower textbox
        if (!item.text.empty())
        {
          float label_w = w * 0.35f;
          GuiLabel({x, iy, label_w, h}, item.text.c_str());
          if (GuiTextBox({x + label_w, iy, w - label_w, h},
                         item.value_s, item.buf_size, item._edit))
            item._edit = !item._edit;
        }
        else
        {
          if (GuiTextBox(bounds, item.value_s, item.buf_size, item._edit))
            item._edit = !item._edit;
        }
      }
      break;

    } // switch
  } // main pass

  // ---- Dropdown pass — drawn last so expanded lists overlay other items ----
  //
  // Two sub-passes ensure correct z-order when multiple dropdowns exist:
  //   Sub-pass A: render all CLOSED dropdowns in their normal positions.
  //   Sub-pass B: render the one OPEN dropdown on top of everything.
  //
  // Each dropdown is called exactly once per frame (standard raygui toggle),
  // and `just_opened` prevents a same-frame double-call that would cause the
  // newly opened list to immediately close again.
  size_t just_opened = SIZE_MAX;

  // If any dropdown is currently open, its expanded list visually overlaps
  // the closed dropdowns below it.  Lock the GUI during sub-pass A so those
  // closed dropdowns draw but cannot steal the click that belongs to the
  // open list — otherwise clicking a list item would simultaneously close
  // the open dropdown AND open the one underneath it.
  bool any_open = false;
  for (size_t i : dropdowns)
    if (items[i]._edit)
    {
      any_open = true;
      break;
    }

  // Sub-pass A — closed dropdowns (locked when one is already open).
  // We honour any external GuiLock (e.g. set by SkyeUI_Panel when the mouse
  // is outside the viewport) by only touching the lock state when we know it
  // was unlocked before we entered this function.
  bool was_locked = GuiIsLocked();
  if (!was_locked && any_open)
    GuiLock();
  for (size_t i : dropdowns)
  {
    auto &item = items[i];
    if (item._edit || !item.value_i)
      continue; // skip already-open ones

    if (GuiDropdownBox({x, ys[i], w, h}, item.text.c_str(), item.value_i, false))
    {
      // Close any other open dropdown so only one can be open at a time
      for (size_t j : dropdowns)
        items[j]._edit = false;
      item._edit = true;
      just_opened = i;
    }
  }
  if (!was_locked && any_open)
    GuiUnlock();

  // Sub-pass B — the one open dropdown, rendered on top, fully interactive
  for (size_t i : dropdowns)
  {
    auto &item = items[i];
    if (!item._edit || !item.value_i || i == just_opened)
      continue;

    if (GuiDropdownBox({x, ys[i], w, h}, item.text.c_str(), item.value_i, true))
    {
      item._edit = false;
      if (item.onpress)
        item.onpress();
    }
  }
}

// ============================================================
//  Panel layout constants
// ============================================================
static constexpr float PANEL_MIN_W = 80.f;
static constexpr float PANEL_MIN_H = 40.f;
static constexpr float SCROLLBAR_W = 8.f;
static constexpr float RESIZE_GRIP = 12.f;

// ============================================================
//  BeginScissorModeLogical
//  Scissor in LOGICAL gui-space coords, compensating for the
//  rlScalef(global_guiscale) transform active inside the GUI FBO.
//  (BeginScissorMode takes raw framebuffer pixel coords.)
// ============================================================
static void BeginScissorModeLogical(float lx, float ly, float lw, float lh)
{
  BeginScissorMode(
      (int)(lx * global_guiscale),
      (int)(ly * global_guiscale),
      (int)(lw * global_guiscale),
      (int)(lh * global_guiscale));
}

// ============================================================
//  SkyeUI_Panel
//  Moveable, resizable, scrollable window panel.
//  Returns false when the X button was clicked.
// ============================================================
bool SkyeUI_Panel(const char *title,
                  std::vector<MenuItem> &items,
                  PanelState &state,
                  float item_h)
{
  // ---- Total content height ----
  float content_h = PANEL_PADDING;
  for (auto &item : items)
    content_h += ItemHeight(item, item_h) + ITEM_GAP;
  content_h += PANEL_PADDING;

  // ---- Viewport height ----
  // h == 0 → auto-fit, capped so the panel stays on screen
  float max_auto = fmaxf(PANEL_MIN_H, LogH() - state.y - PANEL_TITLE_H - 20.f);
  float viewport_h = (state.h > 0.f) ? state.h : fminf(content_h, max_auto);
  float panel_h = PANEL_TITLE_H + viewport_h;
  bool needs_scroll = content_h > viewport_h + 1.f;

  // ---- Z-order input blocking ----
  // Assign this panel a draw index (0 = furthest back, higher = closer to front).
  // If the mouse is over this panel, record it as the current topmost candidate.
  // If a panel drawn AFTER us (higher index) had the mouse last frame, block our input.
  int my_idx = s_panel_draw_idx++;
  Rectangle panel_rect = {state.x, state.y, state.w, panel_h};
  if (CheckCollisionPointRec(GetMousePosition(), panel_rect))
    s_top_panel_this = my_idx; // last writer wins → topmost panel

  // input_blocked: another panel drawn on top of us owned the mouse last frame
  bool input_blocked = (s_top_panel_prev > my_idx) &&
                       CheckCollisionPointRec(GetMousePosition(), panel_rect);

  // ---- Drag to move (title bar minus close button area) ----
  if (!input_blocked)
  {
    Rectangle drag_zone = {state.x, state.y, state.w - PANEL_TITLE_H, PANEL_TITLE_H};
    if (!state._resizing)
    {
      if (!state._dragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), drag_zone))
      {
        state._dragging = true;
        state._drag_offset.x = GetMousePosition().x - state.x;
        state._drag_offset.y = GetMousePosition().y - state.y;
      }
      if (state._dragging)
      {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
          state.x = GetMousePosition().x - state._drag_offset.x;
          state.y = GetMousePosition().y - state._drag_offset.y;
          // Keep entire panel on screen
          state.x = Clamp(state.x, 0.f, fmaxf(0.f, LogW() - state.w));
          state.y = Clamp(state.y, 0.f, fmaxf(0.f, LogH() - panel_h));
        }
        else
          state._dragging = false;
      }
    }

    // ---- Resize grip (bottom-right corner) ----
    Rectangle grip = {state.x + state.w - RESIZE_GRIP,
                      state.y + panel_h - RESIZE_GRIP,
                      RESIZE_GRIP, RESIZE_GRIP};
    if (!state._dragging)
    {
      if (!state._resizing && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), grip))
      {
        state._resizing = true;
        state._drag_offset.x = GetMousePosition().x - (state.x + state.w);
        state._drag_offset.y = GetMousePosition().y - (state.y + panel_h);
      }
      if (state._resizing)
      {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
          float new_right = GetMousePosition().x - state._drag_offset.x;
          float new_bottom = GetMousePosition().y - state._drag_offset.y;
          state.w = Clamp(new_right - state.x, PANEL_MIN_W, LogW() - state.x);
          state.h = Clamp(new_bottom - state.y - PANEL_TITLE_H, PANEL_MIN_H, LogH() - state.y - PANEL_TITLE_H);
          // Recompute after resize
          viewport_h = state.h;
          panel_h = PANEL_TITLE_H + viewport_h;
          needs_scroll = content_h > viewport_h + 1.f;
        }
        else
          state._resizing = false;
      }
    }

    // ---- Mouse-wheel scroll ----
    if (CheckCollisionPointRec(GetMousePosition(), {state.x, state.y, state.w, panel_h}))
      state.scroll.y += GetMouseWheelMove() * (item_h + ITEM_GAP) * 3.f;
  }
  else
  {
    // Release any in-progress drag/resize if we lose input ownership
    state._dragging = false;
    state._resizing = false;
  }

  if (needs_scroll)
    state.scroll.y = Clamp(state.scroll.y, viewport_h - content_h, 0.f);
  else
    state.scroll.y = 0.f;

  // ---- GuiWindowBox — title bar, border, X button ----
  if (input_blocked)
    GuiLock();
  bool closed = (bool)GuiWindowBox({state.x, state.y, state.w, panel_h}, title);
  if (input_blocked)
  {
    GuiUnlock();
    closed = false;
  }

  // ---- Items drawn clipped to viewport ----
  float items_x = state.x + PANEL_PADDING;
  float items_y = state.y + PANEL_TITLE_H + PANEL_PADDING + state.scroll.y;
  float items_w = state.w - PANEL_PADDING * 2.f - (needs_scroll ? SCROLLBAR_W + 2.f : 0.f);

  // Lock raygui input while mouse is outside the visible viewport so that
  // items scrolled under the title bar (or outside the panel entirely) cannot
  // receive clicks even though their raygui bounds still overlap those areas.
  Rectangle vp_input = {state.x + 1.f, state.y + PANEL_TITLE_H + 1.f,
                        state.w - 2.f, viewport_h - 2.f};
  bool mouse_outside_vp = input_blocked || !CheckCollisionPointRec(GetMousePosition(), vp_input);
  if (mouse_outside_vp)
    GuiLock();

  BeginScissorModeLogical(state.x + 1.f, state.y + PANEL_TITLE_H + 1.f,
                          state.w - 2.f, viewport_h - 2.f);
  SkyeUI_VerticalItemList(items, items_x, items_y, items_w, item_h);
  EndScissorMode();

  if (mouse_outside_vp)
    GuiUnlock();

  // ---- Scrollbar ----
  if (needs_scroll)
  {
    float sb_x = state.x + state.w - SCROLLBAR_W - 2.f;
    float sb_y = state.y + PANEL_TITLE_H + 2.f;
    float sb_h = viewport_h - 4.f;

    float travel = content_h - viewport_h; // > 0 when scrollable
    float vis_ratio = Clamp(viewport_h / content_h, 0.f, 1.f);
    float thumb_h = fmaxf(16.f, sb_h * vis_ratio);
    float thumb_travel = sb_h - thumb_h; // pixels the thumb can slide

    // Current thumb position from scroll state
    float frac = (travel > 0.f) ? Clamp((-state.scroll.y) / travel, 0.f, 1.f) : 0.f;
    float thumb_y = sb_y + thumb_travel * frac;

    Rectangle track_rect = {sb_x, sb_y, SCROLLBAR_W, sb_h};
    Rectangle thumb_rect = {sb_x + 1.f, thumb_y, SCROLLBAR_W - 2.f, thumb_h};

    // ---- Thumb / track mouse interaction ----
    Vector2 mouse = GetMousePosition();

    // Exclude the resize grip area so clicking the grip doesn't also fire
    // a track-click that jumps the scrollbar.
    Rectangle grip_zone = {state.x + state.w - RESIZE_GRIP,
                           state.y + panel_h - RESIZE_GRIP,
                           RESIZE_GRIP, RESIZE_GRIP};
    bool over_grip = CheckCollisionPointRec(mouse, grip_zone);

    if (!input_blocked && !state._resizing && !over_grip && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
      if (CheckCollisionPointRec(mouse, thumb_rect))
      {
        state._scroll_dragging = true;
        state._scroll_drag_off = mouse.y - thumb_y;
      }
      else if (CheckCollisionPointRec(mouse, track_rect))
      {
        // Click on track → jump so thumb centres on click point
        float new_frac = (thumb_travel > 0.f)
                             ? Clamp((mouse.y - thumb_h * 0.5f - sb_y) / thumb_travel, 0.f, 1.f)
                             : 0.f;
        state.scroll.y = Clamp(-new_frac * travel, -travel, 0.f);
      }
    }

    if (!input_blocked && state._scroll_dragging)
    {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
      {
        float new_frac = (thumb_travel > 0.f)
                             ? Clamp((mouse.y - state._scroll_drag_off - sb_y) / thumb_travel, 0.f, 1.f)
                             : 0.f;
        state.scroll.y = Clamp(-new_frac * travel, -travel, 0.f);
      }
      else
      {
        state._scroll_dragging = false;
      }
    }

    // Recompute thumb_y after any drag adjustment
    frac = (travel > 0.f) ? Clamp((-state.scroll.y) / travel, 0.f, 1.f) : 0.f;
    thumb_y = sb_y + thumb_travel * frac;

    // Track
    DrawRectangleRec(track_rect, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    // Thumb — brighter while dragging
    Color thumb_col = state._scroll_dragging
                          ? GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_PRESSED))
                          : GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED));
    DrawRectangleRec({sb_x + 1.f, thumb_y, SCROLLBAR_W - 2.f, thumb_h}, thumb_col);
  }
  else
  {
    state._scroll_dragging = false; // reset if panel shrinks and no longer needs scroll
  }

  // ---- Resize grip triangle (visual affordance) ----
  float gx = state.x + state.w;
  float gy = state.y + panel_h;
  Color grip_col = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));
  DrawTriangle({gx - RESIZE_GRIP, gy}, {gx, gy}, {gx, gy - RESIZE_GRIP}, grip_col);

  return !closed;
}

// ============================================================
//  SkyeUI_DrawGUI  — top-level entry point from DrawGUI()
// ============================================================
void SkyeUI_BeginFrame()
{
  s_top_panel_prev = s_top_panel_this;
  s_top_panel_this = -1;
  s_panel_draw_idx = 0;
}

void SkyeUI_DrawGUI()
{
  HUD_DrawGUI();
}

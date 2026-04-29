-- cs_draw.lua
-- Called by the engine BEFORE BeginDrawing each frame — safe to resize the window here.
function draw(dt)
  -- Consume any pending window resize queued by apply/restore_video_settings.
  -- Doing it here (before BeginDrawing) prevents mid-frame FBO/viewport dimension
  -- mismatches that cause the split-screen artifact.
  if pending_video_apply then
    local p = pending_video_apply
    pending_video_apply = nil
    if p.res then set_window_size(p.res) end
    set_window_mode(p.mode)
  end
end

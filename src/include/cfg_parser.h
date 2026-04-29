#pragma once
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>
#include "global.h"

class Config
{
public:
  std::map<std::string, std::string> data;

  bool load(const std::string &filename)
  {
    std::ifstream file(filename);
    if (!file.is_open())
      return false;

    std::string line;
    while (std::getline(file, line))
    {
      line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
      line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
      if (line.empty() || line[0] == '#')
        continue;
      size_t delimiterPos = line.find('=');
      if (delimiterPos != std::string::npos)
      {
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
        data[key] = value;
      }
    }
    return true;
  }

  // Typed setters
  void set(const std::string &key, const std::string &value) { data[key] = value; }
  void set(const std::string &key, int value) { data[key] = std::to_string(value); }
  void set(const std::string &key, float value)
  {
    std::ostringstream ss;
    ss << value;
    data[key] = ss.str();
  }
  void set(const std::string &key, bool value) { data[key] = value ? "true" : "false"; }

  // Write back to disk preserving comments and line order.
  // Existing keys are updated in-place; new keys appended at the end.
  bool save(const std::string &filename)
  {
    std::ifstream in(filename);
    std::vector<std::string> lines;
    std::set<std::string> written;

    if (in.is_open())
    {
      std::string line;
      while (std::getline(in, line))
      {
        std::string trimmed = line;
        trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
        trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());

        bool replaced = false;
        if (!trimmed.empty() && trimmed[0] != '#')
        {
          size_t pos = trimmed.find('=');
          if (pos != std::string::npos)
          {
            std::string key = trimmed.substr(0, pos);
            if (data.count(key))
            {
              lines.push_back(key + "=" + data[key]);
              written.insert(key);
              replaced = true;
            }
          }
        }
        if (!replaced)
          lines.push_back(line);
      }
      in.close();
    }

    for (auto &[k, v] : data)
      if (!written.count(k))
        lines.push_back(k + "=" + v);

    std::ofstream out(filename);
    if (!out.is_open())
      return false;
    for (auto &l : lines)
      out << l << "\n";
    return true;
  }

  std::string getString(std::string key) { return data[key]; }
  int getInt(std::string key) { return std::stoi(data[key]); }
  float getFloat(std::string key) { return std::stof(data[key]); }
  bool getBool(std::string key) { return data[key] == "true" || data[key] == "1"; }
};

inline void CFG_LoadSettings()
{
  Config settings;
  if (settings.load("gamedata/settings.cfg"))
  {
    if (settings.data.count("screen_width"))
      SCREEN_WIDTH = settings.getInt("screen_width");

    if (settings.data.count("screen_height"))
      SCREEN_HEIGHT = settings.getInt("screen_height");

    if (settings.data.count("render_width"))
      RENDER_WIDTH = settings.getInt("render_width");

    if (settings.data.count("window_mode"))
      RENDER_HEIGHT = settings.getInt("render_height");
    GUI_WIDTH = settings.getInt("gui_width");
    GUI_HEIGHT = settings.getInt("gui_height");
    global_guiscale = settings.getFloat("gui_scale");

    global_window_title = settings.getString("window_title");
    global_server_ip = settings.getString("ip");
    global_server_port = settings.getInt("port");
    global_is_hosting = settings.getBool("is_hosting");
    global_map_to_load = MAP_SOURCE_DIR + settings.getString("map_to_load");

    if (settings.data.count("window_mode"))
      global_window_mode = settings.getInt("window_mode");
    if (settings.data.count("vsync"))
      global_vsync = settings.getBool("vsync");
    if (settings.data.count("msaa4x"))
      global_msaa4x = settings.getBool("msaa4x");
    if (settings.data.count("fov"))
      global_fov = settings.getInt("fov");

    global_texture_filter = settings.getInt("texture_filter");
    global_brightness = settings.getFloat("brightness");
  }
}

inline void CFG_SaveSettings()
{
  Config settings;
  settings.load("gamedata/settings.cfg");

  settings.set("screen_width", SCREEN_WIDTH);
  settings.set("screen_height", SCREEN_HEIGHT);
  settings.set("render_width", RENDER_WIDTH);
  settings.set("render_height", RENDER_HEIGHT);
  settings.set("gui_width", (int)GUI_WIDTH);
  settings.set("gui_height", (int)GUI_HEIGHT);
  settings.set("gui_scale", global_guiscale);
  settings.set("window_title", global_window_title);
  settings.set("ip", global_server_ip);
  settings.set("port", (int)global_server_port);
  settings.set("is_hosting", global_is_hosting);
  settings.set("window_mode", global_window_mode);
  settings.set("vsync", global_vsync);
  settings.set("msaa4x", global_msaa4x);
  settings.set("fov", global_fov);
  settings.set("texture_filter", global_texture_filter);
  settings.set("brightness", global_brightness);

  settings.save("gamedata/settings.cfg");
}

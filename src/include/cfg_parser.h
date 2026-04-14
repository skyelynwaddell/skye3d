#pragma once
#include <fstream>
#include <string>
#include <map>
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
      // Remove whitespace and carriage returns
      line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
      line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

      if (line.empty() || line[0] == '#')
        continue;

      size_t delimiterPos = line.find('=');
      if (delimiterPos != std::string::npos)
      {
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        // Remove quotes from strings if they exist
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

        data[key] = value;
      }
    }
    return true;
  }

  // Helper methods to get typed data
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
    global_window_title = settings.getString("window_title");
    global_server_ip = settings.getString("ip");
    global_server_port = settings.getInt("port");
    global_is_hosting = settings.getBool("is_hosting");
    global_map_to_load = MAP_SOURCE_DIR + settings.getString("map_to_load");

    // // Example of using the other values
    // std::string test = settings.getString("hello_world");
    // int val = settings.getInt("ye");
    // float fVal = settings.getFloat("hello");

    // printf(std::format("IS HOSTING : {}\n", global_is_hosting).c_str());
  }
};
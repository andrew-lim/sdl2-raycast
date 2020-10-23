#include "settingsmanager.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;
using namespace al::raycasting;

bool SettingsManager::loadConfig(const string& filename) {
  ifstream file(filename.c_str());
  string line;
  vector<string> lines;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }
  for (int i=0; i<(int)lines.size(); ++i) {
    string line = lines[ i ];
    string delimiter = "=";
    size_t pos = line.find(delimiter);
    if (pos != string::npos) {
      string key = line.substr(0, pos);
      string value = line.substr(pos+1);
      keyvalues[key]=value;
    }
  }
  return true;
}

string SettingsManager::getString(const string& key, const string& fallback) {
  string value = keyvalues[key];
  if (value.size()) {
   return value;
  }
  return fallback;
}

int SettingsManager::getInt(const std::string& key, int fallback) {
  string value = getString(key, "");
  if (value.size()) {
    return fromString<int>(value);
  }
  return fallback;
}

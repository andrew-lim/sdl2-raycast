#ifndef AL_RAYCASTING_SETTINGSMANAGER_H
#define AL_RAYCASTING_SETTINGSMANAGER_H
#include <sstream>
#include <string>
#include <map>

namespace al {
namespace raycasting {

class SettingsManager {
public:
  bool loadConfig(const std::string& filename);
  std::string getString(const std::string& key, const std::string& fallback);
  int getInt(const std::string& key, int fallback);
  template<typename T>
  static T fromString(const std::string& str) {
    std::istringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
  }
private:
  std::map<std::string,std::string> keyvalues;
};

} // raycasting
} // al

#endif

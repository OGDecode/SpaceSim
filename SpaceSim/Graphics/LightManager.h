#pragma once
#include <map>
#include <string>

#include "light.h"

class LightManager
{
public:
    LightManager() {}
    ~LightManager() {}

    void addLight(const std::string& name, const Light& light);
    const Light* getLight(const std::string& name) const;
    Light* getLight(const std::string& name);
    
    const std::vector<const Light*> getLights() const
    {
        std::vector<const Light*> lights;
        for (LightMap::const_iterator it = m_lights.begin(); it != m_lights.end(); ++it)
        {
            lights.push_back(&(it->second));
        }
        return lights;
    }
private:
    typedef std::map< unsigned int, Light> LightMap;
    typedef std::pair< unsigned int, Light> LightPair;

    LightMap m_lights;
};


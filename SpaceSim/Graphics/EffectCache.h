#pragma once

#include "Effect.h"

#include <map>
#include <string>

class EffectCache
{
public:
    EffectCache();
    ~EffectCache();

    const Effect* EffectCache::createEffect(Resource* resource, const std::string& resourceFileName);

    const Effect* getEffect(const std::string& name) const;
    void addEffect(const std::string& name, const Effect& effect);
private:

    std::map<unsigned int, Effect> m_effects;
};


#pragma once

#include "Loader/ModelLoaders/ModelLoader.h"

class MmlLoader : public ModelLoader
{
public:
    MmlLoader() {}
    ~MmlLoader() {}
    virtual Model* LoadModel(Resource* resource, const ShaderInstance& shaderInstance, const std::string& fileName) override;
private:
};
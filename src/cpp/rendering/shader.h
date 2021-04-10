#pragma once

#include "platform/vk_common.h"

enum class ShaderType 
{
    vs,
    ps,
    cs,
};

struct ShaderID 
{
    ShaderID(LiteralString _name)
        : m_name(_name) {};

    void addDefine(LiteralString _define) { m_defines.push_back(_define); }

    LiteralString m_name;
    SmallVector<LiteralString, 8> m_defines;
};

class ShaderCompiler 
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    vector<u8> compileShader(const ShaderID& _id, const ShaderType& _type, LiteralString _entryPoint);

private:
    void* m_dxcDll;
};

class ShaderManager 
{
public:

private:
    std::unordered_map<ShaderID, vk::ShaderModule> m_cache;
};
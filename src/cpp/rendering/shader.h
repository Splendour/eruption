#pragma once

#include "platform/vk_common.h"
#include "core/hash.h"

enum class ShaderType : u8
{
    vs,
    ps,
    cs,
};

struct ShaderID 
{
    ShaderID(LiteralString _name, ShaderType _type, LiteralString _entryPoint)
        : m_name(_name)
        , m_entryPoint(_entryPoint)
        , m_type(_type) {};

    void addDefine(LiteralString _define) { m_defines.push_back(_define); }

    bool operator==(const ShaderID& _o) const
    {
        return m_name == _o.m_name && m_entryPoint == _o.m_entryPoint // strcmp instead of ptr compare?
            && m_type == _o.m_type && m_defines == _o.m_defines;
    }

    LiteralString m_name;
    LiteralString m_entryPoint;
    ShaderType m_type;
    SmallVector<LiteralString, 8> m_defines;

};

namespace std {
template <>
struct hash<ShaderID> {
    std::size_t operator()(ShaderID const& s) const noexcept
    {
        std::size_t h = std::hash<u8> {}(toUnderlyingType(s.m_type));
        hash_combine(h, (s.m_name)); // hash with content instead of ptr?
        hash_combine(h, (s.m_entryPoint));
        hash_combine_array(h, s.m_defines);
        return h;
    }
};
}

class ShaderCompiler 
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    vector<u32> compileShader(const ShaderID& _id);

private:
    void* m_dxcDll;
};

class ShaderManager 

{
public:
    vk::ShaderModule getShaderModule(const ShaderID& _id);

    ShaderManager();
    ~ShaderManager();

private:
    ShaderCompiler m_compiler;
    std::unordered_map<ShaderID, vk::ShaderModule> m_cache;
};
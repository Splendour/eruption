#include "common.h"

#include "shader.h"

#include "dxc/wrl.h"
#include "dxc/dxcapi.h"
#include <fstream>

const wchar_t* shaderTypeToProfile(ShaderType _shaderType)
{
    switch (_shaderType) 
    {
    case ShaderType::cs:
        return L"cs_6_0";
    case ShaderType::vs:
        return L"vs_6_0";
    case ShaderType::ps:
        return L"ps_6_0";
    }
    ASSERT_NOT_IMPLEMENTED();
    return L"";
}

DxcCreateInstanceProc createDxcInstance;
ShaderCompiler::ShaderCompiler()
{
    LiteralString dxcPath = "dxcompiler.dll";
    m_dxcDll = ::LoadLibraryA(dxcPath);
    VERIFY_TRUE_MSG(m_dxcDll, "{} is missing from binary dir", dxcPath);
    createDxcInstance = (DxcCreateInstanceProc)::GetProcAddress((HMODULE)m_dxcDll, "DxcCreateInstance");
    ASSERT_TRUE(createDxcInstance);
}

ShaderCompiler::~ShaderCompiler()
{
    FreeLibrary((HMODULE)m_dxcDll);
}

size_t getFileSize(std::ifstream& _file)
{
    _file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize size = _file.gcount();
    _file.clear(); //  Since ignore will have set eof.notep
    _file.seekg(0, std::ios_base::beg);

    ASSERT_TRUE(size > 0);
    return (size_t)size;
}

std::string readFileIntoString(LiteralString _filePath)
{
    std::ifstream file(_filePath, std::ios::in | std::ios::binary);
    ASSERT_TRUE_MSG(file.is_open(), "Missing shader file {}", _filePath);

    size_t fileSize = getFileSize(file);
    std::string shaderFileSrc;
    shaderFileSrc.reserve(fileSize);
    shaderFileSrc.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return shaderFileSrc;
}

std::string resolveShaderPath(LiteralString _shaderName)
{
#ifdef DEVELOPMENT_MODE
    std::string path = SHADERS_FOLDER;

#else
    std::string path = "shaders";
#endif
    return path + "\\" + _shaderName;
}

std::wstring to_wstring(const std::string& _str)
{
    if (_str.empty())
        return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, &_str[0], (int)_str.size(), NULL, 0);

    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &_str[0], (int)_str.size(), &wide[0], size);
    return wide;
}

interface DefaultIncluder : public IDxcIncludeHandler
{
    IDxcLibrary* m_lib;

public:
    DefaultIncluder(IDxcLibrary * _lib)
        : m_lib(_lib)
    {
    }
    HRESULT QueryInterface(const IID&, void**) { return S_OK; }
    ULONG AddRef() { return 0; }
    ULONG Release() { return 0; }
    HRESULT LoadSource(LPCWSTR _fileName, IDxcBlob * *_output)
    {
        char charPath[2048];
        sprintf_s<2048>(charPath, ("%ws"), _fileName);
        std::string fullPath = resolveShaderPath(charPath);

        IDxcBlobEncoding* encodedShader;
        u32 codePage = CP_UTF8;
        VERIFY_TRUE(SUCCEEDED(m_lib->CreateBlobFromFile(to_wstring(fullPath).c_str(), &codePage, &encodedShader)));
        *_output = encodedShader;
        return S_OK;
    }
};

using ArgsVector = SmallVector<const wchar_t*, 8>;
ArgsVector getDefaultArgs()
{
    ArgsVector args;
    args.push_back(L"-O3");
    args.push_back(L"/Zi");
    args.push_back(L"-spirv");
    return args;
}

void handleShaderCompileError(IDxcLibrary* _lib, IDxcBlob* _errorBlob, LiteralString _shaderName)
{
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> pErrorUtf8;
    _lib->GetBlobAsUtf8(_errorBlob, &pErrorUtf8);

    logError("Error compiling shader: {}", _shaderName);

    std::string errMsg = std::string((char*)pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize());
    VERIFY_TRUE_MSG(false, "{}You may edit the shader and continue to attempt recompilation.", errMsg);
}

vector<u8> ShaderCompiler::compileShader(const ShaderID& _id, const ShaderType& _type, LiteralString _entryPoint)
{
    Microsoft::WRL::ComPtr<IDxcCompiler2> compiler;
    VERIFY_TRUE(SUCCEEDED(createDxcInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()))));

    Microsoft::WRL::ComPtr<IDxcLibrary> lib;
    VERIFY_TRUE(SUCCEEDED(createDxcInstance(CLSID_DxcLibrary, IID_PPV_ARGS(lib.GetAddressOf()))));

    DefaultIncluder includer(lib.Get());

    LiteralString name = _id.m_name;
    SmallVector<std::wstring, 8> wcharDefines; 
    SmallVector<DxcDefine, 8> dxcDefines;
    for (LiteralString def : _id.m_defines) {
        wcharDefines.push_back(to_wstring(def));
        DxcDefine dxcDef = { wcharDefines.back().c_str(), L"1" };
        dxcDefines.emplace_back(dxcDef);
    }

    std::string shaderPath = resolveShaderPath(name);
    std::wstring shaderName = to_wstring(name);
    vector<u8> result;
    bool recompile = false;

    do {
        // Preprocess
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSrcPreprocessed;
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSrc;
        std::string shaderFileSrc = readFileIntoString(shaderPath.c_str());
        VERIFY_TRUE(SUCCEEDED(lib->CreateBlobWithEncodingFromPinned(
            (LPBYTE)shaderFileSrc.c_str(), (UINT32)shaderFileSrc.size(), CP_UTF8, shaderSrc.GetAddressOf())));

        IDxcOperationResult* preprocessResult;
        VERIFY_TRUE(SUCCEEDED(compiler->Preprocess(shaderSrc.Get(), shaderName.c_str(), nullptr, 0, dxcDefines.data(), dxcDefines.size(), &includer, &preprocessResult)));
        Microsoft::WRL::ComPtr<IDxcBlob> preProcessedHlsl;
        VERIFY_TRUE(SUCCEEDED(preprocessResult->GetResult(preProcessedHlsl.GetAddressOf())));

        //std::string ssss((char*)preProcessedHlsl->GetBufferPointer(), preProcessedHlsl->GetBufferSize());
        //logInfo(ssss);

        HRESULT preprocessStatus;
        preprocessResult->GetStatus(&preprocessStatus);

        if (FAILED(preprocessStatus)) // is this possible?
        {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
            preprocessResult->GetErrorBuffer(errorBlob.GetAddressOf());
            handleShaderCompileError(lib.Get(), errorBlob.Get(), name);
        }

        VERIFY_TRUE(SUCCEEDED(lib->CreateBlobWithEncodingFromPinned(
            (LPBYTE)preProcessedHlsl->GetBufferPointer(), (UINT32)preProcessedHlsl->GetBufferSize(),
            CP_UTF8, shaderSrcPreprocessed.GetAddressOf())));

        // print((char*)shaderSrcPreprocessed->GetBufferPointer());
        // todo: cache using preprocessor output hash

        // Compile
        Microsoft::WRL::ComPtr<IDxcOperationResult> compilationResult;

        auto args = getDefaultArgs();
        VERIFY_TRUE(SUCCEEDED(compiler->Compile(shaderSrcPreprocessed.Get(),
            shaderName.c_str(), to_wstring(_entryPoint).c_str(),
            shaderTypeToProfile(_type), args.data(), (u32)args.size(), nullptr, 0, &includer, compilationResult.GetAddressOf())));

        HRESULT compileStatus;
        compilationResult->GetStatus(&compileStatus);
        if (FAILED(compileStatus)) {
            recompile = true;
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
            compilationResult->GetErrorBuffer(errorBlob.GetAddressOf());
            handleShaderCompileError(lib.Get(), errorBlob.Get(), name);
        } else {
            recompile = false;
            Microsoft::WRL::ComPtr<IDxcBlob> code;
            VERIFY_TRUE(SUCCEEDED(compilationResult->GetResult(code.GetAddressOf())));
            result.resize(code->GetBufferSize());
            memcpy(result.data(), code->GetBufferPointer(), code->GetBufferSize());
        }
    } while (recompile);

    return result;
}
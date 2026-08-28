/*
 * Query a named symbol and its recursive type layout from a Microsoft PDB.
 *
 * This uses the DIA SDK DLL without COM registration so the reconstruction
 * audit can inspect the matched retail PDB with the Visual Studio toolchain
 * already present on the host.
 */

#include <Windows.h>
#include <dia2.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>

namespace {

IDiaSession *g_session;

const wchar_t *tag_name(DWORD tag)
{
    switch (tag) {
    case SymTagNull: return L"null";
    case SymTagExe: return L"exe";
    case SymTagCompiland: return L"compiland";
    case SymTagFunction: return L"function";
    case SymTagBlock: return L"block";
    case SymTagData: return L"data";
    case SymTagUDT: return L"udt";
    case SymTagEnum: return L"enum";
    case SymTagFunctionType: return L"function-type";
    case SymTagPointerType: return L"pointer";
    case SymTagArrayType: return L"array";
    case SymTagBaseType: return L"base";
    case SymTagTypedef: return L"typedef";
    case SymTagBaseClass: return L"base-class";
    case SymTagFunctionArgType: return L"argument";
    default: return L"other";
    }
}

void indent(unsigned depth)
{
    for (unsigned index = 0; index < depth; ++index) {
        std::fputws(L"  ", stdout);
    }
}

void print_type(IDiaSymbol *symbol, unsigned depth, unsigned limit)
{
    DWORD tag = SymTagNull;
    DWORD sym_index = 0;
    DWORD type_index = 0;
    DWORD data_kind = 0;
    DWORD location_type = 0;
    DWORD relative_virtual_address = 0;
    LONG offset = 0;
    ULONGLONG length = 0;
    DWORD count = 0;
    BOOL is_const = FALSE;
    BOOL is_volatile = FALSE;
    BOOL is_unaligned = FALSE;
    BOOL is_reference = FALSE;
    BSTR name = nullptr;
    VARIANT value;
    VariantInit(&value);

    if (symbol == nullptr) {
        return;
    }
    (void)symbol->get_symTag(&tag);
    (void)symbol->get_symIndexId(&sym_index);
    (void)symbol->get_typeId(&type_index);
    (void)symbol->get_name(&name);
    (void)symbol->get_length(&length);
    (void)symbol->get_count(&count);
    (void)symbol->get_constType(&is_const);
    (void)symbol->get_volatileType(&is_volatile);
    (void)symbol->get_unalignedType(&is_unaligned);
    (void)symbol->get_reference(&is_reference);
    (void)symbol->get_dataKind(&data_kind);
    (void)symbol->get_locationType(&location_type);
    (void)symbol->get_relativeVirtualAddress(&relative_virtual_address);
    (void)symbol->get_offset(&offset);
    const bool has_value = SUCCEEDED(symbol->get_value(&value));

    indent(depth);
    std::fwprintf(
        stdout,
        L"tag=%ls sym=0x%X type=0x%X name=%ls rva=0x%X length=%llu count=%u",
        tag_name(tag),
        sym_index,
        type_index,
        name != nullptr ? name : L"<unnamed>",
        relative_virtual_address,
        length,
        count);
    if (is_const || is_volatile || is_unaligned || is_reference) {
        std::fputws(L" qualifiers=", stdout);
        if (is_const) std::fputws(L"const,", stdout);
        if (is_volatile) std::fputws(L"volatile,", stdout);
        if (is_unaligned) std::fputws(L"unaligned,", stdout);
        if (is_reference) std::fputws(L"reference,", stdout);
    }
    if (tag == SymTagData || tag == SymTagBaseClass) {
        std::fwprintf(
            stdout,
            L" dataKind=%u location=%u offset=%ld",
            data_kind,
            location_type,
            offset);
    }
    if (has_value) {
        switch (value.vt) {
        case VT_I1: std::fwprintf(stdout, L" value=%d", value.cVal); break;
        case VT_UI1: std::fwprintf(stdout, L" value=%u", value.bVal); break;
        case VT_I2: std::fwprintf(stdout, L" value=%d", value.iVal); break;
        case VT_UI2: std::fwprintf(stdout, L" value=%u", value.uiVal); break;
        case VT_I4: std::fwprintf(stdout, L" value=%ld", value.lVal); break;
        case VT_UI4: std::fwprintf(stdout, L" value=%lu", value.ulVal); break;
        case VT_I8: std::fwprintf(stdout, L" value=%lld", value.llVal); break;
        case VT_UI8: std::fwprintf(stdout, L" value=%llu", value.ullVal); break;
        default: std::fwprintf(stdout, L" value-vt=%u", value.vt); break;
        }
    }
    std::fputwc(L'\n', stdout);
    VariantClear(&value);
    if (name != nullptr) {
        SysFreeString(name);
    }
    if (depth >= limit) {
        return;
    }

    if (tag == SymTagFunction || tag == SymTagData ||
        tag == SymTagPointerType ||
        tag == SymTagArrayType || tag == SymTagTypedef ||
        tag == SymTagFunctionArgType) {
        IDiaSymbol *type = nullptr;

        if ((FAILED(symbol->get_type(&type)) || type == nullptr) &&
            g_session != nullptr && type_index != 0) {
            (void)g_session->symbolById(type_index, &type);
        }
        if (type != nullptr) {
            print_type(type, depth + 1, limit);
            type->Release();
        }
    }
    if (tag == SymTagFunction || tag == SymTagBlock ||
        tag == SymTagUDT || tag == SymTagEnum ||
        tag == SymTagFunctionType) {
        IDiaEnumSymbols *children = nullptr;

        if (SUCCEEDED(symbol->findChildren(
                SymTagNull, nullptr, nsNone, &children)) &&
            children != nullptr) {
            IDiaSymbol *child = nullptr;
            ULONG fetched = 0;

            while (children->Next(1, &child, &fetched) == S_OK &&
                   fetched == 1) {
                print_type(child, depth + 1, limit);
                child->Release();
                child = nullptr;
            }
            children->Release();
        }
    }
}

unsigned print_exact_data(
    IDiaSymbol *scope, DWORD rva, unsigned depth)
{
    IDiaEnumSymbols *symbols = nullptr;
    unsigned matches = 0;

    if (scope == nullptr || FAILED(scope->findChildren(
            SymTagData, nullptr, nsNone, &symbols)) ||
        symbols == nullptr) {
        return 0;
    }
    IDiaSymbol *symbol = nullptr;
    ULONG fetched = 0;
    while (symbols->Next(1, &symbol, &fetched) == S_OK && fetched == 1) {
        DWORD symbol_rva = 0;

        if (SUCCEEDED(symbol->get_relativeVirtualAddress(&symbol_rva)) &&
            symbol_rva == rva) {
            std::fputws(L"exact-data: ", stdout);
            print_type(symbol, 0, depth);
            ++matches;
        }
        symbol->Release();
        symbol = nullptr;
    }
    symbols->Release();
    return matches;
}

HRESULT create_data_source(
    const wchar_t *dia_dll, IDiaDataSource **data_source)
{
    using DllGetClassObjectFn = HRESULT(STDAPICALLTYPE *)(
        REFCLSID, REFIID, void **);
    HMODULE module = LoadLibraryW(dia_dll);
    DllGetClassObjectFn get_class_object;
    IClassFactory *factory = nullptr;
    HRESULT result;

    if (module == nullptr) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    get_class_object = reinterpret_cast<DllGetClassObjectFn>(
        GetProcAddress(module, "DllGetClassObject"));
    if (get_class_object == nullptr) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    result = get_class_object(
        CLSID_DiaSource, IID_IClassFactory,
        reinterpret_cast<void **>(&factory));
    if (FAILED(result)) {
        return result;
    }
    result = factory->CreateInstance(
        nullptr, __uuidof(IDiaDataSource),
        reinterpret_cast<void **>(data_source));
    factory->Release();
    return result;
}

} // namespace

int wmain(int argc, wchar_t **argv)
{
    IDiaDataSource *data_source = nullptr;
    IDiaSession *session = nullptr;
    IDiaSymbol *global = nullptr;
    IDiaEnumSymbols *matches = nullptr;
    HRESULT result;
    unsigned depth = 8;
    int exit_code = EXIT_FAILURE;

    if (argc < 4 || argc > 5) {
        std::fwprintf(
            stderr,
            L"usage: %ls <msdia140.dll> <game.pdb> <symbol> [depth]\n",
            argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 5) {
        depth = static_cast<unsigned>(std::wcstoul(argv[4], nullptr, 0));
    }
    result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        std::fwprintf(stderr, L"CoInitializeEx failed: 0x%08X\n", result);
        return EXIT_FAILURE;
    }
    result = create_data_source(argv[1], &data_source);
    if (FAILED(result)) {
        std::fwprintf(stderr, L"DIA creation failed: 0x%08X\n", result);
        goto cleanup;
    }
    result = data_source->loadDataFromPdb(argv[2]);
    if (FAILED(result)) {
        std::fwprintf(stderr, L"PDB load failed: 0x%08X\n", result);
        goto cleanup;
    }
    result = data_source->openSession(&session);
    if (FAILED(result)) {
        std::fwprintf(stderr, L"session open failed: 0x%08X\n", result);
        goto cleanup;
    }
    g_session = session;
    result = session->get_globalScope(&global);
    if (FAILED(result)) {
        std::fwprintf(stderr, L"global scope failed: 0x%08X\n", result);
        goto cleanup;
    }
    if (argv[3][0] == L'@') {
        IDiaSymbol *symbol = nullptr;
        IDiaEnumSymbols *data_symbols = nullptr;
        LONG displacement = 0;
        DWORD rva = static_cast<DWORD>(
            std::wcstoul(argv[3] + 1, nullptr, 0));

        result = session->findSymbolByRVAEx(
            rva, SymTagNull, &symbol, &displacement);
        if (FAILED(result) || symbol == nullptr) {
            std::fwprintf(
                stderr,
                L"RVA query failed: rva=0x%X result=0x%08X\n",
                rva,
                result);
            goto cleanup;
        }
        std::fwprintf(
            stdout,
            L"RVA 0x%X displacement=0x%lX\n",
            rva,
            displacement);
        print_type(symbol, 0, depth);
        symbol->Release();
        (void)print_exact_data(global, rva, depth);
        if (SUCCEEDED(global->findChildren(
                SymTagCompiland, nullptr, nsNone, &data_symbols)) &&
            data_symbols != nullptr) {
            IDiaSymbol *compiland = nullptr;
            ULONG fetched = 0;

            while (data_symbols->Next(
                       1, &compiland, &fetched) == S_OK &&
                   fetched == 1) {
                (void)print_exact_data(compiland, rva, depth);
                compiland->Release();
                compiland = nullptr;
            }
            data_symbols->Release();
        }
        exit_code = EXIT_SUCCESS;
        goto cleanup;
    }
    result = global->findChildren(
        SymTagNull, argv[3], nsCaseSensitive, &matches);
    if (FAILED(result) || matches == nullptr) {
        std::fwprintf(stderr, L"symbol query failed: 0x%08X\n", result);
        goto cleanup;
    }
    {
        IDiaSymbol *symbol = nullptr;
        ULONG fetched = 0;
        unsigned count = 0;

        while (matches->Next(1, &symbol, &fetched) == S_OK &&
               fetched == 1) {
            print_type(symbol, 0, depth);
            symbol->Release();
            symbol = nullptr;
            ++count;
        }
        if (count == 0) {
            std::fwprintf(stderr, L"symbol not found: %ls\n", argv[3]);
            goto cleanup;
        }
    }
    exit_code = EXIT_SUCCESS;

cleanup:
    g_session = nullptr;
    if (matches != nullptr) matches->Release();
    if (global != nullptr) global->Release();
    if (session != nullptr) session->Release();
    if (data_source != nullptr) data_source->Release();
    CoUninitialize();
    return exit_code;
}

#include "dict.h"

#include <spellcheck.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

namespace ruswitcher {
namespace {

ComPtr<ISpellCheckerFactory> g_factory;
bool g_factory_attempted = false;
std::unordered_map<std::wstring, ComPtr<ISpellChecker>> g_checkers;

void init_factory() noexcept {
    if (g_factory_attempted) return;
    g_factory_attempted = true;
    HRESULT hr = CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&g_factory));
    if (FAILED(hr)) {
        g_factory = nullptr;
    }
}

ComPtr<ISpellChecker> get_checker(const wchar_t* lang_tag) noexcept {
    if (!lang_tag) return nullptr;
    init_factory();
    if (!g_factory) return nullptr;

    const std::wstring key(lang_tag);
    auto it = g_checkers.find(key);
    if (it != g_checkers.end()) return it->second;

    BOOL supported = FALSE;
    HRESULT hr = g_factory->IsSupported(lang_tag, &supported);
    if (FAILED(hr) || !supported) {
        g_checkers[key] = nullptr;
        return nullptr;
    }

    ComPtr<ISpellChecker> checker;
    hr = g_factory->CreateSpellChecker(lang_tag, &checker);
    if (FAILED(hr) || !checker) {
        g_checkers[key] = nullptr;
        return nullptr;
    }

    g_checkers[key] = checker;
    return checker;
}

}  // namespace

bool Dict::is_available() noexcept {
    init_factory();
    return g_factory != nullptr;
}

const wchar_t* Dict::get_lang_tag(HKL layout) noexcept {
    const LANGID lang = PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(layout)));
    if (lang == LANG_RUSSIAN) return L"ru-RU";
    if (lang == LANG_ENGLISH) return L"en-US";
    if (lang == LANG_HEBREW) return L"he-IL";
    if (lang == LANG_UKRAINIAN) return L"uk-UA";
    if (lang == LANG_BELARUSIAN) return L"be-BY";
    if (lang == LANG_GERMAN) return L"de-DE";
    if (lang == LANG_FRENCH) return L"fr-FR";
    if (lang == LANG_SPANISH) return L"es-ES";
    return L"en-US";
}

bool Dict::is_valid_word(std::wstring_view word, HKL layout) noexcept {
    return is_valid_word(word, get_lang_tag(layout));
}

bool Dict::is_valid_word(std::wstring_view word, const wchar_t* lang_tag) noexcept {
    if (word.empty()) return false;
    auto checker = get_checker(lang_tag);
    if (!checker) return false;

    std::wstring null_term(word);
    ComPtr<IEnumSpellingError> errors;
    HRESULT hr = checker->Check(null_term.c_str(), &errors);
    if (FAILED(hr) || !errors) return false;

    ComPtr<ISpellingError> error;
    hr = errors->Next(&error);
    return (hr == S_FALSE || error == nullptr);
}

}  // namespace ruswitcher

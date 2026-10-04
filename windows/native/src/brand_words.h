#pragma once

#include <algorithm>
#include <cwctype>
#include <string>
#include <string_view>

namespace ruswitcher {

// Built-in list of brands and IT terms used as TARGETS of auto-conversion
// (issue #34, parity with macOS 3.5.0b).
// 154 entries sorted alphabetically in lowercase.
inline constexpr std::wstring_view kBrandWords[] = {
    L"aider", L"airpods", L"airtable", L"android", L"ansible", L"anthropic", L"appstore", L"arduino",
    L"asana", L"avito", L"backend", L"bitbucket", L"bitrix", L"chatgpt", L"chrome", L"claude",
    L"clickup", L"cline", L"cloudflare", L"codex", L"confluence", L"copilot", L"cursor", L"dalle",
    L"debian", L"deepseek", L"devin", L"devops", L"discord", L"django", L"docker", L"dropbox",
    L"excel", L"facebook", L"fastapi", L"fedora", L"figma", L"finetune", L"firebase", L"firefox",
    L"flask", L"frontend", L"fullstack", L"gemini", L"gigachat", L"github", L"gitlab", L"gmail",
    L"goland", L"golang", L"google", L"grok", L"habr", L"heroku", L"homebrew", L"hotfix",
    L"huawei", L"huggingface", L"icloud", L"imac", L"instagram", L"intel", L"intellij", L"ipad",
    L"ipados", L"iphone", L"javascript", L"jetbrains", L"jira", L"kafka", L"kandinsky", L"kotlin",
    L"kubernetes", L"langchain", L"laravel", L"linkedin", L"llama", L"localhost", L"lovable", L"macbook",
    L"macos", L"midjourney", L"miro", L"mistral", L"mlops", L"mongodb", L"mysql", L"netflix",
    L"netlify", L"nextjs", L"nginx", L"nintendo", L"nodejs", L"notion", L"nvidia", L"ollama",
    L"onedrive", L"openai", L"openrouter", L"outlook", L"ozon", L"perplexity", L"pinterest", L"playstation",
    L"pnpm", L"postgres", L"postgresql", L"postman", L"powerpoint", L"pycharm", L"qwen", L"raspberry",
    L"reddit", L"redis", L"replit", L"safari", L"samsung", L"skype", L"sora", L"spotify",
    L"stablediffusion", L"steam", L"supabase", L"symfony", L"tabnine", L"teamlead", L"telegram", L"terraform",
    L"tesla", L"tiktok", L"trello", L"twitch", L"twitter", L"typescript", L"ubuntu", L"vercel",
    L"vibecoding", L"viber", L"vite", L"vscode", L"watchos", L"webpack", L"webstorm", L"whatsapp",
    L"wildberries", L"windsurf", L"wordpress", L"xbox", L"xcode", L"xiaomi", L"yandex", L"yandexgpt",
    L"youtube", L"zapier"
};

inline bool is_brand_word(std::wstring_view word) noexcept {
    if (word.size() < 3) return false;
    std::wstring lower;
    lower.reserve(word.size());
    for (wchar_t c : word) {
        lower.push_back(static_cast<wchar_t>(std::towlower(c)));
    }
    return std::binary_search(std::begin(kBrandWords), std::end(kBrandWords), lower);
}

}  // namespace ruswitcher

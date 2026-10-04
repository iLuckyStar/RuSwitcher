namespace RuSwitcher.Win.Core;

/// <summary>
/// Built-in list of brands and IT terms used as TARGETS of auto-conversion (issue #34, parity with macOS 3.5.0b).
/// The system spellchecker does not know many modern tech terms, so "срфепзе" typed in the Russian layout
/// would not become "chatgpt" without this dictionary.
/// </summary>
internal static class BrandWords
{
    public static readonly HashSet<string> All = new(StringComparer.OrdinalIgnoreCase)
    {
        // AI
        "chatgpt", "openai", "anthropic", "claude", "gemini", "copilot", "midjourney", "perplexity",
        "ollama", "llama", "mistral", "deepseek", "qwen", "grok", "dalle", "sora", "huggingface",
        "langchain", "kandinsky", "gigachat", "yandexgpt", "cursor", "windsurf", "cline", "aider",
        "codex", "devin", "tabnine", "lovable", "replit", "openrouter", "stablediffusion",
        // Development
        "github", "gitlab", "bitbucket", "docker", "kubernetes", "terraform", "ansible", "nginx",
        "redis", "postgres", "postgresql", "mysql", "mongodb", "kafka", "nextjs", "nodejs", "pnpm",
        "webpack", "vite", "django", "flask", "fastapi", "golang", "kotlin", "typescript",
        "javascript", "laravel", "symfony", "bitrix", "wordpress", "vscode", "xcode", "jetbrains",
        "intellij", "pycharm", "webstorm", "goland", "postman", "figma", "miro", "jira", "confluence",
        "notion", "trello", "asana", "clickup", "airtable", "zapier", "supabase", "firebase",
        "vercel", "netlify", "heroku", "cloudflare", "devops", "mlops", "hotfix", "fullstack",
        "teamlead", "vibecoding", "finetune", "frontend", "backend", "localhost",
        // Services & Hardware
        "telegram", "whatsapp", "viber", "discord", "youtube", "tiktok", "instagram", "facebook",
        "twitter", "reddit", "habr", "ozon", "wildberries", "avito", "yandex", "google", "iphone",
        "ipad", "macbook", "imac", "macos", "ipados", "watchos", "airpods", "android", "samsung",
        "xiaomi", "huawei", "ubuntu", "debian", "fedora", "firefox", "chrome", "safari", "spotify",
        "netflix", "skype", "steam", "twitch", "linkedin", "pinterest", "dropbox", "gmail",
        "outlook", "powerpoint", "excel", "onedrive", "icloud", "appstore", "playstation", "xbox",
        "nintendo", "tesla", "nvidia", "intel", "raspberry", "arduino", "homebrew",
    };
}

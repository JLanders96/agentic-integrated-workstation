#include "FileCategoryPolicy.hpp"

#include "DocumentCategoryPolicy.hpp"
#include "ImageCategoryPolicy.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <string>
#include <string_view>

namespace {

std::string to_lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalize_file_name(std::string_view file_name)
{
    const auto newline = file_name.find('\n');
    return to_lower_copy(std::string(file_name.substr(0, newline)));
}

bool ends_with(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

bool matches_any_suffix(std::string_view value, std::initializer_list<std::string_view> suffixes)
{
    return std::any_of(suffixes.begin(), suffixes.end(), [&](std::string_view suffix) {
        return ends_with(value, suffix);
    });
}

const std::vector<std::string>& document_categories()
{
    static const std::vector<std::string> categories = {
        "Documents", "Presentations", "Spreadsheets", "Data Exports", "Configs"
    };
    return categories;
}

const std::vector<std::string>& image_categories()
{
    static const std::vector<std::string> categories = {"Images"};
    return categories;
}

const std::vector<std::string>& software_categories()
{
    static const std::vector<std::string> categories = {
        "Software", "Installers", "Drivers", "Operating Systems", "Other"
    };
    return categories;
}

const std::vector<std::string>& archive_categories()
{
    static const std::vector<std::string> categories = {
        "Archives", "Software", "Data Exports", "Other"
    };
    return categories;
}

const std::vector<std::string>& audio_categories()
{
    static const std::vector<std::string> categories = {"Music", "Other"};
    return categories;
}

const std::vector<std::string>& video_categories()
{
    static const std::vector<std::string> categories = {"Videos", "Other"};
    return categories;
}

const std::vector<std::string>& ebook_categories()
{
    static const std::vector<std::string> categories = {"Ebooks", "Documents", "Other"};
    return categories;
}

const std::vector<std::string>& font_categories()
{
    static const std::vector<std::string> categories = {"Fonts", "Other"};
    return categories;
}

const std::vector<std::string>& generic_categories()
{
    static const std::vector<std::string> categories = {
        "Documents", "Images", "Videos", "Music", "Software", "Archives",
        "Data Exports", "Configs", "Drivers", "Operating Systems", "Ebooks",
        "Fonts", "Other"
    };
    return categories;
}

bool is_software_artifact_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name,
                              {".exe", ".msi", ".msix", ".msixbundle", ".appx",
                               ".appxbundle", ".deb", ".rpm", ".pkg", ".dmg",
                               ".appimage", ".apk", ".run", ".bat", ".cmd", ".com"});
}

bool is_archive_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name,
                              {".zip", ".7z", ".rar", ".tar", ".gz", ".bz2", ".xz",
                               ".tgz", ".tbz", ".tbz2", ".txz", ".tar.gz",
                               ".tar.bz2", ".tar.xz"});
}

bool is_audio_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name,
                              {".aac", ".aif", ".aiff", ".alac", ".ape", ".flac",
                               ".m4a", ".mp3", ".ogg", ".oga", ".opus", ".wav",
                               ".wma"});
}

bool is_video_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name,
                              {".3gp", ".avi", ".flv", ".m4v", ".mkv", ".mov",
                               ".mp4", ".mpeg", ".mpg", ".mts", ".m2ts", ".ts",
                               ".webm", ".wmv"});
}

bool is_ebook_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name, {".epub", ".mobi", ".azw", ".azw3", ".fb2"});
}

bool is_font_file_name(const std::string& file_name)
{
    return matches_any_suffix(file_name, {".ttf", ".otf", ".woff", ".woff2"});
}

FileCategoryPolicy::MainCategorySelection make_selection(std::string family_name,
                                                         const std::vector<std::string>& categories)
{
    return FileCategoryPolicy::MainCategorySelection{std::move(family_name), categories};
}

} // namespace

namespace FileCategoryPolicy {

MainCategorySelection determine_main_category_selection(const std::string& file_name,
                                                        FileType file_type)
{
    if (file_type != FileType::File) {
        return {};
    }

    if (ImageCategoryPolicy::is_supported_image_file_name(file_name)) {
        return make_selection("image", image_categories());
    }

    if (DocumentCategoryPolicy::is_supported_document_file_name(file_name)) {
        return make_selection("document", document_categories());
    }

    const std::string normalized_file_name = normalize_file_name(file_name);
    if (normalized_file_name.empty()) {
        return make_selection("generic", generic_categories());
    }

    if (is_software_artifact_file_name(normalized_file_name)) {
        return make_selection("software", software_categories());
    }

    if (is_archive_file_name(normalized_file_name)) {
        return make_selection("archive", archive_categories());
    }

    if (is_video_file_name(normalized_file_name)) {
        return make_selection("video", video_categories());
    }

    if (is_audio_file_name(normalized_file_name)) {
        return make_selection("audio", audio_categories());
    }

    if (is_ebook_file_name(normalized_file_name)) {
        return make_selection("ebook", ebook_categories());
    }

    if (is_font_file_name(normalized_file_name)) {
        return make_selection("font", font_categories());
    }

    return make_selection("generic", generic_categories());
}

} // namespace FileCategoryPolicy

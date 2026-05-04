#include "ArtifactCategoryPolicy.hpp"

#include "FileCategoryPolicy.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class ArtifactFamily {
    None,
    Software,
    Archive
};

std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string collapse_spaces_copy(std::string value)
{
    std::string collapsed;
    collapsed.reserve(value.size());
    bool previous_space = false;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            collapsed.push_back(static_cast<char>(std::tolower(ch)));
            previous_space = false;
            continue;
        }
        if (!previous_space && !collapsed.empty()) {
            collapsed.push_back(' ');
        }
        previous_space = true;
    }
    return trim_copy(std::move(collapsed));
}

std::string normalize_match_text(const std::string& value)
{
    return collapse_spaces_copy(value);
}

std::vector<std::string> split_tokens(const std::string& normalized_text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (unsigned char ch : normalized_text) {
        if (std::isspace(ch)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(static_cast<char>(ch));
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

bool contains_token(const std::vector<std::string>& tokens,
                    std::initializer_list<std::string_view> expected)
{
    return std::any_of(tokens.begin(), tokens.end(), [&](const std::string& token) {
        return std::any_of(expected.begin(), expected.end(), [&](std::string_view item) {
            return token == item;
        });
    });
}

bool contains_phrase(const std::string& normalized_text, std::string_view phrase)
{
    if (normalized_text.empty()) {
        return false;
    }

    const std::string phrase_text(phrase);
    const auto pos = normalized_text.find(phrase_text);
    if (pos == std::string::npos) {
        return false;
    }

    const bool left_ok = pos == 0 || normalized_text[pos - 1] == ' ';
    const std::size_t end = pos + phrase_text.size();
    const bool right_ok = end == normalized_text.size() || normalized_text[end] == ' ';
    return left_ok && right_ok;
}

bool matches_exact_alias(const std::string& normalized_text,
                         std::initializer_list<std::string_view> aliases)
{
    return std::any_of(aliases.begin(), aliases.end(), [&](std::string_view alias) {
        return normalized_text == alias;
    });
}

bool is_other_label(const std::string& normalized_text)
{
    return matches_exact_alias(normalized_text, {"other", "others", "misc", "miscellaneous", "uncategorized"});
}

bool label_has_driver_signal(const std::string& normalized_text);
bool label_has_installer_signal(const std::string& normalized_text);
bool label_has_operating_system_signal(const std::string& normalized_text);
bool label_has_data_export_signal(const std::string& normalized_text);

bool is_artifact_main_alias(const std::string& normalized_text)
{
    if (matches_exact_alias(
            normalized_text,
            {"software", "installers", "installer", "drivers", "driver", "operating system",
             "operating systems", "archives", "archive", "data export", "data exports", "other"})) {
        return true;
    }

    return label_has_installer_signal(normalized_text) ||
           label_has_driver_signal(normalized_text) ||
           label_has_operating_system_signal(normalized_text) ||
           label_has_data_export_signal(normalized_text);
}

bool is_low_information_artifact_label(const std::string& normalized_text)
{
    return normalized_text.empty() ||
           is_other_label(normalized_text) ||
           matches_exact_alias(
               normalized_text,
               {"general", "file", "files", "software", "application", "applications", "app", "apps",
                "program", "programs", "tool", "tools", "utility", "utilities", "installers", "installer",
                "drivers", "driver", "operating system", "operating systems", "archives", "archive",
                "data export", "data exports"});
}

bool label_has_driver_signal(const std::string& normalized_text)
{
    const auto tokens = split_tokens(normalized_text);
    return contains_token(tokens, {"driver", "drivers"});
}

bool label_has_installer_signal(const std::string& normalized_text)
{
    if (contains_phrase(normalized_text, "setup file") ||
        contains_phrase(normalized_text, "setup files") ||
        contains_phrase(normalized_text, "installation file") ||
        contains_phrase(normalized_text, "installation files")) {
        return true;
    }

    const auto tokens = split_tokens(normalized_text);
    return contains_token(tokens, {"setup", "installer", "installers", "installation", "installations"});
}

bool label_has_operating_system_signal(const std::string& normalized_text)
{
    if (contains_phrase(normalized_text, "operating system") ||
        contains_phrase(normalized_text, "operating systems")) {
        return true;
    }

    const auto tokens = split_tokens(normalized_text);
    return contains_token(tokens, {"windows", "macos", "osx", "linux", "ubuntu", "debian", "fedora"});
}

bool label_has_software_signal(const std::string& normalized_text)
{
    const auto tokens = split_tokens(normalized_text);
    return contains_token(tokens,
                          {"software", "application", "applications", "app", "apps", "program", "programs",
                           "tool", "tools", "utility", "utilities", "client", "server", "viewer",
                           "editor", "browser", "player", "plugin", "plugins", "suite", "suites",
                           "runtime", "sdk", "portable", "monitoring", "monitor", "workstation"});
}

bool label_has_data_export_signal(const std::string& normalized_text)
{
    if (contains_phrase(normalized_text, "data export") ||
        contains_phrase(normalized_text, "data exports")) {
        return true;
    }

    const auto tokens = split_tokens(normalized_text);
    return contains_token(tokens, {"export", "exports", "dump", "dumps", "dataset", "datasets", "csv", "tsv"});
}

bool file_name_has_driver_signal(const std::string& normalized_file_name)
{
    const auto tokens = split_tokens(normalized_file_name);
    return contains_token(tokens, {"driver", "drivers", "whql", "dch"});
}

bool file_name_has_installer_signal(const std::string& normalized_file_name)
{
    const auto tokens = split_tokens(normalized_file_name);
    return contains_token(tokens, {"setup", "install", "installer", "portable"});
}

bool file_name_has_operating_system_signal(const std::string& normalized_file_name)
{
    const auto tokens = split_tokens(normalized_file_name);
    return contains_token(tokens, {"windows", "macos", "osx", "linux", "ubuntu", "debian", "fedora"});
}

bool file_name_has_software_signal(const std::string& normalized_file_name)
{
    const auto tokens = split_tokens(normalized_file_name);
    return contains_token(tokens,
                          {"app", "apps", "tool", "tools", "utility", "utilities", "portable",
                           "client", "viewer", "editor", "browser", "player", "plugin",
                           "plugins", "sdk", "runtime", "monitor", "workstation"});
}

bool file_name_has_data_export_signal(const std::string& normalized_file_name)
{
    const auto tokens = split_tokens(normalized_file_name);
    return contains_token(tokens, {"export", "exports", "dump", "dumps", "csv", "tsv"});
}

ArtifactFamily artifact_family_for_file_name(const std::string& file_name)
{
    const auto selection = FileCategoryPolicy::determine_main_category_selection(file_name, FileType::File);
    if (selection.family_name == "software") {
        return ArtifactFamily::Software;
    }
    if (selection.family_name == "archive") {
        return ArtifactFamily::Archive;
    }
    return ArtifactFamily::None;
}

std::string choose_software_main_category(const std::string& normalized_file_name,
                                          const std::string& normalized_category,
                                          const std::string& normalized_subcategory)
{
    if (file_name_has_driver_signal(normalized_file_name) ||
        label_has_driver_signal(normalized_category) ||
        label_has_driver_signal(normalized_subcategory)) {
        return "Drivers";
    }

    if (label_has_installer_signal(normalized_category) ||
        label_has_installer_signal(normalized_subcategory) ||
        file_name_has_installer_signal(normalized_file_name)) {
        return "Installers";
    }

    if (is_other_label(normalized_category)) {
        return "Other";
    }

    if (label_has_operating_system_signal(normalized_subcategory)) {
        return "Operating Systems";
    }

    if (label_has_operating_system_signal(normalized_category) &&
        (normalized_subcategory.empty() ||
         is_low_information_artifact_label(normalized_subcategory) ||
         file_name_has_operating_system_signal(normalized_file_name))) {
        return "Operating Systems";
    }

    return "Software";
}

std::string choose_archive_main_category(const std::string& normalized_file_name,
                                         const std::string& normalized_category,
                                         const std::string& normalized_subcategory)
{
    const bool software_signal =
        file_name_has_software_signal(normalized_file_name) ||
        file_name_has_driver_signal(normalized_file_name) ||
        file_name_has_installer_signal(normalized_file_name) ||
        label_has_software_signal(normalized_category) ||
        label_has_software_signal(normalized_subcategory) ||
        label_has_driver_signal(normalized_category) ||
        label_has_driver_signal(normalized_subcategory) ||
        label_has_installer_signal(normalized_category) ||
        label_has_installer_signal(normalized_subcategory) ||
        label_has_operating_system_signal(normalized_category) ||
        label_has_operating_system_signal(normalized_subcategory);
    if (software_signal) {
        return "Software";
    }

    const bool export_signal =
        file_name_has_data_export_signal(normalized_file_name) ||
        label_has_data_export_signal(normalized_category) ||
        label_has_data_export_signal(normalized_subcategory);
    if (export_signal) {
        return "Data Exports";
    }

    if (is_other_label(normalized_category)) {
        return "Other";
    }

    return "Archives";
}

std::string choose_artifact_subcategory(const std::string& stable_main_category,
                                        const std::string& category,
                                        const std::string& subcategory)
{
    const std::string normalized_main = normalize_match_text(stable_main_category);
    const auto normalize_candidate = [&](const std::string& value, bool allow_main_aliases) {
        const std::string sanitized = Utils::sanitize_path_label(trim_copy(value));
        if (sanitized.empty()) {
            return std::string();
        }

        const std::string normalized_candidate = normalize_match_text(sanitized);
        if (normalized_candidate.empty() || normalized_candidate == normalized_main) {
            return std::string();
        }
        if (is_low_information_artifact_label(normalized_candidate)) {
            return std::string();
        }
        if (!allow_main_aliases && is_artifact_main_alias(normalized_candidate)) {
            return std::string();
        }
        return sanitized;
    };

    if (const std::string candidate = normalize_candidate(category, false); !candidate.empty()) {
        return candidate;
    }
    if (const std::string candidate = normalize_candidate(subcategory, false); !candidate.empty()) {
        return candidate;
    }
    if (const std::string candidate = normalize_candidate(subcategory, true); !candidate.empty()) {
        return candidate;
    }
    if (const std::string candidate = normalize_candidate(category, true); !candidate.empty()) {
        return candidate;
    }
    return "General";
}

} // namespace

namespace ArtifactCategoryPolicy {

bool is_supported_artifact_file_name(const std::string& file_name)
{
    return artifact_family_for_file_name(file_name) != ArtifactFamily::None;
}

std::optional<NormalizedCategoryLabels> normalize_category_labels(const std::string& file_name,
                                                                  const std::string& category,
                                                                  const std::string& subcategory)
{
    const ArtifactFamily family = artifact_family_for_file_name(file_name);
    if (family == ArtifactFamily::None) {
        return std::nullopt;
    }

    const std::string normalized_file_name = normalize_match_text(file_name);
    const std::string normalized_category = normalize_match_text(category);
    const std::string normalized_subcategory = normalize_match_text(subcategory);

    std::string stable_main_category;
    if (family == ArtifactFamily::Software) {
        stable_main_category = choose_software_main_category(normalized_file_name,
                                                             normalized_category,
                                                             normalized_subcategory);
    } else {
        stable_main_category = choose_archive_main_category(normalized_file_name,
                                                            normalized_category,
                                                            normalized_subcategory);
    }

    return NormalizedCategoryLabels{
        stable_main_category,
        choose_artifact_subcategory(stable_main_category, category, subcategory)
    };
}

} // namespace ArtifactCategoryPolicy

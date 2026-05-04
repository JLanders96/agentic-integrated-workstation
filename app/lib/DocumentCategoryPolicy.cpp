#include "DocumentCategoryPolicy.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

namespace {

std::string to_lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string extract_extension_lower(const std::string& file_name)
{
    const auto newline = file_name.find('\n');
    const std::string base = file_name.substr(0, newline);
    const auto dot = base.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= base.size()) {
        return {};
    }
    return to_lower_copy(base.substr(dot));
}

const std::unordered_set<std::string> kCategorizedDocumentExtensions = {
    ".txt", ".md", ".markdown", ".rtf", ".csv", ".tsv", ".log", ".json", ".xml", ".yml", ".yaml",
    ".ini", ".cfg", ".conf", ".html", ".htm", ".tex", ".rst", ".pdf", ".docx", ".xlsx", ".pptx",
    ".odt", ".ods", ".odp", ".doc", ".xls", ".ppt"
};

const std::unordered_set<std::string> kPresentationExtensions = {
    ".pptx", ".odp", ".ppt"
};

const std::unordered_set<std::string> kSpreadsheetExtensions = {
    ".xlsx", ".ods", ".xls"
};

const std::unordered_set<std::string> kDataExportExtensions = {
    ".csv", ".tsv"
};

const std::unordered_set<std::string> kConfigExtensions = {
    ".ini", ".cfg", ".conf"
};

} // namespace

namespace DocumentCategoryPolicy {

bool is_supported_document_file_name(const std::string& file_name)
{
    const std::string extension = extract_extension_lower(file_name);
    return !extension.empty() && kCategorizedDocumentExtensions.contains(extension);
}

std::optional<std::string> preferred_main_category_for_file_name(const std::string& file_name)
{
    const std::string extension = extract_extension_lower(file_name);
    if (extension.empty() || !kCategorizedDocumentExtensions.contains(extension)) {
        return std::nullopt;
    }
    if (kPresentationExtensions.contains(extension)) {
        return std::string("Presentations");
    }
    if (kSpreadsheetExtensions.contains(extension)) {
        return std::string("Spreadsheets");
    }
    if (kDataExportExtensions.contains(extension)) {
        return std::string("Data Exports");
    }
    if (kConfigExtensions.contains(extension)) {
        return std::string("Configs");
    }
    return std::string("Documents");
}

} // namespace DocumentCategoryPolicy

#include "ImageCategoryPolicy.hpp"

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

const std::unordered_set<std::string> kCategorizedImageExtensions = {
    ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp", ".tif", ".tiff",
    ".tga", ".psd", ".hdr", ".pic", ".pnm", ".ppm", ".pgm", ".pbm",
    ".heic", ".heif", ".avif", ".ico", ".svg"
};

} // namespace

namespace ImageCategoryPolicy {

bool is_supported_image_file_name(const std::string& file_name)
{
    const std::string extension = extract_extension_lower(file_name);
    return !extension.empty() && kCategorizedImageExtensions.contains(extension);
}

std::optional<std::string> preferred_main_category_for_file_name(const std::string& file_name)
{
    const std::string extension = extract_extension_lower(file_name);
    if (extension.empty() || !kCategorizedImageExtensions.contains(extension)) {
        return std::nullopt;
    }
    return std::string("Images");
}

} // namespace ImageCategoryPolicy

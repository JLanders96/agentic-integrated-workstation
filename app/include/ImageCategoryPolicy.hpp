#pragma once

#include <optional>
#include <string>

namespace ImageCategoryPolicy {

/**
 * @brief Returns whether the file name should use image categorization rules.
 * @param file_name File name or path fragment to inspect.
 * @return True when the extension matches the image-like set used for
 * categorization.
 */
bool is_supported_image_file_name(const std::string& file_name);

/**
 * @brief Returns the preferred stable main category for a categorized image file.
 * @param file_name File name or path fragment to inspect.
 * @return Preferred main category, or std::nullopt when the file should not use
 * image categorization rules.
 */
std::optional<std::string> preferred_main_category_for_file_name(const std::string& file_name);

} // namespace ImageCategoryPolicy

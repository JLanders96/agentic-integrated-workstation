#pragma once

#include <optional>
#include <string>

namespace DocumentCategoryPolicy {

/**
 * @brief Returns whether the file name should use document categorization rules.
 * @param file_name File name or path fragment to inspect.
 * @return True when the extension matches the document-like set used for
 * categorization, including legacy Office binaries that are not content-analyzable.
 */
bool is_supported_document_file_name(const std::string& file_name);

/**
 * @brief Returns the preferred stable main category for a categorized document file.
 * @param file_name File name or path fragment to inspect.
 * @return Preferred main category, or std::nullopt when the file should not use
 * document categorization rules.
 */
std::optional<std::string> preferred_main_category_for_file_name(const std::string& file_name);

} // namespace DocumentCategoryPolicy

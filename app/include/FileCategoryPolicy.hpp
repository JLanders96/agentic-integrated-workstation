#pragma once

#include "Types.hpp"

#include <string>
#include <vector>

namespace FileCategoryPolicy {

/**
 * @brief Prompt-time main-category selection guidance for a coarse file family.
 */
struct MainCategorySelection {
    /**
     * @brief Descriptive family label used for logging and tests.
     */
    std::string family_name;
    /**
     * @brief Ordered main-category candidates for the current file family.
     */
    std::vector<std::string> categories;
};

/**
 * @brief Determines a bounded list of main-category candidates for an item.
 * @param file_name Prompt-facing file name or path fragment to inspect.
 * @param file_type File or directory type being categorized.
 * @return Candidate main categories ordered from most likely to broadest fallback.
 */
MainCategorySelection determine_main_category_selection(const std::string& file_name,
                                                        FileType file_type);

} // namespace FileCategoryPolicy

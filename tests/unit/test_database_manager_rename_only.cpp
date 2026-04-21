#include <catch2/catch_test_macros.hpp>

#include "DatabaseManager.hpp"
#include "TestHelpers.hpp"

TEST_CASE("DatabaseManager keeps rename-only entries with empty labels") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    const std::string dir_path = "/sample";
    DatabaseManager::ResolvedCategory empty{0, "", ""};
    const std::string suggested_name = "rename_suggestion.png";

    REQUIRE(db.insert_or_update_file_with_categorization(
        "rename.png", "F", dir_path, empty, false, suggested_name, true));
    REQUIRE(db.insert_or_update_file_with_categorization(
        "empty.png", "F", dir_path, empty, false, std::string(), false));

    const auto removed = db.remove_empty_categorizations(dir_path);
    REQUIRE(removed.size() == 1);
    CHECK(removed.front().file_name == "empty.png");

    const auto entries = db.get_categorized_files(dir_path);
    REQUIRE(entries.size() == 1);
    CHECK(entries.front().file_name == "rename.png");
    CHECK(entries.front().rename_only);
    CHECK_FALSE(entries.front().rename_applied);
    CHECK(entries.front().suggested_name == suggested_name);
    CHECK(entries.front().category.empty());
    CHECK(entries.front().subcategory.empty());
}

TEST_CASE("DatabaseManager keeps suggestion-only entries with empty labels") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    const std::string dir_path = "/sample";
    DatabaseManager::ResolvedCategory empty{0, "", ""};
    const std::string suggested_name = "suggested_name.png";

    REQUIRE(db.insert_or_update_file_with_categorization(
        "suggested.png", "F", dir_path, empty, false, suggested_name, false));

    const auto removed = db.remove_empty_categorizations(dir_path);
    CHECK(removed.empty());

    const auto entries = db.get_categorized_files(dir_path);
    REQUIRE(entries.size() == 1);
    CHECK(entries.front().file_name == "suggested.png");
    CHECK_FALSE(entries.front().rename_only);
    CHECK(entries.front().suggested_name == suggested_name);
    CHECK(entries.front().category.empty());
    CHECK(entries.front().subcategory.empty());
}

TEST_CASE("DatabaseManager sanitizes invalid UTF-8 in cached labels") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    const std::string dir_path = "/sample";
    std::string invalid_category = "Imag";
    invalid_category.push_back(static_cast<char>(0xFF));
    invalid_category += "es";
    std::string invalid_subcategory = "Phot";
    invalid_subcategory.push_back(static_cast<char>(0xFF));
    invalid_subcategory += "os";
    std::string invalid_suggested = "sum";
    invalid_suggested.push_back(static_cast<char>(0xFF));
    invalid_suggested += "mer.png";

    DatabaseManager::ResolvedCategory resolved{
        0,
        invalid_category,
        invalid_subcategory,
    };

    REQUIRE(db.insert_or_update_file_with_categorization(
        "photo.png", "F", dir_path, resolved, false, invalid_suggested, false));

    const auto entries = db.get_categorized_files(dir_path);
    REQUIRE(entries.size() == 1);
    CHECK(entries.front().category == "Images");
    CHECK(entries.front().subcategory == "Photos");
    CHECK(entries.front().suggested_name == "summer.png");
}

TEST_CASE("DatabaseManager normalizes subcategory stopword suffixes for taxonomy matching") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto base = db.resolve_category("Images", "Graphics");
    auto with_suffix = db.resolve_category("Images", "Graphics files");

    REQUIRE(base.taxonomy_id > 0);
    CHECK(with_suffix.taxonomy_id == base.taxonomy_id);
    CHECK(with_suffix.category == base.category);
    CHECK(with_suffix.subcategory == base.subcategory);

    auto photos = db.resolve_category("Images", "Photos");
    CHECK(photos.subcategory == "Photos");
}

TEST_CASE("DatabaseManager preserves the Backups family under archive-like labels") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto archives_backups = db.resolve_category("Archives", "Backups");
    auto backup = db.resolve_category("backup files", "General");

    REQUIRE(archives_backups.taxonomy_id > 0);
    CHECK(backup.taxonomy_id == archives_backups.taxonomy_id);
    CHECK(backup.category == "Backups");
    CHECK(backup.subcategory == "General");
    CHECK(archives_backups.category == "Backups");
    CHECK(archives_backups.subcategory == "General");
}

TEST_CASE("DatabaseManager normalizes image category synonyms and image media aliases") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto images = db.resolve_category("Images", "Photos");
    auto graphics = db.resolve_category("Graphics", "Photos");
    auto media_images = db.resolve_category("Media", "Photos");
    auto media_audio = db.resolve_category("Media", "Audio");

    REQUIRE(images.taxonomy_id > 0);
    CHECK(graphics.taxonomy_id == images.taxonomy_id);
    CHECK(media_images.taxonomy_id == images.taxonomy_id);
    CHECK(graphics.category == "Images");
    CHECK(media_images.category == "Images");

    CHECK(media_audio.category == "Media");
    CHECK(media_audio.taxonomy_id != images.taxonomy_id);
}

TEST_CASE("DatabaseManager normalizes document category synonyms for taxonomy matching") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto documents = db.resolve_category("Documents", "Research");
    auto texts = db.resolve_category("Texts", "Research");
    auto papers = db.resolve_category("Papers", "Research");

    REQUIRE(documents.taxonomy_id > 0);
    CHECK(texts.taxonomy_id == documents.taxonomy_id);
    CHECK(papers.taxonomy_id == documents.taxonomy_id);
    CHECK(documents.category == "Documents");
    CHECK(texts.category == "Documents");
    CHECK(papers.category == "Documents");
}

TEST_CASE("DatabaseManager normalizes generic Documents labels into preserved document families when the subcategory is explicit") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto documents_manuals = db.resolve_category("Documents", "Manuals");
    auto manuals_general = db.resolve_category("Manuals", "General");
    auto documents_spreadsheets = db.resolve_category("Documents", "Spreadsheets");
    auto spreadsheets_general = db.resolve_category("Spreadsheets", "General");

    REQUIRE(documents_manuals.taxonomy_id > 0);
    CHECK(documents_manuals.taxonomy_id == manuals_general.taxonomy_id);
    CHECK(documents_manuals.category == "Manuals");
    CHECK(documents_manuals.subcategory == "General");

    REQUIRE(documents_spreadsheets.taxonomy_id > 0);
    CHECK(documents_spreadsheets.taxonomy_id == spreadsheets_general.taxonomy_id);
    CHECK(documents_spreadsheets.category == "Spreadsheets");
    CHECK(documents_spreadsheets.subcategory == "General");
}

TEST_CASE("DatabaseManager keeps specialized document-family categories and normalizes generic subcategories") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto documents_manuals = db.resolve_category("Documents", "Manuals");
    auto manuals_general = db.resolve_category("Manuals", "General");
    auto manuals_empty = db.resolve_category("Manuals", "");
    auto guides_general = db.resolve_category("Guides", "General");

    REQUIRE(documents_manuals.taxonomy_id > 0);
    CHECK(manuals_general.taxonomy_id == documents_manuals.taxonomy_id);
    CHECK(manuals_empty.taxonomy_id == documents_manuals.taxonomy_id);
    CHECK(manuals_general.category == "Manuals");
    CHECK(manuals_empty.category == "Manuals");
    CHECK(manuals_general.subcategory == "General");
    CHECK(manuals_empty.subcategory == "General");

    CHECK(guides_general.category == "Guides");
    CHECK(guides_general.subcategory == "General");
    CHECK(guides_general.taxonomy_id != documents_manuals.taxonomy_id);
}

TEST_CASE("DatabaseManager preserves the Installers family under software-like labels") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto software = db.resolve_category("Software", "Installers");
    auto installers = db.resolve_category("Installers", "Installers");
    auto setup_files = db.resolve_category("Setup files", "Installers");

    REQUIRE(software.taxonomy_id > 0);
    CHECK(installers.taxonomy_id == software.taxonomy_id);
    CHECK(setup_files.taxonomy_id == software.taxonomy_id);
    CHECK(installers.category == "Installers");
    CHECK(setup_files.category == "Installers");
    CHECK(software.category == "Installers");
    CHECK(software.subcategory == "General");
}

TEST_CASE("DatabaseManager keeps non-family software semantics under Software") {
    TempDir base_dir;
    EnvVarGuard config_guard("AI_FILE_SORTER_CONFIG_DIR", base_dir.path().string());
    DatabaseManager db(base_dir.path().string());

    auto updates = db.resolve_category("Software Update", "General");
    auto patches = db.resolve_category("Patches", "General");

    REQUIRE(updates.taxonomy_id > 0);
    CHECK(updates.category == "Software");
    CHECK(patches.category == "Software");
    CHECK(patches.taxonomy_id == updates.taxonomy_id);
}

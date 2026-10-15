#include <doctest/doctest.h>
#include <luxor/resources/storage/vfs.hh>
#include <luxor/resources/storage/archive.hh>

#include <filesystem>
#include <fstream>
#include <sstream>

TEST_CASE("vfs core functionality") {
    luxor::rc::vfs fs;
    CHECK(fs.empty());
    CHECK(fs.mount_count() == 0);

    const auto temp_base = std::filesystem::temp_directory_path() / "luxor_vfs_base_test";
    std::filesystem::remove_all(temp_base);
    std::filesystem::create_directories(temp_base / "data");
    const std::string sample_animdefs_content(15614, 'A');
    {
        std::ofstream ofs(temp_base / "data" / "animdefs.txt");
        ofs << sample_animdefs_content;
    }

    const auto temp_quest = std::filesystem::temp_directory_path() / "luxor_vfs_quest_test";
    std::filesystem::remove_all(temp_quest);
    std::filesystem::create_directories(temp_quest);
    {
        std::ofstream ofs(temp_quest / "animdefs.txt");
        ofs << std::string(19859, 'B');
    }

    // 1. Mount base directory at root ""
    fs.mount("", temp_base);
    CHECK_FALSE(fs.empty());
    CHECK(fs.mount_count() == 1);

    // Case-insensitivity & slash normalization checks
    CHECK(fs.exists("data/animdefs.txt"));
    CHECK(fs.exists("DATA/AnimDefs.TXT"));
    CHECK(fs.exists("data\\AnimDefs.txt"));
    CHECK(fs.exists("/data/animdefs.txt"));
    CHECK(fs.exists("data/./animdefs.txt"));
    CHECK(fs.exists("data/../data/animdefs.txt"));

    auto st1 = fs.stat("DATA/animdefs.txt");
    REQUIRE(st1.has_value());
    CHECK(st1->path == "data/animdefs.txt");
    CHECK(st1->size == 15614);
    CHECK_FALSE(st1->is_directory);

    auto data1 = fs.read("data\\ANIMDEFS.txt");
    CHECK(data1.size() == 15614);

    auto stream1 = fs.open("DATA/animdefs.txt");
    REQUIRE(stream1 != nullptr);
    std::string first_line;
    std::getline(*stream1, first_line);
    CHECK_FALSE(first_line.empty());

    // 2. Mount second directory at sub-path "quest"
    fs.mount("quest", temp_quest);
    CHECK(fs.mount_count() == 2);
    CHECK(fs.exists("quest"));
    auto st_qdir = fs.stat("quest");
    REQUIRE(st_qdir.has_value());
    CHECK(st_qdir->is_directory);

    CHECK(fs.exists("quest/animdefs.txt"));
    CHECK(fs.exists("QUEST\\ANIMDEFS.TXT"));
    auto st2 = fs.stat("quest/animdefs.txt");
    REQUIRE(st2.has_value());
    CHECK(st2->size == 19859);
    CHECK(fs.read("QUEST/AnimDefs.txt").size() == 19859);

    // 3. Mount physical directory with mixed case subdirectories
    const std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "luxor_vfs_test_dir";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir / "SubDir");
    {
        std::ofstream ofs(temp_dir / "SubDir" / "MixedFile.TXT");
        ofs << "Hello from physical directory!";
    }
    fs.mount("dir", temp_dir);
    CHECK(fs.exists("dir/subdir/mixedfile.txt"));
    CHECK(fs.exists("DIR/SUBDIR/MIXEDFILE.TXT"));
    CHECK(fs.exists("dir\\SubDir\\MixedFile.txt"));

    auto data_dir = fs.read("DIR/SubDir/MixedFile.txt");
    std::string text_read(data_dir.begin(), data_dir.end());
    CHECK(text_read == "Hello from physical directory!");

    // 4. Overlay precedence (LIFO) & unmount
    const std::filesystem::path patch_dir = std::filesystem::temp_directory_path() / "luxor_vfs_patch_dir";
    std::filesystem::remove_all(patch_dir);
    std::filesystem::create_directories(patch_dir / "data");
    {
        std::ofstream ofs(patch_dir / "data" / "animdefs.txt");
        ofs << "Overridden animdefs content!";
    }
    fs.mount("", patch_dir);
    auto data_patched = fs.read("data/animdefs.txt");
    std::string text_patched(data_patched.begin(), data_patched.end());
    CHECK(text_patched == "Overridden animdefs content!");

    // Unmount patch directory -> original from nhp restored
    CHECK(fs.unmount(""));
    auto data_restored = fs.read("data/animdefs.txt");
    CHECK(data_restored.size() == 15614);

    // 5. Test list_files()
    auto all_files = fs.list_files();
    CHECK_FALSE(all_files.empty());

    // 6. Test Dump
    const std::filesystem::path dump_dir = std::filesystem::temp_directory_path() / "luxor_vfs_dump_test";
    std::filesystem::remove_all(dump_dir);
    luxor::rc::vfs small_vfs;
    small_vfs.mount("test_quest", temp_quest);
    small_vfs.mount("test_dir", temp_dir);
    luxor::rc::dump(small_vfs, dump_dir);

    CHECK(std::filesystem::is_regular_file(dump_dir / "test_dir" / "subdir" / "mixedfile.txt"));
    CHECK(std::filesystem::is_regular_file(dump_dir / "test_quest" / "animdefs.txt"));
    CHECK(std::filesystem::file_size(dump_dir / "test_quest" / "animdefs.txt") == 19859);
    std::filesystem::remove_all(dump_dir);

    // 7. Negative checks
    CHECK_FALSE(fs.exists("non_existent_file.xyz"));
    CHECK_FALSE(fs.stat("non_existent_file.xyz").has_value());
    CHECK(fs.read("non_existent_file.xyz").empty());
    CHECK(fs.open("non_existent_file.xyz") == nullptr);

    CHECK_THROWS_AS(fs.mount("", "/non/existent/path/for/sure/9999"), std::exception);

    // Clean up temporary test directories
    std::filesystem::remove_all(temp_base);
    std::filesystem::remove_all(temp_quest);
    std::filesystem::remove_all(temp_dir);
    std::filesystem::remove_all(patch_dir);
}

TEST_CASE("vfs locale fallback") {
    luxor::rc::vfs vfs;
    CHECK(vfs.locale().empty());
    vfs.set_locale("  English/ ");
    CHECK(vfs.locale() == "english");

    const auto temp_locale = std::filesystem::temp_directory_path() / "luxor_vfs_locale_mock";
    std::filesystem::remove_all(temp_locale);
    std::filesystem::create_directories(temp_locale / "data");
    std::filesystem::create_directories(temp_locale / "english" / "data");

    {
        std::ofstream(temp_locale / "data" / "base_only.txt") << "base only content";
        std::ofstream(temp_locale / "english" / "strings.utf8") << "Hello localized world!";
        std::ofstream(temp_locale / "english" / "data" / "localized.txt") << "Localized data!";
    }

    vfs.mount("", temp_locale);

    // 1. Check localized strings.utf8 is accessible directly
    CHECK(vfs.exists("strings.utf8"));
    CHECK(vfs.exists("STRINGS.UTF8"));
    auto st_str = vfs.stat("strings.utf8");
    REQUIRE(st_str.has_value());
    CHECK(st_str->path == "strings.utf8");
    CHECK_FALSE(st_str->is_directory);
    CHECK(st_str->size > 0);

    auto data_str = vfs.read("strings.utf8");
    CHECK_FALSE(data_str.empty());
    CHECK(data_str.size() == st_str->size);

    auto stream_str = vfs.open("strings.utf8");
    CHECK(stream_str != nullptr);

    // Explicit access with english/ prefix still succeeds
    CHECK(vfs.exists("english/strings.utf8"));
    CHECK(vfs.read("english/strings.utf8").size() == data_str.size());

    // 2. Base files that are not localized are still accessible
    CHECK(vfs.exists("data/base_only.txt"));
    CHECK_FALSE(vfs.read("data/base_only.txt").empty());

    // 3. Localized file inside subfolder
    auto st_localized = vfs.stat("data/localized.txt");
    REQUIRE(st_localized.has_value());
    CHECK(st_localized->path == "data/localized.txt");
    CHECK_FALSE(vfs.read("data/localized.txt").empty());

    // 4. Test list_files() mapping
    auto files = vfs.list_files();
    bool found_mapped_strings = false;
    bool found_raw_english_strings = false;
    bool found_raw_english_localized = false;
    for (const auto& f : files) {
        if (f.path == "strings.utf8") found_mapped_strings = true;
        if (f.path == "english/strings.utf8") found_raw_english_strings = true;
        if (f.path == "english/data/localized.txt") found_raw_english_localized = true;
    }
    CHECK(found_mapped_strings);
    CHECK_FALSE(found_raw_english_strings);
    CHECK_FALSE(found_raw_english_localized);

    // 5. Test turning off locale
    vfs.set_locale("");
    CHECK(vfs.locale().empty());
    CHECK_FALSE(vfs.exists("strings.utf8"));
    CHECK(vfs.exists("english/strings.utf8"));

    // 6. Test VFS dump with locale active
    vfs.set_locale("english");
    const std::filesystem::path dump_test_dir = std::filesystem::temp_directory_path() / "luxor_vfs_locale_dump_test";
    std::filesystem::remove_all(dump_test_dir);
    luxor::rc::dump(vfs, dump_test_dir);

    CHECK(std::filesystem::is_regular_file(dump_test_dir / "strings.utf8"));
    CHECK(std::filesystem::file_size(dump_test_dir / "strings.utf8") == st_str->size);
    CHECK(std::filesystem::is_regular_file(dump_test_dir / "data" / "localized.txt"));
    CHECK_FALSE(std::filesystem::exists(dump_test_dir / "english"));

    std::filesystem::remove_all(dump_test_dir);
    std::filesystem::remove_all(temp_locale);
}

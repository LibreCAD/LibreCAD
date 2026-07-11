#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct QtReference {
    std::filesystem::path file;
    std::size_t lineNumber = 0;
    std::string token;
    std::string line;
};

bool isIdentifierChar(unsigned char c) {
    return std::isalnum(c) != 0 || c == '_';
}

bool containsWord(const std::string& line, const std::string& word) {
    std::string::size_type pos = 0;
    while ((pos = line.find(word, pos)) != std::string::npos) {
        const bool leftBoundary =
            pos == 0 || !isIdentifierChar(static_cast<unsigned char>(line[pos - 1]));
        const std::string::size_type right = pos + word.size();
        const bool rightBoundary =
            right == line.size()
            || !isIdentifierChar(static_cast<unsigned char>(line[right]));

        if (leftBoundary && rightBoundary)
            return true;

        ++pos;
    }

    return false;
}

std::string withoutLeadingSpace(std::string line) {
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char c) {
        return std::isspace(c) == 0;
    }));
    return line;
}

bool containsQtInclude(const std::string& line) {
    std::string trimmed = withoutLeadingSpace(line);
    if (trimmed.rfind("#", 0) != 0)
        return false;

    trimmed.erase(trimmed.begin());
    trimmed = withoutLeadingSpace(trimmed);
    if (trimmed.rfind("include", 0) != 0)
        return false;

    trimmed.erase(trimmed.begin(), trimmed.begin() + 7);
    trimmed = withoutLeadingSpace(trimmed);
    return trimmed.rfind("<Q", 0) == 0 || trimmed.rfind("\"Q", 0) == 0;
}

std::vector<QtReference> scanForQtReferences(const std::filesystem::path& root) {
    std::vector<QtReference> references;
    const std::vector<std::string> forbiddenWords = {"QString", "QtCore", "qDebug"};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file())
            continue;

        std::ifstream input(entry.path());
        REQUIRE(input.is_open());

        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line)) {
            ++lineNumber;

            if (containsQtInclude(line)) {
                references.push_back({entry.path(), lineNumber, "#include <Q...>", line});
            }

            for (const std::string& word : forbiddenWords) {
                if (containsWord(line, word))
                    references.push_back({entry.path(), lineNumber, word, line});
            }
        }
    }

    return references;
}

std::string formatReferences(const std::vector<QtReference>& references,
                             const std::filesystem::path& sourceRoot) {
    std::ostringstream out;
    for (const QtReference& reference : references) {
        out << std::filesystem::relative(reference.file, sourceRoot).generic_string()
            << ':' << reference.lineNumber << ": " << reference.token << ": "
            << reference.line << '\n';
    }
    return out.str();
}

} // namespace

TEST_CASE("libdxfrw src remains Qt-free", "[libdxfrw][qt-free]") {
    const std::filesystem::path sourceRoot = LIBRECAD_SOURCE_DIR;
    const std::filesystem::path libdxfrwSrc =
        sourceRoot / "libraries" / "libdxfrw" / "src";

    REQUIRE(std::filesystem::is_directory(libdxfrwSrc));

    const std::vector<QtReference> references = scanForQtReferences(libdxfrwSrc);
    INFO(formatReferences(references, sourceRoot));
    CHECK(references.empty());
}

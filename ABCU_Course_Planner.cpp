//==================================================================
// Name:       Fitzbert Anthony Patterson
// Course:     CS-300 - DSA:  Analysis and Design
// Section:    14381 - M01
// Assignment: Project Two_ABCU Curse Planner
// Professor:  Andujar Dewin
// Date:       OCT17, 2026
// 
// Data structure: Binary Search Tree (BST), keyed by course number.
// Compile with a C++17-compatible compiler (Eclipse CDT / g++ / Visual Studio).

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

struct Course {
    std::string courseNumber;
    std::string title;
    std::vector<std::string> prerequisites;
};

// Removes whitespace from the beginning and end of a field.
std::string trim(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

// Normalize course numbers for case-insensitive lookups and sorting.
std::string upperCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return text;
}

// Parse a CSV record, including optional quoted fields and trailing empty fields.
// Returns false when quotation marks are malformed.
bool parseCsvLine(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool quoted = false;
    bool justClosedQuote = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    justClosedQuote = true;
                }
            } else {
                field += ch;
            }
        } else if (ch == ',') {
            fields.push_back(trim(field));
            field.clear();
            justClosedQuote = false;
        } else if (ch == '"') {
            if (!trim(field).empty() || justClosedQuote) return false;
            field.clear();
            quoted = true;
        } else {
            if (justClosedQuote && !std::isspace(static_cast<unsigned char>(ch))) return false;
            field += ch;
        }
    }
    if (quoted) return false;
    fields.push_back(trim(field));
    return true;
}

class BinarySearchTree {
private:
    struct Node {
        explicit Node(Course value) : course(std::move(value)) {}
        Course course;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    std::unique_ptr<Node> root;
    std::size_t count = 0;

    // Insert recursively, branching by the course number.
    static bool insertNode(std::unique_ptr<Node>& node, const Course& course) {
        if (!node) {
            node = std::make_unique<Node>(course);
            return true;
        }
        if (course.courseNumber < node->course.courseNumber)
            return insertNode(node->left, course);
        if (course.courseNumber > node->course.courseNumber)
            return insertNode(node->right, course);
        return false;  // Reject duplicate course numbers.
    }

    // Visit left subtree, current course, then right subtree for sorted output.
    static void printInOrder(const Node* node) {
        if (!node) return;
        printInOrder(node->left.get());
        std::cout << node->course.courseNumber << ", " << node->course.title << '\n';
        printInOrder(node->right.get());
    }

public:
    BinarySearchTree() = default;
    BinarySearchTree(BinarySearchTree&&) = default;
    BinarySearchTree& operator=(BinarySearchTree&&) = default;
    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    bool insert(const Course& course) {
        if (!insertNode(root, course)) return false;
        ++count;
        return true;
    }

    const Course* search(const std::string& courseNumber) const {
        const Node* current = root.get();
        const std::string key = upperCase(trim(courseNumber));
        while (current) {
            if (key == current->course.courseNumber) return &current->course;
            current = key < current->course.courseNumber
                ? current->left.get() : current->right.get();
        }
        return nullptr;
    }

    void printAll() const { printInOrder(root.get()); }
    std::size_t size() const { return count; }
    bool empty() const { return count == 0; }
};

// Read all rows into a temporary tree, so a failed load does not erase valid data.
bool loadCourses(const std::string& fileName, BinarySearchTree& courses) {
    std::ifstream input(fileName);
    if (!input.is_open()) {
        std::cout << "Error: Unable to open file: " << fileName << "\n";
        return false;
    }

    BinarySearchTree temporary;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;

        std::vector<std::string> fields;
        if (!parseCsvLine(line, fields) || fields.size() < 2 ||
            fields[0].empty() || fields[1].empty()) {
            std::cout << "Error: Invalid course format on line " << lineNumber << ".\n";
            return false;
        }

        Course course;
        course.courseNumber = upperCase(fields[0]);
        course.title = fields[1];
        for (std::size_t i = 2; i < fields.size(); ++i) {
            // The provided CSV has trailing commas; ignore empty prerequisites.
            if (!fields[i].empty())
                course.prerequisites.push_back(upperCase(fields[i]));
        }
        if (!temporary.insert(course)) {
            std::cout << "Error: Duplicate course number " << course.courseNumber
                      << " on line " << lineNumber << ".\n";
            return false;
        }
    }
    if (input.bad()) {
        std::cout << "Error: File read failed.\n";
        return false;
    }
    if (temporary.empty()) {
        std::cout << "Error: The file contains no courses.\n";
        return false;
    }

    // Replace the existing tree only after the new file has loaded successfully.
    courses = std::move(temporary);
    std::cout << "Successfully loaded " << courses.size() << " courses.\n";
    return true;
}

void printCourse(const BinarySearchTree& courses, const std::string& number) {
    const Course* course = courses.search(number);
    if (!course) {
        std::cout << "Error: Course " << upperCase(trim(number)) << " was not found.\n";
        return;
    }

    std::cout << course->courseNumber << ", " << course->title << '\n';
    if (course->prerequisites.empty()) {
        std::cout << "Prerequisites: None\n";
        return;
    }

    // Show each prerequisite's number AND its title, as required by Project Two.
    std::cout << "Prerequisites:\n";
    for (const std::string& prerequisite : course->prerequisites) {
        const Course* matchingCourse = courses.search(prerequisite);
        std::cout << "  " << prerequisite;
        if (matchingCourse) std::cout << ", " << matchingCourse->title;
        else std::cout << " (course not found in loaded data)";
        std::cout << '\n';
    }
}

void printMenu() {
    std::cout << "\n    1. Load Data Structure.\n"
                 "    2. Print Course List.\n"
                 "    3. Print Course.\n"
                 "    9. Exit\n\n"
                 "What would you like to do? ";
}

int main() {
    BinarySearchTree courses;
    std::cout << "Welcome to the course planner.\n";

    while (true) {
        printMenu();
        std::string input;
        if (!std::getline(std::cin, input)) break;
        input = trim(input);

        if (input == "1") {
            std::cout << "Enter the course data file name: ";
            std::string fileName;
            if (!std::getline(std::cin, fileName)) break;
            fileName = trim(fileName);
            if (fileName.empty()) {
                std::cout << "Error: File name cannot be empty.\n";
            } else {
                loadCourses(fileName, courses);
            }
        } else if (input == "2") {
            if (courses.empty()) {
                std::cout << "Error: Load course data using option 1 first.\n";
            } else {
                std::cout << "\nHere is a sample schedule:\n\n";
                courses.printAll();
            }
        } else if (input == "3") {
            if (courses.empty()) {
                std::cout << "Error: Load course data using option 1 first.\n";
            } else {
                std::cout << "What course do you want to know about? ";
                std::string number;
                if (!std::getline(std::cin, number)) break;
                if (trim(number).empty())
                    std::cout << "Error: Enter a course number.\n";
                else
                    printCourse(courses, number);
            }
        } else if (input == "9") {
            std::cout << "Thank you for using the course planner!\n";
            break;
        } else {
            std::cout << (input.empty() ? "(blank)" : input)
                      << " is not a valid option.\n";
        }
    }
    return 0;
}

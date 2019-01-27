#include <string>
#include <vector>

std::vector<std::string::size_type> findBreakPoints(
	std::string content, 
	std::vector<int> boundaries
) {
	std::vector<std::string::size_type> breakPoints{};
	auto found = content.find('\n');
	breakPoints.push_back(found);
	found = content.find('\n', found + 1);
	breakPoints.push_back(found);
	return breakPoints;
}

std::string extractFunction(
	std::string original, 
	std::vector<int> lineBoundaries, 
	std::string newName
) {
	auto points = findBreakPoints(original, lineBoundaries);
	return 
		original.substr(0, points.front()) +
		original.substr(points.back()) +
		"\n"
		"\n"
		"void " + newName + "() {\n" +
		original.substr(points.front() + 1, points.back() - points.front()) +
		"}";
}

#include <gtest/gtest.h>
#include <string>

void assertEqual(std::string expected, std::string actual) {
	EXPECT_EQ(expected, actual);
}

class ExtractFunctionTests : public ::testing::Test {

};

TEST_F(ExtractFunctionTests, oneLineNoArgumentsVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    b();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    a();\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    a();\n"
			"    b();\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}
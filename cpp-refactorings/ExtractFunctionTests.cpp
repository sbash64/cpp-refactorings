#include <string>
#include <vector>

struct LineBreaks {
	std::string::size_type first;
	std::string::size_type second;
};

struct LineBoundaries {
	int first;
	int last;
};

LineBreaks findLineBreaks(
	std::string content, 
	LineBoundaries boundaries
) {
	LineBreaks breakPoints{};
	auto found = content.find('\n');
	auto something = boundaries.first - 1;
	for (int i = 0; i < something - 1; ++i)
		found = content.find('\n', found + 1);
	breakPoints.first = found;
	for (int i = 0; i < boundaries.last - something; ++i)
		found = content.find('\n', found + 1);
	breakPoints.second = found;
	return breakPoints;
}

std::string extractFunction(
	std::string original, 
	LineBoundaries lineBoundaries,
	std::string newName
) {
	auto points = findLineBreaks(original, lineBoundaries);
	return 
		original.substr(0, points.first) +
		original.substr(points.second) +
		"\n"
		"\n"
		"void " + newName + "() {\n" +
		original.substr(points.first + 1, points.second - points.first) +
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

TEST_F(ExtractFunctionTests, twoLinesNoArgumentsVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    c();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    a();\n"
		"    b();\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    a();\n"
			"    b();\n"
			"    c();\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}
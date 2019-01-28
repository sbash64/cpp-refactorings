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

LineBreaks findParameterBreaks(std::string line) {
	LineBreaks breaks{};
	auto found = line.find('(');
	breaks.first = found;
	found = line.find(')', found + 1);
	breaks.second = found;
	return breaks;
}

std::string extractFunction(
	std::string original, 
	LineBoundaries lineBoundaries,
	std::string newName
) {
	auto points = findLineBreaks(original, lineBoundaries);
	auto extracted = original.substr(points.first + 1, points.second - points.first);
	auto parameters = findParameterBreaks(extracted);
	auto containingFunctionLine = original.substr(0, points.first + 1);
	auto signatureParameters = findParameterBreaks(containingFunctionLine);
	return 
		original.substr(0, points.first + 1) +
		"    " + newName + "(" + extracted.substr(parameters.first + 1, parameters.second - parameters.first - 1) + ");" +
		original.substr(points.second) +
		"\n"
		"\n"
		"void " + newName + "(" + containingFunctionLine.substr(signatureParameters.first + 1, signatureParameters.second - signatureParameters.first - 1) + ") {\n" +
		extracted +
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
		"    g();\n"
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
		"    g();\n"
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

TEST_F(ExtractFunctionTests, oneLineOneArgumentVoidReturn) {
	assertEqual(
		"void f(int x) {\n"
		"    g(x);\n"
		"    b();\n"
		"}\n"
		"\n"
		"void g(int x) {\n"
		"    a(x);\n"
		"}",
		extractFunction(
			"void f(int x) {\n"
			"    a(x);\n"
			"    b();\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

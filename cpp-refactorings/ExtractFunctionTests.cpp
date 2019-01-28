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
	breaks.first = line.find('(');
	breaks.second = line.find(')', breaks.first + 1);
	return breaks;
}

std::string betweenBreaks(std::string content, LineBreaks breaks) {
	return content.substr(
		breaks.first + 1,
		breaks.second - breaks.first - 1
	);
}

std::string betweenIncludingSecondBreak(
	std::string content, 
	LineBreaks breaks
) {
	return content.substr(
		breaks.first + 1,
		breaks.second - breaks.first
	);
}

std::string extractFunction(
	std::string original, 
	LineBoundaries lineBoundaries,
	std::string newName
) {
	auto extractionBreaks = findLineBreaks(original, lineBoundaries);
	auto extractedBody = betweenIncludingSecondBreak(original, extractionBreaks);
	auto parameters = findParameterBreaks(extractedBody);
	auto extractedFunctionParameterInvocation = betweenBreaks(extractedBody, parameters);
	auto parentFunctionFirstLine = original.substr(0, extractionBreaks.first + 1);
	auto parentFunctionParameterDeclaration = findParameterBreaks(parentFunctionFirstLine);
	auto extractedFunctionParameterDeclaration = betweenBreaks(parentFunctionFirstLine, parentFunctionParameterDeclaration);
	auto extractedFunctionInvocation = newName + "(" + extractedFunctionParameterInvocation + ");";
	auto remainingParentFunction = original.substr(extractionBreaks.second);
	auto extractedFunctionSignature = "void " + newName + "(" + extractedFunctionParameterDeclaration + ")";
	return 
		parentFunctionFirstLine +
		"    " + extractedFunctionInvocation +
		remainingParentFunction +
		"\n"
		"\n" +
		extractedFunctionSignature + " {\n" +
		extractedBody +
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

TEST_F(ExtractFunctionTests, twoLinesOneArgumentVoidReturn) {
	assertEqual(
		"void f(int x) {\n"
		"    g(x);\n"
		"    c();\n"
		"}\n"
		"\n"
		"void g(int x) {\n"
		"    a(x);\n"
		"    b(x);\n"
		"}",
		extractFunction(
			"void f(int x) {\n"
			"    a(x);\n"
			"    b(x);\n"
			"    c();\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}

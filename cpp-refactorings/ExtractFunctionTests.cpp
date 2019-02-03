#include <string>
#include <vector>

struct ContentBreaks {
	std::string::size_type first;
	std::string::size_type second;
};

struct LineBoundaries {
	int first;
	int last;
};

ContentBreaks findLineBreaks(
	std::string content, 
	LineBoundaries boundaries
) {
	ContentBreaks breaks{};
	auto found = content.find('\n');
	auto something = boundaries.first - 1;
	for (int i = 0; i < something - 1; ++i)
		found = content.find('\n', found + 1);
	breaks.first = found;
	for (int i = 0; i < boundaries.last - something; ++i)
		found = content.find('\n', found + 1);
	breaks.second = found;
	return breaks;
}

ContentBreaks findParameterListBreaks(std::string line) {
	ContentBreaks breaks{};
	breaks.first = line.find('(');
	breaks.second = line.find(')', breaks.first + 1);
	return breaks;
}

std::string betweenBreaks(std::string content, ContentBreaks breaks) {
	return content.substr(
		breaks.first + 1,
		breaks.second - breaks.first - 1
	);
}

std::string betweenIncludingSecondBreak(
	std::string content, 
	ContentBreaks breaks
) {
	return content.substr(
		breaks.first + 1,
		breaks.second - breaks.first
	);
}

std::string upToAndIncludingFirstBreak(
	std::string content,
	ContentBreaks breaks
) {
	return content.substr(0, breaks.first + 1);
}

std::string extractFunction(
	std::string original, 
	LineBoundaries lineBoundaries,
	std::string newName
) {
	auto extractionBreaks = findLineBreaks(original, lineBoundaries);
	auto extractedBody = betweenIncludingSecondBreak(original, extractionBreaks);
	auto extractedFunctionParameterInvocation = 
		betweenBreaks(extractedBody, findParameterListBreaks(extractedBody));
	auto parentFunctionFirstLine = upToAndIncludingFirstBreak(original, extractionBreaks);
	auto parentFunctionParameterList = findParameterListBreaks(parentFunctionFirstLine);
	auto extractedFunctionParameterList = extractedFunctionParameterInvocation.empty() 
		? ""
		: betweenBreaks(parentFunctionFirstLine, parentFunctionParameterList);
	auto extractedFunctionInvocation = newName + "(" + extractedFunctionParameterInvocation + ");";
	auto remainingParentFunction = original.substr(extractionBreaks.second);
	auto extractedFunctionDeclaration = "void " + newName + "(" + extractedFunctionParameterList + ")";
	return 
		parentFunctionFirstLine +
		"    " + extractedFunctionInvocation +
		remainingParentFunction +
		"\n"
		"\n" +
		extractedFunctionDeclaration + " {\n" +
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

TEST_F(ExtractFunctionTests, oneLineTwoArgumentsVoidReturn) {
	assertEqual(
		"void f(int x, int y) {\n"
		"    g(x, y);\n"
		"    b();\n"
		"}\n"
		"\n"
		"void g(int x, int y) {\n"
		"    a(x, y);\n"
		"}",
		extractFunction(
			"void f(int x, int y) {\n"
			"    a(x, y);\n"
			"    b();\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, twoLinesTwoArgumentsVoidReturn) {
	assertEqual(
		"void f(int x, int y) {\n"
		"    g(x, y);\n"
		"    c();\n"
		"}\n"
		"\n"
		"void g(int x, int y) {\n"
		"    a(x, y);\n"
		"    b(x, y);\n"
		"}",
		extractFunction(
			"void f(int x, int y) {\n"
			"    a(x, y);\n"
			"    b(x, y);\n"
			"    c();\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineOneUnusedArgumentVoidReturn) {
	assertEqual(
		"void f(int) {\n"
		"    g();\n"
		"    b();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    a();\n"
		"}",
		extractFunction(
			"void f(int) {\n"
			"    a();\n"
			"    b();\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

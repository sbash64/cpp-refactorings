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

std::string::size_type find_nth_element(std::string content, int n, char what) {
	if (n < 1)
		return std::string::npos;
	auto found = content.find(what);
	for (int i = 0; i < n - 1; ++i)
		found = content.find(what, found + 1);
	return found;
}

ContentBreaks findLineBreaks(
	std::string content, 
	LineBoundaries boundaries
) {
	ContentBreaks breaks{};
	breaks.first = find_nth_element(content, boundaries.first - 1, '\n');
	breaks.second = find_nth_element(content, boundaries.last, '\n');
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


std::string secondBreakAndAfter(
	std::string content,
	ContentBreaks breaks
) {
	return content.substr(breaks.second);
}

std::string parameterList(std::string content) {
	return 
		betweenBreaks(
			content,
			findParameterListBreaks(content)
		);
}

bool containsAssignment(std::string content) {
	return content.find("=") != std::string::npos;
}

std::string returnedType(std::string content) {
	auto found = content.find("=");
	auto upUntilAssignment = content.substr(0, found);
	auto endOfReturnedName = upUntilAssignment.find_last_not_of(" ");
	auto upThroughEndOfReturnedName = upUntilAssignment.substr(0, endOfReturnedName + 1);
	auto beforeReturnedName = upThroughEndOfReturnedName.find_last_of(" ");
	auto upUntilReturnedName = upThroughEndOfReturnedName.substr(0, beforeReturnedName + 1);
	auto endOfReturnedType = upUntilReturnedName.find_last_not_of(" ");
	auto upThroughEndOfReturnedType = upUntilReturnedName.substr(0, endOfReturnedType + 1);
	auto beforeReturnedType = upThroughEndOfReturnedType.find_last_of(" ");
	if (beforeReturnedType == std::string::npos)
		return upThroughEndOfReturnedType;
	else
		return upThroughEndOfReturnedType.substr(beforeReturnedType + 1);
}

std::string returnType(std::string content) {
	if (containsAssignment(content))
		return returnedType(content);
	else
		return "void";
}

std::string returnName(std::string content) {
	auto found = content.find("=");
	auto upUntilAssignment = content.substr(0, found);
	auto endOfReturnedName = upUntilAssignment.find_last_not_of(" ");
	auto upThroughEndOfReturnedName = upUntilAssignment.substr(0, endOfReturnedName + 1);
	auto beforeReturnedName = upThroughEndOfReturnedName.find_last_of(" ");
	if (beforeReturnedName == std::string::npos)
		return upThroughEndOfReturnedName;
	else
		return upThroughEndOfReturnedName.substr(beforeReturnedName + 1);
}

std::string extractFunction(
	std::string original, 
	LineBoundaries lineBoundaries,
	std::string newName
) {
	auto extractionBreaks = findLineBreaks(original, lineBoundaries);
	auto extractedBody = betweenIncludingSecondBreak(original, extractionBreaks);
	auto extractedFunctionInvokedParameterList = parameterList(extractedBody);
	auto parentFunctionFirstLine = upToAndIncludingFirstBreak(original, extractionBreaks);
	auto extractedFunctionParameterList = extractedFunctionInvokedParameterList.empty()
		? ""
		: parameterList(parentFunctionFirstLine);

	auto extractedFunctionReturnType = returnType(extractedBody);
	std::string extractedFunctionReturnAssignment{};
	std::string extractedFunctionReturnStatement{};
	if (extractedFunctionReturnType != "void") {
		auto extractedFunctionReturnName = returnName(extractedBody);
		extractedFunctionReturnAssignment = 
			extractedFunctionReturnType + " " + extractedFunctionReturnName + " = ";
		extractedFunctionReturnStatement = "    return " + extractedFunctionReturnName + ";\n";
	}

	auto extractedFunctionInvocation = 
		extractedFunctionReturnAssignment + newName + 
		"(" + extractedFunctionInvokedParameterList + ");";
	auto remainingParentFunction = secondBreakAndAfter(original, extractionBreaks);
	auto extractedFunctionDeclaration = 
		extractedFunctionReturnType + " " + newName + 
		"(" + extractedFunctionParameterList + ")";

	return 
		parentFunctionFirstLine +
		"    " + extractedFunctionInvocation +
		remainingParentFunction +
		"\n"
		"\n" +
		extractedFunctionDeclaration + " {\n" +
		extractedBody + extractedFunctionReturnStatement +
		"}";
}

#include <gtest/gtest.h>
#include <string>

void assertEqual(std::string expected, std::string actual) {
	EXPECT_EQ(expected, actual);
}

class ExtractFunctionTests : public ::testing::Test {

};

TEST_F(ExtractFunctionTests, oneLineBeginningOfFunctionNoArgumentsVoidReturn) {
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

TEST_F(ExtractFunctionTests, oneLineEndOfFunctionNoArgumentsVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    a();\n"
		"    g();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    b();\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    a();\n"
			"    b();\n"
			"}",
			{ 3, 3 },
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

TEST_F(ExtractFunctionTests, oneLineOneFlyOverArgumentVoidReturn) {
	assertEqual(
		"void f(int x) {\n"
		"    g();\n"
		"    b(x);\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    a();\n"
		"}",
		extractFunction(
			"void f(int x) {\n"
			"    a();\n"
			"    b(x);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineNoArgumentsNonVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    int x = g();\n"
		"    b();\n"
		"}\n"
		"\n"
		"int g() {\n"
		"    int x = a();\n"
		"    return x;\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a();\n"
			"    b();\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

#include <string>
#include <vector>

class CodeString {
	std::string content;
public:
	struct ContentBreaks {
		std::string::size_type first;
		std::string::size_type second;
	};

	struct LineBoundaries {
		int first;
		int last;
	};

	CodeString(std::string s = {}) : content{ std::move(s) } {}

	std::string::size_type find_nth_element(int n, char what) {
		if (n < 1)
			return std::string::npos;
		auto found = content.find(what);
		for (int i = 0; i < n - 1; ++i)
			found = content.find(what, found + 1);
		return found;
	}

	ContentBreaks findLineBreaks(
		LineBoundaries boundaries
	) {
		ContentBreaks breaks{};
		breaks.first = find_nth_element(boundaries.first - 1, '\n');
		breaks.second = find_nth_element(boundaries.last, '\n');
		return breaks;
	}

	ContentBreaks findParameterListBreaks() {
		ContentBreaks breaks{};
		breaks.first = content.find('(');
		breaks.second = content.find(')', breaks.first + 1);
		return breaks;
	}

	CodeString betweenBreaks(ContentBreaks breaks) {
		return content.substr(
			breaks.first + 1,
			breaks.second - breaks.first - 1
		);
	}

	CodeString betweenIncludingSecondBreak(
		ContentBreaks breaks
	) {
		return content.substr(
			breaks.first + 1,
			breaks.second - breaks.first
		);
	}

	CodeString upToAndIncludingFirstBreak(
		ContentBreaks breaks
	) {
		return content.substr(0, breaks.first + 1);
	}


	CodeString secondBreakAndAfter(
		ContentBreaks breaks
	) {
		return content.substr(breaks.second);
	}

	CodeString parameterList() {
		return betweenBreaks(findParameterListBreaks());
	}

	bool contains(std::string what) {
		return content.find(what) != std::string::npos;
	}

	bool containsAssignment() {
		return contains("=");
	}

	CodeString upIncludingLastNotOf(std::string what) {
		return content.substr(0, content.find_last_not_of(what) + 1);
	}

	CodeString upUntilLastOf(std::string what) {
		return content.substr(0, content.find_last_of(what));
	}

	CodeString upThroughLastOf(std::string what) {
		return content.substr(0, content.find_last_of(what) + 1);
	}

	CodeString followingLastOf(std::string what) {
		auto lastOf = content.find_last_of(what);
		return lastOf == std::string::npos
			? content
			: content.substr(lastOf + 1);
	}

	CodeString returnName() {
		return upUntilLastOf("=").upIncludingLastNotOf(" ").followingLastOf(" ");
	}

	CodeString returnedType() {
		return upUntilLastOf("=")
			.upIncludingLastNotOf(" ")
			.upThroughLastOf(" ")
			.upIncludingLastNotOf(" ")
			.followingLastOf(" ");
	}

	CodeString returnType() {
		using namespace std::string_literals;
		if (containsAssignment())
			return returnedType();
		else
			return "void"s;
	}

	CodeString operator+(const CodeString &b) const {
		return content + std::string{ b };
	}

	operator std::string() const { return content; }
};

std::string extractFunction(
	std::string original, 
	CodeString::LineBoundaries lineBoundaries,
	std::string newName
) {
	CodeString originalAsCodeString{ original };
	auto extractionBreaks = originalAsCodeString.findLineBreaks(lineBoundaries);
	auto extractedBody = originalAsCodeString.betweenIncludingSecondBreak(extractionBreaks);
	auto extractedFunctionInvokedParameterList = extractedBody.parameterList();
	auto parentFunctionFirstLine = originalAsCodeString.upToAndIncludingFirstBreak(extractionBreaks);
	CodeString extractedFunctionParameterList = 
		std::string{ extractedFunctionInvokedParameterList }.empty()
		? CodeString{}
		: parentFunctionFirstLine.parameterList();

	auto extractedFunctionReturnType = extractedBody.returnType();
	CodeString extractedFunctionReturnAssignment{};
	CodeString extractedFunctionReturnStatement{};
	using namespace std::string_literals;
	if (std::string{ extractedFunctionReturnType } != "void") {
		auto extractedFunctionReturnName = extractedBody.returnName();
		extractedFunctionReturnAssignment = 
			extractedFunctionReturnType + " "s + extractedFunctionReturnName + " = "s;
		extractedFunctionReturnStatement = 
			"    return "s + std::string{ extractedFunctionReturnName } +";\n"s;
	}

	auto extractedFunctionInvocation = 
		extractedFunctionReturnAssignment + newName + 
		"("s + extractedFunctionInvokedParameterList + ");"s;
	auto remainingParentFunction = originalAsCodeString.secondBreakAndAfter(extractionBreaks);
	auto extractedFunctionDeclaration = 
		extractedFunctionReturnType + " "s + newName + 
		"("s + extractedFunctionParameterList + ")"s;

	return 
		parentFunctionFirstLine +
		"    "s + extractedFunctionInvocation +
		remainingParentFunction +
		"\n"s
		"\n" +
		extractedFunctionDeclaration + " {\n"s +
		extractedBody + extractedFunctionReturnStatement +
		"}"s;
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
		"    b(x);\n"
		"}\n"
		"\n"
		"int g() {\n"
		"    int x = a();\n"
		"    return x;\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a();\n"
			"    b(x);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, twoLinesNoArgumentsNonVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    int x = g();\n"
		"    c(x);\n"
		"}\n"
		"\n"
		"int g() {\n"
		"    int x = a();\n"
		"    b();\n"
		"    return x;\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a();\n"
			"    b();\n"
			"    c(x);\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineOneArgumentNonVoidReturn) {
	assertEqual(
		"void f(int x) {\n"
		"    int y = g(x);\n"
		"    b(y);\n"
		"}\n"
		"\n"
		"int g(int x) {\n"
		"    int y = a(x);\n"
		"    return y;\n"
		"}",
		extractFunction(
			"void f(int x) {\n"
			"    int y = a(x);\n"
			"    b(y);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

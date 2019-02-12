#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

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

	std::string extractFunction(
		CodeString::LineBoundaries lineBoundaries,
		std::string newName
	) {
		auto extractionBreaks = findLineBreaks(lineBoundaries);
		auto extractedBody = betweenIncludingSecondBreak(extractionBreaks);
		auto extractedFunctionInvokedParameterList =
			commaSeparated(extractedBody.invokedParameters());

		auto extractedFunctionReturnType = extractedBody.lastAssignmentReturnType();
		CodeString extractedFunctionReturnAssignment{};
		CodeString extractedFunctionReturnStatement{};
		using namespace std::string_literals;
		if (extractedFunctionReturnType.content != "void") {
			auto extractedFunctionReturnName = extractedBody.lastAssignedName();
			extractedFunctionReturnAssignment =
				extractedFunctionReturnType + " "s + extractedFunctionReturnName + " = "s;
			extractedFunctionReturnStatement =
				"    return "s + extractedFunctionReturnName.content + ";\n"s;
		}

		auto extractedFunctionInvocation =
			extractedFunctionReturnAssignment + newName +
			"("s + extractedFunctionInvokedParameterList + ");"s;
		auto parentFunctionFirstLine = upToAndIncludingFirstBreak(extractionBreaks);
		auto extractedFunctionParameters = extractedBody.invokedParameters();
		auto extractedFunctionParameterTypes =
			parentFunctionFirstLine.parameterTypes(extractedFunctionParameters);
		std::vector<std::string> extractedFunctionParametersWithTypes{};
		for (auto item : extractedFunctionParameterTypes)
			extractedFunctionParametersWithTypes.push_back(item.second.content + " " + item.first);
		auto extractedFunctionParameterList =
			commaSeparated(extractedFunctionParametersWithTypes);
		auto extractedFunctionDeclaration =
			extractedFunctionReturnType + " "s + newName +
			"("s + extractedFunctionParameterList + ")"s;
		auto remainingParentFunction = secondBreakAndAfter(extractionBreaks);

		return
			parentFunctionFirstLine.content +
			"    "s + extractedFunctionInvocation.content +
			remainingParentFunction.content +
			"\n"s
			"\n" +
			extractedFunctionDeclaration.content + " {\n"s +
			extractedBody.content + extractedFunctionReturnStatement.content +
			"}"s;

	}

	ContentBreaks findLineBreaks(
		LineBoundaries boundaries
	) {
		ContentBreaks breaks{};
		breaks.first = find_nth_element(boundaries.first - 1, '\n');
		breaks.second = find_nth_element(boundaries.last, '\n');
		return breaks;
	}

	std::string::size_type find_nth_element(int n, char what) {
		auto found = std::string::npos;
		for (int i = 0; i < n; ++i)
			found = content.find(what, found + 1);
		return found;
	}

	CodeString firstParameterList() {
		return betweenBreaks(findFirstParameterListBreaks());
	}

	ContentBreaks findFirstParameterListBreaks() {
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

	CodeString lastAssignmentReturnType() {
		return containsAssignment()
			? lastAssignmentReturnType_()
			: CodeString{ "void" };
	}

	bool containsAssignment() {
		return contains("=");
	}

	bool contains(std::string what) {
		return content.find(what) != std::string::npos;
	}

	CodeString lastAssignmentReturnType_() {
		return upUntilLastOf("=")
			.upIncludingLastNotOf(" ")
			.upThroughLastOf(" ")
			.upIncludingLastNotOf(" ")
			.followingLastOf(" ");
	}

	CodeString upUntilLastOf(std::string what) {
		return content.substr(0, content.find_last_of(what));
	}

	CodeString upIncludingLastNotOf(std::string what) {
		return content.substr(0, content.find_last_not_of(what) + 1);
	}

	CodeString followingLastOf(std::string what) {
		return content.substr(content.find_last_of(what) + 1);
	}

	CodeString followingLastOfEither(std::string this_, std::string that_) {
		return content.substr(
			std::max(content.find_last_of(this_), content.find_last_of(that_)) + 1);
	}

	CodeString upThroughLastOf(std::string what) {
		return content.substr(0, content.find_last_of(what) + 1);
	}

	CodeString lastAssignedName() {
		return upUntilLastOf("=").upIncludingLastNotOf(" ").followingLastOf(" ");
	}

	CodeString operator+(const CodeString &b) const {
		return content + b.content;
	}

	std::vector<CodeString> splitParameterList() {
		std::vector<CodeString> deseparated;
		CodeString search{ *this };
		for (auto found = search.content.find(","); found != std::string::npos; ) {
			deseparated.push_back(search.content.substr(0, found));
			search = { search.content.substr(found + 2) };
			found = search.content.find(",");
		}
		if (!search.content.empty())
			deseparated.push_back(search.content);
		return deseparated;
	}

	std::set<std::string> invokedParameters() {
		std::set<std::string> parameters{};
		CodeString search{ *this };
		for (
			auto breaks = search.findFirstParameterListBreaks(); 
			breaks.first != std::string::npos; 
			breaks = search.findFirstParameterListBreaks()
		) {
			for (auto p : search.betweenBreaks(breaks).splitParameterList())
				parameters.insert(p.content);
			search = search.secondBreakAndAfter(breaks);
		}
		return parameters;
	}

	template<typename container>
	CodeString commaSeparated(container items) {
		CodeString result{};
		for (auto it = items.begin(); it != items.end(); ++it) {
			result.content += *it;
			if (std::next(it) != items.end())
				result.content += ", ";
		}
		return result;
	}

	std::map<std::string, CodeString> parameterTypes(std::set<std::string> parameters) {
		std::map<std::string, CodeString> types{};
		for (auto parameter : parameters)
			types[parameter] = parameterType(parameter);
		return types;
	}

	CodeString parameterType(std::string parameter) {
		return upUntilLastOf(parameter)
			.upIncludingLastNotOf(" ")
			.followingLastOfEither("(", " ");
	}
};

std::string extractFunction(
	std::string original, 
	CodeString::LineBoundaries lineBoundaries,
	std::string newName
) {
	CodeString code{ original };
	return code.extractFunction(lineBoundaries, newName);
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

TEST_F(ExtractFunctionTests, oneLineNoArgumentsBinaryExpressionNonVoidReturn) {
	assertEqual(
		"void f() {\n"
		"    int x = g();\n"
		"    c(x);\n"
		"}\n"
		"\n"
		"int g() {\n"
		"    int x = a() + b();\n"
		"    return x;\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a() + b();\n"
			"    c(x);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineOneArgumentBinaryExpressionNonVoidReturn) {
	assertEqual(
		"void f(int x) {\n"
		"    int y = g(x);\n"
		"    c(y);\n"
		"}\n"
		"\n"
		"int g(int x) {\n"
		"    int y = a(x) + b();\n"
		"    return y;\n"
		"}",
		extractFunction(
			"void f(int x) {\n"
			"    int y = a(x) + b();\n"
			"    c(y);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineTwoArgumentsBinaryExpressionNonVoidReturn) {
	assertEqual(
		"void f(int x, int y) {\n"
		"    int z = g(x, y);\n"
		"    c(z);\n"
		"}\n"
		"\n"
		"int g(int x, int y) {\n"
		"    int z = a(x) + b(y);\n"
		"    return z;\n"
		"}",
		extractFunction(
			"void f(int x, int y) {\n"
			"    int z = a(x) + b(y);\n"
			"    c(z);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, oneLineOneArgumentOneFlyoverVoidReturn) {
	assertEqual(
		"void f(int x, int y) {\n"
		"    g(x);\n"
		"    b(y);\n"
		"}\n"
		"\n"
		"void g(int x) {\n"
		"    a(x);\n"
		"}",
		extractFunction(
			"void f(int x, int y) {\n"
			"    a(x);\n"
			"    b(y);\n"
			"}",
			{ 2, 2 },
			"g"
		)
	);
}
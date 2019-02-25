#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>

template<typename container>
class Set {
	container items;
public:
	explicit Set(container items) : items{ std::move(items) } {}

	template<typename other>
	container excluding(other others) {
		container exclusion{};
		std::set_difference(
			items.begin(),
			items.end(),
			others.begin(),
			others.end(),
			std::inserter(exclusion, exclusion.begin())
		);
		return exclusion;
	}
};

class Code {
	std::string content;
public:
	struct ContentBounds {
		std::string::size_type beginning;
		std::string::size_type end;
	};

	struct ExtractedLines {
		int first;
		int last;
	};

	Code(std::string s = {}) : content{ std::move(s) } {}

	std::string extractFunction(
		ExtractedLines extractedLines,
		std::string newName
	) {
		auto extractionBounds = lineBounds(extractedLines);
		auto remainingParentFunction = endAndFollowing(extractionBounds);
		auto parentFunctionBeginning = upToIncludingBeginning(extractionBounds);
		auto parentFunctionParameters =
			parentFunctionBeginning.firstParameterList().parametersWithoutTypes();
		auto neededReturnedFromExtracted = 
			Set{ remainingParentFunction.undeclaredIdentifiers() }
			.excluding(parentFunctionParameters);
		auto extractedBody = betweenIncludingEnd(extractionBounds);
		using namespace std::string_literals;
		Code extractedFunctionReturnType = neededReturnedFromExtracted.size()
			? extractedBody.parameterType(*neededReturnedFromExtracted.begin())
			: "void"s;
		Code extractedFunctionReturnAssignment{};
		Code extractedFunctionReturnStatement{};
		if (extractedFunctionReturnType.content != "void") {
			auto extractedFunctionReturnName = extractedBody.lastAssignedName();
			extractedFunctionReturnAssignment =
				extractedFunctionReturnType + " "s + extractedFunctionReturnName + " = "s;
			extractedFunctionReturnStatement =
				"    return "s + extractedFunctionReturnName.content + ";\n"s;
		}

		auto extractedFunctionInvocation =
			extractedFunctionReturnAssignment + newName +
			"("s + commaSeparated(extractedBody.undeclaredIdentifiers()) + ");"s;
		auto extractedFunctionParameterList =
			commaSeparated(
				parentFunctionBeginning.deduceTypes(
					extractedBody.undeclaredIdentifiers()
				)
			);
		auto extractedFunctionDeclaration =
			extractedFunctionReturnType + " "s + newName +
			"("s + extractedFunctionParameterList + ")"s;

		return
			parentFunctionBeginning.content +
			"    "s + extractedFunctionInvocation.content +
			remainingParentFunction.content +
			"\n"s
			"\n" +
			extractedFunctionDeclaration.content + " {\n"s +
			extractedBody.content + extractedFunctionReturnStatement.content +
			"}"s;

	}

	ContentBounds lineBounds(
		ExtractedLines boundaries
	) {
		ContentBounds bounds{};
		bounds.beginning = find_nth_element(boundaries.first - 1U, '\n');
		bounds.end = find_nth_element(boundaries.last, '\n');
		return bounds;
	}

	std::string::size_type find_nth_element(int n, char what) {
		auto found = std::string::npos;
		for (int i = 0; i < n; ++i)
			found = content.find(what, found + 1U);
		return found;
	}

	Code firstParameterList() {
		return between(firstParameterListBounds());
	}

	ContentBounds firstParameterListBounds() {
		ContentBounds bounds{};
		bounds.beginning = content.find('(');
		bounds.end = content.find(')', bounds.beginning + 1U);
		return bounds;
	}

	Code between(ContentBounds bounds) {
		return content.substr(
			bounds.beginning + 1U,
			bounds.end - bounds.beginning - 1U
		);
	}

	Code betweenIncludingEnd(ContentBounds bounds) {
		return content.substr(
			bounds.beginning + 1U,
			bounds.end - bounds.beginning
		);
	}

	Code upToIncludingBeginning(ContentBounds bounds) {
		return content.substr(0, bounds.beginning + 1U);
	}

	Code endAndFollowing(ContentBounds bounds) {
		return content.substr(bounds.end);
	}

	Code upUntilLastOf(std::string what) {
		return content.substr(0, content.find_last_of(std::move(what)));
	}

	Code upUntilFirstOf(std::string what) {
		return content.substr(0, content.find_first_of(std::move(what)));
	}
	
	Code upIncludingLastNotOf(std::string what) {
		return content.substr(0, content.find_last_not_of(std::move(what)) + 1U);
	}

	Code followingLastOf(std::string what) {
		return content.substr(content.find_last_of(std::move(what)) + 1U);
	}

	Code followingLastOfEither(std::string this_, std::string that_) {
		return content.substr(
			std::max(
				content.find_last_of(std::move(this_)) + 1U, 
				content.find_last_of(std::move(that_)) + 1U
			)
		);
	}

	Code upThroughLastOf(std::string what) {
		return content.substr(0, content.find_last_of(std::move(what)) + 1U);
	}

	Code lastAssignedName() {
		return upUntilLastOf("=").upIncludingLastNotOf(" ").followingLastOf(" ");
	}

	Code operator+(const Code &b) const {
		return content + b.content;
	}

	std::vector<Code> commaSplit() {
		std::vector<Code> split;
		auto search{ *this };
		for (
			auto found = search.content.find(","); 
			found != std::string::npos; 
			found = search.content.find(",")
		) {
			split.push_back(search.content.substr(0, found));
			search = { search.content.substr(found + 2) };
		}
		if (!search.content.empty())
			split.push_back(search.content);
		return split;
	}

	std::set<std::string> invokedParameters() {
		std::set<std::string> parameters{};
		auto search{ *this };
		for (
			auto bounds = search.firstParameterListBounds(); 
			bounds.beginning != std::string::npos; 
			bounds = search.firstParameterListBounds()
		) {
			for (auto p : search.between(bounds).commaSplit())
				parameters.insert(p.content);
			search = search.endAndFollowing(bounds);
		}
		return parameters;
	}

	std::set<std::string> undeclaredIdentifiers() {
		std::set<std::string> undeclared_{};
		auto search{ *this };
		for (auto p : invokedParameters())
			if (
				search.upUntilFirstOf(p)
				.upIncludingLastNotOf(" ")
				.followingLastOf(" ")
				.contains("(")
			)
				undeclared_.insert(p);
		return undeclared_;
	}

	bool contains(std::string what) {
		return content.find(std::move(what)) != std::string::npos;
	}
	
	template<typename container>
	Code commaSeparated(container items) {
		Code result{};
		for (auto it = items.begin(); it != items.end(); ++it) {
			result.content += *it;
			if (std::next(it) != items.end())
				result.content += ", ";
		}
		return result;
	}

	std::vector<std::string> deduceTypes(std::set<std::string> parameters) {
		std::vector<std::string> withTypes{};
		for (auto item : parameterTypes(parameters))
			withTypes.push_back(item.second.content + " " + item.first);
		return withTypes;
	}

	std::map<std::string, Code> parameterTypes(std::set<std::string> parameters) {
		std::map<std::string, Code> types{};
		for (auto parameter : parameters)
			types[parameter] = parameterType(parameter);
		return types;
	}

	Code parameterType(std::string parameter) {
		return 
			upUntilLastOf(std::move(parameter))
			.upIncludingLastNotOf(" ")
			.followingLastOfEither("(", " ");
	}

	std::vector<std::string> parametersWithoutTypes() {
		std::vector<std::string> withoutTypes{};
		for (auto s : commaSplit())
			withoutTypes.push_back(s.followingLastOf(" ").content);
		return withoutTypes;
	}
};

std::string extractFunction(
	std::string original, 
	Code::ExtractedLines lineBoundaries,
	std::string newName
) {
	Code code{ original };
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

TEST_F(ExtractFunctionTests, twoLinesNoArgumentsSecondVariableReturned) {
	assertEqual(
		"void f() {\n"
		"    int y = g();\n"
		"    c(y);\n"
		"}\n"
		"\n"
		"int g() {\n"
		"    int x = a();\n"
		"    int y = b(x);\n"
		"    return y;\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a();\n"
			"    int y = b(x);\n"
			"    c(y);\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, twoLinesOneArgumentSecondVariableReturned) {
	assertEqual(
		"void f(int z) {\n"
		"    int y = g(z);\n"
		"    c(y);\n"
		"}\n"
		"\n"
		"int g(int z) {\n"
		"    int x = a(z);\n"
		"    int y = b(x);\n"
		"    return y;\n"
		"}",
		extractFunction(
			"void f(int z) {\n"
			"    int x = a(z);\n"
			"    int y = b(x);\n"
			"    c(y);\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}

TEST_F(ExtractFunctionTests, twoLinesNoArgumentsVoidReturnDespiteAssignment) {
	assertEqual(
		"void f() {\n"
		"    g();\n"
		"    c();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    int x = a();\n"
		"    b(x);\n"
		"}",
		extractFunction(
			"void f() {\n"
			"    int x = a();\n"
			"    b(x);\n"
			"    c();\n"
			"}",
			{ 2, 3 },
			"g"
		)
	);
}
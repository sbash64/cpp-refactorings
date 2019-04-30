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
	using size_type = std::string::size_type;

	struct ContentRange {
		size_type beginning;
		size_type end;
	};

	struct ExtractedLines {
		int first;
		int last;
	};

	Code(std::string s = {}) : content{ std::move(s) } {}

	std::string extractFunction(
		const ExtractedLines &extractedLines,
		const std::string &newName
	) {
		auto extractedRange = range(extractedLines);
		auto extractedBody = betweenIncludingEnd(extractedRange);
		auto afterExtracted = endAndFollowing(extractedRange);
		auto beforeExtracted = upToIncludingBeginning(extractedRange);
		auto undeclaredIdentifiersAfterExtracted = 
			afterExtracted.remainingUndeclaredIdentifiers(beforeExtracted);
		auto extractedFunctionReturnType = 
			extractedBody.typeName(undeclaredIdentifiersAfterExtracted);
		Code extractedFunctionReturnAssignment;
		Code extractedFunctionReturnStatement;
		using namespace std::string_literals;
		if (extractedFunctionReturnType.content != "void") {
			auto extractedFunctionReturnName = extractedBody.lastAssignedName();
			extractedFunctionReturnAssignment =
				extractedFunctionReturnType + " "s + extractedFunctionReturnName + " = "s;
			extractedFunctionReturnStatement =
				Code{ "    return " } + extractedFunctionReturnName + ";\n"s;
		}

		auto extractedFunctionInvocation =
			extractedFunctionReturnAssignment + newName +
			"("s + commaSeparated(extractedBody.undeclaredIdentifiers()) + ");"s;
		auto extractedFunctionParameterList =
			commaSeparated(
				beforeExtracted.joinDeducedTypes(
					extractedBody.undeclaredIdentifiers()
				)
			);
		auto extractedFunctionDeclaration =
			extractedFunctionReturnType + " "s + newName +
			"("s + extractedFunctionParameterList + ")"s;

		return
			beforeExtracted +
			"    "s + extractedFunctionInvocation +
			afterExtracted +
			"\n"s + 
			"\n"s +
			extractedFunctionDeclaration + " {\n"s +
			extractedBody + extractedFunctionReturnStatement +
			"}"s;

	}

	Code typeName(const std::set<std::string> &types) {
		return types.size()
			? deducedType(*types.begin())
			: Code{ "void" };
	}

	std::set<std::string> remainingUndeclaredIdentifiers(const Code &code) {
		return Set{ undeclaredIdentifiers() }
		.excluding(code.firstParameterList().withoutTypes());
	}

	operator std::string() { return content;  }

	ContentRange range(const ExtractedLines &lines) {
		ContentRange range_;
		range_.beginning = findLineEnding(lines.first - 1);
		range_.end = findLineEnding(lines.last);
		return range_;
	}

	size_type findLineEnding(int n) {
		return find_nth_element(n, '\n');
	}

	size_type find_nth_element(int n, char what) {
		auto found = std::string::npos;
		for (int i = 0; i < n; ++i)
			found = find(what, found + 1U);
		return found;
	}

	size_type find(char what, size_type offset = 0) const {
		return content.find(what, offset);
	}

	Code firstParameterList() const {
		return between(firstParameterListRange());
	}

	ContentRange firstParameterListRange() const {
		ContentRange range_;
		range_.beginning = find('(');
		range_.end = find(')', range_.beginning + 1U);
		return range_;
	}

	Code between(const ContentRange &range_) const {
		return substr(
			range_.beginning + 1U,
			range_.end - range_.beginning - 1U
		);
	}

	Code substr(size_type offset, size_type count = std::string::npos) const {
		return content.substr(offset, count);
	}

	Code betweenIncludingEnd(const ContentRange &range_) {
		return substr(
			range_.beginning + 1U,
			range_.end - range_.beginning
		);
	}

	Code upToIncludingBeginning(const ContentRange &range_) {
		return substr(0, range_.beginning + 1U);
	}

	Code endAndFollowing(const ContentRange &range_) {
		return substr(range_.end);
	}

	Code upUntilLastOf(const std::string &what) {
		return substr(0, find_last_of(what));
	}

	size_type find_last_of(const std::string &what) {
		return content.find_last_of(what);
	}

	Code upUntilFirstOf(const std::string &what) {
		return substr(0, content.find_first_of(what));
	}
	
	Code upIncludingLastNotOf(const std::string &what) {
		return substr(0, content.find_last_not_of(what) + 1U);
	}

	Code followingLastOf(const std::string &what) {
		return substr(find_last_of(what) + 1U);
	}

	Code followingLastOfEither(const std::string &this_, const std::string &that_) {
		return substr(
			std::max(
				find_last_of(this_) + 1U, 
				find_last_of(that_) + 1U
			)
		);
	}

	Code upThroughLastOf(const std::string &what) {
		return substr(0, find_last_of(what) + 1U);
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
			auto found = search.find(','); 
			found != std::string::npos; 
			found = search.find(',')
		) {
			split.push_back(search.substr(0, found));
			search = { search.substr(found + 2) };
		}
		if (!search.content.empty())
			split.push_back(search.content);
		return split;
	}

	std::set<std::string> invokedParameters() {
		std::set<std::string> parameters{};
		auto search{ *this };
		for (
			auto range_ = search.firstParameterListRange(); 
			range_.beginning != std::string::npos; 
			range_ = search.firstParameterListRange()
		) {
			for (auto p : search.between(range_).commaSplit())
				parameters.insert(p.content);
			search = search.endAndFollowing(range_);
		}
		return parameters;
	}

	std::set<std::string> undeclaredIdentifiers() {
		std::set<std::string> undeclared_;
		for (auto p : invokedParameters())
			if (isUndeclared(p))
				undeclared_.insert(p);
		return undeclared_;
	}

	bool isUndeclared(const std::string &s) {
		return
			upUntilFirstOf(s)
			.upIncludingLastNotOf(" ")
			.followingLastOf(" ")
			.contains("(");
	}

	bool contains(const std::string &what) {
		return content.find(what) != std::string::npos;
	}
	
	template<typename container>
	Code commaSeparated(container items) {
		Code result;
		for (auto it = items.begin(); it != items.end(); ++it) {
			result.content += *it;
			if (std::next(it) != items.end())
				result.content += ", ";
		}
		return result;
	}

	std::vector<std::string> joinDeducedTypes(const std::set<std::string> &parameters) {
		std::vector<std::string> withTypes;
		for (auto item : deducedTypes(parameters))
			withTypes.push_back(item.second.content + " " + item.first);
		return withTypes;
	}

	std::map<std::string, Code> deducedTypes(const std::set<std::string> &parameters) {
		std::map<std::string, Code> types;
		for (auto parameter : parameters)
			types[parameter] = deducedType(parameter);
		return types;
	}

	Code deducedType(const std::string &parameter) {
		return 
			upUntilLastOf(parameter)
			.upIncludingLastNotOf(" ")
			.followingLastOfEither("(", " ");
	}

	std::vector<std::string> withoutTypes() {
		std::vector<std::string> without_;
		for (auto s : commaSplit())
			without_.push_back(s.followingLastOf(" ").content);
		return without_;
	}
};

std::string extractFunction(
	std::string original, 
	const Code::ExtractedLines &lineBoundaries,
	const std::string &newName
) {
	Code code{ std::move(original) };
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

TEST_F(ExtractFunctionTests, DISABLED_tbd) {
	assertEqual(
		"void RefactoredModel::prepareAudioPlayer(\n"
		"    AudioFrameReader &reader,\n"
		"    int framesPerBuffer,\n"
		"    std::string audioDevice\n"
		") {\n"
		"    IAudioPlayer::Preparation playing = preparation(reader, framesPerBuffer, audioDevice);\n"
		"    try {\n"
		"        player->prepareToPlay(std::move(playing));\n"
		"    }\n"
		"    catch (const IAudioPlayer::PreparationFailure &e) {\n"
		"        throw RequestFailure{ e.what() };\n"
		"    }\n"
		"}\n"
		"\n"
		"IAudioPlayer::Preparation preparation(\n"
		"    AudioFrameReader &reader,\n"
		"    int framesPerBuffer,\n"
		"    std::string audioDevice\n"
		") {\n"
		"    IAudioPlayer::Preparation playing;\n"
		"    playing.channels = reader.channels();\n"
		"    playing.sampleRate = reader.sampleRate();\n"
		"    playing.framesPerBuffer = framesPerBuffer;\n"
		"    playing.audioDevice = std::move(audioDevice);\n"
		"    return playing;\n"
		"}",
		extractFunction(
			"void RefactoredModel::prepareAudioPlayer(\n"
			"    AudioFrameReader &reader,\n"
			"    int framesPerBuffer,\n"
			"    std::string audioDevice\n"
		    ") {\n"    
			"    IAudioPlayer::Preparation playing;\n"
	        "    playing.channels = reader.channels();\n"
	        "    playing.sampleRate = reader.sampleRate();\n"
	        "    playing.framesPerBuffer = framesPerBuffer;\n"
	        "    playing.audioDevice = std::move(audioDevice);\n"
	        "    try {\n"
		    "        player->prepareToPlay(std::move(playing));\n"
	        "    }\n"
	        "    catch (const IAudioPlayer::PreparationFailure &e) {\n"
		    "        throw RequestFailure{ e.what() };\n"
	        "    }\n"
			"}",
			{ 6, 10 },
			"preparation"
		)
	);
}
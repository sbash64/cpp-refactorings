#include <string>
#include <vector>

std::string extractFunction(
	std::string original, 
	std::vector<int> lineBoundaries, 
	std::string newName
) {
	return 
		"void f() {\n"
		"    b();\n"
		"}\n"
		"\n"
		"void g() {\n"
		"    a();\n"
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
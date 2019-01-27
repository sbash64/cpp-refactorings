#include <gtest/gtest.h>

class ExtractFunctionTests : public ::testing::Test {

};

TEST_F(ExtractFunctionTests, tbd) {
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
			2,
			"g"
		)
	);
}
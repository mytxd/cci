#include "cci/lex/literal_parser.hpp"
#include "cci/basic/diagnostics.hpp"
#include "cci/basic/source_manager.hpp"
#include "cci/lex/lexer.hpp"
#include "cci/util/contracts.hpp"
#include "cci/util/unicode.hpp"
#include "gtest/gtest.h"
#include <string>
#include <string_view>

using namespace cci;

namespace {

TEST(LiteralParserTest, numericConstants)
{
  const char *code = R"(
42uL // ok, decimal
042 // ok, octal
0xDEADc0dellu // ok, hexadecimal
0uU // error: invalid suffix 'uU'
0LLL // error: invalid suffix 'LLL'
0128 // error: invalid digit '8'
314e10 // ok, double with exp
1.f // ok, float with period and float suffix
1.ef // error: empty exponent
.0 // ok, double
01238. // ok, double
0xabcde.ffP+1 // ok, hexadecimal floating constant
0xep1f // ok, hexadecimal floating constant
0x.f // error: missing exponent
18446744073709551616ull // error: overflow
)";
  DiagnosticsOptions opts;
  CompilerDiagnostics diag(opts);
  auto source = SourceManager::from_buffer(diag, code);
  auto lexer = Lexer(source);
  std::optional<Token> tok;

  // 42ul
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_TRUE(result.is_integer_literal());
    EXPECT_EQ(10, result.radix);
    EXPECT_TRUE(result.is_unsigned);
    EXPECT_TRUE(result.is_long);
    EXPECT_FALSE(result.is_long_long);

    const auto[value, overflowed] = result.eval_to_integer();

    ASSERT_FALSE(overflowed);
    EXPECT_EQ(42ull, value);
  }

  // 042
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_TRUE(result.is_integer_literal());
    EXPECT_EQ(8, result.radix);
    EXPECT_FALSE(result.is_unsigned);
    EXPECT_FALSE(result.is_long);
    EXPECT_FALSE(result.is_long_long);

    const auto[value, overflowed] = result.eval_to_integer();

    ASSERT_FALSE(overflowed);
    EXPECT_EQ(34ull, value);
  }

  // 0xDEADc0dellu
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(16, result.radix);
    EXPECT_TRUE(result.is_integer_literal());
    EXPECT_TRUE(result.is_unsigned);
    EXPECT_FALSE(result.is_long);
    EXPECT_TRUE(result.is_long_long);

    const auto[value, overflowed] = result.eval_to_integer();

    ASSERT_FALSE(overflowed);
    EXPECT_EQ(3735929054ull, value);
  }

  // 0uU // error
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_TRUE(result.has_error);
  }

  // 0LLL // error
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_TRUE(result.has_error);
  }

  // 0128 // error
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_TRUE(result.has_error);
  }

  // 314e10
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(10, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_FALSE(result.is_long);
    EXPECT_FALSE(result.is_float);
  }

  // 1.f // ok, floating with period and float suffix
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(10, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_TRUE(result.has_period);
    EXPECT_TRUE(result.is_float);
    EXPECT_FALSE(result.has_exponent);
  }

  // 1.ef // error: empty exponent
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_TRUE(result.has_error);
  }

  // .0 // ok, double
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(10, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_TRUE(result.has_period);
  }

  // 01238.0 // ok, double
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(10, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_TRUE(result.has_period);
  }

  // 0xabcde.ffP+1 // ok, hexadecimal floating constant
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(16, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_TRUE(result.has_period);
    EXPECT_TRUE(result.has_exponent);
  }

  // 0xep1f // ok, hexadecimal floating constant
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(16, result.radix);
    EXPECT_TRUE(result.is_floating_literal());
    EXPECT_FALSE(result.has_period);
    EXPECT_TRUE(result.is_float);
    EXPECT_TRUE(result.has_exponent);
  }

  // 0x.f // error: missing exponent
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_TRUE(result.has_error);
  }

  // 18446744073709551616 // error: overflow
  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(TokenKind::numeric_constant, tok->kind);

    NumericConstantParser result(lexer, tok->spelling(source), tok->location());
    EXPECT_FALSE(result.has_error);
    const auto[value, overflowed] = result.eval_to_integer();
    ASSERT_TRUE(overflowed);
  }
}

TEST(LiteralParserTest, charConstants)
{
  const char *code = R"(
'A'
'\xff'
'\x' // error: empty hex escape
u'\u00A8'
u'\u00A' // error: invalid UCN
)";
  DiagnosticsOptions opts;
  CompilerDiagnostics diag(opts);
  auto source = SourceManager::from_buffer(diag, code);
  auto lexer = Lexer(source);
  std::optional<Token> tok;

  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    CharConstantParser result(lexer, tok->spelling(source), tok->location(),
                              tok->kind);
    EXPECT_EQ('A', result.value);
  }

  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    CharConstantParser result(lexer, tok->spelling(source), tok->location(),
                              tok->kind);
    EXPECT_EQ(255, result.value);
  }

  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    CharConstantParser result(lexer, tok->spelling(source), tok->location(),
                              tok->kind);
    EXPECT_TRUE(result.has_error);
  }

  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    CharConstantParser result(lexer, tok->spelling(source), tok->location(),
                              tok->kind);
    EXPECT_EQ(u'\u00A8', result.value);
  }

  {
    tok = lexer.next_token();
    ASSERT_TRUE(tok.has_value());
    CharConstantParser result(lexer, tok->spelling(source), tok->location(),
                              tok->kind);
    EXPECT_TRUE(result.has_error);
  }
}

TEST(LiteralParserTest, stringLiterals)
{
  const char *code = R"(
"small string" " that has become long now";
"good" L" wide strings" " are good";
u8"but this one" " is" L" problematic" L"!";
U""; // empty string
u"\U00010437"; // UTF-16 string
u8"𐐷"; // UTF-8 string
)";
  DiagnosticsOptions opts;
  CompilerDiagnostics diag(opts);
  auto source = SourceManager::from_buffer(diag, code);
  auto lexer = Lexer(source);
  TargetInfo target;
  std::vector<Token> string_toks;
  string_toks.reserve(4);
  std::optional<Token> tok;

  {
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    EXPECT_STREQ("small string that has become long now",
                 str.result_buf.data());
  }

  {
    string_toks.clear();
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    EXPECT_EQ(4, str.char_byte_width);
    EXPECT_EQ(TokenKind::wide_string_literal, str.kind);
    EXPECT_STREQ(L"good wide strings are good",
                 reinterpret_cast<wchar_t *>(str.result_buf.data()));
  }

  {
    string_toks.clear();
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    EXPECT_TRUE(str.has_error);
  }

  {
    string_toks.clear();
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    uni::UTF32 chr = reinterpret_cast<char32_t *>(str.result_buf.data())[0];
    EXPECT_EQ(0ul, chr);
  }

  {
    string_toks.clear();
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    char16_t encoded[] = {0xD801, 0xDC37, 0x0000};
    auto result = reinterpret_cast<char16_t *>(str.result_buf.data());
    for (int i = 0; encoded[i]; ++i)
      EXPECT_EQ(encoded[i], result[i]) << "encoded and result differ at " << i;
  }

  {
    string_toks.clear();
    while ((tok = lexer.next_token()) && tok->is_not(TokenKind::semi))
      string_toks.push_back(*tok);
    StringLiteralParser str(lexer, string_toks, target);
    uni::UTF8 encoded[] = {0xF0, 0x90, 0x90, 0xB7, 0x00};
    uni::UTF8 *result = reinterpret_cast<uni::UTF8 *>(str.result_buf.data());
    for (int i = 0; encoded[i]; ++i)
      EXPECT_EQ(encoded[i], result[i]) << "encoded and result differ at " << i;
  }
}

} // namespace
#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "ident.hpp"
#include "parser.hpp"

using std::function, std::string, std::stringstream, std::unique_ptr;
using namespace zettel;

static function<unique_ptr<Id>(string)> parseNumerical = [](string s) { return Id::parse(s, Id::Type::Numeric); };

TEST_CASE("DefaultParser - input without references produces single content block", "[parser]") {
    stringstream s("this is some text");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* text = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(text != nullptr);
    CHECK(text->text().compare("this is some text") == 0);
}

TEST_CASE("DefaultParser - input with zettel reference produces correct blocks", "[parser]") {
    stringstream s("[this is the reference](zettel:1)");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    ReferenceBlock* block = dynamic_cast<ReferenceBlock*>(result.get());
    REQUIRE(block != nullptr);
    const Reference* ref = &block->reference();
    const ZettelReference* zettelRef = dynamic_cast<const ZettelReference*>(ref);
    REQUIRE(zettelRef != nullptr);
    REQUIRE(zettelRef->refId() == NumericId(1));
}

TEST_CASE("DefaultParser - incomplete reference treated as text block", "[parser]") {
    stringstream s("[incomplete reference](zettel:");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* block = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(block != nullptr);
    REQUIRE(block->text().compare("[incomplete reference](zettel:") == 0);
}

TEST_CASE("DefaultParser - incomplete reference treated as text block (unclosed brackets)", "[parser]") {
    stringstream s("[incomplete");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* block = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(block != nullptr);
    REQUIRE(block->text().compare("[incomplete") == 0);
}

TEST_CASE("DefaultParser - properly handles backslash at end", "[parser]") {
    stringstream s("this is some text\\");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* text = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(text != nullptr);
    CHECK(text->text().compare("this is some text\\") == 0);
}

TEST_CASE("DefaultParser - properly handles backslash at end (when trying to read reference)", "[parser]") {
    stringstream s("[incomplete\\");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* text = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(text != nullptr);
    CHECK(text->text().compare("[incomplete\\") == 0);
}

TEST_CASE("DefaultParser - properly handles backslash at end (when trying to read reference) - 2", "[parser]") {
    stringstream s("[reference](zettel\\");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    CHECK(parser.done());
    TextBlock* text = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(text != nullptr);
    CHECK(text->text().compare("[reference](zettel\\") == 0);
}

TEST_CASE("DefaultParser - input with references and content produces correct blocks", "[parser]") {
    stringstream s("please refer to [this note](zettel:1) for more information.");
    DefaultParser parser(s, parseNumerical);
    unique_ptr<ContentBlock> result = parser.next();
    REQUIRE(result != nullptr);
    TextBlock* textBlock = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(textBlock != nullptr);
    REQUIRE(textBlock->text().compare("please refer to ") == 0);
    REQUIRE(!parser.done());
    result = parser.next();
    REQUIRE(result != nullptr);
    ReferenceBlock* refBlock = dynamic_cast<ReferenceBlock*>(result.get());
    REQUIRE(refBlock != nullptr);
    const Reference* ref = &refBlock->reference();
    const ZettelReference* zettelRef = dynamic_cast<const ZettelReference*>(ref);
    REQUIRE(zettelRef != nullptr);
    REQUIRE(zettelRef->refId() == NumericId(1));
    REQUIRE(!parser.done());
    result = parser.next();
    CHECK(parser.done());
    textBlock = dynamic_cast<TextBlock*>(result.get());
    REQUIRE(textBlock != nullptr);
    REQUIRE(textBlock->text().compare(" for more information.") == 0);
}

TEST_CASE("DefaultParser - throws Parser::Exception when Zettel reference has invalid ID", "[parser]") {
    stringstream s("[reference](zettel:blah)");
    DefaultParser parser(s, parseNumerical);
    REQUIRE_THROWS_AS(parser.next(), Parser::Exception);
}

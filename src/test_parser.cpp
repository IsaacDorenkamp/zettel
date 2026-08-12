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

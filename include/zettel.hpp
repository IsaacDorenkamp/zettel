#pragma once

#include <memory>
#include <string>
#include <vector>

namespace zettel {

class ZettelId {
public:
    virtual std::unique_ptr<char[]> encode() const = 0;
    virtual bool operator==(const ZettelId& other) const = 0;
    virtual std::unique_ptr<ZettelId> clone() const = 0;
};

class ZettelContentBlock {
public:
    ZettelContentBlock();
};

class ZettelContent {
public:
    ZettelContent();
private:
    std::vector<ZettelContentBlock> m_blocks;
};

class Zettel {
public:
    Zettel(
        const ZettelId& id,
        const std::string& title
    );
private:
    std::unique_ptr<ZettelId> m_id;
    std::string m_title;
};

}

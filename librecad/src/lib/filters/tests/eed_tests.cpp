/**
 * EED (Extended Entity Data) unit tests.
 *
 * Targets the new DRW_Variant variants (BINARY type, layer-ref flag),
 * and the RS_Entity drwExtData storage path used by the rs_filterdxfrw
 * import/export bridge.
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "drw_base.h"
#include "rs_line.h"
#include "rs_vector.h"

TEST_CASE("DRW_Variant: BINARY round-trip preserves bytes", "[eed][drw_variant]") {
    std::vector<duint8> bytes{0x00, 0x01, 0x7F, 0x80, 0xFF, 0xDE, 0xAD};
    DRW_Variant v(1004, bytes);

    REQUIRE(v.code() == 1004);
    REQUIRE(v.type() == DRW_Variant::BINARY);
    REQUIRE(v.binary() != nullptr);
    REQUIRE(v.binary()->size() == bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
        REQUIRE((*v.binary())[i] == bytes[i]);
    }
}

TEST_CASE("DRW_Variant: copy ctor preserves BINARY data", "[eed][drw_variant]") {
    std::vector<duint8> bytes{0xAA, 0xBB, 0xCC};
    DRW_Variant src(1004, bytes);
    DRW_Variant dst(src);

    REQUIRE(dst.type() == DRW_Variant::BINARY);
    REQUIRE(dst.binary() != nullptr);
    REQUIRE(dst.binary()->size() == 3);
    // The pointer in the union must point to the *destination's* bdata,
    // not the source's — otherwise we have a use-after-move hazard.
    REQUIRE(dst.binary() != src.binary());
    REQUIRE((*dst.binary())[1] == 0xBB);
}

TEST_CASE("DRW_Variant: layer-ref flag survives copy", "[eed][drw_variant]") {
    DRW_Variant v(1003, std::string{"MyLayer"}, /*isLayerRef=*/true);

    REQUIRE(v.code() == 1003);
    REQUIRE(v.type() == DRW_Variant::STRING);
    REQUIRE(v.isLayerRef());
    REQUIRE(std::string{v.c_str()} == "MyLayer");

    DRW_Variant copy(v);
    REQUIRE(copy.isLayerRef());
    REQUIRE(std::string{copy.c_str()} == "MyLayer");
}

TEST_CASE("DRW_Variant: plain string is not a layer ref", "[eed][drw_variant]") {
    DRW_Variant v(1003, std::string{"NotALayer"});
    REQUIRE_FALSE(v.isLayerRef());
}

TEST_CASE("DRW_Variant: setLayerRefName promotes plain string", "[eed][drw_variant]") {
    DRW_Variant v(1003, std::string{}, /*isLayerRef=*/true);
    v.setLayerRefName("ResolvedLayerName");

    REQUIRE(v.isLayerRef());
    REQUIRE(std::string{v.c_str()} == "ResolvedLayerName");
}

TEST_CASE("RS_Entity: drwExtData storage round-trips", "[eed][rs_entity]") {
    RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
    REQUIRE_FALSE(line.hasDrwExtData());
    REQUIRE(line.getDrwExtData().empty());

    std::vector<std::shared_ptr<DRW_Variant>> ext;
    ext.push_back(std::make_shared<DRW_Variant>(1001, std::string{"ACAD"}));
    ext.push_back(std::make_shared<DRW_Variant>(1000, std::string{"hello"}));
    ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{42}));
    ext.push_back(std::make_shared<DRW_Variant>(1040, 3.14));

    line.setDrwExtData(ext);

    REQUIRE(line.hasDrwExtData());
    const auto& stored = line.getDrwExtData();
    REQUIRE(stored.size() == 4);
    REQUIRE(stored[0]->code() == 1001);
    REQUIRE(std::string{stored[0]->c_str()} == "ACAD");
    REQUIRE(stored[1]->code() == 1000);
    REQUIRE(std::string{stored[1]->c_str()} == "hello");
    REQUIRE(stored[2]->code() == 1070);
    REQUIRE(stored[2]->i_val() == 42);
    REQUIRE(stored[3]->code() == 1040);
    REQUIRE(std::abs(stored[3]->d_val() - 3.14) < 1e-12);
}

TEST_CASE("RS_Entity: copy ctor deep-copies drwExtData", "[eed][rs_entity]") {
    RS_Line src(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
    std::vector<std::shared_ptr<DRW_Variant>> ext;
    ext.push_back(std::make_shared<DRW_Variant>(1001, std::string{"APP"}));
    ext.push_back(std::make_shared<DRW_Variant>(1070, dint32{7}));
    src.setDrwExtData(ext);

    RS_Line copy(src);
    REQUIRE(copy.hasDrwExtData());
    REQUIRE(copy.getDrwExtData().size() == 2);
    REQUIRE(copy.getDrwExtData()[0]->code() == 1001);
    REQUIRE(std::string{copy.getDrwExtData()[0]->c_str()} == "APP");

    // Mutating the source must not affect the copy.
    src.setDrwExtData({});
    REQUIRE_FALSE(src.hasDrwExtData());
    REQUIRE(copy.hasDrwExtData());
    REQUIRE(copy.getDrwExtData().size() == 2);
}

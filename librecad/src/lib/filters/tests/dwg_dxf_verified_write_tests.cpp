/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "libdwgr.h"
#include "libdxfrw.h"
#include "fuzz_null_interface.h"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::string contents(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
struct Fixture {
    std::filesystem::path directory, destination;
    Fixture() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i=0;i<128;++i) {
            const auto candidate=std::filesystem::path(LC_VERIFIED_STAGE_TEST_ROOT)/
                ("verified-native-"+std::to_string(stamp)+"-"+std::to_string(i));
            if (std::filesystem::create_directory(candidate)) {directory=candidate;break;}
        }
        if (directory.empty()) throw std::runtime_error("Cannot create private verified-write fixture");
        destination=directory/"drawing.native";
        std::ofstream original(destination,std::ios::binary);original << "old destination";
        if (!original.good()) throw std::runtime_error("Cannot seed private destination");
    }
    ~Fixture() {std::error_code ignored;std::filesystem::remove_all(directory,ignored);}
    std::size_t files() const {
        return static_cast<std::size_t>(std::distance(
            std::filesystem::directory_iterator(directory),std::filesystem::directory_iterator()));
    }
};
DRW_Line literalLine() {
    DRW_Line line;line.layer="0";line.basePoint={1.125,-2.25,0};line.secPoint={7.5,8.75,0};return line;
}
bool sameLine(const DRW_Line &line) {
    const auto expected=literalLine();
    return line.eType==DRW::LINE && line.layer==expected.layer &&
        line.basePoint.x==expected.basePoint.x && line.basePoint.y==expected.basePoint.y &&
        line.basePoint.z==expected.basePoint.z && line.secPoint.x==expected.secPoint.x &&
        line.secPoint.y==expected.secPoint.y && line.secPoint.z==expected.secPoint.z;
}
struct Capture final : FuzzNullInterface {
    std::vector<DRW_Line> lines;
    void addLine(const DRW_Line &line) override {lines.push_back(line);}
};
template<class Writer> struct Output final : FuzzNullInterface {
    Writer &writer;
    bool emitted=false,throwing=false,acceptTraversal=true;
    explicit Output(Writer &w):writer(w) {}
    bool finalizeDxfWrite() override { return acceptTraversal; }
    void writeEntities() override {
        if (throwing) throw std::runtime_error("simulated native emission failure");
        auto line=literalLine();emitted=writer.writeLine(&line);
    }
};
enum class Disposition {Accept,Reject,Throw};
struct Verification {
    std::filesystem::path destination,observed;
    DRW::Version version=DRW::UNKNOWNV;
    bool dwg=false,binary=false;
    Disposition disposition=Disposition::Accept;
    unsigned calls=0;
    bool adjacent=false,oldDestination=false,parsed=false,exactLine=false,exactMode=false;
    static bool inspect(const std::filesystem::path &path,void *context) {
        auto &self=*static_cast<Verification *>(context);++self.calls;self.observed=path;
        self.adjacent=path!=self.destination && path.parent_path()==self.destination.parent_path();
        self.oldDestination=contents(self.destination)=="old destination";
        Capture capture;const auto nativePath=path.string();
        if (self.dwg) {
            dwgRW reader(nativePath.c_str());self.parsed=reader.read(&capture,false) && reader.getVersion()==self.version;
            const auto bytes=contents(path);
            self.exactMode=bytes.size()>=6 && bytes.substr(0,6)==(self.version==DRW::AC1027?"AC1027":"AC1032");
        } else {
            dxfRW reader(nativePath.c_str());self.parsed=reader.read(&capture,false) && reader.getSourceVersion()==self.version;
            const auto bytes=contents(path);
            const std::string signature("AutoCAD Binary DXF\r\n\x1a\0",22);
            self.exactMode=(bytes.compare(0,signature.size(),signature)==0)==self.binary;
        }
        self.exactLine=capture.lines.size()==1 && sameLine(capture.lines.front());
        if (self.disposition==Disposition::Throw) throw std::runtime_error("simulated stage reread refusal");
        // Deliberately bounded oracle: one actual native LINE, not a complete
        // carrier/STYLE graph, whole drawing fidelity or application authority.
        return self.disposition==Disposition::Accept && self.adjacent && self.oldDestination &&
            self.parsed && self.exactLine && self.exactMode;
    }
};
template<class Writer>
void dispositionCase(DRW::Version version,bool dwg,bool binary,Disposition disposition) {
    Fixture fixture;const auto path=fixture.destination.string();Writer writer(path.c_str());Output<Writer> output(writer);
    Verification verifier;verifier.destination=fixture.destination;verifier.version=version;
    verifier.dwg=dwg;verifier.binary=binary;verifier.disposition=disposition;
    const bool accepted=writer.writeVerified(&output,version,binary,&Verification::inspect,&verifier);
    CHECK(accepted==(disposition==Disposition::Accept));CHECK(output.emitted);
    CHECK(verifier.calls==1);CHECK(verifier.adjacent);CHECK(verifier.oldDestination);
    CHECK(verifier.parsed);CHECK(verifier.exactLine);CHECK(verifier.exactMode);
    CHECK_FALSE(std::filesystem::exists(verifier.observed));CHECK(fixture.files()==1);
    if (accepted) CHECK(contents(fixture.destination)!="old destination");
    else {CHECK(contents(fixture.destination)=="old destination");CHECK(writer.getError()==DRW::BAD_OPEN);}
}
template<class Writer>
void retryCase(bool dwg,bool binary) {
    Fixture fixture;const auto path=fixture.destination.string();Writer writer(path.c_str());Output<Writer> output(writer);
    Verification first;first.destination=fixture.destination;first.version=DRW::AC1032;
    first.dwg=dwg;first.binary=binary;first.disposition=Disposition::Reject;
    REQUIRE_FALSE(writer.writeVerified(&output,DRW::AC1032,binary,&Verification::inspect,&first));
    REQUIRE(first.calls==1);REQUIRE(contents(fixture.destination)=="old destination");
    Verification second;second.destination=fixture.destination;second.version=DRW::AC1032;
    second.dwg=dwg;second.binary=binary;
    REQUIRE(writer.writeVerified(&output,DRW::AC1032,binary,&Verification::inspect,&second));
    CHECK(first.calls==1);CHECK(second.calls==1);CHECK(second.parsed);CHECK(second.exactLine);CHECK(second.exactMode);
    REQUIRE(writer.write(&output,DRW::AC1032,binary));
    CHECK(first.calls==1);CHECK(second.calls==1);CHECK(fixture.files()==1);
    CHECK_FALSE(std::filesystem::exists(first.observed));CHECK_FALSE(std::filesystem::exists(second.observed));
}
template<class Writer>
void invalidCase(bool binary) {
    Fixture fixture;const auto path=fixture.destination.string();Writer writer(path.c_str());Output<Writer> output(writer);
    Verification verifier;
    CHECK_FALSE(writer.writeVerified(&output,DRW::AC1032,binary,nullptr,nullptr));
    CHECK_FALSE(writer.writeVerified(nullptr,DRW::AC1032,binary,&Verification::inspect,&verifier));
    CHECK_FALSE(writer.writeVerified(&output,DRW::UNKNOWNV,binary,&Verification::inspect,&verifier));
    output.throwing=true;
    CHECK_FALSE(writer.writeVerified(&output,DRW::AC1032,binary,&Verification::inspect,&verifier));
    CHECK(verifier.calls==0);CHECK_FALSE(output.emitted);CHECK(contents(fixture.destination)=="old destination");
    CHECK(fixture.files()==1);
}
}

TEST_CASE("Native verified writes inspect real stages before accepting rejecting or throwing","[verified-native-write]") {
    for (const auto version:{DRW::AC1027,DRW::AC1032})
        for (const auto disposition:{Disposition::Accept,Disposition::Reject,Disposition::Throw})
            for (const unsigned format:{0u,1u,2u}) {
                CAPTURE(int(version),int(disposition),format);
                if (format==0) dispositionCase<dwgRW>(version,true,false,disposition);
                else dispositionCase<dxfRW>(version,false,format==2,disposition);
            }
}
TEST_CASE("Native verified retries and ordinary writes do not retain callback contexts","[verified-native-write][retry]") {
    retryCase<dwgRW>(true,false);retryCase<dxfRW>(false,false);retryCase<dxfRW>(false,true);
}
TEST_CASE("Native invalid or throwing emissions never verify or replace a prior file","[verified-native-write][invalid]") {
    invalidCase<dwgRW>(false);invalidCase<dxfRW>(false);invalidCase<dxfRW>(true);
}

TEST_CASE("DXF traversal veto precedes verification and leaves the destination intact", "[verified-native-write][veto]") {
    for (const bool binary : {false, true}) {
        Fixture fixture;
        const auto path = fixture.destination.string();
        dxfRW writer(path.c_str());
        Output<dxfRW> output(writer);
        output.acceptTraversal = false;
        Verification verifier;
        verifier.destination = fixture.destination;
        verifier.version = DRW::AC1032;
        verifier.binary = binary;
        CHECK_FALSE(writer.writeVerified(&output, DRW::AC1032, binary, &Verification::inspect, &verifier));
        CHECK(output.emitted);
        CHECK(verifier.calls == 0);
        CHECK(contents(fixture.destination) == "old destination");
        CHECK(fixture.files() == 1);
        CHECK_FALSE(writer.write(&output, DRW::AC1032, binary));
        CHECK(contents(fixture.destination) == "old destination");
        output.acceptTraversal = true;
        REQUIRE(writer.writeVerified(&output, DRW::AC1032, binary, &Verification::inspect, &verifier));
        CHECK(verifier.calls == 1);
        CHECK(fixture.files() == 1);
    }
}

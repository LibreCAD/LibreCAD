/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "intern/dwg_dxf_output_transaction.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iterator>
#include <stdexcept>

namespace {
std::string read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

struct Fixture {
    std::filesystem::path directory, target;
    Fixture() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i=0;i<128;++i) {
            directory=std::filesystem::path(LC_VERIFIED_STAGE_TEST_ROOT)/
                ("owned-stage-test-"+std::to_string(stamp)+"-"+std::to_string(i));
            if (std::filesystem::create_directory(directory)) break;
            directory.clear();
        }
        if (directory.empty()) throw std::runtime_error("Could not create private fixture");
        target=directory/"drawing.dwg";
        std::ofstream original(target,std::ios::binary);
        original << "old destination";
        if (!original.good()) throw std::runtime_error("Could not write private fixture");
    }
    ~Fixture() {
        std::error_code ignored;
        std::filesystem::remove_all(directory,ignored);
    }
    std::size_t files() const {
        return static_cast<std::size_t>(std::distance(
            std::filesystem::directory_iterator(directory),std::filesystem::directory_iterator()));
    }
};

struct Verification {
    DwgDxfOutputTransaction *output;
    std::filesystem::path target, observed;
    unsigned calls=0;
    bool allow=true, throwing=false, reenter=false, reenterVerified=false;
    bool closed=false, exactStage=false, oldTarget=false, adjacent=false;
    bool openRefused=false, commitRefused=false;
    bool nullVerifierRefused=false, nestedVerifierRefused=false;
    static bool inspect(const std::filesystem::path &path,void *opaque) {
        auto &self=*static_cast<Verification *>(opaque);
        ++self.calls;self.observed=path;
        self.closed=!self.output->isOpen();
        self.exactStage=std::filesystem::is_regular_file(path)&&read(path)=="new complete drawing";
        self.oldTarget=std::filesystem::is_regular_file(self.target)&&read(self.target)=="old destination";
        self.adjacent=path.parent_path()==self.target.parent_path()&&path!=self.target;
        if (self.reenter) {
            self.openRefused=!self.output->open();
            self.commitRefused=!self.output->commit();
        }
        if (self.reenterVerified) {
            self.nestedVerifierRefused=!self.output->commitVerified(&Verification::inspect,nullptr);
            self.nullVerifierRefused=!self.output->commitVerified(nullptr,nullptr);
        }
        if (self.throwing) throw std::runtime_error("simulated reread failure");
        return self.allow&&self.closed&&self.exactStage&&self.adjacent;
    }
};

void write(DwgDxfOutputTransaction &output) {
    REQUIRE(output.open());
    output.stream() << "new complete drawing";
    REQUIRE(output.stream().good());
}
}

TEST_CASE("Verified publication inspects the closed adjacent stage before replacing the destination","[stage][accept]") {
    Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);Verification verifier{&output,fixture.target,{}};
    REQUIRE(output.commitVerified(&Verification::inspect,&verifier));
    CHECK(verifier.calls==1);CHECK(verifier.closed);CHECK(verifier.exactStage);
    CHECK(verifier.oldTarget);CHECK(verifier.adjacent);
    CHECK(read(fixture.target)=="new complete drawing");
    CHECK_FALSE(std::filesystem::exists(verifier.observed));CHECK(fixture.files()==1);
    CHECK_FALSE(output.open());CHECK_FALSE(output.commit());
    CHECK_FALSE(output.commitVerified(&Verification::inspect,&verifier));CHECK(verifier.calls==1);
}

TEST_CASE("Rejected and throwing stage verifiers preserve the original destination","[stage][reject]") {
    for (const bool throwing:{false,true}) {
        Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
        write(output);Verification verifier{&output,fixture.target,{}};
        verifier.allow=false;verifier.throwing=throwing;
        CHECK_FALSE(output.commitVerified(&Verification::inspect,&verifier));
        CHECK(verifier.calls==1);CHECK(verifier.closed);CHECK(verifier.exactStage);CHECK(verifier.oldTarget);
        CHECK(read(fixture.target)=="old destination");
        CHECK_FALSE(std::filesystem::exists(verifier.observed));CHECK(fixture.files()==1);
        CHECK_FALSE(output.open());CHECK_FALSE(output.commit());
    }
}

TEST_CASE("A failed stream never reaches verification or publication","[stage][stream]") {
    Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);Verification verifier{&output,fixture.target,{}};
    output.stream().setstate(std::ios::badbit);
    CHECK_FALSE(output.commitVerified(&Verification::inspect,&verifier));CHECK(verifier.calls==0);
    CHECK_FALSE(output.isOpen());CHECK(read(fixture.target)=="old destination");CHECK(fixture.files()==1);
}

TEST_CASE("Missing verification and unopened transactions cannot publish","[stage][invalid]") {
    Fixture fixture;
    {
        DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
        Verification verifier{&output,fixture.target,{}};
        CHECK_FALSE(output.commitVerified(&Verification::inspect,&verifier));CHECK(verifier.calls==0);
        CHECK_FALSE(output.commit());write(output);
        CHECK_FALSE(output.commitVerified(nullptr,nullptr));CHECK_FALSE(output.isOpen());
    }
    CHECK(read(fixture.target)=="old destination");CHECK(fixture.files()==1);
}

TEST_CASE("A verifier cannot reenter transaction publication or reopen its stage","[stage][reentry]") {
    Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);Verification verifier{&output,fixture.target,{}};verifier.reenter=true;
    REQUIRE(output.commitVerified(&Verification::inspect,&verifier));
    CHECK(verifier.calls==1);CHECK(verifier.openRefused);CHECK(verifier.commitRefused);
    CHECK(read(fixture.target)=="new complete drawing");CHECK(fixture.files()==1);
}

TEST_CASE("Atomic replacement failure removes only the private stage","[stage][publish-failure]") {
    Fixture fixture;std::filesystem::remove(fixture.target);std::filesystem::create_directory(fixture.target);
    DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);Verification verifier{&output,fixture.target,{}};
    CHECK_FALSE(output.commitVerified(&Verification::inspect,&verifier));CHECK(verifier.calls==1);
    CHECK(verifier.exactStage);CHECK_FALSE(verifier.oldTarget);
    CHECK(std::filesystem::is_directory(fixture.target));CHECK(std::filesystem::is_empty(fixture.target));
    CHECK_FALSE(std::filesystem::exists(verifier.observed));CHECK(fixture.files()==1);
}

TEST_CASE("Nested verified commits refuse without deleting the active outer stage","[stage][verified-reentry]") {
    Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);Verification verifier{&output,fixture.target,{}};verifier.reenterVerified=true;
    CHECK(output.commitVerified(&Verification::inspect,&verifier));
    CHECK(verifier.calls==1);CHECK(verifier.nestedVerifierRefused);CHECK(verifier.nullVerifierRefused);
    CHECK(read(fixture.target)=="new complete drawing");CHECK(fixture.files()==1);
}

TEST_CASE("Unchecked commit keeps its existing flush-and-replace behavior","[stage][legacy]") {
    Fixture fixture;DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);
    write(output);REQUIRE(output.commit());
    CHECK(read(fixture.target)=="new complete drawing");CHECK(fixture.files()==1);
    CHECK_FALSE(output.open());CHECK_FALSE(output.commit());
}

TEST_CASE("An abandoned owned stage leaves the destination unchanged","[stage][lifetime]") {
    Fixture fixture;
    {DwgDxfOutputTransaction output(fixture.target.string(),std::ios::binary);write(output);CHECK(fixture.files()==2);}
    CHECK(read(fixture.target)=="old destination");CHECK(fixture.files()==1);
}

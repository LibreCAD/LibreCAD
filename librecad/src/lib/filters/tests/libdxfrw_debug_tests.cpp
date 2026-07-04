#include <catch2/catch_test_macros.hpp>

#include <string>

#include "drw_base.h"
#include "intern/drw_dbg.h"

namespace {

class CountingDebugPrinter : public DRW::DebugPrinter {
public:
    int stringCalls = 0;
    int intCalls = 0;
    int uintCalls = 0;
    int doubleCalls = 0;
    int hexCalls = 0;
    int binaryCalls = 0;
    int handleCalls = 0;
    int pointCalls = 0;
    std::string text;

    int totalCalls() const {
        return stringCalls + intCalls + uintCalls + doubleCalls + hexCalls
            + binaryCalls + handleCalls + pointCalls;
    }

    void printS(const std::string& s) override {
        ++stringCalls;
        text += s;
    }

    void printI(long long int) override {
        ++intCalls;
    }

    void printUI(long long unsigned int) override {
        ++uintCalls;
    }

    void printD(double) override {
        ++doubleCalls;
    }

    void printH(long long int) override {
        ++hexCalls;
    }

    void printB(int) override {
        ++binaryCalls;
    }

    void printHL(int, int, int) override {
        ++handleCalls;
    }

    void printPT(double, double, double) override {
        ++pointCalls;
    }
};

class DebugStateReset {
public:
    ~DebugStateReset() {
        DRW_DBGSL(DRW_dbg::Level::None);
        DRW::setCustomDebugPrinter(new DRW::DebugPrinter());
    }
};

} // namespace

TEST_CASE("libdxfrw debug macros skip printer work until enabled",
          "[libdxfrw][debug]") {
    DebugStateReset reset;
    auto* printer = new CountingDebugPrinter();
    DRW::setCustomDebugPrinter(printer);
    DRW_DBGSL(DRW_dbg::Level::None);

    int evaluated = 0;
    DRW_DBG("disabled");
    DRW_DBG(++evaluated);
    DRW_DBGH(++evaluated);
    DRW_DBGB(++evaluated);
    DRW_DBGHL(++evaluated, ++evaluated, ++evaluated);
    DRW_DBGPT(++evaluated, ++evaluated, ++evaluated);

    CHECK(evaluated == 9);
    CHECK(printer->totalCalls() == 0);

    DRW_DBGSL(DRW_dbg::Level::Debug);

    DRW_DBG("enabled");
    DRW_DBG(++evaluated);
    DRW_DBGH(++evaluated);
    DRW_DBGB(++evaluated);
    DRW_DBGHL(++evaluated, ++evaluated, ++evaluated);
    DRW_DBGPT(++evaluated, ++evaluated, ++evaluated);

    CHECK(evaluated == 18);
    CHECK(printer->stringCalls == 1);
    CHECK(printer->intCalls == 1);
    CHECK(printer->hexCalls == 1);
    CHECK(printer->binaryCalls == 1);
    CHECK(printer->handleCalls == 1);
    CHECK(printer->pointCalls == 1);
    CHECK(printer->totalCalls() == 6);
    CHECK(printer->text == "enabled");
}

// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Signs of engineering, architectural and fractional lengths (#3281, #3290):
// a negative length keeps its minus sign, unless it rounds to zero, and
// inches that round to 12 carry into the feet.

#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <QtTest>

#include "rs_units.h"

namespace {
std::string eng(double length, int prec = 2) {
    return RS_Units::formatEngineering(length, RS2::Inch, prec, false).toStdString();
}
std::string arch(double length, RS2::Unit unit = RS2::Inch, int prec = 1) {
    return RS_Units::formatArchitectural(length, unit, prec, false).toStdString();
}
std::string frac(double length, int prec = 2) {
    return RS_Units::formatFractional(length, RS2::Inch, prec, false).toStdString();
}
}

class LC_UnitsFormatTests : public QObject {
    Q_OBJECT

private slots:
    void architecturalKeepsTheSignBelowOneFoot() {
        QCOMPARE(arch(-6.0), std::string("-6\""));
        QCOMPARE(arch(-0.25), std::string("-1/4\""));
        QCOMPARE(arch(-18.0), std::string("-1'-6\""));
        QCOMPARE(arch(-0.5, RS2::Foot), std::string("-6\""));
        QCOMPARE(arch(6.0), std::string("6\""));
        // rounds to zero, so no sign
        QCOMPARE(arch(-0.01), std::string("0\""));
    }

    void engineeringAndFractionalDropTheSignOfZero() {
        QCOMPARE(eng(-0.001), std::string("0.00\""));
        QCOMPARE(eng(-0.0), std::string("0.00\""));
        QCOMPARE(eng(-0.4), std::string("-0.40\""));
        QCOMPARE(eng(-18.0), std::string("-1'-6.00\""));
        QCOMPARE(frac(-0.001), std::string("0"));
        QCOMPARE(frac(-0.4), std::string("- 3/8"));
        QCOMPARE(frac(-1.5), std::string("- 1 1/2"));
    }

    void engineeringCarriesTwelveInchesIntoTheFeet() {
        QCOMPARE(eng(23.999), std::string("2'-0.00\""));
        QCOMPARE(eng(-23.999), std::string("-2'-0.00\""));
        QCOMPARE(eng(11.9999), std::string("1'-0.00\""));
        QCOMPARE(eng(23.999, 0), std::string("2'-0\""));
    }

    // A negative length shows a minus sign exactly when its text has a
    // non-zero digit, and reads the same as the positive length otherwise.
    void signFollowsTheShownDigits() {
        using Formatter = QString (*)(double, RS2::Unit, int, bool);
        const std::pair<const char*, Formatter> formats[] = {
            {"engineering", &RS_Units::formatEngineering},
            {"architectural", &RS_Units::formatArchitectural},
            {"fractional", &RS_Units::formatFractional},
        };
        int checked = 0;
        for (const auto& [name, format] : formats) {
            for (int prec = 0; prec <= 8; ++prec) {
                const double step = (format == &RS_Units::formatEngineering)
                    ? std::pow(10., -prec) : 1. / (2 << prec);
                std::vector<double> lengths{-0., -std::numeric_limits<double>::denorm_min()};
                for (const double base : {0.5 * step, step, 1.5 * step, 12. - 0.5 * step, 12., 24. - 0.5 * step}) {
                    for (const double eps : {0., 1e-12, -1e-12}) {
                        const double x = base + eps;
                        lengths.push_back(-x);
                        lengths.push_back(-std::nextafter(x, 0.));
                        lengths.push_back(-std::nextafter(x, 100.));
                    }
                }
                for (int i = 1; i <= 2000; ++i) {
                    lengths.push_back(-i * 0.0137);
                }
                for (const double length : lengths) {
                    const std::string text = format(length, RS2::Inch, prec, false).toStdString();
                    const std::string positive = format(-length, RS2::Inch, prec, false).toStdString();
                    const QByteArray where = QString("%1 prec %2 length %3: \"%4\" vs \"%5\"")
                        .arg(name).arg(prec).arg(length, 0, 'g', 17)
                        .arg(QString::fromStdString(text), QString::fromStdString(positive)).toUtf8();
                    const bool nonZero = text.find_first_of("123456789") != std::string::npos;
                    QVERIFY2((text.front() == '-') == nonZero, where.constData());
                    QVERIFY2(positive.front() != '-', where.constData());
                    const std::string unsignedText = text.front() != '-' ? text
                        : text.substr(text.compare(0, 2, "- ") == 0 ? 2 : 1);
                    QVERIFY2(unsignedText == positive, where.constData());
                    if (format != &RS_Units::formatFractional) {
                        // never 12", 12.00" or 12 1/2"
                        const std::size_t dash = unsignedText.find("'-");
                        const std::string inches = unsignedText.substr(dash == std::string::npos ? 0 : dash + 2);
                        const bool wholeTwelve = inches.rfind("12", 0) == 0 && inches.size() > 2
                            && (inches[2] == '"' || inches[2] == '.' || inches[2] == ' ');
                        QVERIFY2(!wholeTwelve, where.constData());
                    }
                    ++checked;
                }
            }
        }
        QVERIFY(checked > 50000);
    }
};

QTEST_GUILESS_MAIN(LC_UnitsFormatTests)
#include "lc_units_format_tests.moc"

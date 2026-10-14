// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LC_SHXSHAPENAMES_H
#define LC_SHXSHAPENAMES_H

#include <QByteArray>
#include <QFile>
#include <QString>

#include <cstdint>
#include <map>
#include <optional>
#include <string>

// Indexed AutoCAD-86 shape files store NUL-terminated names before each
// shape's bytecode. Read only that directory; drawing the bytecode is separate.
class LC_ShxShapeNames {
public:
  using Names = std::map<std::uint16_t, std::string>;

  static std::optional<Names> read(const QString &path) {
    QFile file(path);
    constexpr qint64 maxBytes = 16 * 1024 * 1024;
    if (!file.open(QIODevice::ReadOnly) || file.size() > maxBytes)
      return std::nullopt;
    const QByteArray bytes = file.read(maxBytes + 1);
    if (file.error() != QFileDevice::NoError || !file.atEnd())
      return std::nullopt;
    return parse(bytes);
  }

  static std::optional<Names> parse(const QByteArray &bytes) {
    if (!bytes.startsWith("AutoCAD-86 shapes 1.0\r\n\x1a") &&
        !bytes.startsWith("AutoCAD-86 shapes 1.1\r\n\x1a"))
      return std::nullopt;
    if (bytes.size() < 30)
      return std::nullopt;
    const auto word = [&bytes](qsizetype offset) {
      return static_cast<std::uint16_t>(
          static_cast<unsigned char>(bytes[offset]) |
          (static_cast<unsigned char>(bytes[offset + 1]) << 8));
    };
    const std::uint16_t first = word(24);
    const std::uint16_t last = word(26);
    const std::uint16_t count = word(28);
    qsizetype offset = 30 + static_cast<qsizetype>(count) * 4;
    if (count == 0 || offset > bytes.size() || word(30) != first ||
        word(30 + (count - 1) * 4) != last)
      return std::nullopt;
    Names names;
    for (std::uint32_t i = 0; i < count; ++i) {
      const std::uint16_t index = word(30 + i * 4);
      const std::uint16_t length = word(32 + i * 4);
      if (length < 2 || length > bytes.size() - offset)
        return std::nullopt;
      const qsizetype end = offset + length;
      const qsizetype terminator = bytes.indexOf('\0', offset);
      if (terminator < offset || terminator >= end - 1 ||
          bytes[end - 1] != '\0' || terminator - offset > 255)
        return std::nullopt;
      const QByteArray name = bytes.mid(offset, terminator - offset);
      for (unsigned char ch : name) {
        if (ch < 0x20 || ch > 0x7e)
          return std::nullopt;
      }
      if (!names.emplace(index, name.toStdString()).second)
        return std::nullopt;
      offset = end;
    }
    if (bytes.mid(offset, 3) != "EOF" || offset + 3 != bytes.size())
      return std::nullopt;
    return names;
  }
};

#endif

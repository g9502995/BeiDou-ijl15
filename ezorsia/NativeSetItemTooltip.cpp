#include "stdafx.h"
#include "NativeSetItemTooltip.h"
#include "MapleClientCollectionTypes/ZXString.h"
#include <vector>

namespace {

// Fill this with the v83 CUIToolTip::_AddInfoEx address from IDA/reference code.
constexpr DWORD kCUIToolTipAddInfoEx = 0x00000000;

enum ToolTipColorType {
    kToolTipColorDefault = 0,
    kToolTipColorWhite = 1,
    kToolTipColorGray = 2,
    kToolTipColorOrange = 3,
    kToolTipColorGreen = 4,
    kToolTipColorBlue = 5,
};

struct NativeSetItemLine {
    const char* text;
    ToolTipColorType color;
};

typedef void(__fastcall* CUIToolTip_AddInfoEx_Type)(
    void* pToolTip,
    void* edx,
    ZXString<char>* text,
    int colorType,
    int arg8,
    int argC,
    int arg10,
    int arg14);

CUIToolTip_AddInfoEx_Type GetAddInfoEx()
{
    if (kCUIToolTipAddInfoEx == 0) {
        return nullptr;
    }

    return reinterpret_cast<CUIToolTip_AddInfoEx_Type>(kCUIToolTipAddInfoEx);
}

void AddInfoLine(void* pToolTip, const char* text, ToolTipColorType color)
{
    CUIToolTip_AddInfoEx_Type addInfoEx = GetAddInfoEx();
    if (!pToolTip || !text || !addInfoEx) {
        return;
    }

    ZXString<char> line(text);
    addInfoEx(pToolTip, nullptr, &line, static_cast<int>(color), 0, 0, 0, 0);
}

std::vector<NativeSetItemLine> BuildTestSetLines(void* pEquip)
{
    UNREFERENCED_PARAMETER(pEquip);

    std::vector<NativeSetItemLine> lines;
    lines.push_back({ "", kToolTipColorDefault });
    lines.push_back({ "Test Set  (2/5)", kToolTipColorBlue });
    lines.push_back({ "  Test Helmet", kToolTipColorWhite });
    lines.push_back({ "  Test Armor", kToolTipColorWhite });
    lines.push_back({ "  Test Boots", kToolTipColorGray });
    lines.push_back({ "  Test Gloves", kToolTipColorGray });
    lines.push_back({ "  Test Cape", kToolTipColorGray });
    lines.push_back({ "----------------------", kToolTipColorGray });
    lines.push_back({ "2 Set Effect:", kToolTipColorOrange });
    lines.push_back({ "  *DEF: +20", kToolTipColorGreen });
    lines.push_back({ "  *MDEF: +5", kToolTipColorGreen });
    lines.push_back({ "4 Set Effect:", kToolTipColorGray });
    lines.push_back({ "  *STR: +3", kToolTipColorGray });
    lines.push_back({ "  *DEX: +3", kToolTipColorGray });
    return lines;
}

}

void AppendNativeSetItemTooltip(void* pToolTip, void* pEquip)
{
    if (!pToolTip || !pEquip) {
        return;
    }

    const std::vector<NativeSetItemLine> lines = BuildTestSetLines(pEquip);
    for (const NativeSetItemLine& line : lines) {
        AddInfoLine(pToolTip, line.text, line.color);
    }
}

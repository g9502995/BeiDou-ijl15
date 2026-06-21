#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "SetItemPanel.h"
#include "vendor/imgui/imgui.h"
#include "MapleClientCollectionTypes/TSecType.h"
#include <mutex>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <set>

static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    // ImGui STRICTLY REQUIRES UTF-8. DO NOT change this to CP_ACP or 936 (GB2312)!
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::string TranslateStatName(const std::string& statName) {
    if (statName == "incSTR") return WStringToString(L"\u529b\u91cf"); // 力量
    if (statName == "incDEX") return WStringToString(L"\u654f\u6377"); // 敏捷
    if (statName == "incINT") return WStringToString(L"\u667a\u529b"); // 智力
    if (statName == "incLUK") return WStringToString(L"\u5e78\u8fd0"); // 幸运
    if (statName == "incMHP") return WStringToString(L"HP");
    if (statName == "incMMP") return WStringToString(L"MP");
    if (statName == "incPAD") return WStringToString(L"\u653b\u51fb\u529b"); // 攻击力
    if (statName == "incMAD") return WStringToString(L"\u9b54\u6cd5\u653b\u51fb\u529b"); // 魔法攻击力
    if (statName == "incPDD") return WStringToString(L"\u9632\u5fa1\u529b"); // 防御力
    if (statName == "incMDD") return WStringToString(L"\u9b54\u6cd5\u9632\u5fa1\u529b"); // 魔法防御力
    if (statName == "incACC") return WStringToString(L"\u547d\u4e2d\u7387"); // 命中率
    if (statName == "incEVA") return WStringToString(L"\u56de\u907f\u7387"); // 回避率
    if (statName == "incSpeed") return WStringToString(L"\u79fb\u52a8\u901f\u5ea6"); // 移动速度
    if (statName == "incJump") return WStringToString(L"\u8df3\u8dc3\u529b"); // 跳跃力
    if (statName == "incCraft") return WStringToString(L"\u624b\u6280"); // 手技
    return statName;
}

static std::wstring GetItemCategoryW(int itemID) {
    int type = itemID / 10000;
    switch (type) {
        case 100: return L"Cap";
        case 101:
        case 102:
        case 103: return L"Accessory";
        case 104: return L"Coat";
        case 105: return L"Longcoat";
        case 106: return L"Pants";
        case 107: return L"Shoes";
        case 108: return L"Glove";
        case 109: return L"Shield";
        case 110: return L"Cape";
        case 111: return L"Ring";
        case 112: return L"Necklace";
        case 113: return L"Belt";
        case 115: return L"Shoulder";
        default:
            if (type >= 120 && type <= 170) return L"Weapon";
            return L"";
    }
}

static std::wstring GetItemNameW(int itemID) {
    void** ppResMan = reinterpret_cast<void**>(0x00BF14E8);
    if (!ppResMan || !*ppResMan) return L"";
    IWzResMan* pResMan = reinterpret_cast<IWzResMan*>(*ppResMan);

    Ztl_variant_t vEmpty, vEqpStr;
    if (pResMan->raw_GetObject(L"String/Eqp.img", vEmpty, vEmpty, &vEqpStr) >= 0) {
        IWzProperty* pEqpStr = reinterpret_cast<IWzProperty*>(vEqpStr.GetUnknown(false, false));
        if (pEqpStr != NULL) {
            Ztl_variant_t vEqpNode;
            if (pEqpStr->get_item(L"Eqp", &vEqpNode) >= 0) {
                IWzProperty* pEqpNode = reinterpret_cast<IWzProperty*>(vEqpNode.GetUnknown(false, false));
                if (pEqpNode != NULL) {
                    std::wstring category = GetItemCategoryW(itemID);
                    if (!category.empty()) {
                        Ztl_variant_t vCategoryNode;
                        if (pEqpNode->get_item(category.c_str(), &vCategoryNode) >= 0) {
                            IWzProperty* pCategoryNode = reinterpret_cast<IWzProperty*>(vCategoryNode.GetUnknown(false, false));
                            if (pCategoryNode != NULL) {
                                wchar_t szItemID[32];
                                swprintf_s(szItemID, L"%d", itemID);
                                Ztl_variant_t vItemInfo;
                                if (pCategoryNode->get_item(szItemID, &vItemInfo) >= 0) {
                                    IWzProperty* pItemInfo = reinterpret_cast<IWzProperty*>(vItemInfo.GetUnknown(false, false));
                                    if (pItemInfo != NULL) {
                                        Ztl_variant_t vName;
                                        if (pItemInfo->get_item(L"name", &vName) >= 0 && vName.vt == VT_BSTR) {
                                            return vName.bstrVal;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return L"";
}

static std::map<int, std::wstring> g_CachedItemNames;

static std::string GetCachedItemName(int itemID) {
    if (g_CachedItemNames.find(itemID) == g_CachedItemNames.end()) {
        std::wstring wName = GetItemNameW(itemID);
        if (wName.empty()) {
            wName = L"???";
        }
        g_CachedItemNames[itemID] = wName;
    }
    return WStringToString(g_CachedItemNames[itemID]);
}

static std::string GetCategoryChinese(int itemID) {
    int type = itemID / 10000;
    switch (type) {
        case 100: return WStringToString(L"\u5e3d\u5b50"); // 帽子
        case 101:
        case 102:
        case 103: return WStringToString(L"\u9970\u54c1"); // 饰品
        case 104: return WStringToString(L"\u4e0a\u8863"); // 上衣
        case 105: return WStringToString(L"\u5957\u670d"); // 套服
        case 106: return WStringToString(L"\u88e4/\u88d9"); // 裤/裙
        case 107: return WStringToString(L"\u978b\u5b50"); // 鞋子
        case 108: return WStringToString(L"\u624b\u5957"); // 手套
        case 109: return WStringToString(L"\u76fe\u724c"); // 盾牌
        case 110: return WStringToString(L"\u62ab\u98ce"); // 披风
        case 111: return WStringToString(L"\u6212\u6307"); // 戒指
        case 112: return WStringToString(L"\u5760\u9970"); // 坠饰
        case 113: return WStringToString(L"\u8170\u5e26"); // 腰带
        case 114: return WStringToString(L"\u52cb\u7ae0"); // 勋章
        case 115: return WStringToString(L"\u80a9\u8180"); // 肩膀
        default:
            if (type >= 120 && type <= 170) return WStringToString(L"\u6b66\u5668"); // 武器
            return WStringToString(L"\u88c5\u5907"); // 装备
    }
}

std::mutex g_SetPanelMutex;
SetPanelData g_SetPanelData = {};

std::map<int, SetInfo> g_SetInfos;
std::map<int, int> g_ItemToSetID;
std::mutex g_SetItemMutex;

static void ParseItemIDsRecursively(IWzProperty* pPropNode, std::vector<int>& itemIDs, std::map<int, int>& itemToSetID, int setID) {
    if (pPropNode == NULL) return;
    IUnknown* pEnumUnknown = nullptr;
    if (pPropNode->get__NewEnum(&pEnumUnknown) >= 0 && pEnumUnknown != nullptr) {
        IEnumVARIANT* pEnum = nullptr;
        if (pEnumUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnum) >= 0 && pEnum != nullptr) {
            while (true) {
                VARIANT rgVar[1];
                VariantInit(&rgVar[0]);
                ULONG uFetched = 0;
                if (pEnum->Next(1, rgVar, &uFetched) < 0 || uFetched == 0) {
                    VariantClear(&rgVar[0]);
                    break;
                }
                if (rgVar[0].vt == VT_BSTR && rgVar[0].bstrVal != nullptr) {
                    Ztl_variant_t vVal;
                    if (pPropNode->get_item(rgVar[0].bstrVal, &vVal) >= 0) {
                        IUnknown* pSubUnk = vVal.GetUnknown(false, false);
                        if (pSubUnk != nullptr) {
                            IWzProperty* pSubProp = reinterpret_cast<IWzProperty*>(pSubUnk);
                            ParseItemIDsRecursively(pSubProp, itemIDs, itemToSetID, setID);
                        } else {
                            int itemID = (vVal.vt == VT_I4) ? vVal.lVal : (vVal.vt == VT_BSTR && vVal.bstrVal != nullptr ? _wtoi(vVal.bstrVal) : 0);
                            if (itemID > 0) {
                                itemIDs.push_back(itemID);
                                itemToSetID[itemID] = setID;
                            }
                        }
                    }
                }
                VariantClear(&rgVar[0]);
            }
            pEnum->Release();
        }
        pEnumUnknown->Release();
    }
}

void LoadSetItemInfo() {
    std::cout << "[SetItem] Loading SetItemInfo.img from Etc.wz..." << std::endl;
    void** ppResMan = reinterpret_cast<void**>(0x00BF14E8);
    if (ppResMan == NULL || *ppResMan == NULL) {
        std::cout << "[SetItem] Error: WzResMan not initialized!" << std::endl;
        return;
    }

    IWzResMan* pResMan = reinterpret_cast<IWzResMan*>(*ppResMan);
    Ztl_variant_t vEmpty;
    Ztl_variant_t vSetItemInfo;
    
    if (pResMan->raw_GetObject(L"Etc/SetItemInfo.img", vEmpty, vEmpty, &vSetItemInfo) < 0) {
        std::cout << "[SetItem] Error: Path 'Etc/SetItemInfo.img' not found!" << std::endl;
        return;
    }

    IWzProperty* pSetItemInfo = reinterpret_cast<IWzProperty*>(vSetItemInfo.GetUnknown(false, false));
    if (pSetItemInfo == NULL) return;

    IUnknown* pEnumUnknown = nullptr;
    if (pSetItemInfo->get__NewEnum(&pEnumUnknown) < 0 || pEnumUnknown == nullptr) return;

    IEnumVARIANT* pEnum = nullptr;
    if (pEnumUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEnum) < 0 || pEnum == nullptr) {
        pEnumUnknown->Release();
        return;
    }

    int setsLoaded = 0;
    while (true) {
        VARIANT rgVar[1];
        VariantInit(&rgVar[0]);
        ULONG uCeltFetched = 0;
        if (pEnum->Next(1, rgVar, &uCeltFetched) < 0 || uCeltFetched == 0) {
            VariantClear(&rgVar[0]);
            break;
        }

        if (rgVar[0].vt == VT_BSTR && rgVar[0].bstrVal != nullptr) {
            int setID = _wtoi(rgVar[0].bstrVal);
            Ztl_variant_t vProp;
            if (pSetItemInfo->get_item(rgVar[0].bstrVal, &vProp) >= 0) {
                IWzProperty* pProp = reinterpret_cast<IWzProperty*>(vProp.GetUnknown(false, false));
                if (pProp != NULL) {
                    SetInfo info;
                    info.setID = setID;

                    Ztl_variant_t vSetName;
                    if (pProp->get_item(L"setItemName", &vSetName) >= 0) {
                        if (vSetName.vt == VT_BSTR && vSetName.bstrVal != nullptr) {
                            info.setName = vSetName.bstrVal;
                        }
                    }

                    Ztl_variant_t vItemIDNode;
                    if (pProp->get_item(L"ItemID", &vItemIDNode) >= 0) {
                        IWzProperty* pItemIDNode = reinterpret_cast<IWzProperty*>(vItemIDNode.GetUnknown(false, false));
                        if (pItemIDNode != NULL) {
                            ParseItemIDsRecursively(pItemIDNode, info.itemIDs, g_ItemToSetID, setID);
                        }
                    }

                    Ztl_variant_t vEffectNode;
                    if (pProp->get_item(L"Effect", &vEffectNode) >= 0) {
                        IWzProperty* pEffectNode = reinterpret_cast<IWzProperty*>(vEffectNode.GetUnknown(false, false));
                        if (pEffectNode != NULL) {
                            IUnknown* pEffEnumUnknown = nullptr;
                            if (pEffectNode->get__NewEnum(&pEffEnumUnknown) >= 0 && pEffEnumUnknown != nullptr) {
                                IEnumVARIANT* pEffEnum = nullptr;
                                if (pEffEnumUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pEffEnum) >= 0 && pEffEnum != nullptr) {
                                    while (true) {
                                        VARIANT rgEffVar[1];
                                        VariantInit(&rgEffVar[0]);
                                        ULONG uEffFetched = 0;
                                        if (pEffEnum->Next(1, rgEffVar, &uEffFetched) < 0 || uEffFetched == 0) {
                                            VariantClear(&rgEffVar[0]);
                                            break;
                                        }

                                        if (rgEffVar[0].vt == VT_BSTR && rgEffVar[0].bstrVal != nullptr) {
                                            int count = _wtoi(rgEffVar[0].bstrVal);
                                            Ztl_variant_t vEffVal;
                                            if (pEffectNode->get_item(rgEffVar[0].bstrVal, &vEffVal) >= 0) {
                                                IWzProperty* pEffVal = reinterpret_cast<IWzProperty*>(vEffVal.GetUnknown(false, false));
                                                if (pEffVal != nullptr) {
                                                    SetEffect eff;
                                                    eff.count = count;
                                                    
                                                    IUnknown* pStatEnumUnknown = nullptr;
                                                    if (pEffVal->get__NewEnum(&pStatEnumUnknown) >= 0 && pStatEnumUnknown != nullptr) {
                                                        IEnumVARIANT* pStatEnum = nullptr;
                                                        if (pStatEnumUnknown->QueryInterface(IID_IEnumVARIANT, (void**)&pStatEnum) >= 0 && pStatEnum != nullptr) {
                                                            while (true) {
                                                                VARIANT rgStatVar[1];
                                                                VariantInit(&rgStatVar[0]);
                                                                ULONG uStatFetched = 0;
                                                                if (pStatEnum->Next(1, rgStatVar, &uStatFetched) < 0 || uStatFetched == 0) {
                                                                    VariantClear(&rgStatVar[0]);
                                                                    break;
                                                                }
                                                                
                                                                if (rgStatVar[0].vt == VT_BSTR && rgStatVar[0].bstrVal != nullptr) {
                                                                    Ztl_variant_t vStatData;
                                                                    if (pEffVal->get_item(rgStatVar[0].bstrVal, &vStatData) >= 0) {
                                                                        int statValue = 0;
                                                                        if (vStatData.vt == VT_I4) statValue = vStatData.lVal;
                                                                        else if (vStatData.vt == VT_I2) statValue = vStatData.iVal;
                                                                        
                                                                        std::string statName = WStringToString(rgStatVar[0].bstrVal);
                                                                        eff.stats.push_back(std::make_pair(TranslateStatName(statName), statValue));
                                                                    }
                                                                }
                                                                VariantClear(&rgStatVar[0]);
                                                            }
                                                            pStatEnum->Release();
                                                        }
                                                        pStatEnumUnknown->Release();
                                                    }
                                                    
                                                    info.effects.push_back(eff);
                                                }
                                            }
                                        }
                                        VariantClear(&rgEffVar[0]);
                                    }
                                    pEffEnum->Release();
                                }
                                pEffEnumUnknown->Release();
                            }
                        }
                    }

                    std::lock_guard<std::mutex> lock(g_SetItemMutex);
                    g_SetInfos[setID] = info;
                    setsLoaded++;
                }
            }
        }
        VariantClear(&rgVar[0]);
    }

    pEnum->Release();
    pEnumUnknown->Release();
    std::cout << "[SetItem] Loaded " << setsLoaded << " sets from Etc/SetItemInfo.img successfully!" << std::endl;
}


void UpdateSetPanelData(void* pToolTip, void* pEquip) {
    if (!pEquip || reinterpret_cast<uintptr_t>(pEquip) < 0x10000) return;

    std::lock_guard<std::mutex> lock(g_SetPanelMutex);

    TSecType<long>* pItemIDSec = reinterpret_cast<TSecType<long>*>(reinterpret_cast<char*>(pEquip) + 0x0C);
    int itemID = pItemIDSec->GetData();
    if (itemID <= 0) return;

    std::lock_guard<std::mutex> lockItem(g_SetItemMutex);

    auto itSetID = g_ItemToSetID.find(itemID);
    if (itSetID == g_ItemToSetID.end()) {
        if (g_SetPanelData.pToolTip == pToolTip) {
            g_SetPanelData.active = false;
        }
        return;
    }

    int setID = itSetID->second;
    auto itInfo = g_SetInfos.find(setID);
    if (itInfo == g_SetInfos.end()) {
        g_SetPanelData.active = false;
        return;
    }

    const SetInfo& info = itInfo->second;

    int nativeX = -1;
    int nativeY = -1;
    int nativeWidth = 236;

    if (pToolTip) {
        int extX = *reinterpret_cast<int*>(reinterpret_cast<char*>(pToolTip) + 0x14);
        int extY = *reinterpret_cast<int*>(reinterpret_cast<char*>(pToolTip) + 0x18);
        int extRight = *reinterpret_cast<int*>(reinterpret_cast<char*>(pToolTip) + 0x1C);
        
        // Sanity check coordinates to prevent flying off screen (e.g. from broadcast tooltips which have different memory layout)
        if (extX >= -500 && extX <= 4000 && extY >= -500 && extY <= 4000) {
            nativeX = extX;
            nativeY = extY;
            
            int calculatedWidth = extRight - extX;
            if (calculatedWidth >= 150 && calculatedWidth <= 600) {
                nativeWidth = calculatedWidth;
            } else {
                nativeWidth = 236; // Default width fallback
            }
        } else {
            nativeX = -1;
            nativeY = -1;
        }
    }

    // If set panel is already active and updated recently in the same frame,
    // only keep the one with the larger X coordinate (the hovered tooltip on the right),
    // UNLESS we are updating the exact same tooltip, then we must always update coordinates to prevent lagging.
    if (g_SetPanelData.active && (GetTickCount() - g_SetPanelData.lastUpdated < 100)) {
        if (g_SetPanelData.pToolTip != pToolTip) {
            if (nativeX < g_SetPanelData.nativeX) {
                return;
            }
        }
    }

    g_SetPanelData.pToolTip = pToolTip;
    g_SetPanelData.pEquip = pEquip;
    g_SetPanelData.setName = WStringToString(info.setName);
    g_SetPanelData.hideFrameCount = 0;
    g_SetPanelData.nativeX = nativeX;
    g_SetPanelData.nativeY = nativeY;
    g_SetPanelData.nativeWidth = nativeWidth;

    if (info.itemIDs.empty()) return;
    g_SetPanelData.items.clear();
    g_SetPanelData.effects.clear();

    std::set<int> equippedItems;
    void* pLocalUser = *reinterpret_cast<void**>(0x00BEBF98);
    if (pLocalUser != nullptr) {
        char* pBase = reinterpret_cast<char*>(pLocalUser);
        int* pNormal = reinterpret_cast<int*>(pBase + 0xA5);
        for (int slot = 1; slot <= 51; ++slot) {
            int eqID = pNormal[slot];
            if (eqID > 100000 && eqID < 10000000) equippedItems.insert(eqID);
        }
        int* pCash = reinterpret_cast<int*>(pBase + 0x175);
        for (int slot = 1; slot <= 51; ++slot) {
            int eqID = pCash[slot];
            if (eqID > 100000 && eqID < 10000000) equippedItems.insert(eqID);
        }
    }

    int equippedCount = 0;
    for (int sid : info.itemIDs) {
        if (equippedItems.count(sid) > 0) equippedCount++;
    }

    g_SetPanelData.equippedCount = equippedCount;

    for (int sid : info.itemIDs) {
        SetPanelItemInfo dItem;
        dItem.id = sid;
        dItem.name = GetCachedItemName(sid);
        dItem.category = GetCategoryChinese(sid);
        dItem.isEquipped = equippedItems.count(sid) > 0;
        g_SetPanelData.items.push_back(dItem);
    }

    for (const auto& eff : info.effects) {
        SetPanelEffectInfo panelEff;
        panelEff.count = eff.count;
        panelEff.isActive = (equippedCount >= eff.count);
        for (const auto& stat : eff.stats) {
            panelEff.stats.push_back(std::make_pair(stat.first, stat.second));
        }
        g_SetPanelData.effects.push_back(panelEff);
    }

    g_SetPanelData.active = true;
    g_SetPanelData.lastUpdated = GetTickCount();
}

void ClearSetPanelData(void* pToolTip) {
    std::lock_guard<std::mutex> lock(g_SetPanelMutex);
    if (g_SetPanelData.pToolTip != pToolTip) return;
    g_SetPanelData.active = false;
}

void DrawTextWithShadow(const char* text, ImVec4 color) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    // Draw the shadow manually via draw list
    ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 255), text);
    // Draw the actual text using ImGui so it handles cursor layout and boundaries
    ImGui::TextColored(color, "%s", text);
}

void DrawSetItemImGui() {
    std::lock_guard<std::mutex> lock(g_SetPanelMutex);

    if (!g_SetPanelData.active) return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    float screenW = ImGui::GetIO().DisplaySize.x;

    float myWidth = 260.0f;
    float finalX = 0;
    float finalY = 0;
    
    if (g_SetPanelData.nativeX != -1 && g_SetPanelData.nativeY != -1) {
        // User requested: "統一右邊" (Uniformly on the right) and "上面是對齊的" (Top is aligned)
        // Place on the right side of the native tooltip
        finalX = (float)(g_SetPanelData.nativeX + g_SetPanelData.nativeWidth);
        // If it goes off-screen to the right, draw on the left side
        if (finalX + myWidth > screenW) {
            finalX = (float)(g_SetPanelData.nativeX - myWidth);
        }
        // Perfect top alignment
        finalY = (float)g_SetPanelData.nativeY;
    } else {
        // Fallback if extraction failed
        finalX = mousePos.x - myWidth - 10.0f;
        if (finalX < 0) {
            finalX = mousePos.x + 200.0f;
        }
        finalY = mousePos.y + 12.0f;
    }

    ImGui::SetNextWindowPos(ImVec2(finalX, finalY), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(260, 0), ImVec2(400, 600));

    // Style parameters matching MapleStory native tooltip
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.12f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.40f, 0.50f, 0.80f));
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##SetItemPanel", nullptr, flags)) {
        ImVec4 titleColor(0.2f, 1.0f, 0.2f, 1.0f); // Bright green
        
        // Center the title
        float titleWidth = ImGui::CalcTextSize(g_SetPanelData.setName.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleWidth) * 0.5f);
        DrawTextWithShadow(g_SetPanelData.setName.c_str(), titleColor);

        ImGui::Dummy(ImVec2(0, 4));

        for (const auto& item : g_SetPanelData.items) {
            ImVec4 color = item.isEquipped ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.50f, 0.55f, 0.55f, 0.7f);
            
            std::string itemName = item.name;
            std::string catText = "(" + item.category + ")";
            
            DrawTextWithShadow(itemName.c_str(), color);
            
            float catWidth = ImGui::CalcTextSize(catText.c_str()).x;
            ImGui::SameLine();
            // Prevent bounds expansion by setting cursor directly rather than using SameLine with an offset
            float targetX = 260.0f - catWidth - 8.0f; // Window width - cat width - padding
            if (ImGui::GetCursorPosX() < targetX) {
                ImGui::SetCursorPosX(targetX);
            }
            
            // Category text is ALWAYS grey to match the screenshot
            DrawTextWithShadow(catText.c_str(), ImVec4(0.50f, 0.55f, 0.55f, 0.7f));
        }

        ImGui::Dummy(ImVec2(0, 2));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 2));

        std::string setEffectStr = WStringToString(L"%d \u4ef6\u5957\u6548\u679c\uff1a"); // %d 件套效果：
        std::string dot = WStringToString(L"\u00b7");
        std::string colon = WStringToString(L"\uff1a");

        for (const auto& eff : g_SetPanelData.effects) {
            ImVec4 headerColor = eff.isActive
                ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) // Bright green
                : ImVec4(0.50f, 0.55f, 0.55f, 0.7f); // Grey

            char buf[128];
            snprintf(buf, sizeof(buf), setEffectStr.c_str(), eff.count);
            DrawTextWithShadow(buf, headerColor);

            ImVec4 statColor = eff.isActive
                ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) // White
                : ImVec4(0.50f, 0.55f, 0.55f, 0.7f); // Grey

            for (const auto& stat : eff.stats) {
                char statBuf[128];
                // Format: "· 力量 : +3"
                snprintf(statBuf, sizeof(statBuf), "%s %s : +%d", dot.c_str(), stat.first.c_str(), stat.second);
                DrawTextWithShadow(statBuf, statColor);
            }
        }

        ImGui::End();
    }
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

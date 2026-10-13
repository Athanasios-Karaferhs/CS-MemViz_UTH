#pragma once
#include "imgui.h"
#include <cmath>

// pos  = top-left corner in SCREEN coordinates, size = width/height in pixels.
inline void DrawBox(ImDrawList* dl, ImVec2 pos, ImVec2 size, ImU32 fill,
                    const char* line1, const char* line2 = nullptr)
{
    ImVec2 max(pos.x + size.x, pos.y + size.y);

    dl->AddRectFilled(pos, max, fill, 6.0f);                          // the body
    dl->AddRect(pos, max, IM_COL32(255, 255, 255, 200), 6.0f, 0, 2.0f); // the border

    float lineH = ImGui::GetTextLineHeightWithSpacing();
    dl->AddText(ImVec2(pos.x + 8, pos.y + 6), IM_COL32_WHITE, line1);
    if (line2)
        dl->AddText(ImVec2(pos.x + 8, pos.y + 6 + lineH), IM_COL32(220, 220, 220, 255), line2);
}

// A straight arrow from 'from' to 'to' with a triangular head at 'to'.
inline void DrawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 col, float thickness = 2.0f)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;          // nothing to draw
    dx /= len;                       // (dx, dy) is now the unit direction
    dy /= len;

    const float head = 10.0f;        // length of the arrowhead
    ImVec2 base(to.x - dx * head, to.y - dy * head);   // where the head starts

    dl->AddLine(from, base, col, thickness);           // the shaft

    // the two back corners of the head are perpendicular to the direction
    ImVec2 left (base.x - dy * head * 0.5f, base.y + dx * head * 0.5f);
    ImVec2 right(base.x + dy * head * 0.5f, base.y - dx * head * 0.5f);
    dl->AddTriangleFilled(to, left, right, col);       // the head
}
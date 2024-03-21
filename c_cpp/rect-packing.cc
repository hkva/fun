// SPDX-License-Identifier: MIT

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "hk.hh"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <algorithm> // std::sort

using namespace hk;

static inline f32 now() {
    return (f32)SDL_GetTicks() / 1e3f;
}

int main(int argc, const char* argv[]) {
    SDL_version sdlv_l; SDL_GetVersion(&sdlv_l);
    SDL_version sdlv_c; SDL_VERSION(&sdlv_c);
    dbglog("SDL v%d.%d.%d (compiled against v%d.%d.%d)",
        sdlv_l.major, sdlv_l.minor, sdlv_l.patch,
        sdlv_c.major, sdlv_c.minor, sdlv_c.patch);
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        dbgerr("Failed to initialize SDL: %s", SDL_GetError());
    }

    const u32 wndflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    SDL_Window* wnd = SDL_CreateWindow(__FILE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, wndflags);
    if (!wnd) {
        dbgerr("Failed to create SDL window: %s", SDL_GetError());
    }

    SDL_Renderer* r = SDL_CreateRenderer(wnd, -1, 0);
    if (!r) {
        dbgerr("Failed to create SDL renderer: %s", SDL_GetError());
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplSDL2_InitForSDLRenderer(wnd, r);
    ImGui_ImplSDLRenderer2_Init(r);

    SDL_ShowWindow(wnd);
    u8 wants_quit = 0;
    do {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
            case SDL_QUIT: {
                wants_quit = 1;
            } break;
            case SDL_KEYDOWN: {
                if (evt.key.keysym.sym == SDLK_q || evt.key.keysym.sym == SDLK_ESCAPE) {
                    wants_quit = 1;
                }
            } break;
            }

            ImGui_ImplSDL2_ProcessEvent(&evt);
        }

        SDL_SetRenderDrawColor(r, 30, 50, 50, 255);
        SDL_RenderClear(r);

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //
        struct Rect {
            Vec2 pos;
            Vec2 size;
            ImColor color;
        };
        //
        static std::vector<Rect> rects_original = { };
        static std::vector<Rect> rects = { };
        static f32 rects_gen_time = 0;
        //
        static RandomXOR rng = RandomXOR();
        static u32 gen_count = 20;
        static u32 gen_min_size = 25;
        static u32 gen_max_size = 150;
        static u32 canvas_w = 512;
        static u32 canvas_h = 512;
        //
        const char* methods[] = {
            "Noob method",
            "Noob method (sorted by area)",
            "Noob method (reverse sorted by area)",
            "Noob method (sorted by height)",
        };
        static usize selected_method = 0;
        static bool did_pack_at_least_once = false;


        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        if (ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration)) {
            ImGui::Columns(2);
            ImGui::InputInt("Count", (int*)&gen_count);
            ImGui::InputInt("Minimum size", (int*)&gen_min_size);
            ImGui::InputInt("Maximum size", (int*)&gen_max_size);
            
            if (rects.size() == 0 || ImGui::Button("New inputs")) {
                rects.clear();
                for (u32 i = 0; i < gen_count; ++i) {
                    Rect r = Rect();
                    r.size = Vec2(
                        rng.random<f32>(gen_min_size, gen_max_size),
                        rng.random<f32>(gen_min_size, gen_max_size)
                    );
                    r.color = ImColor(
                        rng.random<f32>(0.0f, 1.0f),
                        rng.random<f32>(0.0f, 1.0f),
                        rng.random<f32>(0.0f, 1.0f)
                    );
                    rects.push_back(r);
                }
                rects_gen_time = now();
                rects_original = rects;
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, max(0.0f, 1.0f - ((now() - rects_gen_time) / 2.0f))));
            ImGui::Text("Generated %d rects", (u32)rects.size());
            ImGui::PopStyleColor();

            ImGui::InputInt("Canvas width", (int*)&canvas_w);
            ImGui::InputInt("Canvas height", (int*)&canvas_h);
            if (ImGui::BeginCombo("Method", methods[selected_method])) {
                for (usize i = 0; i < arrlen(methods); ++i) {
                    if (ImGui::Selectable(methods[i], i == selected_method)) {
                        selected_method = i;
                    }
                    if (i == selected_method) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Pack")) {
                rects = rects_original;
                switch (selected_method) {
                case 0: {
                    f32 x_cur = 0;
                    f32 y_cur = 0;
                    f32 y_step = 0;
                    for (auto& rect : rects) {
                        if (x_cur + rect.size.x >= canvas_w) {
                            y_cur += y_step;
                            y_step = 0;
                            x_cur = 0;
                        }
                        y_step = max(y_step, rect.size.y);
                        rect.pos.x = x_cur;
                        rect.pos.y = y_cur;
                        x_cur += rect.size.x;
                    }
                } break;
                case 1: {
                    std::sort(rects.begin(), rects.end(), [](const Rect& left, const Rect& right) {
                        return left.size.x * left.size.y < right.size.x * right.size.y;
                    });

                    f32 x_cur = 0;
                    f32 y_cur = 0;
                    f32 y_step = 0;
                    for (auto& rect : rects) {
                        if (x_cur + rect.size.x >= canvas_w) {
                            y_cur += y_step;
                            y_step = 0;
                            x_cur = 0;
                        }
                        y_step = max(y_step, rect.size.y);
                        rect.pos.x = x_cur;
                        rect.pos.y = y_cur;
                        x_cur += rect.size.x;
                    }
                } break;
                case 2: {
                    std::sort(rects.begin(), rects.end(), [](const Rect& left, const Rect& right) {
                        return left.size.x * left.size.y > right.size.x * right.size.y;
                    });

                    f32 x_cur = 0;
                    f32 y_cur = 0;
                    f32 y_step = 0;
                    for (auto& rect : rects) {
                        if (x_cur + rect.size.x >= canvas_w) {
                            y_cur += y_step;
                            y_step = 0;
                            x_cur = 0;
                        }
                        y_step = max(y_step, rect.size.y);
                        rect.pos.x = x_cur;
                        rect.pos.y = y_cur;
                        x_cur += rect.size.x;
                    }
                } break;
                case 3: {
                    std::sort(rects.begin(), rects.end(), [](const Rect& left, const Rect& right) {
                        return left.size.y < right.size.y;
                    });

                    f32 x_cur = 0;
                    f32 y_cur = 0;
                    f32 y_step = 0;
                    for (auto& rect : rects) {
                        if (x_cur + rect.size.x >= canvas_w) {
                            y_cur += y_step;
                            y_step = 0;
                            x_cur = 0;
                        }
                        y_step = max(y_step, rect.size.y);
                        rect.pos.x = x_cur;
                        rect.pos.y = y_cur;
                        x_cur += rect.size.x;
                    }
                } break;
                }

                did_pack_at_least_once = true;
            }

            ImGui::SetColumnWidth(0, 300.0f);
            ImGui::NextColumn();
            ImGui::Text("%ux%u", (u32)canvas_w, (u32)canvas_h);

            ImVec2 cur = ImGui::GetCursorPos();

            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(cur.x - 1, cur.y - 1),
                ImVec2(cur.x + canvas_w + 1, cur.y + canvas_h + 1),
                ImColor(0.1f, 1.0f, 0.1f)
            );

            if (did_pack_at_least_once) {
                for (usize i = 0; i < rects.size(); ++i) {
                    const Rect& rect = rects[i];
                    ImVec2 p1 = ImVec2(cur.x + rect.pos.x, cur.y + rect.pos.y);
                    ImVec2 p2 = ImVec2(p1.x + rect.size.x, p1.y + rect.size.y);
                    ImGui::GetForegroundDrawList()->AddRect(p1, p2, rect.color);
                    ImVec2 t = ImVec2(p1.x + 5, p1.y + 5);
                    char buf[64]; snprintf(buf, sizeof(buf), "#%u", (u32)i + 1);
                    u32 color = IM_COL32_WHITE;
                    if (rect.pos.x + rect.size.x >= canvas_w || rect.pos.y + rect.size.y >= canvas_h) {
                        color = IM_COL32(0xFF, 0x00, 0x00, 0xFF);
                    }
                    ImGui::GetForegroundDrawList()->AddText(t, color, buf);

                }
            }

            ImGui::NextColumn();
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(r);

    } while (!wants_quit);

    SDL_DestroyWindow(wnd);

    return EXIT_SUCCESS;
}
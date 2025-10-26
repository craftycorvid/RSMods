#pragma once
#include <cstddef>
#include <synchapi.h>
#include <Windows.h>
#include <gdiplus.h>

namespace D3D {
    class GdiplusManager {
    public:
        static GdiplusManager& GetInstance() {
            static GdiplusManager instance;
            return instance;
        }

        bool IsInitialized() const { return initialized; }

        GdiplusManager(const GdiplusManager&) = delete;
        GdiplusManager& operator=(const GdiplusManager&) = delete;
        GdiplusManager(GdiplusManager&&) = delete;
        GdiplusManager& operator=(GdiplusManager&&) = delete;

    private:
        GdiplusManager() {
            while (GetModuleHandleA("gdiplus.dll") == NULL) {
                Sleep(100);
            }

            Gdiplus::GdiplusStartupInput input;
            Gdiplus::Status status = Gdiplus::GdiplusStartup(&token, &input, nullptr);
            initialized = (status == Gdiplus::Ok);
        }

        ~GdiplusManager() {
            if (initialized) {
                Gdiplus::GdiplusShutdown(token);
            }
        }

        ULONG_PTR token = 0;
        bool initialized = false;
    };
}

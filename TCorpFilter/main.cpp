#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <dwmapi.h>
#include <magnification.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "magnification.lib")
#pragma comment(lib, "comctl32.lib")

// Set the subsystem to Windows (not Console)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// For Monitor Configuration API
#pragma comment(lib, "dxva2.lib")

// Forward declarations for Monitor Configuration API
typedef struct _PHYSICAL_MONITOR
{
    HANDLE  hPhysicalMonitor;
    WCHAR   szPhysicalMonitorDescription[128];
} PHYSICAL_MONITOR, *LPPHYSICAL_MONITOR;

extern "C" {
    BOOL WINAPI GetNumberOfPhysicalMonitorsFromHMONITOR(HMONITOR hMonitor, LPDWORD pdwNumberOfPhysicalMonitors);
    BOOL WINAPI GetPhysicalMonitorsFromHMONITOR(HMONITOR hMonitor, DWORD dwPhysicalMonitorArraySize, LPPHYSICAL_MONITOR pPhysicalMonitorArray);
    BOOL WINAPI DestroyPhysicalMonitors(DWORD dwPhysicalMonitorArraySize, LPPHYSICAL_MONITOR pPhysicalMonitorArray);
    BOOL WINAPI SetMonitorBrightness(HANDLE hMonitor, DWORD dwNewBrightness);
    BOOL WINAPI SetMonitorContrast(HANDLE hMonitor, DWORD dwNewContrast);
    BOOL WINAPI GetMonitorBrightness(HANDLE hMonitor, LPDWORD pdwMinimumBrightness, LPDWORD pdwCurrentBrightness, LPDWORD pdwMaximumBrightness);
    BOOL WINAPI GetMonitorContrast(HANDLE hMonitor, LPDWORD pdwMinimumContrast, LPDWORD pdwCurrentContrast, LPDWORD pdwMaximumContrast);
}

// Window controls IDs
#define ID_APPLY_BUTTON 1001
#define ID_RESTORE_BUTTON 1002
#define ID_BRIGHTNESS_SLIDER 1003
#define ID_CONTRAST_SLIDER 1004
#define ID_SATURATION_SLIDER 1005
#define ID_BRIGHTNESS_LABEL 1006
#define ID_CONTRAST_LABEL 1007
#define ID_SATURATION_LABEL 1008
#define ID_METHOD_COMBO 1009
#define ID_METHOD_LABEL 1010

// Global variables
HWND hBrightnessSlider, hContrastSlider, hSaturationSlider;
HWND hBrightnessLabel, hContrastLabel, hSaturationLabel;

// Magnification API state
BOOL magnificationInitialized = FALSE;

// Function to clamp values between min and max
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}





// Method 3: Color Transform using Magnification API
BOOL InitializeMagnification() {
    if (!magnificationInitialized) {
        if (MagInitialize()) {
            magnificationInitialized = TRUE;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL ApplyColorTransform(float brightness, float contrast, float saturation) {
    if (!InitializeMagnification()) {
        return FALSE;
    }

    // Create color transformation matrix
    MAGCOLOREFFECT colorEffect;
    ZeroMemory(&colorEffect, sizeof(colorEffect));

    // Luminance weights for proper grayscale conversion
    float lumR = .33f; // 0.299f;
    float lumG = .33f; // 0.587f;
    float lumB = .33f; // 0.114f;

    // Create saturation matrix
    float sr = lumR * (1.0f - saturation);
    float sg = lumG * (1.0f - saturation);
    float sb = lumB * (1.0f - saturation);

    float satMatrix[3][3] = {
        {sr + saturation, sg,              sb             },
        {sr,              sg + saturation, sb             },
        {sr,              sg,              sb + saturation}
    };

    // Calculate contrast offset (shift to apply contrast around 0.5 gray)
    float contrastOffset = 0.5f * (1.0f - contrast);

    // Fill the MAGCOLOREFFECT matrix (5x5)
    // Apply saturation, then contrast, then brightness
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            colorEffect.transform[i][j] = satMatrix[i][j] * contrast * brightness;
        }
        // Apply contrast offset (brightness is multiplicative, not additive)
        colorEffect.transform[i][4] = contrastOffset;
    }

    // Alpha channel - keep unchanged
    colorEffect.transform[3][3] = 1.0f;
    colorEffect.transform[3][4] = 0.0f;

    // Row 4 (required by API) - identity
    colorEffect.transform[4][0] = 0.0f;
    colorEffect.transform[4][1] = 0.0f;
    colorEffect.transform[4][2] = 0.0f;
    colorEffect.transform[4][3] = 0.0f;
    colorEffect.transform[4][4] = 1.0f;

    // Apply the color effect to the entire screen
    return MagSetFullscreenColorEffect(&colorEffect);
}

void RestoreColorTransform() {
    if (magnificationInitialized) {
        // Reset to identity matrix (no transformation)
        MAGCOLOREFFECT identityEffect;
        ZeroMemory(&identityEffect, sizeof(identityEffect));
        
        // Set up identity matrix
        identityEffect.transform[0][0] = 1.0f;
        identityEffect.transform[1][1] = 1.0f;
        identityEffect.transform[2][2] = 1.0f;
        identityEffect.transform[3][3] = 1.0f;
        identityEffect.transform[4][4] = 1.0f;
        
        MagSetFullscreenColorEffect(&identityEffect);
    }
}



// Apply color transform filter
BOOL ApplyFilter(float brightness, float contrast) {
    int saturationValue = (int)SendMessage(hSaturationSlider, TBM_GETPOS, 0, 0);
    float saturation = saturationValue / 100.0f;
    return ApplyColorTransform(brightness, contrast, saturation);
}

void RestoreOriginal() {
    RestoreColorTransform();
}

// Update the labels with current values
void UpdateLabels() {
    int brightnessValue = (int)SendMessage(hBrightnessSlider, TBM_GETPOS, 0, 0);
    int contrastValue = (int)SendMessage(hContrastSlider, TBM_GETPOS, 0, 0);
    int saturationValue = (int)SendMessage(hSaturationSlider, TBM_GETPOS, 0, 0);

    wchar_t brightnessText[50];
    wchar_t contrastText[50];
    wchar_t saturationText[50];

    swprintf(brightnessText, 50, L"Brightness: %d%%", brightnessValue);
    swprintf(contrastText, 50, L"Contrast: %d%%", contrastValue);
    swprintf(saturationText, 50, L"Color Saturation: %d%%", saturationValue);

    SetWindowText(hBrightnessLabel, brightnessText);
    SetWindowText(hContrastLabel, contrastText);
    SetWindowText(hSaturationLabel, saturationText);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Create brightness label
        hBrightnessLabel = CreateWindow(L"STATIC", L"Brightness: 100%",
            WS_VISIBLE | WS_CHILD,
            20, 20, 150, 20,
            hwnd, (HMENU)ID_BRIGHTNESS_LABEL, GetModuleHandle(NULL), NULL);

        // Create brightness slider (10-100%)
        hBrightnessSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
            20, 45, 200, 30,
            hwnd, (HMENU)ID_BRIGHTNESS_SLIDER, GetModuleHandle(NULL), NULL);
        SendMessage(hBrightnessSlider, TBM_SETRANGE, TRUE, MAKELONG(10, 100));
        SendMessage(hBrightnessSlider, TBM_SETPOS, TRUE, 100);

        // Create contrast label
        hContrastLabel = CreateWindow(L"STATIC", L"Contrast: 100%",
            WS_VISIBLE | WS_CHILD,
            20, 90, 150, 20,
            hwnd, (HMENU)ID_CONTRAST_LABEL, GetModuleHandle(NULL), NULL);

        // Create contrast slider (10-100%)
        hContrastSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
            20, 115, 200, 30,
            hwnd, (HMENU)ID_CONTRAST_SLIDER, GetModuleHandle(NULL), NULL);
        SendMessage(hContrastSlider, TBM_SETRANGE, TRUE, MAKELONG(10, 100));
        SendMessage(hContrastSlider, TBM_SETPOS, TRUE, 100);

        // Create saturation label
        hSaturationLabel = CreateWindow(L"STATIC", L"Color Saturation: 100%",
            WS_VISIBLE | WS_CHILD,
            20, 160, 180, 20,
            hwnd, (HMENU)ID_SATURATION_LABEL, GetModuleHandle(NULL), NULL);

        // Create saturation slider (10-100%)
        hSaturationSlider = CreateWindow(TRACKBAR_CLASS, L"",
            WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
            20, 185, 200, 30,
            hwnd, (HMENU)ID_SATURATION_SLIDER, GetModuleHandle(NULL), NULL);
        SendMessage(hSaturationSlider, TBM_SETRANGE, TRUE, MAKELONG(10, 100));
        SendMessage(hSaturationSlider, TBM_SETPOS, TRUE, 100);

        // Create Apply button
        CreateWindow(L"BUTTON", L"Apply Filter",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 230, 100, 30,
            hwnd, (HMENU)ID_APPLY_BUTTON, GetModuleHandle(NULL), NULL);

        // Create Restore button
        CreateWindow(L"BUTTON", L"Restore Default",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            130, 230, 120, 30,
            hwnd, (HMENU)ID_RESTORE_BUTTON, GetModuleHandle(NULL), NULL);

        UpdateLabels();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_APPLY_BUTTON: {
            int brightnessValue = (int)SendMessage(hBrightnessSlider, TBM_GETPOS, 0, 0);
            int contrastValue = (int)SendMessage(hContrastSlider, TBM_GETPOS, 0, 0);

            float brightness = brightnessValue / 100.0f;
            float contrast = contrastValue / 100.0f;

            if (ApplyFilter(brightness, contrast)) {
                MessageBox(hwnd, L"Color filter applied successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBox(hwnd, L"Failed to apply color filter!\n\nTry restarting the application.", L"Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
        case ID_RESTORE_BUTTON:
            RestoreOriginal();
            MessageBox(hwnd, L"Original settings restored!", L"Success", MB_OK | MB_ICONINFORMATION);
            break;
        }
        break;

    case WM_HSCROLL:
        // Update labels when sliders move
        if ((HWND)lParam == hBrightnessSlider || (HWND)lParam == hContrastSlider || (HWND)lParam == hSaturationSlider) {
            UpdateLabels();
        }
        break;

    case WM_DESTROY:
        // Restore original settings when closing
        RestoreOriginal();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    // Initialize common controls
    InitCommonControls();

    // Register window class
    const wchar_t* className = L"ScreenFilterApp";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Failed to register window class!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create window
    HWND hwnd = CreateWindow(
        className,
        L"T Corp Filter - Epilepsy Protection",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX, // Fixed size window
        CW_USEDEFAULT, CW_USEDEFAULT,
        280, 320,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
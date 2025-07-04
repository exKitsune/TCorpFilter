# TCorpFilter

Made for one of my friends who said some games gave them migraines.

It uses the Windows magnification API to change brightness, contrast, and saturation in order to relieve stress on the eyes. While you can use Windows settings, Nvidia control panel etc., the point of this is that you would tune your default settings to what you like the best, and then use this program to quickly adapt to a particular game or video that may cause problems.

This program contains sliders that go from 10% - 100% for each category, an apply button that works instantly across all monitors, and a reset button that restores normal behavior without messing up your slider settings. Exiting the program will automatically restore default behavior. If the app crashes while the filters are applied, simply reopen the app and hit reset or close it again.

This program is a single file and leaves no other file or any permanent setting changes to your computer.

---

If you want to use a nice looking UI, download a [2.x release](https://github.com/exKitsune/TCorpFilter/releases), but you might need to install the [latest windows SDK runtime](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/downloads)

Otherwise, download a [1.x release](https://github.com/exKitsune/TCorpFilter/releases/tag/v1.2) for the most minimal looking window

---

## Example images

The green in the GUI comes from my personalized windows accent color, it may be different for you.
My cursor is also present to demonstrate the program's non-effect on certain OS elements, but there's no actual cursor in the UI.

### Main GUI Interface

| Main GUI             | Brightness Control                | Contrast Control              | Saturation Control         |
| -------------------- | --------------------------------- | ----------------------------- | -------------------------- |
| ![Main GUI](gui.png) | ![Brightness](gui_brightness.png) | ![Contrast](gui_contrast.png) | ![Saturation](gui_sat.png) |

| Simple Interface              |
| ----------------------------- |
| ![Simple GUI](simple_gui.png) |

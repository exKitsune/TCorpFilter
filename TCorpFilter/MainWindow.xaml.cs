using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using System.Runtime.InteropServices;
using System.ComponentModel;
using Microsoft.UI.Windowing;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace TCorpFilter
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window, INotifyPropertyChanged
    {
        // Windows Magnification API imports
        [DllImport("Magnification.dll", CallingConvention = CallingConvention.StdCall)]
        private static extern bool MagInitialize();

        [DllImport("Magnification.dll", CallingConvention = CallingConvention.StdCall)]
        private static extern bool MagUninitialize();

        [DllImport("Magnification.dll", CallingConvention = CallingConvention.StdCall)]
        private static extern bool MagSetFullscreenColorEffect(ref MAGCOLOREFFECT pEffect);

        [StructLayout(LayoutKind.Sequential)]
        private struct MAGCOLOREFFECT
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 25)]
            public float[] transform;
        }

        // Properties for data binding
        private string _brightnessValue = "100%";
        private string _contrastValue = "100%";
        private string _saturationValue = "100%";

        public string BrightnessValue
        {
            get => _brightnessValue;
            set
            {
                if (_brightnessValue != value)
                {
                    _brightnessValue = value;
                    OnPropertyChanged(nameof(BrightnessValue));
                }
            }
        }

        public string ContrastValue
        {
            get => _contrastValue;
            set
            {
                if (_contrastValue != value)
                {
                    _contrastValue = value;
                    OnPropertyChanged(nameof(ContrastValue));
                }
            }
        }

        public string SaturationValue
        {
            get => _saturationValue;
            set
            {
                if (_saturationValue != value)
                {
                    _saturationValue = value;
                    OnPropertyChanged(nameof(SaturationValue));
                }
            }
        }

        private MAGCOLOREFFECT originalEffect;
        private bool isInitialized = false;

        public event PropertyChangedEventHandler? PropertyChanged;

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public MainWindow()
        {
            this.InitializeComponent();
            
            // Set window properties
            this.Title = "TCorpFilter";

			// Set window size and icon
			var appWindow = this.AppWindow;
			
			// Try to set the icon using the full path
			try
			{
				var iconPath = System.IO.Path.Combine(AppContext.BaseDirectory, "Assets", "favicon.ico");
				if (System.IO.File.Exists(iconPath))
				{
					appWindow.SetIcon(iconPath);
				}
				else
				{
					// Fallback to relative path
					appWindow.SetIcon("Assets/favicon.ico");
				}
			}
			catch
			{
				// If icon setting fails, continue without it
			}
			
			appWindow.Resize(new Windows.Graphics.SizeInt32 { Width = 800, Height = 700 });
            
            // Center the window on screen
            var displayArea = DisplayArea.Primary;
            var centerX = (displayArea.WorkArea.Width - appWindow.Size.Width) / 2;
            var centerY = (displayArea.WorkArea.Height - appWindow.Size.Height) / 2;
            appWindow.Move(new Windows.Graphics.PointInt32(centerX, centerY));
            
            // Set DataContext for binding on the root Grid
            if (this.Content is Grid rootGrid)
            {
                rootGrid.DataContext = this;
            }
            
            // Set image sources programmatically for reliability
            SetImageSources();
            
            // Initialize magnification API
            InitializeMagnification();
            
            // Subscribe to Closed event
            this.Closed += MainWindow_Closed;
        }

        private void SetImageSources()
        {
            try
            {
                // Set app icon using reliable path resolution
                var iconPath = System.IO.Path.Combine(AppContext.BaseDirectory, "Assets", "favicon.scale-200.png");
                if (System.IO.File.Exists(iconPath))
                {
                    AppIcon.Source = new Microsoft.UI.Xaml.Media.Imaging.BitmapImage(new Uri(iconPath));
                }
            }
            catch (Exception ex)
            {
                // If image loading fails, continue without it
                System.Diagnostics.Debug.WriteLine($"Failed to load app icon: {ex.Message}");
            }
        }

        private async void InitializeMagnification()
        {
            try
            {
                if (MagInitialize())
                {
                    isInitialized = true;
                    
                    // Store original effect (identity matrix)
                    originalEffect = new MAGCOLOREFFECT
                    {
                        transform = new float[25]
                        {
                            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                        }
                    };
                }
                else
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
            catch (Exception ex)
            {
                // Show error message
                var dialog = new ContentDialog
                {
                    Title = "Error",
                    Content = $"Failed to initialize display filter: {ex.Message}\n\nPlease run as administrator.",
                    CloseButtonText = "OK",
                    XamlRoot = this.Content.XamlRoot
                };
                await dialog.ShowAsync();
            }
        }

        private async void ApplyColorEffect()
        {
            if (!isInitialized)
            {
                var dialog = new ContentDialog
                {
                    Title = "Error",
                    Content = "Display filter not initialized. Please run as administrator.",
                    CloseButtonText = "OK",
                    XamlRoot = this.Content.XamlRoot
                };
                await dialog.ShowAsync();
                return;
            }

            try
            {
                float brightness = (float)(BrightnessSlider.Value / 100.0);
                float contrast = (float)(ContrastSlider.Value / 100.0);
                float saturation = (float)(SaturationSlider.Value / 100.0);

                // Clamp values to prevent invalid parameters
                brightness = Math.Max(0.1f, Math.Min(1.0f, brightness));
                contrast = Math.Max(0.1f, Math.Min(1.0f, contrast));
                saturation = Math.Max(0.1f, Math.Min(1.0f, saturation));

                // Create color transformation matrix using the correct algorithm
                var effect = CreateColorTransformMatrix(brightness, contrast, saturation);

                // Apply the effect to the entire screen
                bool result = MagSetFullscreenColorEffect(ref effect);
                
                if (!result)
                {
                    var dialog = new ContentDialog
                    {
                        Title = "Error",
                        Content = $"Failed to apply color effect. Error code: {Marshal.GetLastWin32Error()}",
                        CloseButtonText = "OK",
                        XamlRoot = this.Content.XamlRoot
                    };
                    await dialog.ShowAsync();
                }
                else
                {
                    //// Show success message briefly (for debug)
                    //var successDialog = new ContentDialog
                    //{
                    //    Title = "Success",
                    //    Content = $"Applied filter: Brightness={brightness:P0}, Contrast={contrast:P0}, Saturation={saturation:P0}",
                    //    CloseButtonText = "OK",
                    //    XamlRoot = this.Content.XamlRoot
                    //};
                    //await successDialog.ShowAsync();
                }
            }
            catch (Exception ex)
            {
                var dialog = new ContentDialog
                {
                    Title = "Error",
                    Content = $"Error applying color effect: {ex.Message}",
                    CloseButtonText = "OK",
                    XamlRoot = this.Content.XamlRoot
                };
                await dialog.ShowAsync();
            }
        }

        private MAGCOLOREFFECT CreateColorTransformMatrix(float brightness, float contrast, float saturation)
        {
            var effect = new MAGCOLOREFFECT
            {
                transform = new float[25]
            };


            float[,] satMatrix = {
                 { (1f - saturation) * 0.2126f + saturation, (1f - saturation) * 0.2126f,              (1f - saturation) * 0.2126f },
                 { (1f - saturation) * 0.7152f,              (1f - saturation) * 0.7152f + saturation, (1f - saturation) * 0.7152f },
                 { (1f - saturation) * 0.0722f,              (1f - saturation) * 0.0722f,              (1f - saturation) * 0.0722f + saturation }
            };

            float contrastOffset = (1 - contrast) * 0.5f;

            // Fill the MAGCOLOREFFECT matrix (5x5)
            // Apply saturation, then contrast, then brightness
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    effect.transform[i * 5 + j] = satMatrix[i, j] * contrast * brightness;
                }
            }

            // Alpha channel - keep unchanged
            effect.transform[3 * 5 + 3] = 1.0f;
            effect.transform[3 * 5 + 4] = 0.0f;

            // Row 4 addition
			effect.transform[4 * 5 + 0] = contrastOffset;
            effect.transform[4 * 5 + 1] = contrastOffset;
            effect.transform[4 * 5 + 2] = contrastOffset;
            effect.transform[4 * 5 + 3] = 0.0f;
            effect.transform[4 * 5 + 4] = 1.0f;

            return effect;
        }

        private void BrightnessSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            BrightnessValue = $"{(int)e.NewValue}%";
        }

        private void ContrastSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            ContrastValue = $"{(int)e.NewValue}%";
        }

        private void SaturationSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            SaturationValue = $"{(int)e.NewValue}%";
        }

        private void ApplyButton_Click(object sender, RoutedEventArgs e)
        {
            ApplyColorEffect();
        }

        private void ResetButton_Click(object sender, RoutedEventArgs e)
        {
            // Apply default effect (100% for all), but do not change slider positions
            var effect = CreateColorTransformMatrix(1.0f, 1.0f, 1.0f);
            if (isInitialized)
            {
                MagSetFullscreenColorEffect(ref effect);
            }
        }

        private void MainWindow_Closed(object sender, WindowEventArgs e)
        {
            // Restore original display settings
            if (isInitialized)
            {
                try
                {
                    MagSetFullscreenColorEffect(ref originalEffect);
                    MagUninitialize();
                }
                catch
                {
                    // Ignore cleanup errors
                }
            }
        }
    }
}

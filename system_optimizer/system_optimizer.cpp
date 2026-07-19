/*
 * System Optimizer for LineageOS 16.1 - gtexswifi (SM-T280)
 * Reduces system size to ~500MB
 * 
 * Compile with: g++ -std=c++14 -O2 -Wall -o system_optimizer system_optimizer.cpp -lz -lpthread
 * For Android: Use included Android.mk
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <functional>

// Android logging support
#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "SystemOptimizer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGD(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#define LOGW(...) printf(__VA_ARGS__); printf("\n")
#endif

// System directories
const std::string SYSTEM_PATH = "/system";
const std::string APP_PATH = SYSTEM_PATH + "/app";
const std::string PRIV_APP_PATH = SYSTEM_PATH + "/priv-app";
const std::string LIB_PATH = SYSTEM_PATH + "/lib";
const std::string FRAMEWORK_PATH = SYSTEM_PATH + "/framework";
const std::string FONT_PATH = SYSTEM_PATH + "/fonts";
const std::string MEDIA_PATH = SYSTEM_PATH + "/media";
const std::string ETC_PATH = SYSTEM_PATH + "/etc";
const std::string BIN_PATH = SYSTEM_PATH + "/bin";
const std::string XBIN_PATH = SYSTEM_PATH + "/xbin";

// Target size in bytes (500MB)
const size_t TARGET_SIZE = 500 * 1024 * 1024;

class SystemOptimizer {
private:
    struct FileInfo {
        std::string path;
        size_t size;
        std::string type;
        bool isRemovable;
        float priority;
        time_t modified;
    };

    struct ProgressData {
        int totalFiles;
        int processedFiles;
        size_t totalSize;
        size_t freedSize;
        std::string currentOperation;
    };

    std::vector<FileInfo> systemFiles;
    std::map<std::string, size_t> sizeBreakdown;
    size_t originalSize;
    size_t optimizedSize;
    size_t freedSpace;
    ProgressData progress;
    bool verboseMode;
    bool dryRun;
    std::set<std::string> criticalPaths;
    std::set<std::string> removableApps;
    std::set<std::string> essentialFonts;
    std::map<std::string, std::string> optimizationRules;

public:
    SystemOptimizer() : originalSize(0), optimizedSize(0), freedSpace(0),
                        verboseMode(false), dryRun(false) {
        initializeCriticalPaths();
        initializeRemovableApps();
        initializeEssentialFonts();
        initializeOptimizationRules();
        progress.totalFiles = 0;
        progress.processedFiles = 0;
        progress.totalSize = 0;
        progress.freedSize = 0;
    }

    bool optimizeSystem(const std::string& systemPath = SYSTEM_PATH) {
        LOGI("===================================================");
        LOGI("System Optimizer for LineageOS 16.1 - gtexswifi");
        LOGI("Target size: %.2f MB", (float)TARGET_SIZE / (1024 * 1024));
        LOGI("===================================================");

        // Check for root permissions
        if (getuid() != 0) {
            LOGE("ERROR: This tool requires root permissions");
            return false;
        }

        // Mount system as read-write
        if (!mountSystemRW()) {
            LOGE("Failed to mount system as read-write");
            return false;
        }

        // Scan system directory
        LOGI("Scanning system directory...");
        if (!scanDirectory(systemPath, systemFiles)) {
            LOGE("Failed to scan system directory");
            return false;
        }

        originalSize = calculateTotalSize();
        progress.totalSize = originalSize;
        LOGI("Original system size: %.2f MB", (float)originalSize / (1024 * 1024));

        // Perform optimizations
        std::vector<std::pair<std::string, std::function<bool()>>> optimizations = {
            {"Removing unused applications", [this]() { return removeUnusedApps(); }},
            {"Compressing APK files", [this]() { return compressAPKs(); }},
            {"Optimizing resources", [this]() { return optimizeResources(); }},
            {"Removing debug symbols", [this]() { return removeDebugSymbols(); }},
            {"Stripping native libraries", [this]() { return stripNativeLibraries(); }},
            {"Reducing fonts", [this]() { return reduceFonts(); }},
            {"Cleaning media files", [this]() { return cleanupMediaFiles(); }},
            {"Optimizing framework", [this]() { return optimizeFramework(); }},
            {"Cleaning temporary files", [this]() { return cleanTemporaryFiles(); }},
            {"Removing unused locales", [this]() { return removeUnusedLocales(); }},
            {"Optimizing boot animation", [this]() { return optimizeBootAnimation(); }}
        };

        for (const auto& opt : optimizations) {
            progress.currentOperation = opt.first;
            LOGI("\n>>> %s...", opt.first.c_str());
            
            if (!opt.second()) {
                LOGW("Warning: %s had issues, continuing...", opt.first.c_str());
            }
        }

        // Calculate final size
        optimizedSize = calculateTotalSize();
        freedSpace = originalSize - optimizedSize;

        // Generate report
        generateSizeReport();
        printReport();

        // Mount system as read-only
        mountSystemRO();

        return optimizedSize <= TARGET_SIZE;
    }

private:
    void initializeCriticalPaths() {
        criticalPaths = {
            "/system/bin/sh",
            "/system/bin/init",
            "/system/bin/linker",
            "/system/bin/app_process",
            "/system/bin/surfaceflinger",
            "/system/bin/netd",
            "/system/bin/vold",
            "/system/bin/mediaserver",
            "/system/bin/installd",
            "/system/bin/dalvikvm",
            "/system/lib/libc.so",
            "/system/lib/libm.so",
            "/system/lib/libdl.so",
            "/system/lib/liblog.so",
            "/system/lib/libcutils.so",
            "/system/lib/libutils.so",
            "/system/lib/libbinder.so",
            "/system/lib/libsurfaceflinger.so",
            "/system/lib/libandroid_runtime.so",
            "/system/lib/libnativehelper.so",
            "/system/lib/libicuuc.so",
            "/system/lib/libicui18n.so",
            "/system/framework/framework.jar",
            "/system/framework/core.jar",
            "/system/framework/ext.jar",
            "/system/framework/services.jar",
            "/system/framework/telephony-common.jar",
            "/system/framework/voip-common.jar",
            "/system/framework/ims-common.jar",
            "/system/framework/mms-common.jar",
            "/system/framework/android.policy.jar"
        };
    }

    void initializeRemovableApps() {
        // Apps safe to remove on gtexswifi (SM-T280)
        removableApps = {
            "Email", "Exchange2", "Galaxy4", "HoloSpiralWallpaper",
            "LiveWallpapers", "MagicSmokeWallpapers", "NoiseField",
            "PhaseBeam", "PhotoTable", "PicoTts", "QuickSearchBox",
            "SoundRecorder", "Stk", "VisualizationWallpapers",
            "BasicDreams", "PrintSpooler", "WAPPushManager",
            "CloudPrint", "DeskClock", "EasterEgg",
            "HTMLViewer", "InputDevices", "KeyChain",
            "LiveWallpapersPicker", "MusicFX", "NfcNci",
            "PacProcessor", "PhaseBeam", "SamsungDoze",
            "SamsungTTS", "SoundRecorder", "Spherical",
            "Calendar", "Calculator", "Browser", "Camera",
            "Gallery2", "Music", "VideoPlayer", "DownloadProviderUi",
            "DrmProvider", "SharedStorageBackup", "CertInstaller",
            "DefaultContainerService", "BackupRestoreConfirmation",
            "BluetoothMidiService", "CaptivePortalLogin",
            "CarrierConfig", "CellBroadcastReceiver", "CtsShim",
            "CtsShimPriv", "Development", "DeviceHandler",
            "DmService", "DocumentsUI", "ExternalStorageProvider",
            "FusedLocation", "InputDevices", "KeyChain",
            "ManagedProvisioning", "MmsService", "NetworkStack",
            "OpenWnn", "PacProcessor", "ProxyHandler",
            "Shell", "Stk", "Tag", "TelephonyProvider",
            "Traceur", "UserDictionaryProvider", "VpnDialogs",
            "WallpaperBackup", "WifiDisplay"
        };
    }

    void initializeEssentialFonts() {
        essentialFonts = {
            "Roboto-Regular.ttf",
            "Roboto-Bold.ttf",
            "Roboto-Italic.ttf",
            "Roboto-BoldItalic.ttf",
            "Roboto-Medium.ttf",
            "Roboto-MediumItalic.ttf",
            "DroidSans.ttf",
            "DroidSans-Bold.ttf",
            "NotoColorEmoji.ttf",
            "NotoSans-Regular.ttf",
            "NotoSans-Bold.ttf"
        };
    }

    void initializeOptimizationRules() {
        optimizationRules = {
            {"compress_images", "convert {input} -quality 60 {output}"},
            {"compress_apk", "zipalign -p 4 {input} {output}"},
            {"strip_binary", "strip --strip-debug {input}"},
            {"remove_locale", "find {path} -name '*_{locale}*' -delete"},
            {"optimize_png", "pngcrush -brute -reduce {input} {output}"}
        };
    }

    bool mountSystemRW() {
        std::string cmd = "mount -o remount,rw /system 2>/dev/null";
        int result = system(cmd.c_str());
        if (result != 0) {
            // Try alternative mount
            cmd = "mount -o rw,remount /system";
            result = system(cmd.c_str());
        }
        return result == 0;
    }

    bool mountSystemRO() {
        std::string cmd = "mount -o remount,ro /system 2>/dev/null";
        int result = system(cmd.c_str());
        if (result != 0) {
            cmd = "mount -o ro,remount /system";
            result = system(cmd.c_str());
        }
        return result == 0;
    }

    bool scanDirectory(const std::string& dir, std::vector<FileInfo>& files) {
        DIR* d = opendir(dir.c_str());
        if (!d) {
            return false;
        }

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            if (entry->d_name[0] == '.') continue;
            
            std::string fullPath = dir + "/" + entry->d_name;
            struct stat st;
            
            if (stat(fullPath.c_str(), &st) == 0) {
                FileInfo info;
                info.path = fullPath;
                info.size = st.st_size;
                info.modified = st.st_mtime;
                
                if (S_ISDIR(st.st_mode)) {
                    // Recursively scan subdirectories
                    scanDirectory(fullPath, files);
                    continue;
                }
                
                // Determine file type
                if (fullPath.find(".apk") != std::string::npos) {
                    info.type = "application/vnd.android.package-archive";
                } else if (fullPath.find(".so") != std::string::npos) {
                    info.type = "application/x-sharedlib";
                } else if (fullPath.find(".jar") != std::string::npos) {
                    info.type = "application/java-archive";
                } else if (fullPath.find(".ttf") != std::string::npos || 
                           fullPath.find(".otf") != std::string::npos) {
                    info.type = "font/ttf";
                } else if (fullPath.find(".png") != std::string::npos ||
                           fullPath.find(".jpg") != std::string::npos ||
                           fullPath.find(".jpeg") != std::string::npos) {
                    info.type = "image";
                } else if (st.st_mode & S_IXUSR) {
                    info.type = "application/x-executable";
                } else {
                    info.type = "application/octet-stream";
                }
                
                info.isRemovable = !isSystemCritical(fullPath);
                info.priority = calculatePriority(fullPath);
                
                files.push_back(info);
                progress.totalFiles++;
            }
        }
        
        closedir(d);
        return true;
    }

    bool isSystemCritical(const std::string& path) {
        for (const auto& critical : criticalPaths) {
            if (path.find(critical) != std::string::npos) {
                return true;
            }
        }
        
        // Check if it's a core system component
        if (path.find("/system/framework/") != std::string::npos) {
            if (path.find("android") != std::string::npos ||
                path.find("javax") != std::string::npos ||
                path.find("apache") != std::string::npos ||
                path.find("bouncycastle") != std::string::npos ||
                path.find("conscrypt") != std::string::npos ||
                path.find("core") != std::string::npos ||
                path.find("okhttp") != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }

    float calculatePriority(const std::string& path) {
        float priority = 0.5f;
        
        if (isSystemCritical(path)) {
            priority = 1.0f;
        } else if (path.find("/system/app/") != std::string::npos) {
            if (path.find("Settings") != std::string::npos ||
                path.find("SystemUI") != std::string::npos ||
                path.find("Launcher") != std::string::npos) {
                priority = 0.9f;
            } else {
                priority = 0.3f;
            }
        } else if (path.find("/system/priv-app/") != std::string::npos) {
            priority = 0.7f;
        } else if (path.find("/system/lib/") != std::string::npos) {
            priority = 0.8f;
        } else if (path.find("/system/fonts/") != std::string::npos) {
            priority = 0.4f;
        } else if (path.find("/system/media/") != std::string::npos) {
            priority = 0.2f;
        } else if (path.find("/system/etc/") != std::string::npos) {
            priority = 0.3f;
        }
        
        return priority;
    }

    size_t calculateTotalSize() {
        size_t total = 0;
        systemFiles.clear();
        scanDirectory(SYSTEM_PATH, systemFiles);
        for (const auto& file : systemFiles) {
            total += file.size;
        }
        return total;
    }

    bool removeFile(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would remove: %s", path.c_str());
            return true;
        }

        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            return false;
        }
        
        if (S_ISDIR(st.st_mode)) {
            DIR* d = opendir(path.c_str());
            if (!d) return false;
            
            struct dirent* entry;
            while ((entry = readdir(d)) != nullptr) {
                if (entry->d_name[0] == '.') continue;
                std::string fullPath = path + "/" + entry->d_name;
                removeFile(fullPath);
            }
            closedir(d);
            return rmdir(path.c_str()) == 0;
        } else {
            if (unlink(path.c_str()) == 0) {
                freedSpace += st.st_size;
                LOGD("Removed: %s (%.2f KB)", path.c_str(), (float)st.st_size / 1024);
                return true;
            }
            return false;
        }
    }

    bool executeCommand(const std::string& cmd) {
        if (dryRun) {
            LOGD("[DRY RUN] Would execute: %s", cmd.c_str());
            return true;
        }
        return system(cmd.c_str()) == 0;
    }

    // Optimization functions
    bool removeUnusedApps() {
        LOGI("Removing unused applications...");
        size_t removed = 0;
        
        for (const auto& app : removableApps) {
            std::string appPath = APP_PATH + "/" + app;
            if (directoryExists(appPath)) {
                if (removeFile(appPath)) {
                    removed++;
                    LOGD("Removed system app: %s", app.c_str());
                }
            }
            
            appPath = PRIV_APP_PATH + "/" + app;
            if (directoryExists(appPath)) {
                if (removeFile(appPath)) {
                    removed++;
                    LOGD("Removed system priv-app: %s", app.c_str());
                }
            }
        }
        
        LOGI("Removed %zu unused applications", removed);
        return true;
    }

    bool compressAPKs() {
        LOGI("Compressing APK files...");
        size_t compressed = 0;
        
        std::vector<std::string> apkDirs = {APP_PATH, PRIV_APP_PATH};
        for (const auto& dir : apkDirs) {
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    if (file.path.find(".apk") != std::string::npos) {
                        if (compressFile(file.path)) {
                            compressed++;
                            progress.processedFiles++;
                        }
                    }
                }
            }
        }
        
        LOGI("Compressed %zu APK files", compressed);
        return true;
    }

    bool compressFile(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would compress: %s", path.c_str());
            return true;
        }

        std::string cmd = "gzip -9 " + path + " 2>/dev/null";
        if (system(cmd.c_str()) == 0) {
            return true;
        }
        return false;
    }

    bool optimizeResources() {
        LOGI("Optimizing system resources...");
        
        std::vector<std::string> resourceDirs = {
            FRAMEWORK_PATH,
            ETC_PATH,
            "/system/usr"
        };

        for (const auto& dir : resourceDirs) {
            if (!directoryExists(dir)) continue;
            
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    if (file.path.find("_") != std::string::npos) {
                        std::string lang = file.path.substr(file.path.find("_") + 1);
                        if (lang.find("en") != 0 && 
                            lang.find("es") != 0 && 
                            lang.find("fr") != 0 && 
                            lang.find("de") != 0 &&
                            lang.find("zh") != 0 &&
                            lang.find("ja") != 0 &&
                            lang.find("ko") != 0) {
                            if (lang.length() > 5) {
                                removeFile(file.path);
                            }
                        }
                    }
                }
            }
        }

        optimizePNGFiles();
        return true;
    }

    bool optimizePNGFiles() {
        LOGI("Optimizing PNG files...");
        size_t optimized = 0;
        
        std::vector<std::string> imageDirs = {
            "/system/framework",
            "/system/app",
            "/system/priv-app",
            "/system/media"
        };

        for (const auto& dir : imageDirs) {
            if (!directoryExists(dir)) continue;
            
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    if (file.path.find(".png") != std::string::npos) {
                        if (optimizePNG(file.path)) {
                            optimized++;
                        }
                    }
                }
            }
        }
        
        LOGI("Optimized %zu PNG files", optimized);
        return true;
    }

    bool optimizePNG(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would optimize PNG: %s", path.c_str());
            return true;
        }

        std::string cmd = "which pngcrush > /dev/null 2>&1";
        if (system(cmd.c_str()) == 0) {
            cmd = "pngcrush -brute -reduce " + path + " " + path + ".tmp 2>/dev/null";
            if (system(cmd.c_str()) == 0) {
                cmd = "mv " + path + ".tmp " + path;
                system(cmd.c_str());
                return true;
            }
        } else {
            cmd = "convert " + path + " -quality 80 " + path + " 2>/dev/null";
            if (system(cmd.c_str()) == 0) {
                return true;
            }
        }
        return false;
    }

    bool removeDebugSymbols() {
        LOGI("Removing debug symbols from binaries...");
        size_t stripped = 0;
        
        std::vector<std::string> binDirs = {
            BIN_PATH,
            XBIN_PATH,
            LIB_PATH,
            "/system/lib64"
        };

        for (const auto& dir : binDirs) {
            if (!directoryExists(dir)) continue;
            
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    if (file.type == "application/x-executable" ||
                        file.type == "application/x-sharedlib" ||
                        file.path.find(".so") != std::string::npos) {
                        if (stripBinary(file.path)) {
                            stripped++;
                        }
                    }
                }
            }
        }

        LOGI("Stripped %zu binaries", stripped);
        return true;
    }

    bool stripBinary(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would strip: %s", path.c_str());
            return true;
        }

        std::string cmd = "strip --strip-debug " + path + " 2>/dev/null";
        return system(cmd.c_str()) == 0;
    }

    bool stripNativeLibraries() {
        LOGI("Stripping native libraries...");
        size_t stripped = 0;

        std::vector<std::string> archDirs = {
            "/system/lib/arm64",
            "/system/lib/x86",
            "/system/lib/x86_64",
            "/system/lib64",
            "/system/vendor/lib/arm64",
            "/system/vendor/lib/x86"
        };

        for (const auto& dir : archDirs) {
            if (directoryExists(dir)) {
                if (removeFile(dir)) {
                    stripped++;
                    LOGD("Removed architecture directory: %s", dir.c_str());
                }
            }
        }

        std::vector<std::string> libsToRemove = {
            "libart.so.debug",
            "libjavacore.so.debug",
            "libopenjdkjvm.so.debug",
            "libwebviewchromium.so.debug",
            "libchromium_net.so",
            "libchrome.so"
        };

        for (const auto& lib : libsToRemove) {
            std::string libPath = LIB_PATH + "/" + lib;
            if (fileExists(libPath)) {
                if (removeFile(libPath)) {
                    stripped++;
                }
            }
        }

        LOGI("Stripped/removed %zu native libraries", stripped);
        return true;
    }

    bool reduceFonts() {
        LOGI("Reducing font files...");
        size_t removed = 0;

        if (!directoryExists(FONT_PATH)) {
            LOGW("Font directory not found: %s", FONT_PATH.c_str());
            return false;
        }

        std::vector<FileInfo> fontFiles;
        if (!scanDirectory(FONT_PATH, fontFiles)) {
            return false;
        }

        for (const auto& file : fontFiles) {
            if (file.path.find(".ttf") != std::string::npos ||
                file.path.find(".otf") != std::string::npos) {
                
                bool isEssential = false;
                for (const auto& essential : essentialFonts) {
                    if (file.path.find(essential) != std::string::npos) {
                        isEssential = true;
                        break;
                    }
                }
                
                if (file.path.find("Noto") != std::string::npos) {
                    isEssential = true;
                }
                
                if (!isEssential) {
                    if (removeFile(file.path)) {
                        removed++;
                        LOGD("Removed non-essential font: %s", file.path.c_str());
                    }
                }
            }
        }

        LOGI("Removed %zu non-essential font files", removed);
        return true;
    }

    bool cleanupMediaFiles() {
        LOGI("Cleaning up media files...");
        size_t cleaned = 0;

        if (directoryExists(MEDIA_PATH + "/audio")) {
            std::vector<std::string> audioSubDirs = {
                "ringtones",
                "notifications",
                "alarms",
                "ui"
            };

            for (const auto& subDir : audioSubDirs) {
                std::string fullPath = MEDIA_PATH + "/audio/" + subDir;
                if (!directoryExists(fullPath)) continue;

                std::vector<FileInfo> audioFiles;
                if (scanDirectory(fullPath, audioFiles)) {
                    for (const auto& file : audioFiles) {
                        if (file.path.find("default") == std::string::npos &&
                            file.path.find("Default") == std::string::npos &&
                            file.path.find("standard") == std::string::npos &&
                            file.path.find("Standard") == std::string::npos) {
                            if (file.size > 100 * 1024) {
                                if (removeFile(file.path)) {
                                    cleaned++;
                                }
                            }
                        }
                    }
                }
            }
        }

        std::vector<FileInfo> imageFiles;
        if (scanDirectory(MEDIA_PATH, imageFiles)) {
            for (const auto& file : imageFiles) {
                if (file.path.find(".jpg") != std::string::npos ||
                    file.path.find(".jpeg") != std::string::npos ||
                    file.path.find(".png") != std::string::npos) {
                    if (file.size > 1024 * 1024) {
                        compressImage(file.path);
                        cleaned++;
                    }
                }
            }
        }

        LOGI("Cleaned up %zu media files", cleaned);
        return true;
    }

    bool compressImage(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would compress image: %s", path.c_str());
            return true;
        }

        std::string cmd = "convert " + path + " -quality 60 " + path + " 2>/dev/null";
        if (system(cmd.c_str()) == 0) {
            return true;
        }
        return false;
    }

    bool optimizeFramework() {
        LOGI("Optimizing framework...");
        size_t optimized = 0;

        if (!directoryExists(FRAMEWORK_PATH)) {
            LOGW("Framework directory not found");
            return false;
        }

        std::vector<FileInfo> frameworkFiles;
        if (scanDirectory(FRAMEWORK_PATH, frameworkFiles)) {
            for (const auto& file : frameworkFiles) {
                if (file.path.find(".jar") != std::string::npos) {
                    if (optimizeJar(file.path)) {
                        optimized++;
                    }
                }
            }
        }

        LOGI("Optimized %zu framework files", optimized);
        return true;
    }

    bool optimizeJar(const std::string& path) {
        if (dryRun) {
            LOGD("[DRY RUN] Would optimize JAR: %s", path.c_str());
            return true;
        }

        std::string cmd = "zip -d " + path + " '*/META-INF/*.SF' '*/META-INF/*.RSA' '*/META-INF/*.DSA' 2>/dev/null";
        system(cmd.c_str());

        cmd = "gzip -9 " + path + " 2>/dev/null";
        if (system(cmd.c_str()) == 0) {
            return true;
        }
        return false;
    }

    bool cleanTemporaryFiles() {
        LOGI("Cleaning temporary files...");
        size_t cleaned = 0;

        std::vector<std::string> tempDirs = {
            "/data/local/tmp",
            "/cache",
            "/tmp"
        };

        for (const auto& dir : tempDirs) {
            if (!directoryExists(dir)) continue;
            
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    time_t now = time(nullptr);
                    if (now - file.modified > 7 * 24 * 60 * 60) {
                        if (removeFile(file.path)) {
                            cleaned++;
                        }
                    }
                }
            }
        }

        LOGI("Cleaned %zu temporary files", cleaned);
        return true;
    }

    bool removeUnusedLocales() {
        LOGI("Removing unused locales...");
        size_t removed = 0;

        std::vector<std::string> localeDirs = {
            "/system/usr/share/i18n/locales",
            "/system/etc/locales",
            "/system/usr/share/locale"
        };

        std::vector<std::string> keepLocales = {
            "en", "en_US", "en_GB",
            "es", "es_ES",
            "fr", "fr_FR",
            "de", "de_DE",
            "zh", "zh_CN",
            "ja", "ja_JP",
            "ko", "ko_KR"
        };

        for (const auto& dir : localeDirs) {
            if (!directoryExists(dir)) continue;
            
            std::vector<FileInfo> files;
            if (scanDirectory(dir, files)) {
                for (const auto& file : files) {
                    bool keep = false;
                    for (const auto& locale : keepLocales) {
                        if (file.path.find(locale) != std::string::npos) {
                            keep = true;
                            break;
                        }
                    }
                    
                    if (!keep && !isSystemCritical(file.path)) {
                        if (removeFile(file.path)) {
                            removed++;
                        }
                    }
                }
            }
        }

        LOGI("Removed %zu unused locales", removed);
        return true;
    }

    bool optimizeBootAnimation() {
        LOGI("Optimizing boot animation...");
        
        std::string bootAnimPath = MEDIA_PATH + "/bootanimation.zip";
        if (!fileExists(bootAnimPath)) {
            LOGW("Boot animation not found");
            return false;
        }

        if (dryRun) {
            LOGD("[DRY RUN] Would optimize boot animation");
            return true;
        }

        std::string tmpDir = "/tmp/bootanim";
        mkdir(tmpDir.c_str(), 0755);
        
        std::string cmd = "unzip -q " + bootAnimPath + " -d " + tmpDir + " 2>/dev/null";
        if (system(cmd.c_str()) != 0) {
            return false;
        }

        cmd = "find " + tmpDir + " -name '*.png' -exec convert {} -quality 60 {} \\; 2>/dev/null";
        system(cmd.c_str());

        cmd = "cd " + tmpDir + " && zip -r -q " + bootAnimPath + " . 2>/dev/null";
        system(cmd.c_str());

        cmd = "rm -rf " + tmpDir;
        system(cmd.c_str());

        LOGI("Boot animation optimized");
        return true;
    }

    void generateSizeReport() {
        sizeBreakdown.clear();
        
        size_t appSize = 0, libSize = 0, frameworkSize = 0;
        size_t fontSize = 0, mediaSize = 0, etcSize = 0;
        size_t binSize = 0, otherSize = 0;

        for (const auto& file : systemFiles) {
            if (file.path.find("/app/") != std::string::npos ||
                file.path.find("/priv-app/") != std::string::npos) {
                appSize += file.size;
            } else if (file.path.find("/lib") != std::string::npos) {
                libSize += file.size;
            } else if (file.path.find("/framework") != std::string::npos) {
                frameworkSize += file.size;
            } else if (file.path.find("/fonts") != std::string::npos) {
                fontSize += file.size;
            } else if (file.path.find("/media") != std::string::npos) {
                mediaSize += file.size;
            } else if (file.path.find("/etc") != std::string::npos) {
                etcSize += file.size;
            } else if (file.path.find("/bin") != std::string::npos ||
                       file.path.find("/xbin") != std::string::npos) {
                binSize += file.size;
            } else {
                otherSize += file.size;
            }
        }
        
        sizeBreakdown["Applications"] = appSize;
        sizeBreakdown["Libraries"] = libSize;
        sizeBreakdown["Framework"] = frameworkSize;
        sizeBreakdown["Fonts"] = fontSize;
        sizeBreakdown["Media"] = mediaSize;
        sizeBreakdown["Configuration"] = etcSize;
        sizeBreakdown["Binaries"] = binSize;
        sizeBreakdown["Other"] = otherSize;
    }

    void printReport() {
        LOGI("\n===================================================");
        LOGI("OPTIMIZATION REPORT");
        LOGI("===================================================");
        LOGI("Original size:  %.2f MB", (float)originalSize / (1024 * 1024));
        LOGI("Optimized size: %.2f MB", (float)optimizedSize / (1024 * 1024));
        LOGI("Space freed:    %.2f MB", (float)freedSpace / (1024 * 1024));
        LOGI("Reduction:      %.1f%%", ((float)freedSpace / originalSize) * 100.0f);
        LOGI("---------------------------------------------------");
        
        if (optimizedSize <= TARGET_SIZE) {
            LOGI("✓ SUCCESS: System size is within target (%.2f MB / %.2f MB)",
                 (float)optimizedSize / (1024 * 1024),
                 (float)TARGET_SIZE / (1024 * 1024));
        } else {
            LOGI("⚠ WARNING: System size exceeds target (%.2f MB / %.2f MB)",
                 (float)optimizedSize / (1024 * 1024),
                 (float)TARGET_SIZE / (1024 * 1024));
            LOGI("Additional manual optimization may be needed");
        }
        
        LOGI("---------------------------------------------------");
        LOGI("Size breakdown by category:");
        
        for (const auto& [category, size] : sizeBreakdown) {
            if (size > 0) {
                LOGI("  %-15s: %8.2f MB (%5.1f%%)",
                     category.c_str(),
                     (float)size / (1024 * 1024),
                     ((float)size / originalSize) * 100.0f);
            }
        }
        
        LOGI("===================================================");
        LOGI("Files processed: %d", progress.totalFiles);
        LOGI("Optimization completed at: %s", getCurrentTime().c_str());
        LOGI("===================================================");
    }

    std::string getCurrentTime() {
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    bool fileExists(const std::string& path) {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    bool directoryExists(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

public:
    void setVerbose(bool verbose) { verboseMode = verbose; }
    void setDryRun(bool dry) { dryRun = dry; }
    
    size_t getOriginalSize() const { return originalSize; }
    size_t getOptimizedSize() const { return optimizedSize; }
    size_t getFreedSpace() const { return freedSpace; }
    float getReductionPercentage() const {
        if (originalSize == 0) return 0.0f;
        return ((float)freedSpace / originalSize) * 100.0f;
    }
};

// Main entry point
int main(int argc, char* argv[]) {
    SystemOptimizer optimizer;
    
    bool verbose = false;
    bool dryRun = false;
    std::string systemPath = SYSTEM_PATH;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
            optimizer.setVerbose(true);
        } else if (arg == "-d" || arg == "--dry-run") {
            dryRun = true;
            optimizer.setDryRun(true);
            LOGI("DRY RUN MODE - No changes will be made");
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "System Optimizer for LineageOS 16.1 - gtexswifi (SM-T280)" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: " << argv[0] << " [OPTIONS] [PATH]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -v, --verbose   Enable verbose output" << std::endl;
            std::cout << "  -d, --dry-run   Dry run (no changes made)" << std::endl;
            std::cout << "  -h, --help      Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Default path: " << SYSTEM_PATH << std::endl;
            return 0;
        } else if (arg[0] != '-') {
            systemPath = arg;
        }
    }

    LOGI("Starting System Optimizer for gtexswifi");
    if (dryRun) {
        LOGI("DRY RUN MODE ACTIVE - No files will be modified");
    }
    LOGI("System path: %s", systemPath.c_str());

    bool success = optimizer.optimizeSystem(systemPath);
    
    if (success) {
        LOGI("\n✓ Optimization completed successfully!");
        LOGI("System size reduced to: %.2f MB",
             (float)optimizer.getOptimizedSize() / (1024 * 1024));
    } else {
        LOGI("\n⚠ Optimization completed with warnings");
        LOGI("Final system size: %.2f MB (target: %.2f MB)",
             (float)optimizer.getOptimizedSize() / (1024 * 1024),
             (float)TARGET_SIZE / (1024 * 1024));
        if (dryRun) {
            LOGI("DRY RUN: No actual changes were made");
        }
    }

    return success ? 0 : 1;
}

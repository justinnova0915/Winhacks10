#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib> // for system()
namespace fs = std::filesystem;

int main() {
    std::string inputDir;
    std::cout << "Enter source directory: ";
    std::getline(std::cin, inputDir);

    fs::path srcPath = inputDir;
    if (!fs::exists(srcPath) || !fs::is_directory(srcPath)) {
        std::cerr << "Invalid directory!\n";
        return 1;
    }

    fs::path outputRoot = srcPath.parent_path() / "converted";

    try {
        for (auto& entry : fs::recursive_directory_iterator(srcPath)) {
            if (fs::is_regular_file(entry)) {
                fs::path relPath = fs::relative(entry.path(), srcPath);
                fs::path outputPath = outputRoot / relPath;
                outputPath.replace_extension(".bmp"); // force .bmp

                // Make sure the parent directory exists
                fs::create_directories(outputPath.parent_path());

                // Construct ffmpeg command
                std::string cmd = "ffmpeg -y -i \"" + entry.path().string() +
                                  "\" -pix_fmt bgr24 -f image2 \"" +
                                  outputPath.string() + "\"";

                std::cout << "Converting: " << entry.path() << "\n";
                int ret = std::system(cmd.c_str());
                if (ret != 0) {
                    std::cerr << "Failed to convert: " << entry.path() << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Conversion finished! Output folder: " << outputRoot << "\n";
    return 0;
}

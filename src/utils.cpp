#include "utils.h"

#include <curl/curl.h>
#include <iostream>

std::string DEBAR::Utils::format_size(size_t size)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double display_size = static_cast<double>(size);
    while (display_size >= 1024 && unit_index < 4) {
        display_size /= 1024;
        unit_index++;
    }
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2f %s", display_size, units[unit_index]);
    return std::string(buffer);
}

std::vector<std::string> DEBAR::Utils::split_str(const std::string &str, const std::string &split)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(split);
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + split.length();
        end = str.find(split, start);
    }
    result.push_back(str.substr(start, end));
    return result;
}

int progress_callback(void* ptr, curl_off_t total_to_download, curl_off_t now_downloaded, curl_off_t total_to_upload, curl_off_t now_uploaded)
{
    if (total_to_download > 0) {
        std::cout << (char*)ptr << " - " << (now_downloaded * 100 / total_to_download) << "% ("
                  << DEBAR::Utils::format_size(now_downloaded) << " / " << DEBAR::Utils::format_size(total_to_download) << ") \r" << std::flush;
    }
    return 0;
}

bool DEBAR::Utils::download_file(const std::string &url, const std::string &path, const char* prefix)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char errorBuffer[CURL_ERROR_SIZE];
    
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(path.c_str(), "wb");
        if (!fp) {
            std::cerr << "Failed to open file: " << path << std::endl;
            return false;
        }
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "User-Agent: Debian APT-HTTP/1.3 (1.0.1ubuntu2)");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // 启用自定义进度输出
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, prefix);
        
        res = curl_easy_perform(curl);
        std::cout << std::endl;
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << errorBuffer << std::endl;
            fclose(fp);
            curl_easy_cleanup(curl);
            return false;
        }

        fclose(fp);
        curl_easy_cleanup(curl);
        
        return true;
    }
    return false;
}

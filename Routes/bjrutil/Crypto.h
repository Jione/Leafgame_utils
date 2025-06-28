#pragma once
#include <vector>
#include <string>

// BJR → BMP: rawData(암호화된 바이트), filename(원본 .bjr 파일명)
std::vector<uint8_t> decryptData(const std::vector<uint8_t>& rawData, const std::string& filename);

// BMP → BJR: rawData(평범한 BMP 픽셀), filename(출력 .bjr 파일명)
std::vector<uint8_t> encryptData(const std::vector<uint8_t>& rawData, const std::string& filename);
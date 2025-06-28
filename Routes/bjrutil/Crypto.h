#pragma once
#include <vector>
#include <string>

// BJR �� BMP: rawData(��ȣȭ�� ����Ʈ), filename(���� .bjr ���ϸ�)
std::vector<uint8_t> decryptData(const std::vector<uint8_t>& rawData, const std::string& filename);

// BMP �� BJR: rawData(����� BMP �ȼ�), filename(��� .bjr ���ϸ�)
std::vector<uint8_t> encryptData(const std::vector<uint8_t>& rawData, const std::string& filename);
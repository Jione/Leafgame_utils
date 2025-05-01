#pragma once
#include <string>
#include <vector>

// ġȯ ���̺� �˻��� �Լ�
const std::string* find_replacement_string(uint16_t hex);

// ġȯ ���̺� ���˻��� �Լ�
int find_hexcode_from_text(const std::string& utf8text);

// ġȯ ���̺� �ʱ�ȭ (UTF-8 ��ȯ ������)
void initialize_replacement_table(const bool isToCSV);
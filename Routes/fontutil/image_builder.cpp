#include "image_builder.h"
#include "lodepng.h"  // lodepng ��� �¸� ���̺귯��

std::vector<unsigned char> create_empty_image(int width, int height) {
    std::vector<unsigned char> image(width * height * 4, 0); // �ʱⰪ 0 = ���� ����
    return image;
}

bool save_image_as_png(const std::string& filename, const std::vector<unsigned char>& image, int width, int height) {
    unsigned error = lodepng::encode(filename, image, width, height);
    if (error) {
        printf("[LodePNG] Encoding error %u: %s\n", error, lodepng_error_text(error));
        return false;
    }
    return true;
}
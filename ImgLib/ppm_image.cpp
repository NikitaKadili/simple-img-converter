#include "ppm_image.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const string_view PPM_SIG = "P6"sv;
static const int PPM_MAX = 255;

/**
 * Сохраняет изображение в file
*/
bool SavePPM(const Path& file, const Image& image) {
    // Если изображение пустое - возвращаем false
    if (!image) {
        return false;
    }

    // Создаем/открываем выходной файл
    ofstream ofs(file, ios::binary);

    // Возвращаем false в случае неудачи создания/открытия file
    if (!ofs) {
        return false;
    }

    // Записываем заголовок
    ofs << PPM_SIG << '\n' 
        << image.GetWidth() << ' ' << image.GetHeight() << '\n' 
        << PPM_MAX << '\n';

    std::vector<char> buff(image.GetWidth() * 3);
    // Итерируемся по строкам (y)
    for (int y = 0; y < image.GetHeight(); ++y) {
        const Color* line = image.GetLine(y);

        // Итерируемся по колонкам (x)
        for (int x = 0; x < image.GetWidth(); ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].r);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].b);
        }

        // Записываем строку из буфера в файл
        ofs.write(buff.data(), image.GetWidth() * 3);
    }

    return true;
}

Image LoadPPM(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать даные в двоичном формате
    ifstream ifs(file, ios::binary);
    std::string sign;
    int w, h, color_max;

    // читаем заголовок: он содержит формат, размеры изображения
    // и максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (sign != PPM_SIG || color_max != PPM_MAX) {
        return {};
    }

    // пропускаем один байт - это конец строки
    const char next = ifs.get();
    if (next != '\n') {
        return {};
    }

    Image result(w, h, Color::Black());
    std::vector<char> buff(w * 3);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), w * 3);

        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].b = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib

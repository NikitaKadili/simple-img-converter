#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace img_lib {

/**
 * Define BMP format supported constants
*/

static const string BMP_SIGN = "BM"s;

static const uint16_t BMP_LEVELS_NUM = 1;
static const uint16_t BMP_BITS_PER_PIXEL = 24;
static const uint16_t BMP_COMPRESS_TYPE = 0;

static const int32_t BMP_HOR_AND_VER_PIXEL_PER_METER = 11811; // ~300 DPI
static const int32_t BMP_USED_COLORS = 0;
static const int32_t BMP_SIGNIFICENT_COLORS = 0x1000000;

/**
 * Первая часть заголовка
*/
PACKED_STRUCT_BEGIN BitmapFileHeader {
    string sign; // Подпись — 2 байта
    uint32_t header_and_data_size; // Суммарный размер заголовка и данных — 4 байта
    uint32_t reserved_area; // Зарезервированное пространство — 4 байта
    uint32_t padding_from_files_begining; // Отступ данных от начала файла — 4 байта
}
PACKED_STRUCT_END

/**
 * Вторая часть заголовка
*/
PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t inf_header_size; // Размер заголовка — 4 байта
    int32_t image_width; // Ширина изображения в пикселях — 4 байта
    int32_t image_height; // Высота изображения в пикселях — 4 байта
    uint16_t levels_num; // Количество плоскостей — 2 байта
    uint16_t bits_per_pixel; // Количество бит на пиксель — 2 байта
    uint32_t compress_type; // Тип сжатия — 4 байта
    uint32_t bytes_in_data; // Количество байт в данных — 4 байта
    int32_t hor_pixel_per_meter; // Горизонтальное разрешение, пикселей на метр — 4 байта
    int32_t ver_pixel_per_meter; // Вертикальное разрешение, пикселей на метр — 4 байта
    int32_t used_colors; // Количество использованных цветов — 4 байта
    int32_t significant_colors; // Количество значимых цветов — 4 байта
}
PACKED_STRUCT_END

/**
 * Функция вычисления отступа по ширине
*/
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

/**
 * Сохраняет изображение в формате .bmp в файл file
*/
bool SaveBMP(const Path& file, const Image& image) {
    // Если изображение пустое - возвращаем false
    if (!image) {
        return false;
    }

    ofstream ofs(file, ios::binary);

    // Возвращаем false в случае неудачи создания/открытия file
    if (!ofs) {
        return false;
    }

    // Размер с учетом отступа
    int padding = GetBMPStride(image.GetWidth());

    // Создадим и запишем первую часть заголовка
    BitmapFileHeader file_header = { BMP_SIGN, static_cast<uint32_t>(padding * image.GetHeight()), 0, 54 };

    ofs.write(file_header.sign.data(), 2);
    ofs.write(reinterpret_cast<char*>(&file_header.header_and_data_size), 4);
    ofs.write(reinterpret_cast<char*>(&file_header.reserved_area), 4);
    ofs.write(reinterpret_cast<char*>(&file_header.padding_from_files_begining), 4);

    // Создадим и запишем вторую часть заголовка
    BitmapInfoHeader info_header = { 40,
        image.GetWidth(), image.GetHeight(),
        BMP_LEVELS_NUM, BMP_BITS_PER_PIXEL, BMP_COMPRESS_TYPE,
        static_cast<uint32_t>(padding * image.GetHeight()),
        BMP_HOR_AND_VER_PIXEL_PER_METER, BMP_HOR_AND_VER_PIXEL_PER_METER,
        BMP_USED_COLORS, BMP_SIGNIFICENT_COLORS
    };

    ofs.write(reinterpret_cast<char*>(&info_header.inf_header_size), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.image_width), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.image_height), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.levels_num), 2);
    ofs.write(reinterpret_cast<char*>(&info_header.bits_per_pixel), 2);
    ofs.write(reinterpret_cast<char*>(&info_header.compress_type), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.bytes_in_data), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.hor_pixel_per_meter), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.ver_pixel_per_meter), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.used_colors), 4);
    ofs.write(reinterpret_cast<char*>(&info_header.significant_colors), 4);

    std::vector<char> buff(image.GetWidth() * 3);

    padding -= (image.GetWidth() * 3);
    vector<char> padding_buff(padding, 0);

    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);

        for (int x = 0; x < image.GetWidth(); ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }

        // Записываем строку из буфера в файл
        ofs.write(buff.data(), image.GetWidth() * 3);

        // Записываем отступ
        ofs.write(padding_buff.data(), padding);
    }

    return true;
}

/**
 * Загружает изображение из файла file формата .bmp
*/
Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);

    // Возвращаем в случае неудачи создания/открытия file
    if (!ifs) {
        return {};
    }
    
    // Считываем данные из первой части заголовка
    BitmapFileHeader file_header;

    file_header.sign.resize(2);
    ifs.read(file_header.sign.data(), 2);
    ifs.read(reinterpret_cast<char*>(&file_header.header_and_data_size), 4);
    ifs.read(reinterpret_cast<char*>(&file_header.reserved_area), 4);
    ifs.read(reinterpret_cast<char*>(&file_header.padding_from_files_begining), 4);

    if (file_header.sign != BMP_SIGN) {
        return {};
    }

    // Считываем данные из второй части заголовка
    BitmapInfoHeader info_header;
    
    ifs.read(reinterpret_cast<char*>(&info_header.inf_header_size), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.image_width), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.image_height), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.levels_num), 2);
    ifs.read(reinterpret_cast<char*>(&info_header.bits_per_pixel), 2);
    ifs.read(reinterpret_cast<char*>(&info_header.compress_type), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.bytes_in_data), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.hor_pixel_per_meter), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.ver_pixel_per_meter), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.used_colors), 4);
    ifs.read(reinterpret_cast<char*>(&info_header.significant_colors), 4);

    // Проверяем поддержку входящего формата
    if (info_header.levels_num != BMP_LEVELS_NUM || info_header.bits_per_pixel != BMP_BITS_PER_PIXEL 
        || info_header.compress_type != BMP_COMPRESS_TYPE)
    {
        return {};
    }
    if (info_header.hor_pixel_per_meter != BMP_HOR_AND_VER_PIXEL_PER_METER 
        || info_header.ver_pixel_per_meter != BMP_HOR_AND_VER_PIXEL_PER_METER)
    {
        return {};
    }
    if (info_header.used_colors != BMP_USED_COLORS 
        || info_header.significant_colors != BMP_SIGNIFICENT_COLORS)
    {
        return {};
    }

    Image result(info_header.image_width, info_header.image_height, Color::Black());
    
    // Отступ - количество дополнительных байт для кратности 4
    int padding = GetBMPStride(info_header.image_width) - (info_header.image_width * 3);
    vector<char> padding_buff(padding);

    vector<char> buff(info_header.image_width * 3);

    for (int y = result.GetHeight() - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), info_header.image_width * 3);

        for (int x = 0; x < result.GetWidth(); ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }

        // Считываем оставшиеся байты отступа
        ifs.read(padding_buff.data(), padding);
    }

    return result;
}

}  // namespace img_lib
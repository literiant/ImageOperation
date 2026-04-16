#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// BMP文件头结构
// 注：__attribute__((packed)) 确保结构体按紧凑方式存储，避免字节对齐问题
typedef struct
{
    uint16_t type;      // 图像类型，0x4D42表示BMP格式
    uint32_t size;      // 文件大小
    uint16_t reserved1; // 保留字段1
    uint16_t reserved2; // 保留字段2
    uint32_t off_bits;  // 像素数据偏移量
} __attribute__((packed)) bmp_file_header_t;

// BMP信息头结构
typedef struct
{
    uint32_t size;            // 信息头大小
    int32_t width;            // 图像宽度
    int32_t height;           // 图像高度
    uint16_t planes;          // 色彩平面数，通常为1
    uint16_t bit_count;       // 位深度，这里为8位
    uint32_t compression;     // 压缩方式，0表示不压缩
    uint32_t size_image;      // 图像数据大小
    uint32_t x_pels_permeter; // 水平分辨率
    uint32_t y_pels_permeter; // 垂直分辨率
    uint32_t clr_used;        // 实际使用的颜色数
    uint32_t clr_important;   // 重要颜色数
} bmp_info_header_t;

// 颜色表结构
typedef struct
{
    uint8_t blue;  // 蓝色分量
    uint8_t green; // 绿色分量
    uint8_t red;   // 红色分量
    uint8_t alpha; // 透明度分量
} bmp_color_entry_t;

// 全局变量
static bmp_file_header_t s_bmp_file_header = {0x4d42, 0, 0, 0, 0};
static bmp_info_header_t s_bmp_info_header = {0, 0, 0, 1, 8, 0, 0, 0, 0, 0, 0};
static bmp_color_entry_t s_color_table[256] = {0}; // 颜色表
static uint8_t *s_bmpdata = NULL;                  // 动态分配的图像数据
static uint32_t s_bmp_col = 0;                     // 图像宽度
static uint32_t s_bmp_row = 0;                     // 图像高度
static uint16_t s_bit_count = 8;                   // 位深度
char in_file_path[256] = ".bmp";                   // 输入文件路径
char out_file_path[256] = "Test_out.bmp";          // 输出文件路径

/**
 * @brief 分配图像内存
 * @param width 图像宽度
 * @param height 图像高度
 * @param bit_count 位深度
 * @return 分配的内存指针，失败返回NULL
 */
static uint8_t *allocate_image_memory(uint32_t width, uint32_t height, uint16_t bit_count)
{
    size_t size;
    if (bit_count == 8)
    {
        size = width * height;
    }
    else if (bit_count == 24)
    {
        size = width * height * 3;
    }
    else
    {
        return NULL;
    }
    return (uint8_t *)calloc(size, sizeof(uint8_t));
}

/**
 * @brief 释放图像内存
 * @param image 图像数据指针
 */
static void free_image_memory(uint8_t *image)
{
    if (image)
    {
        free(image);
    }
}

/**
 * @brief 计算BMP图像每行的字节数（4字节对齐）
 * @param width 图像宽度
 * @return 每行的字节数
 */
static uint32_t calculate_line_width(uint32_t width)
{
    return (width + 3) / 4 * 4; // 4字节对齐
}

/**
 * @brief 初始化BMP文件头
 * @param width 图像宽度
 * @param height 图像高度
 * @param line_width 每行字节数
 */
static void init_bmp_file_header(uint32_t width, uint32_t height, uint32_t line_width)
{
    s_bmp_file_header.type = 0x4D42; // BMP标识
    if (s_bit_count == 8)
    {
        s_bmp_file_header.off_bits = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + sizeof(s_color_table);
    }
    else if (s_bit_count == 24)
    {
        s_bmp_file_header.off_bits = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t);
    }
    s_bmp_file_header.size = s_bmp_file_header.off_bits + line_width * height;
    s_bmp_file_header.reserved1 = 0;
    s_bmp_file_header.reserved2 = 0;
}

/**
 * @brief 初始化BMP信息头
 * @param width 图像宽度
 * @param height 图像高度
 * @param line_width 每行字节数
 */
static void init_bmp_info_header(uint32_t width, uint32_t height, uint32_t line_width)
{
    s_bmp_info_header.size = sizeof(bmp_info_header_t);
    s_bmp_info_header.width = width;
    s_bmp_info_header.height = height;
    s_bmp_info_header.planes = 1;
    s_bmp_info_header.bit_count = s_bit_count; // 使用实际的位深度
    s_bmp_info_header.compression = 0;         // 不压缩
    s_bmp_info_header.size_image = line_width * height;
    s_bmp_info_header.x_pels_permeter = 0;
    s_bmp_info_header.y_pels_permeter = 0;
    s_bmp_info_header.clr_used = 0;
    s_bmp_info_header.clr_important = 0;
}

/**
 * @brief 初始化灰度颜色表
 */
static void init_gray_color_table(void)
{
    for (int i = 0; i < 256; i++)
    {
        s_color_table[i].blue = i;
        s_color_table[i].green = i;
        s_color_table[i].red = i;
        s_color_table[i].alpha = 0;
    }
}

/**
 * @brief 从BMP文件读取颜色表
 * @param file 文件指针
 * @return 0表示成功，-1表示失败
 */
static int32_t read_color_table(FILE *file)
{
    size_t result = fread(s_color_table, sizeof(s_color_table), 1, file);
    if (result != 1)
    {
        printf("Error: Failed to read color table, result = %zu\n", result);
        perror("fread");
        return -1;
    }
    return 0;
}

/**
 * @brief 向BMP文件写入颜色表
 * @param file 文件指针
 * @return 0表示成功，-1表示失败
 */
static int32_t write_color_table(FILE *file)
{
    if (fwrite(s_color_table, sizeof(s_color_table), 1, file) != 1)
    {
        printf("Error: Failed to write color table\n");
        return -1;
    }
    return 0;
}

/**
 * @brief 从BMP文件读取图像数据
 * @param file_path BMP文件路径
 * @param col 输出图像宽度
 * @param row 输出图像高度
 * @return 0表示成功，-1表示失败
 */
int32_t bmp_file_to_image(const char *file_path, uint32_t *col, uint32_t *row)
{
    FILE *file = NULL;
    uint32_t line_width = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t err = 0;
    uint8_t buf[200 * 200] = {0}; // 临时缓冲区
    char temp[2048] = {0};        // 用于存储颜色表和对齐数据
    int i = 0;

    // 使用do-while(0)结构进行错误处理
    do
    {
        // 检查参数有效性
        if (NULL == file_path || NULL == col || NULL == row)
        {
            printf("Error: Invalid parameters\n");
            err = -1;
            break;
        }

        printf("[bmp_file_to_image] Reading file: %s\n", file_path);

        // 打开BMP文件（二进制模式）
        file = fopen(file_path, "rb");

        // 检查文件是否成功打开
        if (NULL == file)
        {
            printf("Error: Failed to open file: %s\n", file_path);
            perror("fopen");
            err = -1;
            break;
        }
        printf("Success: File opened\n");

        // 读取文件头
        if (fread(&s_bmp_file_header, sizeof(s_bmp_file_header), 1, file) != 1)
        {
            printf("Error: Failed to read file header\n");
            err = -1;
            break;
        }
        printf("Success: File header read\n");

        // 读取信息头
        if (fread(&s_bmp_info_header, sizeof(s_bmp_info_header), 1, file) != 1)
        {
            printf("Error: Failed to read info header\n");
            err = -1;
            break;
        }
        printf("Success: Info header read\n");
        printf("Bit count: %d\n", s_bmp_info_header.bit_count);

        // 保存位深度
        s_bit_count = s_bmp_info_header.bit_count;
        printf("Bit count: %d\n", s_bit_count);

        // 只有8位BMP文件才有颜色表
        if (s_bit_count == 8)
        {
            // 读取颜色表
            if (read_color_table(file) != 0)
            {
                printf("Error: Failed to read color table\n");
                err = -1;
                break;
            }
            printf("Success: Color table read\n");
        }
        else if (s_bit_count == 24)
        {
            printf("Success: 24-bit BMP file detected, no color table\n");
        }
        else
        {
            printf("Error: Only 8-bit and 24-bit BMP files are supported\n");
            err = -1;
            break;
        }

        // 获取图像宽度和高度
        width = s_bmp_info_header.width;
        height = s_bmp_info_header.height;
        *col = width;
        *row = height;

        // 检查图像大小是否合理（设置一个较大的限制，避免分配过多内存）
        if (width > 1000 || height > 1000)
        {
            printf("Error: Image size exceeds buffer size (max 1000x1000)\n");
            err = -1;
            break;
        }

        // 分配图像内存
        s_bmpdata = allocate_image_memory(width, height, s_bit_count);
        if (s_bmpdata == NULL)
        {
            printf("Error: Failed to allocate memory\n");
            err = -1;
            break;
        }
        printf("Success: Memory allocated\n");

        // 计算每行字节数（4字节对齐）
        if (s_bit_count == 8)
        {
            line_width = calculate_line_width(width);
        }
        else if (s_bit_count == 24)
        {
            line_width = calculate_line_width(width * 3);
        }
        printf("[bmp_file_to_image] line_width = %d, width = %d, height = %d\n", line_width, width, height);

        // 读取图像数据（BMP文件存储是从下到上的）
        for (i = height - 1; i >= 0; i--)
        {
            if (s_bit_count == 8)
            {
                // 读取一行图像数据（8位）
                fread(s_bmpdata + i * width, width, 1, file);
            }
            else if (s_bit_count == 24)
            {
                // 读取一行图像数据（24位，每个像素3字节）
                fread(s_bmpdata + i * width * 3, width * 3, 1, file);
            }

            // 跳过对齐字节
            if (line_width > (s_bit_count == 24 ? width * 3 : width))
            {
                fread(temp, line_width - (s_bit_count == 24 ? width * 3 : width), 1, file);
            }
        }
        printf("Success: Image data read\n");

    } while (0);

    // 关闭文件
    if (file != NULL)
    {
        fclose(file);
    }

    // 如果失败，释放已分配的内存
    if (err != 0 && s_bmpdata != NULL)
    {
        free_image_memory(s_bmpdata);
        s_bmpdata = NULL;
    }

    return err;
}

/**
 * @brief 将图像数据保存为BMP文件
 * @param file_path 输出文件路径
 * @param width 图像宽度
 * @param height 图像高度
 * @return 0表示成功，-1表示失败
 */
int32_t dump_image_to_bmp_file(const char *file_path, uint32_t width, uint32_t height)
{
    FILE *file = NULL;
    int32_t err = 0;
    int32_t i; // 循环变量

    // 使用do-while(0)结构进行错误处理
    do
    {
        // 检查参数有效性
        if (NULL == file_path || NULL == s_bmpdata)
        {
            printf("Error: Invalid parameters\n");
            err = -1;
            break;
        }

        // 检查图像大小是否合理（设置一个较大的限制，避免分配过多内存）
        if (width > 1000 || height > 1000)
        {
            printf("Error: Image size exceeds buffer size (max 1000x1000)\n");
            err = -1;
            break;
        }

        // 计算每行字节数（4字节对齐）
        uint32_t line_width;
        if (s_bit_count == 8)
        {
            line_width = calculate_line_width(width);
        }
        else if (s_bit_count == 24)
        {
            line_width = calculate_line_width(width * 3);
        }

        // 初始化BMP文件头和信息头
        init_bmp_file_header(width, height, line_width);
        init_bmp_info_header(width, height, line_width);
        s_bmp_info_header.bit_count = s_bit_count;

        // 只有8位BMP文件需要颜色表
        if (s_bit_count == 8)
        {
            // 初始化灰度颜色表
            init_gray_color_table();
        }

        printf("[dump_image_to_bmp_file] line_width = %d, width = %d, height = %d\n", line_width, width, height);

        // 打开输出文件（二进制模式）
        file = fopen(file_path, "wb");

        // 检查文件是否成功打开
        if (NULL == file)
        {
            printf("Error: Failed to open file for writing\n");
            err = -1;
            break;
        }

        // 写入文件头
        fwrite(&s_bmp_file_header, sizeof(s_bmp_file_header), 1, file);

        // 写入信息头
        fwrite(&s_bmp_info_header, sizeof(bmp_info_header_t), 1, file);

        // 只有8位BMP文件需要写入颜色表
        if (s_bit_count == 8)
        {
            // 写入颜色表
            if (write_color_table(file) != 0)
            {
                err = -1;
                break;
            }
        }

        // 写入图像数据（BMP文件存储是从下到上的）
        for (i = height - 1; i >= 0; i--)
        {
            if (s_bit_count == 8)
            {
                // 写入一行图像数据（8位）
                fwrite(s_bmpdata + i * width, 1, width, file);

                // 写入对齐字节
                if (line_width > width)
                {
                    uint8_t line_align[4] = {0};
                    fwrite(line_align, 1, line_width - width, file);
                }
            }
            else if (s_bit_count == 24)
            {
                // 写入一行图像数据（24位，每个像素3字节）
                fwrite(s_bmpdata + i * width * 3, 1, width * 3, file);

                // 写入对齐字节
                if (line_width > width * 3)
                {
                    uint8_t line_align[4] = {0};
                    fwrite(line_align, 1, line_width - width * 3, file);
                }
            }
        }

        // 刷新文件缓冲区
        fflush(file);
    } while (0);

    // 关闭文件
    if (file != NULL)
    {
        fclose(file);
    }

    return err;
}

/**
 * @brief 主函数
 * @return 0表示成功，-1表示失败
 */
int main()
{
    int32_t err = 0;

    // 读取BMP文件
    err = bmp_file_to_image(in_file_path, &s_bmp_col, &s_bmp_row);
    if (err != 0)
    {
        printf("Error: Failed to read BMP file\n");
        return -1;
    }

    // 打印图像尺寸信息
    printf("[main] Image size: %d x %d, Bit count: %d\n", s_bmp_col, s_bmp_row, s_bit_count);

    // 保存为BMP文件
    err = dump_image_to_bmp_file(out_file_path, s_bmp_col, s_bmp_row);
    if (err != 0)
    {
        printf("Error: Failed to write BMP file\n");
        // 释放内存
        free_image_memory(s_bmpdata);
        s_bmpdata = NULL;
        return -1;
    }

    // 释放内存
    free_image_memory(s_bmpdata);
    s_bmpdata = NULL;

    printf("Success: BMP file processed\n");
    return 0;
}
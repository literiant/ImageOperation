#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局变量：图片地址
const char *INPUT_BMP_PATH = "D:\\Code\\Repository\\ImageOperation\\Test\\BMPimages\\sample.bmp";       // 输入BMP文件路径
const char *OUTPUT_BMP_PATH = "D:\\Code\\Repository\\ImageOperation\\Test\\BMPimages\\Grey_output.bmp"; // 输出BMP文件路径

// BMP文件头结构
#pragma pack(push, 1) // 确保结构体按1字节对齐
typedef struct
{
    unsigned char bfType[2];    // 文件类型，必须为"BM"
    unsigned int bfSize;        // 文件大小（字节）
    unsigned short bfReserved1; // 保留字段1
    unsigned short bfReserved2; // 保留字段2
    unsigned int bfOffBits;     // 像素数据偏移量
} BMP_File_Header;

// BMP信息头结构
typedef struct
{
    unsigned int biSize;         // 信息头大小
    int biWidth;                 // 图像宽度
    int biHeight;                // 图像高度
    unsigned short biPlanes;     // 色彩平面数
    unsigned short biBitCount;   // 位深度
    unsigned int biCompression;  // 压缩类型
    unsigned int biSizeImage;    // 图像大小
    int biXPelsPerMeter;         // 水平分辨率
    int biYPelsPerMeter;         // 垂直分辨率
    unsigned int biClrUsed;      // 颜色表使用数
    unsigned int biClrImportant; // 重要颜色数
} BMP_Infor_Header;
#pragma pack(pop) // 恢复默认对齐方式

// 像素点结构
typedef struct
{
    unsigned char blue;  // 蓝色分量
    unsigned char green; // 绿色分量
    unsigned char red;   // 红色分量
} RGBPixel;

// 函数声明
int convertBMPToGrayscale(const char *inputPath, const char *outputPath);
int readBMPFile(const char *filePath, 
                BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader, 
                RGBPixel **pixels, int *rowSize);
int writeBMPFile(const char *filePath, 
                BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader, 
                RGBPixel *pixels, int rowSize);
void convertToGrayscale(RGBPixel *pixels, int width, int height, int rowSize);
void printBMPInfo(BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader);

int main()
{
    // 使用全局变量作为图片地址
    const char *inputPath = INPUT_BMP_PATH;
    const char *outputPath = OUTPUT_BMP_PATH;

    printf("正在转换 %s 到 %s...\n", inputPath, outputPath);

    int result = convertBMPToGrayscale(inputPath, outputPath);

    if (result == 0)
    {
        printf("转换成功！\n");
    }
    else
    {
        printf("转换失败！\n");
    }

    return result;
}

/**
 * 将24位BMP图像转换为灰度图
 * @param inputPath 输入BMP文件路径
 * @param outputPath 输出BMP文件路径
 * @return 0表示成功，非0表示失败
 */
int convertBMPToGrayscale(const char *inputPath, const char *outputPath)
{
    BMP_File_Header fileHeader;
    BMP_Infor_Header infoHeader;
    RGBPixel *pixels = NULL;
    int rowSize = 0;

    // 读取BMP文件
    if (readBMPFile(inputPath, &fileHeader, &infoHeader, &pixels, &rowSize) != 0)
    {
        printf("读取BMP文件失败！\n");
        return 1;
    }

    // 检查是否为24位深BMP
    if (infoHeader.biBitCount != 24)
    {
        printf("只支持24位深的BMP图像！当前位深度: %d\n", infoHeader.biBitCount);
        free(pixels);
        return 1;
    }

    // 打印BMP信息
    printBMPInfo(&fileHeader, &infoHeader);

    // 转换为灰度图
    convertToGrayscale(pixels, infoHeader.biWidth, infoHeader.biHeight, rowSize);

    // 写入BMP文件
    if (writeBMPFile(outputPath, &fileHeader, &infoHeader, pixels, rowSize) != 0)
    {
        printf("写入BMP文件失败！\n");
        free(pixels);
        return 1;
    }

    // 释放内存
    free(pixels);

    return 0;
}

/**
 * 读取BMP文件
 * @param filePath 文件路径
 * @param fileHeader 文件头
 * @param infoHeader 信息头
 * @param pixels 像素数据
 * @param rowSize 每行像素数据大小（字节）
 * @return 0表示成功，非0表示失败
 */
int readBMPFile(const char *filePath, BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader, RGBPixel **pixels, int *rowSize)
{
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        printf("无法打开文件: %s\n", filePath);
        return 1;
    }

    // 读取文件头
    if (fread(fileHeader, sizeof(BMP_File_Header), 1, file) != 1)
    {
        printf("读取文件头失败！\n");
        fclose(file);
        return 1;
    }

    // 检查是否为BMP文件
    if (fileHeader->bfType[0] != 'B' || fileHeader->bfType[1] != 'M')
    {
        printf("不是有效的BMP文件！\n");
        fclose(file);
        return 1;
    }

    // 读取信息头
    if (fread(infoHeader, sizeof(BMP_Infor_Header), 1, file) != 1)
    {
        printf("读取信息头失败！\n");
        fclose(file);
        return 1;
    }

    // 计算每行像素数据大小（必须是4的倍数）
    *rowSize = ((infoHeader->biWidth * infoHeader->biBitCount + 31) / 32) * 4;

    // 计算像素数据大小
    int pixelDataSize = *rowSize * abs(infoHeader->biHeight);

    // 分配内存存储像素数据
    *pixels = (RGBPixel *)malloc(pixelDataSize);
    if (*pixels == NULL)
    {
        printf("内存分配失败！\n");
        fclose(file);
        return 1;
    }

    // 移动到像素数据位置
    fseek(file, fileHeader->bfOffBits, SEEK_SET);

    // 读取像素数据
    if (fread(*pixels, pixelDataSize, 1, file) != 1)
    {
        printf("读取像素数据失败！\n");
        free(*pixels);
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

/**
 * 写入BMP文件
 * @param filePath 文件路径
 * @param fileHeader 文件头
 * @param infoHeader 信息头
 * @param pixels 像素数据
 * @param rowSize 每行像素数据大小（字节）
 * @return 0表示成功，非0表示失败
 */
int writeBMPFile(const char *filePath, BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader, RGBPixel *pixels, int rowSize)
{
    FILE *file = fopen(filePath, "wb");
    if (file == NULL)
    {
        printf("无法创建文件: %s\n", filePath);
        return 1;
    }

    // 写入文件头
    if (fwrite(fileHeader, sizeof(BMP_File_Header), 1, file) != 1)
    {
        printf("写入文件头失败！\n");
        fclose(file);
        return 1;
    }

    // 写入信息头
    if (fwrite(infoHeader, sizeof(BMP_Infor_Header), 1, file) != 1)
    {
        printf("写入信息头失败！\n");
        fclose(file);
        return 1;
    }

    // 移动到像素数据位置
    fseek(file, fileHeader->bfOffBits, SEEK_SET);

    // 写入像素数据
    int pixelDataSize = rowSize * abs(infoHeader->biHeight);
    if (fwrite(pixels, pixelDataSize, 1, file) != 1)
    {
        printf("写入像素数据失败！\n");
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

/**
 * 将RGB像素转换为灰度
 * @param pixels 像素数据
 * @param width 图像宽度
 * @param height 图像高度
 * @param rowSize 每行像素数据大小（字节）
 */
void convertToGrayscale(RGBPixel *pixels, int width, int height, int rowSize)
{
    int i, j;
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            // 计算当前像素位置
            RGBPixel *pixel = (RGBPixel *)((unsigned char *)pixels + i * rowSize + j * sizeof(RGBPixel));

            // 使用加权平均法计算灰度值
            // 权重：R=0.299, G=0.587, B=0.114
            unsigned char gray = (unsigned char)(0.299 * pixel->red + 0.587 * pixel->green + 0.114 * pixel->blue);

            // 将灰度值赋给RGB三个通道
            pixel->red = gray;
            pixel->green = gray;
            pixel->blue = gray;
        }
    }
}

/**
 * 打印BMP文件信息
 * @param fileHeader 文件头
 * @param infoHeader 信息头
 */
void printBMPInfo(BMP_File_Header *fileHeader, BMP_Infor_Header *infoHeader)
{
    printf("BMP文件信息:\n");
    printf("文件大小: %u 字节\n", fileHeader->bfSize);
    printf("像素数据偏移量: %u 字节\n", fileHeader->bfOffBits);
    printf("图像宽度: %d 像素\n", infoHeader->biWidth);
    printf("图像高度: %d 像素\n", infoHeader->biHeight);
    printf("位深度: %d 位\n", infoHeader->biBitCount);
    printf("压缩类型: %u\n", infoHeader->biCompression);
    printf("图像大小: %u 字节\n", infoHeader->biSizeImage);
    printf("水平分辨率: %d 像素/米\n", infoHeader->biXPelsPerMeter);
    printf("垂直分辨率: %d 像素/米\n", infoHeader->biYPelsPerMeter);
    printf("颜色表使用数: %u\n", infoHeader->biClrUsed);
    printf("重要颜色数: %u\n", infoHeader->biClrImportant);
}
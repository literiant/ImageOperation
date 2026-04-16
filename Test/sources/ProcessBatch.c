#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// 全局变量：图片地址
const char *INPUT_FOLDER = "D:\\Code\\Repository\\ImageOperation\\Test\\BMPimages\\Source";       // 输入BMP文件夹路径
const char *OUTPUT_FOLDER = "D:\\Code\\Repository\\ImageOperation\\Test\\BMPimages\\Gray_output"; // 输出灰度图像文件夹路径

// BMP文件头结构
#pragma pack(push, 1) // 确保结构体按1字节对齐
typedef struct
{
    unsigned char bfType[2];    // 文件类型，必须为"BM"
    unsigned int bfSize;        // 文件大小（字节）
    unsigned short bfReserved1; // 保留字段1
    unsigned short bfReserved2; // 保留字段2
    unsigned int bfOffBits;     // 像素数据偏移量
} MyBITMAPFILEHEADER;

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
} MyBITMAPINFOHEADER;
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
int readBMPFile(const char *filePath, MyBITMAPFILEHEADER *fileHeader, MyBITMAPINFOHEADER *infoHeader, RGBPixel **pixels, int *rowSize);
int writeBMPFile(const char *filePath, MyBITMAPFILEHEADER *fileHeader, MyBITMAPINFOHEADER *infoHeader, RGBPixel *pixels, int rowSize);
void convertToGrayscale(RGBPixel *pixels, int width, int height, int rowSize);
void processBatchBMP(const char *inputFolder, const char *outputFolder);

int main()
{
    printf("正在批量处理 %s 文件夹中的BMP文件...\n", INPUT_FOLDER);

    processBatchBMP(INPUT_FOLDER, OUTPUT_FOLDER);

    printf("批量处理完成！\n");

    return 0;
}

/**
 * 批量处理文件夹中的BMP文件
 * @param inputFolder 输入文件夹路径
 * @param outputFolder 输出文件夹路径
 */
void processBatchBMP(const char *inputFolder, const char *outputFolder)
{
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[MAX_PATH];
    char inputPath[MAX_PATH];
    char outputPath[MAX_PATH];

    // 创建输出文件夹
    if (CreateDirectoryA(outputFolder, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("输出文件夹 %s 已准备就绪\n", outputFolder);
    }
    else
    {
        printf("创建输出文件夹失败！\n");
        return;
    }

    // 构建搜索路径
    snprintf(searchPath, MAX_PATH, "%s\\*.bmp", inputFolder);

    // 查找第一个BMP文件
    hFind = FindFirstFileA(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("在 %s 文件夹中未找到BMP文件！\n", inputFolder);
        return;
    }

    // 遍历所有BMP文件
    do
    {
        // 跳过目录
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // 构建输入文件路径
            snprintf(inputPath, MAX_PATH, "%s\\%s", inputFolder, findFileData.cFileName);

            // 构建输出文件路径
            snprintf(outputPath, MAX_PATH, "%s\\%s", outputFolder, findFileData.cFileName);

            printf("处理文件: %s\n", findFileData.cFileName);

            // 转换为灰度图
            if (convertBMPToGrayscale(inputPath, outputPath) == 0)
            {
                printf("  转换成功！\n");
            }
            else
            {
                printf("  转换失败！\n");
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    // 关闭查找句柄
    FindClose(hFind);
}

/**
 * 将24位BMP图像转换为灰度图
 * @param inputPath 输入BMP文件路径
 * @param outputPath 输出BMP文件路径
 * @return 0表示成功，非0表示失败
 */
int convertBMPToGrayscale(const char *inputPath, const char *outputPath)
{
    MyBITMAPFILEHEADER fileHeader;
    MyBITMAPINFOHEADER infoHeader;
    RGBPixel *pixels = NULL;
    int rowSize = 0;

    // 读取BMP文件
    if (readBMPFile(inputPath, &fileHeader, &infoHeader, &pixels, &rowSize) != 0)
    {
        return 1;
    }

    // 检查是否为24位深BMP
    if (infoHeader.biBitCount != 24)
    {
        free(pixels);
        return 1;
    }

    // 转换为灰度图
    convertToGrayscale(pixels, infoHeader.biWidth, infoHeader.biHeight, rowSize);

    // 写入BMP文件
    if (writeBMPFile(outputPath, &fileHeader, &infoHeader, pixels, rowSize) != 0)
    {
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
int readBMPFile(const char *filePath, MyBITMAPFILEHEADER *fileHeader, MyBITMAPINFOHEADER *infoHeader, RGBPixel **pixels, int *rowSize)
{
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        return 1;
    }

    // 读取文件头
    if (fread(fileHeader, sizeof(MyBITMAPFILEHEADER), 1, file) != 1)
    {
        fclose(file);
        return 1;
    }

    // 检查是否为BMP文件
    if (fileHeader->bfType[0] != 'B' || fileHeader->bfType[1] != 'M')
    {
        fclose(file);
        return 1;
    }

    // 读取信息头
    if (fread(infoHeader, sizeof(MyBITMAPINFOHEADER), 1, file) != 1)
    {
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
        fclose(file);
        return 1;
    }

    // 移动到像素数据位置
    fseek(file, fileHeader->bfOffBits, SEEK_SET);

    // 读取像素数据
    if (fread(*pixels, pixelDataSize, 1, file) != 1)
    {
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
int writeBMPFile(const char *filePath, MyBITMAPFILEHEADER *fileHeader, MyBITMAPINFOHEADER *infoHeader, RGBPixel *pixels, int rowSize)
{
    FILE *file = fopen(filePath, "wb");
    if (file == NULL)
    {
        return 1;
    }

    // 写入文件头
    if (fwrite(fileHeader, sizeof(MyBITMAPFILEHEADER), 1, file) != 1)
    {
        fclose(file);
        return 1;
    }

    // 写入信息头
    if (fwrite(infoHeader, sizeof(MyBITMAPINFOHEADER), 1, file) != 1)
    {
        fclose(file);
        return 1;
    }

    // 移动到像素数据位置
    fseek(file, fileHeader->bfOffBits, SEEK_SET);

    // 写入像素数据
    int pixelDataSize = rowSize * abs(infoHeader->biHeight);
    if (fwrite(pixels, pixelDataSize, 1, file) != 1)
    {
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
#include <stdio.h>
#include <stdint.h> //精准定义整数类型，直观简洁.
#include <stdlib.h> //1.取绝对值用于计算像素数据大小 2.动态分配内存给像素数据
// 图形处理需要对图片文件进行处理，必须用到相关的文件地址
// 为了便于修改，将图片地址定义为全局变量置于最开头处.
const char *INPUT_BMP_PATH = "Test/BMPimages/Source/Simple/C9AEFF_100-100.bmp";
const char *OUTPUT_BMP_PATH = "Test/BMPimages/Source/Simple/Grey_output.bmp";

// 开始定义各类结构体缓存图片数据。
// 使用#pragma pack进行统一的1字节对齐，防止内存映射错误.
#pragma pack(push, 1)

// 第一个结构体：BMP文件头(BMP_File_Header) 共14字节
// 包括五个部分:文件类型：2字节 文件大小：4字节 保留字段1：2字节 保留字段2：2字节 像素数据偏移量：4字节
typedef struct
{
    uint16_t Type[2];   // 文件类型，必须为"BM" "424D"
    uint32_t Size;      // 文件大小（字节） "____ ____"
    uint16_t Reserved1; // 保留字段1 "0000"
    uint16_t Reserved2; // 保留字段2 "0000"
    uint32_t OffBits;   // 像素数据偏移量 54字节 "3600 0000"
} bmpFileHeader;
// 第二个结构体:BMP信息头(BMP_Infor_Header) 共40字节
// 包括 11个部分:信息头大小：4字节 图像宽度：4字节 图像高度：4字节 色彩数：2字节 位深度：2字节 压缩类型：4字节 图像大小：4字节 水平分辨率：4字节 垂直分辨率：4字节 颜色表使用数：4字节 重要颜色数：4字节
typedef struct
{
    uint32_t Size;     // 信息头大小 40字节 "2800 0000"
                       // 由于是带符号的，使用signed类型.
    int32_t Width;     // 图像宽度 "____ ____"
    int32_t Height;    // 图像高度 "____ ____"
                       // 符号决定了位图的像素储存顺序(高正:从下往上.高负:从上往下.宽正:从左往右.宽负:从右往左)
    uint16_t Planes;   // 色彩平面数 1个 "0100"
    uint16_t BitCount; // 位深度 每像素24位 "1800"
    // 仅有的两个2字节类型.
    uint32_t Compression; // 压缩类型 "0000 0000"(24位深位图是全彩无损的)
    uint32_t SizeofImage; // 图像大小 "____ ____"
    // 分辨率决定图片在显示器上的显示密度，通常为 0,表示由显示器自己决定
    int32_t XpelsPerMeter; // 水平分辨率 "0000 0000"
    int32_t YpelsPerMeter; // 垂直分辨率 "0000 0000"
    // 由于分辨率和高度和宽度挂钩，延续signed类型，实则符号无意义，通常为正.
    uint32_t ClrUsed;      // 颜色表使用数 "0000 0000"
    uint32_t ClrImportant; // 重要颜色数 "0000 0000"
                           // 24位深的位图为全彩，使用所有颜色，所有颜色同样重要，故二者为0.
                           // 256色位图等颜色不全的图片才考虑此二种属性.
} bmpInforHeader;

// 第三个结构体:BMP像素点(BMP_Pixel) 共3字节
// 包括以下部分:蓝色分量：1字节 绿色分量：1字节 红色分量：1字节
typedef struct
{
    uint8_t Blue;  // 蓝色分量
    uint8_t Green; // 绿色分量
    uint8_t Red;   // 红色分量
} bmpPixel;

// 第三步，整理声明需要用到的函数模块.
// 第一个:读取BMP文件头和信息头以及像素数据
int readBMPHeader(const char *inputFilePath, bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader, bmpPixel **pixelData, int *rowSize);
// 第二个:写入BMP文件信息以及像素数据
int writeBMPHeader(const char *outputFliePath, bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader, bmpPixel *pixelData, int rowSize);
// 第三个:打印BMP文件重要信息
void printBMPInfor(bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader);
// 第四个:对图像像素进行操作
void Executant(bmpPixel *pixelData, int width, int height, int rowSize);
// 第五个:包装函数功能
int wrappedExecutant(const char *inputPath, const char *outputPath);

// 实现包装函数
int wrappedExecutant(const char *inputPath, const char *outputPath)
{
    bmpFileHeader fileHeader;
    bmpInforHeader infoHeader;
    bmpPixel *pixelData = NULL;
    int rowSize = 0;

    // 读取BMP文件
    if (readBMPHeader(inputPath, &fileHeader, &infoHeader, &pixelData, &rowSize) != 0)
    {
        printf("读取BMP文件失败！\n");
        return 1;
    }

    // 检查是否为24位深BMP
    if (infoHeader.BitCount != 24)
    {
        printf("只支持24位深的BMP图像！当前位深度: %d\n", infoHeader.BitCount);
        free(pixelData);
        return 1;
    }

    // 打印BMP信息
    printBMPInfor(&fileHeader, &infoHeader);

    // 对图像像素进行操作
    if (Executant(pixelData, infoHeader.Width, infoHeader.Height) != 0)
    {
        printf("图像处理失败！\n");
        free(pixelData);
        return 1;
    }

    // 写入BMP文件
    if (writeBMPHeader(outputPath, &fileHeader, &infoHeader, pixelData, rowSize) != 0)
    {
        printf("写入BMP文件失败！\n");
        free(pixelData);
        return 1;
    }

    // 释放内存
    free(pixelData);

    return 0;
}

// 实现图像处理函数
int Executant(bmpPixel *pixelData, int width, int height)
{
    // 这里实现图像处理逻辑，例如灰度转换
    // 简单的灰度转换实现
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            // 计算当前像素位置
            bmpPixel *pixel = &pixelData[i * width + j];

            // 使用加权平均法计算灰度值
            // 权重：R=0.299, G=0.587, B=0.114
            uint8_t gray = (uint8_t)(0.299 * pixel->Red + 0.587 * pixel->Green + 0.114 * pixel->Blue);

            // 将灰度值赋给RGB三个通道
            pixel->Red = gray;
            pixel->Green = gray;
            pixel->Blue = gray;
        }
    }

    return 0;
}

// 第四步:主函数循环
int main()
{
    // 转译文件路径
    char *inputPath = INPUT_BMP_PATH;
    char *outputPath = OUTPUT_BMP_PATH;
    printf("正在处理 %s 到 %s...\n", inputPath, outputPath);
    int result = wrappedExecutant(inputPath, outputPath);
    if (result != 0)
    {
        printf("处理失败！\n");
    }
    else
    {
        printf("处理成功！\n");
    }
    return result;
}
// 第五步:实现包装函数功能
// 第一个读取函数:
int readBMPHeader(const char *inputFilePath, bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader, bmpPixel **pixelData, int *rowSize)
{
    // 打开文件并检查是否打开
    FILE *file = fopen(inputFilePath, "rb");
    if (NULL == file)
    {
        printf("无法打开文件:%s\n", inputFilePath);
        return 1;
    }
    // 成功打开后
    // 读取文件头
    if (1 != fread(bmpFileHeader, sizeof(bmpFileHeader), 1, file))
    {
        printf("读取文件头失败！\n");
        fclose(file);
        return 1;
    }
    // 读取信息头
    if (1 != fread(bmpInforHeader, sizeof(bmpInforHeader), 1, file))
    {
        printf("读取信息投失败!\n");
        fclose(file);
        return 1;
    }
    // 每行像素数据大小（必须是4的倍数）
    *rowSize = ((bmpInforHeader->Width * bmpInforHeader->BitCount + 31) / 32) * 4;
    // 像素数据的实际大小(字节数)
    int pixelDataSize = *rowSize * abs(bmpInforHeader->Height);
    // 分配内存存储像素数据
    *pixelData = (bmpPixel *)malloc(pixelDataSize); // 注意pixelData为双重指针,malloc的返回值是一个void *类型的指针
    if (NULL == *pixelData)
    {
        printf("分配内存失败！\n");
        fclose(file);
        return 1;
    }
    // 移动至像素数据储存位置
    // 注意:使用fseek()和OffBits定位像素数据的储存位置，以更好适应更多类型的BMP文件类型.
    //(如256色位图在文件头，信息头与像素数据之间有颜色表)
    fseek(file, bmpFileHeader->OffBits, SEEK_SET);
    // 读取像素数据
    if (1 != fread(*pixelData, pixelDataSize, 1, file))
    {
        printf("读取像素数据失败!\n");
        fclose(file);
        free(*pixelData);
        *pixelData = NULL;
        return 1;
    }
    fclose(file);
    return 0;
};
// 第二个写入函数:
int writeBMPHeader(const char *outputFilePath, bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader, bmpPixel *pixelData, int rowSize) // 注意:这里直接用在读取时就计算好的rowSize,就不想再计算一次
{
    // 打开文件并检查是否打开
    FILE *file = fopen(outputFilePath, "wb");
    if (NULL == file)
    {
        printf("无法打开文件:%s\n", outputFilePath);
        return 1;
    }
    // 成功打开后
    // 写入文件头
    if (1 != fwrite(bmpFileHeader, sizeof(bmpFileHeader), 1, file))
    {
        printf("写入文件头失败！\n");
        fclose(file);
        return 1;
    }
    // 写入信息头
    if (1 != fwrite(bmpInforHeader, sizeof(bmpInforHeader), 1, file))
    {
        printf("写入信息头失败！\n");
        fclose(file);
        return 1;
    }
    // 像素数据的实际大小(字节数)
    int pixelDataSize = rowSize * abs(bmpInforHeader->Height);
    // 写入像素数据
    if (1 != fwrite(pixelData, pixelDataSize, 1, file))
    {
        printf("写入像素数据失败!\n");
        fclose(file);
        return 1;
    }
    fclose(file);
    return 0;
}
// 第三个包装函数
int wrappedExecutant(const char *inputPath, const char *outputPath)
{
    bmpFileHeader bmpFileHeader;
    bmpInforHeader bmpInforHeader;
    bmpPixel *pixelData = NULL;
    int rowSize = 0;

    if (0 != readBMPHeader(inputPath, &bmpFileHeader, &bmpInforHeader, &pixelData, &rowSize))
    {
        printf("读取BMP文件失败！\n");
        return 1;
    }
    // 打印BMP文件信息
    printBMPInfor(&bmpFileHeader, &bmpInforHeader);
    // 处理像素数据
    executant(pixelData, bmpInforHeader.Width, bmpInforHeader.Height, rowSize);
    // 写入BMP文件
    if (0 != writeBMPHeader(outputPath, &bmpFileHeader, &bmpInforHeader, pixelData, rowSize))
    {
        printf("写入BMP文件失败！\n");
        return 1;
    }
    // 释放内存
    free(pixelData);
    pixelData = NULL;
    return 0;
}
// 第四个:对图像像素进行操作
void executant(bmpPixel *pixelData, int width, int height, int rowSize)
{
    int i, j;
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            // 计算当前像素位置
            bmpPixel *pixel = (bmpPixel *)((unsigned char *)pixelData + i * rowSize + j * sizeof(bmpPixel));

            // 使用加权平均法计算灰度值
            // 权重：R=0.299, G=0.587, B=0.114
            unsigned char gray = (unsigned char)(0.299 * pixel->Red + 0.587 * pixel->Green + 0.114 * pixel->Blue);

            // 将灰度值赋给RGB三个通道
            pixel->Red = gray;
            pixel->Green = gray;
            pixel->Blue = gray;
        }
    }
}
// 第五个打印函数(方便处理的同时查看文件信息)
void printBMPInfor(bmpFileHeader *bmpFileHeader, bmpInforHeader *bmpInforHeader)
{
    printf("BMP文件信息:\n");
    printf("文件大小: %u 字节\n", bmpFileHeader->Size);
    printf("像素数据偏移量: %u 字节\n", bmpFileHeader->OffBits);
    printf("图像宽度: %d 像素\n", bmpInforHeader->Width);
    printf("图像高度: %d 像素\n", bmpInforHeader->Height);
    printf("位深度: %d 位\n", bmpInforHeader->BitCount);
    printf("压缩类型: %u\n", bmpInforHeader->Compression);
    printf("图像大小: %u 字节\n", bmpInforHeader->SizeofImage);
    printf("水平分辨率: %d 像素/米\n", bmpInforHeader->XpelsPerMeter);
    printf("垂直分辨率: %d 像素/米\n", bmpInforHeader->YpelsPerMeter);
    printf("颜色表使用数: %u\n", bmpInforHeader->ClrUsed);
    printf("重要颜色数: %u\n", bmpInforHeader->ClrImportant);
}
# fread 函数

C语言中的 `fread` 函数用于从文件中读取数据。它的名字来源于“file read”（文件读取），通常用来读取二进制文件，但也可以读取文本文件。下面我会用最通俗的方式给你讲清楚它的用法。

---

## 1. `fread` 长什么样？（函数原型）

```c
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
```

这看起来有点吓人，我们一个一个解释：

- **`ptr`**：一个指针，指向内存中的一块空间。`fread` 会把读到的数据放在这里。你可以把它想象成一个“箱子”，用来装从文件里拿出来的东西。
- **`size`**：每个要读取的数据块有多大（单位是字节）。比如要读整数，整数一般是4字节，这里就填 `sizeof(int)`。
- **`nmemb`**：要读取多少个这样大小的数据块。比如你想一次读10个整数，这里就填 `10`。
- **`stream`**：文件指针，就是你用 `fopen` 打开文件后得到的东西，它告诉 `fread` 要从哪个文件读。

**返回值**：返回实际读取到的数据块个数（就是 `nmemb` 那个单位）。如果读到了文件末尾或者出错，返回值可能比请求的少。你可以用这个返回值判断读取是否成功。

---

## 2. 举个最简单的例子：从文件里读一个整数

假设有一个二进制文件 `data.bin`，里面存了一个整数 `12345`。

```c
#include <stdio.h>

int main() {
    FILE *fp = fopen("data.bin", "rb");  // 以二进制读方式打开
    if (fp == NULL) {
        printf("文件打开失败！\n");
        return 1;
    }

    int num;
    size_t result = fread(&num, sizeof(int), 1, fp);  // 读1个整数
    if (result != 1) {
        printf("读取失败或文件结束\n");
    } else {
        printf("读到的整数是：%d\n", num);
    }

    fclose(fp);
    return 0;
}
```

这里：
- `&num` 是存放整数的地址（`ptr`）。
- `sizeof(int)` 是每个数据块的大小（`size`）。
- `1` 是要读的数据块个数（`nmemb`）。
- `fp` 是文件指针。

`fread` 会尝试从文件读取 `1 * sizeof(int)` 个字节，放到 `num` 里。如果成功，返回值就是 `1`。

---

## 3. 读多个数据（比如数组）

假如文件里有 5 个连续的整数，你想一次性读到数组中：

```c
#include <stdio.h>

int main() {
    FILE *fp = fopen("data.bin", "rb");
    if (fp == NULL) return 1;

    int arr[5];
    size_t count = fread(arr, sizeof(int), 5, fp);  // 读5个整数

    if (count == 5) {
        printf("成功读取5个整数：\n");
        for (int i = 0; i < 5; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    } else {
        printf("只读到了 %zu 个整数\n", count);
    }

    fclose(fp);
    return 0;
}
```

这里 `arr` 就是数组名，它本身就是指针，所以直接写 `arr` 就行。

---

## 4. 读结构体数据

`fread` 经常用于读取结构体数组，因为结构体在内存中是连续存放的（通常是这样，忽略内存对齐的话）。

假设文件里存了一个 `struct Student` 的数组：

```c
#include <stdio.h>

struct Student {
    char name[20];
    int age;
    float score;
};

int main() {
    FILE *fp = fopen("students.bin", "rb");
    if (fp == NULL) return 1;

    struct Student stu[3];  // 假设文件里有3个学生
    size_t count = fread(stu, sizeof(struct Student), 3, fp);

    if (count == 3) {
        for (int i = 0; i < 3; i++) {
            printf("姓名：%s，年龄：%d，成绩：%.1f\n",
                   stu[i].name, stu[i].age, stu[i].score);
        }
    } else {
        printf("只读到了 %zu 个学生\n", count);
    }

    fclose(fp);
    return 0;
}
```

---

## 5. 重要细节和注意事项

### (1) 文件打开模式
如果是读取二进制文件，**强烈建议**在 `fopen` 时使用 `"rb"`（读二进制）。如果是文本文件，可以用 `"r"`，但要注意文本模式下，不同系统对换行符 `\n` 的处理可能不同（比如Windows下会把 `\r\n` 转换成 `\n`）。为了避免意外，读二进制文件就用 `"rb"`。

### (2) 检查返回值
一定要检查 `fread` 的返回值！如果返回值小于你请求的 `nmemb`，可能是遇到了文件末尾（EOF）或者发生了错误。你可以用 `feof(fp)` 检查是否到文件尾，用 `ferror(fp)` 检查是否出错。

```c
if (count < nmemb) {
    if (feof(fp)) {
        printf("到达文件末尾\n");
    } else if (ferror(fp)) {
        printf("读取错误\n");
    }
}
```

### (3) 缓冲区大小要足够
`ptr` 指向的内存区域必须足够大，至少能容纳 `size * nmemb` 个字节。比如你要读100个整数，那就得有 `int arr[100]` 或者动态分配 `malloc(100 * sizeof(int))`。

### (4) `fread` 不会自动添加 `\0`
如果读的是字符串，`fread` 不会在末尾自动加 `\0`（不像 `fgets`）。所以如果你用 `fread` 读文本，要自己处理字符串结束符，或者用固定大小的字符数组并确保有足够空间。

---

## 6. 对比其他读取函数

- **`fgetc`**：一次读一个字符。
- **`fgets`**：一次读一行字符串（会加 `\0`）。
- **`fscanf`**：格式化读取，按指定格式解析数据（适合文本文件）。
- **`fread`**：适合二进制数据，直接拷贝内存块，效率高，不做解析。

---

## 7. 总结

- `fread(存放地址, 每块大小, 块数, 文件指针)`  
- 返回值是实际读到的块数，可能小于请求数（文件尾或错误）。  
- 多用于二进制文件，如读整数、结构体等。  
- 记得检查返回值，并根据情况处理文件结束和错误。

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <math.h>
#include <string.h>

#pragma pack(push, 1)  
typedef struct {
    uint16_t bfType;      
    uint32_t bfSize;      
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          
    int32_t biWidth;          
    int32_t biHeight;         
    uint16_t biPlanes;        
    uint16_t biBitCount;      
    uint32_t biCompression;   
    uint32_t biSizeImage;     
    int32_t biXPelsPerMeter;  
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;       
    uint32_t biClrImportant;  
} BITMAPINFOHEADER;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
} RGB;

typedef struct {
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    RGB** img;
} BMP;

#pragma pack()  

typedef enum {
    SUCCESS = 0,
    ERROR_MEM = 40,
    ERROR_BMP_FORMAT,
    ERROR_FILE,
    ERROR_VAL,
    ERROR_COMMAND,
    ERROR_BMP
} ERROR;

struct option long_options[] = {
    {"squared_lines", no_argument, 0, 'S'},
    {"left_up", required_argument, 0, 'u'},
    {"side_size", required_argument, 0, 's'},
    {"thickness", required_argument, 0, 't'},
    {"color", required_argument, 0, 'c'},
    {"fill", no_argument, 0, 'f'},
    {"fill_color", required_argument, 0, 'F'},
    {"rgbfilter", no_argument, 0, 'r'},
    {"component_name", required_argument, 0, 'n'},
    {"component_value", required_argument, 0, 'v'},
    {"rotate", no_argument, 0, 'R'},
    {"right_down", required_argument, 0, 'd'},
    {"angle", required_argument, 0, 'a'},
    {"output", required_argument, 0, 'o'},
    {"input", required_argument, 0, 'i'},
    {"info", required_argument, 0, 'I'},
    {"proba", no_argument, 0, 'p'},
    {"flip_squares", no_argument, 0, 'P'},
    {"square_size", required_argument, 0, 'C'},
    {"orientation ", required_argument, 0, 'O'},
    {0, 0, 0, 0}
};

void printHelp() {
    printf("BMP file processing program usage guide:\n");
    printf("- Supports 24-bit BMP files (V3 format) without compression\n");
    printf("- The program verifies BMP format correctness\n");
    printf("- All headers are preserved in the output file\n\n");
    
    printf("Main options:\n");
    printf("--help or -h  - display this guide\n");
    printf("--info or -i  - show file information\n\n");
    
    printf("Processing functions:\n");
    
    printf("1. Square with diagonals (--squared_lines):\n");
    printf("   --left_up X.Y       - coordinates of the top-left corner\n");
    printf("   --side_size N       - side length of the square\n");
    printf("   --thickness K       - line thickness\n");
    printf("   --color R.G.B       - line color\n");
    printf("   --fill              - flag to fill the square\n");
    printf("   --fill_color R.G.B  - fill color\n\n");
    
    printf("2. RGB filter (--rgbfilter):\n");
    printf("   --component_name red/green/blue - component to modify\n");
    printf("   --component_value 0-255         - new component value\n\n");
    
    printf("3. Image rotation (--rotate):\n");
    printf("   --left_up X.Y       - top-left corner of the area\n");
    printf("   --right_down X.Y    - bottom-right corner of the area\n");
    printf("   --angle 90/180/270  - rotation angle\n");
}

void freeBMP(const BMP* bmp)
{
    int height = bmp->bmih.biHeight;
    for (int i = 0; i < height; i++) {
        free(bmp->img[i]);
    }
    free(bmp->img);
}

BMP* readBMP(const char *filename) {
    BMP* bmp = NULL;
    int error = SUCCESS;
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file.\n");
        error = ERROR_FILE;     
    }
    if(!error){
        bmp = (BMP*)calloc(1, sizeof(BMP));
        if (!bmp) {
            fprintf(stderr, "Error: Memory allocation failed for BMP structure\n");
            error = ERROR_MEM;
        }
    }
    if(!error){
        fread(&bmp->bmfh, sizeof(BITMAPFILEHEADER), 1, file);
        fread(&bmp->bmih, sizeof(BITMAPINFOHEADER), 1, file);
        if (bmp->bmfh.bfType != 0x4D42) {
            fprintf(stderr, "This is not bmp!\n");
            error = ERROR_BMP_FORMAT;
        }
    }
    if(!error){
        size_t height = bmp -> bmih.biHeight;
        size_t width = bmp -> bmih.biWidth;
        size_t row_padded = (width * sizeof(RGB) + 3) & (~3);
        bmp->img = (RGB **)malloc(height * sizeof(RGB *));
        if (bmp->img == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for BMP structure\n");
            error = ERROR_MEM;
        }
        for (size_t i = 0; i < height && !error; i++)
        {
            bmp->img[height - 1 - i] = (RGB *)malloc(row_padded);
            if (!bmp->img[height - 1 - i]) {
                fprintf(stderr, "Error: Memory allocation failed for BMP structure\n");
                error = ERROR_MEM;
            }
            else{
                fread(bmp->img[height - 1 - i], row_padded, 1, file); 
            }   
        }
    }
    if(error && bmp != NULL){
        freeBMP(bmp);  
        free(bmp);    
        bmp = NULL;
    }
    if(file){
        fclose(file);
    }
    return bmp;
}

void writeBMP(const char *filename, BMP* bmp) {
    int error = SUCCESS;
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file.\n");
        error = ERROR_FILE;
    }
    if(!error){
        fwrite(&bmp->bmfh, sizeof(BITMAPFILEHEADER), 1, file);
        fwrite(&bmp->bmih, sizeof(BITMAPINFOHEADER), 1, file);

        size_t height = bmp -> bmih.biHeight;
        size_t width = bmp -> bmih.biWidth;
        size_t row_padded = (width * sizeof(RGB) + 3) & (~3);
        for (size_t i = 0; i < height; i++) {
            fwrite(bmp->img[height - 1 - i], row_padded, 1, file); 
        } 
    }
    if (file != NULL) {
        fclose(file);
    }
}

void setPixel(BMP* bmp, int x, int y, RGB col) {
    int img_width = bmp->bmih.biWidth;
    int img_height = bmp->bmih.biHeight;
    if (x >= 0 && x < img_width && y >= 0 && y < img_height) {
        bmp->img[y][x] = col;
    }
}

void drawLine(BMP* bmp, int x1, int y1, int x2, int y2, RGB col) {
    const int deltaX = abs(x2 - x1);
    const int deltaY = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;
    
    setPixel(bmp, x2, y2, col);
    
    while(x1 != x2 || y1 != y2) {
        setPixel(bmp, x1, y1, col);
        int error2 = error * 2;
        if(error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        if(error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
}

void draw_thick_odd_line(BMP* bmp, int x1, int y1, int x2, int y2, int thickness, RGB line_color) {
    if (thickness == 1) {
        drawLine(bmp, x1, y1, x2, y2, line_color);
    }
    else{
        drawLine(bmp, x1, y1, x2, y2, line_color);
        if (abs(y2 - y1) >= abs(x2 - x1)) {
            for (int i = 1; i <= (thickness) / 2; i++) {
                if(x2 > x1){
                    drawLine(bmp, x1 + i, y1, x2, y2 - i, line_color);
                    drawLine(bmp, x1, y1 + i, x2 - i, y2, line_color);
                }
                else{
                    drawLine(bmp, x1, y1 + i, x2 + i, y2, line_color);
                    drawLine(bmp, x1 - i, y1, x2, y2 - i, line_color);
                }
            }
        }
    } 
}

void draw_square(BMP* bmp, int x, int y, int size, int thickness, RGB color, int fill, RGB fill_color) {
    if(fill){
        for (int j = y; j < y + size; j++) {
            for (int i = x; i < x + size; i++) {
                setPixel(bmp, i, j, fill_color);
            }
        }
    }
    for (int t = -thickness / 2; t <= thickness / 2; t++) {
        // Верхняя граница
        drawLine(bmp, x + t, y + t, x + size - 1 - t, y + t, color);
        // Нижняя граница
        drawLine(bmp, x + t, y + size - 1 - t, x + size - 1 - t, y + size - 1 - t, color);
        // Левая граница
        drawLine(bmp, x + t, y + t, x + t, y + size - 1 - t, color);
        // Правая граница
        drawLine(bmp, x + size - 1 - t, y + t, x + size - 1 - t, y + size - 1 - t, color);
    }
    // Рисование диагоналей
        // Главная диагональ (из левого верхнего в правый нижний угол)
    draw_thick_odd_line(bmp, x, y, x + size - 1, y + size - 1, thickness, color);
        // Побочная диагональ (из правого верхнего в левый нижний угол)
    draw_thick_odd_line(bmp, x + size - 1, y, x, y + size - 1, thickness, color);
}



void rgbfilter(BMP* bmp, const char* сomponent, int value){
    for (int j = 0; j < bmp -> bmih.biHeight; j++) {
        for (int i = 0; i < bmp -> bmih.biWidth; i++) {
            if(strcmp(сomponent, "green") == 0){
                bmp->img[j][i].g = value;
            }
            else if(strcmp(сomponent, "blue") == 0){
                bmp->img[j][i].b = value;
            }
            else if(strcmp(сomponent, "red") == 0){
                bmp->img[j][i].r = value;
            }  
        }
    }
}


int rotate(BMP* bmp, int left_x, int left_y, int right_x, int right_y, int angle) {
    int error = SUCCESS;
    int width = right_x - left_x;
    int height = right_y - left_y;
    int img_width = bmp->bmih.biWidth;
    int img_height = bmp->bmih.biHeight;

    int center_x = (right_x + left_x) / 2;
    int center_y = (right_y + left_y) / 2;
    int x = center_x - height / 2;
    int y = center_y - width / 2;

    int border1 = width;
    int border2 = height;
    if (angle == 180){
        border1 = height;
        border2 = width;
    }
    
    RGB** rgb = calloc(border1, sizeof(RGB*));
    if (!rgb) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        error = ERROR_MEM;
    }
    for (int j = 0; j < border1 && !error; j++) {
        rgb[j] = calloc(border2, sizeof(RGB));
        if (!rgb[j]) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            error = ERROR_MEM;
        } 
    }    
    if(!error){
        switch (angle){
            case 180:
                for (int j = 0; j < height; j++) {
                    for (int i = 0; i < width; i++) {
                        if(left_y + j < img_height && i + left_x < img_width && left_y + j >= 0 && i + left_x >= 0){
                            rgb[j][i] = bmp->img[left_y + j][i + left_x];
                        }
                    }
                }
                break;
            case 90:
                for (int j = 0; j < width; j++) {
                    for (int i = 0; i < height; i++) {
                        if (right_y - i - 1 < img_height && j + left_x < img_width && right_y - i - 1 >= 0 && j + left_x >= 0) {
                            rgb[j][i] = bmp->img[right_y - i - 1][j + left_x];
                        }
                    }
                }
                break;
            case 270:
                for (int j = 0; j < width; j++) {
                    for (int i = 0; i < height; i++) {
                        if(left_y + i < img_height && right_x - j < img_width && left_y + i >= 0 && right_x - j >= 0){
                            rgb[j][i] = bmp->img[left_y + i][right_x - j - 1];
                        }
                    }
                }
                break;
            default:
                fprintf(stderr, "Error in angle");
                error = ERROR_VAL;
                break;
        }
        int new_x = (angle == 180) ? left_x : x;
        int new_y = (angle == 180) ? left_y : y;
        for (int j = 0; j < border1 && !error; j++) {
            for (int i = 0; i < border2; i++) {
                setPixel(bmp, new_x + i, new_y + j, rgb[border1 - j - 1][border2 - i - 1]);
            }
        }  
    }
    for (int j = 0; j < border1; j++) {
        free(rgb[j]);
    }
    free(rgb); 

    return error;
}

int parse_coord(const char* str, int* x, int* y) {
    return sscanf(str, "%d.%d", x, y) == 2;
}

int parse_val(const char* str, int* val) {
    return sscanf(str, "%d", val) == 1;
}

int parse_color(const char *str, RGB* rgb) {
    int success = 0;
    int r, g, b;
    if (sscanf(str, "%d.%d.%d", &r, &g, &b) == 3) {
        rgb->r = (uint8_t)r;
        rgb->g = (uint8_t)g;
        rgb->b = (uint8_t)b;
        success = 1;
    }
    return success;
}

int checkcolor(RGB* color){
    int error = SUCCESS;
    if(color->r > 255 || color->r < 0 || color->g > 255 || color->g < 0 || color->b > 255 || color->b < 0){
        error = ERROR_VAL;
    }
    return error;
}

void displayinfo(BMP* bmp){
    printf("width: %d\n", bmp->bmih.biWidth);
    printf("height: %d\n", bmp->bmih.biHeight);
    printf("size: %d\n", bmp->bmih.biSize);
}

void outside_rect(BMP* bmp, int left_x, int left_y, int right_x, int right_y, RGB color){
    for(int i = 0; i < bmp->bmih.biHeight; i++){
        for(int j = 0; j < bmp->bmih.biWidth; j++){
            if((i < left_y || i > right_y) || (j < left_x || j > right_x)){
                setPixel(bmp, j, i, color);
            }
        }
    }
}

void paving(BMP* bmp, int left_x, int left_y, int right_x, int right_y){
    int dy = right_y - left_y;
    int dx = right_x - left_x;
    int height = bmp->bmih.biHeight;
    int width = bmp->bmih.biWidth;
    size_t row_padded = (dx * sizeof(RGB) + 3) & (~3);
    RGB** new = (RGB **)malloc(dy * sizeof(RGB *));
    for (size_t i = 0; i < dy; i++){
        new[dy - 1 - i] = (RGB *)malloc(row_padded);
    }
    for(int y = left_y; y < right_y; y++){
        for(int x = left_x; x < right_x; x++){
            if (x >= 0 && x < width && y >= 0 && y < height) {
                new[y - left_y][x - left_x] = bmp -> img[y][x];
            }
        }
    }
    for(int i = 0; i < bmp->bmih.biHeight; i++){
        for(int j = 0; j < bmp->bmih.biWidth; j++){
            setPixel(bmp, j, i, new[i % dy][j % dx]);
        }
    }
}

void circle_pixel(BMP* bmp, int size, RGB color, RGB color_new){
    for(int i = 0; i < bmp->bmih.biHeight; i++){
        for(int j = 0; j < bmp->bmih.biWidth; j++){
            if(bmp->img[i][j].g == color.g &&
               bmp->img[i][j].r == color.r &&
               bmp->img[i][j].b == color.b){
                for(int y = -size; y <= size; y++){
                    for(int x = -size; x <= size; x++){
                        if(i+y < 0 || i+y >= bmp->bmih.biHeight || j+x < 0 || j+x >= bmp->bmih.biWidth){
                            continue;
                        }
                        if(bmp->img[i+y][j+x].g != color.g ||
                        bmp->img[i+y][j+x].r != color.r ||
                        bmp->img[i+y][j+x].b != color.b){
                            setPixel(bmp, j + x, i + y, color_new);
                        }
                    }
                }
            }
        }
    }
}

void diag_mirror(BMP* bmp, int left_x, int left_y, int right_x, int right_y){
    int dx = right_x - left_x;
    int dy = right_y - left_y;

    if(dx > dy){
        dx = dy;
    }
    else{
        dy = dx;
    }

    right_x = left_x + dx;
    right_y = left_y + dy;

    size_t row_padded = (dx * sizeof(RGB) + 3) & (~3);
    RGB** new1 = (RGB **)malloc(dy * sizeof(RGB *));
    for (size_t i = 0; i < dy; i++){
        new1[dy - 1 - i] = (RGB *)malloc(row_padded);
    }

    rotate(bmp, left_x, left_y, right_x, right_y, 90);
    for (int i = 0; i < dy; i++)
    {
        for (int j = 0; j < dx; j++)
        {
            new1[i][j] = bmp->img[right_y - i - 1][left_x + j];
        }
    }

    for (int i = left_y; i < right_y; i++)
    {
        for (int j = left_x; j < right_x; j++)
        {
            bmp->img[i][j] = new1[i - left_y][j - left_x];
        }
    }
}

void shift(BMP* bmp, int step, char* axis){
    int height = bmp->bmih.biHeight;
    int width = bmp->bmih.biWidth;
    size_t row_padded = (width * sizeof(RGB) + 3) & (~3);
    RGB** new = (RGB **)malloc(height * sizeof(RGB *));
    for (size_t i = 0; i < height; i++){
        new[i] = (RGB *)malloc(row_padded);
    }
    if(strcmp(axis, "x") == 0){
        int step_x = step % width;
        for(int i = 0; i < bmp->bmih.biHeight; i++){
            for(int j = 0; j < bmp->bmih.biWidth; j++){
                new[i][(j + step_x) % width] = bmp->img[i][(j)];
            }
        }
    }
    else if(strcmp(axis, "y") == 0){
        int step_y = step % height;
        for(int i = 0; i < bmp->bmih.biHeight; i++){
            for(int j = 0; j < bmp->bmih.biWidth; j++){
                new[(i + step_y) % height][j] = bmp->img[i][(j)];
            }
        }
    }
    else if(strcmp(axis, "xy") == 0){
        int step_y = step % height;
        int step_x = step % width;
        for(int i = 0; i < bmp->bmih.biHeight; i++){
            for(int j = 0; j < bmp->bmih.biWidth; j++){
                new[(i + step_y) % height][(j + step_x) % width] = bmp->img[i][(j)];
            }
        }
    }
    bmp->img = new;
}

void compress(BMP* bmp, int N){
    int height = bmp->bmih.biHeight;
    int width = bmp->bmih.biWidth;
    int width_old = (width / N);
    int height_old = (height / N);
    size_t row_padded = (width_old * sizeof(RGB) + 3) & (~3);
    RGB** new = (RGB **)malloc(height_old * sizeof(RGB *));
    for (size_t i = 0; i < height_old; i++){
        new[i] = (RGB *)malloc(row_padded);
    }

    for(int i = 0; i < height_old; i++){
        for(int j = 0; j < width_old; j++){
            int r = 0;
            int g = 0;
            int b = 0;
            for (int h = i*N; h < N*i + N; h++)
            {
                for (int u = j*N; u < N*j + N; u++)
                {
                    g += bmp->img[h][u].g;  
                    r += bmp->img[h][u].r;  
                    b += bmp->img[h][u].b;  
                }
            }
            new[i][j].g = g / (N * N);
            new[i][j].r = r / (N * N);
            new[i][j].b = b / (N * N);
            
        }
    }
    bmp->bmih.biHeight = height_old;
    bmp->bmih.biWidth = width_old;
    bmp->bmih.biSizeImage = height_old * row_padded;
    bmp->img = new;

}

void romb(BMP* bmp, int x, int y, int size, RGB color){
    int a = (int)sqrt(size*size + size*size) / 2 - 1;
    int left_x = x - a;
    int right_x = x + a;
    int center_y = y + a;
    int down_y = y + 2*a;
    
    drawLine(bmp, x, y, right_x, center_y, color);
    drawLine(bmp, x, y, left_x, center_y, color);
    drawLine(bmp, right_x, center_y, x, down_y, color);
    drawLine(bmp, left_x, center_y, x, down_y, color);
    for (int i = y; i < down_y; i++)
    {
        int dx = abs(i - y - a);
        for (int t = left_x + dx; t < right_x - dx; t++)
        {
            setPixel(bmp, t, i, color);
        }     
    }
}

void flip_squares(BMP* bmp, int size, char* orientation){
    int size_y = 0;
    int size_x = 0;
    int val = 0;
    int index = 0;
    for(int i = 0; i < bmp->bmih.biHeight; i += size){
        for(int j = 0; j < bmp->bmih.biWidth; j += size){
            size_y = 0;
            size_x = 0;
            if((index + val) % 2 == 1){
                size_t row_padded = (size * sizeof(RGB) + 3) & (~3);
                RGB** new = (RGB **)malloc(size * sizeof(RGB *));
                for (size_t k = 0; k < size; k++){
                    new[k] = (RGB *)malloc(row_padded);
                }
                for(int y = i; y < (i + size); y++){
                    for(int x = j; x < (j + size); x++){
                            if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                new[y - i][x - j] = bmp -> img[y][x];
                                
                            }
                            if(y == bmp->bmih.biHeight){
                                size_y = y - i;
                            }
                            if (x == bmp->bmih.biWidth)
                            {
                                size_x = x - j;
                            }
                            
                        }
                    }
                if(strcmp("vertical", orientation) == 0){
                    for(int y = i; y < i + size; y++){
                        for(int x = j; x < j + size; x++){
                                if(i + size >= bmp->bmih.biHeight){
                                    if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                        bmp -> img[y][x] = new[i + size_y - 1 - y][x - j];
                                    }
                                }
                                // else if(j + size >= bmp->bmih.biWidth){
                                //     if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                //         bmp -> img[y][x] = new[y - i][j + size_x - 1 - x];
                                //     }
                                // }                          
                                else{
                                    if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                        bmp -> img[y][x] = new[i + size - 1 - y][x - j];
                                    }
                                }
                                
                            }
                        }
                }
                else if(strcmp("horizontal", orientation) == 0){
                    for(int y = i; y < i + size; y++){
                        for(int x = j; x < j + size; x++){
                                if(j + size >= bmp->bmih.biWidth){
                                    if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                        bmp -> img[y][x] = new[y - i][j + size_x - 1 - x];
                                    }
                                }
                                else{
                                    if (x >= 0 && x < bmp->bmih.biWidth && y >= 0 && y < bmp->bmih.biHeight) {
                                        bmp -> img[y][x] = new[y - i][j + size - 1 - x];
                                    }
                                }
                            }
                        }
                }
            }  
            index++; 
        }
        index = 0;
        val++;   
    }
        
}

void blur(BMP* bmp, int size){
    int img_width = bmp->bmih.biWidth;
    int img_height = bmp->bmih.biHeight;
    int row_padded = (img_width * sizeof(RGB) + 3) & (~3);
    if(size % 2 == 0){
        size++;
    }
    RGB** new = (RGB **)malloc(img_height * sizeof(RGB *));
        for (size_t i = 0; i < img_height; i++){
            new[img_height - 1 - i] = (RGB *)malloc(row_padded);
    }

    for (int i = 0; i < img_height; i++)
    {
        for (int j = 0; j < img_width; j++)
        {
            float r = 0;
            float g = 0;
            float b = 0;

            for (int h = -1 * (size / 2); h <= size / 2; h++)
            {
                for (int x = -1 * (size / 2); x <= size / 2; x++)
                {
                    int dy = i + h;
                    int dx = j + x;
                    if(dy < 0){
                        dy = -1 * dy;
                    }
                    else if (dy >= img_height){
                        dy = 2*img_height - dy - 2;
                    }

                    if(dx < 0){
                        dx = -1 * dx;
                    }
                    else if (dx >= img_width){
                        dx = 2*img_width - dx - 2;
                    }
                    r += bmp->img[dy][dx].r;
                    g += bmp->img[dy][dx].g;
                    b += bmp->img[dy][dx].b;
                    
                }
            } 
            r = (r / (size * size));
            g = (g / (size * size));
            b = (b / (size * size));
            new[i][j].r = round(((r)));
            new[i][j].b = round((b));
            new[i][j].g = round(((g)));  
        }
    }
    bmp->img = new;
}

int main(int argc, char** argv){
    int error = SUCCESS;
    printf("Course work for option 4.12, created by Stepan Rodimanov.\n");
    if ((strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printHelp();
    }
    else{
        char flag;
        int opt;
        int count = 0;
        int quanity = 0;
        char* output = NULL;
        BMP* bmp = NULL;
        char* component_name = NULL;
        int component_value = 0;
        int fill = 0; 
        int x;
        int y;
        int size;
        int thickness;
        RGB* color = malloc(sizeof(RGB));
        RGB* fill_color = malloc(sizeof(RGB));
        int right_x;
        int right_y;
        int angle;
        if(!color || !fill_color){
            fprintf(stderr, "Error: Memory allocation failed\n");
            error = ERROR_MEM;
        }
        for (int i = 1; i < argc; i++) {
            if ((strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0)) {
                bmp = readBMP(argv[i+1]);
            }
        }
        if (bmp == NULL && argc > 1) {
            bmp = readBMP(argv[argc-1]);
        }
        if(bmp == NULL){
            error = ERROR_BMP;
        }
        if(!error){
            while ((opt = getopt_long(argc, argv, "S:u:s:t:c:f:F:r:n:v:R:d:a:o:i:I:P:C:O:p", long_options, NULL))) {
                if (opt == -1) break;
                switch (opt) {
                    case 'S':
                        flag = 'S';
                        quanity++;
                        break;  
                    case 'u':
                        if (!parse_coord(optarg, &x, &y)) {
                            fprintf(stderr, "Error generating origin coordinates. Use X.Y\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;  
                    case 's':
                        if (!parse_val(optarg, &size) || size < 0) {
                            fprintf(stderr, "Error entering size.\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 't':
                        if (!parse_val(optarg, &thickness) || thickness < 0) {
                            fprintf(stderr, "Error entering thickness.\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 'c':
                        if (!parse_color(optarg, color) || checkcolor(color)) {
                            fprintf(stderr, "Color format error. Use RRR.GGG.BBB\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 'f':
                        fill = 1;
                        break;
                    case 'F':
                        if (!parse_color(optarg, fill_color) || checkcolor(fill_color)) {
                            fprintf(stderr, "Color format error. Use RRR.GGG.BBB\n");
                            error = ERROR_VAL;
                        }              
                        break; 
                    case 'r':
                        flag = 'r';
                        quanity++;
                        break;
                    case 'n':
                        component_name = malloc(strlen(optarg) + 1);
                        if (!component_name) {
                            fprintf(stderr, "Error: Memory allocation failed\n");
                            error = ERROR_MEM;
                        }
                        else{
                            strcpy(component_name, optarg);
                            if(strcmp(component_name, "red") != 0 && strcmp(component_name, "green") != 0 && strcmp(component_name, "blue") != 0){
                                fprintf(stderr, "Error in component name\n");
                                error = ERROR_VAL;
                            }
                        }     
                        count++;
                        break;            
                    case 'v':
                        if ((!parse_val(optarg, &component_value)) || component_value < 0 || component_value > 255) {
                            fprintf(stderr, "Error in component value\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break; 
                    case 'R':
                        flag = 'R';
                        quanity++;
                        break;  
                    case 'd':
                        if (!parse_coord(optarg, &right_x, &right_y)) {
                            fprintf(stderr, "Coordinate format error. Use X.Y\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 'a':
                        if ((!parse_val(optarg, &angle) || !(angle == 90 || angle == 180 || angle == 270))) {
                            fprintf(stderr, "Error entering angle data.\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 'o':
                        output = malloc(strlen(optarg) + 1);
                        if (!output) {
                            fprintf(stderr, "Error: Memory allocation failed\n");
                            error = ERROR_MEM;
                        }
                        else{
                            strcpy(output, optarg);
                        }
                        break;
                    case 'i':
                        break;
                    case 'I':
                        flag = 'I';
                        quanity++;
                        break;
                    case 'p':
                        flag = 'p';
                        quanity++;
                        break;
                    case 'P':
                        flag = 'P';
                        quanity++;
                        break;
                    case 'C':
                        if (!parse_val(optarg, &size) || size < 0) {
                            fprintf(stderr, "Error entering size.\n");
                            error = ERROR_VAL;
                        }
                        count++;
                        break;
                    case 'O':
                        component_name = malloc(strlen(optarg) + 1);
                        if (!component_name) {
                            fprintf(stderr, "Error: Memory allocation failed\n");
                            error = ERROR_MEM;
                        }
                        else{
                            strcpy(component_name, optarg);
                        }     
                        count++;
                        break;
                    
                    default:
                        fprintf(stderr, "Extra argument\n");  
                        error = ERROR_COMMAND;
                }
            }
        }
        if(output == NULL){
            output = malloc(strlen("out.bmp") + 1);
            if (!output) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                error = ERROR_MEM;
            }
            else{
                strcpy(output, "out.bmp");
            }
        }
        if(bmp){
            if(!error && quanity == 1){
                if(count == 2 && flag == 'r') {
                    rgbfilter(bmp, component_name, component_value);
                }
                else if(count == 4 && flag == 'S') {
                    draw_square(bmp, x, y, size, thickness, *color, fill, *fill_color);
                }
                else if(count == 3 && flag == 'R') {
                    error = rotate(bmp, x, y, right_x, right_y, angle);
                }
                else if(flag == 'p'){
                    blur(bmp, size);
                }
                else if(flag == 'I'){
                    displayinfo(bmp);
                }
            }
            else {
                fprintf(stderr, "Error\n");
                error = ERROR_COMMAND;
            }
            if(output != NULL){
                writeBMP(output, bmp);
            }
            freeBMP(bmp);
            free(bmp);
        }
        if (color) free(color);
        if (fill_color) free(fill_color);   
    }
    return error;
}

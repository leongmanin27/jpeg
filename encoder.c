#include <stdio.h>
#include <stdlib.h>
#include <math.h>
float PI=3.14159265359;
double A=128;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;         // 文件類型
    unsigned int size;           // 文件大小
    unsigned short reserved1, reserved2; 
    unsigned int offset;         // 偏移量
} Header;

typedef struct {
    unsigned int size;           // 結構大小
    int width, height;           // 寬度和高度
    unsigned short planes;    
    unsigned short bits;         // 像素位數
    unsigned int compression;    // 壓縮類型
    unsigned int imagesize;      // 圖像大小
    int xresolution, yresolution;// 水平和垂直分辨率
    unsigned int ncolours;       // 圖像中顏色數
    unsigned int importantcolours;// 重要的顏色數
} InfoHeader;
#pragma pack(pop)

// 保存單個顏色
void saveChannelData(const char *filename, unsigned char *data, int size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    for (int i = 0; i < size; i++) {
        fprintf(file, "%d\n", data[i]);
    }
    fclose(file);
}
int Quantizationtable[8][8] = { //彩度量化表
	{17, 18, 24, 47, 99, 99, 99, 99},
	{18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
	{47, 66, 99, 99, 99, 99, 99, 99},
	{99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
	{99, 99, 99, 99, 99, 99, 99, 99},
	{99, 99, 99, 99, 99, 99, 99, 99}
	};

int Quantizationtable1[8][8]= {//亮度量化表
	16,11,10,16, 24, 40, 51, 61,
	12,12,14,19, 26, 58, 60, 55,
	14,13,16,24, 40, 57, 69, 56,
	14,17,22,29, 51, 87, 80, 62,
	18,22,37,56, 68,109,103, 77,
	24,35,55,64, 81,104,113, 92,
	49,64,78,87,103,121,120,101,
	72,92,95,98,112,100,103, 99};


int zigzag[8][8] = {//zigzag編碼表
	0, 1, 5, 6,14,15,27,28,
	2, 4, 7,13,16,26,29,42,
	3, 8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63 };
struct array {
    unsigned char  R[8][8];
    unsigned char  G[8][8];
    unsigned char  B[8][8];
    double Y[8][8];
    double Cb[8][8];
    double Cr[8][8];
};
typedef struct array pixelarray;
//RGB


struct RGB{
    unsigned char R;
    unsigned char G;
    unsigned char B;
} ;
typedef struct RGB ImgRGB;

void rgb2ycbcr(pixelarray *array){
    
    for(int i=0;i<8;i++){
	    for(int j=0;j<8;j++){
	        array->Y[i][j]  = 0.299*array->R[i][j] + 0.587*array->G[i][j] + 0.114*array->B[i][j];//Y=0.299R+0.587G+0.114B
	        array->Cb[i][j] = 0.596*array->R[i][j] - 0.275*array->G[i][j] - 0.321*array->B[i][j];//Cb=-0.168R-0.331G+0.499B
	        array->Cr[i][j] = 0.212*array->R[i][j] - 0.523*array->G[i][j] + 0.311*array->B[i][j];//Cr=0.5R-0.419G-0.081B
	    }
	}
}

void applyDCT(pixelarray *block , double fastDCT[8][8][8][8]){
    double tmpY[8][8]={};
    double tmpCb[8][8]={};
    double tmpCr[8][8]={};

    for(int u=0;u<8;u++){
        for(int v=0;v<8;v++){
            for(int i=0;i<8;i++){
                for(int j=0;j<8;j++){
                    tmpY[u][v]+=fastDCT[u][v][i][j] * block->Y[i][j];
                    tmpCb[u][v]+=fastDCT[u][v][i][j] * block->Cb[i][j];
                    tmpCr[u][v]+=fastDCT[u][v][i][j] * block->Cr[i][j];
                }         
            }
        }
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block->Y[i][j] = tmpY[i][j];
            block->Cb[i][j] = tmpCb[i][j];
            block->Cr[i][j] = tmpCr[i][j];  
        }
    }
}

void Yquantize(pixelarray *block , int quant_table[8][8]){
    for (int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            block->Y[i][j]=round(block->Y[i][j]/quant_table[i][j]);
        }
    }
}
void CBCRquantize(pixelarray *block , int quant_table[8][8]){
    for (int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            block->Cb[i][j]=round(block->Cb[i][j]/quant_table[i][j]);
            block->Cr[i][j]=round(block->Cr[i][j]/quant_table[i][j]);
        }
    }
}

int main(int argc, char *argv[]) {
    // if (argc != 7) {
    //     printf("Usage: ./encoder 0 [BMP file] [R.txt] [G.txt] [B.txt] [dim.txt]\n");
    //     return 1;

    //     const char *bmpFilename = argv[2];
    //     const char *rFilename = argv[3];
    //     const char *gFilename = argv[4];
    //     const char *bFilename = argv[5];
    //     const char *dimFilename = argv[6];


    //     FILE *bmpFile = fopen(bmpFilename, "rb");
    //     if (bmpFile == NULL) {
    //         perror("Error opening BMP file");
    //         return 1;
    //     }

    //     Header header;
    //     InfoHeader infoHeader;
    //     fread(&header, sizeof(Header), 1, bmpFile);
    //     fread(&infoHeader, sizeof(InfoHeader), 1, bmpFile);

    //     int width = infoHeader.width;
    //     int height = infoHeader.height;
    //     int size = width * height;

    //     unsigned char *rData = malloc(size);
    //     unsigned char *gData = malloc(size);
    //     unsigned char *bData = malloc(size);

    //     fseek(bmpFile, header.offset, SEEK_SET);
    //     for (int i = 0; i < height; i++) {
    //         for (int j = 0; j < width; j++) {
    //             bData[i * width + j] = getc(bmpFile);
    //             gData[i * width + j] = getc(bmpFile);
    //             rData[i * width + j] = getc(bmpFile);
    //         }
    //     }

    //     fclose(bmpFile);

    //     // 存儲RGB到各自txt檔案
    //     saveChannelData(rFilename, rData, size);
    //     saveChannelData(gFilename, gData, size);
    //     saveChannelData(bFilename, bData, size);

    //     // 存儲圖像寬高到dim.txt
    //     FILE *dimFile = fopen(dimFilename, "w");
    //     if (dimFile == NULL) {
    //         perror("Error opening dimension file");
    //         return 1;
    //     }
    //     fprintf(dimFile, "%d %d\n", width, height);
    //     fclose(dimFile);

    //     // 釋放記憶體
    //     free(rData);
    //     free(gData);
    //     free(bData);

    //     return 0;
    // }
    if (argc != 3){
        int i,j,x,y;
        printf("Usage: ./encoder 1 KimberlyNCat.bmp \n");
        return 1;

        const char *bmpFilename = argv[2];              //讀檔
        FILE *bmpFile = fopen(bmpFilename, "rb");
        if (bmpFile == NULL) {
            perror("Error opening BMP file");
            return 1;
        }

        Header header;
        InfoHeader infoHeader;
        fread(&header, sizeof(Header), 1, bmpFile);
        fread(&infoHeader, sizeof(InfoHeader), 1, bmpFile);
        pixelarray array = {};
        ImgRGB** malloc_2D(int , int );
        int W = infoHeader.width;
        int H = infoHeader.height;
        pixelarray image;
        ImgRGB **Data_RGB=malloc_2D(infoHeader.height, infoHeader.width);
        int a,b,c,d;
	    double fastDCT[8][8][8][8]; 
	    double C[8] = {0.707106,1.0,1.0,1.0,1.0,1.0,1.0,1.0}; // Where C(0)=1/根號2、C(n)=1 for 'n' !=0
	
	    for (a=0;a<8;a++) {
            for (b=0;b<8;b++) {
               for (c=0;c<8;c++) {
                     for (d=0;d<8;d++) {
                    fastDCT[a][b][c][d] = 0.25*C[a]*C[b] * cos((double)(2*c+1)*a*PI/2/8) * cos((double)(2*d+1)*b*PI/2/8);//DCT rule
				    }
			    }
		    }
	    }
        //cut every 8*8 array
        for (i=0; i<H; i+=8) {
            for (j=0; j<W; j+=8) {

            //cut every 8*8 array
            for (x=0; x<8; x++) {
                for (y=0; y<8; y++){
                    if (i+x<H && j+y<W) {
                        array.R[x][y] = Data_RGB[i+x][j+y].R;
                        array.G[x][y] = Data_RGB[i+x][j+y].G;
                        array.B[x][y] = Data_RGB[i+x][j+y].B;
                    }
                    else { 
                        array.R[x][y] = 0;  // zero padding 
                        array.G[x][y] = 0;
                        array.B[x][y] = 0;
                    }
                }
            }
            }
            }
            int u,p;
	        rgb2ycbcr(&array);
            //do shift -128
             int m,n;
   			 for(m=0;m<8;m++){
       			 for(n=0;n<8;n++){
            		array.Y[m][n] =array.Y[m][n]  - A;//A=128
            		array.Cb[m][n]=array.Cb[m][n] - A;
            		array.Cr[m][n]=array.Cr[m][n] - A;
        			}
    		}
            //do DCT
            applyDCT(&array, fastDCT);
            Yquantize(&array,Quantizationtable);
            CBCRquantize(&array,Quantizationtable1);
		    }
    }


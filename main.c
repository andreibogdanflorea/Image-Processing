// Florea Andrei-Bogdan, 313CD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "./bmp_header.h"

#define PATHLEN 100
#define BYTE 8

// functie pentru a elibera memoria dintr-un bitmap
void free_map(pixel **bitmap, int height) {
    for (int i = 0; i < height; ++i) {
        free(bitmap[i]);
    }
    free(bitmap);
}

// functie pentru a aloca dinamic un bitmap
pixel **create_bitmap(int height, int width) {
    pixel **bitmap = malloc(height * sizeof(pixel *));
    for (int i = 0; i < height; ++i) {
        bitmap[i] = malloc(width * sizeof(pixel));
    }

    return bitmap;
}

pixel **read_bmp(char *path, bmp_fileheader *header, bmp_infoheader *info) {
    FILE *f_in = fopen(path, "rb");
    if (f_in == NULL) {
        printf("Eroare la deschiderea fisierului %s\n", path);
    }

    fread(header, sizeof(bmp_fileheader), 1, f_in);
    fread(info, sizeof(bmp_infoheader), 1, f_in);

    pixel **bitmap = create_bitmap(info->height, info->width);

    // calculez cat padding voi avea la finalul fiecarei linii
    int row_len = info->width * 3;
    row_len += (4 - (row_len % 4)) % 4;
    int padding = row_len - 3 * info->width;

    // parcurg bitmap-ul
    for (int i = 0; i < info->height; ++i) {
        for (int j = 0; j < info->width; ++j) {
            fread(&bitmap[i][j], sizeof(pixel), 1, f_in);
        }
        // sar peste urmatorii padding bytes 0
        fseek(f_in, padding, SEEK_CUR);
    }

    fclose(f_in);
    return bitmap;
}

void write_bmp(char *path, bmp_fileheader header, bmp_infoheader info,
    pixel **bitmap) {
    FILE *f_out = fopen(path, "wb");
    // scriu header-ul si info-ul
    fwrite(&header, sizeof(bmp_fileheader), 1, f_out);
    fwrite(&info, sizeof(bmp_infoheader), 1, f_out);

    // calculez cat padding voi avea la finalul fiecarei linii
    int row_len = info.width * 3;
    row_len += (4 - (row_len % 4)) % 4;
    int padding = row_len - 3 * info.width;

    // parcurg bitmap-ul
    for (int i = 0; i < info.height; ++i) {
        for (int j = 0; j < info.width; ++j) {
            fwrite(&bitmap[i][j], sizeof(pixel), 1, f_out);
        }
        // scriu byte-ul 0 de padding ori
        for (int j = 0; j < padding; ++j) {
            fputc(0x00, f_out);
        }
    }

    fclose(f_out);
}

pixel **blackwhite(pixel **bitmap, int height, int width) {
    pixel **bwbitmap = create_bitmap(height, width);

    // parcurg bitmap-ul
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            pixel crt_pixel = bitmap[i][j];
            // calculez tonul de gri corespunzator
            int gray_tone = (crt_pixel.R + crt_pixel.G + crt_pixel.B) / 3;
            // si il adaug in noul bitmap
            bwbitmap[i][j].R = gray_tone;
            bwbitmap[i][j].G = gray_tone;
            bwbitmap[i][j].B = gray_tone;
        }
    }

    return bwbitmap;
}

int max(int x, int y) {
    return x > y ? x : y;
}

int min(int x, int y) {
    return x < y ? x : y;
}

pixel **no_crop(pixel **bitmap, int *height, int *width) {
    // dimensiunea noua a patratului este maximul dimensiunilor
    int size = max(*height, *width);
    pixel **nocrop_bmp = create_bitmap(size, size);

    // daca latimea este mai mica, completez la stanga si la dreapta
    if (*height > *width) {
        // numarul de coloane de completat
        int colsToComplete = *height - *width;
        // numarul de coloane de completat in stanga imaginii
        int zoneLeft = (colsToComplete + 1) / 2;
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                // daca sunt in stanga sau in dreapta, completez cu alb
                if (j < zoneLeft || j >= *width + zoneLeft) {
                    nocrop_bmp[i][j].R = 255;
                    nocrop_bmp[i][j].G = 255;
                    nocrop_bmp[i][j].B = 255;
                } else { // altfel copiez din bitmap-ul initial
                    nocrop_bmp[i][j] = bitmap[i][j - zoneLeft];
                }
            }
        }
        // actualizez noua dimensiune din infoheader
        *width = *height;
    } else { // altfel, completez sus si jos
        int linesToComplete = *width - *height;
        int zoneUp = (linesToComplete + 1) / 2;
        for (int  i = 0;  i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                // daca sunt sus sau jos, completez cu alb
                if (i < zoneUp || i >= *height + zoneUp) {
                    nocrop_bmp[i][j].R = 255;
                    nocrop_bmp[i][j].G = 255;
                    nocrop_bmp[i][j].B = 255;
                } else { // altfel copiez din bitmap-ul initial
                    nocrop_bmp[i][j] = bitmap[i - zoneUp][j];
                }
            }
        }
        // actualizez noua dimensiune din infoheader
        *height = *width;
    }

    return nocrop_bmp;
}

int in_bounds(int pos_x, int pos_y, int dim_x, int dim_y) {
    return (pos_x >= 0 && pos_x < dim_x && pos_y >= 0 && pos_y < dim_y);
}

void normalize(int *R, int *G, int *B) {
    if (*B < 0) {
        *B = 0;
    }
    if (*G < 0) {
        *G = 0;
    }
    if (*R < 0) {
        *R = 0;
    }
    if (*B > 255) {
        *B = 255;
    }
    if (*G > 255) {
        *G = 255;
    }
    if (*R > 255) {
        *R = 255;
    }
}

pixel **filter(pixel **bitmap, int height, int width, char *path) {
    FILE *f_filter = fopen(path, "rt");
    // citesc dimensiunea filtrului
    int size;
    fscanf(f_filter, "%d", &size);

    // aloc memorie pentru matricea filtrului si o citesc
    int **filter_matrix = malloc(size * sizeof(int *));
    for (int i = 0; i < size; ++i) {
        filter_matrix[i] = malloc(size * sizeof(int));
    }

    for (int i = size - 1; i >= 0; --i) {
        for (int j = 0; j < size; ++j) {
            fscanf(f_filter, "%d", &filter_matrix[i][j]);
        }
    }
    fclose(f_filter);

    pixel **filter_bmp = create_bitmap(height, width);

    // parcurg bitmap-ul
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int newR = 0, newG = 0, newB = 0;
            for (int row = 0; row < size; ++row) {
                for (int col = 0; col < size; ++col) {
                    // linia si coloana din bitmap-ul initial, corespunzatoare
                    // randului si coloanei din matricea filtrului
                    int bmp_row = i + row - size / 2;
                    int bmp_col = j + col - size / 2;

                    // daca este un punct valid
                    if (in_bounds(bmp_row, bmp_col, height, width)) {
                        pixel crt_pixel = bitmap[bmp_row][bmp_col];
                        // il iau in calcul
                        newR += crt_pixel.R * filter_matrix[row][col];
                        newG += crt_pixel.G * filter_matrix[row][col];
                        newB += crt_pixel.B * filter_matrix[row][col];
                    }
                }
            }

            // verific daca nu cumva valorile sunt < 0 sau > 255 si le schimb
            normalize(&newR, &newG, &newB);
            // le adaug in bitmap-ul nou
            filter_bmp[i][j].B = newB;
            filter_bmp[i][j].G = newG;
            filter_bmp[i][j].R = newR;
        }
    }

    for (int i = 0; i < size; ++i) {
        free(filter_matrix[i]);
    }
    free(filter_matrix);

    return filter_bmp;
}

pixel **pooling(pixel **bitmap, int height, int width, char *path) {
    FILE *f_pooling = fopen(path, "rt");
    // citesc datele
    char type;
    int size;
    fscanf(f_pooling, "%c %d", &type, &size);
    fclose(f_pooling);

    pixel **pooling_bmp = create_bitmap(height, width);

    // parcurg bitmap-ul
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int newR, newG, newB;
            if (type == 'm') {
                // daca fac min pooling, initializez noul pixel
                // cu o valoare maxima
                newR = 255, newG = 255, newB = 255;
            } else {
                // daca fac max pooling, initializez noul pixel
                // cu o valoare minima
                newR = 0, newG = 0, newB = 0;
            }

            // parcurg matricea de pooling
            for (int row = 0; row < size; ++row) {
                for (int col = 0; col < size; ++col) {
                    // calculez ce linie si coloana din matricea initiala
                    // ii corespunde elementului din matricea de pooling
                    int bmp_row = i + row - size / 2;
                    int bmp_col = j + col - size / 2;

                    // daca este un punct valid
                    if (in_bounds(bmp_row, bmp_col, height, width)) {
                        pixel crt_pixel = bitmap[bmp_row][bmp_col];
                        // actualizez elementele
                        if (type == 'm') {
                            newR = min(newR, crt_pixel.R);
                            newG = min(newG, crt_pixel.G);
                            newB = min(newB, crt_pixel.B);
                        } else {
                            newR = max(newR, crt_pixel.R);
                            newG = max(newG, crt_pixel.G);
                            newB = max(newB, crt_pixel.B);
                        }
                    } else {
                        if (type == 'm') {
                            newR = 0, newG = 0, newB = 0;
                        }
                    }
                }
            }

            // adaug elementele gasite in bitmap-ul nou
            pooling_bmp[i][j].B = newB;
            pooling_bmp[i][j].G = newG;
            pooling_bmp[i][j].R = newR;
        }
    }

    return pooling_bmp;
}

// functie care returneaza diferenta dintre 2 pixeli, pentru clustering
int difference(pixel x, pixel y) {
    return abs(x.R - y.R) + abs(x.G - y.G) + abs(x.B - y.B);
}

pixel **clustering(pixel **bitmap, int height, int width, char *path) {
    // citesc threshold-ul
    FILE *f_cluster = fopen(path, "rt");
    int threshold;
    fscanf(f_cluster, "%d", &threshold);

    // matrice pentru a tine cont de pixelii folositi
    int **used = malloc(height * sizeof(int *));
    for (int i = 0; i < height; ++i) {
        used[i] = calloc(width, sizeof(int));
    }

    pixel **cluster_bmp = create_bitmap(height, width);

    // vectori pentru directie
    int dx[] = {-1, 0, 1, 0};
    int dy[] = {0, 1, 0, -1};

    // struct folosit pentru coada (unde retin pozitiile prin care trec)
    typedef struct {
        int x, y;
    } pos;

    // parcurg pixelii din bitmap
    for (int i = height - 1; i >= 0; --i) {
        for (int j = 0; j < width; ++j) {
            if (used[i][j]) {
                // daca pixelul e folosit intr-o zona, trec peste
                continue;
            }

            // algoritmul lui lee pentru a determina zona din care face parte
            // punctul de start
            int front = 0, back = 0;
            // adaug punctul de start in coada si marchez ca am trecut prin el
            pos *queue = malloc(height * width * sizeof(pos));
            queue[0].x = i, queue[0].y = j;
            pixel start = bitmap[i][j];
            used[i][j] = 1;
            int sumR = start.R, sumG = start.G, sumB = start.B;
            int number_of_pixels = 1;

            while (front <= back) {
                int posx = queue[front].x;
                int posy = queue[front].y;
                front++;
                for (int k = 0; k < 4; ++k) {
                    int newx = posx + dx[k];
                    int newy = posy + dy[k];
                    // daca pozitia este in afara map-ului, sar peste
                    if (!in_bounds(newx, newy, height, width)) {
                        continue;
                    }
                    if (!used[newx][newy]) {
                        pixel newpixel = bitmap[newx][newy];
                        if (difference(start, newpixel) <= threshold) {
                            // am gasit un pixel din zona, si il contorizez
                            number_of_pixels++;
                            sumR += newpixel.R;
                            sumG += newpixel.G;
                            sumB += newpixel.B;
                            used[newx][newy] = 1;
                            back++;
                            queue[back].x = newx, queue[back].y = newy;
                        }
                    }
                }
            }

            // fac media ceruta
            pixel average;
            average.R = sumR / number_of_pixels;
            average.G = sumG / number_of_pixels;
            average.B = sumB / number_of_pixels;

            // parcurg zona si pun in noul bitmap pixel-ul mediu
            for (int k = 0; k <= back; ++k) {
                cluster_bmp[queue[k].x][queue[k].y].R = average.R;
                cluster_bmp[queue[k].x][queue[k].y].G = average.G;
                cluster_bmp[queue[k].x][queue[k].y].B = average.B;
            }
            free(queue);
        }
    }

    for (int i = 0; i < height; ++i) {
        free(used[i]);
    }
    free(used);

    return cluster_bmp;
}

// functie folosita pentru a crea calea spre un fisier cu un anumit sufix
char *create_write_path(char *bmp_path, char *suffix) {
    char *path = malloc(PATHLEN * sizeof(char));
    strcpy(path, bmp_path);
    path = strtok(path, ".bmp");
    strcat(path, suffix);
    return path;
}

int main() {
    char *bmp_path = malloc(PATHLEN * sizeof(char));
    char *filter_path = malloc(PATHLEN * sizeof(char));
    char *pooling_path = malloc(PATHLEN * sizeof(char));
    char *clustering_path = malloc(PATHLEN * sizeof(char));

    FILE *f_in = fopen("input.txt", "rt");
    if (f_in == NULL) {
        printf("Eroare la deschiderea fisierului input.txt\n");
        return 0;
    }

    // citesc din input.txt calea spre fiecare fisier
    fscanf(f_in, "%s", bmp_path);
    fscanf(f_in, "%s", filter_path);
    fscanf(f_in, "%s", pooling_path);
    fscanf(f_in, "%s", clustering_path);
    fclose(f_in);

    bmp_fileheader header;
    bmp_infoheader info;
    // citesc bitmap-ul
    pixel **bitmap = read_bmp(bmp_path, &header, &info);
    char *write_path;

    // creez bitmap-ul alb-negru
    pixel **bwbitmap = blackwhite(bitmap, info.height, info.width);
    // aflu calea la care sa scriu bmp-ul
    write_path = create_write_path(bmp_path, "_black_white.bmp");
    write_bmp(write_path, header, info, bwbitmap);
    free(write_path);
    free_map(bwbitmap, info.height);

    // analog pentru celelalte bmp-uri
    bmp_infoheader newinfo = info;
    // aici se modifica si dimensiunile din infoheader
    pixel **noCropbitmap = no_crop(bitmap, &newinfo.height, &newinfo.width);
    write_path = create_write_path(bmp_path, "_nocrop.bmp");
    write_bmp(write_path, header, newinfo, noCropbitmap);
    free(write_path);
    free_map(noCropbitmap, newinfo.height);

    pixel **filter_bmp = filter(bitmap, info.height, info.width, filter_path);
    write_path = create_write_path(bmp_path, "_filter.bmp");
    write_bmp(write_path, header, info, filter_bmp);
    free(write_path);
    free_map(filter_bmp, info.height);

    pixel **pool_bmp = pooling(bitmap, info.height, info.width, pooling_path);
    write_path = create_write_path(bmp_path, "_pooling.bmp");
    write_bmp(write_path, header, info, pool_bmp);
    free(write_path);
    free_map(pool_bmp, info.height);

    pixel **cluster_bmp;
    cluster_bmp = clustering(bitmap, info.height, info.width, clustering_path);
    write_path = create_write_path(bmp_path, "_clustered.bmp");
    write_bmp(write_path, header, info, cluster_bmp);
    free(write_path);
    free_map(cluster_bmp, info.height);

    free_map(bitmap, info.height);
    free(bmp_path);
    free(filter_path);
    free(pooling_path);
    free(clustering_path);
    return 0;
}

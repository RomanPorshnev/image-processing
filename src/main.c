#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <png.h>


void printHelp(){
	printf("\nRange of values of colors and transparency: [0, 255]\n\n");
	printf("\nAvailable functions: \n\n");
	printf("-l --line -s --start <valueX>,<valueY> -e --end <valueX>,<valueY> -c --rgb <valueRED>,<valueGREEN>,<valueBLUE> -v --transparency <value> -w --width <value> <filename_input> <filename_output>\n\n");
	printf("-o --circle <1> -s --start <valueX>,<valueY> -e --end <valueX>,<valueY> <filename_input> <filename_input>\n\t\tor\n");
	printf("-o --circle <2> -s --start <valueX>,<valueY> -r --radious <value> <filename_input> <filename_output>\n\n");
	printf("-t --trim s --start <valueX>,<valueY> -e --end <valueX>,<valueY> <filename_input> <filename_output>\n\n");
	printf("-h --help -? - help\n\n");
}


struct Png{
    int width, height, true_height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
};


struct Configs{
	char func;
	int x0;
	int y0;
	int x1;
	int y1;
	int red;
	int green;
	int blue;
	int transparency;
	int width;
	int radious;
	int type;
	int activate_s;
	int activate_e;
	int activate_c;
	int activate_v;
	int activate_w;
	int activate_r;
};


int read_png_file(char *file_name, struct Png *image) {
    int x,y;
    char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp){
        printf("Error!!!\nFile couldn't be open!!!\n");
        return 1;
    }

    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)){
        // Some error handling: file is not recognized as a PNG
        printf("Error!!!\nFile is not recognized as a PNG!!!\n");
        return 1;
    }

    /* initialize stuff */
    image->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!image->png_ptr){
        // Some error handling: png_create_read_struct failed
        printf("Error!!!\npng_create_read_struct failed!!!\n");
        return 1;
    }

    image->info_ptr = png_create_info_struct(image->png_ptr);
    if (!image->info_ptr){
        // Some error handling: png_create_info_struct failed
        printf("Error!!!\npng_create_info_struct failed!!!\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(image->png_ptr))){
        // Some error handling: error during init_io
        printf("Error!!!\nError during png_init_io!!!\n");
        return 1;
    }

    png_init_io(image->png_ptr, fp);
    png_set_sig_bytes(image->png_ptr, 8);

    png_read_info(image->png_ptr, image->info_ptr);

    image->width = png_get_image_width(image->png_ptr, image->info_ptr);
    image->height = png_get_image_height(image->png_ptr, image->info_ptr);
    image->true_height = png_get_image_height(image->png_ptr, image->info_ptr);
    image->color_type = png_get_color_type(image->png_ptr, image->info_ptr);
    image->bit_depth = png_get_bit_depth(image->png_ptr, image->info_ptr);

    png_read_update_info(image->png_ptr, image->info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(image->png_ptr))){
        // Some error handling: error during read_image
        printf("Error!!!\nError during png_read_image!!!\n");
        return 1;
    }

    image->row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * image->height);
    for (y = 0; y < image->height; y++)
        image->row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(image->png_ptr, image->info_ptr));

    png_read_image(image->png_ptr, image->row_pointers);

    fclose(fp);
}

int write_png_file(char *file_name, struct Png *image) {
    int x,y;
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp){
        printf("Error!!!\nFile couldn't be open!!!\n");
        return 1;
    }

    /* initialize stuff */
    image->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!image->png_ptr){
        printf("Error!!!\npng_create_write_struct failed!!!\n");
        return 1;
    }

    image->info_ptr = png_create_info_struct(image->png_ptr);
    if (!image->info_ptr){
        printf("Error!!!\npng_create_info_struct failed!!!\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(image->png_ptr))){
        printf("Error!!!\nError during png_init_io!!!\n");
        return 1;
    }

    png_init_io(image->png_ptr, fp);


    /* write header */
    if (setjmp(png_jmpbuf(image->png_ptr))){
        printf("Error!!!\nError during png_write_info!!!\n");
        return 1;
    }

    png_set_IHDR(image->png_ptr, image->info_ptr, image->width, image->height,
                 image->bit_depth, image->color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(image->png_ptr, image->info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(image->png_ptr))){
        printf("Error!!!\nError during png_write_image!!!\n");
        return 1;
    }

    png_write_image(image->png_ptr, image->row_pointers);


    /* end write */
    if (setjmp(png_jmpbuf(image->png_ptr))){
        printf("Error!!!\nError during png_write_end!!!\n");
        return 1;
    }

    png_write_end(image->png_ptr, NULL);

    /* cleanup heap allocation */
    for (y = 0; y < image->true_height; y++)
        free(image->row_pointers[y]);
    free(image->row_pointers);

    fclose(fp);
}


void thickness(struct Png *image, int x, int y, int radious, int red, int green, int blue, int transparency){


	for(int i = 0; i < image->height; i++){
		for(int j = 0; j < image->width; j++){
			double r = (x - j) * (x - j) + (y - i) * (y - i);
    		if (r <= (float)(radious * radious)){
    			png_byte *row = image->row_pointers[i];
    			png_byte *ptr = &(row[j * 4]);
    			ptr[0] = red;
    			ptr[1] = green;
    			ptr[2] = blue;
    			ptr[3] = transparency;
    		} 
		}
	}
}
void line(struct Png *image, int x0, int y0, int x1, int y1, int red, int green, int blue, int transparency, int wd){
	
	int addx, addy;
	float newwd = (float)wd / 2;
	int x = x0, y = y0;
    int deltaX = abs(x1 - x0);
    int deltaY = abs(y1 - y0);
    int signX = x0 < x1 ? 1 : -1;
    int signY = y0 < y1 ? 1 : -1;
    int error = deltaX - deltaY;
    while(x0 != x1 || y0 != y1){
        png_byte *row = image->row_pointers[y0];
        png_byte *ptr = &(row[x0 * 4]);
	    ptr[0] = red;
	    ptr[1] = green;
	    ptr[2] = blue;
	    ptr[3] = transparency;
	    
	    thickness(image, x0, y0, (wd - 1) / 2, red, green, blue, transparency);

        int error2 = error;
        if(error2 > -deltaY) 
        {
            error -= deltaY;
            x0 += signX;
        }
        if(error2 < deltaX) 
        {
            error += deltaX;
            y0 += signY;
        }
    }
    png_byte *row = image->row_pointers[y0];
    png_byte *ptr = &(row[x0 * 4]);
    ptr[0] = red;
	ptr[1] = green;
	ptr[2] = blue;
	thickness(image, x0, y0, (wd - 1) / 2, red, green, blue, transparency);
}


void circle(struct Png *image, int x, int y, int radious){

	for(int i = 0; i < image->height; i++){
		for(int j = 0; j < image->width; j++){
			double r = (x - j) * (x - j) + (y - i) * (y - i);
    		if (r < (float)(radious * radious)){
    			png_byte *row = image->row_pointers[i];
    			png_byte *ptr = &(row[j * 4]);
    			ptr[0] = 255 - ptr[0];
    			ptr[1] = 255 - ptr[1];
    			ptr[2] = 255 - ptr[2];
    		} 
		}
	}

}

void fill(int **arr, int x0, int y0, int x1, int y1){

	for(int i = y0; i <= y1; i++)
		for(int j = x0; j <= x1; j++)
			arr[i][j] = 1;

}

void trim(struct Png *image, int x0, int y0, int x1, int y1){

	for(int i = y0; i <= y1; i++)
		for(int j = x0; j <= x1; j++){
			png_byte *row1 = image->row_pointers[i];
    		png_byte *ptr1 = &(row1[j * 4]);
    		png_byte *row = image->row_pointers[i-y0];
   			png_byte *ptr = &(row[(j - x0) * 4]);
   			ptr[0] = ptr1[0];
  			ptr[1] = ptr1[1];
  			ptr[2] = ptr1[2];
    		ptr[3] = ptr1[3];
		}
	image->height = abs(y1-y0);
	image->width = abs(x1-x0);
	
}
int main(int argc, char* argv[]){
	int k = 0;
	char *pch;
	int xs, ys;
	struct Png image;
	struct Configs config = {' ', 0, 0, 0, 0, 255, 255, 255, 255, 1, 0, -1, 0};
	char *opts = "lto:s:e:c:r:h?w:v:i";
	struct option longOpts[]={
		{"line", no_argument, NULL, 'l'},
		{"start", required_argument, NULL, 's'},
		{"end", required_argument, NULL, 'e'},
		{"rgb", required_argument, NULL, 'c'},
		{"transparency", required_argument, NULL, 'v'},
		{"width", required_argument, NULL, 'w'},
		{"circle", required_argument, NULL, 'o'},
		{"radious", required_argument, NULL, 'r'},
		{"trim", no_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{"info", no_argument, NULL, 'i'},
		{NULL, 0, NULL, 0}
	};
	int opt;
	int longIndex;
	opt = getopt_long(argc, argv, opts, longOpts, &longIndex);
	while(opt != -1){
		switch(opt){

			case 'l':
				config.func = 'l';
				break;
			case 'o':
				config.func = 'o';
				if (strcmp(optarg, "0") != 0 && atoi(optarg) == 0){
					printHelp();
					return 0;
				}
				config.type = atoi(optarg);
				break;
			case 't':
				config.func = 't';
				break;


			case 's':
				k = 0;
				pch = strtok(optarg, ",");
				while(pch != NULL){
					if (strcmp(pch, "0") != 0 && atoi(pch) == 0){
						printHelp();
						return 0;
					}
					if (k == 0)
						config.x0 = atoi(pch);
					if (k == 1)
						config.y0 = atoi(pch);
					k++;
					pch = strtok (NULL, ",");
				}
				if (k != 2){
					printHelp();
					return 0;
				}
				config.activate_s = 1;
				break;


			case 'e':
				k = 0;
				pch = strtok(optarg, ",");
				while(pch != NULL){
					if (strcmp(pch, "0") != 0 && atoi(pch) == 0){
						printHelp();
						return 0;
					}
					if (k == 0)
						config.x1 = atoi(pch);
					if (k == 1)
						config.y1 = atoi(pch);
					k++;
					pch = strtok (NULL, ",");
				}
				if (k != 2){
					printHelp();
					return 0;
				}
				config.activate_e = 1;
				break;


			case 'c':
				k = 0;
				pch = strtok(optarg, ",");
				while(pch != NULL){
					if (strcmp(pch, "0") != 0 && atoi(pch) == 0){
						printHelp();
						return 0;
					}
					if (k == 0)
						config.red = atoi(pch);
					if (k == 1)
						config.green = atoi(pch);
					if (k == 2)
						config.blue = atoi(pch);
					pch = strtok (NULL, ",");
					k++;
				}
				if (k != 3){
					printHelp();
					return 0;
				}
				config.activate_c = 1;
				break;


			case 'v':
				if (strcmp(optarg, "0") != 0 && atoi(optarg) == 0){
					printHelp();
					return 0;
				}
				config.transparency = atoi(optarg);
				config.activate_v = 1;
				break;


			case 'r':
				if (strcmp(optarg, "0") != 0 && atoi(optarg) == 0){
					printHelp();
					return 0;
				}
				config.radious = atoi(optarg);
				config.activate_r = 1;
				break;


			case 'w':
				if (strcmp(optarg, "0") != 0 && atoi(optarg) == 0){
					printHelp();
					return 0;
				}
				config.width = atoi(optarg);
				config.activate_w = 1;
				break;

			case 'h':
			case '?':
				printHelp();
				return 0;
			
		}
		opt = getopt_long(argc, argv, opts, longOpts, &longIndex);
	}
	
	argc -= optind;
	argv += optind;

	if (argc != 2){
		printHelp();
		return 0;
	}

	if (read_png_file(argv[0], &image))
		return 0;
	if (png_get_color_type(image.png_ptr, image.info_ptr) == PNG_COLOR_TYPE_RGB){
        printf("Error!!!\nType of color is RGB, but must be RGBA!!!\n");
        return 0;
    }

    if (png_get_color_type(image.png_ptr, image.info_ptr) != PNG_COLOR_TYPE_RGBA){
        printf("Error!!!\nType of color is not RGBA!!!\n");
        return 0;
    }
	if ((config.x0 < 0) || (config.x0 > image.width - 1) || (config.y0 < 0) || (config.y0 > image.height - 1) || 
		(config.x1 < 0) || (config.x1 > image.width - 1) || (config.y1 < 0) || (config.y1 > image.height - 1)){
			printf("Error!!!\n");
			printf("Invalid values of coordinates!!!\n");
			printf("Minimal size of coordinates = 0\n");
			printf("Maximal size of coordinates of x = %d\n", image.width - 1);
			printf("Maximal size of coordinates of y = %d\n", image.height - 1);
			return 0;
	}

	if ((config.red < 0) || (config.red > 255) || (config.blue < 0) ||
		(config.blue > 255) || (config.green < 0) || (config.green > 255)){
			printf("Error!!!\n");
			printf("Invalid values of colors!!!\n");
			printf("Minimal value of colors = 0\n");
			printf("Maximal value of colors = 255\n");
			return 0;
	}

	if ((config.transparency < 0) || (config.transparency > 255)){
			printf("Error!!!\n");
			printf("Invalid value of transparency!!!\n");
			printf("Minimal value of transparency = 0\n");
			printf("Error!!! Maximal value of transparency = 255\n");
			return 0;
	}

	if ((config.width < 0)){
			printf("Error!!!\n");
			printf("Invalid value of width of line!!!\n");
			printf("Minimal value of width = 0\n");
			return 0;
	}
 
	if (config.func == 'l'){
		if ((config.activate_s == 0) && (config.activate_e == 0)){
			line(&image, 0, 0, image.width - 1, image.height - 1, config.red, config.green, config.blue, config.transparency, config.width);
		}
		else if ((config.activate_s == 1) && (config.activate_e == 1)){
			line(&image, config.x0, config.y0, config.x1, config.y1, config.red, config.green, config.blue, config.transparency, config.width);
		}
		else{
			printHelp();
			return 0;
		}
		
	}

	if (config.func == 'o'){
		if (config.type == 1){
			if ((config.activate_s == 0) && (config.activate_e == 0)){
				config.x0 = 0; config.y0 = 0;
				config.x1 = image.width - 1; config.y1 = image.height - 1;
				xs = (config.x0 + config.x1) / 2;
				ys = (config.y0 + config.y1) / 2;
				if (abs(config.x0 - config.x1) != abs(config.y0 - config.y1)){
					printf("Error!!!\nIt's not a quadrate!!!\n");
				}
			}
			else if ((config.activate_s == 1) && (config.activate_e == 1)){
				xs = (config.x0 + config.x1) / 2;
				ys = (config.y0 + config.y1) / 2;
				if (abs(config.x0 - config.x1) != abs(config.y0 - config.y1)){
					printf("Error!!!\nIt's not a quadrate!!!\n");
				}
			}
			else{
				printHelp();
				return 0;
			}
			circle(&image, xs, ys, abs(xs - config.x0));
		}

		else if (config.type == 2){
			if (config.activate_s == 0){
				config.x0 = (image.width - 1) / 2;
				config.y0 = (image.height - 1) / 2;
				circle(&image, config.x0, config.y0, config.radious);
			}
			else if (config.activate_s == 1){
				circle(&image, config.x0, config.y0, config.radious);
			}
			else{
				printHelp();
				return 0;
			}

		}
		else{
			printHelp();
			return 0;
		}

	}

	if (config.func == 't'){
		int t;
		if (config.x0 > config.x1){
			t = config.x0; config.x0 = config.x1; config.x1 = t;
		}
		if (config.y0 > config.y1){
			t = config.y0; config.y0 = config.y1; config.y1 = t;
		}
		trim(&image, config.x0, config.y0, config.x1, config.y1);		

	}
	if (write_png_file(argv[1], &image))
		return 0;
	return 0;
}

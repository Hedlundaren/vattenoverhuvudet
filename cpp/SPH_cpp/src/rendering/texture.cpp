#include "rendering/texture.hpp"


// Simple helper to make a single buffer object.
GLuint makeBO(GLenum type, void* data, GLsizei size, int accessFlags) {
    GLuint bo;
    glGenBuffers(1, &bo);
    glBindBuffer(type, bo);
    glBufferData(type, size, data, accessFlags);
    return(bo);
}

// Helper function to make a buffer object of some size
GLuint makeTextureBuffer(int w, int h, GLenum format, GLint internalFormat) {
	GLuint buffertex;

	glGenTextures(1, &buffertex);
	glBindTexture(GL_TEXTURE_2D, buffertex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, NULL);

	return buffertex;
}

// Load text from a file.
char* loadFile(char* name) {
	long size;
	char* buffer;
	FILE* fp = fopen(name, "rb");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	buffer = (char*)malloc(size + 1);
	fseek(fp, 0, SEEK_SET);
	int res = fread(buffer, 1, size, fp);
	if(res != size) {
		exit(-1);
	}
	fclose(fp);
	buffer[size] = '\0';
	return(buffer);
}



static short le_short(unsigned char *bytes) {
	return bytes[0] | ((char)bytes[1] << 8);
}

// Read a tga file into a buffer for use as an OpenGL texture.
// Original code by Joe Groff, modified to handle alpha channels.
void *readTga(const char *filename, int *width, int *height, int *alpha) {

	// TGA header for loading things into.
	struct tga_header {
		char id_length;
		char color_map_type;
		char data_type_code;
		unsigned char color_map_origin[2];
		unsigned char color_map_length[2];
		char color_map_depth;
		unsigned char x_origin[2];
		unsigned char y_origin[2];
		unsigned char width[2];
		unsigned char height[2];
		char bits_per_pixel;
		char image_descriptor;
	} header;

	int color_map_size, pixels_size;
	FILE *f;
	size_t read;
	void *pixels;

	// Try to open the file.
	f = fopen(filename, "rb");
	if(!f) {
		fprintf(stderr, "Unable to open %s for reading\n", filename);
		return NULL;
	}

	// Check for valid header data.
	read = fread(&header, 1, sizeof(header), f);
	if(read != sizeof(header)) {
		fprintf(stderr, "%s has incomplete tga header\n", filename);
		fclose(f);
		return NULL;
	}
	if(header.data_type_code != 2) {
		fprintf(
				stderr,
				"%s is not an uncompressed RGB tga file\n",
				filename
		);
		fclose(f);
		return NULL;
	}
	if((header.bits_per_pixel != 32) && (header.bits_per_pixel != 24)) {
		fprintf(
				stderr,
				"%s is not 24/32-bit uncompressed RGB/A tga file.\n",
				filename
		);
		fclose(f);
		return NULL;
	}

	// Return to the outside if an alpha channel is present.
	if(header.bits_per_pixel == 32) {
		*alpha = 1;
	}
	else {
		*alpha = 0;
	}

	// Only handling non-palleted images.
	color_map_size =
			le_short(header.color_map_length) * (header.color_map_depth/8);
	if(color_map_size > 0) {
		fprintf(
				stderr,
				"%s is colormapped, cannot handle that.\n",
				filename
		);
		fclose(f);
		return NULL;
	}

	// Set return width/height values and calculate image size.
	*width = le_short(header.width);
	*height = le_short(header.height);
	pixels_size = *width * *height * (header.bits_per_pixel / 8);
	pixels = malloc(pixels_size);

	// Read image.
	read = fread(pixels, 1, pixels_size, f);
	if(read != pixels_size) {
		fprintf(stderr, "%s has incomplete image\n", filename);
		fclose(f);
		free(pixels);
		return NULL;
	}

	return pixels;
}

// Load a texture from a TGA file.
GLuint loadTexture(const char *filename) {

	// Load tga data into buffer.
	int width, height, alpha;
	unsigned char* pixels;

	pixels = (unsigned char*)readTga(filename, &width, &height, &alpha);
	if(pixels == 0) {
		fprintf(stderr, "Image loading failed: %s\n", filename);
		return 0;
	}

	// Generate texture, bind as active texture.
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Load pixels from buffer into texture.
	glTexImage2D(
			GL_TEXTURE_2D,
			0,
			alpha == 1 ? GL_RGBA8 : GL_RGB8,
			width,
			height,
			0,
			alpha == 1 ? GL_BGRA : GL_BGR,
			GL_UNSIGNED_BYTE,
			pixels
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Release buffer.
	free(pixels);

	return texture;
}


// Load a texture from a TGA file.
GLuint genFloatTexture(float *data, int width, int height) {
	// Generate texture, bind as active texture.
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	// Load pixels from buffer into texture.
	glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_R32F,
			width,
			height,
			0,
			GL_RED,
			GL_FLOAT,
			data
	);

	return texture;
}

uint8_t* readFileBytes(const char *name)  {
    FILE *fl = fopen(name, "r");
    fseek(fl, 0, SEEK_END);
    long len = ftell(fl);
    uint8_t *ret = (uint8_t*)malloc(len);
    fseek(fl, 0, SEEK_SET);
    fread(ret, 1, len, fl);
    fclose(fl);
    return ret;
}

float* loadPGM(const char* fileName, int w, int h) {
    uint8_t* inputData = readFileBytes(fileName);

    // Remove header
    int pos = 0;
    while(inputData[pos] != '\n') {
        pos++;
    }
    pos++;

    // Remove width
    while(inputData[pos] != '\n') {
        pos++;
    }
    pos++;

    // Remove height
    while(inputData[pos] != '\n') {
        pos++;
    }
    pos++;

    // Remove rest
    while(inputData[pos] != '\n') {
        pos++;
    }
    pos++;

    float* dataFloats = (float*)malloc(sizeof(float) * w * h);
    float minv = 100000000000000.0f;
    float maxv = 0.0f;

    for(int i = 0; i < w * h; i++) {
        uint8_t pixelData[2];
        pixelData[0] = inputData[2*i+1 + pos];
        pixelData[1] = inputData[2*i + pos];
        uint16_t* pixelDataProper = (uint16_t*)pixelData;
        dataFloats[i] = (float)pixelDataProper[0];
        minv = fmin(minv, dataFloats[i]);
        maxv = fmax(maxv, dataFloats[i]);
    }

    // Normalize
    for(int i = 0; i < w * h; i++) {
        dataFloats[i] = (dataFloats[i] - minv) / (maxv - minv);
    }

    free(inputData);

    return dataFloats;
}

/*	EtcCompressMain.cpp
	Copyright 2015-2016 Fabian Herb

	This file is part of Molecular Engine.
*/

#include "3rdparty/etc/rg_etc1.h"
#include "3rdparty/etc/etc2_encoder.h"
#include <molecular/util/DdsFile.h>
#include <molecular/util/TaskDispatcher.h>
#include <molecular/util/KtxFile.h>
#include <molecular/gfx/opengl/GlConstants.h>
#include <molecular/util/CommandLineParser.h>
#include <molecular/util/StringUtils.h>

using namespace molecular::gfx;
using namespace molecular::util;

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <iostream>
#include <cmath>
#include <algorithm>

uint32_t GetPixel(const uint8_t* data, int width, int components, int x, int y)
{
	uint32_t out[4];
	out[0] = data[(y * width + x) * components];
	out[1] = data[(y * width + x) * components + 1];
	out[2] = data[(y * width + x) * components + 2];
	out[3] = components == 4 ? data[(y * width + x) * components + 3] : 255;
	return (out[3] << 24) | (out[2] << 16) | (out[1] << 8) | (out[0] << 0);
}

uint32_t GetPixelClamped(const uint8_t* data, int width, int height, int components, int x, int y)
{
	if (x >= width)
		x = width - 1;
	if (y >= height)
		y = height - 1;
	return GetPixel(data, width, components, x, y);
}

std::vector<uint8_t> CompressImage(TaskDispatcher& queue, const uint8_t* image, int width, int height, int components, rg_etc1::etc1_pack_params params, bool useEtc2, bool alpha)
{
	const size_t blockSize = (useEtc2 && alpha ? 16 : 8);

	TaskDispatcher::FinishFlag finishFlag;

	std::vector<uint8_t> output( ((width + 3) / 4) * ((height + 3) / 4) * blockSize );
	uint8_t* destination = output.data();
	for(int y = 0; y < height; y += 4)
	{
		for(int x = 0; x < width; x += 4)
		{
			auto task = [=]{
				uint32_t pixelBlock[16];
				// Copy pixel block:
				for(int blockY = 0; blockY < 4; ++blockY)
				{
					for(int blockX = 0; blockX < 4; ++blockX)
					{
						pixelBlock[blockY * 4 + blockX] = GetPixelClamped(image, width, height, components, x + blockX, y + blockY);
					}
				}

				// Compress block:
				if(useEtc2)
				{
					if(alpha)
						etc2_rgba8_block_pack(destination, pixelBlock, params);
					else
						etc2_rgb8_block_pack(destination, pixelBlock, params);
				}
				else
				{
					rg_etc1::etc1_pack_params params2 = params;
					rg_etc1::pack_etc1_block(destination, pixelBlock, params2);
				}
			};
			queue.EnqueueTask(task, finishFlag);
			destination += blockSize;
		}
	}
	queue.WaitUntilFinished(finishFlag);
	return output;
}

void WriteDdsHeader(FILE* file, int width, int height, bool alpha, int mipmapCount)
{
	uint32_t magic = 0x20534444;
	DdsFile::Header header;
	header.size = 124;
	header.flags = DdsFile::Header::kCaps | DdsFile::Header::kPixelformat | DdsFile::Header::kWidth | DdsFile::Header::kHeight;
	header.height = height;
	header.width = width;
	header.pitchOrLinearSize = 0;
	header.depth = 1;
	header.mipMapCount = mipmapCount;
	for(int i = 0; i < 11; ++i)
		header.reserved1[i] = 0;
	header.pixelFormat.size = 32;
	header.pixelFormat.flags = DdsFile::PixelFormatHeader::kFourCc;
	header.pixelFormat.fourCc = alpha ? DdsFile::PixelFormatHeader::kEtc2Alpha : DdsFile::PixelFormatHeader::kEtc2;
	header.pixelFormat.rgbBitCount = 0;
	header.pixelFormat.rBitMask = 0;
	header.pixelFormat.gBitMask = 0;
	header.pixelFormat.bBitMask = 0;
	header.pixelFormat.aBitMask = 0;
	header.caps = DdsFile::Header::kTexture;
	header.caps2 = 0;
	header.caps3 = 0;
	header.caps4 = 0;
	header.reserved2 = 0;
	if(fwrite(&magic, 4, 1, file) != 1 || fwrite(&header, 1, 124, file) != 124)
	{
		std::cerr << "Error writing file header: " << StringUtils::StrError(errno) << std::endl;
		fclose(file);
		exit(EXIT_FAILURE);
	}
}

void WriteKtxHeader(FILE* file, int width, int height, bool alpha, int mipmapCount)
{
	KtxFile::Header header;
	memcpy(header.identifier, KtxFile::Header::kKtxIdentifier, 12);
	header.endianness = 0x04030201;
	header.glType = 0;
	header.glTypeSize = 1;
	header.glFormat = 0;
	if(alpha)
		header.glInternalFormat = GlConstants::COMPRESSED_RGBA8_ETC2_EAC;
	else
		header.glInternalFormat = GlConstants::COMPRESSED_RGB8_ETC2;
	header.glBaseInternalFormat = 0;
	header.pixelWidth = width;
	header.pixelHeight = height;
	header.pixelDepth = 1;
	header.numberOfArrayElements = 1;
	header.numberOfFaces = 1;
	header.numberOfMipmapLevels = mipmapCount;
	header.bytesOfKeyValueData = 0;
	if(fwrite(&header, 1, sizeof(KtxFile::Header), file) != sizeof(KtxFile::Header))
	{
		std::cerr << "Error writing file header: " << StringUtils::StrError(errno) << std::endl;
		fclose(file);
		exit(EXIT_FAILURE);
	}
}

void WriteData(FILE* file, std::vector<uint8_t>& data, bool writeSize)
{
	if(writeSize)
	{
		uint32_t size = data.size();
		if(fwrite(&size, 1, 4, file) != 4)
		{
			std::cerr << "Error writing image size: " << strerror(errno) << std::endl;
			fclose(file);
			exit(EXIT_FAILURE);
		}
	}
	if(fwrite(data.data(), 1, data.size(), file) != data.size())
	{
		std::cerr << "Error writing image: " << strerror(errno) << std::endl;
		fclose(file);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char** argv)
{
	CommandLineParser parser;
	CommandLineParser::PositionalArg<std::string> inputFile(parser, "input file", "Input file");
	CommandLineParser::PositionalArg<std::string> outputFile(parser, "output file", "Output file");
	CommandLineParser::Flag useEtc1Flag(parser, "use-etc1", "Use ETC1.");
	CommandLineParser::Flag useEtc2Flag(parser, "use-etc2", "Use ETC2. This is the default.");

	try
	{
		parser.Parse(argc, argv);
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		parser.PrintHelp("etccompress");
		return EXIT_FAILURE;
	}

	if(useEtc1Flag && useEtc2Flag)
	{
		std::cerr << "Specify only one compression algorithm!\n";
		return EXIT_FAILURE;
	}

	bool useEtc2 = true;
	if(useEtc1Flag)
		useEtc2 = false;

	bool alpha = false; // Only possible with ETC2
	bool writeKtx = false;

	if(StringUtils::EndsWith(*outputFile, ".ktx"))
		writeKtx = true;
	else if(!StringUtils::EndsWith(*outputFile, ".dds"))
	{
		std::cerr << "Unknown output format" << std::endl;
		return EXIT_FAILURE;
	}

	TaskDispatcher queue;

	int width = 0, height = 0, components = 0;
	uint8_t* image = stbi_load(inputFile->c_str(), &width, &height, &components, 0);
	if(!image)
	{
		std::cerr << stbi_failure_reason() << std::endl;
		return EXIT_FAILURE;
	}
	if(components == 3)
		alpha = false;
	else if(components != 4)
	{
		std::cerr << "Unsupported number of channels" << std::endl;
		return EXIT_FAILURE;
	}

	FILE* file = fopen(outputFile->c_str(), "wb");
	if(!file)
	{
		std::cerr << "Error open output file: " << StringUtils::StrError(errno) << std::endl;
		return 1;
	}

	int mipMapCount = std::log2(std::min(width, height)) + 1;
	if(writeKtx)
		WriteKtxHeader(file, width, height, alpha, mipMapCount);
	else
		WriteDdsHeader(file, width, height, alpha, mipMapCount);
	rg_etc1::pack_etc1_block_init();

	rg_etc1::etc1_pack_params params;
	std::vector<uint8_t> compressed = CompressImage(queue, image, width, height, components, params, useEtc2, alpha);
	WriteData(file, compressed, writeKtx);
	
	const int alphaChannel = components == 3 ? STBIR_ALPHA_CHANNEL_NONE : 3;
	std::vector<uint8_t> oldImage(image, image + (width * height * components));
	stbi_image_free(image);
	for(int i = 0; i < mipMapCount - 1; ++i)
	{
		int newWidth = width / 2;
		int newHeight = height / 2;
		std::vector<uint8_t> newImage(newWidth * newHeight * components);
		stbir_resize_uint8_srgb(oldImage.data(), width, height, width * components, newImage.data(), newWidth, newHeight, newWidth * components, components, alphaChannel, 0);

		std::vector<uint8_t> compressed = CompressImage(queue, newImage.data(), newWidth, newHeight, components, params, useEtc2, alpha);
		WriteData(file, compressed, writeKtx);

		width = newWidth;
		height = newHeight;
		std::swap(oldImage, newImage);
	}

	fclose(file);
	return 0;
}


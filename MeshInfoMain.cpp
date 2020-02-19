/*	MeshInfoMain.cpp

MIT License

Copyright (c) 2020 Fabian Herb

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <molecular/meshfile/MeshFile.h>
#include <molecular/util/Blob.h>
#include <molecular/util/Hash.h>
#include <iostream>

using namespace molecular::util;
using namespace molecular::meshfile;

/// Check if indices in the given buffer exceed the vertex count
template<typename T>
void CheckIndices(const void* buffer, unsigned int indexCount, unsigned int vertexCount)
{
	const T* data = static_cast<const T*>(buffer);
	for(unsigned int j = 0; j < indexCount; ++j)
	{
		if(data[j] >= vertexCount)
		{
			std::cout << "\tERROR: Invalid vertex referenced: " << unsigned(data[j]) << std::endl;
			break;
		}
	}
}

const char* HashToString(Hash hash)
{
	switch(hash)
	{
	case "vertexPrt0Attr"_H: return "vertexPrt0Attr";
	case "vertexPrt1Attr"_H: return "vertexPrt1Attr";
	case "vertexPrt2Attr"_H: return "vertexPrt2Attr";
	case "vertexNormalAttr"_H: return "vertexNormalAttr";
	case "vertexPositionAttr"_H: return "vertexPositionAttr";
	case "vertexUv0Attr"_H: return "vertexUv0Attr";
	case "vertexSkinJointsAttr"_H: return "vertexSkinJointsAttr";
	case "vertexSkinWeightsAttr"_H: return "vertexSkinWeightsAttr";
	default: return nullptr;
	}
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <mesh file>" << std::endl;
		return 1;
	}

	FILE* file = fopen(argv[1], "rb");
	if(!file)
	{
		std::cerr << "Could not open file: " << strerror(errno) << std::endl;
		return 1;
	}

	if(fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		std::cerr << "Could not seek to end of file" << std::endl;
		return 1;
	}

	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	Blob blob(size);
	size_t readSize = fread(blob.GetData(), 1, size, file);
	fclose(file);

	if(size != readSize)
	{
		std::cerr << "Could not read file" << std::endl;
		return 1;
	}

	MeshFile* meshFile = static_cast<MeshFile*>(blob.GetData());

	if(meshFile->magic != MeshFile::kMagic)
	{
		std::cerr << "Not a valid mesh file" << std::endl;
		return 1;
	}

	std::cout << "numBuffers: " << meshFile->numBuffers << std::endl;
	std::cout << "numVertexDataSets: " << meshFile->numVertexDataSets << std::endl;
	std::cout << "numIndexSpecs: " << meshFile->numIndexSpecs << std::endl;
	std::cout << "boundsMin: " << meshFile->boundsMin[0] << ", " << meshFile->boundsMin[1] << ", " << meshFile->boundsMin[2] << std::endl;
	std::cout << "boundsMax: " << meshFile->boundsMax[0] << ", " << meshFile->boundsMax[1] << ", " << meshFile->boundsMax[2] << std::endl;

	for(unsigned int i = 0; i < meshFile->numBuffers; ++i)
	{
		std::cout << "Buffer " << i << ": " << std::endl;
		auto& buffer = meshFile->GetBuffer(i);
		std::cout << "\tsize: " << buffer.size << std::endl;
		std::cout << "\ttype: " << buffer.type << std::endl;
		std::cout << "\toffset: " << buffer.offset << std::endl;
		std::cout << "\treserved: " << buffer.reserved << std::endl;
		if(i > 0)
		{
			auto& prevBuffer = meshFile->GetBuffer(i - 1);
			if(prevBuffer.offset + prevBuffer.size > buffer.offset)
				std::cout << "\tERROR: Overlapping with previous buffer\n";
		}
	}

	for(unsigned int i = 0; i < meshFile->numVertexDataSets; ++i)
	{
		std::cout << "Vertex Dataset " << i << ": " << std::endl;
		const MeshFile::VertexDataSet& dataSet = meshFile->GetVertexDataSet(i);
		std::cout << "\tnumVertices: " << dataSet.numVertices << std::endl;
		for(unsigned int j = 0; j < dataSet.numVertexSpecs; ++j)
		{
			std::cout << "\tVertex Spec " << j << std::endl;
			const VertexAttributeInfo& spec = meshFile->GetVertexSpec(i, j);
			std::cout << "\t\tsemantic: ";
			const char* name = HashToString(spec.semantic);
			if(name)
				std::cout << name << std::endl;
			else
				std::cout << std::hex << spec.semantic << std::dec << std::endl;

			std::cout << "\t\ttype: " << spec.type << std::endl;
			std::cout << "\t\tcomponents: " << spec.components << std::endl;
			std::cout << "\t\toffset: " << spec.offset << std::endl;
			std::cout << "\t\tstride: " << spec.stride << std::endl;
			std::cout << "\t\tbuffer: " << spec.buffer << std::endl;
			std::cout << "\t\tnormalized: " << spec.normalized << std::endl;
		}
	}

	for(unsigned int i = 0; i < meshFile->numIndexSpecs; ++i)
	{
		std::cout << "Index Spec " << i << ": " << std::endl;
		const IndexBufferInfo& spec = meshFile->GetIndexSpec(i);
		std::cout << "\tmode: " << spec.mode << std::endl;
		std::cout << "\ttype: " << spec.type << std::endl;
		std::cout << "\tbuffer: " << spec.buffer << std::endl;
		std::cout << "\toffset: " << spec.offset << std::endl;
		std::cout << "\tcount: " << spec.count << std::endl;
		std::cout << "\tvertexDataSet: " << spec.vertexDataSet << std::endl;
		std::cout << "\tmaterial: " << spec.material << std::endl;

		if(spec.buffer >= meshFile->numBuffers)
		{
			std::cout << "ERROR: Invalid buffer!" << std::endl;
			continue;
		}
		const MeshFile::Buffer& buffer = meshFile->GetBuffer(spec.buffer);
		if(buffer.type != MeshFile::Buffer::Type::kIndex)
		{
			std::cout << "ERROR: Buffer " << spec.buffer << " not an index buffer!" << std::endl;
			continue;
		}
		if(spec.count + spec.offset > buffer.size)
		{
			std::cout << "\tERROR: Buffer too small" << std::endl;
			continue;
		}
		auto numVertices = meshFile->GetVertexDataSet(i).numVertices;
		const void* bufferData = meshFile->GetBufferData(spec.buffer);
		if(spec.type == IndexBufferInfo::Type::kUInt8)
			CheckIndices<uint8_t>(bufferData, spec.count, numVertices);
		else if(spec.type == IndexBufferInfo::Type::kUInt16)
			CheckIndices<uint16_t>(bufferData, spec.count, numVertices);
		else if(spec.type == IndexBufferInfo::Type::kUInt32)
			CheckIndices<uint32_t>(bufferData, spec.count, numVertices);
	}
}

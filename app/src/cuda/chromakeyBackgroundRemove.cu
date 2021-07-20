#include "cudaUtils.cuh"
#include <stdio.h>
#include "../InputCamera/InputBase.h"

__global__ void chromaKeyBackgroundRemove_kernel(
    unsigned char* colorRaw, cudaSurfaceObject_t output, unsigned int w, unsigned int h, glm::vec3 chromakey,float threshold)
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    unsigned int index = (h-1-y) * w + x;

    glm::vec3 color = glm::vec3(
        colorRaw[index * INPUT_COLOR_CHANNEL + 2],
        colorRaw[index * INPUT_COLOR_CHANNEL + 1],
        colorRaw[index * INPUT_COLOR_CHANNEL + 0]
    );

    uchar4 pixel = {
        colorRaw[index * INPUT_COLOR_CHANNEL + 2],
        colorRaw[index * INPUT_COLOR_CHANNEL + 1],
        colorRaw[index * INPUT_COLOR_CHANNEL + 0],
        glm::distance(color,chromakey)> threshold ?255:0
    };
    //Write the new pixel color to the 
    surf2Dwrite(pixel, output, x * sizeof(uchar4), y);
}

__global__ void chromaKeyBackgroundRemove_kernel(
    unsigned char* colorRaw, cudaSurfaceObject_t output, unsigned int w, unsigned int h, glm::vec3 chromakey,float threshold, glm::vec3 replaceColor)
{
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    unsigned int index = (h-1-y) * w + x;

    glm::vec3 color = glm::vec3(
        colorRaw[index * INPUT_COLOR_CHANNEL + 2],
        colorRaw[index * INPUT_COLOR_CHANNEL + 1],
        colorRaw[index * INPUT_COLOR_CHANNEL + 0]
    );

    uchar4 pixel = {
        replaceColor.x,
        replaceColor.y,
        replaceColor.z,
        glm::distance(color,chromakey)> threshold ?255:0
    };
    //Write the new pixel color to the 
    surf2Dwrite(pixel, output, x * sizeof(uchar4), y);
}

void launch_kernel(
    unsigned char* colorRaw, cudaSurfaceObject_t output, unsigned int mesh_width, unsigned int mesh_height, glm::vec3 color, float threshold)
{
    // execute the kernel
    dim3 block(8, 8, 1);
    dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);
    chromaKeyBackgroundRemove_kernel << < grid, block >> > (colorRaw, output, mesh_width, mesh_height, color, threshold);
}

void launch_kernel(
    unsigned char* colorRaw, cudaSurfaceObject_t output, unsigned int mesh_width, unsigned int mesh_height, glm::vec3 color, float threshold, glm::vec3 replaceColor)
{
    // execute the kernel
    dim3 block(8, 8, 1);
    dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);
    chromaKeyBackgroundRemove_kernel << < grid, block >> > (colorRaw, output, mesh_width, mesh_height, color, threshold, replaceColor);
}

void CudaAlogrithm::chromaKeyBackgroundRemove(
    cudaGraphicsResource_t* cudaTexture,
    unsigned char* colorRaw, unsigned int w, unsigned int h,glm::vec3 color, float threshold)
{
    cudaArray* texture_ptr;

    cudaGraphicsMapResources(1, cudaTexture, 0);
    cudaGraphicsSubResourceGetMappedArray(&texture_ptr, *cudaTexture, 0, 0);


    cudaResourceDesc resDesc;
    memset(&resDesc, 0, sizeof(resDesc));
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = texture_ptr;
    cudaSurfaceObject_t surfObject;
    cudaCreateSurfaceObject(&surfObject, &resDesc);
    // You now have a CUDA Surface object that refers to the GL texture.
    // Write to the Surface using CUDA.
    launch_kernel(colorRaw, surfObject, w, h, color, threshold);

    // We're not going to use this Surface object again.  We'll make a new one next frame.
    cudaDestroySurfaceObject(surfObject);
    cudaGraphicsUnmapResources(1, cudaTexture, 0);
}

void CudaAlogrithm::chromaKeyBackgroundRemove(
    cudaGraphicsResource_t* cudaTexture,
    unsigned char* colorRaw, unsigned int w, unsigned int h, glm::vec3 color, float threshold,glm::vec3 replaceColor)
{
    cudaArray* texture_ptr;

    cudaGraphicsMapResources(1, cudaTexture, 0);
    cudaGraphicsSubResourceGetMappedArray(&texture_ptr, *cudaTexture, 0, 0);


    cudaResourceDesc resDesc;
    memset(&resDesc, 0, sizeof(resDesc));
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = texture_ptr;
    cudaSurfaceObject_t surfObject;
    cudaCreateSurfaceObject(&surfObject, &resDesc);
    // You now have a CUDA Surface object that refers to the GL texture.
    // Write to the Surface using CUDA.
    launch_kernel(colorRaw, surfObject, w, h, color, threshold, replaceColor);

    // We're not going to use this Surface object again.  We'll make a new one next frame.
    cudaDestroySurfaceObject(surfObject);
    cudaGraphicsUnmapResources(1, cudaTexture, 0);
}
#include "./CudaOpenGLUtils.h"

CudaGLDepth2PlaneMesh::CudaGLDepth2PlaneMesh(int w, int h,int colorChannel) {
	width = w;
	height = h;
	glGenVertexArrays(1, &vao);
	CudaOpenGL::createBufferObject(&vbo, &cuda_vbo_resource, cudaGraphicsMapFlagsNone, width * height * ATTRIBUTESIZE * sizeof(float), GL_ARRAY_BUFFER);
	CudaOpenGL::createBufferObject(&ibo, &cuda_ibo_resource, cudaGraphicsMapFlagsNone, width * height * 2 * 3 * sizeof(sizeof(unsigned int)), GL_ELEMENT_ARRAY_BUFFER);
	cudaMalloc((void**)&cudaIndicesCount, sizeof(int));
	cudaMalloc((void**)&cudaDepthData, width * height * sizeof(uint16_t));
	cudaMalloc((void**)&cudaColorData, width * height * colorChannel * sizeof(unsigned char));
};

void CudaGLDepth2PlaneMesh::destory() {
	glDeleteVertexArrays(1, &vao);
	CudaOpenGL::deleteVBO(&vbo, cuda_vbo_resource);
	CudaOpenGL::deleteVBO(&ibo, cuda_ibo_resource);
	cudaFree(cudaIndicesCount);
	cudaFree(cudaDepthData);
	cudaFree(cudaColorData);
}

void CudaGLDepth2PlaneMesh::render(std::function<void(GLuint& vao, int& count)>callback) {
	glBindVertexArray(vao);
	// generate and bind the buffer object
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// set up generic attrib pointers
	glEnableVertexAttribArray(ATTRIBUTEINDEX_VERTEX);
	glVertexAttribPointer(ATTRIBUTEINDEX_VERTEX, ATTRIBUTESIZE_VERTEX, GL_FLOAT, GL_FALSE, ATTRIBUTESIZE * sizeof(GLfloat), (char*)0 + 0 * sizeof(GLfloat));
	glEnableVertexAttribArray(ATTRIBUTEINDEX_UV);
	glVertexAttribPointer(ATTRIBUTEINDEX_UV, ATTRIBUTESIZE_UV, GL_FLOAT, GL_FALSE, ATTRIBUTESIZE * sizeof(GLfloat), (char*)0 + ATTRIBUTE_OFFSET_UV * sizeof(GLfloat));
	glEnableVertexAttribArray(ATTRIBUTEINDEX_NORAML);
	glVertexAttribPointer(ATTRIBUTEINDEX_NORAML, ATTRIBUTESIZE_NORAML, GL_FLOAT, GL_FALSE, ATTRIBUTESIZE * sizeof(GLfloat), (char*)0 + ATTRIBUTE_OFFSET_NORMAL * sizeof(GLfloat));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	int count = 0;
	cudaMemcpy(&count, cudaIndicesCount, sizeof(int), cudaMemcpyDeviceToHost);
	callback(vao, count);
}